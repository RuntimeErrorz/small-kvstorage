#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <iostream>
#include <vector>
#include <string>
#include <cstring> // for memcpy

class Serializer {
public:
    template<typename V>
    static void serialize(std::vector<char>& buffer, const V& value) {
        const char* data = reinterpret_cast<const char*>(&value);
        buffer.insert(buffer.end(), data, data + sizeof(V));
    }

    static void serialize(std::vector<char>& buffer, const std::string& value) {
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

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
