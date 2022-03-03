# Copyright 2022 Vectorized, Inc.
#
# Use of this software is governed by the Business Source License
# included in the file licenses/BSL.md
#
# As of the Change Date specified in that file, in accordance with
# the Business Source License, use of this software will be governed
# by the Apache License, Version 2.0

import random

from rptest.services.admin import Admin
from rptest.tests.redpanda_test import RedpandaTest
from rptest.services.cluster import cluster
from rptest.clients.types import TopicSpec
from ducktape.utils.util import wait_until


class MaintenanceTest(RedpandaTest):
    topics = (TopicSpec(partition_count=10, replication_factor=3),
              TopicSpec(partition_count=20, replication_factor=3))

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.admin = Admin(self.redpanda)

    def _leader_count(self, node):
        partitions = self.admin.get_partitions(node=node)
        return len(list(filter(lambda p: p["is_leader"], partitions)))

    def _maintenance_status(self, node, expected):
        status = self.admin.maintenance_status(node)
        return status["draining"] == expected

    @cluster(num_nodes=5)
    def test_maintenance(self):
        target = random.choice(self.redpanda.nodes)

        # target is leader for some partitions
        wait_until(lambda: self._leader_count(target) > 0,
                   timeout_sec=60,
                   backoff_sec=10)

        # target is not in a maintenance state
        status = self.admin.maintenance_status(target)
        assert status[
            "draining"] == False, f"Node {target.name} has unexpected maintenance state {status['draining']}"

        self.admin.maintenance_start(target)

        # target is now in maintenance state
        wait_until(
            lambda: self._maintenance_status(target, True),
            timeout_sec=30,
            backoff_sec=5,
            err_msg=f"Node {target.name} failed expected maintenance state True"
        )

        # wait for no more leaders
        wait_until(lambda: self._leader_count(target) == 0,
                   timeout_sec=60,
                   backoff_sec=10)

        def maintenance_finished():
            status = self.admin.maintenance_status(target)
            self.logger.debug(f"maintenance status: {status}")
            return status["finished"] and not status["errors"] and \
                    status["partitions"] > 0

        # status should now reflect completion of maintenance
        wait_until(maintenance_finished, timeout_sec=60, backoff_sec=10)

        # stop maintenance. leadership rebalancer should move some leaders back
        self.admin.maintenance_stop(target)

        # target is now not in maintenance state
        wait_until(
            lambda: self._maintenance_status(target, False),
            timeout_sec=30,
            backoff_sec=5,
            err_msg=
            f"Node {target.name} failed expected maintenance state False")

        wait_until(lambda: self._leader_count(target) > 0,
                   timeout_sec=120,
                   backoff_sec=10)
