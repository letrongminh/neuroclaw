package session

import (
	"errors"

	"github.com/spf13/cobra"
)

func newClearCommand() *cobra.Command {
	var all bool

	cmd := &cobra.Command{
		Use:   "clear <key> | --all",
		Short: "Clear a session's history (the gateway must be stopped)",
		Long: `Clear the history and summary of a stored session, as /clear does in chat.

The gateway reads and appends to the same session files while it runs, so a
clear made from outside could be written over mid-turn. This command refuses to
run while a gateway is running: stop it first, or send /clear in the chat.`,
		Example: `neuroclaw session clear agent:main:telegram:direct:123
neuroclaw session clear --all`,
		Args: func(_ *cobra.Command, args []string) error {
			if all && len(args) > 0 {
				return errors.New("give a session key or --all, not both")
			}
			if !all && len(args) != 1 {
				return errors.New("give one session key, or --all")
			}
			return nil
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			var keys []string
			if !all {
				keys = args
			}
			return sessionClearCmd(cmd.OutOrStdout(), keys)
		},
	}

	cmd.Flags().BoolVar(&all, "all", false, "Clear every stored session")

	return cmd
}
