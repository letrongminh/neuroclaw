package integrationtools

import (
	"context"
	"strings"
	"testing"

	"github.com/modelcontextprotocol/go-sdk/mcp"
)

func TestNeuroEdgeGuidance_AQuestionForTheOwnerIsNotDone(t *testing.T) {
	got := neuroedgeGuidance(map[string]any{
		"tool": "front_door_unlock", "status": "BLOCK", "gate": "door@1.0.0",
		"reason": "condition_not_met", "failed_criterion": "owner_confirmed", "on_block": "ask",
		"confirmation": map[string]any{
			"id": "confirm_1", "message": "Bộ não xin mở cửa — chủ nhà có đồng ý không?",
			"expires_in_ms": 10000.0, "who": "owner",
		},
	})
	for _, want := range []string{
		"did NOT do this yet", "page at home", "about 10 seconds",
		"you cannot approve it", "Do not call the tool again", "Bộ não xin mở cửa",
	} {
		if !strings.Contains(got, want) {
			t.Errorf("missing %q in %q", want, got)
		}
	}
}

func TestNeuroEdgeGuidance_ABlockWithoutAQuestionGivesTheReason(t *testing.T) {
	got := neuroedgeGuidance(map[string]any{
		"tool": "front_door_unlock", "status": "BLOCK", "reason": "argument_out_of_range",
		"failed_criterion": "duration_s", "message": "Tối đa 30 giây.",
	})
	for _, want := range []string{"refused", "argument_out_of_range (duration_s)", "Tối đa 30 giây.", "Do not retry"} {
		if !strings.Contains(got, want) {
			t.Errorf("missing %q in %q", want, got)
		}
	}
}

func TestNeuroEdgeGuidance_RejectedAndAllowedAndForeignResults(t *testing.T) {
	if got := neuroedgeGuidance(map[string]any{"status": "REJECTED"}); !strings.Contains(got, "nothing ran") {
		t.Errorf("REJECTED: %q", got)
	}
	for label, structured := range map[string]any{
		"ALLOW":          map[string]any{"tool": "living_light_on", "status": "ALLOW"},
		"no structure":   nil,
		"another server": map[string]any{"temperature": 21},
		"not an object":  []any{1, 2},
	} {
		if got := neuroedgeGuidance(structured); got != "" {
			t.Errorf("%s: got guidance %q", label, got)
		}
	}
}

// TC-COM-08-1 (NeuroEdge deployment_profiles.md, DP-COM-08): a tool that declares the
// NeuroEdge contract is read fail-closed.
func TestNeuroEdgeToolGuidance_AnUnknownStatusOrMajorIsNotDone(t *testing.T) {
	v1 := map[string]any{neuroedgeContractKey: "tool-result.v1"}
	for label, c := range map[string]struct {
		meta       map[string]any
		structured any
	}{
		"status MAYBE":          {v1, map[string]any{"tool": "front_door_unlock", "status": "MAYBE"}},
		"no status":             {v1, map[string]any{"tool": "front_door_unlock"}},
		"no result":             {v1, nil},
		"major v2, ALLOW":       {map[string]any{neuroedgeContractKey: "tool-result.v2"}, map[string]any{"tool": "x", "status": "ALLOW"}},
		"contract not a string": {map[string]any{neuroedgeContractKey: 1}, map[string]any{"tool": "x", "status": "ALLOW"}},
	} {
		got := neuroedgeToolGuidance(c.meta, c.structured)
		for _, want := range []string{"NOT done", "do not retry it as if allowed"} {
			if !strings.Contains(got, want) {
				t.Errorf("%s: missing %q in %q", label, want, got)
			}
		}
	}
	for label, meta := range map[string]map[string]any{
		"v1":          v1,
		"later minor": {neuroedgeContractKey: "tool-result.v1.3"},
		"undeclared":  nil,
	} {
		if got := neuroedgeToolGuidance(meta, map[string]any{"tool": "x", "status": "ALLOW"}); got != "" {
			t.Errorf("%s: an ALLOW got guidance %q", label, got)
		}
	}
	// A server that is not a NeuroEdge box is untouched, whatever its status says.
	if got := neuroedgeToolGuidance(nil, map[string]any{"status": "ok"}); got != "" {
		t.Errorf("foreign server: %q", got)
	}
}

func TestNeuroEdgeGuidance_EscalationAndFallbackAreSaid(t *testing.T) {
	escalated := neuroedgeGuidance(map[string]any{
		"tool": "guest_unlock", "status": "BLOCK", "gate": "guest@1.0.0", "reason": "condition_not_met",
		"on_block": "escalate", "escalated_to": "night_manager", "message": "A night manager must approve.",
	})
	if !strings.Contains(escalated, "passed the request to night_manager") {
		t.Errorf("escalate: %q", escalated)
	}
	degraded := neuroedgeGuidance(map[string]any{
		"tool": "siren_on", "status": "BLOCK", "gate": "siren@1.0.0", "reason": "condition_not_met",
		"on_block": "degrade", "fallback": map[string]any{"tool": "lamp_on", "status": "ALLOW"},
	})
	if !strings.Contains(degraded, "Instead, the box did lamp_on") {
		t.Errorf("degrade: %q", degraded)
	}
	blocked := neuroedgeGuidance(map[string]any{
		"tool": "siren_on", "status": "BLOCK", "gate": "siren@1.0.0", "reason": "condition_not_met",
		"on_block": "degrade", "fallback": map[string]any{"tool": "lamp_on", "status": "BLOCK"},
	})
	if !strings.Contains(blocked, "fallback lamp_on was not done either") {
		t.Errorf("degrade, blocked: %q", blocked)
	}
}

func TestMCPTool_ANeuroEdgeResultInAnUnknownContractIsNotDone(t *testing.T) {
	manager := &MockMCPManager{
		callToolFunc: func(ctx context.Context, serverName, toolName string, arguments map[string]any) (*mcp.CallToolResult, error) {
			return &mcp.CallToolResult{
				Content:           []mcp.Content{&mcp.TextContent{Text: `{"tool":"front_door_unlock","status":"MAYBE"}`}},
				StructuredContent: map[string]any{"tool": "front_door_unlock", "status": "MAYBE"},
			}, nil
		},
	}
	tool := &mcp.Tool{Name: "front_door_unlock", Meta: mcp.Meta{neuroedgeContractKey: "tool-result.v1"}}
	result := NewMCPTool(manager, "neuroedge", tool).Execute(context.Background(), map[string]any{})
	if !strings.HasPrefix(result.ForLLM, "[NeuroEdge] The home box answered in a way this brain cannot read") {
		t.Errorf("ForLLM: %q", result.ForLLM)
	}
}
