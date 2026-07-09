#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <filesystem>
#include <expected>
#include <list>
#include <unordered_set>

namespace TheSeed::Render {

// ============================================================================
// Virtual Texturing / Texture Streaming – Phase 1 W12+13
// Deliverable: 4K-Texturen ohne Stutter durch asynchrones Streaming + LRU
// ============================================================================

enum class TextureFormat : uint8_t {
    RGBA8_UNORM,
    RGB8_UNORM,
    RG8_UNORM,
    R8_UNORM,
    RGBA16_FLOAT,
    BC1,  // DXT1
    BC3,  // DXT5
    BC5,  // 3Dc/ATI2
    BC7,
    Count
};

enum class TextureType : uint8_t {
    Albedo,
    Normal,
    Metallic,
    Roughness,
    AO,
    Emissive,
    Count
};

// Mipmap level descriptor
struct MipLevel {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    size_t   dataSize = 0;
    std::vector<uint8_t> data;  // CPU-side pixel data
};

// CPU-side texture representation (GPU upload prepared)
struct Texture {
    uint32_t id = 0;
    std::string name;
    std::filesystem::path sourcePath;
    TextureFormat format = TextureFormat::RGBA8_UNORM;
    TextureType type = TextureType::Albedo;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipCount = 0;
    bool isStreaming = false;     // true while async load in progress
    bool resident = false;        // true when fully loaded to GPU
    float priority = 1.0f;        // streaming priority (0..1)
    std::vector<MipLevel> mips;   // mip 0 = full resolution
    std::atomic<uint32_t> refCount{0};
};

// Handle returned to users (material system, renderer)
struct TextureHandle {
    uint32_t id = 0;
    uint32_t mipBias = 0;  // forced lower mip for distant objects
    bool operator==(const TextureHandle& o) const { return id == o.id && mipBias == o.mipBias; }
    bool operator!=(const TextureHandle& o) const { return !(*this == o); }
};

// Metrics for profiling / debugging
struct StreamingMetrics {
    uint64_t totalTextures = 0;
    uint64_t residentTextures = 0;
    uint64_t streamingTextures = 0;
    uint64_t evictedTextures = 0;
    uint64_t totalBytes = 0;
    uint64_t residentBytes = 0;
    uint64_t budgetBytes = 0;
    uint64_t peakBytes = 0;
    uint64_t requestsPending = 0;
    double   avgLoadTimeMs = 0.0;
    uint64_t loadCount = 0;
};

// Async load request
struct LoadRequest {
    std::filesystem::path path;
    TextureFormat format = TextureFormat::RGBA8_UNORM;
    TextureType type = TextureType::Albedo;
    float priority = 1.0f;
    uint32_t targetMipCount = 0;  // 0 = all mips
};

// Forward declarations
class TextureStreamer;

// ============================================================================
// Texture Cache: LRU eviction with memory budget
// ============================================================================
class TextureCache {
public:
    explicit TextureCache(size_t budgetBytes);
    ~TextureCache() = default;

    // Insert a fully loaded texture into cache (takes ownership of mips data)
    bool Insert(std::shared_ptr<Texture> tex);

    // Mark texture as recently used (promote in LRU)
    void Touch(uint32_t textureId);

    // Evict textures until we have 'requiredBytes' free
    // Returns number of textures evicted
    size_t Evict(size_t requiredBytes);

    // Get texture if resident
    std::shared_ptr<Texture> Get(uint32_t textureId);

    // Remove specific texture
    bool Remove(uint32_t textureId);

    // Clear everything
    void Clear();

    size_t GetUsedBytes() const { return m_usedBytes; }
    size_t GetBudgetBytes() const { return m_budgetBytes; }
    bool HasBudget(size_t bytes) const { return m_usedBytes + bytes <= m_budgetBytes; }

    size_t GetResidentCount() const;

private:
    struct CacheEntry {
        std::shared_ptr<Texture> texture;
        mutable std::list<uint32_t>::iterator lruIter;
    };

    mutable std::mutex m_mutex;
    size_t m_budgetBytes;
    size_t m_usedBytes = 0;
    std::unordered_map<uint32_t, CacheEntry> m_entries;
    std::list<uint32_t> m_lru;  // front = most recently used

    size_t EvictInternal(size_t requiredBytes);
};

// ============================================================================
// Texture Streamer: Async loading + virtual texturing
// ============================================================================
class TextureStreamer {
public:
    TextureStreamer();
    ~TextureStreamer();

    // Initialize with memory budget (default 512MB)
    bool Initialize(size_t memoryBudgetBytes = 512ULL * 1024 * 1024);
    void Shutdown();

    // Set asset root directory for relative paths
    void SetAssetRoot(const std::filesystem::path& root);

    // Request texture load (async). Returns handle immediately.
    // The texture will be streamed in background.
    TextureHandle RequestTexture(const std::filesystem::path& path,
                                 TextureFormat format = TextureFormat::RGBA8_UNORM,
                                 TextureType type = TextureType::Albedo,
                                 float priority = 1.0f);

    // Request texture with specific max mip count (for LOD)
    TextureHandle RequestTextureLOD(const std::filesystem::path& path,
                                    uint32_t maxMipCount,
                                    TextureFormat format = TextureFormat::RGBA8_UNORM,
                                    TextureType type = TextureType::Albedo,
                                    float priority = 1.0f);

    // Check if texture is resident (ready for rendering)
    bool IsResident(TextureHandle handle) const;

    // Get texture info (nullptr if not resident yet)
    std::shared_ptr<Texture> GetTexture(TextureHandle handle) const;

    // Update streaming priority (e.g. based on distance)
    void SetPriority(TextureHandle handle, float priority);

    // Force immediate load (blocking) – for critical textures
    bool ForceLoad(TextureHandle handle);

    // Update streaming (call once per frame)
    // Processes completed async loads, evictions, etc.
    void Update([[maybe_unused]] float deltaTime);

    // Preload a set of textures (e.g. for upcoming zone)
    void PreloadTextures(const std::vector<std::filesystem::path>& paths);

    // Release texture reference
    void Release(TextureHandle handle);

    // Get metrics
    StreamingMetrics GetMetrics() const;

    // Memory budget management
    void SetMemoryBudget(size_t bytes);
    size_t GetMemoryBudget() const;

    // Mip bias based on distance / screen coverage
    void SetGlobalMipBias(float bias);  // positive = lower quality
    float GetGlobalMipBias() const;

    // Debug: dump cache state
    void DumpState() const;

private:
    std::atomic<bool> m_running{false};
    std::atomic<uint32_t> m_nextId{1};

    std::filesystem::path m_assetRoot;

    // Cache
    std::unique_ptr<TextureCache> m_cache;

    // Texture registry (all known textures)
    mutable std::mutex m_registryMutex;
    std::unordered_map<uint32_t, std::shared_ptr<Texture>> m_registry;

    // Async work queue
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::priority_queue<std::pair<float, uint32_t>> m_requestQueue;  // priority, id
    std::unordered_set<uint32_t> m_inQueue;

    // Completed loads (main thread picks these up)
    std::mutex m_completeMutex;
    std::vector<uint32_t> m_completed;

    // Worker thread
    std::thread m_workerThread;

    // Metrics
    mutable std::mutex m_metricsMutex;
    StreamingMetrics m_metrics{};

    // Settings
    std::atomic<float> m_globalMipBias{0.0f};

    // Internal
    void WorkerLoop();
    std::filesystem::path ResolvePath(const std::filesystem::path& path) const;
    bool LoadTextureFile(const std::filesystem::path& path, Texture& outTex, 
                         [[maybe_unused]] uint32_t maxMipCount);
    void UpdateMetrics();
};

} // namespace TheSeed::Render
