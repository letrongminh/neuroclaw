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
	const other = "agent:main:t"
	if err = store.AddMessage(ctx, other, "user", "and the red one"); err != nil {
		t.Fatal(err)
	}
	for _, k := range []string{key, other} {
		msgs := []seahorse.Message{{Role: "user", Content: "remember the door " + k}}
		if _, err = engine.Ingest(ctx, k, msgs); err != nil {
			t.Fatal(err)
		}
	}
	engine.Close()

	// --all: one engine for the directory clears both sessions.
	cleared, err := ClearStoredSessions(ctx, cfg, nil)
	if err != nil || len(cleared) != 2 {
		t.Fatalf("ClearStoredSessions(all) = %+v, %v", cleared, err)
	}

	engine, err = seahorse.NewEngine(seahorse.Config{DBPath: dbPath}, nil)
	if err != nil {
		t.Fatal(err)
	}
	defer engine.Close()
	st := engine.GetRetrieval().Store()
	for _, k := range []string{key, other} {
		conv, err := st.GetConversationBySessionKey(ctx, k)
		if err != nil {
			t.Fatal(err)
		}
		if conv == nil {
			continue
		}
		n, err := st.GetMessageCount(ctx, conv.ConversationID)
		if err != nil {
			t.Fatal(err)
		}
		if n != 0 {
			t.Fatalf("seahorse still holds %d messages of %s after clear", n, k)
		}
	}
}
