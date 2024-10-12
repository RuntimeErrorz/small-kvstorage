#include "KVStore.h"
#include <iostream>

int main() {
    KVStore store("data.bin", "data.meta", 128 * 1024 * 1024); // 128MB buffer

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

    for (int i = 0; i < 20; ++i) {
        std::string value = store.get(i);
        if (!value.empty()) {
            std::cout << "Get key " << i << ": " << value << std::endl;
        }
        else {
            std::cout << "Key " << i << " not found." << std::endl;
        }
    }
    return 0;
}