package providers

import (
	"errors"
	"strings"
	"testing"
	"time"
)

// budgetTestZone has whole-hour offsets, so the hourly window starts on the hour.
var budgetTestZone = time.FixedZone("UTC+7", 7*3600)

func newTestBudget(perHour, perDay int, now *time.Time) *LLMBudget {
	b := NewLLMBudget(perHour, perDay)
	b.nowFunc = func() time.Time { return *now }
	return b
}

func TestLLMBudget_NilAndZeroAllowEverything(t *testing.T) {
	var nilBudget *LLMBudget
	if err := nilBudget.Acquire(); err != nil {
		t.Fatalf("nil budget Acquire() = %v", err)
	}
	if nilBudget.Enabled() {
		t.Fatal("nil budget must not be enabled")
	}
	b := NewLLMBudget(0, 0)
	for i := 0; i < 1000; i++ {
		if err := b.Acquire(); err != nil {
			t.Fatalf("unlimited budget Acquire() #%d = %v", i, err)
		}
	}
}

func TestLLMBudget_HourlyLimitResetsAtTopOfHour(t *testing.T) {
	now := time.Date(2026, 10, 13, 14, 20, 0, 0, budgetTestZone)
	b := newTestBudget(2, 0, &now)

	for i := 0; i < 2; i++ {
		if err := b.Acquire(); err != nil {
			t.Fatalf("Acquire() #%d = %v", i, err)
		}
	}
	err := b.Acquire()
	var exceeded *LLMBudgetExceededError
	if !errors.As(err, &exceeded) {
		t.Fatalf("Acquire() = %v, want *LLMBudgetExceededError", err)
	}
	if exceeded.Window != "hourly" || exceeded.Limit != 2 || exceeded.Used != 2 {
		t.Fatalf("exceeded = %+v", exceeded)
	}
	if want := time.Date(2026, 10, 13, 15, 0, 0, 0, budgetTestZone); !exceeded.ResetAt.Equal(want) {
		t.Fatalf("ResetAt = %v, want %v", exceeded.ResetAt, want)
	}
	if !strings.Contains(err.Error(), "hourly") || !strings.Contains(err.Error(), "15:00") {
		t.Fatalf("message %q should name the window and the reset time", err.Error())
	}
	// A refused call is not counted.
	if got := b.Snapshot().CallsThisHour; got != 2 {
		t.Fatalf("CallsThisHour = %d, want 2", got)
	}

	now = time.Date(2026, 10, 13, 15, 0, 0, 0, budgetTestZone)
	if err := b.Acquire(); err != nil {
		t.Fatalf("Acquire() after the hour rolled = %v", err)
	}
	if got := b.Snapshot().CallsToday; got != 3 {
		t.Fatalf("CallsToday = %d, want 3", got)
	}
}

func TestLLMBudget_DailyLimitWinsAndResetsAtMidnight(t *testing.T) {
	now := time.Date(2026, 10, 13, 22, 59, 0, 0, budgetTestZone)
	b := newTestBudget(2, 3, &now)

	mustAcquire := func() {
		t.Helper()
		if err := b.Acquire(); err != nil {
			t.Fatalf("Acquire() = %v", err)
		}
	}
	mustAcquire()
	mustAcquire()
	now = now.Add(2 * time.Minute) // 23:01, new hour
	mustAcquire()

	err := b.Acquire()
	var exceeded *LLMBudgetExceededError
	if !errors.As(err, &exceeded) || exceeded.Window != "daily" {
		t.Fatalf("Acquire() = %v, want daily budget error", err)
	}
	if want := time.Date(2026, 10, 14, 0, 0, 0, 0, budgetTestZone); !exceeded.ResetAt.Equal(want) {
		t.Fatalf("ResetAt = %v, want %v", exceeded.ResetAt, want)
	}
	if msg := err.Error(); !strings.Contains(msg, "tomorrow at 00:00") || !strings.Contains(msg, "2026-10-14") {
		t.Fatalf("daily message %q should say tomorrow and give the date", msg)
	}
	if err := b.Check(); err == nil {
		t.Fatal("Check() = nil while the daily budget is used up")
	}

	now = time.Date(2026, 10, 14, 0, 0, 1, 0, budgetTestZone)
	if err := b.Check(); err != nil {
		t.Fatalf("Check() after midnight = %v", err)
	}
	mustAcquire()
}

func TestLLMBudget_CheckDoesNotCount(t *testing.T) {
	now := time.Date(2026, 10, 13, 9, 0, 0, 0, budgetTestZone)
	b := newTestBudget(1, 0, &now)
	for i := 0; i < 5; i++ {
		if err := b.Check(); err != nil {
			t.Fatalf("Check() = %v", err)
		}
	}
	if err := b.Acquire(); err != nil {
		t.Fatalf("Acquire() = %v", err)
	}
	if err := b.Check(); err == nil {
		t.Fatal("Check() = nil after the only call was used")
	}
}

func TestLLMBudget_SetLimitsKeepsCounters(t *testing.T) {
	now := time.Date(2026, 10, 13, 9, 0, 0, 0, budgetTestZone)
	b := newTestBudget(1, 0, &now)
	if err := b.Acquire(); err != nil {
		t.Fatalf("Acquire() = %v", err)
	}
	b.SetLimits(3, 10)
	s := b.Snapshot()
	if s.CallsThisHour != 1 || s.MaxCallsPerHour != 3 || s.MaxCallsPerDay != 10 {
		t.Fatalf("Snapshot() = %+v", s)
	}
	b.SetLimits(0, 0)
	if b.Enabled() {
		t.Fatal("budget with zero limits must not be enabled")
	}
}

func TestLLMBudget_DSTFallBackKeepsHourlyWindowsApart(t *testing.T) {
	ny, err := time.LoadLocation("America/New_York")
	if err != nil {
		t.Fatalf("load America/New_York: %v", err)
	}
	// 2026-11-01: clocks go back from 02:00 EDT to 01:00 EST, so 01:xx happens twice.
	firstOne := time.Date(2026, 11, 1, 5, 30, 0, 0, time.UTC).In(ny) // 01:30 EDT
	secondOne := firstOne.Add(time.Hour)                             // 01:30 EST
	if firstOne.Hour() != 1 || secondOne.Hour() != 1 {
		t.Fatalf("test setup: hours %d and %d, want both 1", firstOne.Hour(), secondOne.Hour())
	}

	now := firstOne
	b := newTestBudget(1, 3, &now)
	if err := b.Acquire(); err != nil {
		t.Fatalf("Acquire() in the first 01:xx = %v", err)
	}
	if err := b.Acquire(); err == nil {
		t.Fatal("second call in the same hour should hit the hourly budget")
	}

	now = secondOne // same wall-clock hour, a new absolute hour
	if err := b.Acquire(); err != nil {
		t.Fatalf("Acquire() in the repeated 01:xx = %v, want a fresh hourly window", err)
	}
	s := b.Snapshot()
	if s.CallsToday != 2 {
		t.Fatalf("CallsToday = %d, want 2 (both hours are the same day)", s.CallsToday)
	}
	wantDayEnd := time.Date(2026, 11, 2, 0, 0, 0, 0, ny)
	if !s.DayResetsAt.Equal(wantDayEnd) {
		t.Fatalf("DayResetsAt = %v, want %v (the 25-hour day is one window)", s.DayResetsAt, wantDayEnd)
	}

	now = time.Date(2026, 11, 1, 23, 59, 0, 0, ny)
	if err := b.Acquire(); err != nil {
		t.Fatalf("Acquire() late in the long day = %v", err)
	}
	if err := b.Acquire(); err == nil || !strings.Contains(err.Error(), "daily") {
		t.Fatalf("Acquire() over the daily limit = %v, want the daily error", err)
	}
	now = wantDayEnd
	if err := b.Acquire(); err != nil {
		t.Fatalf("Acquire() at the next midnight = %v", err)
	}
}

func TestLLMBudget_ClockMovedBackStartsNewWindow(t *testing.T) {
	now := time.Date(2026, 10, 13, 10, 0, 0, 0, time.UTC)
	b := newTestBudget(1, 0, &now)
	if err := b.Acquire(); err != nil {
		t.Fatal(err)
	}
	now = now.Add(-2 * time.Hour)
	if err := b.Acquire(); err != nil {
		t.Fatalf("Acquire() after the clock moved back = %v, want a fresh window", err)
	}
}
