# Copyright 2023 Redpanda Data, Inc.
#
# Use of this software is governed by the Business Source License
# included in the file licenses/BSL.md
#
# As of the Change Date specified in that file, in accordance with
# the Business Source License, use of this software will be governed
# by the Apache License, Version 2.0

from rptest.tests.redpanda_test import RedpandaTest
from rptest.services.cluster import cluster
from rptest.services.kgo_verifier_services import KgoVerifierProducer, KgoVerifierSeqConsumer
from rptest.services.redpanda import SISettings, MetricsEndpoint, ResourceSettings
from rptest.services.admin import Admin
from rptest.clients.rpk import RpkTool
from rptest.utils.si_utils import quiesce_uploads
from ducktape.utils.util import wait_until
from typing import Optional
from ducktape.mark import matrix
import time
from enum import Enum


class LogStorageTargetSizeTest(RedpandaTest):
    segment_upload_interval = 30
    manifest_upload_interval = 10

    def __init__(self, test_context, *args, **kwargs):
        super().__init__(test_context, *args, **kwargs)

    def setUp(self):
        # defer redpanda startup to the test
        pass

    def _kafka_size_on_disk(self, node):
        total_bytes = 0
        observed = list(self.redpanda.data_stat(node))
        for file, size in observed:
            if len(file.parents) == 1:
                continue
            if file.parents[-2].name == "kafka":
                total_bytes += size
        return total_bytes

    @cluster(num_nodes=4)
    @matrix(log_segment_size=[1024 * 1024])
    def streaming_cache_test(self, log_segment_size):
        topic_name = "target-size-topic"
        partition_count = 16
        replica_count = 3
        msg_size = 16384
        data_size = 1 * int(1000E6)
        msg_count = data_size // msg_size
        rate_limit_bps = int(10E6)

        # should be estimated based on size and throughput
        estimated_produce_duration = 160

        # there is a reasonable minimum based on several factors such as how
        # how big the segment size is multiplied by partition count because we
        # won't collect the active segment. other factors too. actually kind of
        # a complicated model. target size needs to take into account the case
        # where we almost fill up an entire segment but it doesn't roll and we
        # also don't wait for automatic rolling etc... either segmetn ms or the
        # max data age from the cloud layer
        target_size = 400 * 1024 * 1024
        target_size = max(target_size, partition_count * log_segment_size)

        # configure and start redpanda
        extra_rp_conf = {
            'cloud_storage_segment_max_upload_interval_sec':
            self.segment_upload_interval,
            'cloud_storage_manifest_max_upload_interval_sec':
            self.manifest_upload_interval,
            'log_storage_target_size': target_size,
        }
        si_settings = SISettings(test_context=self.test_context,
                                 log_segment_size=log_segment_size)
        self.redpanda.set_extra_rp_conf(extra_rp_conf)
        self.redpanda.set_si_settings(si_settings)
        self.redpanda.start()

        # topic setup
        rpk = RpkTool(self.redpanda)
        rpk.create_topic(
            topic_name,
            partitions=partition_count,
            replicas=3)

        # Sanity check test parameters against the nodes we are running on
        disk_space_required = data_size
        assert self.redpanda.get_node_disk_free(
        ) >= disk_space_required, f"Need at least {disk_space_required} bytes space"

        producer = KgoVerifierProducer(
            self.test_context,
            self.redpanda,
            topic_name,
            msg_size=msg_size,
            msg_count=msg_count,
            rate_limit_bps=rate_limit_bps,
            batch_max_bytes=msg_size * 8)

        producer.start()
        start_time = time.time()

        max_totals = None
        last_time = time.time()
        while producer.produce_status.acked < msg_count:
            totals = [self._kafka_size_on_disk(n) for n in self.redpanda.nodes]
            if not max_totals:
                max_totals = totals

            targets = [target_size] * len(totals)
            cur_overages = [a - b for a, b in zip(totals, targets)]
            max_overages = [a - b for a, b in zip(max_totals, targets)]
            max_totals = [max(a, b) for a, b in zip(max_totals, totals)]

            def h(bs):
                return [round(t / (1024 * 1024), 1) for t in bs]

            acked = producer.produce_status.acked
            self.logger.debug(f"XXXXXX acked {acked} total {h(totals)} max {h(max_totals)}")
            self.logger.debug(f"XXXXXX curr overage {h(cur_overages)} max overage {h(max_overages)}")

            report_delay = 5 - (time.time() - last_time)
            time.sleep(max(report_delay, 0))

        producer.wait(timeout_sec=estimated_produce_duration)
        produce_duration = time.time() - start_time
        self.logger.info(
            f"Produced x {data_size} bytes in {produce_duration} seconds, {(data_size/produce_duration)/1000000.0:.2f}MB/s"
        )
        producer.stop()
        producer.free()

        def target_size_reached():
            totals = [self._kafka_size_on_disk(n) for n in self.redpanda.nodes]
            targets = [target_size + 3 * log_segment_size] * len(totals)
            overages = [a - b for a, b in zip(totals, targets)]
            targets_met = all(t <= 0 for t in overages)
            self.logger.debug(f"Totals {totals} targets {targets} overages {overages} met {targets_met}")
            return targets_met

        wait_until(target_size_reached, timeout_sec=120, backoff_sec=5)
