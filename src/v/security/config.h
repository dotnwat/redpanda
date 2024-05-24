#pragma once

#include "base/seastarx.h"

#include <seastar/core/sstring.hh>

#include <optional>
#include <vector>

namespace security::tls {

std::optional<ss::sstring>
validate_rules(const std::optional<std::vector<ss::sstring>>& r) noexcept;

}

namespace security::oidc {

std::optional<ss::sstring>
validate_principal_mapping_rule(ss::sstring const& rule);

}
