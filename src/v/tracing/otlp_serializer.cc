// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "tracing/otlp_serializer.h"

#include <cstring>

namespace tracing {

// Protobuf wire types
static constexpr uint8_t wire_varint = 0;
static constexpr uint8_t wire_64bit = 1;
static constexpr uint8_t wire_length_delimited = 2;

proto_writer::proto_writer(iobuf& out)
  : _out(out) {}

void proto_writer::write_varint(uint64_t v) {
    // Protobuf base-128 varint, up to 10 bytes for uint64.
    uint8_t buf[10];
    size_t n = 0;
    while (v >= 0x80) {
        buf[n++] = static_cast<uint8_t>(v | 0x80);
        v >>= 7;
    }
    buf[n++] = static_cast<uint8_t>(v);
    _out.append(buf, n);
}

void proto_writer::write_fixed64(uint64_t v) {
    // Little-endian 8 bytes.
    uint8_t buf[8];
    std::memcpy(buf, &v, 8);
    // Ensure little-endian on big-endian platforms.
    static_assert(
      __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
      "big-endian not supported");
    _out.append(buf, 8);
}

void proto_writer::write_tag(uint32_t field_number, uint8_t wire_type) {
    write_varint((static_cast<uint64_t>(field_number) << 3) | wire_type);
}

void proto_writer::write_bytes_field(
  uint32_t field, const uint8_t* data, size_t len) {
    if (len == 0) {
        return;
    }
    write_tag(field, wire_length_delimited);
    write_varint(len);
    _out.append(data, len);
}

void proto_writer::write_string_field(uint32_t field, std::string_view s) {
    if (s.empty()) {
        return;
    }
    write_tag(field, wire_length_delimited);
    write_varint(s.size());
    _out.append(s.data(), s.size());
}

void proto_writer::write_varint_field(uint32_t field, uint64_t v) {
    if (v == 0) {
        return;
    }
    write_tag(field, wire_varint);
    write_varint(v);
}

void proto_writer::write_fixed64_field(uint32_t field, uint64_t v) {
    if (v == 0) {
        return;
    }
    write_tag(field, wire_64bit);
    write_fixed64(v);
}

void proto_writer::write_submessage(uint32_t field, const iobuf& inner) {
    if (inner.empty()) {
        return;
    }
    write_tag(field, wire_length_delimited);
    write_varint(inner.size_bytes());
    // Copy inner fragments into output.
    for (const auto& frag : inner) {
        _out.append(frag.get(), frag.size());
    }
}

// -----------------------------------------------------------------------
// OTLP protobuf field numbers
// -----------------------------------------------------------------------

// ExportTraceServiceRequest
static constexpr uint32_t f_export_resource_spans = 1;

// ResourceSpans
static constexpr uint32_t f_rs_resource = 1;
static constexpr uint32_t f_rs_scope_spans = 2;

// Resource
static constexpr uint32_t f_resource_attributes = 1;

// KeyValue
static constexpr uint32_t f_kv_key = 1;
static constexpr uint32_t f_kv_value = 2;

// AnyValue
static constexpr uint32_t f_anyvalue_string = 1;

// ScopeSpans
static constexpr uint32_t f_ss_scope = 1;
static constexpr uint32_t f_ss_spans = 2;

// InstrumentationScope
static constexpr uint32_t f_scope_name = 1;

// Span
static constexpr uint32_t f_span_trace_id = 1;
static constexpr uint32_t f_span_span_id = 2;
static constexpr uint32_t f_span_parent_span_id = 4;
static constexpr uint32_t f_span_name = 6;
static constexpr uint32_t f_span_kind = 7;
static constexpr uint32_t f_span_start_time = 8;
static constexpr uint32_t f_span_end_time = 9;
static constexpr uint32_t f_span_status = 16;

// Status
static constexpr uint32_t f_status_message = 1;
static constexpr uint32_t f_status_code = 2;

// Status code values
static constexpr uint64_t status_ok = 1;
static constexpr uint64_t status_error = 2;

namespace {

/// Serialize a KeyValue { key, AnyValue { string_value } }.
void write_string_attribute(
  proto_writer& w, uint32_t field, std::string_view key, std::string_view val) {
    // AnyValue submessage
    iobuf any_buf;
    proto_writer any_w(any_buf);
    any_w.write_string_field(f_anyvalue_string, val);

    // KeyValue submessage
    iobuf kv_buf;
    proto_writer kv_w(kv_buf);
    kv_w.write_string_field(f_kv_key, key);
    kv_w.write_submessage(f_kv_value, any_buf);

    w.write_submessage(field, kv_buf);
}

/// Serialize a single Span protobuf message.
iobuf serialize_span(const completed_span& s) {
    iobuf buf;
    proto_writer w(buf);

    w.write_bytes_field(f_span_trace_id, s.trace.data(), s.trace.size());
    w.write_bytes_field(f_span_span_id, s.id.data(), s.id.size());

    if (is_valid(s.parent)) {
        w.write_bytes_field(
          f_span_parent_span_id, s.parent.data(), s.parent.size());
    }

    w.write_string_field(f_span_name, s.name);
    w.write_varint_field(f_span_kind, static_cast<uint64_t>(s.kind));

    // Timestamps are fixed64 (nanoseconds since epoch).
    w.write_fixed64_field(f_span_start_time, s.start_ns);
    w.write_fixed64_field(f_span_end_time, s.end_ns);

    // Status submessage.
    if (s.is_error) {
        iobuf status_buf;
        proto_writer sw(status_buf);
        if (!s.error_message.empty()) {
            sw.write_string_field(f_status_message, s.error_message);
        }
        sw.write_varint_field(f_status_code, status_error);
        w.write_submessage(f_span_status, status_buf);
    } else {
        iobuf status_buf;
        proto_writer sw(status_buf);
        sw.write_varint_field(f_status_code, status_ok);
        w.write_submessage(f_span_status, status_buf);
    }

    return buf;
}

} // namespace

iobuf serialize_otlp_traces(
  std::string_view service_name,
  const chunked_vector<completed_span>& spans) {
    // Build InstrumentationScope.
    iobuf scope_buf;
    {
        proto_writer sw(scope_buf);
        sw.write_string_field(f_scope_name, "redpanda");
    }

    // Build ScopeSpans.
    iobuf scope_spans_buf;
    {
        proto_writer ssw(scope_spans_buf);
        ssw.write_submessage(f_ss_scope, scope_buf);
        for (const auto& s : spans) {
            auto span_buf = serialize_span(s);
            ssw.write_submessage(f_ss_spans, span_buf);
        }
    }

    // Build Resource.
    iobuf resource_buf;
    {
        proto_writer rw(resource_buf);
        write_string_attribute(
          rw, f_resource_attributes, "service.name", service_name);
    }

    // Build ResourceSpans.
    iobuf rs_buf;
    {
        proto_writer rsw(rs_buf);
        rsw.write_submessage(f_rs_resource, resource_buf);
        rsw.write_submessage(f_rs_scope_spans, scope_spans_buf);
    }

    // Build ExportTraceServiceRequest.
    iobuf out;
    {
        proto_writer ow(out);
        ow.write_submessage(f_export_resource_spans, rs_buf);
    }

    return out;
}

} // namespace tracing
