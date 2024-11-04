#include "kvstore.h"
#include "struct.h"

template<typename V>
KVStore<V>::KVStore(const std::string& filename, const std::string& metaname, size_t bufferCapacity)
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
    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
        storageThreads.emplace_back(&KVStore::writeDisk, this);
}

template<typename V>
KVStore<V>::~KVStore() {
    stop = true;
    bufferFullCond.notify_all();
    for (auto& thread : storageThreads)
        if (thread.joinable())
            thread.join();
    if (buffer.size())
        flushBuffer();
    if (dataFile.is_open())
        dataFile.close();
    saveMetadata();
    if (metaFile.is_open())
        metaFile.close();
}

template<typename V>
void KVStore<V>::put(int key, const V& value) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    if constexpr (std::is_class<V>::value && !std::is_same_v<V, std::string>)
        value.serialize(buffer);
    else
        Serializer::serialize(buffer, value);

    short valueSize;
    if constexpr (std::is_class<V>::value && !std::is_same_v<V, std::string>)
        valueSize = value.logicalSize();
    else if constexpr (std::is_same_v<V, std::string>)
        valueSize = value.size();
    else
        valueSize = sizeof(value);

    metadataMap[key] = Metadata{ currentOffset, valueSize };
    currentOffset += valueSize;
    if (buffer.size() >= bufferCapacity) {
        std::unique_lock<std::mutex> lock(queueMutex);
        taskQueue.push(buffer);
        bufferFullCond.notify_one();
        buffer.clear();
    }
}

template<typename V>
V KVStore<V>::get(int key) {
    if (buffer.size())
        flushBuffer();
    if (metadataMap.find(key) != metadataMap.end()) {
        Metadata meta = metadataMap[key];
        std::unique_lock<std::mutex> fileLock(fileMutex);
        dataFile.seekg(meta.offset, std::ios::beg);
        std::vector<char> temp(meta.size);
        dataFile.read(temp.data(), meta.size);
        fileLock.unlock();
        if constexpr (std::is_class<V>::value && !std::is_same_v<V, std::string>)
            return V::deserialize(temp);
        return Serializer::deserialize<V>(temp);
    }
    throw std::out_of_range("Key " + std::to_string(key) + " not found while getting");
}

template<typename V>
void KVStore<V>::del(int key) {
    auto it = metadataMap.find(key);
    if (it != metadataMap.end())
        metadataMap.erase(it);
    else
        throw std::out_of_range("Key " + std::to_string(key) + " not found while deleting");
}

template<typename V>
void KVStore<V>::flushBuffer() {
    std::unique_lock<std::mutex> dataFileLock(fileMutex);
    dataFile.seekp(0, std::ios::end);
    dataFile.write(buffer.data(), buffer.size());
}

template<typename V>
void KVStore<V>::writeDisk() {
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

template<typename V>
void KVStore<V>::saveMetadata() {
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

template<typename V>
void KVStore<V>::loadMetadata() {
    metaFile.seekg(0, std::ios::beg);
    size_t mapSize;
    if (!metaFile.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize))) {
        metaFile.clear();
        return;
    }
    for (size_t i = 0; i < mapSize; ++i) {
        int key;
        Metadata meta;
        if (!metaFile.read(reinterpret_cast<char*>(&key), sizeof(key)) ||
            !metaFile.read(reinterpret_cast<char*>(&meta.offset), sizeof(meta.offset)) ||
            !metaFile.read(reinterpret_cast<char*>(&meta.size), sizeof(meta.size))) {
            break;
        }
        metadataMap[key] = meta;
    }
}

template class KVStore<int>;
template class KVStore<double>;
template class KVStore<float>;
template class KVStore<std::string>;
template class KVStore<Person>;