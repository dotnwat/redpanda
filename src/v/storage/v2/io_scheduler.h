#pragma once

/*
 *
 */
#include <seastar/core/condition-variable.hh>
#include <seastar/core/future.hh>
#include <seastar/core/temporary_buffer.hh>

#include <deque>

class segment;

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
