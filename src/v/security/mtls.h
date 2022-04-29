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

#include "seastarx.h"
#include "security/logger.h"
#include "security/sasl_authentication.h"
#include "ssx/future-util.h"
#include "vlog.h"

#include <seastar/core/sstring.hh>
#include <seastar/net/api.hh>
#include <seastar/util/bool_class.hh>

#include <fmt/core.h>

#include <iosfwd>
#include <optional>
#include <regex>
#include <string_view>

namespace security::tls {

class rule {
public:
    using make_lower = ss::bool_class<struct make_lower_tag>;
    using make_upper = ss::bool_class<struct make_upper_tag>;

    rule() = default;

    rule(
      std::string_view pattern,
      std::optional<std::string_view> replacement,
      make_lower to_lower,
      make_upper to_upper);

    std::optional<ss::sstring> apply(std::string_view dn) const;

private:
    friend struct fmt::formatter<rule>;

    friend std::ostream& operator<<(std::ostream& os, const rule& r);

    std::regex _regex;
    std::optional<ss::sstring> _pattern;
    std::optional<ss::sstring> _replacement;
    bool _is_default{true};
    make_lower _to_lower{false};
    make_upper _to_upper{false};
};

namespace detail {

std::vector<rule> parse_rules(std::optional<std::string_view> unparsed_rules);

} // namespace detail

class principal_mapper {
public:
    explicit principal_mapper(std::optional<std::string_view> sv)
      : _rules{detail::parse_rules(sv)} {}

    std::optional<ss::sstring> apply(std::string_view sv) const {
        for (const auto& r : _rules) {
            if (auto p = r.apply(sv); p.has_value()) {
                return {std::move(p).value()};
            }
        }
        return std::nullopt;
    }

private:
    friend struct fmt::formatter<principal_mapper>;

    friend std::ostream&
    operator<<(std::ostream& os, const principal_mapper& p);

    std::vector<rule> _rules;
};

class sasl_mechanism final : public security::sasl_mechanism {
public:
    explicit sasl_mechanism(
      principal_mapper tls_pm,
      ss::future<std::optional<ss::session_dn>> fut,
      ss::gate& gate)
      : _tls_pm{std::move(tls_pm)}
      , _gate{gate}
      , _fut{accept_dn(std::move(fut))} {}

    result<bytes> authenticate(bytes_view) final { return bytes{}; }

    bool complete() const final { return !_fut.has_value(); }
    bool failed() const final {
        return !_fut.has_value() && !_principal.has_value();
    }
    const ss::sstring& principal() const final { return *_principal; }

private:
    ss::future<> accept_dn(ss::future<std::optional<ss::session_dn>> fut) {
        return ss::with_gate(_gate, [this, fut{std::move(fut)}]() mutable {
            return fut
              .then([this](auto dn) {
                  vlog(
                    seclog.info,
                    "got distinguished name: {}",
                    dn ? dn->subject : "<none>");
                  if (dn.has_value()) {
                      auto maybe_principal = _tls_pm.apply(dn->subject);
                      if (maybe_principal) {
                          vlog(
                            seclog.info,
                            "got principal: {}, from distinguished name: {}",
                            *maybe_principal,
                            dn ? dn->subject : "<none>");
                          _principal = *maybe_principal;
                      }
                  }
                  _fut.reset();
              })
              .handle_exception([](std::exception_ptr e) {
                  vlog(seclog.info, "Auth failed: {}", e);
              });
        });
    }

    principal_mapper _tls_pm;
    ss::gate& _gate;
    std::optional<ss::future<>> _fut;
    std::optional<ss::sstring> _principal;
};

} // namespace security::tls

template<>
struct fmt::formatter<security::tls::rule> {
    using type = security::tls::rule;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    typename FormatContext::iterator format(const type& r, FormatContext& ctx);
};

template<>
struct fmt::formatter<security::tls::principal_mapper> {
    using type = security::tls::principal_mapper;

    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    typename FormatContext::iterator format(const type& r, FormatContext& ctx);
};
