#include "core/material.hpp"

namespace TheSeed::Render {

uint32_t MaterialLibrary::RegisterMaterial(const std::string& name, const Material& mat) {
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end()) {
        *m_materials[it->second] = mat;
        return it->second;
    }

    uint32_t id = m_nextId++;
    m_nameToId[name] = id;
    m_materials[id] = std::make_unique<Material>(mat);
    return id;
}

Material* MaterialLibrary::GetMaterial(uint32_t id) {
    auto it = m_materials.find(id);
    if (it != m_materials.end()) return it->second.get();
    return nullptr;
}

Material* MaterialLibrary::GetMaterial(const std::string& name) {
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end()) return GetMaterial(it->second);
    return nullptr;
}

void MaterialLibrary::UpdateMaterial(uint32_t id, const Material& mat) {
    auto it = m_materials.find(id);
    if (it != m_materials.end()) {
        *it->second = mat;
    }
}

void MaterialLibrary::RemoveMaterial(uint32_t id) {
    auto it = m_materials.find(id);
    if (it != m_materials.end()) {
        for (auto& [name, mid] : m_nameToId) {
            if (mid == id) {
                m_nameToId.erase(name);
                break;
            }
        }
        m_materials.erase(it);
    }
}

void MaterialLibrary::Clear() {
    m_nameToId.clear();
    m_materials.clear();
    m_nextId = 1;
}

// --- Predefined Materials ---

Material Materials::RedPlastic() {
    Material m;
    m.albedoR = 1.0f; m.albedoG = 0.1f; m.albedoB = 0.1f;
    m.metallic = 0.0f;
    m.roughness = 0.3f;
    m.ao = 1.0f;
    return m;
}

Material Materials::GreenMetal() {
    Material m;
    m.albedoR = 0.1f; m.albedoG = 1.0f; m.albedoB = 0.1f;
    m.metallic = 0.8f;
    m.roughness = 0.2f;
    m.ao = 1.0f;
    return m;
}

Material Materials::BlueRubber() {
    Material m;
    m.albedoR = 0.1f; m.albedoG = 0.1f; m.albedoB = 1.0f;
    m.metallic = 0.0f;
    m.roughness = 0.9f;
    m.ao = 1.0f;
    return m;
}

Material Materials::Gold() {
    Material m;
    m.albedoR = 1.0f; m.albedoG = 0.78f; m.albedoB = 0.34f;
    m.metallic = 1.0f;
    m.roughness = 0.15f;
    m.ao = 1.0f;
    return m;
}

Material Materials::Silver() {
    Material m;
    m.albedoR = 0.97f; m.albedoG = 0.96f; m.albedoB = 0.91f;
    m.metallic = 1.0f;
    m.roughness = 0.1f;
    m.ao = 1.0f;
    return m;
}

Material Materials::RustyMetal() {
    Material m;
    m.albedoR = 0.72f; m.albedoG = 0.33f; m.albedoB = 0.1f;
    m.metallic = 0.6f;
    m.roughness = 0.7f;
    m.ao = 0.8f;
    m.emissiveR = 0.02f; m.emissiveG = 0.01f; m.emissiveB = 0.0f;
    m.emissiveStrength = 0.5f;
    return m;
}

} // namespace TheSeed::Render
