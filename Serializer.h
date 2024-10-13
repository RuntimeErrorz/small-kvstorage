#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <iostream>
#include <vector>
#include <string>

class Serializer {
public:
    template<typename T>
    static void serialize(std::vector<char>& buffer, const T& value) {
        const char* data = reinterpret_cast<const char*>(&value);
        buffer.insert(buffer.end(), data, data + sizeof(T));
    }

    static void serialize(std::vector<char>& buffer, const std::string& value) {
        size_t size = value.size();
        serialize(buffer, size);
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    template<typename T>
    static void serialize(std::vector<char>& buffer, const std::vector<T>& vec) {
        size_t size = vec.size();
        serialize(buffer, size);
        for (const auto& item : vec) {
            serialize(buffer, item);
        }
    }

    template<typename T>
    static T deserialize(const std::vector<char>& buffer) {
        T value = *reinterpret_cast<const T*>(&buffer);
        return value;
    }

    // static void deserialize(const std::vector<char>& buffer, size_t& offset, std::string& value) {
    //     size_t size;
    //     deserialize(buffer, offset, size);
    //     value.assign(&buffer[offset], size);
    //     offset += size;
    // }

    // template<typename T>
    // static void deserialize(const std::vector<char>& buffer, size_t& offset, std::vector<T>& vec) {
    //     size_t size;
    //     deserialize(buffer, offset, size);
    //     vec.resize(size);
    //     for (auto& item : vec) {
    //         deserialize(buffer, offset, item);
    //     }
    // }
};

#endif // SERIALIZER_H