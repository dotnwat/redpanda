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

#include "container/intrusive_list_helpers.h"

#include <seastar/core/semaphore.hh>
#include <seastar/core/sstring.hh>

#include <utility>

struct named_semaphore_info;

namespace ssx {

// We use named semaphores because the provided name will be included in
// exception messages, making diagnosing broken or timed-out semaphores much
// easier.

template<typename Clock = seastar::timer<>::clock>
class named_semaphore
  : public seastar::
      basic_semaphore<seastar::named_semaphore_exception_factory, Clock> {
    using base_t
      = seastar::basic_semaphore<seastar::named_semaphore_exception_factory, Clock>;

    intrusive_list_hook hook_;
    static inline thread_local intrusive_list<
      named_semaphore<Clock>,
      &named_semaphore<Clock>::hook_>
      semaphores_;
    seastar::sstring name_;

    friend struct ::named_semaphore_info;

public:
    named_semaphore(const named_semaphore&) = delete;
    named_semaphore operator=(const named_semaphore&) = delete;
    ~named_semaphore() = default;

    named_semaphore(size_t count, seastar::sstring name)
      : base_t(count, seastar::named_semaphore_exception_factory{name})
      , name_(std::move(name)) {
        semaphores_.push_back(*this);
    }

    named_semaphore(named_semaphore&& other) noexcept
      : base_t(std::move(other))
      , name_(std::move(other.name_)) {
        semaphores_.push_back(*this);
    }

    named_semaphore& operator=(named_semaphore&& other) noexcept {
        if (this != &other) {
            base_t::operator=(std::move(other));
            name_ = std::move(other.name_);
        }
        return *this;
    }
};

using semaphore = named_semaphore<>;

using semaphore_units = seastar::semaphore_units<semaphore::exception_factory>;

} // namespace ssx
