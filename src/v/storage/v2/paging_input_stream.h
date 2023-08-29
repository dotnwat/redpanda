#pragma once
#include "storage/v2/segment.h"

#include <seastar/core/future.hh>
#include <seastar/core/iostream.hh>
#include <seastar/core/temporary_buffer.hh>

/*
 * TODO: add support for non-zero starting offset.
 *
 * TODO: be more explicit about page state pending / waiters so we aren't using
 * data.empty as a signal that it's a pending page.
 */
class paging_input_stream final : public seastar::data_source_impl {
public:
    explicit paging_input_stream(segment* segment);

    seastar::future<seastar::temporary_buffer<char>> get() override;

private:
    seastar::temporary_buffer<char>
    finalize(seastar::temporary_buffer<char> page);

    segment* segment_;

    /// The next offset from which to read.
    uint64_t offset_{0};

    /// The amount of data left to read.
    uint64_t len_;
};
