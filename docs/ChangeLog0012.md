# Changelog – The Seed MMORPG Platform

## [Phase 1 W12+13] – Virtual Texturing / Texture Streaming

### Added
- **TextureStreamer** (`src/core/texture_streamer.hpp`, `src/core/texture_streamer.cpp`)
  - Asynchronous texture loading via dedicated worker thread
  - Priority queue for load ordering (higher priority = loaded first)
  - Memory budget management with configurable limits (default 512 MB)
  - LRU (Least Recently Used) cache eviction when budget exceeded
  - Force-load API for synchronous/blocking texture loading
  - Preload batch API for zone/level loading
  - Procedural texture fallback for missing files
  - Custom raw texture format (`.rawtex`) for fast loading
  - Streaming metrics: resident bytes, load times, eviction counts, pending requests
  - Global mip bias support for LOD quality control
  - Reference counting for automatic texture lifecycle management

- **TextureCache** (internal to TextureStreamer)
  - Thread-safe LRU eviction with `std::list` + `std::unordered_map`
  - `mutable` iterator in `CacheEntry` for const-correct LRU promotion
  - Budget enforcement with automatic eviction of least-recently-used textures
  - `Touch()` API to manually promote textures to MRU position

- **Tests** (`tests/test_texture_streamer.cpp`)
  - 15 test cases covering initialization, async loading, force loading
  - LRU eviction under memory pressure
  - 4K texture streaming performance (< 1s load time)
  - 20 concurrent texture requests
  - Priority ordering verification
  - Preload batch loading
  - Invalid path handling with procedural fallback
  - Metrics tracking accuracy
  - Mip bias settings
  - Release & re-request lifecycle
  - Texture format variants (R8, RG8, RGBA8)
  - TextureCache insert/get/eviction unit tests

### Fixed
- **test_material_system.cpp**: `MaterialLibrary::ReloadAll` test expectation corrected
  - Changed `CHECK(reloaded == 1)` → `CHECK(reloaded == 2)`
  - Both textures have valid config paths, so both get reloaded by `ReloadAll()`

### Changed
- **CMakeLists.txt**: Added `texture_streamer.cpp`, `texture_streamer.hpp`, and `test_texture_streamer.cpp` to build targets

### Technical Details
- **MSVC Compatibility**: Fixed multiple C++23 / MSVC-specific issues:
  - C2270: Removed `const` from non-member function declarations
  - C3861: Forward-declared `CalcTexSize` and `EvictInternal` before use
  - C2039: Added missing `EvictInternal` declaration to `TextureCache` class
  - C2065: Resolved member variable visibility issues
  - C2678: Removed `const_cast` hack; made `Get()` non-const with `mutable` iterator
  - C4100: Added `[[maybe_unused]]` for intentionally unused parameters
  - C2511: Fixed overloaded member function signature mismatch

- **Thread Safety**: All public APIs are thread-safe via `std::mutex`:
  - `m_registryMutex` protects texture registry
  - `m_queueMutex` + `m_queueCv` for async work queue
  - `m_completeMutex` for completed load notifications
  - `m_metricsMutex` for metrics aggregation
  - `m_mutex` inside `TextureCache` for cache operations

- **Memory Layout**:
  ```
  TextureStreamer
  ├── TextureCache (LRU + budget)
  │   ├── std::unordered_map<uint32_t, CacheEntry> m_entries
  │   └── std::list<uint32_t> m_lru
  ├── std::unordered_map<uint32_t, std::shared_ptr<Texture>> m_registry
  ├── std::priority_queue<std::pair<float, uint32_t>> m_requestQueue
  ├── std::vector<uint32_t> m_completed
  └── std::thread m_workerThread
  ```

### Known Limitations
- `maxMipCount` parameter in `RequestTextureLOD` and `LoadTextureFile` is accepted but not yet enforced (procedural fallback generates full mip chain)
- `deltaTime` in `Update()` is reserved for future frame-budget streaming
- GPU upload not yet implemented (CPU-side cache only)
- No support for compressed texture formats (BC1/BC3/BC5/BC7) yet
- No KTX2/DDS loader yet

### Next Steps (W14+)
- Interest Management (Spatial Hash) – no packets from distant zones
- Integrate TextureStreamer into MaterialLibrary (replace raw IDs with TextureHandle)
- GPU upload via Vulkan
- KTX2/DDS loader for compressed textures
- Virtual texturing with page tables for massive textures (terrain, etc.)
