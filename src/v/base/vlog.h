/*
 * Copyright 2020 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#pragma once
#include "base/source_location.h"
#include "container/intrusive_list_helpers.h"

#include <cstdint>
#include <string>

namespace vlog {

struct waypoint;

// waypoint_user is a thread-local structure at each waypoint call site that
// tracks the number of times the waypoint is currently active on the call
// stack. Each thread maintains its own list of waypoint_users for enumeration.
struct waypoint_user {
    const waypoint* wp{nullptr};
    const std::string* name{nullptr};
    uint64_t count{0};
    intrusive_list_hook hook_;

    static inline thread_local intrusive_list<waypoint_user, &waypoint_user::hook_>
      users_;

    explicit waypoint_user(const waypoint* w)
      : wp(w) {
        users_.push_back(*this);
    }

    waypoint_user(const waypoint_user&) = delete;
    waypoint_user& operator=(const waypoint_user&) = delete;
    waypoint_user(waypoint_user&&) = delete;
    waypoint_user& operator=(waypoint_user&&) = delete;
    ~waypoint_user() = default;
};

// A waypoint is a static marker placed in the code that can be enumerated
// at runtime. Waypoints are placed in a dedicated ELF section and can be
// iterated over using waypoint_iterator.
struct waypoint {
    const char* filename;
    unsigned line;
};

// RAII guard that increments the waypoint_user count on construction and
// decrements it on destruction. This tracks how many times a waypoint is
// currently active on the call stack. The name can be customized at runtime
// for each invocation and can be a formatted string.
struct waypoint_guard {
    waypoint_user& user;
    const std::string* prev_name;
    std::string name_;

    waypoint_guard(waypoint_user& u, std::string name)
      : user(u)
      , prev_name(u.name)
      , name_(std::move(name)) {
        user.name = &name_;
        ++user.count;
    }

    waypoint_guard(const waypoint_guard&) = delete;
    waypoint_guard& operator=(const waypoint_guard&) = delete;
    waypoint_guard(waypoint_guard&&) = delete;
    waypoint_guard& operator=(waypoint_guard&&) = delete;

    ~waypoint_guard() {
        --user.count;
        user.name = prev_name;
    }
};

struct waypoint_iterator;
struct waypoint_user_iterator;

} // namespace vlog

// Linker-provided symbols marking the bounds of the waypoints section.
// Declared as weak so they resolve to nullptr if no waypoints are defined.
// NOLINTNEXTLINE(*-avoid-c-arrays)
extern const vlog::waypoint __start_waypoints[] __attribute__((weak));
// NOLINTNEXTLINE(*-avoid-c-arrays)
extern const vlog::waypoint __stop_waypoints[] __attribute__((weak));

namespace vlog {

// Iterator over all waypoints in the ELF section (global, not per-thread).
struct waypoint_iterator {
    template<typename Func>
    static void for_each(Func&& func) {
        if (__start_waypoints == nullptr || __stop_waypoints == nullptr) {
            return;
        }
        for (const waypoint* wp = __start_waypoints; wp < __stop_waypoints;
             ++wp) {
            func(*wp);
        }
    }
};

// Iterator over all waypoint_users on the current thread.
// This provides per-thread waypoint hit counts.
struct waypoint_user_iterator {
    template<typename Func>
    static void for_each(Func&& func) {
        for (auto& user : waypoint_user::users_) {
            func(user);
        }
    }
};

} // namespace vlog

// NOLINTNEXTLINE
#define VPOINT_CONCAT_INNER(a, b) a##b
// NOLINTNEXTLINE
#define VPOINT_CONCAT(a, b) VPOINT_CONCAT_INNER(a, b)

// Internal implementation macro - do not use directly.
// NOLINTNEXTLINE
#define VPOINT_IMPL(counter, name_arg)                                         \
    static constinit const vlog::waypoint                                      \
      __attribute__((section("waypoints"), used))                              \
      VPOINT_CONCAT(_vpoint_wp_, counter)                                      \
      = {                                                                      \
        .filename = vlog::detail::file_basename(__FILE__),                     \
        .line = __LINE__,                                                      \
    };                                                                         \
    static thread_local vlog::waypoint_user VPOINT_CONCAT(_vpoint_user_, counter)( \
      &VPOINT_CONCAT(_vpoint_wp_, counter));                                   \
    vlog::waypoint_guard VPOINT_CONCAT(_vpoint_guard_, counter)(               \
      VPOINT_CONCAT(_vpoint_user_, counter), name_arg)

// vpoint() - Register a waypoint at this location in the code.
// The waypoint can be enumerated at runtime via vlog::waypoint_iterator.
// Per-thread hit counts are tracked via vlog::waypoint_user_iterator.
// The name can be customized at runtime for each invocation.
// Usage:
//   vpoint();              // Anonymous waypoint
//   vpoint("my_marker");   // Named waypoint
// NOLINTNEXTLINE
#define vpoint(name) VPOINT_IMPL(__COUNTER__, name)

// NOLINTNEXTLINE
#define fmt_with_ctx(method, fmt, args...)                                     \
    method("{} - " fmt, vlog::file_line::current(), ##args)

// NOLINTNEXTLINE
#define vlog(method, fmt, args...) fmt_with_ctx(method, fmt, ##args)

// NOLINTNEXTLINE
#define fmt_with_ctx_level(logger, level, fmt, args...)                        \
    logger.log(level, "{} - " fmt, vlog::file_line::current(), ##args)

// NOLINTNEXTLINE
#define vlogl(logger, level, fmt, args...)                                     \
    fmt_with_ctx_level(logger, level, fmt, ##args)

// NOLINTNEXTLINE
#define fmt_with_ctx_level_and_rate(logger, level, rate, fmt, args...)         \
    logger.log(level, rate, "{} - " fmt, vlog::file_line::current(), ##args)

// NOLINTNEXTLINE
#define vloglr(logger, level, rate, fmt, args...)                              \
    fmt_with_ctx_level_and_rate(logger, level, rate, fmt, ##args)
