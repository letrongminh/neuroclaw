package config

import (
	"os"
	"path/filepath"
	"testing"
)

// a config.json that asks for every road to the world NeuroClaw closes
const wideOpen = `{
  "version": 3,
  "tools": {
    "exec": {"enabled": true},
    "i2c": {"enabled": true},
    "spi": {"enabled": true},
    "serial": {"enabled": true},
    "install_skill": {"enabled": true},
    "find_skills": {"enabled": true},
    "cron": {"enabled": true, "allow_command": true},
    "skills": {"enabled": true, "registries": {"clawhub": {"enabled": true, "base_url": "https://clawhub.ai"}}},
    "mcp": {"enabled": true, "servers": {
      "shell": {"enabled": true, "command": "sh", "args": ["-c", "id"]},
      "box": {"enabled": true, "type": "http", "url": "https://box.example/mcp"}
    }}
  },
  "devices": {"enabled": true, "monitor_usb": true},
  "hooks": {"enabled": true, "processes": {"run": {"enabled": true, "command": ["sh", "-c", "id"]}}}
}`

func loadLocked(t *testing.T, body string) *Config {
	t.Helper()
	path := filepath.Join(t.TempDir(), "config.json")
	if err := os.WriteFile(path, []byte(body), 0o600); err != nil {
		t.Fatal(err)
	}
	cfg, err := LoadConfig(path)
	if err != nil {
		t.Fatalf("LoadConfig: %v", err)
	}
	return cfg
}

func assertLocked(t *testing.T, cfg *Config) {
	t.Helper()
	for name := range LockedTools {
		if cfg.Tools.IsToolEnabled(name) {
			t.Errorf("tool %s is enabled", name)
		}
	}
	if cfg.Tools.Exec.Enabled || cfg.Tools.I2C.Enabled || cfg.Tools.SPI.Enabled || cfg.Tools.Serial.Enabled {
		t.Error("a locked tool flag is still set")
	}
	if cfg.Tools.Cron.AllowCommand {
		t.Error("cron may run commands")
	}
	if cfg.Devices.Enabled || cfg.Devices.MonitorUSB {
		t.Error("device monitoring is on")
	}
	for _, registry := range cfg.Tools.Skills.Registries {
		if registry != nil && registry.Enabled {
			t.Errorf("skill registry %s is on", registry.Name)
		}
	}
	for name, spec := range cfg.Hooks.Processes {
		if spec.Enabled {
			t.Errorf("process hook %s is on", name)
		}
	}
	for name, server := range cfg.Tools.MCP.Servers {
		if server.Enabled && EffectiveMCPTransportType(server) == "stdio" {
			t.Errorf("stdio MCP server %s is on", name)
		}
	}
}

func TestLockdown_AConfigThatAsksForEverythingGetsNone(t *testing.T) {
	cfg := loadLocked(t, wideOpen)
	assertLocked(t, cfg)
	// what NeuroClaw keeps
	if !cfg.Tools.MCP.Servers["box"].Enabled {
		t.Error("the http MCP server (the home box) was turned off")
	}
	if !cfg.Tools.Cron.Enabled || !cfg.Tools.Skills.Enabled {
		t.Error("cron or built-in skills were turned off")
	}
}

func TestLockdown_EnvironmentVariablesCannotReopenIt(t *testing.T) {
	for _, name := range []string{
		"NEUROCLAW_TOOLS_EXEC_ENABLED", "NEUROCLAW_TOOLS_I2C_ENABLED", "NEUROCLAW_TOOLS_SPI_ENABLED",
		"NEUROCLAW_TOOLS_SERIAL_ENABLED", "NEUROCLAW_TOOLS_INSTALL_SKILL_ENABLED",
		"NEUROCLAW_TOOLS_FIND_SKILLS_ENABLED", "NEUROCLAW_TOOLS_CRON_ALLOW_COMMAND",
		"NEUROCLAW_DEVICES_ENABLED", "NEUROCLAW_DEVICES_MONITOR_USB",
	} {
		t.Setenv(name, "true")
	}
	assertLocked(t, loadLocked(t, `{"version": 3}`))
}

func TestLockdown_TheDefaultConfigIsLocked(t *testing.T) {
	assertLocked(t, DefaultConfig())
}

func TestLockdown_ReportsWhatItTurnedOff(t *testing.T) {
	cfg := defaultConfig()
	cfg.Tools.Exec.Enabled = true
	cfg.Tools.Cron.AllowCommand = true
	got := ApplyLockdown(cfg)
	want := map[string]bool{"tools.exec": true, "tools.cron.allow_command": true}
	for _, name := range got {
		delete(want, name)
	}
	if len(want) != 0 {
		t.Fatalf("not reported: %v (got %v)", want, got)
	}
	if again := ApplyLockdown(cfg); len(again) != 0 {
		t.Fatalf("a second pass found more: %v", again)
	}
}
