# ChangeLog 0011 – Woche 11: Material-System (PBR, Hot-Reload)

**Datum:** 2026-07-21
**Version:** 0.2.2 → 0.2.3
**Phase:** 1 (Engine-Kern)
**Stunden:** 35

---

## Zusammenfassung

Vollstaendiges Material-System mit PBR Material-Definitionen, YAML/JSON Config-Loading,
Shader Hot-Reload ohne Engine-Neustart, Custom Shader Support, und Runtime Material-Editing.

## Hinzugefuegt

### Material-Config System
- MaterialConfig struct: PBR Parameters, Texture Paths, Shader Overrides, Flags
- MaterialConfigLoader: YAML/JSON Load/Save mit yaml-cpp
- Auto-Namensgenerierung aus Dateiname
- Asset-Root Pfad-Aufloesung
- Vollstaendige Validierung

### Shader Hot-Reload System
- ShaderManager: SPIR-V Loading, Caching, Hot-Reload
- LoadSPIRV(), LoadEmbedded(), GetShader(), HasShader()
- EnableHotReload() mit konfigurierbarem Poll-Intervall
- ReloadShader(), ReloadAllChanged()
- Callback-System fuer Renderer-Notification
- Thread-Safe mit std::mutex
- SPIR-V Validierung (Magic Number 0x07230203)

### Material-System Erweiterung
- Material struct erweitert: GPU Data, Custom Shaders, Dirty Tracking
- MaterialLibrary erweitert: Config Loading, Runtime Editing, Hot-Reload
- 8 Predefined Materials: RedPlastic, GreenMetal, BlueRubber, Gold, Silver, RustyMetal, EmissiveRed, Glass

### Tests (test_material_system.cpp)
- 71 Test Cases, 100% Code-Abdeckung

## Gefixt

- MaterialLibrary::GetMaterial returnt nullptr statt dangling pointer
- MaterialLibrary::RegisterMaterial mit Duplicate Name returnt existierende ID
- Material::UpdateGPUData korrigiert: Flags korrekt gepackt
- ShaderManager::LoadSPIRVFile validiert SPIR-V Magic Number
- MaterialConfigLoader::Validate prueft alle PBR-Werte auf [0,1]

## Performance

| Metrik | Wert | Ziel |
|--------|------|------|
| YAML Load | < 1ms | < 5ms |
| Material Register | < 0.1ms | < 1ms |
| Shader Hot-Reload | < 500ms | < 2s |
| Runtime Edit | < 0.01ms | < 1ms |
| Test Suite | < 2s | < 10s |

## Gate-Review

**Phase 1 – Woche 11 Gate:**

✅ **BESTANDEN**
- [x] Material-Config aus YAML laden
- [x] 8 vordefinierte Materialien mit korrekten PBR-Werten
- [x] Shader Hot-Reload: Aenderung sichtbar in < 2 Sekunden
- [x] Custom Shader: User kann eigenes SPIR-V laden
- [x] Runtime Material Edit: Metallic/Roughness aenderbar
- [x] Tests: 71 Cases, 100% Coverage
- [x] Kein Memory-Leak bei wiederholtem Reload

## Dateien

### Neu
- src/core/material_config.hpp/.cpp
- src/core/shader_manager.hpp/.cpp
- tests/test_material_system.cpp
- test_data/materials/*.yaml (4 Dateien)

### Modifiziert
- src/core/material.hpp/.cpp
- CMakeLists.txt
