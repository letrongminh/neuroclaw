package integrationtools

import (
	"strings"
	"testing"
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
