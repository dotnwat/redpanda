#pragma once
#include "cluster/metadata_dissemination_types.h"
#include "model/tests/randoms.h"
#include "test_utils/randoms.h"

namespace tests {

template<>
struct instance_generator<cluster::update_leadership_request> {
    static cluster::update_leadership_request random() {
        return cluster::update_leadership_request({
          cluster::ntp_leader(
            model::random_ntp(),
            random_named_int<model::term_id>(),
            random_named_int<model::node_id>()),
        });
    }

    static std::vector<cluster::update_leadership_request> limits() {
        return {};
    }
};

} // namespace tests
