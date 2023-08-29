#include "storage/v2/segment.h"

#include "storage/v2/paging_input_stream.h"

#include <seastar/core/coroutine.hh>
#include <seastar/core/file.hh>
#include <seastar/core/fstream.hh>
#include <seastar/core/seastar.hh>
#include <seastar/coroutine/as_future.hh>
#include <seastar/util/defer.hh>

seastar::future<segment>
segment::open(std::filesystem::path path, io_scheduler* io_sched) {
    auto file = co_await seastar::open_file_dma(
      path.string(), seastar::open_flags::ro);

    auto stat = co_await seastar::coroutine::as_future(file.stat());
    auto memory_alignment = file.memory_dma_alignment();
    auto disk_read_alignment = file.disk_read_dma_alignment();

    // defer logging so close errors are printed last
    auto close = co_await seastar::coroutine::as_future(file.close());
    auto defer = seastar::defer([close = std::move(close)]() mutable {
        if (close.failed()) {
            close.get_exception();
        }
    });

    if (stat.failed()) {
        std::rethrow_exception(stat.get_exception());
    }

    co_return segment(
      std::move(path),
      stat.get().st_size,
      memory_alignment,
      disk_read_alignment,
      io_sched);
}

/*
 * An unaligned request is made. The goal is to submit aligned requests to the
 * io scheduler.
 */
seastar::future<> segment::request_page(uint64_t offset, uint64_t) {
    auto buf = seastar::temporary_buffer<char>::aligned(
      memory_alignment_, disk_read_alignment_);
    offset = seastar::align_down(offset, disk_read_alignment_);

    io_scheduler::request req(this, offset, std::move(buf));
    io_sched_->submit_request(std::move(req));

    pages_.try_emplace(offset);
    co_return;
}

/*
 * Handle data from io scheduler. In affect we want to merge this into cache
 * without too many assumptions really other than offset is aligned.
 */
void segment::insert_page(
  uint64_t offset, seastar::temporary_buffer<char> buf) {
    auto it = pages_.find(offset);
    if (it == pages_.end()) {
        pages_.emplace(offset, std::move(buf));
        return;
    }

    it->second.data() = std::move(buf);
    for (auto& waiter : it->second.waiters()) {
        waiter.set_value();
    }
    it->second.waiters().clear();
}

seastar::input_stream<char> segment::open() {
    seastar::data_source ds(std::make_unique<paging_input_stream>(this));
    return seastar::input_stream<char>(std::move(ds));
}
