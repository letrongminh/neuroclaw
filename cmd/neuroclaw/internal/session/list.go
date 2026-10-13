package session

import "github.com/spf13/cobra"

func newListCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:   "list",
		Short: "List sessions stored in the agents' workspaces",
		Args:  cobra.NoArgs,
		RunE: func(cmd *cobra.Command, _ []string) error {
			return sessionListCmd(cmd.OutOrStdout())
		},
	}

	return cmd
}
