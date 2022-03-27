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

    result >> a >> b >> c;

    return 0;
}
```

[Compiler Explorer](https://godbolt.org/z/ehE6fhY7x)

### Message Transfer

```cpp
```

## Reference

- [Better Code: Concurrency - Sean Parent](https://www.youtube.com/watch?v=zULU6Hhp42w)
