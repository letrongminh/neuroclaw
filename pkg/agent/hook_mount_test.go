package agent

import (
	"context"
	"encoding/json"
	"os"
	"path/filepath"
	"slices"
	"testing"

	"github.com/letrongminh/neuroclaw/pkg/bus"
	"github.com/letrongminh/neuroclaw/pkg/config"
)

type builtinAutoHookConfig struct {
	Model  string `json:"model"`
	Suffix string `json:"suffix"`
}

type builtinAutoHook struct {
	model  string
	suffix string
}

func (h *builtinAutoHook) BeforeLLM(
	ctx context.Context,
	req *LLMHookRequest,
) (*LLMHookRequest, HookDecision, error) {
	next := req.Clone()
	next.Model = h.model
	return next, HookDecision{Action: HookActionModify}, nil
}

func (h *builtinAutoHook) AfterLLM(
	ctx context.Context,
	resp *LLMHookResponse,
) (*LLMHookResponse, HookDecision, error) {
	next := resp.Clone()
	if next.Response != nil {
		next.Response.Content += h.suffix
	}
	return next, HookDecision{Action: HookActionModify}, nil
}

func newConfiguredHookLoop(t *testing.T, provider *llmHookTestProvider, hooks config.HooksConfig) *AgentLoop {
	t.Helper()

	cfg := &config.Config{
		Agents: config.AgentsConfig{
			Defaults: config.AgentDefaults{
				Workspace:         t.TempDir(),
				ModelName:         "test-model",
				MaxTokens:         4096,
				MaxToolIterations: 10,
			},
		},
		Hooks: hooks,
	}

	return NewAgentLoop(cfg, bus.NewMessageBus(), provider)
}

func TestAgentLoop_ProcessDirectWithChannel_AutoMountsBuiltinHook(t *testing.T) {
	const hookName = "test-auto-builtin-hook"

	if err := RegisterBuiltinHook(hookName, func(
		ctx context.Context,
		spec config.BuiltinHookConfig,
	) (any, error) {
		var hookCfg builtinAutoHookConfig
		if len(spec.Config) > 0 {
			if err := json.Unmarshal(spec.Config, &hookCfg); err != nil {
				return nil, err
			}
		}
		return &builtinAutoHook{
			model:  hookCfg.Model,
			suffix: hookCfg.Suffix,
		}, nil
	}); err != nil {
		t.Fatalf("RegisterBuiltinHook failed: %v", err)
	}
	t.Cleanup(func() {
		unregisterBuiltinHook(hookName)
	})

	rawCfg, err := json.Marshal(builtinAutoHookConfig{
		Model:  "builtin-model",
		Suffix: "|builtin",
	})
	if err != nil {
		t.Fatalf("json.Marshal failed: %v", err)
	}

	provider := &llmHookTestProvider{}
	al := newConfiguredHookLoop(t, provider, config.HooksConfig{
		Enabled: true,
		Builtins: map[string]config.BuiltinHookConfig{
			hookName: {
				Enabled: true,
				Config:  rawCfg,
			},
		},
	})
	defer al.Close()

	resp, err := al.ProcessDirectWithChannel(context.Background(), "hello", "session-1", "cli", "direct")
	if err != nil {
		t.Fatalf("ProcessDirectWithChannel failed: %v", err)
	}
	if resp != "provider content|builtin" {
		t.Fatalf("expected builtin-hooked content, got %q", resp)
	}

	provider.mu.Lock()
	lastModel := provider.lastModel
	provider.mu.Unlock()
	if lastModel != "builtin-model" {
		t.Fatalf("expected builtin model, got %q", lastModel)
	}
}

func TestAgentLoop_ProcessDirectWithChannel_NeverMountsAProcessHook(t *testing.T) {
	provider := &llmHookTestProvider{}
	eventLog := filepath.Join(t.TempDir(), "events.log")

	al := newConfiguredHookLoop(t, provider, config.HooksConfig{
		Enabled: true,
		Processes: map[string]config.ProcessHookConfig{
			"ipc-auto": {
				Enabled: true,
				Command: processHookHelperCommand(),
				Env: map[string]string{
					"NEUROCLAW_HOOK_HELPER":    "1",
					"NEUROCLAW_HOOK_MODE":      "rewrite",
					"NEUROCLAW_HOOK_EVENT_LOG": eventLog,
				},
				Observe:   []string{"turn_end"},
				Intercept: []string{"before_llm", "after_llm"},
			},
		},
	})
	defer al.Close()

	resp, err := al.ProcessDirectWithChannel(context.Background(), "hello", "session-1", "cli", "direct")
	if err != nil {
		t.Fatalf("ProcessDirectWithChannel failed: %v", err)
	}
	// NeuroClaw lockdown: a configured process hook is never mounted, so the provider
	// answers alone and the hook program never starts.
	if resp != "provider content" {
		t.Fatalf("a process hook ran: got %q", resp)
	}
	if _, err := os.Stat(eventLog); err == nil {
		t.Fatal("the process hook wrote its event log")
	}
}

func TestProcessHookObserveKindsFromConfigAcceptsRuntimeNames(t *testing.T) {
	kinds, enabled, err := processHookObserveKindsFromConfig([]string{
		"tool_exec_start",
		"agent.tool.exec_end",
		"gateway.ready",
		"mcp.server.failed",
	})
	if err != nil {
		t.Fatalf("processHookObserveKindsFromConfig failed: %v", err)
	}
	if !enabled {
		t.Fatal("expected observe to be enabled")
	}

	want := []string{"agent.tool.exec_start", "agent.tool.exec_end", "gateway.ready", "mcp.server.failed"}
	if !slices.Equal(kinds, want) {
		t.Fatalf("observe kinds = %v, want %v", kinds, want)
	}
}
