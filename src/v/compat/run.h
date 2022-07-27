#pragma once

#include "seastarx.h"

#include <seastar/core/future.hh>

#include <filesystem>

ss::future<> write_corpus(std::filesystem::path);
ss::future<> read_corpus(std::filesystem::path);
