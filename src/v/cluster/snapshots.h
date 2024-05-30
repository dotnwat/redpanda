#pragma once

#include <seastar/core/sstring.hh>

namespace cluster {

/// Names of snapshot files used by stm's
static const ss::sstring archival_stm_snapshot = "archival_metadata.snapshot";
static const ss::sstring rm_stm_snapshot = "tx.snapshot";
static const ss::sstring tm_stm_snapshot = "tx.coordinator.snapshot";
static const ss::sstring id_allocator_snapshot = "id.snapshot";
static const ss::sstring tx_registry_snapshot = "tx_registry.snapshot";

} // namespace cluster
