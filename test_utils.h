#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <iostream>
#include <thread>
#include <stdexcept>
#include "kvstore.h" 
#include "struct.h"

inline std::string randomString(size_t length, std::mt19937& gen) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(chars[dist(gen)]);
    }
    return result;
}

template<typename V>
void insertRandomKeys(KVStore<V>& store, std::mutex& storeMutex, int threadId, int numInsertsPerThread, std::mt19937& gen, bool isStress) {
    std::uniform_int_distribution<> intDist(0, 100);
    std::uniform_real_distribution<> doubleDist(0.0, 100.0);

    for (int i = 0; i < numInsertsPerThread; ++i) {
        int key = threadId * numInsertsPerThread + i;
        V value;
        if constexpr (std::is_same<V, int>::value)
            value = intDist(gen);
        else if constexpr (std::is_same<V, double>::value)
            value = doubleDist(gen);
        else if constexpr (std::is_same<V, std::string>::value)
            value = randomString(8, gen);
        else if constexpr (std::is_same<V, Person>::value) {
            value.age = intDist(gen);
            value.height = doubleDist(gen);
            value.name = randomString(8, gen);
        }
        {
            std::lock_guard<std::mutex> lock(storeMutex);
            if (!isStress)
                std::cout << "Inserting key: " << key << ", Value: " << value << std::endl;
            store.put(key, value);
        }
    }
}

template<typename V>
void KVStorePutGetDel(bool isStress, const std::string& dataFilename, const std::string& metaFilename, int numThreads, int numInsertsPerThread, int bufferSize) {
    KVStore<V> store(dataFilename, metaFilename, bufferSize);
    std::mutex storeMutex;

    std::vector<std::thread> threads;
    std::random_device rd;
    std::mt19937 gen(rd());
    for (int i = 0; i < numThreads; ++i)
        threads.emplace_back(insertRandomKeys<V>, std::ref(store), std::ref(storeMutex), i, numInsertsPerThread, std::ref(gen), isStress);
    for (auto& thread : threads)
        if (thread.joinable())
            thread.join();
    for (int i = 5; i <= 10; ++i) {
        try {
            store.del(i);
            std::cout << "Deleting key: " << i << std::endl;
        }
        catch (const std::out_of_range& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
    KVStoreGetImpl(store);
}

template<typename V>
void KVStoreGet(const std::string& dataFilename, const std::string& metaFilename) {
    KVStore<V> store(dataFilename, metaFilename, 16);
    KVStoreGetImpl(store);
}

template<typename V>
void KVStoreGetImpl(KVStore<V>& store) {
    for (int i = 0; i < 10; ++i) {
        try {
            V value = store.get(i);
            std::cout << "Key: " << i << ", Value: " << value << std::endl;
        }
        catch (const std::out_of_range& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}
#endif
