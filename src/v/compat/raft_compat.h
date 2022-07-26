#pragma once
#include "bytes/iobuf.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"
#include "raft/types.h"
#include "seastarx.h"

#include <seastar/core/sstring.hh>

#include <vector>

struct compat_binary {
    ss::sstring name;
    iobuf data;

    explicit compat_binary(iobuf data)
      : compat_binary("serde", std::move(data)) {}

    compat_binary(ss::sstring name, iobuf data)
      : name(std::move(name))
      , data(std::move(data)) {}
};

template<typename T>
std::tuple<T, T> compat_copy(T t) {
    return {t, t};
}

template<typename T>
struct compat_check {
    /*
     * Generate test cases. It's good to build some random cases, as well as
     * convering edge cases such as min/max values or empty values, etc...
     */
    static std::vector<T> create_test_cases();

    /*
     * Serialize an instance of T to the JSON writer.
     */
    static void to_json(T, json::Writer<json::StringBuffer>&);

    /*
     * Serialize an instance of T to supported binary formats.
     */
    static std::vector<compat_binary> to_binary(T);

    /*
     * Check compatibility.
     */
    static bool check(json::Value&, std::vector<compat_binary>);
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_COMPAT_CHECK(type)                                             \
    template<>                                                                 \
    struct compat_check<type> {                                                \
        static constexpr std::string_view name = #type;                        \
        static std::vector<type> create_test_cases();                          \
        static void to_json(type, json::Writer<json::StringBuffer>&);          \
        static std::vector<compat_binary> to_binary(type);                     \
        static bool check(json::Value&, std::vector<compat_binary>);           \
    };

DECLARE_COMPAT_CHECK(raft::timeout_now_request)
DECLARE_COMPAT_CHECK(raft::timeout_now_reply)

ss::future<> write_corpus();
