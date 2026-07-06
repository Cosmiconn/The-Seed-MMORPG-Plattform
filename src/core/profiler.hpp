#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <array>

namespace TheSeed::Profiler {

// --- Frame-Time Profiler ---
struct FrameSample {
    float frameTimeMs;
    float cpuTimeMs;
    uint64_t timestamp;
};

class FrameProfiler {
public:
    static constexpr size_t MAX_SAMPLES = 256;

    void BeginFrame();
    void EndFrame();

    float GetAverageFps() const;
    float GetAverageFrameTime() const;
    float GetMinFrameTime() const;
    float GetMaxFrameTime() const;
    float GetP99FrameTime() const;

    std::vector<FrameSample> GetHistory() const;
    void Reset();

private:
    std::array<FrameSample, MAX_SAMPLES> m_samples{};
    std::atomic<size_t> m_writeIndex{0};
    std::chrono::high_resolution_clock::time_point m_frameStart;
    std::chrono::high_resolution_clock::time_point m_cpuStart;
};

// --- Scoped Timer ---
class ScopedTimer {
public:
    explicit ScopedTimer(const char* name, FrameProfiler* profiler = nullptr);
    ~ScopedTimer();

private:
    const char* m_name;
    FrameProfiler* m_profiler;
    std::chrono::high_resolution_clock::time_point m_start;
};

// --- Memory Tracker ---
struct MemoryStats {
    size_t totalAllocated = 0;
    size_t totalFreed = 0;
    size_t currentUsed = 0;
    size_t peakUsed = 0;
    size_t allocationCount = 0;
    size_t freeCount = 0;
};

class MemoryTracker {
public:
    static MemoryTracker& Instance();

    void RecordAlloc(size_t size, const char* tag = "unknown");
    void RecordFree(size_t size, const char* tag = "unknown");

    MemoryStats GetGlobalStats() const;
    std::unordered_map<std::string, MemoryStats> GetPerTagStats() const;

    void Reset();
    void DumpToLog() const;

private:
    MemoryTracker() = default;
    mutable std::mutex m_mutex;
    MemoryStats m_global;
    std::unordered_map<std::string, MemoryStats> m_perTag;
};

// --- System Profiler (aggregiert alles) ---
class ProfilerSystem {
public:
    void Initialize();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    FrameProfiler& GetFrameProfiler() { return m_frameProfiler; }
    MemoryTracker& GetMemoryTracker() { return MemoryTracker::Instance(); }

    void DumpReport() const;

private:
    FrameProfiler m_frameProfiler;
    bool m_initialized = false;
};

// Convenience Makros
#define TS_PROFILE_FRAME(profiler) profiler.BeginFrame(); auto _ts_frameGuard = ::TheSeed::Profiler::FrameGuard(profiler)
#define TS_PROFILE_SCOPE(name) ::TheSeed::Profiler::ScopedTimer _ts_timer_##__LINE__(name)
#define TS_TRACK_ALLOC(size, tag) ::TheSeed::Profiler::MemoryTracker::Instance().RecordAlloc(size, tag)
#define TS_TRACK_FREE(size, tag) ::TheSeed::Profiler::MemoryTracker::Instance().RecordFree(size, tag)

// RAII Frame Guard
class FrameGuard {
public:
    explicit FrameGuard(FrameProfiler& p) : m_profiler(p) {}
    ~FrameGuard() { m_profiler.EndFrame(); }
private:
    FrameProfiler& m_profiler;
};

} // namespace TheSeed::Profiler
