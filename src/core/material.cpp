#include "core/material.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace TheSeed::Render {

void Material::UpdateGPUData() {
    gpuData.albedoR = pbr.albedo[0]; gpuData.albedoG = pbr.albedo[1]; gpuData.albedoB = pbr.albedo[2];
    gpuData.metallic = pbr.metallic; gpuData.roughness = pbr.roughness; gpuData.ao = pbr.ao;
    gpuData.emissiveR = pbr.emissive[0]; gpuData.emissiveG = pbr.emissive[1]; gpuData.emissiveB = pbr.emissive[2];
    gpuData.emissiveStrength = pbr.emissiveStrength;
    gpuData.albedoMap = albedoMap; gpuData.normalMap = normalMap; gpuData.metallicMap = metallicMap;
    gpuData.roughnessMap = roughnessMap; gpuData.aoMap = aoMap; gpuData.emissiveMap = emissiveMap;
    uint32_t flags = 0;
    if (transparent) flags |= 0x01;
    if (doubleSided) flags |= 0x02;
    if (unlit) flags |= 0x04;
    if (wireframe) flags |= 0x08;
    gpuData.flags = flags;
    gpuDataDirty = false;
}

MaterialLibrary::MaterialLibrary() = default;
MaterialLibrary::~MaterialLibrary() = default;

uint32_t MaterialLibrary::AllocateId() { return m_nextId++; }

void MaterialLibrary::MarkDirty(uint32_t id) {
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->dirty = true; it->second->gpuDataDirty = true; }
}

uint32_t MaterialLibrary::RegisterMaterial(const MaterialConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t id = AllocateId();
    auto mat = std::make_unique<Material>();
    mat->id = id; mat->name = config.name; mat->pbr = config.pbr;
    mat->transparent = config.transparent; mat->doubleSided = config.doubleSided;
    mat->unlit = config.unlit; mat->wireframe = config.wireframe;
    mat->UpdateGPUData();
    m_materials[id] = std::move(mat);
    m_nameToId[config.name] = id;
    spdlog::debug("MaterialLibrary: Registered '{}' (ID={})", config.name, id);
    return id;
}

uint32_t MaterialLibrary::RegisterMaterial(const std::string& name, const Material& matTemplate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end()) {
        spdlog::warn("MaterialLibrary: '{}' already exists, returning ID={}", name, it->second);
        return it->second;
    }
    uint32_t id = AllocateId();
    auto mat = std::make_unique<Material>(matTemplate);
    mat->id = id; mat->name = name; mat->UpdateGPUData();
    m_materials[id] = std::move(mat);
    m_nameToId[name] = id;
    spdlog::debug("MaterialLibrary: Registered '{}' (ID={})", name, id);
    return id;
}

uint32_t MaterialLibrary::LoadFromConfig(const std::filesystem::path& configPath, MaterialConfigLoader& loader) {
    auto config = loader.LoadFromYAML(configPath);
    if (!config.has_value()) { spdlog::error("MaterialLibrary: Failed to load config: {}", config.error()); return 0; }
    auto id = RegisterMaterial(config.value());
    if (id != 0) { std::lock_guard<std::mutex> lock(m_mutex); auto it = m_materials.find(id); if (it != m_materials.end()) it->second->configPath = configPath; }
    return id;
}

Material* MaterialLibrary::GetMaterial(uint32_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    return (it != m_materials.end()) ? it->second.get() : nullptr;
}

Material* MaterialLibrary::GetMaterial(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nameToId.find(name);
    return (it != m_nameToId.end()) ? m_materials[it->second].get() : nullptr;
}

const Material* MaterialLibrary::GetMaterial(uint32_t id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    return (it != m_materials.end()) ? it->second.get() : nullptr;
}

const Material* MaterialLibrary::GetMaterial(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nameToId.find(name);
    return (it != m_nameToId.end()) ? m_materials.at(it->second).get() : nullptr;
}

bool MaterialLibrary::RemoveMaterial(uint32_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it == m_materials.end()) return false;
    std::string name = it->second->name;
    m_materials.erase(it); m_nameToId.erase(name);
    spdlog::debug("MaterialLibrary: Removed '{}' (ID={})", name, id);
    return true;
}

bool MaterialLibrary::RemoveMaterial(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nameToId.find(name);
    if (it == m_nameToId.end()) return false;
    uint32_t id = it->second;
    auto mat_it = m_materials.find(id);
    if (mat_it == m_materials.end()) return false;
    m_materials.erase(mat_it);
    m_nameToId.erase(it);
    spdlog::debug("MaterialLibrary: Removed '{}' (ID={})", name, id);
    return true;
}

void MaterialLibrary::SetAlbedo(uint32_t id, float r, float g, float b) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->pbr.albedo = {r, g, b}; MarkDirty(id); }
}

void MaterialLibrary::SetMetallic(uint32_t id, float v) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->pbr.metallic = std::clamp(v, 0.0f, 1.0f); MarkDirty(id); }
}

void MaterialLibrary::SetRoughness(uint32_t id, float v) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->pbr.roughness = std::clamp(v, 0.0f, 1.0f); MarkDirty(id); }
}

void MaterialLibrary::SetAO(uint32_t id, float v) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->pbr.ao = std::clamp(v, 0.0f, 1.0f); MarkDirty(id); }
}

void MaterialLibrary::SetEmissive(uint32_t id, float r, float g, float b, float strength) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) {
        it->second->pbr.emissive = {r, g, b};
        it->second->pbr.emissiveStrength = std::max(0.0f, strength);
        MarkDirty(id);
    }
}

void MaterialLibrary::SetTransparent(uint32_t id, bool v) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->transparent = v; MarkDirty(id); }
}

void MaterialLibrary::SetDoubleSided(uint32_t id, bool v) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->doubleSided = v; MarkDirty(id); }
}

void MaterialLibrary::SetUnlit(uint32_t id, bool v) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->unlit = v; MarkDirty(id); }
}

void MaterialLibrary::SetWireframe(uint32_t id, bool v) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) { it->second->wireframe = v; MarkDirty(id); }
}

void MaterialLibrary::SetCustomShaders(uint32_t id, VkShaderModule vert, VkShaderModule frag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) {
        it->second->customVertexShader = vert; it->second->customFragmentShader = frag;
        it->second->useCustomShaders = (vert != VK_NULL_HANDLE && frag != VK_NULL_HANDLE);
        MarkDirty(id);
    }
}

void MaterialLibrary::ClearCustomShaders(uint32_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it != m_materials.end()) {
        it->second->customVertexShader = VK_NULL_HANDLE; it->second->customFragmentShader = VK_NULL_HANDLE;
        it->second->useCustomShaders = false; MarkDirty(id);
    }
}

bool MaterialLibrary::ReloadMaterial(uint32_t id, MaterialConfigLoader& loader) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_materials.find(id);
    if (it == m_materials.end() || it->second->configPath.empty()) return false;
    auto config = loader.LoadFromYAML(it->second->configPath);
    if (!config.has_value()) { spdlog::error("MaterialLibrary: Failed to reload {}: {}", id, config.error()); return false; }
    auto& mat = it->second;
    mat->name = config->name; mat->pbr = config->pbr;
    mat->transparent = config->transparent; mat->doubleSided = config->doubleSided;
    mat->unlit = config->unlit; mat->wireframe = config->wireframe;
    mat->UpdateGPUData(); mat->dirty = true;
    spdlog::info("MaterialLibrary: Reloaded '{}' (ID={})", mat->name, id);
    return true;
}

bool MaterialLibrary::ReloadMaterial(const std::string& name, MaterialConfigLoader& loader) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nameToId.find(name);
    if (it == m_nameToId.end()) return false;
    uint32_t id = it->second;
    auto mat_it = m_materials.find(id);
    if (mat_it == m_materials.end() || mat_it->second->configPath.empty()) return false;
    auto config = loader.LoadFromYAML(mat_it->second->configPath);
    if (!config.has_value()) { spdlog::error("MaterialLibrary: Failed to reload {}: {}", id, config.error()); return false; }
    auto& mat = mat_it->second;
    mat->name = config->name; mat->pbr = config->pbr;
    mat->transparent = config->transparent; mat->doubleSided = config->doubleSided;
    mat->unlit = config->unlit; mat->wireframe = config->wireframe;
    mat->UpdateGPUData(); mat->dirty = true;
    spdlog::info("MaterialLibrary: Reloaded '{}' (ID={})", mat->name, id);
    return true;
}

size_t MaterialLibrary::ReloadAll(MaterialConfigLoader& loader) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t reloaded = 0;
    for (auto& [id, mat] : m_materials) {
        if (!mat->configPath.empty()) {
            auto config = loader.LoadFromYAML(mat->configPath);
            if (config.has_value()) {
                mat->name = config->name; mat->pbr = config->pbr;
                mat->transparent = config->transparent; mat->doubleSided = config->doubleSided;
                mat->unlit = config->unlit; mat->wireframe = config->wireframe;
                mat->UpdateGPUData(); mat->dirty = true;
                reloaded++;
                spdlog::info("MaterialLibrary: Reloaded '{}' (ID={})", mat->name, id);
            }
        }
    }
    return reloaded;
}

size_t MaterialLibrary::Count() const { std::lock_guard<std::mutex> lock(m_mutex); return m_materials.size(); }

bool MaterialLibrary::HasMaterial(uint32_t id) const { std::lock_guard<std::mutex> lock(m_mutex); return m_materials.contains(id); }

bool MaterialLibrary::HasMaterial(const std::string& name) const { std::lock_guard<std::mutex> lock(m_mutex); return m_nameToId.contains(name); }

std::vector<uint32_t> MaterialLibrary::GetAllIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<uint32_t> ids; ids.reserve(m_materials.size());
    for (const auto& [id, _] : m_materials) ids.push_back(id);
    return ids;
}

std::vector<std::string> MaterialLibrary::GetAllNames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names; names.reserve(m_nameToId.size());
    for (const auto& [name, _] : m_nameToId) names.push_back(name);
    return names;
}

void MaterialLibrary::Clear() { std::lock_guard<std::mutex> lock(m_mutex); m_materials.clear(); m_nameToId.clear(); m_nextId = 1; }

bool MaterialLibrary::HasDirtyMaterials() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [_, mat] : m_materials) if (mat->dirty) return true;
    return false;
}

std::vector<uint32_t> MaterialLibrary::GetDirtyIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<uint32_t> ids;
    for (const auto& [id, mat] : m_materials) if (mat->dirty) ids.push_back(id);
    return ids;
}

void MaterialLibrary::ClearDirtyFlags() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [_, mat] : m_materials) mat->dirty = false;
}

Material MaterialLibrary::RedPlastic() {
    Material m; m.pbr.albedo = {1.0f, 0.0f, 0.0f}; m.pbr.metallic = 0.0f; m.pbr.roughness = 0.3f; m.pbr.ao = 1.0f; return m;
}

Material MaterialLibrary::GreenMetal() {
    Material m; m.pbr.albedo = {0.1f, 0.7f, 0.2f}; m.pbr.metallic = 0.9f; m.pbr.roughness = 0.2f; m.pbr.ao = 1.0f; return m;
}

Material MaterialLibrary::BlueRubber() {
    Material m; m.pbr.albedo = {0.1f, 0.3f, 0.85f}; m.pbr.metallic = 0.0f; m.pbr.roughness = 0.9f; m.pbr.ao = 1.0f; return m;
}

Material MaterialLibrary::Gold() {
    Material m; m.pbr.albedo = {1.0f, 0.78f, 0.34f}; m.pbr.metallic = 1.0f; m.pbr.roughness = 0.15f; m.pbr.ao = 1.0f; return m;
}

Material MaterialLibrary::Silver() {
    Material m; m.pbr.albedo = {0.97f, 0.96f, 0.91f}; m.pbr.metallic = 1.0f; m.pbr.roughness = 0.1f; m.pbr.ao = 1.0f; return m;
}

Material MaterialLibrary::RustyMetal() {
    Material m; m.pbr.albedo = {0.72f, 0.45f, 0.20f}; m.pbr.metallic = 0.95f; m.pbr.roughness = 0.65f; m.pbr.ao = 1.0f; return m;
}

Material MaterialLibrary::EmissiveRed() {
    Material m; m.pbr.albedo = {0.1f, 0.05f, 0.05f}; m.pbr.metallic = 0.0f; m.pbr.roughness = 0.5f;
    m.pbr.ao = 1.0f; m.pbr.emissive = {1.0f, 0.1f, 0.1f}; m.pbr.emissiveStrength = 2.0f; return m;
}

Material MaterialLibrary::Glass() {
    Material m; m.pbr.albedo = {0.95f, 0.95f, 0.95f}; m.pbr.metallic = 0.0f; m.pbr.roughness = 0.05f;
    m.pbr.ao = 1.0f; m.transparent = true; return m;
}

} // namespace TheSeed::Render
