#ifndef BIT_H_
#define BIT_H_

#include <cstdint>

#ifdef _MSC_VER
#include <cstdlib>
#endif

#include "type_traits.h"

namespace bit {

namespace detail {

    std::uint16_t byteswap_u16(std::uint16_t value) noexcept
    {
#ifdef _MSC_VER
        return _byteswap_ushort(value);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap16(value);
#else
        return static_cast<std::uint16_t>((value << 8) | (value >> 8));
#endif
    }

    std::uint32_t byteswap_u32(std::uint32_t value) noexcept
    {
#ifdef _MSC_VER
        return _byteswap_ulong(value);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(value);
#else
        return (value << 24) | ((value << 8) & 0x00FF'0000) | ((value >> 8) & 0x0000'FF00) | (value >> 24);
#endif
    }

    std::uint64_t byteswap_u64(std::uint64_t value) noexcept
    {
#ifdef _MSC_VER
        return _byteswap_uint64(value);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap64(value);
#else
        return (value << 56) | ((value << 40) & 0x00FF'0000'0000'0000) | ((value << 24) & 0x0000'FF00'0000'0000) | ((value << 8) & 0x0000'00FF'0000'0000)
            | ((value >> 8) & 0x0000'0000'FF00'0000) | ((value >> 24) & 0x0000'0000'00FF'0000) | ((value >> 40) & 0x0000'0000'0000'FF00) | (value >> 56);
#endif
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
#ifdef _MSC_VER
    little = 0,
    big = 1,
    native = little
#elif defined(__GNUC__) || defined(__clang__)
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