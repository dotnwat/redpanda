// Copyright 2022 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

package maintenance

import (
    "fmt"
	"github.com/redpanda-data/redpanda/src/go/rpk/pkg/api/admin"
	"github.com/redpanda-data/redpanda/src/go/rpk/pkg/config"
	"github.com/redpanda-data/redpanda/src/go/rpk/pkg/out"
	"github.com/spf13/afero"
	"github.com/spf13/cobra"
)

func newStatusCommand(fs afero.Fs) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "status",
		Short: "Report maintenance status.",
		Long: `Report maintenance status.

This command reports maintenance status for each node in the cluster.
`,
		Args: cobra.ExactArgs(0),
		Run: func(cmd *cobra.Command, args []string) {
			p := config.ParamsFromCommand(cmd)
			cfg, err := p.Load(fs)
			out.MaybeDie(err, "unable to load config: %v", err)

			client, err := admin.NewClient(fs, cfg)
			out.MaybeDie(err, "unable to initialize admin client: %v", err)

			brokers, err := client.Brokers()
			out.MaybeDie(err, "unable to request brokers: %v", err)

			headers := []string{"Node-ID", "Maintenance Status", "Errors"}
			table := out.NewTable(headers...)
			defer table.Flush()

			for _, broker := range brokers {
				errors := "-"
				status := "disabled"
                fmt.Println(*broker.Maintenance)
				if broker.Maintenance.Draining {
					status = "enabled"
					if !broker.Maintenance.Finished {
						status = "draining"
					}
					errors = "none"
					if broker.Maintenance.Errors {
						errors = "yes"
					}
				}
				table.Print(broker.NodeID, status, errors)
			}
		},
	}
	return cmd
}
