package heartbeat

import (
	"bytes"
	"context"
	"errors"
	"os"
	"path/filepath"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"github.com/letrongminh/neuroclaw/pkg/config"
	"github.com/letrongminh/neuroclaw/pkg/pid"
	"github.com/letrongminh/neuroclaw/pkg/providers"
)

func TestNewHeartbeatCommand(t *testing.T) {
	cmd := NewHeartbeatCommand()

	require.NotNil(t, cmd)
	assert.Equal(t, "heartbeat", cmd.Use)
	assert.False(t, cmd.HasFlags())
	assert.Nil(t, cmd.Run)
	assert.NotNil(t, cmd.RunE)
	assert.True(t, cmd.HasSubCommands())

	subcommands := cmd.Commands()
	require.Len(t, subcommands, 1)
	run := subcommands[0]
	assert.Equal(t, "run", run.Name())
	assert.False(t, run.Hidden)
	assert.False(t, run.HasSubCommands())
	assert.Nil(t, run.Run)
	assert.NotNil(t, run.RunE)
	assert.NotNil(t, run.Flags().Lookup("force"))
}

func workspaceWithTask(t *testing.T) string {
	t.Helper()
	ws := t.TempDir()
	require.NoError(t, os.WriteFile(filepath.Join(ws, "HEARTBEAT.md"), []byte("- check the door"), 0o644))
	return ws
}

func TestRunHeartbeat_PrintsAnswer(t *testing.T) {
	var out bytes.Buffer
	err := runHeartbeat(&out, workspaceWithTask(t), 30,
		func(context.Context, string, string, string) (string, error) { return "The door is open", nil })
	require.NoError(t, err)
	assert.Contains(t, out.String(), "The door is open")

	out.Reset()
	err = runHeartbeat(&out, workspaceWithTask(t), 30,
		func(context.Context, string, string, string) (string, error) { return "HEARTBEAT_OK", nil })
	require.NoError(t, err)
	assert.Contains(t, out.String(), "nothing to report")
}

func TestRunHeartbeat_BudgetSkipExitsWithError(t *testing.T) {
	budgetErr := &providers.LLMBudgetExceededError{
		Window: "hourly", Limit: 1, Used: 1, ResetAt: time.Now().Add(time.Hour),
	}
	err := runHeartbeat(&bytes.Buffer{}, workspaceWithTask(t), 30,
		func(context.Context, string, string, string) (string, error) { return "", budgetErr })
	require.Error(t, err)
	assert.Contains(t, err.Error(), "heartbeat skipped")
	assert.Contains(t, err.Error(), "hourly LLM call budget")

	err = runHeartbeat(&bytes.Buffer{}, workspaceWithTask(t), 30,
		func(context.Context, string, string, string) (string, error) { return "", errors.New("boom") })
	require.Error(t, err)
	assert.Contains(t, err.Error(), "boom")
}

func TestRunHeartbeat_NoTasks(t *testing.T) {
	var out bytes.Buffer
	called := false
	err := runHeartbeat(&out, t.TempDir(), 30, func(context.Context, string, string, string) (string, error) {
		called = true
		return "", nil
	})
	require.NoError(t, err)
	assert.False(t, called)
	assert.Contains(t, out.String(), "Nothing to run")
}

func TestHeartbeatRunRefusesWhileGatewayRuns(t *testing.T) {
	home := t.TempDir()
	t.Setenv(config.EnvHome, home)
	_, err := pid.WritePidFile(home, "127.0.0.1", 18790) // this test process stands in for a gateway
	require.NoError(t, err)

	err = heartbeatRunCmd(&bytes.Buffer{}, false)
	require.Error(t, err)
	assert.Contains(t, err.Error(), "gateway is running")
	assert.Contains(t, err.Error(), "--force")
}
