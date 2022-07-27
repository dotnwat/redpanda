#pragma once
#include "model/fundamental.h"
#include "raft/types.h"
#include "test_utils/randoms.h"

template<typename T>
struct instance_generator {
    static T random();
    static std::vector<T> limits();
};

template<>
struct instance_generator<raft::vnode> {
    static raft::vnode random() {
        return {
          ::tests::random_named_int<model::node_id>(),
          ::tests::random_named_int<model::revision_id>()};
    }

    static std::vector<raft::vnode> limits() { return {}; }
};

template<>
struct instance_generator<raft::timeout_now_request> {
    static raft::timeout_now_request random() {
        return {
          .target_node_id = instance_generator<raft::vnode>::random(),
          .node_id = instance_generator<raft::vnode>::random(),
          .group = tests::random_named_int<raft::group_id>(),
          .term = tests::random_named_int<model::term_id>(),
        };
    }

    static std::vector<raft::timeout_now_request> limits() { return {}; }
};

template<>
struct instance_generator<raft::timeout_now_reply> {
    static raft::timeout_now_reply random() {
        return {
          .target_node_id = instance_generator<raft::vnode>::random(),
          .term = tests::random_named_int<model::term_id>(),
          .result = random_generators::random_choice(
            {raft::timeout_now_reply::status::success,
             raft::timeout_now_reply::status::failure}),
        };
    }

    static std::vector<raft::timeout_now_reply> limits() { return {}; }
};
