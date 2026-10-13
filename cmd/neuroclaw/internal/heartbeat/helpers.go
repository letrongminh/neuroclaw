package heartbeat

import (
	"errors"
	"fmt"
	"io"

	"github.com/letrongminh/neuroclaw/cmd/neuroclaw/internal"
	"github.com/letrongminh/neuroclaw/pkg/agent"
	"github.com/letrongminh/neuroclaw/pkg/bus"
	"github.com/letrongminh/neuroclaw/pkg/config"
	"github.com/letrongminh/neuroclaw/pkg/gateway"
	"github.com/letrongminh/neuroclaw/pkg/heartbeat"
	"github.com/letrongminh/neuroclaw/pkg/logger"
	"github.com/letrongminh/neuroclaw/pkg/providers"
	"github.com/letrongminh/neuroclaw/pkg/tools"
)

func heartbeatRunCmd(out io.Writer) error {
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

	hs := heartbeat.NewHeartbeatService(cfg.WorkspacePath(), cfg.Heartbeat.Interval, true)
	handler := gateway.NewHeartbeatHandler(agentLoop)
	// Answer here, not in a chat: no channel manager runs in this process.
	hs.SetHandler(func(prompt, _, _ string) *tools.ToolResult {
		return handler(prompt, "cli", "direct")
	})

	result, err := hs.RunNow()
	if errors.Is(err, heartbeat.ErrNoHeartbeatTasks) {
		fmt.Fprintln(out, "Nothing to run: HEARTBEAT.md has no tasks below its marker line.")
		return nil
	}
	if err != nil {
		return err
	}
	return printResult(out, result)
}

func printResult(out io.Writer, result *tools.ToolResult) error {
	if result == nil {
		fmt.Fprintln(out, "Heartbeat returned no result.")
		return nil
	}
	if result.IsError {
		return errors.New(result.ForLLM)
	}
	text := result.ForUser
	if text == "" {
		text = result.ForLLM
	}
	fmt.Fprintf(out, "%s %s\n", internal.Logo, text)
	return nil
}
