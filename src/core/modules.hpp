#pragma once
#include <string>
#include <memory>
#include <vector>
#include <expected>
#include <filesystem>
#include <unordered_map>
#include <cstdint>
#include "core/serialize.hpp"
#include "core/events.hpp"

namespace TheSeed::Modules {

using TheSeed::Serialize::ISerializer;
using TheSeed::Serialize::IDeserializer;
using TheSeed::Events::EventType;
using TheSeed::Events::GameEvent;

// --- Konfiguration (YAML/JSON) ---
struct ModuleConfig {
    std::string name;
    std::string version;
    std::filesystem::path assetPath;
    std::unordered_map<std::string, std::string> parameters;

    bool GetBool(const std::string& key, bool defaultVal = false) const;
    int GetInt(const std::string& key, int defaultVal = 0) const;
    float GetFloat(const std::string& key, float defaultVal = 0.0f) const;
    std::string GetString(const std::string& key, const std::string& defaultVal = "") const;
};

// --- Modul-Interface ---
class IModule {
public:
    virtual ~IModule() = default;

    virtual const char* GetName() const = 0;
    virtual const char* GetVersion() const = 0;
    virtual const char* GetAuthor() const = 0;

    virtual bool Initialize(const ModuleConfig& config) = 0;
    virtual void PostInitialize() = 0;
    virtual void Shutdown() = 0;

    virtual void Tick(float deltaTime) = 0;
    virtual void FixedTick(float fixedDelta) = 0;

    virtual void Serialize(ISerializer& ser) = 0;
    virtual void Deserialize(IDeserializer& de) = 0;

    virtual void OnHotReload() = 0;
    virtual void OnEvent(const GameEvent& evt) = 0;
};

// --- Export-Table (fuer DLLs) ---
using CreateModuleFunc  = IModule* (*)();
using DestroyModuleFunc = void (*)(IModule*);
using GetModuleInfoFunc = const char* (*)();

struct ModuleExportTable {
    const char*         apiVersion;
    CreateModuleFunc    create;
    DestroyModuleFunc   destroy;
    GetModuleInfoFunc   getInfo;
};

// --- Makro fuer DLL-Export ---
#ifdef _WIN32
    #define THESEED_EXPORT extern "C" __declspec(dllexport)
#else
    #define THESEED_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define THESEED_MODULE(className, modName, modVersion)     THESEED_EXPORT const char* GetModuleInfo() {         return "{\"name\":\"" modName "\",\"version\":\"" modVersion "\",\"api\":\"TheSeed.ModuleAPI.v1\"}";     }     THESEED_EXPORT TheSeed::Modules::IModule* CreateModule() {         return new className();     }     THESEED_EXPORT void DestroyModule(TheSeed::Modules::IModule* m) {         delete m;     }

// --- Modul-Loader ---
class ModuleLoader {
public:
    ModuleLoader();
    ~ModuleLoader();

    std::expected<IModule*, std::string> Load(const std::filesystem::path& path, const ModuleConfig& config);
    bool Unload(IModule* module);
    std::expected<IModule*, std::string> HotReload(IModule* module);

    IModule* GetModule(const std::string& name) const;
    std::vector<IModule*> GetAllModules() const;

    void TickAll(float deltaTime);
    void FixedTickAll(float fixedDelta);

private:
    struct ModuleHandle {
        std::filesystem::path path;
        IModule* instance;
        void* osHandle;
        ModuleConfig config;
    };
    std::vector<std::unique_ptr<ModuleHandle>> m_modules;
    std::unordered_map<std::string, size_t> m_nameIndex;

    void* OpenLibrary(const std::filesystem::path& path);
    void CloseLibrary(void* handle);
    void* GetSymbol(void* handle, const char* name);
};

} // namespace TheSeed::Modules
