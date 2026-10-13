package config

import (
	"sort"
	"strings"

	"github.com/letrongminh/neuroclaw/pkg/logger"
)

// RiskLabel names a capability that lets the model reach beyond chat, its
// workspace and the tools of a NeuroEdge home box. NeuroClaw keeps every
// PicoClaw capability and says plainly which ones are on (NeuroEdge Q-73,
// Q-80: transparency instead of prohibition).
type RiskLabel struct {
	Name string // risk=<Name> in the log
	Says string // what it lets the model do
}

// RiskLabels lists the risky capabilities cfg turns on, sorted by name.
func RiskLabels(cfg *Config) []RiskLabel {
	if cfg == nil {
		return nil
	}
	var labels []RiskLabel
	add := func(name, says string) { labels = append(labels, RiskLabel{Name: name, Says: says}) }
	t := &cfg.Tools
	if t.Exec.Enabled {
		add("shell", "the model can run shell commands on this machine (tools.exec)")
	}
	if t.Cron.Enabled && t.Cron.AllowCommand && t.Exec.Enabled {
		add("cron_shell", "scheduled jobs can run shell commands (tools.cron.allow_command)")
	}
	var buses []string
	if t.I2C.Enabled {
		buses = append(buses, "i2c")
	}
	if t.SPI.Enabled {
		buses = append(buses, "spi")
	}
	if t.Serial.Enabled {
		buses = append(buses, "serial")
	}
	if len(buses) > 0 {
		add("hardware_bus", "the model can talk to hardware directly ("+strings.Join(buses, ", ")+
			"), not through a NeuroEdge gate")
	}
	// As agent_init registers them: only with the skills tool on.
	var installs []string
	if t.Skills.Enabled && t.InstallSkill.Enabled {
		installs = append(installs, "tools.install_skill")
	}
	if t.Skills.Enabled && t.FindSkills.Enabled {
		installs = append(installs, "tools.find_skills")
	}
	if len(installs) > 0 {
		for _, registry := range t.Skills.Registries {
			if registry != nil && registry.Enabled {
				installs = append(installs, "registry "+registry.Name)
			}
		}
		add("skill_install", "skills (instructions the model follows) can be fetched from the internet ("+
			strings.Join(installs, ", ")+")")
	}
	if cfg.Hooks.Enabled {
		var hooks []string
		for name, spec := range cfg.Hooks.Processes {
			if spec.Enabled {
				hooks = append(hooks, name)
			}
		}
		if len(hooks) > 0 {
			sort.Strings(hooks)
			add("process_hooks", "external programs run on agent events ("+strings.Join(hooks, ", ")+")")
		}
	}
	if cfg.Devices.Enabled && cfg.Devices.MonitorUSB {
		add("usb_monitor", "USB devices plugged into this machine are watched and announced")
	}
	if t.MCP.Enabled {
		var stdio []string
		for name, server := range t.MCP.Servers {
			if server.Enabled && EffectiveMCPTransportType(server) == "stdio" {
				stdio = append(stdio, name)
			}
		}
		if len(stdio) > 0 {
			sort.Strings(stdio)
			add("stdio_mcp", "MCP servers run as programs on this machine ("+strings.Join(stdio, ", ")+")")
		}
	}
	sort.Slice(labels, func(i, j int) bool { return labels[i].Name < labels[j].Name })
	return labels
}

// LogRiskLabels writes one warning per risky capability that is on.
func LogRiskLabels(cfg *Config) {
	for _, label := range RiskLabels(cfg) {
		logger.WarnCF("config", "NeuroClaw risk="+label.Name+": "+label.Says,
			map[string]any{"risk": label.Name})
	}
}
