#include "core/jobs.hpp"
#include <spdlog/spdlog.h>
#include <chrono>

namespace TheSeed::Jobs {

void JobSystem::WorkQueue::Push(Job job) {
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push_back(std::move(job));
}

bool JobSystem::WorkQueue::Pop(Job& job) {
    std::lock_guard<std::mutex> lock(mtx);
    if (tasks.empty()) return false;
    job = std::move(tasks.back());
    tasks.pop_back();
    return true;
}

bool JobSystem::WorkQueue::Steal(Job& job) {
    std::lock_guard<std::mutex> lock(mtx);
    if (tasks.empty()) return false;
    job = std::move(tasks.front());
    tasks.pop_front();
    return true;
}

JobSystem::JobSystem() : m_rng(m_rd()) {}

JobSystem::~JobSystem() {
    Shutdown();
}

void JobSystem::Initialize(uint32_t numWorkers) {
    if (m_running) return;

    if (numWorkers == 0) {
        numWorkers = std::max(1u, std::thread::hardware_concurrency());
    }

    m_running = true;
    m_pendingTasks.store(0);

    for (uint32_t i = 0; i < numWorkers; ++i) {
        auto worker = std::make_unique<Worker>();
        worker->thread = std::thread(&JobSystem::WorkerLoop, this, i);
        m_workers.push_back(std::move(worker));
    }

    spdlog::info("JobSystem gestartet mit {} Workern", numWorkers);
}

void JobSystem::Shutdown() {
    if (!m_running) return;

    m_running = false;
    m_waitCv.notify_all();

    for (auto& w : m_workers) {
        if (w->thread.joinable()) {
            w->thread.join();
        }
    }

    m_workers.clear();
    spdlog::info("JobSystem heruntergefahren");
}

void JobSystem::Submit(Job job) {
    if (!m_running) {
        job(); // Fallback: synchron ausführen
        return;
    }

    m_pendingTasks.fetch_add(1);

    // Round-robin Verteilung
    static std::atomic<uint32_t> nextWorker{0};
    uint32_t idx = nextWorker.fetch_add(1) % m_workers.size();

    m_workers[idx]->queue.Push([this, job = std::move(job)]() mutable {
        job();
        if (m_pendingTasks.fetch_sub(1) == 1) {
            std::lock_guard<std::mutex> lock(m_waitMutex);
            m_waitCv.notify_all();
        }
    });
}

void JobSystem::SubmitAndWait(Job job) {
    Submit(std::move(job));
    WaitForAll();
}

void JobSystem::WaitForAll() {
    std::unique_lock<std::mutex> lock(m_waitMutex);
    m_waitCv.wait(lock, [this] {
        return m_pendingTasks.load() == 0 || !m_running;
    });
}

void JobSystem::WorkerLoop(uint32_t workerId) {
    while (m_running) {
        Job job;
        if (TryGetJob(workerId, job)) {
            job();
        } else {
            std::this_thread::yield();
        }
    }
}

bool JobSystem::TryGetJob(uint32_t workerId, Job& job) {
    // 1. Eigene Queue (LIFO → Cache-freundlich)
    if (m_workers[workerId]->queue.Pop(job)) {
        return true;
    }

    // 2. Steal von zufälligem anderen Worker (FIFO → Fairness)
    if (m_workers.size() > 1) {
        std::uniform_int_distribution<uint32_t> dist(0, static_cast<uint32_t>(m_workers.size() - 1));
        for (int attempt = 0; attempt < 2; ++attempt) {
            uint32_t victim = dist(m_rng);
            if (victim != workerId && m_workers[victim]->queue.Steal(job)) {
                return true;
            }
        }
    }

    return false;
}

} // namespace TheSeed::Jobs
