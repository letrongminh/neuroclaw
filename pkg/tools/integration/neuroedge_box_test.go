//go:build neuroedge

// TC-A-07-1 (NeuroEdge docs/spec/deployment_profiles.md, DP-A-07): NeuroClaw against a real,
// pinned NeuroEdge box. The box serves its agents over stdio (`neuroedge mcp serve`, sim);
// every tool must declare the contract this brain reads, every result must be a valid
// tool-result.v1, and the guidance must say what happened — a question for the owner, a
// refusal, an escalation, a fallback that ran instead.
//
//	NEUROEDGE_REPO=/path/to/neuroedge-init NEUROEDGE_BIN=neuroedge \
//	  go test -tags neuroedge ./pkg/tools/integration/ -run NeuroEdgeBox
//
// CI (.github/workflows/neuroedge-contract.yml) checks out the box at the pinned commit.
package integrationtools

import (
	"context"
	"encoding/json"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
	"time"

	"github.com/google/jsonschema-go/jsonschema"
	"github.com/modelcontextprotocol/go-sdk/mcp"
)

type boxCase struct {
	tool      string
	arguments map[string]any
	status    string
	guidance  string // a phrase the guidance must hold ("" for none at all)
}

func boxRepo(t *testing.T) string {
	repo := os.Getenv("NEUROEDGE_REPO")
	if repo == "" {
		t.Fatal("NEUROEDGE_REPO is not set: this test runs against a checked-out NeuroEdge box")
	}
	return repo
}

func resultSchema(t *testing.T, repo string) *jsonschema.Resolved {
	raw, err := os.ReadFile(filepath.Join(repo, "schemas", "tool-result.v1.json"))
	if err != nil {
		t.Fatal(err)
	}
	var schema jsonschema.Schema
	if err := json.Unmarshal(raw, &schema); err != nil {
		t.Fatal(err)
	}
	resolved, err := schema.Resolve(nil)
	if err != nil {
		t.Fatal(err)
	}
	return resolved
}

// serve starts `neuroedge mcp serve --agent agent` and returns a connected session.
func serve(t *testing.T, agent string) *mcp.ClientSession {
	bin := os.Getenv("NEUROEDGE_BIN")
	if bin == "" {
		bin = "neuroedge"
	}
	ctx, cancel := context.WithTimeout(context.Background(), 60*time.Second)
	t.Cleanup(cancel)
	client := mcp.NewClient(&mcp.Implementation{Name: "neuroclaw-contract", Version: "test"}, nil)
	transport := &mcp.CommandTransport{Command: exec.Command(bin, "mcp", "serve", "--agent", agent)}
	session, err := client.Connect(ctx, transport, nil)
	if err != nil {
		t.Fatalf("start the box on %s: %v", agent, err)
	}
	t.Cleanup(func() { _ = session.Close() })
	return session
}

func runBox(t *testing.T, agent string, cases []boxCase) {
	repo := boxRepo(t)
	schema := resultSchema(t, repo)
	session := serve(t, agent)
	ctx := context.Background()
	listed, err := session.ListTools(ctx, nil)
	if err != nil {
		t.Fatal(err)
	}
	tools := map[string]*mcp.Tool{}
	for _, tool := range listed.Tools {
		if contract, ok := neuroedgeDeclared(tool.Meta); !ok || !neuroedgeKnown(contract) {
			t.Errorf("%s: contract %v, want %s", tool.Name, tool.Meta[neuroedgeContractKey], neuroedgeContract)
		}
		tools[tool.Name] = tool
	}
	for _, c := range cases {
		tool := tools[c.tool]
		if tool == nil {
			t.Fatalf("the box lists no tool %s", c.tool)
		}
		result, err := session.CallTool(ctx, &mcp.CallToolParams{Name: c.tool, Arguments: c.arguments})
		if err != nil {
			t.Fatalf("%s: %v", c.tool, err)
		}
		raw, _ := json.Marshal(result.StructuredContent)
		var instance any
		_ = json.Unmarshal(raw, &instance)
		if err := schema.Validate(instance); err != nil {
			t.Errorf("%s %v: not tool-result.v1: %v\n%s", c.tool, c.arguments, err, raw)
		}
		var got neuroedgeResult
		_ = json.Unmarshal(raw, &got)
		if got.Status != c.status {
			t.Errorf("%s %v: status %q, want %q\n%s", c.tool, c.arguments, got.Status, c.status, raw)
		}
		guidance := neuroedgeToolGuidance(tool.Meta, result.StructuredContent)
		if c.guidance == "" && guidance != "" {
			t.Errorf("%s: unexpected guidance %q", c.tool, guidance)
		}
		if !strings.Contains(guidance, c.guidance) {
			t.Errorf("%s %v: guidance %q lacks %q", c.tool, c.arguments, guidance, c.guidance)
		}
		if strings.Contains(guidance, "cannot read") {
			t.Errorf("%s: a valid result read as unreadable: %q", c.tool, guidance)
		}
	}
}

func TestNeuroEdgeBox_TheDemoLightAndDoor(t *testing.T) {
	agent := filepath.Join(boxRepo(t), "fixtures", "agents", "chat-den-cua", "agent.toml")
	runBox(t, agent, []boxCase{
		{"living_light_on", map[string]any{}, "ALLOW", ""},
		{"front_door_unlock", map[string]any{"duration_s": 10}, "BLOCK", "did NOT do this yet"},
		{"front_door_unlock", map[string]any{"duration_s": 45}, "BLOCK", "argument_out_of_range (duration_s)"},
		{"front_door_unlock", map[string]any{"duration_s": "soon"}, "REJECTED", "rejected the call itself"},
	})
}

func TestNeuroEdgeBox_TheDoorAtNight(t *testing.T) {
	source := filepath.Join(boxRepo(t), "fixtures", "agents", "chat-den-cua")
	night := filepath.Join(t.TempDir(), "chat-den-cua")
	if err := os.CopyFS(night, os.DirFS(source)); err != nil {
		t.Fatal(err)
	}
	manifest := filepath.Join(night, "agent.toml")
	raw, err := os.ReadFile(manifest)
	if err != nil {
		t.Fatal(err)
	}
	text := strings.Replace(string(raw), "daytime          = true", "daytime          = false", 1)
	if text == string(raw) {
		t.Fatal("the demo agent no longer sets `daytime` in [sim.facts]")
	}
	if err := os.WriteFile(manifest, []byte(text), 0o644); err != nil {
		t.Fatal(err)
	}
	runBox(t, manifest, []boxCase{
		{"front_door_unlock", map[string]any{"duration_s": 10}, "BLOCK", "condition_not_met (daytime)"},
	})
}

func TestNeuroEdgeBox_EscalationAndFallback(t *testing.T) {
	agent := filepath.Join(boxRepo(t), "fixtures", "agents", "contract-bench", "agent.toml")
	runBox(t, agent, []boxCase{
		{"siren_on", map[string]any{}, "BLOCK", "Instead, the box did porch_lamp_on"},
		{"guest_unlock", map[string]any{}, "BLOCK", "passed the request to night_manager"},
		{"heater_on", map[string]any{"level": 9}, "BLOCK", "argument_out_of_range (level)"},
	})
}
