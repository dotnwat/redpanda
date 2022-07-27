#include "compat/raft_compat.h"
#include "seastarx.h"
#include "utils/base64.h"
#include "utils/file_io.h"
#include "json/prettywriter.h"

#include <seastar/core/thread.hh>

template<typename T>
struct corpus_writer {
    using check = compat_check<T>;

    static ss::future<>
    write_test_case(T t, json::Writer<json::StringBuffer>& w) {
        auto&& [ta, tb] = compat_copy(std::move(t));

        /*
         * {
         *   "name": "raft::some_request",
         */
        w.StartObject();
        w.Key("name");
        w.String(check::name.data());

        /*
         *   "fields": { test case field values },
         */
        w.Key("fields");
        w.StartObject();
        check::to_json(std::move(ta), w);
        w.EndObject();

        /*
         *   "binaries": [
         *     {
         *       "name": "serde",
         *       "data": "base64 encoding",
         *     },
         *   ]
         * }
         */
        w.Key("binaries");
        w.StartArray();
        for (auto& b : check::to_binary(std::move(tb))) {
            w.StartObject();
            w.Key("name");
            w.String(b.name);
            w.Key("data");
            w.String(iobuf_to_base64(b.data));
            w.EndObject();
        }
        w.EndArray();
        w.EndObject();

        return ss::now();
    }

    static ss::future<> write(std::filesystem::path dir) {
        int i = 0;
        for (auto& t : check::create_test_cases()) {
            auto sb = json::StringBuffer{};
            auto w = json::Writer<json::StringBuffer>{sb};
            write_test_case(std::move(t), w).get();
            std::string j = sb.GetString();
            iobuf jo;
            jo.append(j.data(), j.size());
            auto jp = dir / fmt::format("{}_{}.json", check::name, i++);
            write_fully(jp, std::move(jo)).get();
        }
        return ss::now();
    }
};

ss::future<> write_corpus(std::filesystem::path dir) {
    return ss::async([dir] {
        corpus_writer<raft::timeout_now_request>::write(dir).get();
        corpus_writer<raft::timeout_now_reply>::write(dir).get();
    });
}
