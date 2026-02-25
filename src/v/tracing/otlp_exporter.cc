// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "tracing/otlp_exporter.h"

#include "config/configuration.h"
#include "http/client.h"
#include "utils/unresolved_address.h"
#include "ssx/future-util.h"
#include "tracing/otlp_serializer.h"

#include <seastar/core/coroutine.hh>
#include <seastar/core/smp.hh>
#include <seastar/util/log.hh>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>

#include <charconv>

// Defined in span_buffer.cc.
extern ss::logger tlog;

namespace tracing {

namespace {

struct parsed_endpoint {
    ss::sstring host;
    uint16_t port{0};
    bool is_tls{false};
};

parsed_endpoint parse_endpoint(const ss::sstring& url) {
    parsed_endpoint ep;
    std::string_view sv(url);

    if (sv.starts_with("https://")) {
        ep.is_tls = true;
        sv.remove_prefix(8);
        ep.port = 443;
    } else if (sv.starts_with("http://")) {
        sv.remove_prefix(7);
        ep.port = 80;
    }

    // Strip trailing path.
    auto slash = sv.find('/');
    if (slash != std::string_view::npos) {
        sv = sv.substr(0, slash);
    }

    // Parse host:port.
    auto colon = sv.rfind(':');
    if (colon != std::string_view::npos) {
        ep.host = ss::sstring(sv.substr(0, colon));
        auto port_sv = sv.substr(colon + 1);
        uint16_t port{0};
        std::from_chars(port_sv.data(), port_sv.data() + port_sv.size(), port);
        ep.port = port;
    } else {
        ep.host = ss::sstring(sv);
    }

    return ep;
}

} // namespace

otlp_exporter::otlp_exporter(ss::sharded<span_buffer>& spans)
  : _spans(spans) {}

ss::future<> otlp_exporter::start() {
    _timer.set_callback([this] { arm_timer(); });
    arm_timer();
    co_return;
}

ss::future<> otlp_exporter::stop() {
    _as.request_abort();
    _timer.cancel();
    co_await _gate.close();
}

void otlp_exporter::arm_timer() {
    auto holder = _gate.hold();
    // Fire and forget; errors are logged in do_export.
    ssx::background = do_export().finally(
      [this, h = std::move(holder)]() mutable {
          if (!_gate.is_closed() && !_as.abort_requested()) {
              auto interval
                = config::shard_local_cfg().tracing_flush_interval_ms();
              _timer.arm(interval);
          }
      });
}

ss::future<> otlp_exporter::do_export() {
    if (!config::shard_local_cfg().tracing_enabled()) {
        co_return;
    }

    try {
        // Collect spans from all shards.
        chunked_vector<completed_span> all_spans;
        co_await _spans.invoke_on_all(
          [&all_spans](span_buffer& buf) {
              auto drained = buf.drain();
              for (auto& s : drained) {
                  all_spans.push_back(std::move(s));
              }
          });

        if (all_spans.empty()) {
            vlog(tlog.debug, "No trace spans to export");
            co_return;
        }

        vlog(tlog.debug, "Exporting {} trace spans", all_spans.size());

        auto payload = serialize_otlp_traces("redpanda", all_spans);
        co_await send(std::move(payload));
    } catch (...) {
        vlog(
          tlog.warn,
          "Failed to export trace spans: {}",
          std::current_exception());
    }
}

ss::future<> otlp_exporter::send(iobuf payload) {
    auto endpoint_str = config::shard_local_cfg().tracing_endpoint();
    if (endpoint_str.empty()) {
        co_return;
    }

    auto ep = parse_endpoint(endpoint_str);

    net::base_transport::configuration client_cfg;
    client_cfg.server_addr = net::unresolved_address(ep.host, ep.port);
    client_cfg.disable_metrics = net::metrics_disabled::yes;

    http::client client(client_cfg, _as);

    auto timeout = config::shard_local_cfg().tracing_flush_interval_ms();
    auto res = co_await client.get_connected(
      timeout, prefix_logger(tlog, ""));

    if (res != http::reconnect_result_t::connected) {
        vlog(tlog.debug, "Unable to connect to tracing endpoint");
        client.shutdown();
        co_return;
    }

    http::client::request_header header;
    header.method(boost::beast::http::verb::post);
    header.target("/v1/traces");
    header.insert(
      boost::beast::http::field::content_type, "application/x-protobuf");
    header.insert(
      boost::beast::http::field::content_length,
      fmt::format("{}", payload.size_bytes()));

    auto resp = co_await client.request(
      std::move(header), std::move(payload), timeout);
    co_await resp->prefetch_headers();

    auto status = resp->get_headers().result();

    // Drain response body so the response_stream is fully consumed.
    iobuf response_body;
    while (!resp->is_done()) {
        auto chunk = co_await resp->recv_some();
        response_body.append(std::move(chunk));
    }

    if (status != boost::beast::http::status::ok) {
        auto body_str = iobuf_to_bytes(response_body);
        vlog(
          tlog.warn,
          "Trace export failed: HTTP {} - {}",
          static_cast<unsigned>(status),
          std::string_view(
            reinterpret_cast<const char*>(body_str.data()),
            body_str.size()));
    } else {
        vlog(tlog.debug, "Trace export completed");
    }

    co_await client.stop();
    client.shutdown();
}

} // namespace tracing
