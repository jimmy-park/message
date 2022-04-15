#include "message.h"

#include <cassert>
#include <cstddef>
#include <cstring>

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <memory_resource>
#include <numeric>

#include "util/bit.h"

namespace detail {

template <typename T>
inline constexpr bool is_array_like = type_traits::is_vector_v<T> || std::is_same_v<T, std::string>;

template <typename T>
constexpr std::uint8_t CalculateArraySize(T value)
{
    static_assert(std::is_integral_v<T>);

    if (value <= std::numeric_limits<std::uint8_t>::max())
        return 1;
    else if (value <= std::numeric_limits<std::uint16_t>::max())
        return 2;
    else if (value <= std::numeric_limits<std::uint32_t>::max())
        return 4;
    else if (value <= std::numeric_limits<std::uint64_t>::max())
        return 8;
    else
        return 0;
}

std::size_t CalculateTotalSize(const Message::Items& items)
{
    return std::transform_reduce(
        std::cbegin(items),
        std::cend(items),
        std::size(items),
        std::plus<> {},
        [](const auto& item) {
            return std::visit([](auto&& value) -> std::size_t {
                using T = type_traits::remove_cvref_t<decltype(value)>;

                if constexpr (std::is_integral_v<T>) {
                    return sizeof(T);
                } else if constexpr (detail::is_array_like<T>) {

                    const auto container_size = std::size(value);
                    const auto array_size = CalculateArraySize(container_size);

                    return array_size + container_size * sizeof(typename T::value_type);
                } else {
                    return 0;
                }
            },
                item);
        });
}

// Encode type and size within 8-bit
// X : type code, Y : size code
//
// Default type
//  - Used for error handling
// +--------+
// |00000000|
// +--------+
//
// Integer type
//  - DATA has fixed length for each integer
// +--------+========+
// |0000XXXX|  DATA  |
// +--------+========+
//
// Array type
//  - DATA has arbitrary length
//  - Length of SIZE depends on length of DATA
// +--------+========+~~~~~~~~+
// |YYYYXXXX|  SIZE  |  DATA  |
// +--------+========+~~~~~~~~+
std::uint8_t Encode(const Message::Item& item)
{
    const auto size_code = std::visit(
        [](auto&& value) -> std::uint8_t {
            using T = type_traits::remove_cvref_t<decltype(value)>;

            if constexpr (std::is_integral_v<T>) {
                return 0;
            } else if constexpr (detail::is_array_like<T>) {
                return CalculateArraySize(std::size(value));
            } else {
                return 0;
            }
        },
        item);

    return (size_code << 4) | static_cast<std::uint8_t>(item.index());
}

template <typename T>
void SerializeInt(std::vector<std::uint8_t>& buffer, T value)
{
    static_assert(std::is_integral_v<T>);

    if constexpr (sizeof(T) == 1) {
        buffer.emplace_back(static_cast<std::uint8_t>(value));
    } else {
        if constexpr (bit::endian::native == bit::endian::little && sizeof(T) != 1)
            value = bit::hton(value);

        const auto* ptr = reinterpret_cast<const std::uint8_t*>(&value);
        buffer.insert(std::cend(buffer), ptr, ptr + sizeof(T));
    }
}

template <typename T>
void SerializeArray(std::vector<std::uint8_t>& buffer, const T& container)
{
    static_assert(detail::is_array_like<T>);

    const auto container_size = std::size(container);
    const auto array_size = CalculateArraySize(container_size);
    const auto value_size = container_size * sizeof(typename T::value_type);

    switch (array_size) {
    case 1:
        SerializeInt(buffer, static_cast<std::uint8_t>(container_size));
        break;
    case 2:
        SerializeInt(buffer, static_cast<std::uint16_t>(container_size));
        break;
    case 4:
        SerializeInt(buffer, static_cast<std::uint32_t>(container_size));
        break;
    case 8:
        SerializeInt(buffer, static_cast<std::uint64_t>(container_size));
        break;
    default:
        assert(false);
        break;
    }

    if (container_size == 0)
        return;

    if constexpr (bit::endian::native == bit::endian::little && sizeof(typename T::value_type) != 1) {
        static constexpr std::size_t kBufferSize { 1024 * sizeof(typename T::value_type) };

        std::array<std::byte, kBufferSize> stack_buffer;
        std::pmr::monotonic_buffer_resource mbr { std::data(stack_buffer), std::size(stack_buffer) };
        std::pmr::vector<typename T::value_type> copy { std::cbegin(container), std::cend(container), &mbr };

        std::transform(
            std::begin(copy),
            std::end(copy),
            std::begin(copy),
            bit::hton<typename T::value_type>);

        const auto* value_ptr = reinterpret_cast<const std::uint8_t*>(std::data(copy));
        buffer.insert(std::cend(buffer), value_ptr, value_ptr + value_size);
    } else {
        const auto* value_ptr = reinterpret_cast<const std::uint8_t*>(std::data(container));
        buffer.insert(std::cend(buffer), value_ptr, value_ptr + value_size);
    }
}

std::pair<Message::Item, std::uint8_t> Decode(std::uint8_t code)
{
    std::pair<Message::Item, std::uint8_t> ret;
    auto& [item, size] = ret;

    const auto type_code = code & 0x0F;
    const auto size_code = code & 0xF0;

    switch (type_code) {
    // Integer type
    case 1:
        item.emplace<1>();
        break;
    case 2:
        item.emplace<2>();
        break;
    case 3:
        item.emplace<3>();
        break;
    case 4:
        item.emplace<4>();
        break;
    case 5:
        item.emplace<5>();
        break;
    case 6:
        item.emplace<6>();
        break;
    case 7:
        item.emplace<7>();
        break;
    case 8:
        item.emplace<8>();
        break;
    case 9:
        item.emplace<9>();
        break;
    case 10:
        item.emplace<10>();
        break;
    // Array type
    case 11:
        item.emplace<11>();
        break;
    case 12:
        item.emplace<12>();
        break;
    case 13:
        item.emplace<13>();
        break;
    // Default type
    default:
        // item.emplace<0>();
        assert(false);
        break;
    }

    auto has_single_bit = [](auto x) { return x != 0 && (x & (x - 1)) == 0; };

    if (type_code >= 10 && type_code <= 12 && !has_single_bit(size_code)) {
        item.emplace<0>();
    } else if (type_code < 10 && size_code != 0) {
        item.emplace<0>();
    } else {
        size = static_cast<std::uint8_t>(size_code >> 4);
    }

    return ret;
}

template <typename T>
T DeserializeInt(const std::uint8_t* first)
{
    static_assert(std::is_integral_v<T>);

    if constexpr (sizeof(T) == 1) {
        return static_cast<T>(*first);
    } else {
        T value;
        auto* ptr = reinterpret_cast<std::uint8_t*>(&value);

        std::memcpy(ptr, first, sizeof(T));

        if constexpr (bit::endian::native == bit::endian::little)
            value = bit::ntoh(value);

        return value;
    }
}

template <typename T>
std::size_t DeserializeArray(T& container, const std::uint8_t* first, std::uint8_t array_size)
{
    static_assert(detail::is_array_like<T>);

    auto container_size = [first, array_size]() -> std::size_t {
        switch (array_size) {
        case 1:
            return DeserializeInt<std::uint8_t>(first);
        case 2:
            return DeserializeInt<std::uint16_t>(first);
        case 4:
            return DeserializeInt<std::uint32_t>(first);
        case 8:
            return static_cast<std::size_t>(DeserializeInt<std::uint64_t>(first));
        default:
            assert(false);
            return 0;
        }
    }();

    if (container_size == 0)
        return array_size;

    const auto* ptr = reinterpret_cast<const typename T::value_type*>(first + array_size);

    container.reserve(container_size);
    container.assign(ptr, ptr + container_size);

    if constexpr (bit::endian::native == bit::endian::little && sizeof(typename T::value_type) != 1) {
        std::transform(
            std::begin(container),
            std::end(container),
            std::begin(container),
            bit::ntoh<typename T::value_type>);
    }

    return array_size + container_size * sizeof(typename T::value_type);
}

} // namespace detail

std::vector<std::uint8_t> Message::Serialize(const Message& message)
{
    std::vector<std::uint8_t> buffer;

    const auto& items = message.body;
    const auto total_size = 1 /* item_count */ + detail::CalculateTotalSize(items);

    if (total_size != 1) {
        buffer.reserve(total_size);

        const auto item_count = static_cast<std::uint8_t>(std::min(std::size_t { 0xFF }, std::size(items)));
        detail::SerializeInt(buffer, item_count);
    }

    for (const auto& item : items) {
        const auto code = detail::Encode(item);
        if (code == 0) {
            assert(false);
            buffer.clear();
            buffer.shrink_to_fit();
            break;
        }

        detail::SerializeInt(buffer, code);

        std::visit([&buffer](auto&& value) {
            using T = type_traits::remove_cvref_t<decltype(value)>;

            if constexpr (std::is_integral_v<T>) {
                detail::SerializeInt(buffer, value);
            } else if constexpr (detail::is_array_like<T>) {
                detail::SerializeArray(buffer, value);
            }
        },
            item);
    }

    assert(total_size == 1 || (total_size == buffer.capacity() && total_size == buffer.size()));

    return buffer;
}

std::optional<Message> Message::Deserialize(const std::vector<std::uint8_t>& buffer)
{
    auto maybe_message = std::make_optional<Message>();
    auto& message = *maybe_message;

    if (!buffer.empty()) {
        auto first = std::data(buffer);
        const auto last = first + std::size(buffer);

        const auto item_count = detail::DeserializeInt<std::uint8_t>(first);
        message.body.reserve(item_count);

        ++first;

        while (first < last) {
            auto [item, size] = detail::Decode(*first);
            if (item.index() == 0)
                break;

            ++first;

            const auto read = std::visit([first, size = size](auto&& value) -> std::size_t {
                using T = type_traits::remove_cvref_t<decltype(value)>;

                if constexpr (std::is_integral_v<T>) {
                    value = detail::DeserializeInt<T>(first);
                    return sizeof(T);
                } else if constexpr (detail::is_array_like<T>) {
                    return detail::DeserializeArray(value, first, size);
                } else {
                    return 0;
                }
            },
                item);

            message.body.emplace_back(std::move(item));
            first += read;

            assert(first <= last);
        }

        if (first != last)
            maybe_message.reset();
    }

    return maybe_message;
}