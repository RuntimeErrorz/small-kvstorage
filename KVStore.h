#ifndef KVSTORE_H
#define KVSTORE_H

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>

class KVStore {
public:
    KVStore(const std::string& filename, const std::string& metaname,size_t bufferSize);
    ~KVStore();

    bool put(int key, const std::string& value);
    std::string get(int key);
    bool del(int key);
    void flushBufferNow();

private:
    struct Metadata {
        size_t offset;
        size_t size;
    };

    std::fstream dataFile;
    std::fstream metaFile;
    size_t bufferSize;
    size_t currentBufferSize;
    std::vector<std::pair<int, std::string>> buffer;
    std::unordered_map<int, Metadata> metadataMap;
    std::mutex bufferMutex;
    std::mutex fileMutex;
    std::mutex mapMutex;
    std::condition_variable bufferCond;
    std::thread ioThread;
    bool stop;

    void diskIOService();
    void flushBuffer(const std::vector<std::pair<int, std::string>>& bufferToFlush);
    void saveMetadata();
    void loadMetadata();
};

#endif // KVSTORE_H