#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <iostream>
#include <vector>
#include <string>
#include <cstring> // for memcpy

class Serializer {
public:
    // 序列化基本类型（如 int、float 等）
    template<typename T>
    static void serialize(std::vector<char>& buffer, const T& value) {
        const char* data = reinterpret_cast<const char*>(&value);
        buffer.insert(buffer.end(), data, data + sizeof(T));
    }

    // 序列化 std::string
    static void serialize(std::vector<char>& buffer, const std::string& value) {
        size_t size = value.size();
        serialize(buffer, size);  // 先序列化字符串的大小
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    // 序列化 std::vector
    template<typename T>
    static void serialize(std::vector<char>& buffer, const std::vector<T>& vec) {
        size_t size = vec.size();
        serialize(buffer, size);  // 先序列化向量的大小
        for (const auto& item : vec) {
            serialize(buffer, item);  // 逐个序列化向量的每个元素
        }
    }

    // 反序列化基本类型，默认从 buffer 开始
    template<typename T>
    static T deserialize(const std::vector<char>& buffer) {
        T value;
        std::memcpy(&value, buffer.data(), sizeof(T));
        return value;
    }

    // 反序列化 std::string
    static std::string deserializeString(const std::vector<char>& buffer) {
        size_t size = deserialize<size_t>(buffer);  // 先读取字符串大小
        std::string value(buffer.data(), size);  // 从缓冲区提取字符串
        return value;
    }

    // 反序列化 std::vector
    template<typename T>
    static std::vector<T> deserializeVector(const std::vector<char>& buffer) {
        size_t size = deserialize<size_t>(buffer);  // 先读取向量大小
        std::vector<T> vec(size);
        for (size_t i = 0; i < size; ++i) {
            vec[i] = deserialize<T>(buffer);  // 逐个反序列化向量元素
        }
        return vec;
    }

    // 重置反序列化的偏移量

};
#endif // SERIALIZER_H
