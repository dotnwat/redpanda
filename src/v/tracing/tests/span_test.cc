// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "tracing/span.h"

#include "tracing/span_buffer.h"
#include "tracing/trace_context.h"

#include <gtest/gtest.h>

TEST(TraceContextTest, MakeTraceId) {
    auto id = tracing::make_trace_id();
    EXPECT_TRUE(tracing::is_valid(id));
}

TEST(TraceContextTest, MakeSpanId) {
    auto id = tracing::make_span_id();
    EXPECT_TRUE(tracing::is_valid(id));
}

TEST(TraceContextTest, ZeroIdsAreInvalid) {
    tracing::trace_id zero_trace{};
    tracing::span_id zero_span{};
    EXPECT_FALSE(tracing::is_valid(zero_trace));
    EXPECT_FALSE(tracing::is_valid(zero_span));
}

TEST(SpanBufferTest, SubmitAndDrain) {
    tracing::span_buffer buf;
    buf.start().get();

    tracing::completed_span cs;
    cs.name = "test.span";
    cs.kind = tracing::span_kind::server;
    cs.trace = tracing::make_trace_id();
    cs.id = tracing::make_span_id();
    cs.start_ns = 1000;
    cs.end_ns = 2000;

    buf.submit(std::move(cs));
    auto drained = buf.drain();

    ASSERT_EQ(drained.size(), 1);
    EXPECT_EQ(drained[0].name, "test.span");
    EXPECT_EQ(drained[0].start_ns, 1000);
    EXPECT_EQ(drained[0].end_ns, 2000);

    // Drain again should be empty.
    auto empty = buf.drain();
    EXPECT_TRUE(empty.empty());

    buf.stop().get();
}

TEST(SpanBufferTest, DropsWhenFull) {
    tracing::span_buffer buf;
    buf.start().get();

    for (size_t i = 0; i < tracing::span_buffer::max_buffer_size + 100; ++i) {
        tracing::completed_span cs;
        cs.name = "overflow";
        cs.trace = tracing::make_trace_id();
        cs.id = tracing::make_span_id();
        buf.submit(std::move(cs));
    }

    auto drained = buf.drain();
    EXPECT_EQ(drained.size(), tracing::span_buffer::max_buffer_size);

    buf.stop().get();
}

TEST(SpanTest, RootSpanGeneratesValidIds) {
    tracing::span_buffer buf;
    buf.start().get();

    {
        tracing::span s(buf, "root.span", tracing::span_kind::server);
        EXPECT_TRUE(s.is_enabled());
        auto ctx = s.context();
        EXPECT_TRUE(tracing::is_valid(ctx.trace));
        EXPECT_TRUE(tracing::is_valid(ctx.parent_span));
        EXPECT_TRUE(ctx.is_sampled());
    }

    auto drained = buf.drain();
    ASSERT_EQ(drained.size(), 1);
    EXPECT_EQ(drained[0].name, "root.span");
    EXPECT_EQ(
      static_cast<uint8_t>(drained[0].kind),
      static_cast<uint8_t>(tracing::span_kind::server));
    EXPECT_GT(drained[0].start_ns, 0);
    EXPECT_GE(drained[0].end_ns, drained[0].start_ns);

    buf.stop().get();
}

TEST(SpanTest, ChildSpanInheritsTraceId) {
    tracing::span_buffer buf;
    buf.start().get();

    tracing::trace_context parent_ctx;
    {
        tracing::span parent(buf, "parent", tracing::span_kind::server);
        parent_ctx = parent.context();
    }

    {
        tracing::span child(
          buf, "child", tracing::span_kind::internal, parent_ctx);
        auto child_ctx = child.context();
        EXPECT_EQ(child_ctx.trace, parent_ctx.trace);
        EXPECT_EQ(child.context().trace, parent_ctx.trace);
    }

    auto drained = buf.drain();
    ASSERT_EQ(drained.size(), 2);

    // First drained is the parent (submitted first when destroyed).
    EXPECT_EQ(drained[0].name, "parent");
    // Second is the child.
    EXPECT_EQ(drained[1].name, "child");
    // Child's parent_span_id should match parent's span_id.
    EXPECT_EQ(drained[1].parent, drained[0].id);
    // Same trace.
    EXPECT_EQ(drained[0].trace, drained[1].trace);

    buf.stop().get();
}

TEST(SpanTest, DisabledSpanIsNoOp) {
    tracing::span s;
    EXPECT_FALSE(s.is_enabled());
    auto ctx = s.context();
    EXPECT_FALSE(tracing::is_valid(ctx.trace));
    EXPECT_FALSE(ctx.is_sampled());
    // Should not crash.
    s.set_error("error");
}

TEST(SpanTest, MoveSemantics) {
    tracing::span_buffer buf;
    buf.start().get();

    tracing::span s1(buf, "moveme", tracing::span_kind::internal);
    tracing::span s2(std::move(s1));
    EXPECT_FALSE(s1.is_enabled()); // NOLINT(bugprone-use-after-move)
    EXPECT_TRUE(s2.is_enabled());

    // s2 goes out of scope, should submit.
    { auto _ = std::move(s2); }

    auto drained = buf.drain();
    ASSERT_EQ(drained.size(), 1);
    EXPECT_EQ(drained[0].name, "moveme");

    buf.stop().get();
}

TEST(SpanTest, SetError) {
    tracing::span_buffer buf;
    buf.start().get();

    {
        tracing::span s(buf, "error.span", tracing::span_kind::server);
        s.set_error("something went wrong");
    }

    auto drained = buf.drain();
    ASSERT_EQ(drained.size(), 1);
    EXPECT_TRUE(drained[0].is_error);
    EXPECT_EQ(drained[0].error_message, "something went wrong");

    buf.stop().get();
}
