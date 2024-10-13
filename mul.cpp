#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <string>

std::unordered_map<int, std::string> cache; // Key-Value缓存
std::queue<std::unordered_map<int, std::string>> taskQueue; // 写任务队列
std::mutex cacheMutex, queueMutex;
std::condition_variable cv;
bool stop = false; // 用于停止消费者线程的标志

// 生产者：写入缓存
void producer(int id, int startKey, int numKeys, size_t maxCacheSize) {
    for (int i = startKey; i < startKey + numKeys; ++i) {
        std::unique_lock<std::mutex> lock(cacheMutex);
        cache[i] = "value_" + std::to_string(i) + "_from_producer_" + std::to_string(id);

        // 如果缓存满了，生成写入任务
        if (cache.size() >= maxCacheSize) {
            std::unique_lock<std::mutex> qLock(queueMutex);
            taskQueue.push(cache); // 将当前缓存放入任务队列
            cv.notify_one(); // 通知消费者线程
            cache.clear(); // 清空缓存
        }
    }
}

// 消费者：从任务队列中读取任务并模拟写入磁盘
void consumer(int id) {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [] { return !taskQueue.empty() || stop; });

        if (stop && taskQueue.empty()) {
            break; // 如果停止标志为true且任务队列为空，退出线程
        }

        // 从任务队列中取任务
        while (!taskQueue.empty()) {
            auto task = taskQueue.front();
            taskQueue.pop();
            lock.unlock(); // 释放锁，让其他线程继续处理任务队列

            // 模拟写入磁盘
            for (const auto& kv : task) {
                std::cout << "Consumer " << id << " writing key: " << kv.first << ", value: " << kv.second << std::endl;
            }

            lock.lock(); // 重新获得锁，检查队列
        }
    }
}

int main() {
    size_t maxCacheSize = 12; // 缓存最大128条记录

    // 创建多个消费者线程
    std::vector<std::thread> consumers;
    int numConsumers = 3; // 3个消费者
    for (int i = 0; i < numConsumers; ++i) {
        consumers.emplace_back(consumer, i);
    }

    // 创建多个生产者线程
    std::vector<std::thread> producers;
    int numProducers = 5; // 5个生产者
    int keysPerProducer = 3; // 每个生产者插入300个键值对
    for (int i = 0; i < numProducers; ++i) {
        producers.emplace_back(producer, i, i * keysPerProducer, keysPerProducer, maxCacheSize);
    }

    // 等待所有生产者线程完成
    for (auto& p : producers) {
        p.join();
    }

    // 通知消费者线程停止
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    cv.notify_all(); // 唤醒所有消费者线程

    // 等待所有消费者线程完成
    for (auto& c : consumers) {
        c.join();
    }

    return 0;
}
