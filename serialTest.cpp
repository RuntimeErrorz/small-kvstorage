#include <iostream>
#include <vector>
#include <cstring>  // for std::memcpy
#include <string>

class Serializer {
public:
    // 通用反序列化函数，处理基本类型
    template<typename V>
    static V deserialize(const std::vector<char>& buffer) {
        V value;
        std::memcpy(&value, buffer.data(), sizeof(V));  // 从缓冲区提取基本类型数据
        return value;
    }

    // 重载函数，处理 std::string 类型的反序列化
    static std::string deserialize(const std::vector<char>& buffer) {
        return std::string(buffer.data(), buffer.size());  // 从缓冲区提取字符串
    }

    // 序列化基本类型
    template<typename V>
    static std::vector<char> serialize(const V& value) {
        std::vector<char> buffer(sizeof(V));
        std::memcpy(buffer.data(), &value, sizeof(V));
        return buffer;
    }

    // 序列化 std::string
    static std::vector<char> serialize(const std::string& value) {
        return std::vector<char>(value.begin(), value.end());
    }
};

// 示例测试
int main() {
    // 序列化和反序列化整数
    int originalInt = 42;
    std::vector<char> intBuffer = Serializer::serialize(originalInt);
    int deserializedInt = Serializer::deserialize<int>(intBuffer);
    std::cout << "Original int: " << originalInt << ", Deserialized int: " << deserializedInt << std::endl;

    // 序列化和反序列化字符串
    std::string originalString = "Hello, World!";
    std::vector<char> stringBuffer = Serializer::serialize(originalString);
    std::string deserializedString = Serializer::deserialize(stringBuffer);  // 调用重载的deserialize
    std::cout << "Original string: " << originalString << ", Deserialized string: " << deserializedString << std::endl;

    return 0;
}
