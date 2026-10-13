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

var errorURLPattern = regexp.MustCompile(`[a-zA-Z][a-zA-Z0-9+.-]*://[^\s"'<>]+`)

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

// recordLLMError keeps a redacted, truncated copy of an LLM failure.
func (al *AgentLoop) recordLLMError(err error) {
	if err == nil {
		return
	}
	msg := redactErrorURLs(err.Error())
	if cfg := al.GetConfig(); cfg != nil {
		msg = cfg.FilterSensitiveData(msg)
	}
	al.lastLLMError.Store(&llmErrorRecord{At: time.Now(), Message: utils.Truncate(msg, 300)})
}

// acquireLLMBudget counts one LLM call against the hourly/daily budget and
// logs when the budget refuses it.
func (al *AgentLoop) acquireLLMBudget(source, agentID string) error {
	err := al.llmBudget.Acquire()
	if err != nil {
		logLLMBudget(err, source, agentID, "")
	}
	return err
}

// LLMBudgetBlocked reports, without counting a call, whether the LLM budget
// is used up. Background turns (cron, heartbeat) are skipped when it is.
func (al *AgentLoop) LLMBudgetBlocked() error {
	return al.llmBudget.Check()
}

// LogLLMBudgetSkip logs a background turn skipped because of the LLM budget.
func LogLLMBudgetSkip(err error, source string) {
	logLLMBudget(err, source, "", "skipped")
}

func logLLMBudget(err error, source, agentID, action string) {
	fields := map[string]any{"source": source}
	if agentID != "" {
		fields["agent_id"] = agentID
	}
	if action != "" {
		fields["action"] = action
	}
	var exceeded *providers.LLMBudgetExceededError
	if errors.As(err, &exceeded) {
		fields["window"] = exceeded.Window
		fields["used"] = exceeded.Used
		fields["limit"] = exceeded.Limit
		fields["resets_at"] = exceeded.ResetAt.Format(time.RFC3339)
	}
	logger.WarnCF("agent", "NeuroClaw llm_budget", fields)
}
