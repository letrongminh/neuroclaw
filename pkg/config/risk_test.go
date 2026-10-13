package config

import (
	"reflect"
	"strings"
	"testing"
)

func riskNames(cfg *Config) []string {
	var names []string
	for _, label := range RiskLabels(cfg) {
		names = append(names, label.Name)
	}
	return names
}

// The defaults are PicoClaw's: the model has a shell, scheduled shell
// commands, skill installs and USB monitoring, and each is labelled.
func TestRiskLabels_DefaultsArePicoClawsAndLabelled(t *testing.T) {
	got := riskNames(DefaultConfig())
	want := []string{"cron_shell", "shell", "skill_install"}
	if !reflect.DeepEqual(got, want) {
		t.Fatalf("RiskLabels(DefaultConfig()) = %v, want %v", got, want)
	}
	cfg := DefaultConfig()
	if !cfg.Tools.Exec.Enabled || !cfg.Tools.Cron.AllowCommand || !cfg.Tools.InstallSkill.Enabled ||
		!cfg.Tools.FindSkills.Enabled || !cfg.Devices.MonitorUSB {
		t.Fatalf("defaults are not PicoClaw's: %+v %+v", cfg.Tools.Exec, cfg.Devices)
	}
}

func TestRiskLabels_EachCapability(t *testing.T) {
	quiet := func() *Config {
		cfg := DefaultConfig()
		cfg.Tools.Exec.Enabled = false
		cfg.Tools.Cron.AllowCommand = false
		cfg.Tools.InstallSkill.Enabled = false
		cfg.Tools.FindSkills.Enabled = false
		for _, registry := range cfg.Tools.Skills.Registries {
			registry.Enabled = false
		}
		cfg.Devices.Enabled = false
		cfg.Hooks.Processes = nil
		cfg.Tools.MCP.Servers = nil
		return cfg
	}
	if got := riskNames(quiet()); len(got) != 0 {
		t.Fatalf("a closed config has labels: %v", got)
	}
	cases := []struct {
		name   string
		turnOn func(*Config)
		want   []string
		says   string
	}{
		{"shell", func(c *Config) { c.Tools.Exec.Enabled = true }, []string{"shell"}, "shell commands"},
		{"cron needs exec", func(c *Config) { c.Tools.Cron.AllowCommand = true }, nil, ""},
		{"cron off", func(c *Config) {
			c.Tools.Exec.Enabled = true
			c.Tools.Cron.AllowCommand = true
			c.Tools.Cron.Enabled = false
		}, []string{"shell"}, "shell commands"},
		{"cron with exec", func(c *Config) {
			c.Tools.Exec.Enabled = true
			c.Tools.Cron.AllowCommand = true
		}, []string{"cron_shell", "shell"}, "scheduled"},
		{"buses", func(c *Config) {
			c.Tools.I2C.Enabled = true
			c.Tools.Serial.Enabled = true
		}, []string{"hardware_bus"}, "i2c, serial"},
		// a registry alone serves the CLI, not the model
		{"registry alone", func(c *Config) { c.Tools.Skills.Registries[0].Enabled = true }, nil, ""},
		{"install_skill", func(c *Config) {
			c.Tools.InstallSkill.Enabled = true
			c.Tools.Skills.Registries[0].Enabled = true
		}, []string{"skill_install"}, "registry"},
		{"skills tool off", func(c *Config) {
			c.Tools.InstallSkill.Enabled = true
			c.Tools.FindSkills.Enabled = true
			c.Tools.Skills.Enabled = false
		}, nil, ""},
		{"hooks", func(c *Config) {
			c.Hooks.Enabled = true
			c.Hooks.Processes = map[string]ProcessHookConfig{"notify": {Enabled: true}, "off": {}}
		}, []string{"process_hooks"}, "notify"},
		{"hooks off", func(c *Config) {
			c.Hooks.Enabled = false
			c.Hooks.Processes = map[string]ProcessHookConfig{"notify": {Enabled: true}}
		}, nil, ""},
		{"usb", func(c *Config) {
			c.Devices.Enabled = true
			c.Devices.MonitorUSB = true
		}, []string{"usb_monitor"}, "USB"},
		{"stdio mcp", func(c *Config) {
			c.Tools.MCP.Enabled = true
			c.Tools.MCP.Servers = map[string]MCPServerConfig{
				"fs":   {Enabled: true, Command: "npx"},
				"home": {Enabled: true, Type: "http", URL: "https://box:8443/mcp"},
			}
		}, []string{"stdio_mcp"}, "(fs)"},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			cfg := quiet()
			tc.turnOn(cfg)
			labels := RiskLabels(cfg)
			var got []string
			for _, label := range labels {
				got = append(got, label.Name)
			}
			if !reflect.DeepEqual(got, tc.want) {
				t.Fatalf("labels = %v, want %v", got, tc.want)
			}
			if tc.says != "" && !strings.Contains(labels[0].Says, tc.says) {
				t.Fatalf("label says %q, want it to mention %q", labels[0].Says, tc.says)
			}
		})
	}
	if RiskLabels(nil) != nil {
		t.Fatal("nil config has labels")
	}
}
