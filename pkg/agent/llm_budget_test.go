package agent

import (
	"context"
	"errors"
	"os"
	"path/filepath"
	"strings"
	"sync/atomic"
	"testing"

	"github.com/letrongminh/neuroclaw/pkg/bus"
	"github.com/letrongminh/neuroclaw/pkg/config"
	"github.com/letrongminh/neuroclaw/pkg/media"
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

func TestRedactLLMError(t *testing.T) {
	for _, tc := range []struct {
		in     string
		secret string
		keep   string
	}{
		{
			in:     `Post "https://user:secret@generativelanguage.googleapis.com/v1/models?key=AIzaSECRET": dial tcp`,
			secret: "AIzaSECRET",
			keep:   "https://generativelanguage.googleapis.com/v1/models",
		},
		{in: "invalid api key sk-proj-abcDEF123456789 provided", secret: "abcDEF123456789", keep: "invalid api key"},
		{in: "header Authorization: Bearer eyJhbGciOi.payload.sig rejected", secret: "eyJhbGciOi", keep: "rejected"},
		{in: "request failed: api_key=XYZ987secret&model=gpt", secret: "XYZ987secret", keep: "request failed"},
		{in: `upstream said token: "tok_live_123456" is expired`, secret: "tok_live_123456", keep: "is expired"},
		{in: "telegram: POST /bot123456789:AAHdqTcvCH1vGWJxfSeofSAs0K5PALDsaw/sendMessage 401", secret: "AAHdqTcvCH1vGWJxfSeofSAs0K5PALDsaw", keep: "/sendMessage 401"},
	} {
		got := redactLLMError(tc.in)
		if strings.Contains(got, tc.secret) {
			t.Errorf("redactLLMError(%q) = %q still carries %q", tc.in, got, tc.secret)
		}
		if !strings.Contains(got, tc.keep) {
			t.Errorf("redactLLMError(%q) = %q lost %q", tc.in, got, tc.keep)
		}
	}
}

func TestLLMBudget_EveryAgentProviderCounts(t *testing.T) {
	al, provider := newBudgetTestLoop(t, 100)
	agent := al.GetRegistry().GetDefaultAgent()
	// Subagents, summaries and seahorse call agent.Provider directly, not
	// through the turn pipeline: those calls count too.
	if _, err := agent.Provider.Chat(context.Background(), nil, nil, "test-model", nil); err != nil {
		t.Fatal(err)
	}
	if got := al.llmBudget.Snapshot().CallsThisHour; got != 1 || provider.calls.Load() != 1 {
		t.Fatalf("direct agent.Provider call: budget %d, provider %d; want 1 and 1", got, provider.calls.Load())
	}
	if providers.UnwrapLLMBudget(agent.Provider) != providers.LLMProvider(provider) {
		t.Fatal("agent.Provider does not wrap the configured provider")
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

func TestLLMBudget_TranscriptionCountsAndIsRefusedWhenUsedUp(t *testing.T) {
	al, _ := newBudgetTestLoop(t, 1)
	store := media.NewFileMediaStore()
	audioPath := filepath.Join(t.TempDir(), "voice.ogg")
	if err := os.WriteFile(audioPath, []byte("fake audio"), 0o644); err != nil {
		t.Fatal(err)
	}
	ref, err := store.Store(audioPath, media.MediaMeta{
		Filename: "voice.ogg", ContentType: "audio/ogg", CleanupPolicy: media.CleanupPolicyForgetOnly,
	}, "scope-voice")
	if err != nil {
		t.Fatal(err)
	}
	al.SetMediaStore(store)
	al.SetTranscriber(&fixedTranscriber{text: "open the door"})

	msg := bus.InboundMessage{Content: "[voice]", Media: []string{ref}}
	got, _ := al.transcribeAudioInMessage(context.Background(), msg)
	if !strings.Contains(got.Content, "open the door") {
		t.Fatalf("first clip not transcribed: %q", got.Content)
	}
	if n := al.llmBudget.Snapshot().CallsThisHour; n != 1 {
		t.Fatalf("calls after one transcription = %d, want 1", n)
	}

	got, _ = al.transcribeAudioInMessage(context.Background(), msg)
	if strings.Contains(got.Content, "open the door") {
		t.Fatalf("clip transcribed although the budget is used up: %q", got.Content)
	}
}
