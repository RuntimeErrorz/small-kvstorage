#include <iostream>
#include <fstream>
#include <cstring> // for strlen

int main() {
    // 创建并写入初始数据（如果文件不存在）
    {
        std::ofstream outFile("example.bin", std::ios::binary);
        const char* initialData = "Hello, World!";
        outFile.write(initialData, strlen(initialData));
        outFile.close();
    }

    // 使用 std::ios::in | std::ios::out | std::ios::binary 打开文件
    std::fstream file("example.bin", std::ios::in | std::ios::out | std::ios::binary);

    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "无法打开文件!" << std::endl;
        return 1;
    }

    // 读取文件内容
    char buffer[50];
    file.seekg(0, std::ios::beg); // 移动到文件开头
    file.read(buffer, sizeof(buffer) - 1); // 读取文件内容
    buffer[file.gcount()] = '\0'; // 确保 buffer 以 null 结尾
    std::cout << "文件内容: " << buffer << std::endl;

    // 在文件开头写入新的数据
    const char* newData = "TS"; // 要写入的数据

    // 清除状态标志

    file.seekp(0, std::ios::beg); // 移动到文件开头
    file.write(newData, strlen(newData)); // 写入数据

    // 检查写入位置
    if (file.tellp() == -1) {
        std::cerr << "写入位置错误!" << std::endl;
        return 1;
    }

    // 再次读取文件内容
    file.seekg(0, std::ios::beg); // 移动到文件开头
    file.read(buffer, sizeof(buffer) - 1); // 读取文件内容
    buffer[file.gcount()] = '\0'; // 确保 buffer 以 null 结尾
    std::cout << "修改后的文件内容: " << buffer << std::endl;

    file.close(); // 关闭文件
    return 0;
}