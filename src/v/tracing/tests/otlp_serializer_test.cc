// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "bytes/iobuf.h"
#include "tracing/otlp_serializer.h"
#include "tracing/span_buffer.h"
#include "tracing/trace_context.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

namespace {

/// Linearize an iobuf into a contiguous byte vector.
std::vector<uint8_t> linearize(const iobuf& buf) {
    std::vector<uint8_t> out;
    out.reserve(buf.size_bytes());
    for (const auto& frag : buf) {
        out.insert(
          out.end(),
          reinterpret_cast<const uint8_t*>(frag.get()),
          reinterpret_cast<const uint8_t*>(frag.get()) + frag.size());
    }
    return out;
}

/// Decode a protobuf varint from `data` starting at `pos`.
/// Returns {value, new_pos}.
std::pair<uint64_t, size_t>
decode_varint(const std::vector<uint8_t>& data, size_t pos) {
    uint64_t result = 0;
    int shift = 0;
    while (pos < data.size()) {
        uint8_t byte = data[pos++];
        result |= static_cast<uint64_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            break;
        }
        shift += 7;
    }
    return {result, pos};
}

/// Decode a protobuf tag from `data` starting at `pos`.
/// Returns {field_number, wire_type, new_pos}.
struct tag_result {
    uint32_t field;
    uint8_t wire_type;
    size_t pos;
};

tag_result decode_tag(const std::vector<uint8_t>& data, size_t pos) {
    auto [val, new_pos] = decode_varint(data, pos);
    return {
      static_cast<uint32_t>(val >> 3),
      static_cast<uint8_t>(val & 0x07),
      new_pos};
}

} // namespace

TEST(ProtoWriterTest, WriteVarint) {
    iobuf buf;
    tracing::proto_writer w(buf);

    w.write_varint(0);
    w.write_varint(1);
    w.write_varint(127);
    w.write_varint(128);
    w.write_varint(300);

    auto bytes = linearize(buf);

    // 0 => 0x00
    EXPECT_EQ(bytes[0], 0x00);
    // 1 => 0x01
    EXPECT_EQ(bytes[1], 0x01);
    // 127 => 0x7F
    EXPECT_EQ(bytes[2], 0x7F);
    // 128 => 0x80 0x01
    EXPECT_EQ(bytes[3], 0x80);
    EXPECT_EQ(bytes[4], 0x01);
    // 300 => 0xAC 0x02
    EXPECT_EQ(bytes[5], 0xAC);
    EXPECT_EQ(bytes[6], 0x02);
}

TEST(ProtoWriterTest, WriteFixed64) {
    iobuf buf;
    tracing::proto_writer w(buf);

    uint64_t val = 0x0102030405060708ULL;
    w.write_fixed64(val);

    auto bytes = linearize(buf);
    ASSERT_EQ(bytes.size(), 8);

    // Little-endian.
    uint64_t decoded = 0;
    std::memcpy(&decoded, bytes.data(), 8);
    EXPECT_EQ(decoded, val);
}

TEST(OtlpSerializerTest, SerializeEmptySpans) {
    chunked_vector<tracing::completed_span> spans;
    auto result = tracing::serialize_otlp_traces("test-service", spans);

    // Even with no spans, we should get a valid protobuf message
    // (ResourceSpans with empty ScopeSpans).
    EXPECT_GT(result.size_bytes(), 0);
}

TEST(OtlpSerializerTest, SerializeSingleSpan) {
    chunked_vector<tracing::completed_span> spans;

    tracing::completed_span cs;
    cs.trace = tracing::make_trace_id();
    cs.id = tracing::make_span_id();
    cs.name = "test.operation";
    cs.kind = tracing::span_kind::server;
    cs.start_ns = 1000000000ULL; // 1 second since epoch
    cs.end_ns = 2000000000ULL;   // 2 seconds since epoch
    cs.is_error = false;

    spans.push_back(std::move(cs));

    auto result = tracing::serialize_otlp_traces("my-service", spans);
    auto bytes = linearize(result);

    // The output should be a valid protobuf message. Decode the outer
    // ExportTraceServiceRequest. Field 1 = ResourceSpans (LEN).
    ASSERT_GT(bytes.size(), 10);

    auto [tag, wire_type, pos] = decode_tag(bytes, 0);
    EXPECT_EQ(tag, 1);       // field 1 = resource_spans
    EXPECT_EQ(wire_type, 2); // wire type 2 = length-delimited

    // Decode the length of the ResourceSpans submessage.
    auto [rs_len, rs_start] = decode_varint(bytes, pos);
    EXPECT_GT(rs_len, 0);
    EXPECT_EQ(rs_start + rs_len, bytes.size());
}

TEST(OtlpSerializerTest, SerializeSpanWithError) {
    chunked_vector<tracing::completed_span> spans;

    tracing::completed_span cs;
    cs.trace = tracing::make_trace_id();
    cs.id = tracing::make_span_id();
    cs.name = "error.op";
    cs.kind = tracing::span_kind::internal;
    cs.start_ns = 100;
    cs.end_ns = 200;
    cs.is_error = true;
    cs.error_message = "something failed";

    spans.push_back(std::move(cs));

    auto result = tracing::serialize_otlp_traces("svc", spans);
    EXPECT_GT(result.size_bytes(), 0);

    // We can verify the output contains the error message string.
    auto bytes = linearize(result);
    std::string error_msg = "something failed";
    bool found = false;
    for (size_t i = 0; i + error_msg.size() <= bytes.size(); ++i) {
        if (
          std::memcmp(bytes.data() + i, error_msg.data(), error_msg.size())
          == 0) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Error message not found in serialized output";
}

TEST(OtlpSerializerTest, ServiceNamePresent) {
    chunked_vector<tracing::completed_span> spans;

    tracing::completed_span cs;
    cs.trace = tracing::make_trace_id();
    cs.id = tracing::make_span_id();
    cs.name = "op";
    cs.kind = tracing::span_kind::server;
    cs.start_ns = 1;
    cs.end_ns = 2;

    spans.push_back(std::move(cs));

    auto result = tracing::serialize_otlp_traces("redpanda", spans);
    auto bytes = linearize(result);

    // Verify "redpanda" appears in the output (as service.name attribute
    // value).
    std::string svc = "redpanda";
    bool found = false;
    for (size_t i = 0; i + svc.size() <= bytes.size(); ++i) {
        if (std::memcmp(bytes.data() + i, svc.data(), svc.size()) == 0) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Service name 'redpanda' not found in output";
}

TEST(OtlpSerializerTest, SpanNamePresent) {
    chunked_vector<tracing::completed_span> spans;

    tracing::completed_span cs;
    cs.trace = tracing::make_trace_id();
    cs.id = tracing::make_span_id();
    cs.name = "kafka.produce";
    cs.kind = tracing::span_kind::server;
    cs.start_ns = 1;
    cs.end_ns = 2;

    spans.push_back(std::move(cs));

    auto result = tracing::serialize_otlp_traces("svc", spans);
    auto bytes = linearize(result);

    std::string span_name = "kafka.produce";
    bool found = false;
    for (size_t i = 0; i + span_name.size() <= bytes.size(); ++i) {
        if (
          std::memcmp(bytes.data() + i, span_name.data(), span_name.size())
          == 0) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Span name 'kafka.produce' not found in output";
}

TEST(OtlpSerializerTest, MultipleSpans) {
    chunked_vector<tracing::completed_span> spans;

    for (int i = 0; i < 3; ++i) {
        tracing::completed_span cs;
        cs.trace = tracing::make_trace_id();
        cs.id = tracing::make_span_id();
        cs.name = ss::sstring("span.") + std::to_string(i);
        cs.kind = tracing::span_kind::internal;
        cs.start_ns = 1000 * i;
        cs.end_ns = 1000 * (i + 1);
        spans.push_back(std::move(cs));
    }

    auto result = tracing::serialize_otlp_traces("svc", spans);
    auto bytes = linearize(result);

    // Verify all three span names appear.
    for (int i = 0; i < 3; ++i) {
        auto name = "span." + std::to_string(i);
        bool found = false;
        for (size_t j = 0; j + name.size() <= bytes.size(); ++j) {
            if (std::memcmp(bytes.data() + j, name.data(), name.size()) == 0) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Span name '" << name << "' not found in output";
    }
}
