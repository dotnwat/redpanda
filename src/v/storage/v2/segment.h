#pragma once
#include <seastar/core/condition-variable.hh>
#include <seastar/core/iostream.hh>
#include <seastar/core/temporary_buffer.hh>

#include <absl/container/btree_map.h>

#include <deque>
#include <filesystem>

class segment;

/*
 *
 */
class page {
public:
    page() = default;

    explicit page(seastar::temporary_buffer<char> data)
      : data_(std::move(data)) {}

    seastar::future<> wait() { return waiters_.emplace_back().get_future(); }

    seastar::temporary_buffer<char>& data() { return data_; }
    std::vector<seastar::promise<>>& waiters() { return waiters_; }

private:
    seastar::temporary_buffer<char> data_;
    std::vector<seastar::promise<>> waiters_;
};

/*
 *
 */
class io_scheduler {
public:
    struct request {
        segment* segment;
        uint64_t offset;
        seastar::temporary_buffer<char> buf;
    };

    io_scheduler()
      : drainer_(seastar::make_ready_future<>()) {}

    void start();
    seastar::future<> stop();

    void submit_request(request);

private:
    std::deque<request> requests_;
    seastar::condition_variable cond_;

    seastar::future<> drainer_;
    seastar::future<> drain();

    bool stop_{false};
};

/*
 * the size can be the size of the underlying file--it doesn't need to be a
 * multiple of block size--for example, maybe the last block is partially
 * written because the last batch doesn't line up perfectly. how to handle
 * appends for active segment? witht he size--it limits what is visibible.
 */
class segment {
public:
    segment(
      std::filesystem::path path,
      size_t size,
      uint64_t memory_alignment,
      uint64_t disk_read_alignment,
      io_scheduler* io_sched)
      : path_(std::move(path))
      , size_(size)
      , memory_alignment_(memory_alignment)
      , disk_read_alignment_(disk_read_alignment)
      , io_sched_(io_sched) {}

    /*
     *
     */
    static seastar::future<segment>
    open(std::filesystem::path, io_scheduler* io_sched);

    /*
     *
     */
    seastar::input_stream<char> open();

    std::filesystem::path path() const { return path_; }
    size_t size() const { return size_; }
    absl::btree_map<uint64_t, page>& pages() { return pages_; }

    seastar::future<> request_page(uint64_t offset, uint64_t length);
    void insert_page(uint64_t offset, seastar::temporary_buffer<char> buf);

private:
    std::filesystem::path path_;
    size_t size_;
    uint64_t memory_alignment_;
    uint64_t disk_read_alignment_;
    io_scheduler* io_sched_;

    /// Pages of data in the segment
    absl::btree_map<uint64_t, page> pages_;
};
