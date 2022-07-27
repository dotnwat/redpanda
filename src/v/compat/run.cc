#include "compat/run.h"

#include "compat/check.h"
#include "compat/raft_compat.h"
#include "json/prettywriter.h"
#include "seastarx.h"
#include "utils/base64.h"
#include "utils/directory_walker.h"
#include "utils/file_io.h"

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
            std::cout << "Writing: " << jp << std::endl;
            write_fully(jp, std::move(jo)).get();
        }
        return ss::now();
    }

    static ss::future<> verify(json::Document doc) {
        auto obj = check::from_json(doc["fields"].GetObject());
        auto binaries = doc["binaries"].GetArray();
        for (const auto& binary : binaries) {
            auto o = binary.GetObject();
            compat_binary b(
              o["name"].GetString(),
              bytes_to_iobuf(base64_to_bytes(o["data"].GetString())));
            auto&& [ta, tb] = compat_copy(std::move(obj));
            obj = std::move(ta);
            check::check(std::move(tb), std::move(b));
        }
        return ss::now();
    }
};

static ss::future<> verify(json::Document doc) {
    auto name = doc["name"].GetString();

    /*
     * Add for each corpus_writer<...> type
     */

    {
        using type = corpus_writer<raft::timeout_now_request>;
        if (type::check::name == name) {
            return type::verify(std::move(doc));
        }
    }

    {
        using type = corpus_writer<raft::timeout_now_reply>;
        if (type::check::name == name) {
            return type::verify(std::move(doc));
        }
    }

    vassert(false, "Type {} not found", name);
    return ss::now();
}

ss::future<> write_corpus(std::filesystem::path dir) {
    return ss::async([dir] {
        /*
         * run for each corpus_writer<...>
         */
        corpus_writer<raft::timeout_now_request>::write(dir).get();
        corpus_writer<raft::timeout_now_reply>::write(dir).get();
    });
}

ss::future<> read_corpus(std::filesystem::path dir) {
    return directory_walker::walk(dir.string(), [dir](ss::directory_entry ent) {
        if (!ent.type || *ent.type != ss::directory_entry_type::regular) {
            return ss::now();
        }
        if (ent.name.find(".json") == ss::sstring::npos) {
            std::cout << "Skipping non-json file " << ent.name << std::endl;
            return ss::now();
        }
        std::cout << "Processing " << ent.name << std::endl;
        return read_fully_to_string(dir / std::string(ent.name))
          .then([ent](auto js) {
              json::Document doc;
              doc.Parse(js);
              vassert(
                !doc.HasParseError(), "JSON {} has parse errors", ent.name);
              return verify(std::move(doc));
          });
    });
}
