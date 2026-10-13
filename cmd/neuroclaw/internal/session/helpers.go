package session

import (
	"context"
	"fmt"
	"io"

	"github.com/letrongminh/neuroclaw/cmd/neuroclaw/internal"
	"github.com/letrongminh/neuroclaw/pkg/agent"
	"github.com/letrongminh/neuroclaw/pkg/pid"
)

func sessionListCmd(out io.Writer) error {
	cfg, err := internal.LoadConfig()
	if err != nil {
		return fmt.Errorf("error loading config: %w", err)
	}

	sessions, err := agent.ListStoredSessions(context.Background(), cfg)
	if err != nil {
		return err
	}
	if len(sessions) == 0 {
		fmt.Fprintln(out, "No stored sessions.")
		return nil
	}

	fmt.Fprintln(out, "\nStored Sessions:")
	fmt.Fprintln(out, "----------------")
	for _, s := range sessions {
		updated := "-"
		if !s.UpdatedAt.IsZero() {
			updated = s.UpdatedAt.Local().Format("2006-01-02 15:04")
		}
		fmt.Fprintf(out, "  %s\n", s.Key)
		fmt.Fprintf(out, "    Agent: %s  Messages: %d  Updated: %s\n", s.AgentID, s.Messages, updated)
	}
	return nil
}

// gatewayRunning reports the PID of a live gateway, or 0.
func gatewayRunning() int {
	if data := pid.ReadPidFileWithCheck(internal.GetPicoclawHome()); data != nil {
		return data.PID
	}
	return 0
}

func sessionClearCmd(out io.Writer, keys []string) error {
	if running := gatewayRunning(); running != 0 {
		return fmt.Errorf(
			"the gateway is running (PID %d) and writes to the same session files; "+
				"stop it first, or send /clear in the chat", running)
	}

	cfg, err := internal.LoadConfig()
	if err != nil {
		return fmt.Errorf("error loading config: %w", err)
	}

	cleared, err := agent.ClearStoredSessions(context.Background(), cfg, keys)
	for _, s := range cleared {
		fmt.Fprintf(out, "✓ Cleared %s (agent %s)\n", s.Key, s.AgentID)
	}
	if err != nil {
		return err
	}
	if len(cleared) == 0 {
		fmt.Fprintln(out, "No stored sessions.")
	}
	return nil
}
