#include "kafka/server/handlers/handlers.h"

struct handler_interface {
    handler_interface() = default;
    handler_interface(const handler_interface&) = delete;
    handler_interface& operator=(const handler_interface&) = delete;
    handler_interface(handler_interface&&) noexcept = delete;
    handler_interface& operator=(handler_interface&&) noexcept = delete;
    virtual ~handler_interface() = default;

    virtual kafka::api_version min_supported() const = 0;
    virtual kafka::api_version max_supported() const = 0;
    virtual void operator()(iobuf, kafka::api_version) const = 0;
};

template<typename T>
struct handler_base final : public handler_interface {
    kafka::api_version min_supported() const override {
        return T::min_supported;
    }
    kafka::api_version max_supported() const override {
        return T::max_supported;
    }
    void operator()(iobuf data, kafka::api_version version) const override {
        kafka::request_reader reader(std::move(data));
        typename T::api::request_type request;
        request.decode(reader, version);
    }
};

template<typename H>
struct handler_holder {
    static const inline handler_base<H> instance{};
};

template<typename... Ts>
constexpr auto make_lut(kafka::type_list<Ts...>) {
    constexpr int max_index = std::max({Ts::api::key...});

    std::array<const handler_interface*, max_index + 1> lut{};
    ((lut[Ts::api::key] = &handler_holder<Ts>::instance), ...);

    return lut;
}

template<typename T>
T read(const uint8_t*& data, size_t& size) {
    typename T::type ret;
    std::memcpy(&ret, data, sizeof(T));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    data += sizeof(T);
    size -= sizeof(T);
    return T(ret);
}

static constexpr auto ok = std::to_array<std::string_view>({
  "Null topics received for version 0 of metadata request",
  "Cannot decode string as UTF8",
  "sstring overflow",
});

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < (sizeof(kafka::api_key) + sizeof(kafka::api_version))) {
        return -1;
    }

    const auto key = kafka::api_key(read<kafka::api_key>(data, size));
    const auto ver = kafka::api_version(read<kafka::api_version>(data, size));

    static constexpr auto lut = make_lut(kafka::request_types{});
    if (key < kafka::api_key(0) || key >= kafka::api_key(lut.size())) {
        return -1;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    auto handler = lut[key];
    if (!handler) {
        return -1;
    }

    if (ver < handler->min_supported() || ver > handler->max_supported()) {
        return -1;
    }

    iobuf buf;
    buf.append(data, size);

    try {
        handler->operator()(std::move(buf), ver);
    } catch (const std::out_of_range&) {
        return -1;
    } catch (const std::bad_alloc&) {
        return -1;
    } catch (const std::runtime_error& e) {
        for (const auto& s : ok) {
            if (e.what() == s) {
                return -1;
            }
        }
        throw;
    }

    return 0;
}
