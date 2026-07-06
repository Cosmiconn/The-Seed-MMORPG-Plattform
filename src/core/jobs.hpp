#pragma once
#include <cstdint>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <random>
#include <memory>

namespace TheSeed::Jobs {

class JobSystem {
public:
    using Job = std::function<void()>;

    JobSystem();
    ~JobSystem();

    void Initialize(uint32_t numWorkers = 0);
    void Shutdown();

    void Submit(Job job);
    void SubmitAndWait(Job job);
    void WaitForAll();

    uint32_t WorkerCount() const { return static_cast<uint32_t>(m_workers.size()); }
    bool IsRunning() const { return m_running; }

private:
    struct WorkQueue {
        std::deque<Job> tasks;
        std::mutex mtx;

        void Push(Job job);
        bool Pop(Job& job);    // Worker nimmt von hinten (LIFO)
        bool Steal(Job& job);  // Anderer Worker stiehlt von vorne (FIFO)
    };

    struct Worker {
        WorkQueue queue;
        std::thread thread;
    };

    std::vector<std::unique_ptr<Worker>> m_workers;
    std::atomic<bool> m_running{false};
    std::atomic<uint32_t> m_pendingTasks{0};
    std::mutex m_waitMutex;
    std::condition_variable m_waitCv;

    std::random_device m_rd;
    std::mt19937 m_rng;

    void WorkerLoop(uint32_t workerId);
    bool TryGetJob(uint32_t workerId, Job& job);
};

} // namespace TheSeed::Jobs
