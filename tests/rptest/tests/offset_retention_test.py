# Copyright 2022 Redpanda Data, Inc.
#
# Use of this software is governed by the Business Source License
# included in the file licenses/BSL.md
#
# As of the Change Date specified in that file, in accordance with
# the Business Source License, use of this software will be governed
# by the Apache License, Version 2.0
import os
import re
import time

from ducktape.utils.util import wait_until
from rptest.utils.rpenv import sample_license
from rptest.services.admin import Admin
from ducktape.utils.util import wait_until
from rptest.tests.redpanda_test import RedpandaTest
from rptest.services.redpanda import SISettings
from rptest.services.cluster import cluster
from requests.exceptions import HTTPError
from rptest.services.redpanda import RESTART_LOG_ALLOW_LIST
from rptest.services.redpanda_installer import RedpandaInstaller, wait_for_num_versions

class OffsetRetentionConfigUpgradeTest(RedpandaTest):
    def __init__(self, test_context):
        super(OffsetRetentionConfigUpgradeTest, self).__init__(test_context=test_context, num_brokers=3)

    @cluster(num_nodes=3)
    def test_upgrade(self):
        pass
