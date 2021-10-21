# Copyright 2020 Vectorized, Inc.
#
# Use of this software is governed by the Business Source License
# included in the file licenses/BSL.md
#
# As of the Change Date specified in that file, in accordance with
# the Business Source License, use of this software will be governed
# by the Apache License, Version 2.0

import tempfile
from ducktape.mark import matrix, ignore
from ducktape.mark.resource import cluster
from ducktape.utils.util import wait_until

from rptest.clients.types import TopicSpec
from rptest.tests.redpanda_test import RedpandaTest
from rptest.clients.kafka_cli_tools import KafkaCliTools

import storage as vstorage


class PrefixTruncateRecoveryTest(RedpandaTest):
    """
    Verify that a kafka log that's been prefix truncated due to retention policy
    eventually converges with other raft group nodes.

    The high-level strategy for this test is to:

       1. Write some data into a three node cluster and wait for this data to be
       fully replicated on all nodes.

       2. jj
    """
    topics = (TopicSpec(cleanup_policy=TopicSpec.CLEANUP_DELETE), )

    def __init__(self, test_context):
        extra_rp_conf = dict(
            log_segment_size=1048576,
            retention_bytes=5242880,
            log_compaction_interval_ms=2000,
        )

        super(PrefixTruncateRecoveryTest,
              self).__init__(test_context=test_context,
                             num_brokers=3,
                             extra_rp_conf=extra_rp_conf)

        self.kafka_tools = KafkaCliTools(self.redpanda)
        self.deleted = None

    def fully_replicated(self, nodes):
        df = self.redpanda.metrics2(nodes, "vectorized_cluster_partition_under_replicated_replicas", "vectorized_cluster_partition_under_replicated_replicas")
        df = df[df['namespace'] == "kafka"]
        df = df[df['topic'] == self.topic]
        return (df["value"] == 0).all()

        assert len(df) == len(nodes)

    def produce_until_reclaim(self, stopped_node, acks):
        assert self.redpanda.nodes[0] == stopped_node

        df = self.redpanda.metrics2(self.redpanda.nodes[1:], "vectorized_storage_log_log_segments_removed", "vectorized_storage_log_log_segments_removed_total")
        df = df[df['namespace'] == "kafka"]
        df = df[df['topic'] == self.topic]
        assert len(df) == 2

        if self.deleted is None:
            self.deleted = max(df['value'])

        print("XXX", self.deleted)
        print(df["value"])

        if (df['value'] > (self.deleted + 3)).all():
            return True

        self.kafka_tools.produce(self.topic, 1024, 1024, acks=acks)
        return False

    @cluster(num_nodes=3)
    @matrix(acks=[-1, 1])
    def test_prefix_truncate_recovery(self, acks):
        # produce data into topic and wait until its fully replicated. this
        # isn't important other than to establish that the system initially
        # appears to be healthy at the start of the test.
        self.kafka_tools.produce(self.topic, 2048, 1024, acks=acks)
        wait_until(lambda: self.fully_replicated(self.redpanda.nodes),
                timeout_sec=30,
                backoff_sec=5)

        # stop an unfortunate node
        stopped_node = self.redpanda.nodes[0]
        self.redpanda.stop_node(stopped_node)

        # produce data into the topic until segments are reclaimed
        # by the configured retention policy
        wait_until(lambda: self.produce_until_reclaim(stopped_node, acks),
                timeout_sec=60,
                backoff_sec=5)

        # at this point the partition should be under replicated
        wait_until(lambda: not self.fully_replicated(self.redpanda.nodes[1:]),
                timeout_sec=5,
                backoff_sec=1)

        # finally restart the node and wait until fully replicated
        self.redpanda.start_node(stopped_node)
        wait_until(lambda: self.fully_replicated(self.redpanda.nodes),
                timeout_sec=60,
                backoff_sec=5)

        # stop one of the nodes
        #node = self.redpanda.controller()

        #self.kafka_tools.produce(self.topic, 1024, 1024, acks=acks)



        # produce data to the topic until we observe that the retention policy
        # has kicked in and one or more segments has been deleted.
        #self.produce_until_deleted(node)

        #self.verify_recovery(node)

    def produce_until_deleted(self, ignore_node):
        partitions = {}

        #
        # Produce until at least 3 segments per partition appear on disk.
        #
        def produce_until_segments(count):
            self.kafka_tools.produce(self.topic, 1000, 1000)
            storage = self.redpanda.storage()
            for p in storage.partitions("kafka", self.topic):
                if p.node == ignore_node:
                    continue
                if p.num not in partitions or len(
                        partitions[p.num].segments) < count:
                    partitions[p.num] = p
            self.logger.debug("Found partitions: %s", partitions)
            return partitions and all(
                map(lambda p: len(p[1].segments) >= count, partitions.items()))

        wait_until(lambda: produce_until_segments(3),
                   timeout_sec=60,
                   backoff_sec=1,
                   err_msg="Expected segments did not materialize")

        def make_segment_sets(partitions):
            return {
                p[0]: {s[0]
                       for s in p[1].segments.items()}
                for p in partitions.items()
            }

        orig_segments = make_segment_sets(partitions)
        self.logger.debug(f"Original segments: {orig_segments}")

        #
        # Continue producing until the original segments above have been deleted
        # because of the retention / cleanup policy.
        #
        def produce_until_segments_deleted():
            self.kafka_tools.produce(self.topic, 1000, 1000)
            storage = self.redpanda.storage()
            curr_segments = make_segment_sets(
                {p.num: p
                 for p in storage.partitions("kafka", self.topic)})
            for p, segs in orig_segments.items():
                self.logger.debug("Partition %d segment set intersection: %s",
                                  p, segs.intersection(curr_segments[p]))
                if not segs.isdisjoint(curr_segments[p]):
                    return False
            return True

        wait_until(lambda: produce_until_segments_deleted(),
                   timeout_sec=60,
                   backoff_sec=1,
                   err_msg="Original segments were not deleted")

    def verify_recovery(self, node):
        # repeat until true
        #  1. collect segment files from quroum members
        #  2. verify byte-for-byte equivalence of common range
        #  3. success
        with tempfile.TemporaryDirectory() as d:
            self.redpanda.copy_data(d, node)
            store = vstorage.Store(d)
            for ntp in store.ntps:
                for path in ntp.segments:
                    try:
                        s = vstorage.Segment(path)
                    except vstorage.CorruptBatchError as e:
                        print("corruption detected in batch {} of segment: {}".
                              format(e.batch.index, path))
                        print("header of corrupt batch: {}".format(
                            e.batch.header))
                        continue
                    print("successfully decoded segment: {}".format(path))
