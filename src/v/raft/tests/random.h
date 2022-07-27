#pragma once
#include "raft/types.h"
#include "test_utils/randoms.h"

namespace tests {

template<>
struct instance_generator<raft::vnode> {
    static raft::vnode random() {
        return {
          random_named_int<model::node_id>(),
          random_named_int<model::revision_id>()};
    }

    static std::vector<raft::vnode> limits() {
        return {
          {model::node_id::min(), model::revision_id::min()},
          {model::node_id::max(), model::revision_id::max()},
          {model::node_id{0}, model::revision_id{0}},
        };
    }
};

template<>
struct instance_generator<raft::timeout_now_request> {
    static raft::timeout_now_request random() {
        return {
          .target_node_id = instance_generator<raft::vnode>::random(),
          .node_id = instance_generator<raft::vnode>::random(),
          .group = random_named_int<raft::group_id>(),
          .term = random_named_int<model::term_id>(),
        };
    }

    static std::vector<raft::timeout_now_request> limits() {
        return {
          {
            .target_node_id = instance_generator<raft::vnode>::random(),
            .node_id = instance_generator<raft::vnode>::random(),
            .group = raft::group_id::min(),
            .term = model::term_id::min(),
          },
          {
            .target_node_id = instance_generator<raft::vnode>::random(),
            .node_id = instance_generator<raft::vnode>::random(),
            .group = raft::group_id::max(),
            .term = model::term_id::max(),
          },
        };
    }
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

    static std::vector<raft::timeout_now_reply> limits() {
        return {
          {
            .target_node_id = instance_generator<raft::vnode>::random(),
            .term = model::term_id::min(),
            .result = raft::timeout_now_reply::status::success,
          },
          {
            .target_node_id = instance_generator<raft::vnode>::random(),
            .term = model::term_id::max(),
            .result = raft::timeout_now_reply::status::failure,
          },
        };
    }
};

} // namespace tests
