// Copyright 2020 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "compat/raft_compat.h"
#include "test_utils/tmp_dir.h"

#include <seastar/core/thread.hh>
#include <seastar/testing/thread_test_case.hh>

#include <boost/test/unit_test.hpp>

SEASTAR_THREAD_TEST_CASE(compat_self_test) {
    temporary_dir corpus("compat_self_test");
    write_corpus(corpus.get_path()).get();
    read_corpus(corpus.get_path()).get();
}
