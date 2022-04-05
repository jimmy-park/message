# Message

Implement messaging system using C++17 features (std::variant, std::execution, ...)

## Usage

### Data Serialization

```cpp
#include "message.h"

int main()
{
    Message msg { /* id, value1, value2, ... */ };
    msg << 123
        << "abc"
        << std::vector<int> { 1, 2, 3, 4, 5 };

    auto buffer = Message::Serialize(msg);
    auto result = Message::Deserialize(buffer);

    assert(msg.body == result.body);

    std::vector<int> a;
    std::string b;
    int c;

    // Take out in reverse order
    result >> a >> b >> c;

    return 0;
}
```

[Compiler Explorer](https://godbolt.org/z/cqPv4zfjj)

### Message Transfer

```cpp
#include "example/client.h"

int main()
{
    Client client_a { "A" }, client_b { "B" };

    for (std::string line; std::getline(std::cin, line);) {
        if (line.empty())
            break;

        // Equivalent to
        //
        // Message msg;
        // msg.from = "A";
        // msg.to = "B";
        // msg << line;
        // MessageRouter::GetInstance().Post(std::move(msg));
        client_a.Send("B", line);
    }

    return 0;
}
```

## Reference

- [MessagePack](https://github.com/msgpack/msgpack/blob/master/spec.md)
- [Better Code: Concurrency - Sean Parent](https://www.youtube.com/watch?v=zULU6Hhp42w)
