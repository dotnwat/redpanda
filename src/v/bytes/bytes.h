/*
 * Copyright 2020 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */

#pragma once

#include "base/seastarx.h"
#include "bytes/iobuf.h"

#include <seastar/core/sstring.hh>

#include <algorithm>
#include <cstdint>
#include <iosfwd>
#include <span>

class bytes_view;

constexpr size_t bytes_inline_size = 31;

class bytes {
    using container_type
      = ss::basic_sstring<char, uint32_t, bytes_inline_size, false>;

public:
    using value_type = uint8_t;
    using size_type = uint32_t;
    using iterator = value_type*;
    using const_iterator = const value_type*;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    bytes() = default;
    bytes(const bytes&) = default;
    bytes& operator=(const bytes&) = default;
    bytes(bytes&&) noexcept = default;
    bytes& operator=(bytes&&) noexcept = default;
    ~bytes() = default;

    bytes(const char* s)
      : data_(
        // NOLINTNEXTLINE
        reinterpret_cast<const value_type*>(s),
        // NOLINTNEXTLINE
        reinterpret_cast<const value_type*>(s) + std::strlen(s)) {}

    bytes(size_type size, value_type v)
      : data_(size, v) {}

    bytes(const value_type* data, size_t size)
      // NOLINTNEXTLINE
      : data_(data, data + size) {}

    bytes(const value_type* begin, const value_type* end)
      : data_(begin, end) {}

    bytes(std::initializer_list<uint8_t> x)
      : bytes(x.begin(), x.end() - x.begin()) {}

    template<typename It>
    bytes(It begin, It end)
      : data_(begin, end) {}

    struct initialized_later {};
    bytes(initialized_later, size_type size)
      : data_(container_type::initialized_later{}, size) {}

    explicit bytes(bytes_view);

    reference operator[](size_type pos) noexcept { return *cast(&data_[pos]); }
    const_reference operator[](size_type pos) const noexcept {
        return *cast(&data_[pos]);
    }

    pointer data() noexcept { return cast(data_.data()); }
    const_pointer data() const noexcept { return cast(data_.data()); }

    iterator begin() noexcept { return cast(data_.begin()); }
    const_iterator begin() const noexcept { return cast(data_.begin()); }
    const_iterator cbegin() const noexcept { return cast(data_.cbegin()); }

    iterator end() noexcept { return cast(data_.end()); }
    const_iterator end() const noexcept { return cast(data_.end()); }
    const_iterator cend() const noexcept { return cast(data_.cend()); }

    size_type size() const noexcept { return data_.size(); }
    bool empty() const noexcept { return data_.empty(); }

    void resize(size_type size) { data_.resize(size); }

    void append(const_pointer s, size_t n) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        data_.append(reinterpret_cast<const char*>(s), n);
    }

    void append(std::string_view v) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        data_.append(reinterpret_cast<const char*>(v.data()), v.size());
    }

    bytes& operator+=(const bytes& v) {
        append(v.data(), v.size());
        return *this;
    }

    friend bool operator==(const bytes&, const bytes&) = default;

    friend bool operator<(const bytes& a, const bytes& b) {
        return a.data_ < b.data_;
    }

private:
    friend class bytes_view;

    static value_type* cast(char* p) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<value_type*>(p);
    }

    static const value_type* cast(const char* p) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<const value_type*>(p);
    }

    container_type data_;
};

class bytes_view {
    using container_type = std::string_view;

public:
    using value_type = bytes::value_type;
    using const_pointer = const value_type*;

    bytes_view() = default;
    bytes_view(const bytes_view&) = default;
    bytes_view& operator=(const bytes_view&) = default;
    bytes_view(bytes_view&&) noexcept = default;
    bytes_view& operator=(bytes_view&&) noexcept = default;
    ~bytes_view() = default;

    bytes_view(const bytes& bytes)
      : data_(bytes.data_) {}

    bytes_view(const uint8_t* data, size_t size)
      : data_(reinterpret_cast<const char*>(data), size) {}

    const_pointer data() const noexcept { return cast(data_.data()); }
    auto size() const noexcept { return data_.size(); }

    const_pointer begin() const noexcept { return cast(data_.begin()); }
    const_pointer cbegin() const noexcept { return cast(data_.begin()); }

    const_pointer end() const noexcept { return cast(data_.end()); }
    const_pointer cend() const noexcept { return cast(data_.end()); }

    friend bool operator==(const bytes_view& a, const bytes_view& b) = default;

    friend bool operator<(const bytes_view& a, const bytes_view& b) {
        return std::lexicographical_compare(
          a.begin(), a.end(), b.begin(), b.end());
    }

    const value_type& operator[](size_t pos) const noexcept {
        return *cast(&data_[pos]);
    }

    bool starts_with(bytes_view v) const noexcept {
        return data_.starts_with(v.data_);
    }

    bytes_view substr(size_t offset) const {
        return bytes_view(data_.substr(offset));
    }

    bool empty() const noexcept { return data_.empty(); }

private:
    explicit bytes_view(container_type data)
      : data_(data) {}

    static value_type* cast(char* p) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<value_type*>(p);
    }

    static const value_type* cast(const char* p) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<const value_type*>(p);
    }

    container_type data_;
};

inline bytes::bytes(bytes_view v)
  : data_(v.begin(), v.end()) {}

template<std::size_t Extent = std::dynamic_extent>
using bytes_span = std::span<bytes::value_type, Extent>;

template<typename R, R (*HashFunction)(bytes::const_pointer, size_t)>
requires requires(bytes::const_pointer data, size_t len) {
    { HashFunction(data, len) } -> std::same_as<R>;
}
struct bytes_hasher {
    using is_transparent = std::true_type;

    R operator()(bytes_view b) const {
        return HashFunction(b.data(), b.size());
    }
    R operator()(const bytes& bb) const { return operator()(bytes_view(bb)); }
};

struct bytes_type_eq {
    using is_transparent = std::true_type;
    bool operator()(const bytes& lhs, const bytes_view& rhs) const;
    bool operator()(const bytes& lhs, const bytes& rhs) const;
    bool operator()(const bytes& lhs, const iobuf& rhs) const;
};

ss::sstring to_hex(bytes_view b);
ss::sstring to_hex(const bytes& b);

template<typename Char, size_t Size>
inline bytes_view to_bytes_view(const std::array<Char, Size>& data) {
    static_assert(sizeof(Char) == 1, "to_bytes_view only accepts bytes");
    return bytes_view(
      reinterpret_cast<const uint8_t*>(data.data()), Size); // NOLINT
}

template<typename Char, size_t Size>
inline ss::sstring to_hex(const std::array<Char, Size>& data) {
    return to_hex(to_bytes_view(data));
}

std::ostream& operator<<(std::ostream& os, const bytes& b);

inline bytes iobuf_to_bytes(const iobuf& in) {
    bytes out(bytes::initialized_later{}, in.size_bytes());
    {
        iobuf::iterator_consumer it(in.cbegin(), in.cend());
        it.consume_to(in.size_bytes(), out.data());
    }
    return out;
}

inline iobuf bytes_to_iobuf(const bytes& in) {
    iobuf out;
    // NOLINTNEXTLINE
    out.append(reinterpret_cast<const char*>(in.data()), in.size());
    return out;
}

inline iobuf bytes_to_iobuf(bytes_view in) {
    iobuf out;
    // NOLINTNEXTLINE
    out.append(reinterpret_cast<const char*>(in.data()), in.size());
    return out;
}

// NOLINTNEXTLINE(cert-dcl58-cpp): hash<> specialization
namespace std {
template<>
struct hash<bytes_view> {
    size_t operator()(bytes_view v) const {
        return hash<std::string_view>()(
          // NOLINTNEXTLINE
          {reinterpret_cast<const char*>(v.data()), v.size()});
    }
};

template<>
struct hash<bytes> {
    size_t operator()(const bytes& v) const { return hash<bytes_view>()(v); }
};
} // namespace std

// FIXME: remove overload from std::
// NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std {
std::ostream& operator<<(std::ostream& os, const bytes_view& b);
}

inline bool
bytes_type_eq::operator()(const bytes& lhs, const bytes& rhs) const {
    return lhs == rhs;
}
inline bool
bytes_type_eq::operator()(const bytes& lhs, const bytes_view& rhs) const {
    return bytes_view(lhs) == rhs;
}
inline bool
bytes_type_eq::operator()(const bytes& lhs, const iobuf& rhs) const {
    if (lhs.size() != rhs.size_bytes()) {
        return false;
    }
    auto iobuf_end = iobuf::byte_iterator(rhs.cend(), rhs.cend());
    auto iobuf_it = iobuf::byte_iterator(rhs.cbegin(), rhs.cend());
    size_t bytes_idx = 0;
    const size_t max = lhs.size();
    while (iobuf_it != iobuf_end && bytes_idx < max) {
        const char r_c = *iobuf_it;
        // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        const char l_c = lhs[bytes_idx];
        if (l_c != r_c) {
            return false;
        }
        // the equals case
        ++bytes_idx;
        ++iobuf_it;
    }
    return true;
}

inline bytes operator^(bytes_view a, bytes_view b) {
    if (unlikely(a.size() != b.size())) {
        throw std::runtime_error(
          "Cannot compute xor for different size byte strings");
    }
    bytes res(bytes::initialized_later{}, a.size());
    std::transform(
      a.cbegin(), a.cend(), b.cbegin(), res.begin(), std::bit_xor<>());
    return res;
}

template<size_t Size>
inline std::array<char, Size>
operator^(const std::array<char, Size>& a, const std::array<char, Size>& b) {
    std::array<char, Size> out; // NOLINT
    std::transform(
      a.begin(), a.end(), b.begin(), out.begin(), std::bit_xor<>());
    return out;
}
