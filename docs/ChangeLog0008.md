# ChangeLog 0008 – Woche 8: Profiler + Memory-Tracker

**Datum:** 2026-07-06  
**Version:** 0.2.0

## Hinzugefuegt
- FrameProfiler: Sliding-Window (256 Samples)
  - Avg FPS, Avg/Min/Max/P99 Frame Time
  - CPU-Time Tracking
- ScopedTimer: RAII-Profiling mit spdlog-Output
- MemoryTracker: Singleton, Thread-safe
  - Global + Per-Tag Statistiken
  - Alloc/Free/Pending/Peak Tracking
- ProfilerSystem: Aggregiert Frame + Memory

## Makros
```cpp
TS_PROFILE_FRAME(profiler)   // Frame-Begin/End
TS_PROFILE_SCOPE("name")     // Scoped Timer
TS_TRACK_ALLOC(size, "tag")  // Allocation tracking
TS_TRACK_FREE(size, "tag")   // Free tracking
```

## Tests
- FrameProfiler Basic
- Multiple Frames
- History
- MemoryTracker Singleton
- Alloc/Free Korrektheit
- Per-Tag Tracking
- ScopedTimer
- ProfilerSystem Lifecycle

## Dateien
- `src/core/profiler.hpp`
- `src/core/profiler.cpp`
- `tests/test_profiler.cpp`
