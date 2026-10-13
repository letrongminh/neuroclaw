package heartbeat

import (
	"bytes"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"github.com/letrongminh/neuroclaw/pkg/tools"
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
}

func TestPrintResult(t *testing.T) {
	var out bytes.Buffer
	require.NoError(t, printResult(&out, tools.SilentResult("HEARTBEAT_OK seen")))
	assert.Contains(t, out.String(), "HEARTBEAT_OK seen")

	err := printResult(&out, tools.ErrorResult("Heartbeat error: boom"))
	require.Error(t, err)
	assert.Contains(t, err.Error(), "boom")

	out.Reset()
	require.NoError(t, printResult(&out, nil))
	assert.Contains(t, out.String(), "no result")
}
