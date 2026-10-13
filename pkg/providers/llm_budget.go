package providers

import (
	"fmt"
	"sync"
	"time"
)

// LLMBudget caps how many LLM calls are made per clock hour and per calendar
// day, across all models. It sits beside the per-model rpm token bucket:
// RateLimiterRegistry smooths request rate, LLMBudget bounds the total.
//
// Windows are fixed and follow local wall-clock time: the hourly window
// resets at the top of each hour (14:00, 15:00, ...) and the daily window at
// local midnight. A fixed window can let up to twice the hourly limit through
// around a boundary (end of one hour, start of the next); that is accepted in
// exchange for a reset time the user can be told plainly.
//
// Counters live in memory only and restart from zero when the process
// restarts. A nil *LLMBudget allows everything. Thread-safe.
type LLMBudget struct {
	mu        sync.Mutex
	maxHour   int
	maxDay    int
	hourStart time.Time
	dayStart  time.Time
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
	return fmt.Sprintf(
		"The %s LLM call budget is used up (%d of %d calls), so I am not calling the model now. "+
			"It resets at %s (local time).",
		e.Window, e.Used, e.Limit, e.ResetAt.Format("15:04"),
	)
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
	hour := time.Date(now.Year(), now.Month(), now.Day(), now.Hour(), 0, 0, 0, now.Location())
	if !hour.Equal(b.hourStart) {
		b.hourStart = hour
		b.hourCount = 0
	}
	day := time.Date(now.Year(), now.Month(), now.Day(), 0, 0, 0, 0, now.Location())
	if !day.Equal(b.dayStart) {
		b.dayStart = day
		b.dayCount = 0
	}
}

// exceededLocked returns the window that is used up, daily first because it
// resets later and is the one the user has to wait for.
func (b *LLMBudget) exceededLocked() *LLMBudgetExceededError {
	if b.maxDay > 0 && b.dayCount >= b.maxDay {
		return &LLMBudgetExceededError{
			Window: "daily", Limit: b.maxDay, Used: b.dayCount, ResetAt: b.dayStart.AddDate(0, 0, 1),
		}
	}
	if b.maxHour > 0 && b.hourCount >= b.maxHour {
		return &LLMBudgetExceededError{
			Window: "hourly", Limit: b.maxHour, Used: b.hourCount, ResetAt: b.hourStart.Add(time.Hour),
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
		HourResetsAt:    b.hourStart.Add(time.Hour),
		DayResetsAt:     b.dayStart.AddDate(0, 0, 1),
	}
}
