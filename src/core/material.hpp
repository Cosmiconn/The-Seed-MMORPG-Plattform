#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace TheSeed::Render {

// PBR Material (Metallic/Roughness Workflow)
struct Material {
    float albedoR = 1.0f, albedoG = 1.0f, albedoB = 1.0f;
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    float emissiveR = 0.0f, emissiveG = 0.0f, emissiveB = 0.0f;
    float emissiveStrength = 0.0f;

    // Texture handles (0 = none)
    uint32_t albedoMap = 0;
    uint32_t normalMap = 0;
    uint32_t metallicMap = 0;
    uint32_t roughnessMap = 0;
    uint32_t aoMap = 0;
    uint32_t emissiveMap = 0;
};

// Material Library (hot-reloadable)
class MaterialLibrary {
public:
    uint32_t RegisterMaterial(const std::string& name, const Material& mat);
    Material* GetMaterial(uint32_t id);
    Material* GetMaterial(const std::string& name);

    void UpdateMaterial(uint32_t id, const Material& mat);
    void RemoveMaterial(uint32_t id);

    size_t Count() const { return m_materials.size(); }
    void Clear();

private:
    std::unordered_map<std::string, uint32_t> m_nameToId;
    std::unordered_map<uint32_t, std::unique_ptr<Material>> m_materials;
    uint32_t m_nextId = 1;
};

// Predefined materials
namespace Materials {
    Material RedPlastic();
    Material GreenMetal();
    Material BlueRubber();
    Material Gold();
    Material Silver();
    Material RustyMetal();
}

} // namespace TheSeed::Render
