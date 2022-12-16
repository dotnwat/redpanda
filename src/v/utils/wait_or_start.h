/*
 * Copyright 2022 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#pragma once

#include "ssx/semaphore.h"

#include <seastar/core/future.hh>
#include <seastar/util/noncopyable_function.hh>

/// \brief wait or start a function
///
/// Start the function and wait for it to finish, or, if an instance of the
/// function is already running, wait for that one to finish.
class wait_or_start {
public:
    // Prevent accidentally calling the protected func.
    struct tag {};
    using func = seastar::noncopyable_function<seastar::future<>(tag)>;

    wait_or_start(std::string_view name, func func)
      : _lock(1, seastar::sstring(name))
      , _func{std::move(func)} {}

    seastar::future<> operator()() {
        if (_lock.try_wait()) {
            return _func(tag{}).finally(
              [this]() { _lock.signal(_lock.waiters() + 1); });
        }
        return _lock.wait();
    }

private:
    ssx::semaphore _lock;
    func _func;
};
