package agent

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"time"

	"github.com/letrongminh/neuroclaw/pkg/config"
	"github.com/letrongminh/neuroclaw/pkg/memory"
	"github.com/letrongminh/neuroclaw/pkg/routing"
)

// StoredSession is one session persisted on disk, read without starting an
// agent (for `neuroclaw session list|clear`).
type StoredSession struct {
	AgentID   string
	Key       string
	Messages  int
	UpdatedAt time.Time
}

type agentSessionDir struct {
	agentIDs []string
	dir      string
}

// agentSessionDirs returns the existing sessions directories of the
// configured agents, as NewAgentRegistry would resolve them. Agents that share
// a workspace share one directory.
func agentSessionDirs(cfg *config.Config) []agentSessionDir {
	agentCfgs := cfg.Agents.List
	if len(agentCfgs) == 0 {
		agentCfgs = []config.AgentConfig{{ID: "main", Default: true}}
	}
	byDir := map[string]*agentSessionDir{}
	var order []string
	for i := range agentCfgs {
		ac := &agentCfgs[i]
		dir := filepath.Clean(filepath.Join(resolveAgentWorkspace(ac, &cfg.Agents.Defaults), "sessions"))
		if info, err := os.Stat(dir); err != nil || !info.IsDir() {
			continue
		}
		entry := byDir[dir]
		if entry == nil {
			entry = &agentSessionDir{dir: dir}
			byDir[dir] = entry
			order = append(order, dir)
		}
		entry.agentIDs = append(entry.agentIDs, routing.NormalizeAgentID(ac.ID))
	}
	out := make([]agentSessionDir, 0, len(order))
	for _, dir := range order {
		out = append(out, *byDir[dir])
	}
	return out
}

// ListStoredSessions lists the sessions stored on disk for every configured
// agent, sorted by agent then key.
func ListStoredSessions(ctx context.Context, cfg *config.Config) ([]StoredSession, error) {
	var out []StoredSession
	for _, d := range agentSessionDirs(cfg) {
		store, err := memory.NewJSONLStore(d.dir)
		if err != nil {
			return nil, err
		}
		for _, key := range store.ListSessions() {
			s := StoredSession{AgentID: d.agentIDs[0], Key: key}
			if meta, err := store.GetSessionMeta(ctx, key); err == nil {
				s.UpdatedAt = meta.UpdatedAt
			}
			if history, err := store.GetHistory(ctx, key); err == nil {
				s.Messages = len(history)
			}
			out = append(out, s)
		}
	}
	sort.Slice(out, func(i, j int) bool {
		if out[i].AgentID != out[j].AgentID {
			return out[i].AgentID < out[j].AgentID
		}
		return out[i].Key < out[j].Key
	})
	return out, nil
}

// ClearStoredSessions empties the history and summary of the given sessions
// on disk, as /clear does in chat: the JSONL history, and the seahorse context
// database when one exists. keys == nil clears every session. A key may be a
// session alias. Returns the sessions cleared; a key found nowhere is an error.
//
// The caller must make sure no gateway is running: it reads and appends to
// the same files and could write over the clear mid-turn.
func ClearStoredSessions(ctx context.Context, cfg *config.Config, keys []string) ([]StoredSession, error) {
	var cleared []StoredSession
	found := map[string]bool{}
	for _, d := range agentSessionDirs(cfg) {
		store, err := memory.NewJSONLStore(d.dir)
		if err != nil {
			return cleared, err
		}
		var targets []string
		if keys == nil {
			targets = store.ListSessions()
		} else {
			for _, key := range keys {
				canonical, ok, err := store.ResolveSessionKey(ctx, key)
				if err != nil {
					return cleared, err
				}
				if ok {
					found[key] = true
					targets = append(targets, canonical)
				}
			}
		}
		var clearSeahorse func(context.Context, string) error
		if seahorseDB := filepath.Join(d.dir, "seahorse.db"); len(targets) > 0 && fileExists(seahorseDB) {
			clearFn, closeFn, err := openSeahorseClearer(seahorseDB)
			if err != nil {
				return cleared, fmt.Errorf("open seahorse context: %w", err)
			}
			defer closeFn()
			clearSeahorse = clearFn
		}
		for _, key := range targets {
			if err := store.SetHistory(ctx, key, nil); err != nil {
				return cleared, fmt.Errorf("clear %s: %w", key, err)
			}
			if err := store.SetSummary(ctx, key, ""); err != nil {
				return cleared, fmt.Errorf("clear summary of %s: %w", key, err)
			}
			if clearSeahorse != nil {
				if err := clearSeahorse(ctx, key); err != nil {
					return cleared, fmt.Errorf("clear seahorse context of %s: %w", key, err)
				}
			}
			cleared = append(cleared, StoredSession{AgentID: d.agentIDs[0], Key: key})
		}
	}
	for _, key := range keys {
		if !found[key] {
			return cleared, fmt.Errorf("no stored session %q (see `neuroclaw session list`)", key)
		}
	}
	return cleared, nil
}
