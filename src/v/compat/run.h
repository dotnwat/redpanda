#pragma once

#include "seastarx.h"

#include <seastar/core/future.hh>

#include <filesystem>

namespace compat {

ss::future<> write_corpus(const std::filesystem::path&);
ss::future<> check_type(const std::filesystem::path&);

} // namespace compat
