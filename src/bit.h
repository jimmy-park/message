#ifndef BIT_H_
#define BIT_H_

#include <cstdint>

#include "type_traits"

namespace bit {

namespace detail {

    constexpr std::uint16_t byteswap_u16(std::uint16_t value) noexcept
    {
        return static_cast<std::uint16_t>((value << 8) | (value >> 8));
    }

    constexpr std::uint32_t byteswap_u32(std::uint32_t value) noexcept
    {
        return (value << 24) | ((value << 8) & 0x00FF'0000) | ((value >> 8) & 0x0000'FF00) | (value >> 24);
    }

    constexpr std::uint64_t byteswap_u64(std::uint64_t value) noexcept
    {
        return (value << 56) | ((value << 40) & 0x00FF'0000'0000'0000) | ((value << 24) & 0x0000'FF00'0000'0000) | ((value << 8) & 0x0000'00FF'0000'0000)
            | ((value >> 8) & 0x0000'0000'FF00'0000) | ((value >> 24) & 0x0000'0000'00FF'0000) | ((value >> 40) & 0x0000'0000'0000'FF00) | (value >> 56);
    }

} // namespace detail

template <typename T>
constexpr T byteswap(T value) noexcept
{
    static_assert(std::is_integral_v<T>);

    using U = std::make_unsigned_t<T>;

    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(detail::byteswap_u16(static_cast<U>(value)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(detail::byteswap_u32(static_cast<U>(value)));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(detail::byteswap_u64(static_cast<U>(value)));
    } else {
        static_assert(type_traits::always_false_v<T>);
    }
}

enum class endian {
#ifdef _WIN32
    little = 0,
    big = 1,
    native = little
#else
    little = __ORDER_LITTLE_ENDIAN__,
    big = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};

template <typename T>
struct hton_t {
    constexpr T operator()(const T& value) const
    {
        if constexpr (endian::native == endian::little)
            return byteswap(value);
        else
            return value;
    }
};

template <typename T>
using ntoh_t = hton_t<T>;

template <typename T>
inline constexpr auto hton = hton_t<T> {};

template <typename T>
inline constexpr auto ntoh = hton<T>;

} // namespace bit

#endif // BIT_H_