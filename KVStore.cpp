#include "KVStore.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>

// Implementation of KVStore methods

KVStore::KVStore(const std::string& filename, const std::string& metaname, size_t bufferSize)
    : bufferSize(bufferSize), currentBufferSize(0), stop(false) {
    dataFile.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!dataFile.is_open()) {
        dataFile.open(filename, std::ios::out | std::ios::binary);
        dataFile.close();
        dataFile.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    }
    metaFile.open(metaname, std::ios::in | std::ios::out | std::ios::binary);
    if (!metaFile.is_open()) {
        metaFile.open(metaname, std::ios::out | std::ios::binary);
        metaFile.close();
        metaFile.open(metaname, std::ios::in | std::ios::out | std::ios::binary);
    }
    else
        loadMetadata();
    ioThread = std::thread(&KVStore::diskIOService, this);
}

KVStore::~KVStore() {
    flushBufferNow();
    saveMetadata();
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        stop = true;
        bufferCond.notify_all();
    }
    ioThread.join();
    if (dataFile.is_open()) {
        dataFile.close();
    }
    if (metaFile.is_open()) {
        metaFile.close();
    }
}

bool KVStore::put(int key, const std::string& value) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    buffer.emplace_back(key, value);
    currentBufferSize += sizeof(key) + value.size();
    if (currentBufferSize >= bufferSize) {
        bufferCond.notify_one();
    }
    return true;
}

std::string KVStore::get(int key) {
    std::lock_guard<std::mutex> lock(mapMutex);
    if (metadataMap.find(key) != metadataMap.end()) {
        Metadata meta = metadataMap[key];
        std::lock_guard<std::mutex> fileLock(fileMutex);
        dataFile.seekg(meta.offset, std::ios::beg);
        std::vector<char> buffer(meta.size);
        dataFile.read(buffer.data(), meta.size);
        return std::string(buffer.begin(), buffer.end());
    }
    return "";
}

bool KVStore::del(int key) {
    std::lock_guard<std::mutex> lock(mapMutex);
    metadataMap.erase(key);
    return true;
}

void KVStore::flushBufferNow() {
    std::vector<std::pair<int, std::string>> tempBuffer;
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        tempBuffer.swap(buffer);
        currentBufferSize = 0;
    }
    flushBuffer(tempBuffer);
}

void KVStore::diskIOService() {
    while (true) {
        std::vector<std::pair<int, std::string>> tempBuffer;
        {
            std::unique_lock<std::mutex> lock(bufferMutex);
            bufferCond.wait(lock, [this] { return !buffer.empty() || stop; });
            if (stop && buffer.empty()) break;
            tempBuffer.swap(buffer);
            currentBufferSize = 0;
        }
        flushBuffer(tempBuffer);
    }
}

void KVStore::flushBuffer(const std::vector<std::pair<int, std::string>>& bufferToFlush) {
    std::lock_guard<std::mutex> fileLock(fileMutex);
    dataFile.seekp(0, std::ios::end);
    for (const auto& [key, value] : bufferToFlush) {
        size_t offset = dataFile.tellp();
        size_t valueSize = value.size();
        dataFile.write(value.c_str(), valueSize);
        {
            std::lock_guard<std::mutex> mapLock(mapMutex);
            metadataMap[key] = { offset, valueSize };
        }
    }
    dataFile.flush();
}

void KVStore::saveMetadata() {
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

void KVStore::loadMetadata() {
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
}