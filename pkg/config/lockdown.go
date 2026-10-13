package config

import (
	"sort"

	"github.com/letrongminh/neuroclaw/pkg/logger"
)

// NeuroClaw lockdown.
//
// NeuroClaw is the brain of a NeuroEdge home (Q-76): it chats, calls an LLM, remembers,
// schedules, searches the web and calls the MCP tools of the home box. It never runs a
// command or touches hardware itself: the only way to a device is a box tool, behind the
// box's gate and the owner's approval at home.
//
// So these are off, whatever config.json or NEUROCLAW_* environment variables say:
//   - tools: exec, i2c, spi, serial, install_skill, find_skills (LockedTools);
//   - shell commands in cron jobs (cron.allow_command);
//   - process hooks (hooks.processes), which run external programs;
//   - USB device monitoring (devices);
//   - skill registries (skills come with the binary or the workspace, never fetched);
//   - stdio MCP servers, which are commands too (only http/sse servers load).
//
// The lockdown is applied after the file and the environment are read, again in
// IsToolEnabled, and once more where commands would start (cron, hooks, MCP stdio).

// LockedTools are the tools NeuroClaw never registers.
var LockedTools = map[string]bool{
	"exec":          true,
	"i2c":           true,
	"spi":           true,
	"serial":        true,
	"install_skill": true,
	"find_skills":   true,
}

// ApplyLockdown turns every locked setting off and returns the ones that were on.
func ApplyLockdown(cfg *Config) []string {
	if cfg == nil {
		return nil
	}
	var turnedOff []string
	off := func(name string, enabled *bool) {
		if *enabled {
			turnedOff = append(turnedOff, name)
			*enabled = false
		}
	}
	t := &cfg.Tools
	off("tools.exec", &t.Exec.Enabled)
	off("tools.i2c", &t.I2C.Enabled)
	off("tools.spi", &t.SPI.Enabled)
	off("tools.serial", &t.Serial.Enabled)
	off("tools.install_skill", &t.InstallSkill.Enabled)
	off("tools.find_skills", &t.FindSkills.Enabled)
	off("tools.cron.allow_command", &t.Cron.AllowCommand)
	off("devices", &cfg.Devices.Enabled)
	off("devices.monitor_usb", &cfg.Devices.MonitorUSB)
	for _, registry := range t.Skills.Registries {
		if registry != nil {
			off("tools.skills.registries."+registry.Name, &registry.Enabled)
		}
	}
	for name, spec := range cfg.Hooks.Processes {
		if spec.Enabled {
			turnedOff = append(turnedOff, "hooks.processes."+name)
			spec.Enabled = false
			cfg.Hooks.Processes[name] = spec
		}
	}
	for name, server := range t.MCP.Servers {
		if server.Enabled && EffectiveMCPTransportType(server) == "stdio" {
			turnedOff = append(turnedOff, "tools.mcp.servers."+name+" (stdio)")
			server.Enabled = false
			t.MCP.Servers[name] = server
		}
	}
	sort.Strings(turnedOff)
	return turnedOff
}

func applyLockdownAndWarn(cfg *Config, source string) {
	if turnedOff := ApplyLockdown(cfg); len(turnedOff) > 0 {
		logger.WarnCF("config", "NeuroClaw lockdown: turned off settings that run commands or touch hardware",
			map[string]any{"source": source, "settings": turnedOff})
	}
}
