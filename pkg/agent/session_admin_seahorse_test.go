//go:build !mipsle && !netbsd && !(freebsd && arm)

package agent

import (
	"context"
	"path/filepath"
	"testing"

	"github.com/letrongminh/neuroclaw/pkg/seahorse"
)

func TestClearStoredSessions_ClearsSeahorseContext(t *testing.T) {
	ctx := context.Background()
	cfg, store := newSessionAdminConfig(t)
	const key = "agent:main:s"
	if err := store.AddMessage(ctx, key, "user", "remember the blue door"); err != nil {
		t.Fatal(err)
	}

	dbPath := filepath.Join(cfg.Agents.Defaults.Workspace, "sessions", "seahorse.db")
	engine, err := seahorse.NewEngine(seahorse.Config{DBPath: dbPath}, nil)
	if err != nil {
		t.Fatal(err)
	}
	msgs := []seahorse.Message{{Role: "user", Content: "remember the blue door"}}
	if _, err = engine.Ingest(ctx, key, msgs); err != nil {
		t.Fatal(err)
	}
	engine.Close()

	if _, err = ClearStoredSessions(ctx, cfg, []string{key}); err != nil {
		t.Fatal(err)
	}

	engine, err = seahorse.NewEngine(seahorse.Config{DBPath: dbPath}, nil)
	if err != nil {
		t.Fatal(err)
	}
	defer engine.Close()
	st := engine.GetRetrieval().Store()
	conv, err := st.GetConversationBySessionKey(ctx, key)
	if err != nil {
		t.Fatal(err)
	}
	if conv != nil {
		n, err := st.GetMessageCount(ctx, conv.ConversationID)
		if err != nil {
			t.Fatal(err)
		}
		if n != 0 {
			t.Fatalf("seahorse still holds %d messages after clear", n)
		}
	}
}
