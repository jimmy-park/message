#include "message.h"

#include <cassert>
#include <cstddef>
#include <cstring>

#include <execution>
#include <functional>
#include <limits>
#include <numeric>

namespace detail {

template <typename, typename = void>
inline constexpr bool is_container_like = false;

template <typename T>
inline constexpr bool is_container_like<T, std::void_t<typename T::value_type, decltype(std::declval<T>().size())>> = true;

template <typename, typename = void>
inline constexpr bool has_contiguous_memory = false;

template <typename T>
inline constexpr bool has_contiguous_memory<T, std::void_t<typename T::iterator::iterator_category>> = std::is_same_v<typename T::iterator::iterator_category, std::random_access_iterator_tag>;

template <typename T>
inline constexpr bool is_array_like = is_container_like<T>&& has_contiguous_memory<T>;

template <typename T>
constexpr std::uint8_t CalculateArraySize(T value)
{
    static_assert(std::is_integral_v<T>);

    if (value <= std::numeric_limits<std::uint8_t>::max())
        return 0b0001;
    else if (value <= std::numeric_limits<std::uint16_t>::max())
        return 0b0010;
    else if (value <= std::numeric_limits<std::uint32_t>::max())
        return 0b0100;
    else if (value <= std::numeric_limits<std::uint64_t>::max())
        return 0b1000;
    else
        return 0b0000;
}

std::size_t CalculateTotalSize(const Message::Items& items)
{
    return std::transform_reduce(
        std::execution::par_unseq,
        std::cbegin(items),
        std::cend(items),
        std::size(items),
        std::plus<> {},
        [](const auto& item) {
            return std::visit([](auto&& value) {
                using T = detail::remove_cvref_t<decltype(value)>;

                if constexpr (detail::is_array_like<T>) {
                    const auto container_size = std::size(value);
                    const auto array_size = CalculateArraySize(container_size);

                    return array_size + container_size * sizeof(typename T::value_type);
                } else {
                    return sizeof(T);
                }
            },
                item);
        });
}

// Encode type and size within 8-bit
// X : type code, Y : size code
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
    assert(item.index() <= 0b0000'1111);

    const auto array_size = std::visit(
        [](auto&& value) {
            using T = detail::remove_cvref_t<decltype(value)>;

            if constexpr (detail::is_array_like<T>) {
                assert(std::size(value) != 0);
                return CalculateArraySize(std::size(value));
            } else {
                return std::uint8_t { 0 };
            }
        },
        item);

    return array_size << 4 | static_cast<std::uint8_t>(item.index());
}

template <typename T>
void SerializeInt(std::vector<std::uint8_t>& buffer, T value)
{
    static_assert(std::is_integral_v<T>);

    // value = hton(value);
    const auto* ptr = reinterpret_cast<const std::uint8_t*>(&value);

    buffer.insert(std::cend(buffer), ptr, ptr + sizeof(T));
}

template <typename T>
void SerializeArray(std::vector<std::uint8_t>& buffer, const T& value)
{
    static_assert(detail::is_array_like<T>);

    const auto container_size = std::size(value);
    const auto array_size = CalculateArraySize(container_size);

    switch (array_size) {
    case 0b0001:
        SerializeInt(buffer, static_cast<std::uint8_t>(container_size));
        break;
    case 0b0010:
        SerializeInt(buffer, static_cast<std::uint16_t>(container_size));
        break;
    case 0b0100:
        SerializeInt(buffer, static_cast<std::uint32_t>(container_size));
        break;
    case 0b1000:
        SerializeInt(buffer, static_cast<std::uint64_t>(container_size));
        break;
    default:
        assert(false);
    }

    const auto value_size = container_size * sizeof(typename T::value_type);
    const auto* value_ptr = reinterpret_cast<const std::uint8_t*>(std::data(value));

    buffer.insert(std::cend(buffer), value_ptr, value_ptr + value_size);
}

std::pair<Message::Item, std::uint8_t> Decode(std::uint8_t code)
{
    std::pair<Message::Item, std::uint8_t> ret;
    auto& [item, size] = ret;

    const auto type_code = code & 0b0000'1111;
    const auto size_code = code & 0b1111'0000;

    size = static_cast<std::uint8_t>(size_code >> 4);

    assert((type_code < 10 && size == 0) || (type_code >= 10 && size != 0));

    switch (type_code) {
    // Integer type
    case 0:
        item.emplace<0>();
        break;
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
    // Array type
    case 10:
        item.emplace<10>();
        break;
    case 11:
        item.emplace<11>();
        break;
    case 12:
        item.emplace<12>();
        break;
    default:
        assert(false);
    }

    return ret;
}

template <typename T>
T DeserializeInt(const std::uint8_t* first)
{
    static_assert(std::is_integral_v<T>);

    T value {};
    auto* ptr = reinterpret_cast<std::uint8_t*>(&value);

    std::memcpy(ptr, first, sizeof(T));

    // value = ntoh(value);
    return value;
}

template <typename T>
std::size_t DeserializeArray(T& value, const std::uint8_t* first, std::uint8_t array_size)
{
    static_assert(detail::is_array_like<T>);

    std::size_t container_size {};

    switch (array_size) {
    case 0b0001:
        container_size = DeserializeInt<std::uint8_t>(first);
        first += sizeof(std::uint8_t);
        break;
    case 0b0010:
        container_size = DeserializeInt<std::uint16_t>(first);
        first += sizeof(std::uint16_t);
        break;
    case 0b0100:
        container_size = DeserializeInt<std::uint32_t>(first);
        first += sizeof(std::uint32_t);
        break;
    case 0b1000:
        container_size = static_cast<std::size_t>(DeserializeInt<std::uint64_t>(first));
        first += sizeof(std::uint64_t);
        break;
    default:
        assert(false);
    }

    const auto* ptr = reinterpret_cast<const typename T::value_type*>(first);

    value.reserve(container_size);
    value.assign(ptr, ptr + container_size);

    return array_size + container_size * sizeof(typename T::value_type);
}

} // namespace detail

std::vector<std::uint8_t> Message::Serialize(const Message& message)
{
    std::vector<std::uint8_t> buffer;

    const auto& items = message.body;
    const auto total_size = detail::CalculateTotalSize(items);
    buffer.reserve(total_size);

    for (const auto& item : items) {
        const auto code = detail::Encode(item);
        detail::SerializeInt(buffer, code);

        std::visit([&buffer](auto&& value) {
            using T = detail::remove_cvref_t<decltype(value)>;

            if constexpr (detail::is_array_like<T>) {
                detail::SerializeArray(buffer, value);
            } else {
                detail::SerializeInt(buffer, value);
            }
        },
            item);
    }

    assert(total_size == buffer.capacity() && total_size == buffer.size());

    return buffer;
}

Message Message::Deserialize(const std::vector<std::uint8_t>& buffer)
{
    Message message;

    for (auto first = std::data(buffer), last = first + std::size(buffer); first != last;) {
        auto [item, size] = detail::Decode(*first);
        ++first;

        const auto read = std::visit([first, last, size = size](auto&& value) {
            using T = detail::remove_cvref_t<decltype(value)>;

            if constexpr (detail::is_array_like<T>) {
                assert(first + size <= last);

                return detail::DeserializeArray(value, first, size);
            } else {
                assert(first + sizeof(T) <= last);

                value = detail::DeserializeInt<T>(first);
                return sizeof(T);
            }
        },
            item);

        message.body.emplace_back(std::move(item));
        first += read;
    }

    return message;
}