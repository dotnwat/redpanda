// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#pragma once

#include "bytes/iobuf.h"
#include "container/chunked_vector.h"
#include "tracing/span_buffer.h"

#include <cstdint>
#include <string_view>

namespace tracing {

/// Low-level protobuf wire format writer backed by iobuf.
class proto_writer {
public:
    explicit proto_writer(iobuf& out);

    void write_varint(uint64_t v);
    void write_fixed64(uint64_t v);
    void write_tag(uint32_t field_number, uint8_t wire_type);
    void write_bytes_field(
      uint32_t field, const uint8_t* data, size_t len);
    void write_string_field(uint32_t field, std::string_view s);
    void write_varint_field(uint32_t field, uint64_t v);
    void write_fixed64_field(uint32_t field, uint64_t v);
    void write_submessage(uint32_t field, const iobuf& inner);

private:
    iobuf& _out;
};

/// Serialize a batch of completed spans into an OTLP
/// ExportTraceServiceRequest protobuf message.
iobuf serialize_otlp_traces(
  std::string_view service_name,
  const chunked_vector<completed_span>& spans);

} // namespace tracing
