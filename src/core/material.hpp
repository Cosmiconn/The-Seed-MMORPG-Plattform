#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vulkan/vulkan.h>
#include "core/material_config.hpp"

namespace TheSeed::Render {

struct GPUMaterialData {
    float albedoR, albedoG, albedoB, pad1;
    float metallic, roughness, ao, pad2;
    float emissiveR, emissiveG, emissiveB, emissiveStrength;
    uint32_t albedoMap, normalMap, metallicMap, roughnessMap, aoMap, emissiveMap;
    uint32_t flags;
    uint32_t pad3[3];
};

struct Material {
    uint32_t id = 0;
    std::string name;
    PBRParameters pbr;
    uint32_t albedoMap = 0;
    uint32_t normalMap = 0;
    uint32_t metallicMap = 0;
    uint32_t roughnessMap = 0;
    uint32_t aoMap = 0;
    uint32_t emissiveMap = 0;
    VkShaderModule customVertexShader = VK_NULL_HANDLE;
    VkShaderModule customFragmentShader = VK_NULL_HANDLE;
    bool useCustomShaders = false;
    bool transparent = false;
    bool doubleSided = false;
    bool unlit = false;
    bool wireframe = false;
    bool dirty = true;
    bool gpuDataDirty = true;
    GPUMaterialData gpuData{};
    std::filesystem::path configPath;
    void UpdateGPUData();
};

class MaterialLibrary {
public:
    MaterialLibrary();
    ~MaterialLibrary();
    uint32_t RegisterMaterial(const MaterialConfig& config);
    uint32_t RegisterMaterial(const std::string& name, const Material& mat);
    uint32_t LoadFromConfig(const std::filesystem::path& configPath, MaterialConfigLoader& loader);
    Material* GetMaterial(uint32_t id);
    Material* GetMaterial(const std::string& name);
    const Material* GetMaterial(uint32_t id) const;
    const Material* GetMaterial(const std::string& name) const;
    bool RemoveMaterial(uint32_t id);
    bool RemoveMaterial(const std::string& name);
    void SetAlbedo(uint32_t id, float r, float g, float b);
    void SetMetallic(uint32_t id, float v);
    void SetRoughness(uint32_t id, float v);
    void SetAO(uint32_t id, float v);
    void SetEmissive(uint32_t id, float r, float g, float b, float strength);
    void SetTransparent(uint32_t id, bool v);
    void SetDoubleSided(uint32_t id, bool v);
    void SetUnlit(uint32_t id, bool v);
    void SetWireframe(uint32_t id, bool v);
    void SetCustomShaders(uint32_t id, VkShaderModule vert, VkShaderModule frag);
    void ClearCustomShaders(uint32_t id);
    bool ReloadMaterial(uint32_t id, MaterialConfigLoader& loader);
    bool ReloadMaterial(const std::string& name, MaterialConfigLoader& loader);
    size_t ReloadAll(MaterialConfigLoader& loader);
    size_t Count() const;
    bool HasMaterial(uint32_t id) const;
    bool HasMaterial(const std::string& name) const;
    std::vector<uint32_t> GetAllIds() const;
    std::vector<std::string> GetAllNames() const;
    void Clear();
    bool HasDirtyMaterials() const;
    std::vector<uint32_t> GetDirtyIds() const;
    void ClearDirtyFlags();
    static Material RedPlastic();
    static Material GreenMetal();
    static Material BlueRubber();
    static Material Gold();
    static Material Silver();
    static Material RustyMetal();
    static Material EmissiveRed();
    static Material Glass();

private:
    std::unordered_map<std::string, uint32_t> m_nameToId;
    std::unordered_map<uint32_t, std::unique_ptr<Material>> m_materials;
    uint32_t m_nextId = 1;
    mutable std::mutex m_mutex;
    uint32_t AllocateId();
    void MarkDirty(uint32_t id);
};

} // namespace TheSeed::Render
