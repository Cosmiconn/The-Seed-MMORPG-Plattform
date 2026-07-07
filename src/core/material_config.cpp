#include "core/material_config.hpp"
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace TheSeed::Render {

bool MaterialConfig::operator==(const MaterialConfig& other) const {
    return name == other.name && pbr.albedo == other.pbr.albedo &&
           pbr.metallic == other.pbr.metallic && pbr.roughness == other.pbr.roughness &&
           pbr.ao == other.pbr.ao && pbr.emissive == other.pbr.emissive &&
           pbr.emissiveStrength == other.pbr.emissiveStrength &&
           albedoMapPath == other.albedoMapPath && normalMapPath == other.normalMapPath &&
           metallicMapPath == other.metallicMapPath && roughnessMapPath == other.roughnessMapPath &&
           aoMapPath == other.aoMapPath && emissiveMapPath == other.emissiveMapPath &&
           vertexShaderPath == other.vertexShaderPath && fragmentShaderPath == other.fragmentShaderPath &&
           transparent == other.transparent && doubleSided == other.doubleSided &&
           unlit == other.unlit && wireframe == other.wireframe;
}

MaterialConfigLoader::MaterialConfigLoader() = default;
MaterialConfigLoader::~MaterialConfigLoader() = default;

void MaterialConfigLoader::SetAssetRoot(const std::filesystem::path& root) { m_assetRoot = root; }

std::filesystem::path MaterialConfigLoader::ResolvePath(const std::filesystem::path& relative) const {
    if (relative.empty()) return {};
    if (relative.is_absolute()) return relative;
    if (!m_assetRoot.empty()) {
        auto resolved = m_assetRoot / relative;
        if (std::filesystem::exists(resolved)) return resolved;
    }
    return relative;
}

std::expected<MaterialConfig, std::string> MaterialConfigLoader::LoadFromYAML(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::unexpected("Failed to open YAML file: " + path.string());
    std::stringstream buffer; buffer << file.rdbuf();
    return ParseYAML(buffer.str(), path);
}

std::expected<MaterialConfig, std::string> MaterialConfigLoader::LoadFromJSON(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::unexpected("Failed to open JSON file: " + path.string());
    std::stringstream buffer; buffer << file.rdbuf();
    return ParseJSON(buffer.str(), path);
}

std::expected<MaterialConfig, std::string> MaterialConfigLoader::ParseYAML(const std::string& content, const std::filesystem::path& sourcePath) {
    try {
        YAML::Node root = YAML::Load(content);
        if (!root["material"]) return std::unexpected("Missing 'material' root node");
        YAML::Node mat = root["material"];
        MaterialConfig config;
        config.name = mat["name"] ? mat["name"].as<std::string>() : sourcePath.stem().string();
        if (mat["pbr"]) {
            YAML::Node pbr = mat["pbr"];
            if (pbr["albedo"] && pbr["albedo"].IsSequence() && pbr["albedo"].size() >= 3) {
                config.pbr.albedo[0] = pbr["albedo"][0].as<float>();
                config.pbr.albedo[1] = pbr["albedo"][1].as<float>();
                config.pbr.albedo[2] = pbr["albedo"][2].as<float>();
            }
            if (pbr["metallic"]) config.pbr.metallic = pbr["metallic"].as<float>();
            if (pbr["roughness"]) config.pbr.roughness = pbr["roughness"].as<float>();
            if (pbr["ao"]) config.pbr.ao = pbr["ao"].as<float>();
            if (pbr["emissive"] && pbr["emissive"].IsSequence() && pbr["emissive"].size() >= 3) {
                config.pbr.emissive[0] = pbr["emissive"][0].as<float>();
                config.pbr.emissive[1] = pbr["emissive"][1].as<float>();
                config.pbr.emissive[2] = pbr["emissive"][2].as<float>();
            }
            if (pbr["emissive_strength"]) config.pbr.emissiveStrength = pbr["emissive_strength"].as<float>();
        }
        if (mat["textures"]) {
            YAML::Node tex = mat["textures"];
            if (tex["albedo"]) config.albedoMapPath = tex["albedo"].as<std::string>();
            if (tex["normal"]) config.normalMapPath = tex["normal"].as<std::string>();
            if (tex["metallic"]) config.metallicMapPath = tex["metallic"].as<std::string>();
            if (tex["roughness"]) config.roughnessMapPath = tex["roughness"].as<std::string>();
            if (tex["ao"]) config.aoMapPath = tex["ao"].as<std::string>();
            if (tex["emissive"]) config.emissiveMapPath = tex["emissive"].as<std::string>();
        }
        if (mat["shaders"]) {
            YAML::Node sh = mat["shaders"];
            if (sh["vertex"]) config.vertexShaderPath = sh["vertex"].as<std::string>();
            if (sh["fragment"]) config.fragmentShaderPath = sh["fragment"].as<std::string>();
        }
        if (mat["flags"]) {
            YAML::Node flags = mat["flags"];
            if (flags["transparent"]) config.transparent = flags["transparent"].as<bool>();
            if (flags["double_sided"]) config.doubleSided = flags["double_sided"].as<bool>();
            if (flags["unlit"]) config.unlit = flags["unlit"].as<bool>();
            if (flags["wireframe"]) config.wireframe = flags["wireframe"].as<bool>();
        }
        return config;
    } catch (const YAML::Exception& e) { return std::unexpected(std::string("YAML parse error: ") + e.what()); }
    catch (const std::exception& e) { return std::unexpected(std::string("Parse error: ") + e.what()); }
}

std::expected<MaterialConfig, std::string> MaterialConfigLoader::ParseJSON(const std::string& content, const std::filesystem::path& sourcePath) {
    return ParseYAML(content, sourcePath);
}

bool MaterialConfigLoader::SaveToYAML(const std::filesystem::path& path, const MaterialConfig& config) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) { spdlog::error("Failed to open file for writing: {}", path.string()); return false; }
        file << SerializeYAML(config); return true;
    } catch (const std::exception& e) { spdlog::error("Failed to save YAML: {}", e.what()); return false; }
}

bool MaterialConfigLoader::SaveToJSON(const std::filesystem::path& path, const MaterialConfig& config) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) { spdlog::error("Failed to open file for writing: {}", path.string()); return false; }
        file << SerializeJSON(config); return true;
    } catch (const std::exception& e) { spdlog::error("Failed to save JSON: {}", e.what()); return false; }
}

std::string MaterialConfigLoader::SerializeYAML(const MaterialConfig& config) {
    std::ostringstream oss;
    oss << "material:\n";
    oss << "  name: \"" << config.name << "\"\n";
    oss << "  pbr:\n";
    oss << "    albedo: [" << config.pbr.albedo[0] << ", " << config.pbr.albedo[1] << ", " << config.pbr.albedo[2] << "]\n";
    oss << "    metallic: " << config.pbr.metallic << "\n";
    oss << "    roughness: " << config.pbr.roughness << "\n";
    oss << "    ao: " << config.pbr.ao << "\n";
    oss << "    emissive: [" << config.pbr.emissive[0] << ", " << config.pbr.emissive[1] << ", " << config.pbr.emissive[2] << "]\n";
    oss << "    emissive_strength: " << config.pbr.emissiveStrength << "\n";
    oss << "  textures:\n";
    if (!config.albedoMapPath.empty()) oss << "    albedo: \"" << config.albedoMapPath << "\"\n";
    if (!config.normalMapPath.empty()) oss << "    normal: \"" << config.normalMapPath << "\"\n";
    if (!config.metallicMapPath.empty()) oss << "    metallic: \"" << config.metallicMapPath << "\"\n";
    if (!config.roughnessMapPath.empty()) oss << "    roughness: \"" << config.roughnessMapPath << "\"\n";
    if (!config.aoMapPath.empty()) oss << "    ao: \"" << config.aoMapPath << "\"\n";
    if (!config.emissiveMapPath.empty()) oss << "    emissive: \"" << config.emissiveMapPath << "\"\n";
    if (!config.vertexShaderPath.empty() || !config.fragmentShaderPath.empty()) {
        oss << "  shaders:\n";
        if (!config.vertexShaderPath.empty()) oss << "    vertex: \"" << config.vertexShaderPath << "\"\n";
        if (!config.fragmentShaderPath.empty()) oss << "    fragment: \"" << config.fragmentShaderPath << "\"\n";
    }
    oss << "  flags:\n";
    oss << "    transparent: " << (config.transparent ? "true" : "false") << "\n";
    oss << "    double_sided: " << (config.doubleSided ? "true" : "false") << "\n";
    oss << "    unlit: " << (config.unlit ? "true" : "false") << "\n";
    oss << "    wireframe: " << (config.wireframe ? "true" : "false") << "\n";
    return oss.str();
}

std::string MaterialConfigLoader::SerializeJSON(const MaterialConfig& config) {
    std::ostringstream oss;
    oss << "{\n  \"material\": {\n";
    oss << "    \"name\": \"" << config.name << "\",\n";
    oss << "    \"pbr\": {\n";
    oss << "      \"albedo\": [" << config.pbr.albedo[0] << ", " << config.pbr.albedo[1] << ", " << config.pbr.albedo[2] << "],\n";
    oss << "      \"metallic\": " << config.pbr.metallic << ",\n";
    oss << "      \"roughness\": " << config.pbr.roughness << ",\n";
    oss << "      \"ao\": " << config.pbr.ao << ",\n";
    oss << "      \"emissive\": [" << config.pbr.emissive[0] << ", " << config.pbr.emissive[1] << ", " << config.pbr.emissive[2] << "],\n";
    oss << "      \"emissive_strength\": " << config.pbr.emissiveStrength << "\n";
    oss << "    },\n    \"textures\": {\n";
    bool first = true;
    auto wt = [&](const char* k, const std::string& v) { if (!v.empty()) { if (!first) oss << ",\n"; oss << "      \"" << k << "\": \"" << v << "\""; first = false; } };
    wt("albedo", config.albedoMapPath); wt("normal", config.normalMapPath); wt("metallic", config.metallicMapPath);
    wt("roughness", config.roughnessMapPath); wt("ao", config.aoMapPath); wt("emissive", config.emissiveMapPath);
    oss << "\n    },\n    \"flags\": {\n";
    oss << "      \"transparent\": " << (config.transparent ? "true" : "false") << ",\n";
    oss << "      \"double_sided\": " << (config.doubleSided ? "true" : "false") << ",\n";
    oss << "      \"unlit\": " << (config.unlit ? "true" : "false") << ",\n";
    oss << "      \"wireframe\": " << (config.wireframe ? "true" : "false") << "\n";
    oss << "    }\n  }\n}\n";
    return oss.str();
}

std::expected<void, std::string> MaterialConfigLoader::Validate(const MaterialConfig& config) const {
    if (config.name.empty()) return std::unexpected("Material name is required");
    if (config.pbr.metallic < 0.0f || config.pbr.metallic > 1.0f) return std::unexpected("Metallic must be in range [0, 1]");
    if (config.pbr.roughness < 0.0f || config.pbr.roughness > 1.0f) return std::unexpected("Roughness must be in range [0, 1]");
    if (config.pbr.ao < 0.0f || config.pbr.ao > 1.0f) return std::unexpected("AO must be in range [0, 1]");
    if (config.pbr.emissiveStrength < 0.0f) return std::unexpected("Emissive strength must be >= 0");
    auto cp = [&](const std::string& p, const char* n) -> std::expected<void, std::string> {
        if (p.empty()) return {};
        auto r = ResolvePath(p);
        if (!r.empty() && !std::filesystem::exists(r)) return std::unexpected(std::string("Texture not found: ") + n + " = " + p);
        return {};
    };
    if (auto r = cp(config.albedoMapPath, "albedo"); !r) return r;
    if (auto r = cp(config.normalMapPath, "normal"); !r) return r;
    if (auto r = cp(config.metallicMapPath, "metallic"); !r) return r;
    if (auto r = cp(config.roughnessMapPath, "roughness"); !r) return r;
    if (auto r = cp(config.aoMapPath, "ao"); !r) return r;
    if (auto r = cp(config.emissiveMapPath, "emissive"); !r) return r;
    return {};
}

} // namespace TheSeed::Render
