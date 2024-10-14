#include "KVStore.h"
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <mutex>

// 生成随机字符串
std::string randomString(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += chars[dist(gen)];
    }
    return result;
}

// 向KVStore插入随机键值对
template<typename V>
void insertRandomKeys(KVStore<V>& store, std::mutex& storeMutex, int threadId, int numInsertsPerThread) {
    // 随机数生成器
    std::random_device rd;  // 用于生成随机数的种子
    std::mt19937 gen(rd());  // 随机数引擎
    std::uniform_int_distribution<> intDist(0, 100); // 整数范围
    std::uniform_real_distribution<> floatDist(0.0, 100.0); // 浮点数范围
    for (int i = 0; i < numInsertsPerThread; ++i) {
        int key = threadId * numInsertsPerThread + i; // 唯一键
        V value;
        // 根据类型生成随机数
        if constexpr (std::is_same<V, int>::value) {
            value = intDist(gen); // 生成随机整数
        }
        else if constexpr (std::is_same<V, float>::value) {
            value = floatDist(gen); // 生成随机浮点数
        }
        else if constexpr (std::is_same<V, std::string>::value) {
            value = randomString(7); // 生成随机字符串
        }
        // 加锁以保护对store的访问
        {
            std::lock_guard<std::mutex> lock(storeMutex);
            std::cout << "inserting: " << key << " " << value << std::endl;
            store.put(key, value); // 插入随机数
        }
    }
}

// 测试KVStore的插入和读取功能
template<typename V>
void testKVStoreInsert(const std::string& dataFilename, const std::string& metaFilename, int numThreads, int numInsertsPerThread, int bufferSize) {
    KVStore<V> store(dataFilename, metaFilename, bufferSize); // 128MB buffer
    std::mutex storeMutex; // 用于保护store的互斥量

    // 创建线程
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(insertRandomKeys<V>, std::ref(store), std::ref(storeMutex), i, numInsertsPerThread);
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // 读取并打印前10个键的值
    for (int i = 0; i < 10; ++i) {
        std::cout << "Key: " << i << ", Value: " << store.get(i) << std::endl;
    }
}

// 测试KVStore只读取功能
template<typename V>
void testKVStoreReadOnly(const std::string& dataFilename, const std::string& metaFilename) {
    KVStore<V> store(dataFilename, metaFilename, 16); // 128MB buffer

    // 读取并打印前10个键的值
    for (int i = 0; i < 10; ++i) {
        std::cout << "Key: " << i << ", Value: " << store.get(i) << std::endl;
    }
}

int main() {
    std::string dataFilename = "data.bin";
    std::string metaFilename = "data.meta";
    std::string dataFilename2 = "data2.bin";
    std::string metaFilename2 = "data2.meta";

    int numThreads = 3; // 并发的线程数
    int numInsertsPerThread = 3; // 每个线程要插入的键值对数
    // std::cout << "第一次测试（插入随机整数数据并读取）:" << std::endl;
    // testKVStoreInsert<int>(dataFilename, metaFilename, numThreads, numInsertsPerThread, 8);
    // std::cout << "第二次测试（重新打开KVStore并只读取随机整数数据）:" << std::endl;
    // testKVStoreReadOnly<int>(dataFilename, metaFilename);
    // std::cout << "第三次测试（插入随机字符串数据并读取）:" << std::endl;
    testKVStoreInsert<std::string>(dataFilename2, metaFilename2, numThreads, numInsertsPerThread, 20);
    // std::cout << "第四次测试（重新打开KVStore并只读取随机字符串数据）:" << std::endl;
    // testKVStoreReadOnly<std::string>(dataFilename2, metaFilename2);
    return 0;
}
