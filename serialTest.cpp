#include "Serializer.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<char> buffer;

    // 测试 int 的序列化和反序列化
    int intValue = 42;
    Serializer::serialize(buffer, intValue);
    int deserializedInt = Serializer::deserialize<int>(buffer);
    std::cout << "原始 int: " << intValue << ", 反序列化 int: " << deserializedInt << std::endl;

    buffer.clear();  // 清空缓冲区
}
