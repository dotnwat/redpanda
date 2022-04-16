// Copyright 2021 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

// Package brokers contains commands to talk to the Redpanda's admin brokers
// endpoints.
package brokers

import (
	"fmt"
	"reflect"
	"time"

	"github.com/redpanda-data/redpanda/src/go/rpk/pkg/api/admin"
	"github.com/redpanda-data/redpanda/src/go/rpk/pkg/config"
	"github.com/redpanda-data/redpanda/src/go/rpk/pkg/out"
	"github.com/spf13/afero"
	"github.com/spf13/cobra"
)

// NewCommand returns the brokers admin command.
func NewCommand(fs afero.Fs) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "cluster",
		Short: "Interact with redpanda cluster API",
		Args:  cobra.ExactArgs(0),
	}
	cmd.AddCommand(
		newOverviewCommand(fs),
	)
	return cmd
}

func newOverviewCommand(fs afero.Fs) *cobra.Command {
	var (
		wait bool
		exit bool
	)
	cmd := cobra.Command{
		Use:   "health",
		Short: "Queries cluster for health overview.",
		Args:  cobra.ExactArgs(0),
		Run: func(cmd *cobra.Command, _ []string) {
			p := config.ParamsFromCommand(cmd)
			cfg, err := p.Load(fs)
			out.MaybeDie(err, "unable to load config: %v", err)

			cl, err := admin.NewClient(fs, cfg)
			out.MaybeDie(err, "unable to initialize admin client: %v", err)

			var last_overview admin.ClusterHealthOverview
			for {
				ret, err := cl.GetHealthOverview()
				out.MaybeDie(err, "unable to request cluster health: %v", err)
				if !reflect.DeepEqual(ret, last_overview) {
					printHealthOverview(&ret)
				}
				last_overview = ret
				if !wait || exit && last_overview.IsHealthy {
					break
				} else {
					time.Sleep(2 * time.Second)
				}
			}

		},
	}
	cmd.Flags().BoolVarP(&wait, "wait", "w", false, "blocks and writes out all cluster health changes")
	cmd.Flags().BoolVarP(&exit, "exit-when-healthy", "e", false, "when used with wait exits after cluster is back in healthy state")
	return &cmd
}

func printHealthOverview(hov *admin.ClusterHealthOverview) {
	fmt.Println("CLUSTER HEALTH OVERVIEW")
	fmt.Println("=======================")
	fmt.Printf("Healthy:               %v\n", hov.IsHealthy)
	fmt.Printf("Controller ID:         %v\n", hov.ControllerID)
	fmt.Printf("Nodes down:            %v\n", hov.NodesDown)
	fmt.Printf("Leaderless partitions: %v\n", hov.LeaderlessPartitions)
	fmt.Println()
}
