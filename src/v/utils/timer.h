/*
 * Copyright 2023 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#pragma once
#include "config/property.h"

#include <seastar/core/gate.hh>
#include <seastar/core/timer.hh>

#include <optional>
#include <string_view>

/*
 * closed loop
 * should have a name
 */
class periodic_task {
public:
    periodic_task();
    periodic_task(const periodic_task&) = delete;
    periodic_task(periodic_task&&) = delete;
    periodic_task& operator=(const periodic_task&) = delete;
    periodic_task& operator=(periodic_task&&) = delete;
    virtual ~periodic_task() = default;

    virtual seastar::future<> task() const = 0;
    void start();
    seastar::future<> stop();

    void set_binding(config::binding<std::chrono::milliseconds>);

private:
    void timer_handler();

    std::optional<config::binding<std::chrono::milliseconds>> _binding;
    seastar::gate _gate;
    seastar::timer<> _timer;
};
