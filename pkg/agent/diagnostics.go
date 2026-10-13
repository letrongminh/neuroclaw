package agent

import (
	"sort"
	"time"

	"github.com/letrongminh/neuroclaw/pkg/config"
)

// diagnosticsRuntimeInfo is the agent's part of get_diagnostics: names,
// counters and states only, never keys, tokens or URLs.
func (al *AgentLoop) diagnosticsRuntimeInfo() map[string]any {
	al.mu.RLock()
	cfg := al.cfg
	fallback := al.fallback
	al.mu.RUnlock()

	info := map[string]any{}
	models := map[string]any{}
	if cfg != nil {
		var channels []string
		for name, ch := range cfg.Channels {
			if ch != nil && ch.Enabled {
				channels = append(channels, name)
			}
		}
		sort.Strings(channels)
		info["channels_enabled"] = channels

		risks := config.RiskLabels(cfg)
		labels := make([]map[string]string, 0, len(risks))
		for _, label := range risks {
			labels = append(labels, map[string]string{"name": label.Name, "says": label.Says})
		}
		info["risk_labels"] = labels

		models["default"] = cfg.Agents.Defaults.GetModelName()
		models["fallbacks"] = cfg.Agents.Defaults.ModelFallbacks
	}
	// Providers that failed at least once since start; available=false means
	// the fallback chain is skipping them until the cooldown ends.
	if cooldown := fallback.Cooldown(); cooldown != nil {
		models["cooldowns"] = cooldown.Snapshot()
	}
	info["models"] = models

	budget := al.llmBudget.Snapshot()
	info["llm_budget"] = map[string]any{
		"enabled":            al.llmBudget.Enabled(),
		"max_calls_per_hour": budget.MaxCallsPerHour,
		"max_calls_per_day":  budget.MaxCallsPerDay,
		"calls_this_hour":    budget.CallsThisHour,
		"calls_today":        budget.CallsToday,
		"hour_resets_at":     budget.HourResetsAt.Format(time.RFC3339),
		"day_resets_at":      budget.DayResetsAt.Format(time.RFC3339),
		"counters":           "in memory since process start",
	}

	if last := al.lastLLMError.Load(); last != nil {
		info["last_llm_error"] = map[string]any{
			"at":      last.At.Format(time.RFC3339),
			"message": last.Message,
		}
	} else {
		info["last_llm_error"] = nil
	}
	return info
}
