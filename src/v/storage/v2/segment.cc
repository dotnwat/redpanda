#include "storage/v2/segment.h"

#include <seastar/core/coroutine.hh>
#include <seastar/core/file.hh>
#include <seastar/core/fstream.hh>
#include <seastar/core/seastar.hh>
#include <seastar/coroutine/as_future.hh>
#include <seastar/util/defer.hh>

/*
 * TODO: add support for non-zero starting offset.
 * TODO: be more explicit about page state pending / waiters
 */
class paging_input_stream final : public seastar::data_source_impl {
public:
    explicit paging_input_stream(segment* segment)
      : segment_(segment)
      , len_(segment_->size()) {}

    seastar::future<seastar::temporary_buffer<char>> get() override {
        while (true) {
            if (len_ == 0) {
                co_return seastar::temporary_buffer<char>();
            }

            // lower_bound: first entry having it->first >= offset
            auto it = segment_->pages().lower_bound(offset_);
            if (it == segment_->pages().end()) {
                if (segment_->pages().empty()) {
                    co_await segment_->request_page(offset_, len_);
                    continue;
                }
                // offset may be in the last page
                it = std::prev(it);

            } else if (it->first == offset_) {
                /*
                 * aligned hit. this is the most common case since the caching
                 * layer will only perform aligned io. the offset may unaligned
                 * if it is the starting offset which can be a random seek.
                 */
                if (it->second.data().empty()) {
                    co_await it->second.wait();
                    continue;
                }
                co_return finalize(it->second.data().share());

            } else if (it == segment_->pages().begin()) {
                // offset is before the first cached page
                co_await segment_->request_page(offset_, len_);
                continue;

            } else {
                // offset may be in the previous page
                it = std::prev(it);
            }

            // check if offset landed in the previous page
            if (offset_ < (it->first + it->second.data().size())) {
                if (it->second.data().empty()) {
                    co_await it->second.wait();
                    continue;
                }
                co_return finalize(it->second.data().share());
            }

            co_await segment_->request_page(offset_, len_);
        }
    }

private:
    seastar::temporary_buffer<char>
    finalize(seastar::temporary_buffer<char> page) {
        auto size = page.size();
        if (size > len_) {
            page.trim(len_);
        }
        offset_ += size;
        len_ -= page.size();
        return page;
    }

    segment* segment_;

    /// The next offset from which to read.
    uint64_t offset_{0};

    /// The amount of data left to read.
    uint64_t len_;
};

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

void io_scheduler::start() { drainer_ = drain(); }

seastar::future<> io_scheduler::stop() {
    /*
     * allow the scheduler to shutdown gracefully before demanding
     */
    stop_ = true;
    cond_.signal();

    return std::move(drainer_);
}

seastar::future<> io_scheduler::drain() {
    while (true) {
        co_await cond_.wait([this] { return !requests_.empty() || stop_; });

        if (requests_.empty()) {
            if (stop_) {
                break;
            }
            continue;
        }

        auto& req = requests_.front();

        auto file = co_await seastar::open_file_dma(
          req.segment->path().string(), seastar::open_flags::ro);

        // short reads might occur, and we should handle those. but i think that
        // is better handled by the segment when the read is delivered. for
        // example, it might be the end of the file or it might not be but the
        // segment will accept a partial set of pages.
        co_await file.dma_read<char>(
          req.offset, req.buf.get_write(), req.buf.size());

        co_await file.close();

        req.segment->insert_page(req.offset, std::move(req.buf));
        requests_.pop_front();
    }
}

void io_scheduler::submit_request(request request) {
    requests_.push_back(std::move(request));
    cond_.signal();
}
