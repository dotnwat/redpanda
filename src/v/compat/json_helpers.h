// Copyright 2022 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#pragma once

#include "json/document.h"
#include "json/json.h"
#include "model/timestamp.h"
#include "verify.h"

namespace json {

inline char const* to_str(rapidjson::Type const t) {
    static char const* str[] = {
      "Null", "False", "True", "Object", "Array", "String", "Number"};
    return str[t];
}

inline void read_value(json::Value const& v, int64_t& target) {
    verify(v.IsInt64(), "expected Int64, got {}", to_str(v.GetType()));
    target = v.GetInt64();
}

inline void read_value(json::Value const& v, uint64_t& target) {
    verify(v.IsUint64(), "expected UInt64, got {}", to_str(v.GetType()));
    target = v.GetUint64();
}

inline void read_value(json::Value const& v, uint32_t& target) {
    verify(v.IsUint(), "expected UInt, got {}", to_str(v.GetType()));
    target = v.GetUint();
}

inline void read_value(json::Value const& v, int32_t& target) {
    verify(v.IsInt(), "expected Int, got {}", to_str(v.GetType()));
    target = v.GetInt();
}

inline void read_value(json::Value const& v, int8_t& target) {
    verify(v.IsInt(), "expected Int, got {}", to_str(v.GetType()));
    target = v.GetInt();
}

inline void read_value(json::Value const& v, uint8_t& target) {
    verify(v.IsUint(), "expected Int, got {}", to_str(v.GetType()));
    target = v.GetUint();
}

template<typename T, typename Tag, typename IsConstexpr>
void read_value(
  json::Value const& v, detail::base_named_type<T, Tag, IsConstexpr>& target) {
    auto t = T{};
    read_value(v, t);
    target = detail::base_named_type<T, Tag, IsConstexpr>{t};
}

template<typename T, size_t A>
void read_value(json::Value const& v, fragmented_vector<T, A>& target) {
    for (auto const& e : v.GetArray()) {
        auto t = T{};
        read_value(e, t);
        target.push_back(t);
    }
}

template<typename T>
void read_value(json::Value const& v, std::vector<T>& target) {
    for (auto const& e : v.GetArray()) {
        auto t = T{};
        read_value(e, t);
        target.push_back(t);
    }
}

template<typename Writer, typename T>
void write_member(Writer& w, char const* key, T const& value) {
    w.String(key);
    rjson_serialize(w, value);
}

template<typename T>
void read_member(json::Value const& v, char const* key, T& target) {
    auto const it = v.FindMember(key);
    if (it != v.MemberEnd()) {
        read_value(it->value, target);
    } else {
        target = {};
        std::cout << "key " << key << " not found, default initializing";
    }
}

inline void
read_member(json::Value const& v, char const* key, model::timestamp& target) {
    auto const it = v.FindMember(key);
    verify(it != v.MemberEnd(), "member {} not found", key);
    model::timestamp::type base_t{};
    read_value(it->value, base_t);
    target = model::timestamp{base_t};
}

template<typename Enum>
inline auto read_member_enum(json::Value const& v, char const* key, Enum)
  -> std::underlying_type_t<Enum> {
    std::underlying_type_t<Enum> value;
    read_member(v, key, value);
    return value;
}

} // namespace json
