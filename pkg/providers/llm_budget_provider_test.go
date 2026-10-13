package providers

import (
	"context"
	"errors"
	"testing"
	"time"
)

type budgetStubProvider struct{ calls int }

func (p *budgetStubProvider) Chat(
	context.Context, []Message, []ToolDefinition, string, map[string]any,
) (*LLMResponse, error) {
	p.calls++
	return &LLMResponse{Content: "ok"}, nil
}
func (p *budgetStubProvider) GetDefaultModel() string { return "stub" }

type budgetStreamStub struct{ budgetStubProvider }

func (p *budgetStreamStub) ChatStream(
	context.Context, []Message, []ToolDefinition, string, map[string]any, func(string),
) (*LLMResponse, error) {
	p.calls++
	return &LLMResponse{Content: "streamed"}, nil
}
func (p *budgetStreamStub) SupportsThinking() bool { return true }

func TestWithLLMBudget_KeepsOptionalInterfaces(t *testing.T) {
	b := NewLLMBudget(10, 0)

	plain := WithLLMBudget(&budgetStubProvider{}, b)
	if _, ok := plain.(StreamingProvider); ok {
		t.Fatal("wrapper of a non-streaming provider must not claim StreamingProvider")
	}
	if _, ok := plain.(StreamingEventProvider); ok {
		t.Fatal("wrapper of a non-streaming provider must not claim StreamingEventProvider")
	}
	if tc, ok := plain.(ThinkingCapable); ok && tc.SupportsThinking() {
		t.Fatal("wrapper must not report thinking the provider lacks")
	}

	inner := &budgetStreamStub{}
	streaming := WithLLMBudget(inner, b)
	sp, ok := streaming.(StreamingProvider)
	if !ok {
		t.Fatal("wrapper of a streaming provider lost StreamingProvider")
	}
	if tc, ok := streaming.(ThinkingCapable); !ok || !tc.SupportsThinking() {
		t.Fatal("wrapper lost SupportsThinking")
	}
	if _, err := sp.ChatStream(context.Background(), nil, nil, "m", nil, func(string) {}); err != nil {
		t.Fatal(err)
	}
	if b.Snapshot().CallsThisHour != 1 || inner.calls != 1 {
		t.Fatalf("a stream request must count once: budget %+v, inner calls %d", b.Snapshot(), inner.calls)
	}

	if again := WithLLMBudget(streaming, b); again != streaming {
		t.Fatal("wrapping twice with the same budget must return the same provider")
	}
	if UnwrapLLMBudget(streaming) != LLMProvider(inner) {
		t.Fatal("UnwrapLLMBudget did not return the wrapped provider")
	}
	if WithLLMBudget(inner, nil) != LLMProvider(inner) {
		t.Fatal("a nil budget must leave the provider unwrapped")
	}
}

func TestWithLLMBudget_RefusesWithoutCallingTheProvider(t *testing.T) {
	inner := &budgetStubProvider{}
	p := WithLLMBudget(inner, NewLLMBudget(1, 0))
	if _, err := p.Chat(context.Background(), nil, nil, "m", nil); err != nil {
		t.Fatal(err)
	}
	_, err := p.Chat(context.Background(), nil, nil, "m", nil)
	if !isLLMBudgetExceeded(err) {
		t.Fatalf("second Chat() error = %v, want the budget error", err)
	}
	if inner.calls != 1 {
		t.Fatalf("provider called %d times, want 1", inner.calls)
	}
}

func TestFallbackChain_EachCandidateAttemptCountsAndBudgetStopsTheChain(t *testing.T) {
	now := time.Date(2026, 10, 14, 9, 0, 0, 0, time.UTC)
	b := NewLLMBudget(2, 0)
	b.nowFunc = func() time.Time { return now }

	failing := WithLLMBudget(&failingStub{}, b)
	working := WithLLMBudget(&budgetStubProvider{}, b)
	byModel := map[string]LLMProvider{"a": failing, "b": working}
	candidates := []FallbackCandidate{{Provider: "openai", Model: "a"}, {Provider: "openai", Model: "b"}}
	run := func(ctx context.Context, c FallbackCandidate) (*LLMResponse, error) {
		return byModel[c.Model].Chat(ctx, nil, nil, c.Model, nil)
	}

	cooldown := NewCooldownTracker()
	chain := NewFallbackChain(cooldown, nil)
	if _, err := chain.ExecuteCandidate(context.Background(), candidates, run); err != nil {
		t.Fatalf("first chain run: %v", err)
	}
	if got := b.Snapshot().CallsThisHour; got != 2 {
		t.Fatalf("calls after one chain run with a fallback = %d, want 2 (one per attempt)", got)
	}

	cooldown2 := NewCooldownTracker()
	_, err := NewFallbackChain(cooldown2, nil).ExecuteCandidate(context.Background(), candidates, run)
	if !isLLMBudgetExceeded(err) {
		t.Fatalf("chain run over budget error = %v, want the budget error itself", err)
	}
	if len(cooldown2.Snapshot()) != 0 {
		t.Fatalf("a budget refusal must not put a model in cooldown: %+v", cooldown2.Snapshot())
	}
}

type failingStub struct{}

func (failingStub) Chat(context.Context, []Message, []ToolDefinition, string, map[string]any) (*LLMResponse, error) {
	return nil, errors.New("status: 429 rate limit")
}
func (failingStub) GetDefaultModel() string { return "failing" }
