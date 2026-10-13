package session

import (
	"bytes"
	"context"
	"os"
	"path/filepath"
	"slices"
	"testing"

	"github.com/spf13/cobra"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"github.com/letrongminh/neuroclaw/pkg/config"
	"github.com/letrongminh/neuroclaw/pkg/memory"
	"github.com/letrongminh/neuroclaw/pkg/pid"
)

func TestNewSessionCommand(t *testing.T) {
	cmd := NewSessionCommand()

	require.NotNil(t, cmd)
	assert.Equal(t, "session", cmd.Use)
	assert.False(t, cmd.HasFlags())
	assert.Nil(t, cmd.Run)
	assert.NotNil(t, cmd.RunE)
	assert.True(t, cmd.HasSubCommands())

	allowedCommands := []string{"list", "clear"}
	subcommands := cmd.Commands()
	assert.Len(t, subcommands, len(allowedCommands))
	for _, subcmd := range subcommands {
		assert.True(t, slices.Contains(allowedCommands, subcmd.Name()), "unexpected subcommand %q", subcmd.Name())
		assert.False(t, subcmd.Hidden)
		assert.False(t, subcmd.HasSubCommands())
		assert.Nil(t, subcmd.Run)
		assert.NotNil(t, subcmd.RunE)
	}
}

func TestClearCommandArgs(t *testing.T) {
	for _, tc := range []struct {
		args []string
		ok   bool
	}{
		{args: []string{"agent:main:x"}, ok: true},
		{args: []string{"--all"}, ok: true},
		{args: []string{}, ok: false},
		{args: []string{"a", "b"}, ok: false},
		{args: []string{"--all", "a"}, ok: false},
	} {
		cmd := newClearCommand()
		cmd.RunE = func(*cobra.Command, []string) error { return nil }
		cmd.SetArgs(tc.args)
		cmd.SetOut(&bytes.Buffer{})
		cmd.SetErr(&bytes.Buffer{})
		err := cmd.Execute()
		assert.Equal(t, tc.ok, err == nil, "args %v: err = %v", tc.args, err)
	}
}

// setupHome points NEUROCLAW_HOME and the config at a temp dir and returns a
// session store in the default workspace.
func setupHome(t *testing.T) (string, *memory.JSONLStore) {
	t.Helper()
	home := t.TempDir()
	t.Setenv(config.EnvHome, home)
	configPath := filepath.Join(home, "config.json")
	t.Setenv(config.EnvConfig, configPath)

	cfg := config.DefaultConfig()
	cfg.Agents.Defaults.Workspace = filepath.Join(home, "workspace")
	require.NoError(t, config.SaveConfig(configPath, cfg))

	store, err := memory.NewJSONLStore(filepath.Join(home, "workspace", "sessions"))
	require.NoError(t, err)
	require.NoError(t, store.AddMessage(context.Background(), "agent:main:x", "user", "hello"))
	return home, store
}

func TestSessionListAndClear(t *testing.T) {
	_, store := setupHome(t)

	var out bytes.Buffer
	require.NoError(t, sessionListCmd(&out))
	assert.Contains(t, out.String(), "agent:main:x")
	assert.Contains(t, out.String(), "Messages: 1")

	out.Reset()
	require.NoError(t, sessionClearCmd(&out, []string{"agent:main:x"}))
	assert.Contains(t, out.String(), "Cleared agent:main:x")
	history, err := store.GetHistory(context.Background(), "agent:main:x")
	require.NoError(t, err)
	assert.Empty(t, history)
}

func TestSessionClearRefusesWhileGatewayRuns(t *testing.T) {
	home, store := setupHome(t)
	_, err := pid.WritePidFile(home, "127.0.0.1", 18790) // this test process stands in for a gateway
	require.NoError(t, err)
	t.Cleanup(func() { _ = os.Remove(filepath.Join(home, ".neuroclaw.pid")) })

	err = sessionClearCmd(&bytes.Buffer{}, nil)
	require.Error(t, err)
	assert.Contains(t, err.Error(), "gateway is running")
	history, herr := store.GetHistory(context.Background(), "agent:main:x")
	require.NoError(t, herr)
	assert.Len(t, history, 1)
}
