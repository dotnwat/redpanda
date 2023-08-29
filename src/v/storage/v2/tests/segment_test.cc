#include "storage/v2/segment.h"
#include "test_utils/test.h"
#include "units.h"

#include <seastar/core/file-types.hh>
#include <seastar/core/file.hh>
#include <seastar/core/seastar.hh>
#include <seastar/util/defer.hh>

TEST_CORO(Foo, Bar) {
    io_scheduler io;
    io.start();

    for (size_t size = 0; size < 5_KiB; ++size) {
        std::filesystem::path path("foo");

        // make the input file
        auto file = co_await seastar::open_file_dma(
          path.string(),
          seastar::open_flags::create | seastar::open_flags::rw
            | seastar::open_flags::truncate);
        {
            auto buf = seastar::temporary_buffer<char>::aligned(
              file.memory_dma_alignment(),
              seastar::align_up(size, file.disk_write_dma_alignment()));
            co_await file.dma_write<char>(0, buf.get(), buf.size());
            co_await file.truncate(size);
        }
        co_await file.close();

        // open the file as a segment
        auto seg = co_await segment::open(path, &io);

        // check its input stream returns size bytes
        size_t total = 0;
        auto input = seg.open();
        while (!input.eof()) {
            auto data = co_await input.read();
            total += data.size();
        }

        ASSERT_EQ_CORO(total, size);
    }

    co_await io.stop();
}
