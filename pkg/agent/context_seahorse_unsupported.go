//go:build mipsle || netbsd || (freebsd && arm)

package agent

import (
	"context"
	"encoding/json"
	"fmt"
)

// newSeahorseContextManager is unavailable on platforms where modernc sqlite/libc
// currently has no stable build path for this project.
func newSeahorseContextManager(_ json.RawMessage, _ *AgentLoop) (ContextManager, error) {
	return nil, fmt.Errorf("seahorse context manager is unavailable on this platform")
}

// openSeahorseClearer has nothing to clear: seahorse never runs here.
func openSeahorseClearer(string) (clearFn func(context.Context, string) error, closeFn func(), err error) {
	return func(context.Context, string) error { return nil }, func() {}, nil
}

func init() {
	if err := RegisterContextManager("seahorse", newSeahorseContextManager); err != nil {
		panic(fmt.Sprintf("register seahorse context manager: %v", err))
	}
}
