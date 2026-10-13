package heartbeat

import (
	"context"
	"errors"
	"fmt"
	"io"

	"github.com/letrongminh/neuroclaw/cmd/neuroclaw/internal"
	"github.com/letrongminh/neuroclaw/pkg/agent"
	"github.com/letrongminh/neuroclaw/pkg/bus"
	"github.com/letrongminh/neuroclaw/pkg/config"
	"github.com/letrongminh/neuroclaw/pkg/heartbeat"
	"github.com/letrongminh/neuroclaw/pkg/logger"
	"github.com/letrongminh/neuroclaw/pkg/pid"
	"github.com/letrongminh/neuroclaw/pkg/providers"
	"github.com/letrongminh/neuroclaw/pkg/tools"
)

// gatewayRunning reports the PID of a live gateway, or 0.
func gatewayRunning() int {
	if data := pid.ReadPidFileWithCheck(internal.GetPicoclawHome()); data != nil {
		return data.PID
	}
	return 0
}

func heartbeatRunCmd(out io.Writer, force bool) error {
	if running := gatewayRunning(); running != 0 && !force {
		return fmt.Errorf(
			"the gateway is running (PID %d) and does its own heartbeats; running one here as well "+
				"could repeat their actions. Use --force to run anyway", running)
	}

	cfg, err := internal.LoadConfig()
	if err != nil {
		return fmt.Errorf("error loading config: %w", err)
	}
	logger.ConfigureFromEnv()
	config.LogRiskLabels(cfg)

	if !cfg.Heartbeat.Enabled {
		fmt.Fprintln(out, "Note: heartbeat.enabled is false; running once anyway.")
	}

	provider, modelID, err := providers.CreateProvider(cfg)
	if err != nil {
		return fmt.Errorf("error creating provider: %w", err)
	}
	if modelID != "" {
		cfg.Agents.Defaults.ModelName = modelID
	}

	msgBus := bus.NewMessageBus()
	defer msgBus.Close()
	agentLoop := agent.NewAgentLoop(cfg, msgBus, provider)
	defer agentLoop.Close()

	return runHeartbeat(out, cfg.WorkspacePath(), cfg.Heartbeat.Interval, agentLoop.ProcessHeartbeat)
}

type processHeartbeatFunc func(ctx context.Context, content, channel, chatID string) (string, error)

// runHeartbeat runs one heartbeat turn and prints its answer. The answer goes
// to the terminal, not a chat: no channel manager runs in this process.
func runHeartbeat(out io.Writer, workspace string, interval int, process processHeartbeatFunc) error {
	var turnErr error
	var response string
	hs := heartbeat.NewHeartbeatService(workspace, interval, true)
	hs.SetHandler(func(prompt, _, _ string) *tools.ToolResult {
		response, turnErr = process(context.Background(), prompt, "cli", "direct")
		if turnErr != nil {
			return tools.ErrorResult(turnErr.Error())
		}
		return tools.SilentResult(response)
	})

	_, err := hs.RunNow()
	if errors.Is(err, heartbeat.ErrNoHeartbeatTasks) {
		fmt.Fprintln(out, "Nothing to run: HEARTBEAT.md has no tasks below its marker line.")
		return nil
	}
	if err != nil {
		return err
	}

	var budgetErr *providers.LLMBudgetExceededError
	if errors.As(turnErr, &budgetErr) {
		return fmt.Errorf("heartbeat skipped: %w", budgetErr)
	}
	if turnErr != nil {
		return fmt.Errorf("heartbeat failed: %w", turnErr)
	}
	if response == "" || response == "HEARTBEAT_OK" {
		fmt.Fprintf(out, "%s Heartbeat OK: nothing to report.\n", internal.Logo)
		return nil
	}
	fmt.Fprintf(out, "%s %s\n", internal.Logo, response)
	return nil
}
