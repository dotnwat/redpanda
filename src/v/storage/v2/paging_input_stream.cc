#include "storage/v2/paging_input_stream.h"

paging_input_stream::paging_input_stream(segment* segment)
  : segment_(segment)
  , len_(segment_->size()) {}

seastar::future<seastar::temporary_buffer<char>> paging_input_stream::get() {
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

seastar::temporary_buffer<char>
paging_input_stream::finalize(seastar::temporary_buffer<char> page) {
    auto size = page.size();
    if (size > len_) {
        page.trim(len_);
    }
    offset_ += size;
    len_ -= page.size();
    return page;
}
