package agent

import (
	"errors"
	"net/url"
	"regexp"
	"time"

	"github.com/letrongminh/neuroclaw/pkg/logger"
	"github.com/letrongminh/neuroclaw/pkg/providers"
	"github.com/letrongminh/neuroclaw/pkg/utils"
)

// llmErrorRecord is the last failed LLM call, kept for get_diagnostics.
type llmErrorRecord struct {
	At      time.Time
	Message string
}

var (
	errorURLPattern = regexp.MustCompile(`[a-zA-Z][a-zA-Z0-9+.-]*://[^\s"'<>]+`)

	// Secrets that can appear in provider error text outside a URL query.
	errorSecretPatterns = []struct {
		re   *regexp.Regexp
		with string
	}{
		{regexp.MustCompile(`/bot\d+:[A-Za-z0-9_-]+`), "/bot[redacted]"},                // Telegram bot token in a path
		{regexp.MustCompile(`(?i)\bBearer\s+[A-Za-z0-9._~+/=-]+`), "Bearer [redacted]"}, // Authorization header
		{regexp.MustCompile(`\bsk-[A-Za-z0-9_-]{6,}`), "sk-[redacted]"},                 // OpenAI/Anthropic-style keys
		{
			regexp.MustCompile(`(?i)\b((?:api_?key|key|token|access_token|secret)\s*[=:]\s*"?)[^\s"&,;]+`),
			"${1}[redacted]",
		},
	}
)

// redactErrorURLs drops credentials and query strings (where providers such
// as Gemini carry the API key) from URLs quoted in an error message.
func redactErrorURLs(s string) string {
	return errorURLPattern.ReplaceAllStringFunc(s, func(raw string) string {
		u, err := url.Parse(raw)
		if err != nil || u.Host == "" {
			return "[url]"
		}
		u.User = nil
		u.RawQuery = ""
		u.Fragment = ""
		return u.String()
	})
}

// redactLLMError removes credentials from an LLM error message: URL
// userinfo and query strings, then keys, bearer tokens, key=/token= values
// and Telegram bot tokens wherever they appear.
func redactLLMError(s string) string {
	s = redactErrorURLs(s)
	for _, p := range errorSecretPatterns {
		s = p.re.ReplaceAllString(s, p.with)
	}
	return s
}

// recordLLMError keeps a redacted, truncated copy of an LLM failure. A used-up
// LLM budget is not a failure and is not recorded.
func (al *AgentLoop) recordLLMError(err error) {
	if err == nil || isLLMBudgetExceeded(err) {
		return
	}
	msg := redactLLMError(err.Error())
	if cfg := al.GetConfig(); cfg != nil {
		msg = cfg.FilterSensitiveData(msg)
	}
	al.lastLLMError.Store(&llmErrorRecord{At: time.Now(), Message: utils.Truncate(msg, 300)})
}

func isLLMBudgetExceeded(err error) bool {
	var exceeded *providers.LLMBudgetExceededError
	return errors.As(err, &exceeded)
}

// budgetRegistryProviders makes every provider of every agent count against
// the LLM budget: the main one, the light (routing) one and the per-candidate
// fallback ones. Subagents, summaries and seahorse use these providers.
func budgetRegistryProviders(registry *AgentRegistry, budget *providers.LLMBudget) {
	if registry == nil {
		return
	}
	for _, agentID := range registry.ListAgentIDs() {
		agent, ok := registry.GetAgent(agentID)
		if !ok {
			continue
		}
		agent.Provider = providers.WithLLMBudget(agent.Provider, budget)
		agent.LightProvider = providers.WithLLMBudget(agent.LightProvider, budget)
		for key, p := range agent.CandidateProviders {
			agent.CandidateProviders[key] = providers.WithLLMBudget(p, budget)
		}
	}
}

// LLMBudgetBlocked reports, without counting a call, whether the LLM budget
// is used up. Background turns (cron, heartbeat) are skipped when it is.
func (al *AgentLoop) LLMBudgetBlocked() error {
	return al.llmBudget.Check()
}

// LogLLMBudgetSkip logs work skipped because of the LLM budget.
func LogLLMBudgetSkip(err error, source string) {
	fields := map[string]any{"source": source, "action": "skipped"}
	var exceeded *providers.LLMBudgetExceededError
	if errors.As(err, &exceeded) {
		fields["window"] = exceeded.Window
		fields["used"] = exceeded.Used
		fields["limit"] = exceeded.Limit
		fields["resets_at"] = exceeded.ResetAt.Format(time.RFC3339)
	}
	logger.WarnCF("agent", "NeuroClaw llm_budget", fields)
}
