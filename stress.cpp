#include "kvstore.h"
#include "test_utils.h"
#include "struct.h"
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <mutex>
#include <filesystem>
#include <chrono>

int main() {
    int numThreads = std::thread::hardware_concurrency();
    int numInsertsPerThread = 10.24 * 1000 * 1000 / 8;
    int bufferSize = 128 * 1024 * 1024;
    std::cout << "Number of threads: " << numThreads << ", inserting per thread: " << numInsertsPerThread << std::endl;
    std::cout << "Buffer Size: " << bufferSize / (1024 * 1024) << "MB" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    int i = 1;
    while (1) {
        auto roundStart = std::chrono::high_resolution_clock::now();
        KVStorePutGetDel<Person>(true, "struct.bin", "struct.meta", numThreads, numInsertsPerThread, bufferSize);
        auto roundEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> roundDuration = roundEnd - roundStart;
        std::chrono::duration<double> duration = roundEnd - start;
        std::cout << "------------ Round" << i++ << ": " << roundDuration.count() << " seconds | Total: " << duration.count() << " seconds ------------" << std::endl;
        std::filesystem::remove("struct.bin");
        std::filesystem::remove("struct.meta");
        if (duration.count() > 12 * 60 * 60)
            break;
    }
    // Person: int 4 bytes, double 8 bytes, string size = 52 bytes;
    // 10.24M Person: 625 MB
    return 0;
}
