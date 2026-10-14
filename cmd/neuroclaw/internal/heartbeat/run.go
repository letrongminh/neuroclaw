package heartbeat

import "github.com/spf13/cobra"

func newRunCommand() *cobra.Command {
	var force bool

	cmd := &cobra.Command{
		Use:   "run",
		Short: "Run one heartbeat turn now and print the result",
		Long: `Run one heartbeat turn now with the current config and HEARTBEAT.md, in this
process, and print the result here instead of sending it to a chat. It runs even
when heartbeat.enabled is false.

A running gateway does its own heartbeats; running one here as well could repeat
their actions, so this command refuses while a gateway is running unless --force
is given. Its LLM budget counters are separate from the gateway's. When the LLM
budget is used up, the turn is skipped and the command exits with an error.`,
		Example: `neuroclaw heartbeat run
neuroclaw heartbeat run --force`,
		Args: cobra.NoArgs,
		RunE: func(cmd *cobra.Command, _ []string) error {
			return heartbeatRunCmd(cmd.OutOrStdout(), force)
		},
	}

	cmd.Flags().BoolVar(&force, "force", false, "Run even while a gateway is running")

	return cmd
}
