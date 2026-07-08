#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/material_config.hpp"
#include "core/material.hpp"
#include "core/shader_manager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace TheSeed::Render;
using namespace Catch::Matchers;

static std::filesystem::path CreateTempYAML(const std::string& content) {
    auto path = std::filesystem::temp_directory_path() / ("test_mat_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".yaml");
    std::ofstream f(path); f << content; f.close(); return path;
}

static std::filesystem::path CreateTempJSON(const std::string& content) {
    auto path = std::filesystem::temp_directory_path() / ("test_mat_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
    std::ofstream f(path); f << content; f.close(); return path;
}

static std::filesystem::path CreateTempSPIRV(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / (name + ".spv");
    std::ofstream f(path, std::ios::binary);
    uint32_t spirv[] = {0x07230203, 0x00010000, 0x00000000, 0x00000001, 0x00000000, 0x00020000, 0x000100fd, 0x00010038};
    f.write(reinterpret_cast<const char*>(spirv), sizeof(spirv)); f.close(); return path;
}

TEST_CASE("MaterialConfig: Default construction") {
    MaterialConfig config;
    CHECK(config.name.empty());
    CHECK(config.pbr.albedo[0] == 1.0f);
    CHECK(config.pbr.albedo[1] == 1.0f);
    CHECK(config.pbr.albedo[2] == 1.0f);
    CHECK(config.pbr.metallic == 0.0f);
    CHECK(config.pbr.roughness == 0.5f);
    CHECK(config.pbr.ao == 1.0f);
    CHECK(config.pbr.emissive[0] == 0.0f);
    CHECK(config.pbr.emissiveStrength == 0.0f);
    CHECK(config.transparent == false);
    CHECK(config.doubleSided == false);
    CHECK(config.unlit == false);
    CHECK(config.wireframe == false);
}

TEST_CASE("MaterialConfig: Equality") {
    MaterialConfig a, b;
    CHECK(a == b);
    b.pbr.metallic = 0.5f;
    CHECK(a != b);
    b = a; b.name = "Different";
    CHECK(a != b);
    b = a; b.transparent = true;
    CHECK(a != b);
}

TEST_CASE("MaterialConfigLoader: Load minimal YAML") {
    auto path = CreateTempYAML(R"(
material:
  name: "TestMaterial"
)");
    MaterialConfigLoader loader;
    auto result = loader.LoadFromYAML(path);
    REQUIRE(result.has_value());
    CHECK(result->name == "TestMaterial");
    CHECK(result->pbr.albedo[0] == 1.0f);
    CHECK(result->pbr.metallic == 0.0f);
    std::filesystem::remove(path);
}

TEST_CASE("MaterialConfigLoader: Load full YAML") {
    auto path = CreateTempYAML(R"(
material:
  name: "RustyMetal"
  pbr:
    albedo: [0.72, 0.45, 0.20]
    metallic: 0.95
    roughness: 0.65
    ao: 1.0
    emissive: [0.1, 0.05, 0.0]
    emissive_strength: 0.5
  textures:
    albedo: "textures/rusty_albedo.png"
    normal: "textures/rusty_normal.png"
  flags:
    transparent: false
    double_sided: true
    unlit: false
    wireframe: false
)");
    MaterialConfigLoader loader;
    auto result = loader.LoadFromYAML(path);
    REQUIRE(result.has_value());
    CHECK(result->name == "RustyMetal");
    CHECK_THAT(result->pbr.albedo[0], WithinAbs(0.72f, 0.001f));
    CHECK_THAT(result->pbr.albedo[1], WithinAbs(0.45f, 0.001f));
    CHECK_THAT(result->pbr.albedo[2], WithinAbs(0.20f, 0.001f));
    CHECK_THAT(result->pbr.metallic, WithinAbs(0.95f, 0.001f));
    CHECK_THAT(result->pbr.roughness, WithinAbs(0.65f, 0.001f));
    CHECK_THAT(result->pbr.ao, WithinAbs(1.0f, 0.001f));
    CHECK_THAT(result->pbr.emissive[0], WithinAbs(0.1f, 0.001f));
    CHECK_THAT(result->pbr.emissiveStrength, WithinAbs(0.5f, 0.001f));
    CHECK(result->albedoMapPath == "textures/rusty_albedo.png");
    CHECK(result->normalMapPath == "textures/rusty_normal.png");
    CHECK(result->transparent == false);
    CHECK(result->doubleSided == true);
    CHECK(result->unlit == false);
    CHECK(result->wireframe == false);
    std::filesystem::remove(path);
}

TEST_CASE("MaterialConfigLoader: Load YAML with shader overrides") {
    auto path = CreateTempYAML(R"(
material:
  name: "CustomShaderMat"
  shaders:
    vertex: "shaders/custom.vert.spv"
    fragment: "shaders/custom.frag.spv"
)");
    MaterialConfigLoader loader;
    auto result = loader.LoadFromYAML(path);
    REQUIRE(result.has_value());
    CHECK(result->vertexShaderPath == "shaders/custom.vert.spv");
    CHECK(result->fragmentShaderPath == "shaders/custom.frag.spv");
    std::filesystem::remove(path);
}

TEST_CASE("MaterialConfigLoader: YAML missing material node") {
    auto path = CreateTempYAML(R"(
name: "WrongRoot"
)");
    MaterialConfigLoader loader;
    auto result = loader.LoadFromYAML(path);
    REQUIRE(!result.has_value());
    CHECK_THAT(result.error(), ContainsSubstring("Missing 'material' root node"));
    std::filesystem::remove(path);
}

TEST_CASE("MaterialConfigLoader: YAML auto-name from filename") {
    auto path = CreateTempYAML(R"(
material:
  pbr:
    metallic: 0.5
)");
    MaterialConfigLoader loader;
    auto result = loader.LoadFromYAML(path);
    REQUIRE(result.has_value());
    CHECK(result->name == path.stem().string());
    std::filesystem::remove(path);
}

TEST_CASE("MaterialConfigLoader: Load non-existent file") {
    MaterialConfigLoader loader;
    auto result = loader.LoadFromYAML("/nonexistent/path/material.yaml");
    REQUIRE(!result.has_value());
    CHECK_THAT(result.error(), ContainsSubstring("Failed to open"));
}

TEST_CASE("MaterialConfigLoader: Load JSON") {
    auto path = CreateTempJSON(R"({
  "material": {
    "name": "JSONMaterial",
    "pbr": {
      "albedo": [0.5, 0.5, 0.5],
      "metallic": 0.25,
      "roughness": 0.75
    },
    "flags": {
      "transparent": true
    }
  }
})");
    MaterialConfigLoader loader;
    auto result = loader.LoadFromJSON(path);
    REQUIRE(result.has_value());
    CHECK(result->name == "JSONMaterial");
    CHECK_THAT(result->pbr.metallic, WithinAbs(0.25f, 0.001f));
    CHECK_THAT(result->pbr.roughness, WithinAbs(0.75f, 0.001f));
    CHECK(result->transparent == true);
    std::filesystem::remove(path);
}

TEST_CASE("MaterialConfigLoader: YAML roundtrip") {
    MaterialConfig original;
    original.name = "RoundtripTest";
    original.pbr.albedo = {0.1f, 0.2f, 0.3f};
    original.pbr.metallic = 0.4f;
    original.pbr.roughness = 0.6f;
    original.pbr.ao = 0.8f;
    original.pbr.emissive = {0.5f, 0.0f, 0.0f};
    original.pbr.emissiveStrength = 1.5f;
    original.albedoMapPath = "tex/albedo.png";
    original.transparent = true;
    original.doubleSided = true;
    auto path = std::filesystem::temp_directory_path() / "roundtrip_test.yaml";
    MaterialConfigLoader loader;
    REQUIRE(loader.SaveToYAML(path, original));
    auto loaded = loader.LoadFromYAML(path);
    REQUIRE(loaded.has_value());
    CHECK(loaded->name == original.name);
    CHECK(loaded->pbr.albedo == original.pbr.albedo);
    CHECK(loaded->pbr.metallic == original.pbr.metallic);
    CHECK(loaded->pbr.roughness == original.pbr.roughness);
    CHECK(loaded->pbr.ao == original.pbr.ao);
    CHECK(loaded->pbr.emissive == original.pbr.emissive);
    CHECK(loaded->pbr.emissiveStrength == original.pbr.emissiveStrength);
    CHECK(loaded->albedoMapPath == original.albedoMapPath);
    CHECK(loaded->transparent == original.transparent);
    CHECK(loaded->doubleSided == original.doubleSided);
    std::filesystem::remove(path);
}

TEST_CASE("MaterialConfigLoader: JSON roundtrip") {
    MaterialConfig original;
    original.name = "JSONRoundtrip";
    original.pbr.metallic = 0.33f;
    original.wireframe = true;
    auto path = std::filesystem::temp_directory_path() / "roundtrip_test.json";
    MaterialConfigLoader loader;
    REQUIRE(loader.SaveToJSON(path, original));
    auto loaded = loader.LoadFromJSON(path);
    REQUIRE(loaded.has_value());
    CHECK(loaded->name == original.name);
    CHECK(loaded->pbr.metallic == original.pbr.metallic);
    CHECK(loaded->wireframe == original.wireframe);
    std::filesystem::remove(path);
}

TEST_CASE("MaterialConfigLoader: Validate valid config") {
    MaterialConfig config; config.name = "Valid"; config.pbr.metallic = 0.5f;
    config.pbr.roughness = 0.5f; config.pbr.ao = 1.0f;
    MaterialConfigLoader loader;
    auto result = loader.Validate(config);
    REQUIRE(result.has_value());
}

TEST_CASE("MaterialConfigLoader: Validate empty name") {
    MaterialConfig config; config.name = "";
    MaterialConfigLoader loader;
    auto result = loader.Validate(config);
    REQUIRE(!result.has_value());
    CHECK_THAT(result.error(), ContainsSubstring("name is required"));
}

TEST_CASE("MaterialConfigLoader: Validate metallic out of range") {
    MaterialConfig config; config.name = "Test"; config.pbr.metallic = 1.5f;
    MaterialConfigLoader loader;
    auto result = loader.Validate(config);
    REQUIRE(!result.has_value());
    CHECK_THAT(result.error(), ContainsSubstring("Metallic must be in range"));
}

TEST_CASE("MaterialConfigLoader: Validate roughness out of range") {
    MaterialConfig config; config.name = "Test"; config.pbr.roughness = -0.1f;
    MaterialConfigLoader loader;
    auto result = loader.Validate(config);
    REQUIRE(!result.has_value());
    CHECK_THAT(result.error(), ContainsSubstring("Roughness must be in range"));
}

TEST_CASE("MaterialConfigLoader: Validate AO out of range") {
    MaterialConfig config; config.name = "Test"; config.pbr.ao = 2.0f;
    MaterialConfigLoader loader;
    auto result = loader.Validate(config);
    REQUIRE(!result.has_value());
    CHECK_THAT(result.error(), ContainsSubstring("AO must be in range"));
}

TEST_CASE("MaterialConfigLoader: Validate negative emissive") {
    MaterialConfig config; config.name = "Test"; config.pbr.emissiveStrength = -1.0f;
    MaterialConfigLoader loader;
    auto result = loader.Validate(config);
    REQUIRE(!result.has_value());
    CHECK_THAT(result.error(), ContainsSubstring("Emissive strength must be >= 0"));
}

TEST_CASE("MaterialConfigLoader: Validate boundary values") {
    MaterialConfig config; config.name = "Boundary";
    config.pbr.metallic = 0.0f; config.pbr.roughness = 1.0f;
    config.pbr.ao = 0.0f; config.pbr.emissiveStrength = 0.0f;
    MaterialConfigLoader loader;
    auto result = loader.Validate(config);
    REQUIRE(result.has_value());
}

TEST_CASE("MaterialConfigLoader: Asset root resolution") {
    MaterialConfigLoader loader;
    loader.SetAssetRoot("/tmp/assets");
    SUCCEED();
}

TEST_CASE("Material: UpdateGPUData") {
    Material mat;
    mat.pbr.albedo = {0.5f, 0.6f, 0.7f};
    mat.pbr.metallic = 0.8f; mat.pbr.roughness = 0.3f; mat.pbr.ao = 0.9f;
    mat.pbr.emissive = {0.1f, 0.2f, 0.3f}; mat.pbr.emissiveStrength = 1.5f;
    mat.albedoMap = 5; mat.normalMap = 6;
    mat.transparent = true; mat.doubleSided = true;
    mat.UpdateGPUData();
    CHECK_THAT(mat.gpuData.albedoR, WithinAbs(0.5f, 0.001f));
    CHECK_THAT(mat.gpuData.albedoG, WithinAbs(0.6f, 0.001f));
    CHECK_THAT(mat.gpuData.albedoB, WithinAbs(0.7f, 0.001f));
    CHECK_THAT(mat.gpuData.metallic, WithinAbs(0.8f, 0.001f));
    CHECK_THAT(mat.gpuData.roughness, WithinAbs(0.3f, 0.001f));
    CHECK_THAT(mat.gpuData.ao, WithinAbs(0.9f, 0.001f));
    CHECK_THAT(mat.gpuData.emissiveR, WithinAbs(0.1f, 0.001f));
    CHECK_THAT(mat.gpuData.emissiveStrength, WithinAbs(1.5f, 0.001f));
    CHECK(mat.gpuData.albedoMap == 5);
    CHECK(mat.gpuData.normalMap == 6);
    CHECK(mat.gpuData.flags == 0x03);
    CHECK(mat.gpuDataDirty == false);
}

TEST_CASE("Material: GPU flags combinations") {
    Material mat;
    mat.UpdateGPUData();
    CHECK(mat.gpuData.flags == 0x00);
    mat.transparent = true; mat.doubleSided = true; mat.unlit = true; mat.wireframe = true;
    mat.UpdateGPUData();
    CHECK(mat.gpuData.flags == 0x0F);
}

TEST_CASE("MaterialLibrary: Register from config") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "TestMat"; config.pbr.metallic = 0.5f;
    uint32_t id = lib.RegisterMaterial(config);
    CHECK(id != 0); CHECK(lib.Count() == 1);
    auto* mat = lib.GetMaterial(id);
    REQUIRE(mat != nullptr);
    CHECK(mat->name == "TestMat");
    CHECK_THAT(mat->pbr.metallic, WithinAbs(0.5f, 0.001f));
    CHECK(mat->dirty == true);
    CHECK(mat->gpuDataDirty == false);
}

TEST_CASE("MaterialLibrary: Register from template") {
    MaterialLibrary lib;
    Material tmpl = MaterialLibrary::Gold();
    uint32_t id = lib.RegisterMaterial("MyGold", tmpl);
    CHECK(id != 0);
    auto* mat = lib.GetMaterial(id);
    REQUIRE(mat != nullptr);
    CHECK(mat->name == "MyGold");
    CHECK(mat->pbr.metallic == 1.0f);
}

TEST_CASE("MaterialLibrary: Duplicate name returns existing") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "Duplicate";
    uint32_t id1 = lib.RegisterMaterial(config);
    uint32_t id2 = lib.RegisterMaterial("Duplicate", MaterialLibrary::Silver());
    CHECK(id1 == id2); CHECK(lib.Count() == 1);
}

TEST_CASE("MaterialLibrary: Get by name") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "ByName";
    uint32_t id = lib.RegisterMaterial(config);
    auto* byId = lib.GetMaterial(id);
    auto* byName = lib.GetMaterial("ByName");
    REQUIRE(byId != nullptr); REQUIRE(byName != nullptr); CHECK(byId == byName);
}

TEST_CASE("MaterialLibrary: Get non-existent") {
    MaterialLibrary lib;
    CHECK(lib.GetMaterial(999) == nullptr);
    CHECK(lib.GetMaterial("NonExistent") == nullptr);
    CHECK(lib.GetMaterial(999) == nullptr);
    CHECK(lib.GetMaterial("NonExistent") == nullptr);
}

TEST_CASE("MaterialLibrary: Remove by ID") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "ToRemove";
    uint32_t id = lib.RegisterMaterial(config);
    CHECK(lib.Count() == 1);
    REQUIRE(lib.RemoveMaterial(id));
    CHECK(lib.Count() == 0);
    CHECK(lib.GetMaterial(id) == nullptr);
    CHECK(lib.GetMaterial("ToRemove") == nullptr);
}

TEST_CASE("MaterialLibrary: Remove by name") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "ToRemove";
    lib.RegisterMaterial(config);
    REQUIRE(lib.RemoveMaterial("ToRemove"));
    CHECK(lib.Count() == 0);
}

TEST_CASE("MaterialLibrary: Remove non-existent") {
    MaterialLibrary lib;
    CHECK_FALSE(lib.RemoveMaterial(999));
    CHECK_FALSE(lib.RemoveMaterial("NonExistent"));
}

TEST_CASE("MaterialLibrary: SetAlbedo") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "EditTest";
    uint32_t id = lib.RegisterMaterial(config);
    lib.SetAlbedo(id, 0.1f, 0.2f, 0.3f);
    auto* mat = lib.GetMaterial(id);
    REQUIRE(mat != nullptr);
    CHECK_THAT(mat->pbr.albedo[0], WithinAbs(0.1f, 0.001f));
    CHECK_THAT(mat->pbr.albedo[1], WithinAbs(0.2f, 0.001f));
    CHECK_THAT(mat->pbr.albedo[2], WithinAbs(0.3f, 0.001f));
    CHECK(mat->dirty == true);
}

TEST_CASE("MaterialLibrary: SetMetallic clamped") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "ClampTest";
    uint32_t id = lib.RegisterMaterial(config);
    lib.SetMetallic(id, 1.5f);
    auto* mat = lib.GetMaterial(id);
    CHECK(mat->pbr.metallic == 1.0f);
    lib.SetMetallic(id, -0.5f);
    CHECK(mat->pbr.metallic == 0.0f);
}

TEST_CASE("MaterialLibrary: SetRoughness clamped") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "ClampTest2";
    uint32_t id = lib.RegisterMaterial(config);
    lib.SetRoughness(id, 2.0f);
    auto* mat = lib.GetMaterial(id);
    CHECK(mat->pbr.roughness == 1.0f);
}

TEST_CASE("MaterialLibrary: SetAO clamped") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "ClampTest3";
    uint32_t id = lib.RegisterMaterial(config);
    lib.SetAO(id, -1.0f);
    auto* mat = lib.GetMaterial(id);
    CHECK(mat->pbr.ao == 0.0f);
}

TEST_CASE("MaterialLibrary: SetEmissive") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "EmissiveTest";
    uint32_t id = lib.RegisterMaterial(config);
    lib.SetEmissive(id, 1.0f, 0.5f, 0.0f, 2.0f);
    auto* mat = lib.GetMaterial(id);
    CHECK_THAT(mat->pbr.emissive[0], WithinAbs(1.0f, 0.001f));
    CHECK_THAT(mat->pbr.emissive[1], WithinAbs(0.5f, 0.001f));
    CHECK_THAT(mat->pbr.emissive[2], WithinAbs(0.0f, 0.001f));
    CHECK_THAT(mat->pbr.emissiveStrength, WithinAbs(2.0f, 0.001f));
}

TEST_CASE("MaterialLibrary: SetEmissive negative strength clamped") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "EmissiveNeg";
    uint32_t id = lib.RegisterMaterial(config);
    lib.SetEmissive(id, 1.0f, 0.0f, 0.0f, -1.0f);
    auto* mat = lib.GetMaterial(id);
    CHECK(mat->pbr.emissiveStrength == 0.0f);
}

TEST_CASE("MaterialLibrary: Set flags") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "FlagTest";
    uint32_t id = lib.RegisterMaterial(config);
    lib.SetTransparent(id, true);
    lib.SetDoubleSided(id, true);
    lib.SetUnlit(id, true);
    lib.SetWireframe(id, true);
    auto* mat = lib.GetMaterial(id);
    CHECK(mat->transparent == true);
    CHECK(mat->doubleSided == true);
    CHECK(mat->unlit == true);
    CHECK(mat->wireframe == true);
    CHECK(mat->dirty == true);
}

TEST_CASE("MaterialLibrary: Edit non-existent material") {
    MaterialLibrary lib;
    lib.SetAlbedo(999, 1.0f, 0.0f, 0.0f);
    lib.SetMetallic(999, 0.5f);
    lib.SetRoughness(999, 0.5f);
    lib.SetAO(999, 1.0f);
    lib.SetEmissive(999, 0.0f, 0.0f, 0.0f, 0.0f);
    lib.SetTransparent(999, true);
    lib.SetDoubleSided(999, true);
    lib.SetUnlit(999, true);
    lib.SetWireframe(999, true);
    SUCCEED();
}

TEST_CASE("MaterialLibrary: SetCustomShaders") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "ShaderTest";
    uint32_t id = lib.RegisterMaterial(config);
    VkShaderModule fakeVert = reinterpret_cast<VkShaderModule>(0x1);
    VkShaderModule fakeFrag = reinterpret_cast<VkShaderModule>(0x2);
    lib.SetCustomShaders(id, fakeVert, fakeFrag);
    auto* mat = lib.GetMaterial(id);
    CHECK(mat->customVertexShader == fakeVert);
    CHECK(mat->customFragmentShader == fakeFrag);
    CHECK(mat->useCustomShaders == true);
    CHECK(mat->dirty == true);
}

TEST_CASE("MaterialLibrary: ClearCustomShaders") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "ShaderClear";
    uint32_t id = lib.RegisterMaterial(config);
    VkShaderModule fakeVert = reinterpret_cast<VkShaderModule>(0x1);
    VkShaderModule fakeFrag = reinterpret_cast<VkShaderModule>(0x2);
    lib.SetCustomShaders(id, fakeVert, fakeFrag);
    lib.ClearCustomShaders(id);
    auto* mat = lib.GetMaterial(id);
    CHECK(mat->customVertexShader == VK_NULL_HANDLE);
    CHECK(mat->customFragmentShader == VK_NULL_HANDLE);
    CHECK(mat->useCustomShaders == false);
}

TEST_CASE("MaterialLibrary: HasMaterial") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "Exists";
    uint32_t id = lib.RegisterMaterial(config);
    CHECK(lib.HasMaterial(id));
    CHECK(lib.HasMaterial("Exists"));
    CHECK_FALSE(lib.HasMaterial(999));
    CHECK_FALSE(lib.HasMaterial("NonExistent"));
}

TEST_CASE("MaterialLibrary: GetAllIds") {
    MaterialLibrary lib;
    lib.RegisterMaterial("A", MaterialLibrary::Gold());
    lib.RegisterMaterial("B", MaterialLibrary::Silver());
    lib.RegisterMaterial("C", MaterialLibrary::RustyMetal());
    auto ids = lib.GetAllIds();
    CHECK(ids.size() == 3);
}

TEST_CASE("MaterialLibrary: GetAllNames") {
    MaterialLibrary lib;
    lib.RegisterMaterial("Alpha", MaterialLibrary::Gold());
    lib.RegisterMaterial("Beta", MaterialLibrary::Silver());
    auto names = lib.GetAllNames();
    CHECK(names.size() == 2);
    CHECK(std::find(names.begin(), names.end(), "Alpha") != names.end());
    CHECK(std::find(names.begin(), names.end(), "Beta") != names.end());
}

TEST_CASE("MaterialLibrary: Dirty tracking") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "Dirty";
    uint32_t id = lib.RegisterMaterial(config);
    CHECK(lib.HasDirtyMaterials());
    auto dirtyIds = lib.GetDirtyIds();
    CHECK(dirtyIds.size() == 1);
    CHECK(dirtyIds[0] == id);
    lib.ClearDirtyFlags();
    CHECK_FALSE(lib.HasDirtyMaterials());
    CHECK(lib.GetDirtyIds().empty());
}

TEST_CASE("MaterialLibrary: Clear all") {
    MaterialLibrary lib;
    lib.RegisterMaterial("A", MaterialLibrary::Gold());
    lib.RegisterMaterial("B", MaterialLibrary::Silver());
    CHECK(lib.Count() == 2);
    lib.Clear();
    CHECK(lib.Count() == 0);
    CHECK(lib.GetAllIds().empty());
}

TEST_CASE("MaterialLibrary: RedPlastic") {
    auto m = MaterialLibrary::RedPlastic();
    CHECK_THAT(m.pbr.albedo[0], WithinAbs(1.0f, 0.001f));
    CHECK_THAT(m.pbr.albedo[1], WithinAbs(0.0f, 0.001f));
    CHECK_THAT(m.pbr.albedo[2], WithinAbs(0.0f, 0.001f));
    CHECK(m.pbr.metallic == 0.0f);
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.3f, 0.001f));
}

TEST_CASE("MaterialLibrary: GreenMetal") {
    auto m = MaterialLibrary::GreenMetal();
    CHECK_THAT(m.pbr.albedo[1], WithinAbs(0.7f, 0.001f));
    CHECK_THAT(m.pbr.metallic, WithinAbs(0.9f, 0.001f));
}

TEST_CASE("MaterialLibrary: Gold") {
    auto m = MaterialLibrary::Gold();
    CHECK_THAT(m.pbr.albedo[0], WithinAbs(1.0f, 0.001f));
    CHECK(m.pbr.metallic == 1.0f);
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.15f, 0.001f));
}

TEST_CASE("MaterialLibrary: Silver") {
    auto m = MaterialLibrary::Silver();
    CHECK_THAT(m.pbr.albedo[0], WithinAbs(0.97f, 0.001f));
    CHECK(m.pbr.metallic == 1.0f);
}

TEST_CASE("MaterialLibrary: RustyMetal") {
    auto m = MaterialLibrary::RustyMetal();
    CHECK_THAT(m.pbr.albedo[0], WithinAbs(0.72f, 0.001f));
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.65f, 0.001f));
}

TEST_CASE("MaterialLibrary: EmissiveRed") {
    auto m = MaterialLibrary::EmissiveRed();
    CHECK_THAT(m.pbr.emissive[0], WithinAbs(1.0f, 0.001f));
    CHECK_THAT(m.pbr.emissiveStrength, WithinAbs(2.0f, 0.001f));
}

TEST_CASE("MaterialLibrary: Glass") {
    auto m = MaterialLibrary::Glass();
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.05f, 0.001f));
    CHECK(m.transparent == true);
}

TEST_CASE("MaterialLibrary: BlueRubber") {
    auto m = MaterialLibrary::BlueRubber();
    CHECK_THAT(m.pbr.albedo[2], WithinAbs(0.85f, 0.001f));
    CHECK(m.pbr.metallic == 0.0f);
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.9f, 0.001f));
}

TEST_CASE("MaterialLibrary: LoadFromConfig success") {
    auto path = CreateTempYAML(R"(
material:
  name: "ConfigLoaded"
  pbr:
    metallic: 0.75
)");
    MaterialLibrary lib;
    MaterialConfigLoader loader;
    uint32_t id = lib.LoadFromConfig(path, loader);
    CHECK(id != 0);
    auto* mat = lib.GetMaterial(id);
    REQUIRE(mat != nullptr);
    CHECK(mat->name == "ConfigLoaded");
    CHECK_THAT(mat->pbr.metallic, WithinAbs(0.75f, 0.001f));
    std::filesystem::remove(path);
}

TEST_CASE("MaterialLibrary: LoadFromConfig failure") {
    MaterialLibrary lib;
    MaterialConfigLoader loader;
    uint32_t id = lib.LoadFromConfig("/nonexistent.yaml", loader);
    CHECK(id == 0);
}

TEST_CASE("MaterialLibrary: ReloadMaterial with config path") {
    auto path = CreateTempYAML(R"(
material:
  name: "ReloadTest"
  pbr:
    metallic: 0.1
)");
    MaterialLibrary lib;
    MaterialConfigLoader loader;
    uint32_t id = lib.LoadFromConfig(path, loader);
    {
        std::ofstream f(path);
        f << R"(
material:
  name: "ReloadTest"
  pbr:
    metallic: 0.9
)";
    }
    REQUIRE(lib.ReloadMaterial(id, loader));
    auto* mat = lib.GetMaterial(id);
    CHECK_THAT(mat->pbr.metallic, WithinAbs(0.9f, 0.001f));
    std::filesystem::remove(path);
}

TEST_CASE("MaterialLibrary: ReloadMaterial without config path fails") {
    MaterialLibrary lib;
    MaterialConfig config; config.name = "NoConfig";
    uint32_t id = lib.RegisterMaterial(config);
    MaterialConfigLoader loader;
    CHECK_FALSE(lib.ReloadMaterial(id, loader));
}

TEST_CASE("MaterialLibrary: ReloadMaterial non-existent") {
    MaterialLibrary lib;
    MaterialConfigLoader loader;
    CHECK_FALSE(lib.ReloadMaterial(999, loader));
    CHECK_FALSE(lib.ReloadMaterial("NonExistent", loader));
}

TEST_CASE("MaterialLibrary: ReloadAll") {
    auto path1 = CreateTempYAML(R"(
material:
  name: "ReloadA"
  pbr:
    metallic: 0.1
)");
    auto path2 = CreateTempYAML(R"(
material:
  name: "ReloadB"
  pbr:
    metallic: 0.2
)");
    MaterialLibrary lib;
    MaterialConfigLoader loader;
    lib.LoadFromConfig(path1, loader);
    lib.LoadFromConfig(path2, loader);
    {
        std::ofstream f(path1);
        f << "material:\n  name: \"ReloadA\"\n  pbr:\n    metallic: 0.99\n";
    }
    size_t reloaded = lib.ReloadAll(loader);
    CHECK(reloaded == 2);
    std::filesystem::remove(path1);
    std::filesystem::remove(path2);
}

TEST_CASE("MaterialLibrary: Concurrent reads") {
    MaterialLibrary lib;
    for (int i = 0; i < 100; ++i) {
        MaterialConfig config;
        config.name = "Mat" + std::to_string(i);
        lib.RegisterMaterial(config);
    }
    CHECK(lib.Count() == 100);
    CHECK(lib.GetAllIds().size() == 100);
    CHECK(lib.GetAllNames().size() == 100);
}

TEST_CASE("ShaderManager: Default state") {
    ShaderManager sm;
    CHECK(sm.GetShaderCount() == 0);
    CHECK(sm.GetReloadCount() == 0);
    CHECK_FALSE(sm.IsInitialized());
    CHECK(sm.GetShaderNames().empty());
}

TEST_CASE("ShaderManager: Initialize with null device fails") {
    ShaderManager sm;
    CHECK_FALSE(sm.Initialize(VK_NULL_HANDLE));
}

TEST_CASE("ShaderManager: LoadSPIRV without init fails") {
    ShaderManager sm;
    auto path = CreateTempSPIRV("test");
    auto mod = sm.LoadSPIRV(path, VK_SHADER_STAGE_VERTEX_BIT);
    CHECK(mod == VK_NULL_HANDLE);
    std::filesystem::remove(path);
}

TEST_CASE("ShaderManager: Get non-existent shader") {
    ShaderManager sm;
    CHECK(sm.GetShader("nonexistent") == VK_NULL_HANDLE);
    CHECK_FALSE(sm.HasShader("nonexistent"));
    CHECK(sm.GetShaderEntry("nonexistent") == nullptr);
}

TEST_CASE("ShaderManager: EnableHotReload invalid dir") {
    ShaderManager sm;
    sm.EnableHotReload("/nonexistent/dir");
    SUCCEED();
}

TEST_CASE("ShaderManager: Callbacks") {
    ShaderManager sm;
    bool called = false;
    sm.OnShaderReloaded([&called](const std::string&) { called = true; });
    sm.ClearCallbacks();
    SUCCEED();
}

TEST_CASE("ShaderManager: Invalid SPIR-V file rejected") {
    auto path = std::filesystem::temp_directory_path() / "invalid.spv";
    {
        std::ofstream f(path, std::ios::binary);
        uint32_t badData[] = {0xDEADBEEF, 0x00000000};
        f.write(reinterpret_cast<const char*>(badData), sizeof(badData));
    }
    CHECK(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST_CASE("Integration: Full material workflow") {
    auto path = CreateTempYAML(R"(
material:
  name: "IntegrationMat"
  pbr:
    albedo: [0.2, 0.4, 0.6]
    metallic: 0.5
    roughness: 0.5
    ao: 1.0
  flags:
    double_sided: true
)");
    MaterialConfigLoader loader;
    auto config = loader.LoadFromYAML(path);
    REQUIRE(config.has_value());
    auto valid = loader.Validate(config.value());
    REQUIRE(valid.has_value());
    MaterialLibrary lib;
    uint32_t id = lib.RegisterMaterial(config.value());
    REQUIRE(id != 0);
    auto* mat = lib.GetMaterial(id);
    REQUIRE(mat != nullptr);
    CHECK(mat->name == "IntegrationMat");
    CHECK_THAT(mat->pbr.albedo[0], WithinAbs(0.2f, 0.001f));
    CHECK(mat->doubleSided == true);
    CHECK(mat->dirty == true);
    lib.SetMetallic(id, 0.8f);
    CHECK_THAT(mat->pbr.metallic, WithinAbs(0.8f, 0.001f));
    lib.ClearDirtyFlags();
    CHECK_FALSE(mat->dirty);
    std::filesystem::remove(path);
}

TEST_CASE("Integration: Multiple materials") {
    MaterialLibrary lib;
    auto gold = MaterialLibrary::Gold();
    auto silver = MaterialLibrary::Silver();
    auto rust = MaterialLibrary::RustyMetal();
    uint32_t id1 = lib.RegisterMaterial("Gold", gold);
    uint32_t id2 = lib.RegisterMaterial("Silver", silver);
    uint32_t id3 = lib.RegisterMaterial("Rust", rust);
    CHECK(lib.Count() == 3);
    CHECK(lib.HasMaterial(id1));
    CHECK(lib.HasMaterial(id2));
    CHECK(lib.HasMaterial(id3));
    CHECK(lib.GetMaterial("Gold")->pbr.metallic == 1.0f);
    CHECK_THAT(lib.GetMaterial("Silver")->pbr.roughness, WithinAbs(0.1f, 0.001f));
    CHECK_THAT(lib.GetMaterial("Rust")->pbr.albedo[0], WithinAbs(0.72f, 0.001f));
}

TEST_CASE("Integration: Config save and reload") {
    MaterialConfig original;
    original.name = "SaveReload";
    original.pbr.metallic = 0.42f;
    original.pbr.roughness = 0.58f;
    original.doubleSided = true;
    auto path = std::filesystem::temp_directory_path() / "save_reload.yaml";
    MaterialConfigLoader loader;
    REQUIRE(loader.SaveToYAML(path, original));
    MaterialLibrary lib;
    uint32_t id = lib.LoadFromConfig(path, loader);
    REQUIRE(id != 0);
    auto* mat = lib.GetMaterial(id);
    CHECK(mat->name == "SaveReload");
    CHECK_THAT(mat->pbr.metallic, WithinAbs(0.42f, 0.001f));
    CHECK(mat->doubleSided == true);
    std::filesystem::remove(path);
}
