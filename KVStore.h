#ifndef KVSTORE_H
#define KVSTORE_H

#include "Serializer.h"
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

template<typename V>
class KVStore {
public:
    KVStore(const std::string& filename, const std::string& metaname, size_t bufferCapacity)
        : bufferCapacity(bufferCapacity), stop(false) {
        dataFile.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        metaFile.open(metaname, std::ios::in | std::ios::out | std::ios::binary);
        if (!dataFile.is_open()) {
            dataFile.open(filename, std::ios::out | std::ios::binary);
            dataFile.close();
            dataFile.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        }
        if (!metaFile.is_open()) {
            metaFile.open(metaname, std::ios::out | std::ios::binary);
            metaFile.close();
            metaFile.open(metaname, std::ios::in | std::ios::out | std::ios::binary);
        }
        dataFile.seekg(0, std::ios::end);
        currentOffset = dataFile.tellg();
        loadMetadata();
        metaFile.clear();
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
            storageThreads.emplace_back(&KVStore::writeDisk, this);
        }
    }
    ~KVStore() {
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            stop = true;
            bufferFullCond.notify_all();
        }
        for (auto& thread : storageThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        flushBuffer();
        if (dataFile.is_open()) {
            dataFile.close();
        }
        saveMetadata();
        if (metaFile.is_open()) {
            metaFile.close();
        }
    }
    void put(int key, const V& value) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        Serializer::serialize(buffer, value);
        {
            std::lock_guard<std::mutex> mlock(metaMutex);
            std::lock_guard<std::mutex> dflock(fileMutex);
            dataFile.seekg(0, std::ios::end);
            size_t valueSize;
            if constexpr (std::is_same_v<V, std::string>) {
                valueSize = value.size();
            }
            else {
                valueSize = sizeof(value);
            }
            metadataMap[key] = { currentOffset, valueSize };
            currentOffset += valueSize;
        }
        if (buffer.size() >= bufferCapacity) {
            std::unique_lock<std::mutex> lock(queueMutex);
            taskQueue.push(buffer);
            bufferFullCond.notify_one();
            buffer.clear();
        }
    }
    V get(int key) {
        if (buffer.size())
            flushBuffer();
        if (metadataMap.find(key) != metadataMap.end()) {
            Metadata meta = metadataMap[key];
            std::lock_guard<std::mutex> fileLock(fileMutex);
            dataFile.seekg(meta.offset, std::ios::beg);
            std::vector<char> temp(meta.size);
            dataFile.read(temp.data(), meta.size);
            return Serializer::deserialize<V>(temp);
        }
    }
    bool del(int key) {
        std::lock_guard<std::mutex> lock(metaMutex);
        metadataMap.erase(key);
        return true;
    }
    void flushBuffer() {
        std::unique_lock<std::mutex> dataFileLock(fileMutex);
        dataFile.seekp(0, std::ios::end);
        dataFile.write(buffer.data(), buffer.size());
    }

private:
    struct Metadata {
        size_t offset;
        size_t size;
    };

    std::fstream dataFile;
    std::fstream metaFile;

    size_t currentOffset;
    size_t bufferCapacity;

    std::vector<char> buffer;
    std::queue<std::vector<char>> taskQueue;
    std::unordered_map<int, Metadata> metadataMap;

    std::mutex bufferMutex;
    std::mutex fileMutex;
    std::mutex metaMutex;
    std::mutex queueMutex;
    std::condition_variable bufferFullCond;

    std::vector<std::thread> bufferThreads;
    std::vector<std::thread> storageThreads;
    bool stop;

    void writeDisk() {
        while (!stop) {
            std::unique_lock<std::mutex> qlock(queueMutex);
            bufferFullCond.wait(qlock, [this] { return !taskQueue.empty() || stop; });
            while (!taskQueue.empty()) {
                auto tempBuffer = taskQueue.front();
                taskQueue.pop();
                qlock.unlock();
                {
                    std::unique_lock<std::mutex> dataFileLock(fileMutex);
                    dataFile.seekp(0, std::ios::end);
                    dataFile.write(tempBuffer.data(), tempBuffer.size());
                }
                qlock.lock();
            }
        }
    }

    void saveMetadata() {
        metaFile.seekp(0, std::ios::beg);
        size_t mapSize = metadataMap.size();
        metaFile.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));
        for (const auto& [key, meta] : metadataMap) {
            metaFile.write(reinterpret_cast<const char*>(&key), sizeof(key));
            metaFile.write(reinterpret_cast<const char*>(&meta.offset), sizeof(meta.offset));
            metaFile.write(reinterpret_cast<const char*>(&meta.size), sizeof(meta.size));
        }
        metaFile.flush();
    }
    void loadMetadata() {
        std::lock_guard<std::mutex> fileLock(metaMutex);
        metaFile.seekg(0, std::ios::beg);

        size_t mapSize;
        if (!metaFile.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize))) {
            return; // 文件为空或读取失败
        }
        size_t metadataSize = sizeof(mapSize);
        for (size_t i = 0; i < mapSize; ++i) {
            int key;
            Metadata meta;
            if (!metaFile.read(reinterpret_cast<char*>(&key), sizeof(key)) ||
                !metaFile.read(reinterpret_cast<char*>(&meta.offset), sizeof(meta.offset)) ||
                !metaFile.read(reinterpret_cast<char*>(&meta.size), sizeof(meta.size))) {
                break; // 读取失败，可能是文件损坏
            }
            metadataSize += sizeof(key) + sizeof(meta.offset) + sizeof(meta.size);
            metadataMap[key] = meta;
        }
    };
};
#endif // KVSTORE_H