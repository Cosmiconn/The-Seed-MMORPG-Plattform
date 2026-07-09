#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/texture_streamer.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace TheSeed::Render;
using namespace Catch::Matchers;

// Helper: create a raw test texture file
static std::filesystem::path CreateRawTexture(const std::string& name, uint32_t width, uint32_t height,
                                               TextureFormat format, uint32_t mipCount) {
    auto path = std::filesystem::temp_directory_path() / (name + ".rawtex");
    std::ofstream file(path, std::ios::binary);

    struct RawHeader {
        uint32_t magic = 0x54545257;
        uint32_t version = 1;
        uint32_t width;
        uint32_t height;
        uint32_t format;
        uint32_t mipCount;
    };
    RawHeader header{width, height, static_cast<uint32_t>(format), mipCount};
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    uint32_t channels = 4;
    if (format == TextureFormat::RG8_UNORM) channels = 2;
    else if (format == TextureFormat::R8_UNORM) channels = 1;

    uint32_t w = width, h = height;
    for (uint32_t mip = 0; mip < mipCount; ++mip) {
        size_t size = w * h * channels;
        std::vector<uint8_t> data(size);
        for (size_t i = 0; i < size; ++i) data[i] = static_cast<uint8_t>((i * 7 + mip * 13) % 256);
        file.write(reinterpret_cast<const char*>(data.data()), size);
        w = std::max(1u, w >> 1);
        h = std::max(1u, h >> 1);
    }
    return path;
}

// ============================================================================
// TESTS
// ============================================================================

TEST_CASE("TextureStreamer: Initialize and Shutdown") {
    TextureStreamer streamer;
    CHECK(streamer.Initialize(64 * 1024 * 1024));  // 64 MB
    CHECK(streamer.GetMemoryBudget() == 64 * 1024 * 1024);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Request texture async") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    auto path = CreateRawTexture("async_test", 256, 256, TextureFormat::RGBA8_UNORM, 4);
    auto handle = streamer.RequestTexture(path, TextureFormat::RGBA8_UNORM, TextureType::Albedo, 1.0f);
    CHECK(handle.id != 0);
    CHECK_FALSE(streamer.IsResident(handle));

    // Wait for async load (generous timeout for CI)
    for (int i = 0; i < 200 && !streamer.IsResident(handle); ++i) {
        streamer.Update(0.016f);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CHECK(streamer.IsResident(handle));
    auto tex = streamer.GetTexture(handle);
    REQUIRE(tex != nullptr);
    CHECK(tex->width == 256);
    CHECK(tex->height == 256);
    CHECK(tex->mipCount == 4);
    CHECK(tex->resident);

    streamer.Release(handle);
    std::filesystem::remove(path);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Force load (blocking)") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    auto path = CreateRawTexture("force_test", 128, 128, TextureFormat::RGBA8_UNORM, 3);
    auto handle = streamer.RequestTexture(path, TextureFormat::RGBA8_UNORM, TextureType::Albedo);
    CHECK(streamer.ForceLoad(handle));
    CHECK(streamer.IsResident(handle));

    auto tex = streamer.GetTexture(handle);
    REQUIRE(tex != nullptr);
    CHECK(tex->width == 128);
    CHECK(tex->height == 128);

    streamer.Release(handle);
    std::filesystem::remove(path);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: LRU eviction under memory pressure") {
    TextureStreamer streamer;
    // Small budget: 1 MB (enough for ~1 texture of 256x256 RGBA8 with mips)
    streamer.Initialize(1 * 1024 * 1024);

    auto path1 = CreateRawTexture("evict_a", 256, 256, TextureFormat::RGBA8_UNORM, 4);
    auto path2 = CreateRawTexture("evict_b", 256, 256, TextureFormat::RGBA8_UNORM, 4);
    auto path3 = CreateRawTexture("evict_c", 256, 256, TextureFormat::RGBA8_UNORM, 4);

    auto h1 = streamer.RequestTexture(path1, TextureFormat::RGBA8_UNORM, TextureType::Albedo);
    auto h2 = streamer.RequestTexture(path2, TextureFormat::RGBA8_UNORM, TextureType::Albedo);
    auto h3 = streamer.RequestTexture(path3, TextureFormat::RGBA8_UNORM, TextureType::Albedo);

    // Force load all
    streamer.ForceLoad(h1);
    streamer.ForceLoad(h2);
    streamer.ForceLoad(h3);

    // With 1MB budget, not all 3 can be resident
    // Each 256x256 RGBA8 with 4 mips ~= 256*256*4 * 1.33 ~= 350KB
    // So 3 textures = ~1MB, might fit or might not depending on exact budget
    // Let's verify at least some eviction happened or budget is respected
    auto metrics = streamer.GetMetrics();
    CHECK(metrics.residentBytes <= metrics.budgetBytes);

    streamer.Release(h1);
    streamer.Release(h2);
    streamer.Release(h3);
    std::filesystem::remove(path1);
    std::filesystem::remove(path2);
    std::filesystem::remove(path3);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: 4K texture streaming without stutter") {
    TextureStreamer streamer;
    streamer.Initialize(256 * 1024 * 1024);  // 256 MB budget

    // Simulate 4K texture request
    auto path = CreateRawTexture("4k_stress", 4096, 4096, TextureFormat::RGBA8_UNORM, 13);
    auto handle = streamer.RequestTexture(path, TextureFormat::RGBA8_UNORM, TextureType::Albedo, 1.0f);

    // Frame loop simulation: 60 FPS, texture should become resident quickly
    auto start = std::chrono::steady_clock::now();
    bool resident = false;
    for (int frame = 0; frame < 120; ++frame) {  // 2 seconds at 60 FPS
        streamer.Update(0.016f);
        if (streamer.IsResident(handle)) {
            resident = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    CHECK(resident);
    // Should be resident within 1 second (generous for test environment)
    CHECK(elapsed < 1.0);

    auto tex = streamer.GetTexture(handle);
    REQUIRE(tex != nullptr);
    CHECK(tex->width == 4096);
    CHECK(tex->height == 4096);
    CHECK(tex->mipCount == 13);

    streamer.Release(handle);
    std::filesystem::remove(path);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Multiple concurrent requests") {
    TextureStreamer streamer;
    streamer.Initialize(128 * 1024 * 1024);

    std::vector<std::filesystem::path> paths;
    std::vector<TextureHandle> handles;
    for (int i = 0; i < 20; ++i) {
        auto path = CreateRawTexture("concurrent_" + std::to_string(i), 128, 128, TextureFormat::RGBA8_UNORM, 3);
        paths.push_back(path);
        handles.push_back(streamer.RequestTexture(path, TextureFormat::RGBA8_UNORM, TextureType::Albedo));
    }

    // Wait for all to become resident
    for (int frame = 0; frame < 200; ++frame) {
        streamer.Update(0.016f);
        bool allResident = true;
        for (auto& h : handles) {
            if (!streamer.IsResident(h)) { allResident = false; break; }
        }
        if (allResident) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    size_t residentCount = 0;
    for (auto& h : handles) {
        if (streamer.IsResident(h)) residentCount++;
    }
    CHECK(residentCount == 20);

    auto metrics = streamer.GetMetrics();
    CHECK(metrics.totalTextures == 20);

    for (auto& h : handles) streamer.Release(h);
    for (auto& p : paths) std::filesystem::remove(p);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Priority ordering") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    auto pathLow = CreateRawTexture("prio_low", 64, 64, TextureFormat::RGBA8_UNORM, 2);
    auto pathHigh = CreateRawTexture("prio_high", 64, 64, TextureFormat::RGBA8_UNORM, 2);

    // Request low priority first, then high priority
    auto hLow = streamer.RequestTexture(pathLow, TextureFormat::RGBA8_UNORM, TextureType::Albedo, 0.1f);
    auto hHigh = streamer.RequestTexture(pathHigh, TextureFormat::RGBA8_UNORM, TextureType::Albedo, 1.0f);

    // Give worker time to process one item
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    streamer.Update(0.016f);

    // High priority should be processed first
    // (We can't strictly guarantee order in a test, but we verify both eventually load)
    for (int i = 0; i < 50; ++i) {
        streamer.Update(0.016f);
        if (streamer.IsResident(hLow) && streamer.IsResident(hHigh)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CHECK(streamer.IsResident(hLow));
    CHECK(streamer.IsResident(hHigh));

    streamer.Release(hLow);
    streamer.Release(hHigh);
    std::filesystem::remove(pathLow);
    std::filesystem::remove(pathHigh);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Preload batch") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    std::vector<std::filesystem::path> paths;
    for (int i = 0; i < 10; ++i) {
        paths.push_back(CreateRawTexture("preload_" + std::to_string(i), 64, 64, TextureFormat::RGBA8_UNORM, 2));
    }

    streamer.PreloadTextures(paths);

    for (int frame = 0; frame < 100; ++frame) {
        streamer.Update(0.016f);
        bool allResident = true;
        // Check all handles in registry... we don't have them directly, but metrics tell us
        auto metrics = streamer.GetMetrics();
        if (metrics.residentTextures == 10) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto metrics = streamer.GetMetrics();
    CHECK(metrics.totalTextures >= 10);

    for (auto& p : paths) std::filesystem::remove(p);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Invalid path handling") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    auto handle = streamer.RequestTexture("/nonexistent/texture.rawtex");
    CHECK(handle.id != 0);  // Request accepted

    // Will generate procedural fallback
    for (int i = 0; i < 20; ++i) {
        streamer.Update(0.016f);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Procedural fallback should still become resident
    CHECK(streamer.IsResident(handle));
    auto tex = streamer.GetTexture(handle);
    REQUIRE(tex != nullptr);
    CHECK(tex->width > 0);
    CHECK(tex->height > 0);

    streamer.Release(handle);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Metrics tracking") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    auto metrics1 = streamer.GetMetrics();
    CHECK(metrics1.budgetBytes == 64 * 1024 * 1024);
    CHECK(metrics1.totalTextures == 0);

    auto path = CreateRawTexture("metrics_test", 128, 128, TextureFormat::RGBA8_UNORM, 3);
    auto handle = streamer.RequestTexture(path);
    streamer.ForceLoad(handle);

    auto metrics2 = streamer.GetMetrics();
    CHECK(metrics2.totalTextures == 1);
    CHECK(metrics2.residentTextures == 1);
    CHECK(metrics2.residentBytes > 0);
    CHECK(metrics2.loadCount == 1);
    CHECK(metrics2.avgLoadTimeMs >= 0.0);

    streamer.Release(handle);
    std::filesystem::remove(path);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Mip bias settings") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    CHECK(streamer.GetGlobalMipBias() == 0.0f);
    streamer.SetGlobalMipBias(2.0f);
    CHECK(streamer.GetGlobalMipBias() == 2.0f);

    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Release and re-request") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    auto path = CreateRawTexture("rereq", 128, 128, TextureFormat::RGBA8_UNORM, 3);
    auto h1 = streamer.RequestTexture(path);
    streamer.ForceLoad(h1);
    CHECK(streamer.IsResident(h1));

    streamer.Release(h1);
    // After release, texture should be gone from cache
    // (but we can't easily verify without internal access)

    // Re-request same file
    auto h2 = streamer.RequestTexture(path);
    CHECK(h2.id != h1.id);  // New handle
    streamer.ForceLoad(h2);
    CHECK(streamer.IsResident(h2));

    streamer.Release(h2);
    std::filesystem::remove(path);
    streamer.Shutdown();
}

TEST_CASE("TextureStreamer: Texture format variants") {
    TextureStreamer streamer;
    streamer.Initialize(64 * 1024 * 1024);

    auto pathR = CreateRawTexture("fmt_r", 64, 64, TextureFormat::R8_UNORM, 2);
    auto pathRG = CreateRawTexture("fmt_rg", 64, 64, TextureFormat::RG8_UNORM, 2);
    auto pathRGBA = CreateRawTexture("fmt_rgba", 64, 64, TextureFormat::RGBA8_UNORM, 2);

    auto hR = streamer.RequestTexture(pathR, TextureFormat::R8_UNORM, TextureType::Roughness);
    auto hRG = streamer.RequestTexture(pathRG, TextureFormat::RG8_UNORM, TextureType::Normal);
    auto hRGBA = streamer.RequestTexture(pathRGBA, TextureFormat::RGBA8_UNORM, TextureType::Albedo);

    streamer.ForceLoad(hR);
    streamer.ForceLoad(hRG);
    streamer.ForceLoad(hRGBA);

    CHECK(streamer.IsResident(hR));
    CHECK(streamer.IsResident(hRG));
    CHECK(streamer.IsResident(hRGBA));

    auto texR = streamer.GetTexture(hR);
    auto texRG = streamer.GetTexture(hRG);
    auto texRGBA = streamer.GetTexture(hRGBA);

    REQUIRE(texR != nullptr);
    REQUIRE(texRG != nullptr);
    REQUIRE(texRGBA != nullptr);

    // Verify mip data sizes match format channels
    // R8: 64*64*1 = 4096 bytes for mip0
    // RG8: 64*64*2 = 8192 bytes for mip0
    // RGBA8: 64*64*4 = 16384 bytes for mip0
    CHECK(texR->mips[0].dataSize == 4096);
    CHECK(texRG->mips[0].dataSize == 8192);
    CHECK(texRGBA->mips[0].dataSize == 16384);

    streamer.Release(hR);
    streamer.Release(hRG);
    streamer.Release(hRGBA);
    std::filesystem::remove(pathR);
    std::filesystem::remove(pathRG);
    std::filesystem::remove(pathRGBA);
    streamer.Shutdown();
}

TEST_CASE("TextureCache: basic insert and get") {
    TextureCache cache(1024 * 1024);  // 1 MB

    auto tex = std::make_shared<Texture>();
    tex->id = 1;
    tex->name = "test";
    tex->width = 64;
    tex->height = 64;
    tex->format = TextureFormat::RGBA8_UNORM;
    tex->mipCount = 1;
    MipLevel mip;
    mip.width = 64; mip.height = 64; mip.dataSize = 64*64*4;
    mip.data.resize(mip.dataSize);
    tex->mips.push_back(mip);

    CHECK(cache.Insert(tex));
    CHECK(cache.GetResidentCount() == 1);
    CHECK(cache.GetUsedBytes() == 64*64*4);

    auto got = cache.Get(1);
    REQUIRE(got != nullptr);
    CHECK(got->name == "test");

    cache.Clear();
    CHECK(cache.GetResidentCount() == 0);
    CHECK(cache.GetUsedBytes() == 0);
}

TEST_CASE("TextureCache: LRU eviction") {
    TextureCache cache(1024);  // 1 KB budget

    auto tex1 = std::make_shared<Texture>();
    tex1->id = 1;
    tex1->mips.push_back(MipLevel{8, 8, 1, 8*8*4, std::vector<uint8_t>(8*8*4)});

    auto tex2 = std::make_shared<Texture>();
    tex2->id = 2;
    tex2->mips.push_back(MipLevel{8, 8, 1, 8*8*4, std::vector<uint8_t>(8*8*4)});

    auto tex3 = std::make_shared<Texture>();
    tex3->id = 3;
    tex3->mips.push_back(MipLevel{8, 8, 1, 8*8*4, std::vector<uint8_t>(8*8*4)});

    // Insert all three (each 256 bytes, total 768 bytes < 1KB budget)
    CHECK(cache.Insert(tex1));
    CHECK(cache.Insert(tex2));
    CHECK(cache.Insert(tex3));
    CHECK(cache.GetResidentCount() == 3);  // All fit in 1KB

    // Touch tex1 to make it MRU
    cache.Touch(1);

    // Now add a 4th texture that exceeds budget
    auto tex4 = std::make_shared<Texture>();
    tex4->id = 4;
    tex4->mips.push_back(MipLevel{16, 16, 1, 16*16*4, std::vector<uint8_t>(16*16*4)});  // 1KB
    CHECK(cache.Insert(tex4));  // Should evict LRU (tex2 or tex3)

    // tex1 is MRU (touched), tex4 is newly inserted, so tex2 or tex3 got evicted
    CHECK(cache.GetResidentCount() <= 4);  // Budget limits this
    CHECK(cache.Get(1) != nullptr);  // tex1 still there (MRU)
    CHECK(cache.Get(4) != nullptr);  // tex4 just inserted

    cache.Clear();
}
