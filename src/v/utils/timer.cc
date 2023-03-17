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
#include "utils/timer.h"

#include "ssx/future-util.h"

periodic_task::periodic_task()
  : _timer([this] { timer_handler(); }) {}

void periodic_task::timer_handler() {
    ssx::spawn_with_gate(_gate, [this] {
        return task().finally([this] {
            if (!_gate.is_closed()) {
                _timer.arm((_binding.value())());
            }
        });
    });
}

void periodic_task::start() { _timer.arm((_binding.value())()); }

ss::future<> periodic_task::stop() {
    _timer.cancel();
    return _gate.close();
}

void periodic_task::set_binding(
  config::binding<std::chrono::milliseconds> binding) {
    _binding = std::move(binding);
    _binding.value().watch([this] {
        if (_timer.armed()) {
            _timer.cancel();
            _timer.arm((_binding.value())());
        }
    });
}
