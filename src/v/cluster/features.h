#pragma once
#include "cluster/version.h"
#include "serde/serde.h"

#include <iosfwd>

namespace cluster {

struct feature_update_action
  : serde::envelope<
      feature_update_action,
      serde::version<0>,
      serde::compat_version<0>> {
    static constexpr int8_t current_version = 1;
    enum class action_t : std::uint16_t {
        // Notify when a feature is done with preparing phase
        complete_preparing = 1,
        // Notify when a feature is made available, either by an administrator
        // or via auto-activation policy
        activate = 2,
        // Notify when a feature is explicitly disabled by an administrator
        deactivate = 3
    };

    // Features have an internal bitflag representation, but it is not
    // meant to be stable for use on the wire, so we refer to features by name
    ss::sstring feature_name;
    action_t action;

    friend bool
    operator==(const feature_update_action&, const feature_update_action&)
      = default;

    auto serde_fields() { return std::tie(feature_name, action); }

    friend std::ostream&
    operator<<(std::ostream&, const feature_update_action&);
};

struct feature_update_cmd_data
  : serde::envelope<
      feature_update_cmd_data,
      serde::version<0>,
      serde::compat_version<0>> {
    // To avoid ambiguity on 'versions' here: `current_version`
    // is the encoding version of the struct, subsequent version
    // fields are the payload.
    static constexpr int8_t current_version = 1;

    cluster_version logical_version;
    std::vector<feature_update_action> actions;

    friend bool
    operator==(const feature_update_cmd_data&, const feature_update_cmd_data&)
      = default;

    auto serde_fields() { return std::tie(logical_version, actions); }

    friend std::ostream&
    operator<<(std::ostream&, const feature_update_cmd_data&);
};

} // namespace cluster
