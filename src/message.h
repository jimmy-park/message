#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <cstdint>

#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "type_traits.h"

struct Message {
    using Item = std::variant<
        /* Integer type */ bool, char, std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t,
        /* Array type   */ std::vector<std::uint8_t>, std::vector<int>, std::string>;
    using Items = std::vector<Item>;

    // Number of types is limited due to encoding
    static_assert(std::variant_size_v<Item> <= 0x0F);

    static constexpr auto kDefaultMessageSize = 10;

    ~Message() = default;
    Message(const Message&) = delete;
    Message(Message&&) noexcept = default;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) noexcept = default;

    Message()
    {
        body.reserve(kDefaultMessageSize);
    }

    template <typename Id, typename... Values>
    Message(Id id, Values&&... values)
    {
        static_assert(std::is_integral_v<Id> || std::is_enum_v<Id>);

        auto& self = *this;

        self.id = static_cast<std::uint16_t>(id);
        self.body.reserve(sizeof...(values) != 0 ? sizeof...(values) : kDefaultMessageSize);

        (self << ... << std::forward<Values>(values));
    }

    template <typename Value>
    friend Message& operator<<(Message& message, Value&& value)
    {
        using T = type_traits::remove_cvref_t<Value>;
        static_assert(std::is_constructible_v<Item, T> || std::is_constructible_v<std::string, T> || std::is_enum_v<T>);

        if constexpr (std::is_constructible_v<std::string, T>) {
            message.body.emplace_back(std::in_place_type<std::string>, std::forward<Value>(value));
        } else if constexpr (std::is_enum_v<T>) {
            using Int = std::underlying_type_t<T>;
            message.body.emplace_back(std::in_place_type<Int>, static_cast<Int>(value));
        } else {
            message.body.emplace_back(std::in_place_type<T>, std::forward<Value>(value));
        }

        return message;
    }

    template <typename Value>
    friend Message& operator>>(Message& message, Value& value)
    {
        static_assert(std::is_constructible_v<Item, Value> || std::is_enum_v<Value>);

        if constexpr (std::is_enum_v<Value>) {
            using Int = std::underlying_type_t<Value>;
            value = static_cast<Value>(std::get<Int>(std::move(message.body.back())));
        } else {
            value = std::get<Value>(std::move(message.body.back()));
        }

        message.body.pop_back();

        return message;
    }

    static std::vector<std::uint8_t> Serialize(const Message& message);
    static Message Deserialize(const std::vector<std::uint8_t>& buffer);

    std::string from;
    std::string to;
    Items body;
    std::uint16_t id { 0 };
};

#endif // MESSAGE_H_