# Distributed Tracing for Redpanda: Grafana Tempo Support Report

## TL;DR

**Yes, OpenTelemetry (OTLP) is the right choice for Grafana Tempo.** Tempo's internals are built on OTel standards — its wire format, storage format, and receiver layer are all OTLP-based. Even when Tempo accepts Jaeger/Zipkin traces, it converts them to OTLP internally. Jaeger has deprecated its own SDKs in favor of OTel. There is no serious alternative for a new integration.

The main challenge is not "which protocol" but rather **integrating the OTel C++ SDK with Seastar's thread-per-core async model**, which requires custom work.

---

## What Grafana Tempo Supports

| Protocol | Ports | Status |
|---|---|---|
| **OTLP** (gRPC + HTTP) | 4317 / 4318 | **Recommended** — native format |
| Jaeger (thrift, gRPC) | 14268 / 14250 / 6831-6832 | Supported, but Jaeger SDKs are deprecated |
| Zipkin (HTTP) | 9411 | Supported, niche |
| Kafka consumer | configurable | Supported, for decoupled pipelines |

The canonical pipeline Grafana recommends:
```
App (OTel SDK) --> Grafana Alloy / OTel Collector --> Tempo --> Grafana
```

---

## Current State of Tracing in Redpanda

The codebase has **no distributed tracing today**. Specifically:

- No OpenTelemetry instrumentation in C++ core
- No W3C Trace Context or B3 header propagation
- No span creation/management
- No trace exporters
- OTel Go deps exist in `src/go/rpk/go.mod` but are indirect/unused
- Redpanda Connect (Go) already has OTel support, but that's separate from the core broker

**What does exist that's relevant:**

1. **Kafka protocol correlation IDs** — per-request IDs for matching responses, not cross-service tracing
2. **Internal RPC correlation IDs** — same, for internal node-to-node calls
3. **Admin V2 context system** (`src/v/serde/protobuf/rpc.h`) — an extensible `context` struct with `std::map<std::type_index, std::any>` that already propagates auth and routing info through the RPC layer. This is a natural extension point for trace context.
4. **Metrics probes** — extensive per-handler metrics, but these are counters/histograms, not traces
5. **`retry_chain_node`** (`src/v/utils/retry_chain_node.h`) — an explicit tree-structured context passed through async call chains, similar to how trace parent/child relationships would need to work

---

## Key Technical Challenges

### 1. Thread-Per-Core vs Thread-Local Context (The Big One)

The OTel C++ SDK stores the "current span" in a `thread_local` variable. Seastar runs many coroutines/fibers on a single OS thread per core. This means one fiber's span would pollute another fiber's context.

**Solutions (pick one):**
- **Explicit context passing** (recommended): Pass `SpanContext` through function parameters, like `retry_chain_node` is already passed today. The OTel SDK supports this via `StartSpanOptions::parent`. This avoids the thread-local problem entirely.
- **Custom `RuntimeContextStorage`**: The SDK allows replacing its context storage. You could implement one backed by Seastar's fiber/task context, but this is fragile and couples deeply to Seastar internals.

### 2. Background Threads

The SDK's `BatchSpanProcessor` and OTLP exporters spawn background threads (for batching and HTTP/gRPC I/O). This conflicts with Seastar's model.

**Solution:** Implement a **custom Seastar-native SpanProcessor + Exporter**:
- Buffer completed spans in a per-shard structure
- Use a Seastar timer to periodically flush batches
- Export via Seastar's HTTP client (non-blocking, reactor-integrated)
- Zero background threads

### 3. Blocking I/O in Exporters

Standard exporters use libcurl (blocking) or gRPC (its own thread pool).

**Solution:** Custom exporter using Seastar's `ss::httpd` client to POST OTLP protobuf to Tempo's HTTP endpoint (port 4318). This keeps all I/O on the reactor.

### 4. Dependency Conflicts

| Dep | OTel expects | Redpanda has | Risk |
|---|---|---|---|
| protobuf | 29.0 | **33.5** (patched) | **High** — protobuf codegen is version-sensitive |
| abseil-cpp | 20240116 | **20250814** | Medium — source-compatible in theory |
| gRPC | 1.66.0 | not used | Avoidable if using HTTP-only |
| curl | 8.8.0 | not used | Avoidable with custom exporter |

---

## Recommended Architecture

There are two viable strategies, ranging from lighter to heavier:

### Strategy A: OTel API + Custom Seastar Backend (Recommended)

Use the **OTel C++ API** (header-only, minimal deps) for standard span/trace types, but implement the processing and export pipeline entirely in Seastar-native code.

```
┌─────────────────── Redpanda Broker (per shard) ───────────────────┐
│                                                                    │
│  Kafka Handler / RPC Handler / Admin Handler                       │
│       │                                                            │
│       ▼                                                            │
│  OTel API: Tracer::StartSpan() with explicit parent context        │
│       │                                                            │
│       ▼                                                            │
│  Custom SeastarSpanProcessor (per-shard ring buffer)               │
│       │                                                            │
│       ▼ (Seastar timer, periodic flush)                            │
│  Custom SeastarOtlpExporter (ss::http::client → OTLP HTTP)        │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
         │
         ▼  OTLP/HTTP protobuf (port 4318)
   ┌──────────────┐      ┌───────────┐      ┌─────────┐
   │ OTel Collector│ ───▶ │   Tempo   │ ───▶ │ Grafana │
   │ (or Alloy)   │      │           │      │         │
   └──────────────┘      └───────────┘      └─────────┘
```

**Pros:** Standard OTel span semantics, no background threads, no blocking I/O, minimal dependency surface (API is header-only), builds with Bazel.

**Cons:** Must implement processor + exporter (~1-2K lines). Must serialize OTLP protobuf manually (though the `opentelemetry-proto` schemas are available).

### Strategy B: Full Custom Implementation (ClickHouse Approach)

Skip the OTel SDK entirely. Define your own `Span` type, propagate trace/span IDs through the call chain, and serialize directly to OTLP protobuf for export.

**Pros:** Zero external dependencies beyond `opentelemetry-proto` schemas. Total control. No version conflicts.

**Cons:** More implementation work. Must implement W3C Trace Context parsing, sampling, etc. from scratch. Doesn't benefit from future OTel SDK improvements.

### Precedent

- **ScyllaDB** (also Seastar-based) uses a fully custom tracing system, not OTel
- **ClickHouse** implements OTel-compatible tracing without using the OTel C++ SDK
- Neither is a perfect template, but both confirm that standard C++ tracing SDKs don't fit thread-per-core systems without significant adaptation

---

## Implementation Scope

### Phase 1: Core Infrastructure
- Trace context types (trace ID, span ID, span context)
- Span lifecycle management (create, set attributes, add events, end)
- Per-shard span buffer and batch processor
- OTLP HTTP exporter using Seastar HTTP client
- Configuration: enable/disable, sampling rate, exporter endpoint, batch size
- W3C Trace Context propagation (inject/extract from headers)

### Phase 2: Kafka Protocol Instrumentation
- Span creation on request arrival (extract trace context from Kafka headers)
- Per-handler spans (produce, fetch, metadata, etc.)
- Internal RPC span propagation (node-to-node calls carry trace context)
- Raft operation spans (append, replicate)

### Phase 3: Admin API + Additional Instrumentation
- Admin V2 API spans (extend existing context system in `serde::pb::rpc::context`)
- Partition movement / rebalance spans
- Tiered storage operation spans
- Log correlation (inject trace_id into log lines)

### Phase 4: Enrichment
- Semantic conventions (service name, Kafka-specific attributes)
- Tail-based sampling support
- Exemplar linking (connect metrics to traces)
- rpk CLI trace propagation

---

## Grafana Tempo Ingestion Details

### Supported Protocols

Tempo's receiver layer is built directly on top of the OpenTelemetry Collector receiver infrastructure. Even when accepting Jaeger or Zipkin traces, Tempo converts them to OTLP internally for storage.

| Protocol | Sub-protocols / Transports | Default Port(s) |
|---|---|---|
| **OTLP** | gRPC, HTTP | 4317 (gRPC), 4318 (HTTP) |
| **Jaeger** | thrift_http, gRPC, thrift_binary, thrift_compact | 14268, 14250, 6832, 6831 |
| **Zipkin** | HTTP | 9411 |
| **OpenCensus** | gRPC | configurable |
| **Kafka** | Kafka consumer | configurable |

### Recommended Format

OTLP with **binary Protobuf encoding** (optional gzip compression) over gRPC or HTTP. JSON Protobuf is supported but recommended only for low-traffic dev/test scenarios.

### Tempo Has No Native SDKs

Tempo is purely a storage and query backend. It relies entirely on OpenTelemetry (or Jaeger/Zipkin) SDKs for application instrumentation. Grafana also offers **Beyla** (eBPF auto-instrumentation) and **Alloy** (their OTel Collector distribution) as part of the pipeline.

---

## OpenTelemetry C++ SDK Details

### Maturity

- **Stable/GA** for traces, metrics, and logs
- Latest release: v1.23.0 (September 2025), BCR version 1.24.0
- Supports C++14 through C++23 (`WITH_STL=CXX23` uses stdlib types instead of `nostd` wrappers)
- API is header-only with ABI stability guarantees

### Key Components

**API Layer (header-only):**
- `TracerProvider` — factory for `Tracer` objects
- `Tracer` — creates spans
- `Span` — represents a unit of work (start, set attributes, add events, end)
- `SpanContext` — immutable trace ID + span ID + flags
- `RuntimeContext` — context propagation (thread-local by default, pluggable)
- `TextMapPropagator` — inject/extract trace context from headers (W3C format)

**SDK Layer (implementation):**
- `SimpleSpanProcessor` — synchronous, no background threads, uses spin-lock
- `BatchSpanProcessor` — background thread for batching (incompatible with Seastar)
- `SpanExporter` — protocol-specific export interface
- `Sampler` — AlwaysOn, AlwaysOff, TraceIdRatio, ParentBased

### Available Exporters

| Exporter | Transport | Notes |
|---|---|---|
| **OTLP gRPC** | gRPC (protobuf) | Requires gRPC dependency |
| **OTLP HTTP** | HTTP (protobuf or JSON) | Uses libcurl internally |
| **Console/OStream** | stdout | For debugging |
| **Zipkin** | HTTP (JSON) | Legacy |
| **Custom** | Any | Implement `SpanExporter` interface |

### Context Propagation and Async Code

The `RuntimeContext` class uses pluggable `RuntimeContextStorage`:

```cpp
class RuntimeContextStorage {
public:
  virtual Context GetCurrent() noexcept = 0;
  virtual nostd::unique_ptr<Token> Attach(const Context &context) noexcept = 0;
  virtual bool Detach(Token &token) noexcept = 0;
};
```

Default is `ThreadLocalContextStorage`. For Seastar, the OTel community recommends avoiding implicit context propagation and instead passing parent span context explicitly:

```cpp
options.parent = parent_span->GetContext();
```

### Bazel Support

Available in the Bazel Central Registry:
```starlark
bazel_dep(name = "opentelemetry-cpp", version = "1.24.0.bcr.1")
```

Tested on Bazel 7.x through 9.x.

---

## Conclusion (Initial Research)

OpenTelemetry is the clear and only practical choice for Grafana Tempo integration. Strategy A (OTel API + custom backend) and Strategy B (fully custom) are both viable, proven by precedent (ClickHouse, ScyllaDB). The detailed plan below explores Strategy B — a fully custom implementation with zero external SDK dependencies.

---

# Detailed Plan: Fully Custom Distributed Tracing (Strategy B)

## Design Principles

1. **No opentelemetry-cpp SDK dependency.** We own all the code. Zero version conflicts, zero threading surprises.
2. **Seastar-native throughout.** No background threads, no mutexes, no blocking I/O. Everything runs on the reactor.
3. **Explicit context passing.** No thread-local tricks. Trace context is a value you pass through function parameters, like `retry_chain_node`.
4. **Start small.** First milestone: trace a single Raft replicate operation from leader → follower across two nodes, each on a single core. No cross-shard tracing yet.
5. **OTLP-compatible output.** Export spans as OTLP protobuf over HTTP so Tempo (or any OTel Collector) can ingest them directly.

## Scope: What We're Tracing First

A Raft replicate on the leader, with the append_entries RPC to a single follower:

```
Node A (leader, shard N)              Node B (follower, shard M)
─────────────────────────             ──────────────────────────
consensus::replicate()
  │
  ├─ [span: raft.replicate]
  │   │
  │   ├─ append_to_self()
  │   │   └─ [span: raft.append_local]
  │   │
  │   └─ send_append_entries_request()
  │       └─ [span: raft.append_entries_rpc (CLIENT)]
  │            │
  │            │── RPC ──────────────▶ service::append_entries()
  │            │                         └─ [span: raft.append_entries_rpc (SERVER)]
  │            │                              │
  │            │                              └─ consensus::do_append_entries()
  │            │                                   └─ [span: raft.append_remote]
  │            │◀── reply ───────────────────────────┘
  │            │
  │   └─ process_append_entries_reply()
  │
  └─ [replicate span ends]
```

This gives us 4 spans in a single trace, spanning two nodes. Enough to validate the full pipeline end-to-end.

## Components

### 1. Trace Context Types (`src/v/tracing/types.h`)

Minimal types — no external dependencies:

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace tracing {

// 16-byte trace ID (W3C standard)
using trace_id = std::array<uint8_t, 16>;

// 8-byte span ID (W3C standard)
using span_id = std::array<uint8_t, 8>;

// Immutable context that travels across RPC boundaries.
// This is the only thing that gets serialized into the wire protocol.
struct trace_context {
    trace_id trace;
    span_id parent_span;
    uint8_t flags{0}; // bit 0 = sampled

    bool is_sampled() const { return flags & 0x01; }
};

enum class span_kind : uint8_t {
    internal = 1,
    server = 2,
    client = 3,
};

} // namespace tracing
```

This is ~30 lines. The `trace_context` is the value that gets passed through function parameters and serialized into RPC messages.

### 2. Span Type (`src/v/tracing/span.h`)

A simple RAII span that records timing and attributes:

```cpp
#pragma once
#include "tracing/types.h"
#include <seastar/core/lowres_clock.hh>
#include <string_view>
#include <vector>
#include <variant>

namespace tracing {

// Forward declaration — span_buffer is the per-shard collector
class span_buffer;

struct attribute {
    std::string_view key; // must outlive the span (use literals)
    std::variant<int64_t, std::string_view, bool> value;
};

class span {
public:
    // Create a root span (new trace)
    span(span_buffer&, std::string_view name, span_kind);

    // Create a child span (same trace, parent = ctx)
    span(span_buffer&, std::string_view name, span_kind,
         const trace_context& parent);

    ~span(); // records end_time, submits to span_buffer

    // Non-copyable, movable
    span(const span&) = delete;
    span& operator=(const span&) = delete;
    span(span&&) noexcept;
    span& operator=(span&&) noexcept;

    // Get context to pass to child spans or serialize into RPC
    trace_context context() const;

    void set_attribute(std::string_view key, int64_t val);
    void set_attribute(std::string_view key, std::string_view val);
    void set_error(std::string_view message);

private:
    span_buffer* _buffer;
    trace_id _trace_id;
    span_id _span_id;
    span_id _parent_span_id; // zero if root
    std::string_view _name;
    span_kind _kind;
    ss::lowres_clock::time_point _start;
    ss::lowres_clock::time_point _end;
    std::vector<attribute> _attributes;
    bool _is_error{false};
    std::string_view _error_message;
    bool _submitted{false};
};

} // namespace tracing
```

Key design choices:
- `string_view` for name and attribute keys — callers use string literals, zero allocation.
- `lowres_clock` — microsecond resolution is fine for traces, avoids syscall overhead.
- Destructor submits to `span_buffer` — RAII ensures spans always get recorded even on exceptions.
- No thread-local anything. The `span_buffer&` is passed explicitly.

### 3. Per-Shard Span Buffer (`src/v/tracing/span_buffer.h`)

Collects completed spans on a single shard and periodically flushes them:

```cpp
#pragma once
#include "tracing/types.h"
#include <seastar/core/timer.hh>
#include <seastar/core/sharded.hh>

namespace tracing {

// Completed span data ready for export (POD, no RAII)
struct completed_span {
    trace_id trace;
    span_id id;
    span_id parent;
    std::string_view name;
    span_kind kind;
    uint64_t start_ns; // nanoseconds since epoch
    uint64_t end_ns;
    std::vector<attribute> attributes;
    bool is_error;
    std::string_view error_message;
};

class span_buffer : public ss::peering_sharded_service<span_buffer> {
public:
    span_buffer();

    void submit(completed_span);

    // Called by timer or explicitly — drains buffer for export
    chunked_vector<completed_span> drain();

    // ss::sharded lifecycle
    ss::future<> start();
    ss::future<> stop();

private:
    chunked_vector<completed_span> _buffer;
    size_t _max_buffer_size{4096}; // drop spans if buffer full
};

} // namespace tracing
```

This is purely a per-shard ring buffer. No locking needed — only the owning shard touches it. If the buffer fills up, oldest spans are dropped (standard OTel behavior under back-pressure).

### 4. OTLP HTTP Exporter (`src/v/tracing/otlp_exporter.h`)

Serializes spans to OTLP protobuf and POSTs to a collector:

```cpp
#pragma once
#include "http/client.h"
#include "tracing/span_buffer.h"
#include <seastar/core/sharded.hh>
#include <seastar/core/timer.hh>

namespace tracing {

class otlp_exporter : public ss::peering_sharded_service<otlp_exporter> {
public:
    otlp_exporter(
      ss::sharded<span_buffer>&,
      std::string_view endpoint // e.g. "http://tempo:4318"
    );

    ss::future<> start();
    ss::future<> stop();

private:
    // Periodic flush: drain span_buffer, serialize, POST
    ss::future<> flush();

    // Serialize spans to OTLP protobuf (ExportTraceServiceRequest)
    iobuf serialize_otlp(const chunked_vector<completed_span>&);

    ss::sharded<span_buffer>& _spans;
    std::string _endpoint;
    ss::timer<ss::lowres_clock> _timer;
    std::unique_ptr<http::client> _client;
};

} // namespace tracing
```

The serialization writes raw protobuf bytes into an `iobuf` — no protobuf library needed.
The flush runs on shard 0 only. It gathers spans from all shards via
`_spans.invoke_on_all()`, serializes a single `ExportTraceServiceRequest`, and
POSTs it. Alternatively, each shard can export independently.

### 5. OTLP Protobuf Serialization (`src/v/tracing/otlp_serializer.h`)

Hand-rolled protobuf writer for the subset of OTLP we need. The full OTLP trace
proto is large, but we only need a handful of message types and field numbers:

```
ExportTraceServiceRequest (field 1: repeated ResourceSpans)
  └─ ResourceSpans (field 1: Resource, field 2: repeated ScopeSpans)
       ├─ Resource (field 1: repeated KeyValue)
       └─ ScopeSpans (field 1: InstrumentationScope, field 2: repeated Span)
            ├─ InstrumentationScope (field 1: name string)
            └─ Span:
                 field 1:  trace_id (bytes, 16B)
                 field 2:  span_id (bytes, 8B)
                 field 4:  parent_span_id (bytes, 8B)
                 field 6:  name (string)
                 field 7:  kind (enum/varint)
                 field 8:  start_time_unix_nano (fixed64)
                 field 9:  end_time_unix_nano (fixed64)
                 field 10: attributes (repeated KeyValue)
                 field 16: status (Status message)
```

The protobuf wire format is simple — each field is a (field_number << 3 | wire_type) varint tag followed by the value. We need:
- Wire type 0 (varint): for enums, booleans, small integers
- Wire type 1 (64-bit): for fixed64 timestamps
- Wire type 2 (length-delimited): for bytes, strings, embedded messages

A minimal protobuf writer is ~100 lines:

```cpp
namespace tracing {

class proto_writer {
public:
    explicit proto_writer(iobuf& out) : _out(out) {}

    void write_varint(uint64_t v);
    void write_fixed64(uint64_t v);
    void write_tag(uint32_t field, uint8_t wire_type);
    void write_bytes(uint32_t field, const uint8_t* data, size_t len);
    void write_string(uint32_t field, std::string_view s);
    void write_varint_field(uint32_t field, uint64_t v);
    void write_fixed64_field(uint32_t field, uint64_t v);

    // For nested messages: write tag + length prefix + contents
    // Use a temporary iobuf, serialize the inner message, then
    // write it as a length-delimited field.
    void write_message(uint32_t field, const iobuf& inner);

private:
    iobuf& _out;
};

} // namespace tracing
```

This is the only "from scratch" piece that's mildly tedious, but it's
straightforward and well-understood. The protobuf wire format hasn't changed
in 15+ years.

### 6. Trace Context in Raft RPC (`src/v/raft/types.h` changes)

Propagate trace context through the existing `append_entries_request` using
serde envelope versioning. The request is currently at `serde::version<0>`:

```cpp
// Current:
struct append_entries_request
  : serde::envelope<
      append_entries_request,
      serde::version<0>,       // ← bump to version<1>
      serde::compat_version<0>> {

    // ... existing fields ...

    // NEW (version 1): optional trace context
    // Old readers at compat_version<0> will simply stop reading
    // before these fields — serde handles this automatically.
    std::optional<tracing::trace_context> _trace_ctx;
};
```

Serde envelope versioning handles backward compatibility automatically:
- A v0 reader parsing a v1 message will stop at the envelope boundary
  and skip the trace context bytes. No crash, no error.
- A v1 reader parsing a v0 message will see the envelope end before the
  trace context field and leave `_trace_ctx` as `std::nullopt`.

The same approach applies to `append_entries_reply` (currently at
`serde::version<1>`, bump to `version<2>`).

Alternatively, if we don't want to modify these core types initially,
trace context can be passed through the `append_entries_request_serde_wrapper`
(also currently at `version<0>`) which wraps the request for full-serde
transport. This is less invasive since the wrapper is only used in the
serde RPC path.

### 7. Configuration (`src/v/config/configuration.h`)

Minimal config properties:

```cpp
// Tracing
property<bool> tracing_enabled;           // default: false
property<std::string> tracing_endpoint;   // default: "" (e.g. "http://tempo:4318")
property<double> tracing_sample_rate;     // default: 0.01 (1%)
property<int32_t> tracing_batch_size;     // default: 256
property<std::chrono::milliseconds> tracing_flush_interval; // default: 5000ms
```

All dynamically reconfigurable so tracing can be turned on/off at runtime
without restart.

## Instrumentation Points (Raft Only for v1)

### Leader Side (`src/v/raft/replicate_entries_stm.cc`)

**`replicate_entries_stm::apply()`** — the top-level span:
```cpp
ss::future<result<replicate_result>> apply(units_t u) {
    // Create root span if sampled
    auto span = _ptr->maybe_create_span(
        "raft.replicate", tracing::span_kind::internal);
    // ... existing code ...
    // Pass span.context() to dispatch_one/send_append_entries_request
}
```

**`replicate_entries_stm::send_append_entries_request()`** — client RPC span:
```cpp
ss::future<result<append_entries_reply>>
send_append_entries_request(vnode n, ..., std::optional<tracing::trace_context> ctx) {
    auto span = maybe_create_child_span(
        ctx, "raft.append_entries_rpc", tracing::span_kind::client);
    span.set_attribute("rpc.target_node", n.id()());

    // Inject trace context into the request
    auto req = append_entries_request(...);
    req.set_trace_context(span.context());

    return _ptr->_client_protocol.append_entries(n.id(), std::move(req), ...);
}
```

### Follower Side (`src/v/raft/service.h` and `src/v/raft/consensus.cc`)

**`service::append_entries_full_serde()`** — extract context, create server span:
```cpp
ss::future<append_entries_reply>
append_entries_full_serde(append_entries_request_serde_wrapper r, rpc::streaming_context&) {
    auto request = std::move(r).release();
    auto ctx = request.trace_context(); // extract from envelope

    auto span = maybe_create_child_span(
        ctx, "raft.append_entries_rpc", tracing::span_kind::server);

    // ... dispatch to consensus::append_entries() ...
}
```

**`consensus::do_append_entries()`** — follower append span:
```cpp
ss::future<append_entries_reply>
do_append_entries(append_entries_request&& r) {
    auto span = maybe_create_child_span(
        r.trace_context(), "raft.append_remote", tracing::span_kind::internal);
    // ... existing validation + append logic ...
}
```

## File Layout

```
src/v/tracing/
├── BUILD                    # Bazel build file
├── types.h                  # trace_id, span_id, trace_context, span_kind
├── span.h                   # span class (RAII)
├── span.cc                  # span implementation
├── span_buffer.h            # per-shard completed span collector
├── span_buffer.cc
├── otlp_serializer.h        # raw protobuf writer for OTLP trace messages
├── otlp_serializer.cc
├── otlp_exporter.h          # HTTP exporter using Seastar http::client
├── otlp_exporter.cc
├── sampling.h               # simple probability-based sampler
└── tests/
    ├── span_test.cc          # unit tests for span lifecycle
    ├── otlp_serializer_test.cc  # round-trip: serialize → parse with protobuf
    └── span_buffer_test.cc
```

Estimated size: ~1500-2000 lines of C++ for the tracing library itself,
plus ~200 lines of instrumentation changes in `src/v/raft/`.

## What This Doesn't Include (Future Work)

- **Cross-shard tracing**: a request may move between shards on the same node
  (e.g., `service::dispatch_request()` calls `_group_manager.invoke_on(shard, ...)`).
  This requires passing trace context through the `invoke_on()` call. Doable
  but adds complexity to every cross-shard call site.
- **Kafka protocol integration**: extracting/injecting W3C Trace Context
  from/to Kafka record headers. This is the natural next step after Raft.
- **Sampling strategies**: initially just probability-based. Could add
  tail-based sampling or per-topic sampling later.
- **Log correlation**: injecting trace_id into `vlog()` output so logs
  and traces can be correlated in Grafana.
- **Metrics exemplars**: linking histogram buckets to trace IDs.

## End-to-End Validation Plan

1. **Unit tests**: span lifecycle, protobuf serialization round-trip,
   span buffer drain behavior.
2. **Integration test**: stand up a 3-node Redpanda cluster + OTel Collector
   + Tempo + Grafana. Produce messages, verify traces appear in Grafana
   with correct parent/child relationships across nodes.
3. **Performance test**: measure overhead of span creation + serialization
   on the hot path with tracing enabled at 1% and 100% sample rates.
   Target: < 1μs per span creation, < 1% throughput impact at 1% sampling.

---

## Sources

- [Grafana Tempo Documentation](https://grafana.com/docs/tempo/latest/)
- [Tempo GitHub Repository](https://github.com/grafana/tempo)
- [OpenTelemetry at Grafana Labs](https://grafana.com/docs/opentelemetry/)
- [OpenTelemetry C++ SDK](https://github.com/open-telemetry/opentelemetry-cpp)
- [OTel C++ Async Context Discussion](https://github.com/open-telemetry/opentelemetry-cpp/discussions/2588)
- [OTel C++ Thread Control (Issue #3174)](https://github.com/open-telemetry/opentelemetry-cpp/issues/3174)
- [Bazel Central Registry - opentelemetry-cpp](https://registry.bazel.build/modules/opentelemetry-cpp)
- [ClickHouse OpenTelemetry Tracing](https://clickhouse.com/docs/operations/opentelemetry)
- [ScyllaDB Tracing](https://github.com/scylladb/scylladb/wiki/Tracing)
- [Redpanda Connect OTel Support](https://docs.redpanda.com/redpanda-connect/components/tracers/open_telemetry_collector/)
