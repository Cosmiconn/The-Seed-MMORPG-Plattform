#include "core/profiler.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>

namespace TheSeed::Profiler {

// --- FrameProfiler ---

void FrameProfiler::BeginFrame() {
    m_frameStart = std::chrono::high_resolution_clock::now();
    m_cpuStart = m_frameStart;
}

void FrameProfiler::EndFrame() {
    auto now = std::chrono::high_resolution_clock::now();
    float frameMs = std::chrono::duration<float, std::milli>(now - m_frameStart).count();
    float cpuMs = std::chrono::duration<float, std::milli>(now - m_cpuStart).count();

    size_t idx = m_writeIndex.fetch_add(1) % MAX_SAMPLES;
    m_samples[idx] = { frameMs, cpuMs,
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) };
}

float FrameProfiler::GetAverageFps() const {
    auto avg = GetAverageFrameTime();
    return avg > 0.0f ? 1000.0f / avg : 0.0f;
}

float FrameProfiler::GetAverageFrameTime() const {
    size_t count = std::min(m_writeIndex.load(), MAX_SAMPLES);
    if (count == 0) return 0.0f;

    float sum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum += m_samples[i].frameTimeMs;
    }
    return sum / static_cast<float>(count);
}

float FrameProfiler::GetMinFrameTime() const {
    size_t count = std::min(m_writeIndex.load(), MAX_SAMPLES);
    if (count == 0) return 0.0f;

    float minVal = m_samples[0].frameTimeMs;
    for (size_t i = 1; i < count; ++i) {
        minVal = std::min(minVal, m_samples[i].frameTimeMs);
    }
    return minVal;
}

float FrameProfiler::GetMaxFrameTime() const {
    size_t count = std::min(m_writeIndex.load(), MAX_SAMPLES);
    if (count == 0) return 0.0f;

    float maxVal = m_samples[0].frameTimeMs;
    for (size_t i = 1; i < count; ++i) {
        maxVal = std::max(maxVal, m_samples[i].frameTimeMs);
    }
    return maxVal;
}

float FrameProfiler::GetP99FrameTime() const {
    size_t count = std::min(m_writeIndex.load(), MAX_SAMPLES);
    if (count == 0) return 0.0f;

    std::vector<float> times;
    times.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        times.push_back(m_samples[i].frameTimeMs);
    }
    std::sort(times.begin(), times.end());
    size_t p99Idx = static_cast<size_t>(times.size() * 0.99f);
    return times[std::min(p99Idx, times.size() - 1)];
}

std::vector<FrameSample> FrameProfiler::GetHistory() const {
    size_t count = std::min(m_writeIndex.load(), MAX_SAMPLES);
    std::vector<FrameSample> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.push_back(m_samples[i]);
    }
    return result;
}

void FrameProfiler::Reset() {
    m_writeIndex.store(0);
}

// --- ScopedTimer ---

ScopedTimer::ScopedTimer(const char* name, FrameProfiler* profiler)
    : m_name(name), m_profiler(profiler), m_start(std::chrono::high_resolution_clock::now()) {}

ScopedTimer::~ScopedTimer() {
    auto elapsed = std::chrono::high_resolution_clock::now() - m_start;
    float ms = std::chrono::duration<float, std::milli>(elapsed).count();
    if (m_profiler) {
        // Optional: Record to profiler
    }
    spdlog::debug("[PROFILE] {}: {:.3f} ms", m_name, ms);
}

// --- MemoryTracker ---

MemoryTracker& MemoryTracker::Instance() {
    static MemoryTracker instance;
    return instance;
}

void MemoryTracker::RecordAlloc(size_t size, const char* tag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_global.totalAllocated += size;
    m_global.currentUsed += size;
    m_global.allocationCount++;
    m_global.peakUsed = std::max(m_global.peakUsed, m_global.currentUsed);

    auto& tagStats = m_perTag[tag];
    tagStats.totalAllocated += size;
    tagStats.currentUsed += size;
    tagStats.allocationCount++;
    tagStats.peakUsed = std::max(tagStats.peakUsed, tagStats.currentUsed);
}

void MemoryTracker::RecordFree(size_t size, const char* tag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_global.totalFreed += size;
    m_global.currentUsed -= std::min(size, m_global.currentUsed);
    m_global.freeCount++;

    auto& tagStats = m_perTag[tag];
    tagStats.totalFreed += size;
    tagStats.currentUsed -= std::min(size, tagStats.currentUsed);
    tagStats.freeCount++;
}

MemoryStats MemoryTracker::GetGlobalStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_global;
}

std::unordered_map<std::string, MemoryStats> MemoryTracker::GetPerTagStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_perTag;
}

void MemoryTracker::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_global = MemoryStats{};
    m_perTag.clear();
}

void MemoryTracker::DumpToLog() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    spdlog::info("=== Memory Report ===");
    spdlog::info("  Total Allocated: {} bytes", m_global.totalAllocated);
    spdlog::info("  Total Freed:     {} bytes", m_global.totalFreed);
    spdlog::info("  Current Used:    {} bytes", m_global.currentUsed);
    spdlog::info("  Peak Used:       {} bytes", m_global.peakUsed);
    spdlog::info("  Allocs/Frees:    {}/{}", m_global.allocationCount, m_global.freeCount);

    for (const auto& [tag, stats] : m_perTag) {
        if (stats.currentUsed > 0) {
            spdlog::info("  [{}] Current: {} bytes, Peak: {} bytes",
                tag, stats.currentUsed, stats.peakUsed);
        }
    }
}

// --- ProfilerSystem ---

void ProfilerSystem::Initialize() {
    m_initialized = true;
    MemoryTracker::Instance().Reset();
    spdlog::info("ProfilerSystem initialisiert");
}

void ProfilerSystem::Shutdown() {
    if (m_initialized) {
        DumpReport();
    }
    m_initialized = false;
}

void ProfilerSystem::BeginFrame() {
    m_frameProfiler.BeginFrame();
}

void ProfilerSystem::EndFrame() {
    m_frameProfiler.EndFrame();
}

void ProfilerSystem::DumpReport() const {
    spdlog::info("=== Performance Report ===");
    spdlog::info("  Avg FPS:         {:.1f}", m_frameProfiler.GetAverageFps());
    spdlog::info("  Avg Frame Time:  {:.3f} ms", m_frameProfiler.GetAverageFrameTime());
    spdlog::info("  Min Frame Time:  {:.3f} ms", m_frameProfiler.GetMinFrameTime());
    spdlog::info("  Max Frame Time:  {:.3f} ms", m_frameProfiler.GetMaxFrameTime());
    spdlog::info("  P99 Frame Time:  {:.3f} ms", m_frameProfiler.GetP99FrameTime());
    MemoryTracker::Instance().DumpToLog();
}

} // namespace TheSeed::Profiler
