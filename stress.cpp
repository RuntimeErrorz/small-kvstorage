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

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <duration_limit><total_insert_num><buffer_size>" << std::endl;
        return 1;
    }
    int numThreads = 4;
    int durationLimit = atoi(argv[1]);
    int numInsertsPerThread = atoi(argv[2]) / numThreads;
    int bufferSize = atoi(argv[3]);
    std::cout << "Duration Limit: " << durationLimit << " seconds" << std::endl;
    std::cout << "Number of threads: " << numThreads << ", inserting per thread: " << numInsertsPerThread << std::endl;
    std::cout << "Total Estimated Object Size: " << numInsertsPerThread * numThreads * 137 / 1024 / 1024 << "MB" << std::endl;
    std::cout << "Buffer Size: " << bufferSize / (1024 * 1024) << "MB" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    int i = 1;
    while (1) {
        auto roundStart = std::chrono::high_resolution_clock::now();

        KVStorePutDelGet<Person>(true, "struct.bin", "struct.meta", numThreads, numInsertsPerThread, bufferSize);        
        KVStoreGet<Person>(true, "struct.bin", "struct.meta");
        std::filesystem::remove("struct.bin");
        std::filesystem::remove("struct.meta");

        auto roundEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> roundDuration = roundEnd - roundStart;
        std::chrono::duration<double> duration = roundEnd - start;
        std::cout << "------------ Round" << i++ << ": " << roundDuration.count() << " seconds | Total: " << duration.count() << " seconds ------------" << std::endl;
        if (duration.count() > durationLimit)
            break;
    }
    return 0;
}
