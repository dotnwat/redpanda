#include "storage/v2/io_scheduler.h"

#include "storage/v2/segment.h"

#include <seastar/core/file.hh>
#include <seastar/core/seastar.hh>

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
