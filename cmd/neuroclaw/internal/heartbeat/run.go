package heartbeat

import "github.com/spf13/cobra"

func newRunCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:   "run",
		Short: "Run one heartbeat turn now and print the result",
		Long: `Run one heartbeat turn now with the current config and HEARTBEAT.md, in this
process, and print the result here instead of sending it to a chat. It runs even
when heartbeat.enabled is false. A running gateway is not involved: its LLM
budget counters are separate from this process's.`,
		Args: cobra.NoArgs,
		RunE: func(cmd *cobra.Command, _ []string) error {
			return heartbeatRunCmd(cmd.OutOrStdout())
		},
	}

	return cmd
}
