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
//
// A box lists every tool with `_meta["dev.neuroedge/contract"] = "tool-result.v1"`
// (NeuroEdge docs/spec/deployment_profiles.md DP-COM-08). For such a tool the result is read
// fail-closed: a contract major this code does not know, or a status outside ALLOW / BLOCK /
// REJECTED, is told to the model as REJECTED — never as done, never as worth a retry.

// neuroedgeContractKey names the result contract a NeuroEdge tool declares in its _meta.
const neuroedgeContractKey = "dev.neuroedge/contract"

// neuroedgeContract is the contract major this code reads.
const neuroedgeContract = "tool-result.v1"

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
	EscalatedTo     string                 `json:"escalated_to"`
	Fallback        *neuroedgeResult       `json:"fallback"`
	Confirmation    *neuroedgeConfirmation `json:"confirmation"`
}

// neuroedgeDeclared returns the contract a tool's _meta names, and whether it names one.
func neuroedgeDeclared(meta map[string]any) (string, bool) {
	value, ok := meta[neuroedgeContractKey]
	if !ok {
		return "", false
	}
	contract, _ := value.(string)
	return contract, true
}

// neuroedgeKnown reports whether contract is a version of the major this code reads
// ("tool-result.v1", or a later minor such as "tool-result.v1.2").
func neuroedgeKnown(contract string) bool {
	return contract == neuroedgeContract || strings.HasPrefix(contract, neuroedgeContract+".")
}

// neuroedgeRejected is what the model is told for a result it must not trust.
func neuroedgeRejected(why string) string {
	return "[NeuroEdge] The home box answered in a way this brain cannot read (" + why + "), " +
		"so treat it as NOT done. Tell the user it was not done; do not retry it as if allowed."
}

// neuroedgeToolGuidance is neuroedgeGuidance for a tool whose _meta is meta: for a tool that
// declares the NeuroEdge contract, an unknown major, a missing result or an unknown status is
// REJECTED (DP-COM-08); for any other tool it is neuroedgeGuidance as is.
func neuroedgeToolGuidance(meta map[string]any, structured any) string {
	contract, declared := neuroedgeDeclared(meta)
	if !declared {
		return neuroedgeGuidance(structured)
	}
	if !neuroedgeKnown(contract) {
		return neuroedgeRejected(fmt.Sprintf("contract %q, this brain reads %s", contract, neuroedgeContract))
	}
	raw, err := json.Marshal(structured)
	var result neuroedgeResult
	if structured == nil || err != nil || json.Unmarshal(raw, &result) != nil {
		return neuroedgeRejected("no tool-result object")
	}
	switch result.Status {
	case "ALLOW", "BLOCK", "REJECTED":
		return neuroedgeGuidance(structured)
	}
	return neuroedgeRejected(fmt.Sprintf("status %q", result.Status))
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
		guidance := strings.TrimSpace(fmt.Sprintf(
			"[NeuroEdge] The home box refused this: %s. %s "+
				"Tell the user it was not done and why. Do not retry with other arguments "+
				"or look for another way around the rule.", why, result.Message))
		if result.EscalatedTo != "" {
			guidance += fmt.Sprintf(" The box passed the request to %s; say so, and that "+
				"they decide, not you.", result.EscalatedTo)
		}
		if f := result.Fallback; f != nil && f.Tool != "" {
			if f.Status == "ALLOW" {
				guidance += fmt.Sprintf(" Instead, the box did %s (its safe fallback); "+
					"tell the user that is what happened.", f.Tool)
			} else {
				guidance += fmt.Sprintf(" Its fallback %s was not done either.", f.Tool)
			}
		}
		return guidance
	case "REJECTED":
		return "[NeuroEdge] The home box rejected the call itself (unknown tool or arguments " +
			"outside its schema), so nothing ran. Tell the user it was not done."
	}
	return ""
}
