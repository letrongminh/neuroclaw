package config

import (
	"os"
	"path/filepath"
	"testing"
)

func TestLoadConfigLLMBudgetAndDiagnosticsFromJSONAndEnv(t *testing.T) {
	path := filepath.Join(t.TempDir(), "config.json")
	data := []byte(`{"version": 3, "agents": {"defaults": {"llm_budget": {"max_calls_per_hour": 7}}}}`)
	if err := os.WriteFile(path, data, 0o600); err != nil {
		t.Fatalf("write config: %v", err)
	}

	cfg, err := LoadConfig(path)
	if err != nil {
		t.Fatalf("LoadConfig() error: %v", err)
	}
	if got := cfg.Agents.Defaults.LLMBudget; got.MaxCallsPerHour != 7 || got.MaxCallsPerDay != 0 {
		t.Fatalf("llm_budget from JSON = %+v, want 7/0", got)
	}
	if !cfg.Tools.IsToolEnabled("get_diagnostics") {
		t.Fatal("get_diagnostics is off by default, want on")
	}

	t.Setenv("NEUROCLAW_AGENTS_DEFAULTS_LLM_BUDGET_MAX_CALLS_PER_HOUR", "30")
	t.Setenv("NEUROCLAW_AGENTS_DEFAULTS_LLM_BUDGET_MAX_CALLS_PER_DAY", "200")
	t.Setenv("NEUROCLAW_TOOLS_GET_DIAGNOSTICS_ENABLED", "false")
	cfg, err = LoadConfig(path)
	if err != nil {
		t.Fatalf("LoadConfig() with env error: %v", err)
	}
	if got := cfg.Agents.Defaults.LLMBudget; got.MaxCallsPerHour != 30 || got.MaxCallsPerDay != 200 {
		t.Fatalf("llm_budget from env = %+v, want 30/200", got)
	}
	if cfg.Tools.IsToolEnabled("get_diagnostics") {
		t.Fatal("NEUROCLAW_TOOLS_GET_DIAGNOSTICS_ENABLED=false did not turn the tool off")
	}
}
