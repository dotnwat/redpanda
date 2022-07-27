#pragma once
#include "bytes/iobuf.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"
#include "raft/types.h"
#include "seastarx.h"

#include <seastar/core/sstring.hh>

#include <vector>

// TODO: architecture
// TODO: add compat binary serde factory

/*
 *
 */
struct compat_binary {
    ss::sstring name;
    iobuf data;

    template<typename T>
    static compat_binary serde(T v) {
        return {"serde", serde::to_iobuf(v)};
    }

    compat_binary(ss::sstring name, iobuf data)
      : name(std::move(name))
      , data(std::move(data)) {}

    compat_binary(compat_binary&&) noexcept = default;
    compat_binary& operator=(compat_binary&&) noexcept = default;

    /*
     * copy ctor/assignment are defined for ease of use, which would normally be
     * disabled due to the move-only iobuf data member.
     */
    compat_binary(const compat_binary& other)
      : name(other.name)
      , data(other.data.copy()) {}

    compat_binary& operator=(const compat_binary& other) {
        if (this != &other) {
            name = other.name;
            data = other.data.copy();
        }
        return *this;
    }

    ~compat_binary() = default;
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
     * Deserialize an instance of T from the JSON value.
     */
    static T from_json(json::Value&);

    /*
     * Serialize an instance of T to supported binary formats.
     */
    static std::vector<compat_binary> to_binary(T);

    /*
     * Check compatibility.
     */
    static void check(T, compat_binary);
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_COMPAT_CHECK(type)                                             \
    template<>                                                                 \
    struct compat_check<type> {                                                \
        static constexpr std::string_view name = #type;                        \
        static std::vector<type> create_test_cases();                          \
        static void to_json(type, json::Writer<json::StringBuffer>&);          \
        static type from_json(json::Value&);                                   \
        static std::vector<compat_binary> to_binary(type);                     \
        static void check(type, compat_binary);                                \
    };

DECLARE_COMPAT_CHECK(raft::timeout_now_request)
DECLARE_COMPAT_CHECK(raft::timeout_now_reply)

ss::future<> write_corpus();
