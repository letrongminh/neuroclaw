package agent

import (
	"context"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/letrongminh/neuroclaw/pkg/config"
	"github.com/letrongminh/neuroclaw/pkg/memory"
)

func newSessionAdminConfig(t *testing.T) (*config.Config, *memory.JSONLStore) {
	t.Helper()
	cfg := config.DefaultConfig()
	cfg.Agents.Defaults.Workspace = t.TempDir()
	store, err := memory.NewJSONLStore(filepath.Join(cfg.Agents.Defaults.Workspace, "sessions"))
	if err != nil {
		t.Fatal(err)
	}
	return cfg, store
}

func TestListAndClearStoredSessions(t *testing.T) {
	ctx := context.Background()
	cfg, store := newSessionAdminConfig(t)
	for _, key := range []string{"agent:main:a", "agent:main:b"} {
		if err := store.AddMessage(ctx, key, "user", "hello"); err != nil {
			t.Fatal(err)
		}
		if err := store.AddMessage(ctx, key, "assistant", "hi"); err != nil {
			t.Fatal(err)
		}
		if err := store.SetSummary(ctx, key, "greeting"); err != nil {
			t.Fatal(err)
		}
	}

	sessions, err := ListStoredSessions(ctx, cfg)
	if err != nil {
		t.Fatal(err)
	}
	if len(sessions) != 2 || sessions[0].Key != "agent:main:a" || sessions[0].Messages != 2 ||
		sessions[0].AgentID != "main" || sessions[0].UpdatedAt.IsZero() {
		t.Fatalf("ListStoredSessions() = %+v", sessions)
	}

	cleared, err := ClearStoredSessions(ctx, cfg, []string{"agent:main:a"})
	if err != nil || len(cleared) != 1 {
		t.Fatalf("ClearStoredSessions(a) = %+v, %v", cleared, err)
	}
	if h, _ := store.GetHistory(ctx, "agent:main:a"); len(h) != 0 {
		t.Fatalf("history of a after clear = %d messages", len(h))
	}
	if s, _ := store.GetSummary(ctx, "agent:main:a"); s != "" {
		t.Fatalf("summary of a after clear = %q", s)
	}
	if h, _ := store.GetHistory(ctx, "agent:main:b"); len(h) != 2 {
		t.Fatalf("history of b = %d messages, want untouched", len(h))
	}

	if _, err = ClearStoredSessions(ctx, cfg, []string{"agent:main:missing"}); err == nil ||
		!strings.Contains(err.Error(), "no stored session") {
		t.Fatalf("clearing an unknown key error = %v", err)
	}

	cleared, err = ClearStoredSessions(ctx, cfg, nil)
	if err != nil || len(cleared) != 2 {
		t.Fatalf("ClearStoredSessions(all) = %+v, %v", cleared, err)
	}
	if h, _ := store.GetHistory(ctx, "agent:main:b"); len(h) != 0 {
		t.Fatalf("history of b after --all = %d messages", len(h))
	}
}

func TestListStoredSessions_NoSessionsDirIsEmptyAndNotCreated(t *testing.T) {
	cfg := config.DefaultConfig()
	cfg.Agents.Defaults.Workspace = filepath.Join(t.TempDir(), "workspace")
	sessions, err := ListStoredSessions(context.Background(), cfg)
	if err != nil || len(sessions) != 0 {
		t.Fatalf("ListStoredSessions() = %+v, %v", sessions, err)
	}
	if _, err := os.Stat(cfg.Agents.Defaults.Workspace); !os.IsNotExist(err) {
		t.Fatalf("listing created the workspace: %v", err)
	}
}
