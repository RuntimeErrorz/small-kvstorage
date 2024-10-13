#include "KVStore.h"
#include <iostream>
#include <optional>

int main() {
    KVStore<int> store("data.bin", "data.meta", 128 * 1024 * 1024); // 128MB buffer
    // std::thread producer1([&store]() {
    //     for (int i = 0; i < 10; ++i) {
    //         store.put(i, i);
    //     }
    //     });
    // std::thread producer2([&store]() {
    //     for (int i = 10; i < 20; ++i) {
    //         store.put(i, i);
    //     }
    //     });
    // producer1.join();
    // producer2.join();
    store.put(1, 1);
    store.flushBuffer();

    for (int i = 0; i < 20; ++i) {
        std::cout << store.get(i);
    }
    return 0;
}