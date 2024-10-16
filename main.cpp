#include "KVStore.h"
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <mutex>
struct Person {
    int age;
    double height;
    std::string name;

    void serialize(std::vector<char>& buffer) const {
        Serializer::serialize(buffer, age);
        Serializer::serialize(buffer, height);
        Serializer::serialize(buffer, name);
    }
    static Person deserialize(const std::vector<char>& buffer) {
        Person person;
        person.age = Serializer::deserialize<int>(std::vector<char>(buffer.begin(), buffer.begin() + sizeof(int)));
        person.height = Serializer::deserialize<double>(std::vector<char>(buffer.begin() + sizeof(int), buffer.begin() + sizeof(int) + sizeof(double)));
        person.name = Serializer::deserialize<std::string>(std::vector<char>(buffer.begin() + sizeof(int) + sizeof(double), buffer.end()));
        return person;
    }
    friend std::ostream& operator<<(std::ostream& os, const Person& person) {
        os << "Person [Name: " << person.name << ", Age: " << person.age << ", Height: " << person.height << "]";
        return os;
    }
    size_t logicalSize() const {
        return name.size() + sizeof(age) + sizeof(height);
    }
};

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

template<typename V>
void insertRandomKeys(KVStore<V>& store, std::mutex& storeMutex, int threadId, int numInsertsPerThread) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> intDist(0, 100);
    std::uniform_real_distribution<> floatDist(0.0, 100.0);
    for (int i = 0; i < numInsertsPerThread; ++i) {
        int key = threadId * numInsertsPerThread + i;
        V value;
        if constexpr (std::is_same<V, int>::value) {
            value = intDist(gen);
        }
        else if constexpr (std::is_same<V, float>::value) {
            value = floatDist(gen); // 生成随机浮点数
        }
        else if constexpr (std::is_same<V, std::string>::value) {
            value = randomString(7);
        }
        else if constexpr (std::is_same<V, Person>::value) {
            value.age = intDist(gen);
            value.height = floatDist(gen);
            value.name = randomString(9);
        }
        {
            std::lock_guard<std::mutex> lock(storeMutex);
            std::cout << "inserting: " << key << " " << value << std::endl;
            store.put(key, value); // 插入随机数
        }
    }
}

template<typename V>
void testKVStoreInsert(const std::string& dataFilename, const std::string& metaFilename, int numThreads, int numInsertsPerThread, int bufferSize) {
    KVStore<V> store(dataFilename, metaFilename, bufferSize);
    std::mutex storeMutex;
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(insertRandomKeys<V>, std::ref(store), std::ref(storeMutex), i, numInsertsPerThread);
    }
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    for (int i = 0; i < 10; ++i) {
        try {
            std::cout << "Key: " << i << ", Value: " << store.get(i) << std::endl;
        }
        catch (const std::out_of_range& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

template<typename V>
void testKVStoreReadOnly(const std::string& dataFilename, const std::string& metaFilename) {
    KVStore<V> store(dataFilename, metaFilename, 16);
    for (int i = 0; i < 10; ++i) {
        try {
            std::cout << "Key: " << i << ", Value: " << store.get(i) << std::endl;
        }
        catch (const std::out_of_range& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

int main() {
    int numThreads = 3; // 并发的线程数
    int numInsertsPerThread = 3; // 每个线程要插入的键值对数

    std::cout << "1.1：插入随机整数并读取:" << std::endl;
    testKVStoreInsert<int>("int.bin", "int.meta", numThreads, numInsertsPerThread, 8);
    std::cout << "1.2：重新打开并读取整数:" << std::endl;
    testKVStoreReadOnly<int>("int.bin", "int.meta");
    std::cout << "2.1：插入随机浮点数并读取：" << std::endl;
    testKVStoreInsert<int>("float.bin", "float.meta", numThreads, numInsertsPerThread, 8);
    std::cout << "2.2：重新打开并读取浮点数：" << std::endl;
    testKVStoreReadOnly<int>("float.bin", "float.meta");
    std::cout << "3.1：插入随机字符串并读取:" << std::endl;
    testKVStoreInsert<std::string>("string.bin", "string.meta", numThreads, numInsertsPerThread, 128);
    std::cout << "3.2：重新打开并读取字符串:" << std::endl;
    testKVStoreReadOnly<std::string>("string.bin", "string.meta");
    std::cout << "4.1：插入随机自定义类并读取:" << std::endl;
    testKVStoreInsert<Person>("struct.bin", "struct.meta", numThreads, numInsertsPerThread, 128);
    std::cout << "4.2：重新打开并读取自定义类" << std::endl;
    testKVStoreReadOnly<Person>("struct.bin", "struct.meta");
    return 0;
}
