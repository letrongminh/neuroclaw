package integrationtools

import (
	"encoding/json"
	"fmt"
	"strings"
)

// NeuroClaw: what a NeuroEdge home box answers, said plainly to the model.
//
// A box tool returns a Gated Tool Profile result (NeuroEdge schemas/tool-result.v1.json) as
// structuredContent: status ALLOW, BLOCK or REJECTED. A BLOCK with `confirmation` means the
// gate asked the owner, who must answer on the page at home; nothing the brain does can
// answer it. Left to itself a model may retry, claim the door is open, or ask the user to
// "confirm here" in the chat. neuroedgeGuidance turns the result into fixed instructions,
// so the reply is right whatever the model.

type neuroedgeConfirmation struct {
	ID          string  `json:"id"`
	Message     string  `json:"message"`
	ExpiresInMS float64 `json:"expires_in_ms"`
	Who         string  `json:"who"`
}

type neuroedgeResult struct {
	Tool            string                 `json:"tool"`
	Status          string                 `json:"status"`
	Gate            string                 `json:"gate"`
	Reason          string                 `json:"reason"`
	FailedCriterion string                 `json:"failed_criterion"`
	Message         string                 `json:"message"`
	Confirmation    *neuroedgeConfirmation `json:"confirmation"`
}

// neuroedgeGuidance returns instructions for the model when structured is a NeuroEdge
// result that is not a plain ALLOW, and "" otherwise (other MCP servers are untouched).
func neuroedgeGuidance(structured any) string {
	if structured == nil {
		return ""
	}
	raw, err := json.Marshal(structured)
	if err != nil {
		return ""
	}
	var result neuroedgeResult
	if json.Unmarshal(raw, &result) != nil || result.Status == "" {
		return ""
	}
	switch result.Status {
	case "BLOCK":
		if result.Confirmation != nil {
			seconds := int(result.Confirmation.ExpiresInMS / 1000)
			return fmt.Sprintf(
				"[NeuroEdge] The home box did NOT do this yet. Its rule asks the owner first: %q. "+
					"The owner must approve on the NeuroEdge page at home within about %d seconds; "+
					"you cannot approve it, and approval in this chat does not count. "+
					"Tell the user it is waiting for approval at home. "+
					"Do not call the tool again for this request and do not say it is done.",
				result.Confirmation.Message, seconds)
		}
		why := result.Reason
		if result.FailedCriterion != "" {
			why = fmt.Sprintf("%s (%s)", result.Reason, result.FailedCriterion)
		}
		return strings.TrimSpace(fmt.Sprintf(
			"[NeuroEdge] The home box refused this: %s. %s "+
				"Tell the user it was not done and why. Do not retry with other arguments "+
				"or look for another way around the rule.", why, result.Message))
	case "REJECTED":
		return "[NeuroEdge] The home box rejected the call itself (unknown tool or arguments " +
			"outside its schema), so nothing ran. Tell the user it was not done."
	}
	return ""
}
