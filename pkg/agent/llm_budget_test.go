package agent

import (
	"context"
	"errors"
	"strings"
	"sync/atomic"
	"testing"

	"github.com/letrongminh/neuroclaw/pkg/bus"
	"github.com/letrongminh/neuroclaw/pkg/config"
	"github.com/letrongminh/neuroclaw/pkg/providers"
)

type countingProvider struct {
	calls atomic.Int32
}

func (p *countingProvider) Chat(
	_ context.Context,
	_ []providers.Message,
	_ []providers.ToolDefinition,
	_ string,
	_ map[string]any,
) (*providers.LLMResponse, error) {
	p.calls.Add(1)
	return &providers.LLMResponse{Content: "ok"}, nil
}

func (p *countingProvider) GetDefaultModel() string { return "counting-model" }

func newBudgetTestLoop(t *testing.T, perHour int) (*AgentLoop, *countingProvider) {
	t.Helper()
	cfg := config.DefaultConfig()
	cfg.Agents.Defaults.Workspace = t.TempDir()
	cfg.Agents.Defaults.ModelName = "test-model"
	cfg.Agents.Defaults.LLMBudget.MaxCallsPerHour = perHour
	provider := &countingProvider{}
	al := NewAgentLoop(cfg, bus.NewMessageBus(), provider)
	t.Cleanup(al.Close)
	return al, provider
}

func TestLLMBudget_RefusesTurnPlainlyWithoutCallingTheModel(t *testing.T) {
	al, provider := newBudgetTestLoop(t, 1)
	ctx := context.Background()

	if _, err := al.ProcessDirect(ctx, "first", "cli:budget"); err != nil {
		t.Fatalf("first turn: %v", err)
	}
	if got := provider.calls.Load(); got != 1 {
		t.Fatalf("provider calls = %d, want 1", got)
	}

	_, err := al.ProcessDirect(ctx, "second", "cli:budget")
	var exceeded *providers.LLMBudgetExceededError
	if !errors.As(err, &exceeded) {
		t.Fatalf("second turn error = %v, want *LLMBudgetExceededError", err)
	}
	if got := provider.calls.Load(); got != 1 {
		t.Fatalf("provider calls after budget hit = %d, want still 1", got)
	}
	msg := formatProcessingError(err)
	if strings.HasPrefix(msg, "Error processing message") || !strings.Contains(msg, "hourly LLM call budget") ||
		!strings.Contains(msg, "resets at") {
		t.Fatalf("user message = %q, want the plain budget message", msg)
	}
}

func TestLLMBudget_HeartbeatSkippedWhenUsedUp(t *testing.T) {
	al, provider := newBudgetTestLoop(t, 1)
	ctx := context.Background()

	if _, err := al.ProcessDirect(ctx, "use the only call", "cli:budget"); err != nil {
		t.Fatalf("turn: %v", err)
	}
	_, err := al.ProcessHeartbeat(ctx, "check things", "", "")
	var exceeded *providers.LLMBudgetExceededError
	if !errors.As(err, &exceeded) {
		t.Fatalf("ProcessHeartbeat() error = %v, want budget error", err)
	}
	if got := provider.calls.Load(); got != 1 {
		t.Fatalf("provider calls = %d, want 1 (heartbeat skipped)", got)
	}
	if err := al.LLMBudgetBlocked(); err == nil {
		t.Fatal("LLMBudgetBlocked() = nil while the budget is used up")
	}
}

func TestLLMBudget_ReloadKeepsCountersAndAppliesNewLimits(t *testing.T) {
	al, _ := newBudgetTestLoop(t, 1)
	if _, err := al.ProcessDirect(context.Background(), "hi", "cli:budget"); err != nil {
		t.Fatalf("turn: %v", err)
	}
	cfg := config.DefaultConfig()
	cfg.Agents.Defaults.Workspace = al.GetConfig().Agents.Defaults.Workspace
	cfg.Agents.Defaults.ModelName = "test-model"
	cfg.Agents.Defaults.LLMBudget.MaxCallsPerHour = 5
	if err := al.ReloadProviderAndConfig(context.Background(), &countingProvider{}, cfg); err != nil {
		t.Fatalf("reload: %v", err)
	}
	s := al.llmBudget.Snapshot()
	if s.MaxCallsPerHour != 5 || s.CallsThisHour != 1 {
		t.Fatalf("budget after reload = %+v, want limit 5 and the earlier call kept", s)
	}
}

func TestRedactErrorURLs(t *testing.T) {
	in := `Post "https://user:secret@generativelanguage.googleapis.com/v1/models?key=AIzaSECRET": dial tcp`
	got := redactErrorURLs(in)
	if strings.Contains(got, "AIzaSECRET") || strings.Contains(got, "secret@") {
		t.Fatalf("redactErrorURLs() = %q still carries a credential", got)
	}
	if !strings.Contains(got, "https://generativelanguage.googleapis.com/v1/models") {
		t.Fatalf("redactErrorURLs() = %q lost the host/path", got)
	}
}

func TestGetDiagnostics_RegisteredAndFreeOfSecrets(t *testing.T) {
	cfg := config.DefaultConfig()
	cfg.Agents.Defaults.Workspace = t.TempDir()
	cfg.Agents.Defaults.ModelName = "test-model"
	cfg.Agents.Defaults.LLMBudget.MaxCallsPerDay = 7
	cfg.ModelList = []*config.ModelConfig{{ModelName: "test-model", Model: "openai/gpt-x"}}
	cfg.ModelList[0].SetAPIKey("sk-diagnostics-secret")
	al := NewAgentLoop(cfg, bus.NewMessageBus(), &countingProvider{})
	t.Cleanup(al.Close)
	al.recordLLMError(errors.New(`Post "https://api.example.com/v1/chat?key=sk-diagnostics-secret": 500`))

	tool, ok := al.GetRegistry().GetDefaultAgent().Tools.Get("get_diagnostics")
	if !ok {
		t.Fatal("get_diagnostics is not registered by default")
	}
	out := tool.Execute(context.Background(), nil).ForLLM
	if strings.Contains(out, "sk-diagnostics-secret") {
		t.Fatalf("diagnostics leak the API key:\n%s", out)
	}
	for _, want := range []string{`"max_calls_per_day": 7`, `"last_llm_error"`, `"risk_labels"`, `"cooldowns"`} {
		if !strings.Contains(out, want) {
			t.Errorf("diagnostics miss %s:\n%s", want, out)
		}
	}

	cfg2 := config.DefaultConfig()
	cfg2.Agents.Defaults.Workspace = t.TempDir()
	cfg2.Tools.GetDiagnostics.Enabled = false
	al2 := NewAgentLoop(cfg2, bus.NewMessageBus(), &countingProvider{})
	t.Cleanup(al2.Close)
	if _, ok := al2.GetRegistry().GetDefaultAgent().Tools.Get("get_diagnostics"); ok {
		t.Fatal("get_diagnostics registered although tools.get_diagnostics.enabled is false")
	}
}
