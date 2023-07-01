# Copyright 2023 Redpanda Data, Inc.
#
# Use of this software is governed by the Business Source License
# included in the file licenses/BSL.md
#
# As of the Change Date specified in that file, in accordance with
# the Business Source License, use of this software will be governed
# by the Apache License, Version 2.0

import concurrent.futures
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
        return self.redpanda.data_usage("kafka", node)

    @cluster(num_nodes=4)
    @matrix(log_segment_size=[1024 * 1024])
    def streaming_cache_test(self, log_segment_size):
        if self.redpanda.dedicated_nodes:
            partition_count = 128
            rate_limit_bps = 100 * 1024 * 1024
            target_size = 1 * 1024 * 1024 * 1024
            data_size = 10 * target_size
        else:
            partition_count = 16
            rate_limit_bps = 10 * 1024 * 1024
            target_size = 250 * 1024 * 1024
            data_size = 2 * target_size

        msg_size = 16384
        msg_count = data_size // msg_size
        topic_name = "log-storage-target-size-topic"

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

        # Sanity check test parameters against the nodes we are running on
        disk_space_required = data_size
        assert self.redpanda.get_node_disk_free(
        ) >= disk_space_required, f"Need at least {disk_space_required} bytes space"

        # create the target topic
        rpk = RpkTool(self.redpanda)
        rpk.create_topic(
            topic_name,
            partitions=partition_count,
            replicas=3)

        # setup and start the background producer
        producer = KgoVerifierProducer(
            self.test_context,
            self.redpanda,
            topic_name,
            msg_size=msg_size,
            msg_count=msg_count,
            rate_limit_bps=rate_limit_bps,
            batch_max_bytes=msg_size * 8)
        producer.start()
        produce_start_time = time.time()

        max_totals = None
        last_report_time = 0
        while producer.produce_status.acked < msg_count:
            # calculate and report disk usage across nodes
            if time.time() - last_report_time >= 10:
                with concurrent.futures.ThreadPoolExecutor(max_workers=len(self.redpanda.nodes)) as executor:
                    totals = list(executor.map(lambda n: self._kafka_size_on_disk(n), self.redpanda.nodes))
                if not max_totals:
                    max_totals = totals
                targets = [target_size] * len(totals)
                cur_overages = [a - b for a, b in zip(totals, targets)]
                max_overages = [a - b for a, b in zip(max_totals, targets)]
                max_totals = [max(a, b) for a, b in zip(max_totals, totals)]
                h = lambda bs: [round(t / (1024 * 1024), 1) for t in bs]
                self.logger.info(f" Acked msgs {producer.produce_status.acked}")
                self.logger.info(f"Total usage {h(totals)}")
                self.logger.info(f"  Max usage {h(max_totals)}")
                self.logger.info(f"Cur overage {h(cur_overages)}")
                self.logger.info(f"Max overage {h(max_overages)}")
                last_report_time = time.time()
            # don't sleep long--we want good responsivenses when producing is
            # completed so that our calculated bandwidth is accurate
            time.sleep(1)

        # we shouldn't start waiting until it looks like we are done, so timeout
        # is only really used here as a backup if something get stucks.
        producer.wait(timeout_sec=30)
        produce_duration = time.time() - produce_start_time
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


        # there is a reasonable minimum based on several factors such as how
        # how big the segment size is multiplied by partition count because we
        # won't collect the active segment. other factors too. actually kind of
        # a complicated model. target size needs to take into account the case
        # where we almost fill up an entire segment but it doesn't roll and we
        # also don't wait for automatic rolling etc... either segmetn ms or the
        # max data age from the cloud layer
        #target_size = 400 * 1024 * 1024
        #target_size = max(target_size, partition_count * log_segment_size)





