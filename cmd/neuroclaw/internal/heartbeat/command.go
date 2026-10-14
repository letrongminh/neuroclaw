package heartbeat

import (
	"github.com/spf13/cobra"
)

func NewHeartbeatCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:   "heartbeat",
		Short: "Run heartbeat checks from HEARTBEAT.md",
		Args:  cobra.NoArgs,
		RunE: func(cmd *cobra.Command, _ []string) error {
			return cmd.Help()
		},
	}

	cmd.AddCommand(newRunCommand())

	return cmd
}
