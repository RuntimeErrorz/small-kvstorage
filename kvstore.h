#ifndef KVSTORE_H
#define KVSTORE_H

#include "serializer.h"
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
    KVStore(const std::string& filename, const std::string& metaname, size_t bufferCapacity);
    ~KVStore();
    void put(int key, const V& value);
    V get(int key);
    void del(int key);
    void flushBuffer();

private:
    struct Metadata {
        size_t offset;
        short size;
    };

    std::fstream dataFile;
    std::fstream metaFile;

    size_t currentOffset;
    int bufferCapacity;

    std::vector<char> buffer;
    std::queue<std::vector<char>> taskQueue;
    std::unordered_map<int, Metadata> metadataMap;

    std::mutex bufferMutex;
    std::mutex fileMutex;
    std::mutex queueMutex;
    std::condition_variable bufferFullCond;

    std::vector<std::thread> storageThreads;
    bool stop;

    void writeDisk();
    void loadMetadata();
    void saveMetadata();
};


#endif // KVSTORE_H