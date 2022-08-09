# Copyright 2022 Redpanda Data, Inc.
#
# Use of this software is governed by the Business Source License
# included in the file licenses/BSL.md
#
# As of the Change Date specified in that file, in accordance with
# the Business Source License, use of this software will be governed
# by the Apache License, Version 2.0
import time
import random as mod_random
from rptest.services.cluster import cluster
from rptest.services.redpanda import RESTART_LOG_ALLOW_LIST
from rptest.clients.types import TopicSpec
from rptest.tests.end_to_end import EndToEndTest
from ducktape.mark import matrix

# seed an instance of the random generator dedicated to this test so we can
# reproduce its choices based on the seed. it is perfect: don't use threads to
# access this object in the test, but it isn't affected by other calls to the
# global random module (e.g. random.randint). python doesn't seem to guarantee
# that this all works across python versions, but does guarantee that if things
# change they'll provide a backwards compatible seeding mechanism. so far it
# doesn't seem this has been necessary.
_random_seed = int(time.time())
r_random = mod_random.Random(_random_seed)

class BrokenConnectionTest(EndToEndTest):
    def __init__(self, ctx, *args, **kwargs):
        super(BrokenConnectionTest, self).__init__(
            ctx,
            *args,
            extra_rp_conf={
                'enable_leader_balancer': False
            },
            **kwargs)
        self._ctx = ctx
        self.logger.info(f"Random seed: {_random_seed}")

    def _node_ip(self, node):
        """
        Resolve a node's IP address from inside the redpanda service network.
        """
        out = self.redpanda.nodes[0].account.ssh_output(f"getent hosts {node.name}")
        return out.split()[0].decode()

    def _make_rule(self, ip):
        reject_types = ["icmp-host-unreachable", "tcp-reset"]
        chains = ["INPUT", "OUTPUT"]
        rule_targets = ["DROP", "REJECT"]

        chain = r_random.choice(chains)
        reject_type = r_random.choice(reject_types)
        rule_target = r_random.choice(rule_targets)

        rule = f"{chain} -s {ip} -p tcp --dport ${self.redpanda.REDPANDA_RPC_PORT} "
        if rule_target == "DROP":
            rule += "-j DROP"
        elif rule_target == "REJECT":
            rule += f"-j REJECT --reject-with {reject_type}"
        else:
            assert False, f"Hi, what is this target: {rule_target}?"
        return rule

    @cluster(num_nodes=5,
             log_allow_list=RESTART_LOG_ALLOW_LIST)
    def broken_connection_test(self):
        self.start_redpanda(num_nodes=3,
                            extra_rp_conf={
                                "default_topic_replications": 3,
                            })

        spec = TopicSpec(partition_count=20, replication_factor=3)
        self.client().create_topic(spec)
        self.topic = spec.name

        self.start_producer(1, throughput=10000)
        self.start_consumer(1)
        self.await_startup()

        node_ips = [(node, self._node_ip(node)) for node in self.redpanda.nodes]

        for _ in range(20):
            rule_on = r_random.choice(node_ips)
            target = r_random.choice(node_ips)
            rule = self._make_rule(target[1])
            self.logger.info(f"Applying rule on {rule_on[0].name}[{rule_on[1]}]: {rule}")
            rule_on[0].account.ssh_output(f"iptables -A {rule}")
            time.sleep(r_random.choice(range(10)))
            rule_on[0].account.ssh_output(f"iptables -D {rule}")
            time.sleep(r_random.choice(range(10)))
            x = self.redpanda.metrics_sample("read_dispatch_errors")
            self.logger.info(f"XXXXX: {x.samples}")

        self.run_validation(enable_idempotence=False,
                            consumer_timeout_sec=45,
                            min_records=10000)
