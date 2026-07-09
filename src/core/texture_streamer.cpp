#include "core/texture_streamer.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cstring>
#include <cmath>

namespace TheSeed::Render {

// ============================================================================
// Helper: Calculate texture size from mips
// ============================================================================
static size_t CalcTexSize(const Texture& tex) {
    size_t s = 0;
    for (const auto& mip : tex.mips) s += mip.dataSize;
    return s;
}

// ============================================================================
// TextureCache – LRU eviction
// ============================================================================
TextureCache::TextureCache(size_t budgetBytes) : m_budgetBytes(budgetBytes) {
    spdlog::info("TextureCache: initialized with {} MB budget", budgetBytes / (1024.0 * 1024.0));
}

bool TextureCache::Insert(std::shared_ptr<Texture> tex) {
    if (!tex || tex->mips.empty()) return false;

    size_t texSize = CalcTexSize(*tex);

    std::lock_guard<std::mutex> lock(m_mutex);

    // FIX: Evict LRU textures until we have enough budget.
    // If texSize > budget, evict everything (single-item override).
    while (m_usedBytes + texSize > m_budgetBytes && !m_lru.empty()) {
        uint32_t id = m_lru.back();
        m_lru.pop_back();
        auto it = m_entries.find(id);
        if (it != m_entries.end()) {
            size_t freed = CalcTexSize(*it->second.texture);
            it->second.texture->resident = false;
            it->second.texture->mips.clear();
            it->second.texture->mips.shrink_to_fit();
            m_usedBytes -= freed;
            m_entries.erase(it);
            spdlog::debug("TextureCache: evicted ID={} (freed {} bytes)", id, freed);
        }
    }

    // Remove old entry if exists (refresh / update)
    auto it = m_entries.find(tex->id);
    if (it != m_entries.end()) {
        m_lru.erase(it->second.lruIter);
        m_usedBytes -= CalcTexSize(*it->second.texture);
        m_entries.erase(it);
    }

    // Insert at front (MRU)
    m_lru.push_front(tex->id);
    CacheEntry entry{tex, m_lru.begin()};
    m_entries[tex->id] = std::move(entry);
    m_usedBytes += texSize;

    tex->resident = true;
    spdlog::debug("TextureCache: inserted '{}' ({}x{}, {} mips, {} bytes, total {} bytes)",
                  tex->name, tex->width, tex->height, tex->mipCount,
                  texSize, m_usedBytes);
    return true;
}

void TextureCache::Touch(uint32_t textureId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entries.find(textureId);
    if (it == m_entries.end()) return;
    m_lru.erase(it->second.lruIter);
    m_lru.push_front(textureId);
    it->second.lruIter = m_lru.begin();
}

size_t TextureCache::Evict(size_t requiredBytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return EvictInternal(requiredBytes);
}

size_t TextureCache::EvictInternal(size_t requiredBytes) {
    size_t evicted = 0;
    while (m_usedBytes + requiredBytes > m_budgetBytes && !m_lru.empty()) {
        uint32_t id = m_lru.back();
        m_lru.pop_back();
        auto it = m_entries.find(id);
        if (it != m_entries.end()) {
            size_t freed = CalcTexSize(*it->second.texture);
            it->second.texture->resident = false;
            it->second.texture->mips.clear();  // Free CPU pixel data
            it->second.texture->mips.shrink_to_fit();
            m_usedBytes -= freed;
            m_entries.erase(it);
            evicted++;
            spdlog::debug("TextureCache: evicted ID={} (freed {} MB)", id, freed / (1024.0*1024.0));
        }
    }
    return evicted;
}

std::shared_ptr<Texture> TextureCache::Get(uint32_t textureId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entries.find(textureId);
    if (it != m_entries.end()) {
        // Promote to MRU on access
        m_lru.erase(it->second.lruIter);
        m_lru.push_front(textureId);
        it->second.lruIter = m_lru.begin();
        return it->second.texture;
    }
    return nullptr;
}

bool TextureCache::Remove(uint32_t textureId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entries.find(textureId);
    if (it == m_entries.end()) return false;
    m_usedBytes -= CalcTexSize(*it->second.texture);
    m_lru.erase(it->second.lruIter);
    m_entries.erase(it);
    return true;
}

void TextureCache::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, entry] : m_entries) {
        entry.texture->resident = false;
        entry.texture->mips.clear();
    }
    m_entries.clear();
    m_lru.clear();
    m_usedBytes = 0;
}

size_t TextureCache::GetResidentCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries.size();
}

// ============================================================================
// TextureStreamer – Async loading + virtual texturing
// ============================================================================
TextureStreamer::TextureStreamer() = default;

TextureStreamer::~TextureStreamer() {
    Shutdown();
}

bool TextureStreamer::Initialize(size_t memoryBudgetBytes) {
    if (m_running) return true;
    m_cache = std::make_unique<TextureCache>(memoryBudgetBytes);
    m_running = true;
    m_workerThread = std::thread(&TextureStreamer::WorkerLoop, this);
    spdlog::info("TextureStreamer: initialized with {} MB budget", memoryBudgetBytes / (1024.0*1024.0));

    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.budgetBytes = memoryBudgetBytes;
    }
    return true;
}

void TextureStreamer::Shutdown() {
    if (!m_running) return;
    m_running = false;
    m_queueCv.notify_all();
    if (m_workerThread.joinable()) m_workerThread.join();

    if (m_cache) m_cache->Clear();
    m_registry.clear();
    spdlog::info("TextureStreamer: shutdown complete");
}

void TextureStreamer::SetAssetRoot(const std::filesystem::path& root) {
    m_assetRoot = root;
}

std::filesystem::path TextureStreamer::ResolvePath(const std::filesystem::path& path) const {
    if (path.empty()) return {};
    if (path.is_absolute()) return path;
    if (!m_assetRoot.empty()) {
        auto resolved = m_assetRoot / path;
        if (std::filesystem::exists(resolved)) return resolved;
    }
    return path;
}

TextureHandle TextureStreamer::RequestTexture(const std::filesystem::path& path,
                                               TextureFormat format,
                                               TextureType type,
                                               float priority) {
    return RequestTextureLOD(path, 0, format, type, priority);
}

TextureHandle TextureStreamer::RequestTextureLOD(const std::filesystem::path& path,
                                                  uint32_t maxMipCount,
                                                  TextureFormat format,
                                                  TextureType type,
                                                  float priority) {
    (void)maxMipCount;  // reserved for future LOD implementation
    auto resolved = ResolvePath(path);
    if (resolved.empty()) {
        spdlog::warn("TextureStreamer: empty path requested");
        return TextureHandle{};
    }

    uint32_t id = m_nextId.fetch_add(1);
    auto tex = std::make_shared<Texture>();
    tex->id = id;
    tex->name = resolved.stem().string();
    tex->sourcePath = resolved;
    tex->format = format;
    tex->type = type;
    tex->priority = priority;
    tex->isStreaming = true;
    // refCount removed - using shared_ptr reference counting

    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        m_registry[id] = tex;
    }

    // Queue for async loading
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_requestQueue.push({priority, id});
        m_inQueue.insert(id);
        m_queueCv.notify_one();
    }

    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.totalTextures++;
        m_metrics.requestsPending = m_inQueue.size();
    }

    spdlog::debug("TextureStreamer: requested '{}' (ID={}, priority={})", resolved.string(), id, priority);
    return TextureHandle{id, 0};
}

bool TextureStreamer::IsResident(TextureHandle handle) const {
    if (handle.id == 0) return false;
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_registry.find(handle.id);
    if (it == m_registry.end()) return false;
    return it->second->resident;
}

std::shared_ptr<Texture> TextureStreamer::GetTexture(TextureHandle handle) const {
    if (handle.id == 0) return nullptr;

    // First check cache (fast path)
    auto cached = m_cache->Get(handle.id);
    if (cached) return cached;

    // Fallback to registry
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_registry.find(handle.id);
    if (it == m_registry.end()) return nullptr;
    return it->second;
}

void TextureStreamer::SetPriority(TextureHandle handle, float priority) {
    if (handle.id == 0) return;
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_registry.find(handle.id);
    if (it != m_registry.end()) it->second->priority = priority;
}

bool TextureStreamer::ForceLoad(TextureHandle handle) {
    if (handle.id == 0) return false;

    std::shared_ptr<Texture> tex;
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        auto it = m_registry.find(handle.id);
        if (it == m_registry.end()) return false;
        tex = it->second;
    }

    if (tex->resident) return true;

    // FIX: If worker is currently loading this texture, wait for it
    if (tex->isLoading.exchange(true)) {
        // Another thread is loading; wait for completion
        while (tex->isLoading) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return tex->resident;
    }

    // We own the load now
    auto t0 = std::chrono::steady_clock::now();
    bool ok = LoadTextureFile(tex->sourcePath, *tex, 0);
    tex->isLoading = false;
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (ok) {
        m_cache->Insert(tex);
        {
            std::lock_guard<std::mutex> lock(m_metricsMutex);
            m_metrics.avgLoadTimeMs = (m_metrics.avgLoadTimeMs * m_metrics.loadCount + ms) / (m_metrics.loadCount + 1);
            m_metrics.loadCount++;
            m_metrics.residentBytes = m_cache->GetUsedBytes();
            m_metrics.residentTextures = m_cache->GetResidentCount();
            m_metrics.peakBytes = std::max(m_metrics.peakBytes, m_metrics.residentBytes);
        }
        spdlog::info("TextureStreamer: force-loaded '{}' in {:.2f} ms", tex->name, ms);
    } else {
        spdlog::error("TextureStreamer: force-load failed for '{}'", tex->sourcePath.string());
    }

    tex->isStreaming = false;
    return ok;
}

void TextureStreamer::Update([[maybe_unused]] float deltaTime) {
    // Process completed loads
    std::vector<uint32_t> completed;
    {
        std::lock_guard<std::mutex> lock(m_completeMutex);
        completed.swap(m_completed);
    }

    for (uint32_t id : completed) {
        std::shared_ptr<Texture> tex;
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            auto it = m_registry.find(id);
            if (it == m_registry.end()) continue;
            tex = it->second;
        }

        if (!tex->mips.empty()) {
            m_cache->Insert(tex);
        }
        tex->isStreaming = false;
    }

    // FIX: Always keep metrics in sync with actual cache state
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.requestsPending = m_inQueue.size();
        m_metrics.residentTextures = m_cache->GetResidentCount();
        m_metrics.residentBytes = m_cache->GetUsedBytes();
        m_metrics.streamingTextures = 0;

        for (const auto& [id, tex] : m_registry) {
            if (tex->isStreaming && !tex->resident) {
                m_metrics.streamingTextures++;
            }
        }
    }
}

void TextureStreamer::PreloadTextures(const std::vector<std::filesystem::path>& paths) {
    for (const auto& path : paths) {
        RequestTexture(path, TextureFormat::RGBA8_UNORM, TextureType::Albedo, 1.0f);
    }
}

void TextureStreamer::Release(TextureHandle handle) {
    if (handle.id == 0) return;

    // FIX: Remove from cache and registry directly, no broken manual refCount
    m_cache->Remove(handle.id);

    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        auto it = m_registry.find(handle.id);
        if (it == m_registry.end()) return;
        m_registry.erase(it);
    }

    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.evictedTextures++;
        if (m_metrics.totalTextures > 0) m_metrics.totalTextures--;
    }

    spdlog::debug("TextureStreamer: released texture ID={}", handle.id);
}

StreamingMetrics TextureStreamer::GetMetrics() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    return m_metrics;
}

void TextureStreamer::SetMemoryBudget(size_t bytes) {
    if (m_cache) {
        // Re-create cache with new budget (simpler than resizing)
        auto oldCache = std::move(m_cache);
        m_cache = std::make_unique<TextureCache>(bytes);
        // Re-insert all resident textures
        // (simplified: in production we'd iterate old cache entries)
    }
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.budgetBytes = bytes;
    }
}

size_t TextureStreamer::GetMemoryBudget() const {
    if (!m_cache) return 0;
    return m_cache->GetBudgetBytes();
}

void TextureStreamer::SetGlobalMipBias(float bias) {
    m_globalMipBias = bias;
}

float TextureStreamer::GetGlobalMipBias() const {
    return m_globalMipBias.load();
}

void TextureStreamer::DumpState() const {
    auto metrics = GetMetrics();
    spdlog::info("=== TextureStreamer State ===");
    spdlog::info("  Total textures:    {}", metrics.totalTextures);
    spdlog::info("  Resident:          {}", metrics.residentTextures);
    spdlog::info("  Streaming:         {}", metrics.streamingTextures);
    spdlog::info("  Evicted:           {}", metrics.evictedTextures);
    spdlog::info("  Memory:            {} / {} MB (peak {} MB)",
                 metrics.residentBytes / (1024*1024),
                 metrics.budgetBytes / (1024*1024),
                 metrics.peakBytes / (1024*1024));
    spdlog::info("  Pending requests:  {}", metrics.requestsPending);
    spdlog::info("  Avg load time:     {:.2f} ms", metrics.avgLoadTimeMs);
}

// ============================================================================
// Worker Thread
// ============================================================================
void TextureStreamer::WorkerLoop() {
    spdlog::debug("TextureStreamer: worker thread started");
    while (m_running) {
        uint32_t id = 0;
        float priority = 0.0f;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait(lock, [this] { return !m_requestQueue.empty() || !m_running; });
            if (!m_running) break;
            if (m_requestQueue.empty()) continue;

            auto item = m_requestQueue.top();
            priority = item.first;
            id = item.second;
            m_requestQueue.pop();
            m_inQueue.erase(id);
        }

        std::shared_ptr<Texture> tex;
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            auto it = m_registry.find(id);
            if (it == m_registry.end()) continue;
            tex = it->second;
        }

        // FIX: Skip if ForceLoad already handled this texture
        if (tex->resident || tex->isLoading.exchange(true)) {
            continue;  // Already resident or ForceLoad is handling it
        }

        auto t0 = std::chrono::steady_clock::now();
        bool ok = LoadTextureFile(tex->sourcePath, *tex, 0);
        tex->isLoading = false;
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        if (ok) {
            {
                std::lock_guard<std::mutex> lock(m_metricsMutex);
                if (m_metrics.loadCount == 0) {
                    m_metrics.avgLoadTimeMs = ms;
                } else {
                    m_metrics.avgLoadTimeMs = (m_metrics.avgLoadTimeMs * m_metrics.loadCount + ms) / (m_metrics.loadCount + 1);
                }
                m_metrics.loadCount++;
            }
            spdlog::debug("TextureStreamer: async loaded '{}' ({}x{}, {} mips) in {:.2f} ms",
                          tex->name, tex->width, tex->height, tex->mipCount, ms);

            {
                std::lock_guard<std::mutex> lock(m_completeMutex);
                m_completed.push_back(id);
            }
        } else {
            spdlog::error("TextureStreamer: failed to load '{}'", tex->sourcePath.string());
            tex->isStreaming = false;
        }
    }
    spdlog::debug("TextureStreamer: worker thread stopped");
}

// ============================================================================
// Texture File Loading (stub with procedural fallback for testing)
// ============================================================================

// Simple raw texture format header for testing
struct RawTextureHeader {
    uint32_t magic = 0x54545257;  // 'WRIT' (little endian)
    uint32_t version = 1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t format = 0;  // TextureFormat enum
    uint32_t mipCount = 0;
};

static bool LoadRawTexture(const std::filesystem::path& path, Texture& outTex) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    RawTextureHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.magic != 0x54545257) return false;

    outTex.width = header.width;
    outTex.height = header.height;
    outTex.format = static_cast<TextureFormat>(header.format);
    outTex.mipCount = header.mipCount;

    uint32_t channels = 4;  // default RGBA
    if (outTex.format == TextureFormat::RG8_UNORM) channels = 2;
    else if (outTex.format == TextureFormat::R8_UNORM) channels = 1;

    for (uint32_t i = 0; i < outTex.mipCount; ++i) {
        MipLevel mip;
        mip.width = std::max(1u, outTex.width >> i);
        mip.height = std::max(1u, outTex.height >> i);
        mip.dataSize = mip.width * mip.height * channels;
        mip.data.resize(mip.dataSize);
        file.read(reinterpret_cast<char*>(mip.data.data()), mip.dataSize);
        outTex.mips.push_back(std::move(mip));
    }
    return true;
}

// Generate a procedural test texture (checkerboard or noise)
static void GenerateProceduralTexture(Texture& outTex, uint32_t width, uint32_t height,
                                       TextureFormat format, TextureType type) {
    outTex.width = width;
    outTex.height = height;
    outTex.format = format;
    outTex.type = type;

    uint32_t channels = 4;
    if (format == TextureFormat::RG8_UNORM) channels = 2;
    else if (format == TextureFormat::R8_UNORM) channels = 1;

    // Generate mip chain
    uint32_t mipLevels = 1;
    uint32_t w = width, h = height;
    while (w > 1 || h > 1) {
        w = std::max(1u, w >> 1);
        h = std::max(1u, h >> 1);
        mipLevels++;
    }
    outTex.mipCount = mipLevels;

    w = width; h = height;
    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
        MipLevel level;
        level.width = w;
        level.height = h;
        level.dataSize = w * h * channels;
        level.data.resize(level.dataSize);

        // Procedural pattern based on type
        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                size_t idx = (y * w + x) * channels;
                if (type == TextureType::Albedo) {
                    // Checkerboard pattern
                    bool check = ((x / 16) + (y / 16)) % 2 == 0;
                    uint8_t val = check ? 200 : 50;
                    level.data[idx + 0] = val;
                    level.data[idx + 1] = val;
                    level.data[idx + 2] = val;
                    if (channels == 4) level.data[idx + 3] = 255;
                } else if (type == TextureType::Normal) {
                    // Flat normal map (0.5, 0.5, 1.0)
                    level.data[idx + 0] = 128;
                    level.data[idx + 1] = 128;
                    if (channels >= 3) level.data[idx + 2] = 255;
                    if (channels == 4) level.data[idx + 3] = 255;
                } else if (type == TextureType::Roughness) {
                    uint8_t val = static_cast<uint8_t>(128 + 64 * std::sin(x * 0.1) * std::cos(y * 0.1));
                    level.data[idx] = val;
                } else {
                    // Default gradient
                    uint8_t val = static_cast<uint8_t>(255 * (x + y) / (w + h));
                    for (uint32_t c = 0; c < channels; ++c) level.data[idx + c] = val;
                }
            }
        }
        outTex.mips.push_back(std::move(level));
        w = std::max(1u, w >> 1);
        h = std::max(1u, h >> 1);
    }
}

bool TextureStreamer::LoadTextureFile(const std::filesystem::path& path, Texture& outTex, 
                                        [[maybe_unused]] uint32_t maxMipCount) {
    // Try raw format first
    if (LoadRawTexture(path, outTex)) {
        return true;
    }

    // Fallback: generate procedural texture for testing
    // In production, this would try PNG/JPG via stb_image, DDS, KTX2, etc.
    spdlog::debug("TextureStreamer: generating procedural fallback for '{}'", path.string());

    // Determine size from filename hint (e.g. "4k_" prefix)
    uint32_t width = 256, height = 256;
    std::string filename = path.filename().string();
    if (filename.find("4k") != std::string::npos || filename.find("4096") != std::string::npos) {
        width = 4096; height = 4096;
    } else if (filename.find("2k") != std::string::npos || filename.find("2048") != std::string::npos) {
        width = 2048; height = 2048;
    } else if (filename.find("1k") != std::string::npos || filename.find("1024") != std::string::npos) {
        width = 1024; height = 1024;
    }

    GenerateProceduralTexture(outTex, width, height, outTex.format, outTex.type);
    return true;
}

void TextureStreamer::UpdateMetrics() {
    // Called periodically
}

} // namespace TheSeed::Render
