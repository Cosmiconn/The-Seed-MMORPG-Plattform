#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <expected>
#include <array>

namespace TheSeed::Render {

struct PBRParameters {
    std::array<float, 3> albedo = {1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    std::array<float, 3> emissive = {0.0f, 0.0f, 0.0f};
    float emissiveStrength = 0.0f;
};

struct MaterialConfig {
    std::string name;
    PBRParameters pbr;
    std::string albedoMapPath;
    std::string normalMapPath;
    std::string metallicMapPath;
    std::string roughnessMapPath;
    std::string aoMapPath;
    std::string emissiveMapPath;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    bool transparent = false;
    bool doubleSided = false;
    bool unlit = false;
    bool wireframe = false;
    bool operator==(const MaterialConfig& other) const;
    bool operator!=(const MaterialConfig& other) const { return !(*this == other); }
};

class MaterialConfigLoader {
public:
    MaterialConfigLoader();
    ~MaterialConfigLoader();
    std::expected<MaterialConfig, std::string> LoadFromYAML(const std::filesystem::path& path);
    std::expected<MaterialConfig, std::string> LoadFromJSON(const std::filesystem::path& path);
    bool SaveToYAML(const std::filesystem::path& path, const MaterialConfig& config);
    bool SaveToJSON(const std::filesystem::path& path, const MaterialConfig& config);
    std::expected<void, std::string> Validate(const MaterialConfig& config) const;
    void SetAssetRoot(const std::filesystem::path& root);
private:
    std::filesystem::path m_assetRoot;
    std::expected<MaterialConfig, std::string> ParseYAML(const std::string& content, const std::filesystem::path& sourcePath);
    std::expected<MaterialConfig, std::string> ParseJSON(const std::string& content, const std::filesystem::path& sourcePath);
    std::string SerializeYAML(const MaterialConfig& config);
    std::string SerializeJSON(const MaterialConfig& config);
    std::filesystem::path ResolvePath(const std::filesystem::path& relative) const;
};

} // namespace TheSeed::Render
