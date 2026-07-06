#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/profiler.hpp"
#include <thread>

using namespace TheSeed::Profiler;

TEST_CASE("Profiler: FrameProfiler Basic", "[profiler]") {
    FrameProfiler fp;
    fp.BeginFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    fp.EndFrame();

    REQUIRE(fp.GetAverageFrameTime() > 0.0f);
    REQUIRE(fp.GetAverageFps() > 0.0f);
    REQUIRE(fp.GetMinFrameTime() > 0.0f);
    REQUIRE(fp.GetMaxFrameTime() > 0.0f);
}

TEST_CASE("Profiler: FrameProfiler Multiple Frames", "[profiler]") {
    FrameProfiler fp;
    for (int i = 0; i < 10; ++i) {
        fp.BeginFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        fp.EndFrame();
    }

    REQUIRE(fp.GetAverageFrameTime() >= 5.0f);
    REQUIRE(fp.GetMinFrameTime() >= 5.0f);
    REQUIRE(fp.GetMaxFrameTime() >= 5.0f);
    REQUIRE(fp.GetP99FrameTime() >= 5.0f);
}

TEST_CASE("Profiler: FrameProfiler History", "[profiler]") {
    FrameProfiler fp;
    for (int i = 0; i < 5; ++i) {
        fp.BeginFrame();
        fp.EndFrame();
    }

    auto history = fp.GetHistory();
    REQUIRE(history.size() == 5);
}

TEST_CASE("Profiler: MemoryTracker Singleton", "[profiler]") {
    auto& tracker1 = MemoryTracker::Instance();
    auto& tracker2 = MemoryTracker::Instance();
    REQUIRE(&tracker1 == &tracker2);
}

TEST_CASE("Profiler: MemoryTracker Alloc/Free", "[profiler]") {
    MemoryTracker::Instance().Reset();

    MemoryTracker::Instance().RecordAlloc(1024, "test");
    auto stats = MemoryTracker::Instance().GetGlobalStats();
    REQUIRE(stats.currentUsed == 1024);
    REQUIRE(stats.totalAllocated == 1024);
    REQUIRE(stats.allocationCount == 1);

    MemoryTracker::Instance().RecordFree(512, "test");
    stats = MemoryTracker::Instance().GetGlobalStats();
    REQUIRE(stats.currentUsed == 512);
    REQUIRE(stats.totalFreed == 512);
    REQUIRE(stats.freeCount == 1);

    MemoryTracker::Instance().RecordFree(512, "test");
    stats = MemoryTracker::Instance().GetGlobalStats();
    REQUIRE(stats.currentUsed == 0);

    MemoryTracker::Instance().Reset();
}

TEST_CASE("Profiler: MemoryTracker Per-Tag", "[profiler]") {
    MemoryTracker::Instance().Reset();

    MemoryTracker::Instance().RecordAlloc(100, "ecs");
    MemoryTracker::Instance().RecordAlloc(200, "net");
    MemoryTracker::Instance().RecordAlloc(50, "ecs");

    auto perTag = MemoryTracker::Instance().GetPerTagStats();
    REQUIRE(perTag.count("ecs") == 1);
    REQUIRE(perTag.count("net") == 1);
    REQUIRE(perTag["ecs"].currentUsed == 150);
    REQUIRE(perTag["net"].currentUsed == 200);

    MemoryTracker::Instance().Reset();
}

TEST_CASE("Profiler: ScopedTimer", "[profiler]") {
    {
        ScopedTimer timer("test_scope");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    // Timer logged to spdlog::debug, no crash = success
    REQUIRE(true);
}

TEST_CASE("Profiler: ProfilerSystem Lifecycle", "[profiler]") {
    ProfilerSystem sys;
    sys.Initialize();

    for (int i = 0; i < 5; ++i) {
        sys.BeginFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        sys.EndFrame();
    }

    REQUIRE(sys.GetFrameProfiler().GetAverageFrameTime() > 0.0f);
    sys.Shutdown();
}
