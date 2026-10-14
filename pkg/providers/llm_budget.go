package providers

import (
	"errors"
	"fmt"
	"sync"
	"time"
)

// LLMBudget caps how many LLM calls are made per clock hour and per calendar
// day, across all models. It sits beside the per-model rpm token bucket:
// RateLimiterRegistry smooths request rate, LLMBudget bounds the total.
//
// Windows are fixed and keyed by absolute time, so a daylight-saving change
// neither merges nor skips a window: the hourly window is the absolute hour
// (now.Truncate(time.Hour), on the hour in whole-hour zones) and the daily
// window ends at the next local midnight, computed from the calendar date
// (a 23- or 25-hour day stays one day). Wall-clock time is only used to tell
// the user when the budget resets. A fixed window can let up to twice the
// hourly limit through around a boundary (end of one hour, start of the
// next); that is accepted in exchange for a reset time the user can be told
// plainly.
//
// Counters live in memory only and restart from zero when the process
// restarts. A nil *LLMBudget allows everything. Thread-safe.
type LLMBudget struct {
	mu        sync.Mutex
	maxHour   int
	maxDay    int
	hourEnd   time.Time // end of the current hourly window (exclusive)
	dayEnd    time.Time // end of the current daily window: next local midnight
	hourCount int
	dayCount  int
	nowFunc   func() time.Time // for testing
}

// LLMBudgetStatus is a point-in-time view of the budget for diagnostics.
type LLMBudgetStatus struct {
	MaxCallsPerHour int       `json:"max_calls_per_hour"`
	MaxCallsPerDay  int       `json:"max_calls_per_day"`
	CallsThisHour   int       `json:"calls_this_hour"`
	CallsToday      int       `json:"calls_today"`
	HourResetsAt    time.Time `json:"hour_resets_at"`
	DayResetsAt     time.Time `json:"day_resets_at"`
}

// LLMBudgetExceededError reports that a budget window is used up.
type LLMBudgetExceededError struct {
	Window  string // "hourly" or "daily"
	Limit   int
	Used    int
	ResetAt time.Time
}

func (e *LLMBudgetExceededError) Error() string {
	reset := "at " + e.ResetAt.Format("15:04") + " (local time)"
	if e.Window == "daily" {
		reset = "tomorrow at " + e.ResetAt.Format("15:04") + " (local time, " + e.ResetAt.Format("2006-01-02") + ")"
	}
	return fmt.Sprintf(
		"The %s LLM call budget is used up (%d of %d calls), so I am not calling the model now. It resets %s.",
		e.Window, e.Used, e.Limit, reset,
	)
}

func isLLMBudgetExceeded(err error) bool {
	var exceeded *LLMBudgetExceededError
	return errors.As(err, &exceeded)
}

// NewLLMBudget creates a budget; 0 (or less) for a limit means no limit.
func NewLLMBudget(maxCallsPerHour, maxCallsPerDay int) *LLMBudget {
	return &LLMBudget{
		maxHour: max(maxCallsPerHour, 0),
		maxDay:  max(maxCallsPerDay, 0),
		nowFunc: time.Now,
	}
}

// SetLimits changes the limits (config reload) and keeps the counters.
func (b *LLMBudget) SetLimits(maxCallsPerHour, maxCallsPerDay int) {
	if b == nil {
		return
	}
	b.mu.Lock()
	defer b.mu.Unlock()
	b.maxHour = max(maxCallsPerHour, 0)
	b.maxDay = max(maxCallsPerDay, 0)
}

// Enabled reports whether any limit is set.
func (b *LLMBudget) Enabled() bool {
	if b == nil {
		return false
	}
	b.mu.Lock()
	defer b.mu.Unlock()
	return b.maxHour > 0 || b.maxDay > 0
}

// rollLocked starts a new window when the clock has moved past the current one.
func (b *LLMBudget) rollLocked(now time.Time) {
	if hourEnd := now.Truncate(time.Hour).Add(time.Hour); !hourEnd.Equal(b.hourEnd) {
		b.hourEnd = hourEnd
		b.hourCount = 0
	}
	if dayEnd := nextLocalMidnight(now); !dayEnd.Equal(b.dayEnd) {
		b.dayEnd = dayEnd
		b.dayCount = 0
	}
}

// nextLocalMidnight is the first instant of the calendar day after now's, in
// now's location. time.Date normalizes the day overflow and a midnight that a
// DST change skips.
func nextLocalMidnight(now time.Time) time.Time {
	y, m, d := now.Date()
	return time.Date(y, m, d+1, 0, 0, 0, 0, now.Location())
}

// exceededLocked returns the window that is used up, daily first because it
// resets later and is the one the user has to wait for.
func (b *LLMBudget) exceededLocked() *LLMBudgetExceededError {
	if b.maxDay > 0 && b.dayCount >= b.maxDay {
		return &LLMBudgetExceededError{
			Window: "daily", Limit: b.maxDay, Used: b.dayCount, ResetAt: b.dayEnd,
		}
	}
	if b.maxHour > 0 && b.hourCount >= b.maxHour {
		return &LLMBudgetExceededError{
			Window: "hourly", Limit: b.maxHour, Used: b.hourCount, ResetAt: b.hourEnd,
		}
	}
	return nil
}

// Acquire counts one LLM call, or returns *LLMBudgetExceededError without
// counting when a window is used up.
func (b *LLMBudget) Acquire() error {
	if b == nil {
		return nil
	}
	b.mu.Lock()
	defer b.mu.Unlock()
	if b.maxHour <= 0 && b.maxDay <= 0 {
		return nil
	}
	b.rollLocked(b.nowFunc())
	if err := b.exceededLocked(); err != nil {
		return err
	}
	b.hourCount++
	b.dayCount++
	return nil
}

// Check reports whether a window is used up without counting a call. Used to
// skip whole background turns (cron, heartbeat) before they start.
func (b *LLMBudget) Check() error {
	if b == nil {
		return nil
	}
	b.mu.Lock()
	defer b.mu.Unlock()
	if b.maxHour <= 0 && b.maxDay <= 0 {
		return nil
	}
	b.rollLocked(b.nowFunc())
	if err := b.exceededLocked(); err != nil {
		return err
	}
	return nil
}

// Snapshot returns the current limits, counts and reset times.
func (b *LLMBudget) Snapshot() LLMBudgetStatus {
	if b == nil {
		return LLMBudgetStatus{}
	}
	b.mu.Lock()
	defer b.mu.Unlock()
	b.rollLocked(b.nowFunc())
	return LLMBudgetStatus{
		MaxCallsPerHour: b.maxHour,
		MaxCallsPerDay:  b.maxDay,
		CallsThisHour:   b.hourCount,
		CallsToday:      b.dayCount,
		HourResetsAt:    b.hourEnd,
		DayResetsAt:     b.dayEnd,
	}
}
