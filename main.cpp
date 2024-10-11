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
    KVStore(const std::string& filename, size_t bufferSize)
        : bufferSize(bufferSize), currentBufferSize(0), stop(false) {
        dataFile.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        if (!dataFile.is_open()) {
            throw std::runtime_error("Failed to open file");
        }
        ioThread = std::thread(&KVStore::diskIOService, this);
    }

    ~KVStore() {
        flushBufferNow();
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            stop = true;
            bufferCond.notify_all();
        }
        ioThread.join();
        if (dataFile.is_open()) {
            dataFile.close();
        }
    }

    bool put(int key, const std::string& value) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        buffer.emplace_back(key, value);
        currentBufferSize += sizeof(key) + value.size();

        if (currentBufferSize >= bufferSize) {
            bufferCond.notify_one();
        }
        return true;
    }

    std::string get(int key) {
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

    bool del(int key) {
        std::lock_guard<std::mutex> lock(mapMutex);
        metadataMap.erase(key);
        return true;
    }
    void flushBufferNow() {
        std::vector<std::pair<int, std::string>> tempBuffer;
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            tempBuffer.swap(buffer);
            currentBufferSize = 0;
        }
        flushBuffer(tempBuffer);
    }
private:
    struct Metadata {
        size_t offset;
        size_t size;
    };

    std::fstream dataFile;
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

    void diskIOService() {
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

    void flushBuffer(const std::vector<std::pair<int, std::string>>& bufferToFlush) {
        std::lock_guard<std::mutex> fileLock(fileMutex);
        for (const auto& [key, value] : bufferToFlush) {
            size_t offset = dataFile.tellp();
            size_t valueSize = value.size();
            dataFile.write(reinterpret_cast<const char*>(&valueSize), sizeof(valueSize));
            dataFile.write(value.c_str(), valueSize);
            {
                std::lock_guard<std::mutex> mapLock(mapMutex);
                metadataMap[key] = { offset + sizeof(valueSize), valueSize };
            }
        }
        dataFile.flush();
    }


};

int main() {
    KVStore store("data.bin", 128 * 1024 * 1024); // 128MB buffer

    std::thread producer1([&store]() {
        for (int i = 0; i < 10; ++i) {
            store.put(i, "value" + std::to_string(i));
        }
        });

    std::thread producer2([&store]() {
        for (int i = 10; i < 20; ++i) {
            store.put(i, "value" + std::to_string(i));
        }
        });

    producer1.join();
    producer2.join();
    store.flushBufferNow();

    std::thread consumer([&store]() {
        for (int i = 0; i < 20; ++i) {
            std::string value = store.get(i);
            if (!value.empty()) {
                std::cout << "Get key " << i << ": " << value << std::endl;
            }
            else {
                std::cout << "Key " << i << " not found." << std::endl;
            }
        }
        });

    consumer.join();

    return 0;
}