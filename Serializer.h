#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <iostream>
#include <vector>
#include <string>
#include <cstring> // for memcpy

class Serializer {
public:
    // 序列化基本类型（如 int、float 等）
    template<typename V>
    static void serialize(std::vector<char>& buffer, const V& value) {
        const char* data = reinterpret_cast<const char*>(&value);
        buffer.insert(buffer.end(), data, data + sizeof(V));
    }

    // 序列化 std::string
    static void serialize(std::vector<char>& buffer, const std::string& value) {
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    // 序列化 std::vector

    // 反序列化基本类型，默认从 buffer 开始
    template<typename V>
    static V deserialize(const std::vector<char>& buffer) {
        if constexpr (std::is_same_v<V, std::string>) {
            return std::string(buffer.data(), buffer.size());
        }
        else {
            V value;
            std::memcpy(&value, buffer.data(), sizeof(V));
            return value;
        }
    }
};
#endif // SERIALIZER_H
