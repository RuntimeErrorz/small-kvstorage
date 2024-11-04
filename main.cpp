#include "kvstore.h"
#include "test_utils.h"
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <mutex>


int main() {
    int numThreads = 3; // 并发的线程数
    int numInsertsPerThread = 3; // 每个线程要插入的键值对数

    std::cout << "-----------1.1：插入随机整数并读取-----------" << std::endl;
    KVStorePutDelGet<int>(false, "int.bin", "int.meta", numThreads, numInsertsPerThread, 128);
    std::cout << "-----------1.2：重新打开并读取整数-----------" << std::endl;
    KVStoreGet<int>(false, "int.bin", "int.meta");
    std::cout << "-----------2.1：插入随机浮点数并读取-----------" << std::endl;
    KVStorePutDelGet<double>(false, "double.bin", "double.meta", numThreads, numInsertsPerThread, 128);
    std::cout << "-----------2.2：重新打开并读取浮点数-----------m" << std::endl;
    KVStoreGet<double>(false, "double.bin", "double.meta");
    std::cout << "-----------3.1：插入随机字符串并读取-----------" << std::endl;
    KVStorePutDelGet<std::string>(false, "string.bin", "string.meta", numThreads, numInsertsPerThread, 128);
    std::cout << "-----------3.2：重新打开并读取字符串-----------" << std::endl;
    KVStoreGet<std::string>(false, "string.bin", "string.meta");
    std::cout << "-----------4.1：插入随机自定义结构体并读取-----------" << std::endl;
    KVStorePutDelGet<Person>(false, "struct.bin", "struct.meta", numThreads, numInsertsPerThread, 128);
    std::cout << "-----------4.2：重新打开并读取自定义结构体-----------" << std::endl;
    KVStoreGet<Person>(false, "struct.bin", "struct.meta");
    return 0;
}
