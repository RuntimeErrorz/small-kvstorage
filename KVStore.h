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
    KVStore(const std::string& filename, const std::string& metaname, size_t bufferSize)
        : bufferSize(bufferSize), currentBufferSize(0), stop(false) {
        dataFile.open(filename, std::ios::binary);
        if (!dataFile.is_open()) {
            dataFile.open(filename, std::ios::binary);
            dataFile.close();
            dataFile.open(filename, std::ios::binary);
        }
        metaFile.open(metaname, std::ios::binary);
        if (!metaFile.is_open()) {
            metaFile.open(metaname, std::ios::binary);
            metaFile.close();
            metaFile.open(metaname, std::ios::binary);
        }
        else
            loadMetadata();
        // ioThread = std::thread(&KVStore<V>::diskIOService, this);
    }
    ~KVStore() {
        flushBuffer();
        saveMetadata();
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            stop = true;
            bufferFullCond.notify_all();
        }
        // ioThread.join();
        if (dataFile.is_open()) {
            dataFile.close();
        }
        if (metaFile.is_open()) {
            metaFile.close();
        }
    }
    bool put(int key, const V& value) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        Serializer::serialize(buffer, value);
        dataFile.seekg(0, std::ios::end);
        metadataMap[key] = { dataFile.tellg() + currentBufferSize, sizeof(value) };
        currentBufferSize += sizeof(value);
        if (currentBufferSize >= bufferSize) {
            std::unique_lock<std::mutex> lock(queueMutex);
            taskQueue.push(buffer);
            bufferFullCond.notify_one();
            buffer.clear();
        }
        return true;
    }
    V get(int key) {
        std::lock_guard<std::mutex> lock(mapMutex);
        if (metadataMap.find(key) != metadataMap.end()) {
            Metadata meta = metadataMap[key];
            std::lock_guard<std::mutex> fileLock(fileMutex);
            dataFile.seekg(meta.offset, std::ios::beg);
            std::vector<char> temp(meta.size);
            dataFile.read(temp.data(), meta.size);
            return Serializer::deserialize<V>(temp);
        }
        return -1;
    }
    bool del(int key) {
        std::lock_guard<std::mutex> lock(mapMutex);
        metadataMap.erase(key);
        return true;
    }
    void flushBuffer() {
        std::lock_guard<std::mutex> fileLock(fileMutex);
        dataFile.seekp(0, std::ios::end);
        dataFile.write(buffer.data(), buffer.size());
        dataFile.flush();
    }

private:
    struct Metadata {
        size_t offset;
        size_t size;
    };

    std::fstream dataFile;
    std::fstream metaFile;
    size_t bufferSize;
    size_t currentBufferSize;
    std::vector<char> buffer;
    std::queue<std::vector<char>> taskQueue;
    std::unordered_map<int, Metadata> metadataMap;
    std::mutex bufferMutex;
    std::mutex fileMutex;
    std::mutex mapMutex;
    std::mutex queueMutex;
    std::condition_variable bufferFullCond;
    std::vector<std::thread> bufferThreads;
    std::thread storageThread;
    bool stop;

    void writeDisk() {
        while (true) {
            std::unique_lock<std::mutex> lock(bufferMutex);
            bufferFullCond.wait(lock, [this] { return !buffer.empty() || stop; });
            flushBuffer();
        }
    }

    void saveMetadata() {
        std::lock_guard<std::mutex> fileLock(fileMutex);
        metaFile.seekp(0, std::ios::beg);

        size_t mapSize = metadataMap.size();
        metaFile.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));

        for (const auto& [key, meta] : metadataMap) {
            metaFile.write(reinterpret_cast<const char*>(&key), sizeof(key));
            metaFile.write(reinterpret_cast<const char*>(&meta.offset), sizeof(meta.offset));
            metaFile.write(reinterpret_cast<const char*>(&meta.size), sizeof(meta.size));
        }
    }
    void loadMetadata() {
        std::lock_guard<std::mutex> fileLock(fileMutex);
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