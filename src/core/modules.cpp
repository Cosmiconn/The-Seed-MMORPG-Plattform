#include "core/modules.hpp"
#include "core/serialize.hpp"
#include "core/events.hpp"
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace TheSeed::Modules {

// --- ModuleConfig ---

bool ModuleConfig::GetBool(const std::string& key, bool defaultVal) const {
    auto it = parameters.find(key);
    if (it == parameters.end()) return defaultVal;
    if (it->second == "true" || it->second == "1" || it->second == "yes") return true;
    if (it->second == "false" || it->second == "0" || it->second == "no") return false;
    return defaultVal;
}

int ModuleConfig::GetInt(const std::string& key, int defaultVal) const {
    auto it = parameters.find(key);
    if (it == parameters.end()) return defaultVal;
    try { return std::stoi(it->second); } catch (...) { return defaultVal; }
}

float ModuleConfig::GetFloat(const std::string& key, float defaultVal) const {
    auto it = parameters.find(key);
    if (it == parameters.end()) return defaultVal;
    try { return std::stof(it->second); } catch (...) { return defaultVal; }
}

std::string ModuleConfig::GetString(const std::string& key, const std::string& defaultVal) const {
    auto it = parameters.find(key);
    if (it == parameters.end()) return defaultVal;
    return it->second;
}

// --- ModuleLoader ---

ModuleLoader::ModuleLoader() = default;
ModuleLoader::~ModuleLoader() {
    for (auto& handle : m_modules) {
        if (handle->instance) {
            handle->instance->Shutdown();
        }
        if (handle->osHandle) {
            CloseLibrary(handle->osHandle);
        }
    }
}

void* ModuleLoader::OpenLibrary(const std::filesystem::path& path) {
#ifdef _WIN32
    return LoadLibraryW(path.wstring().c_str());
#else
    return dlopen(path.c_str(), RTLD_NOW);
#endif
}

void ModuleLoader::CloseLibrary(void* handle) {
    if (!handle) return;
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* ModuleLoader::GetSymbol(void* handle, const char* name) {
    if (!handle) return nullptr;
#ifdef _WIN32
    return GetProcAddress(static_cast<HMODULE>(handle), name);
#else
    return dlsym(handle, name);
#endif
}

std::expected<IModule*, std::string> ModuleLoader::Load(const std::filesystem::path& path, const ModuleConfig& config) {
    if (!std::filesystem::exists(path)) {
        return std::unexpected("Modul-Datei nicht gefunden: " + path.string());
    }

    void* handle = OpenLibrary(path);
    if (!handle) {
        return std::unexpected("Konnte DLL/SO nicht laden: " + path.string());
    }

    auto createFn = reinterpret_cast<CreateModuleFunc>(GetSymbol(handle, "CreateModule"));
    auto destroyFn = reinterpret_cast<DestroyModuleFunc>(GetSymbol(handle, "DestroyModule"));
    auto infoFn = reinterpret_cast<GetModuleInfoFunc>(GetSymbol(handle, "GetModuleInfo"));

    if (!createFn || !destroyFn || !infoFn) {
        CloseLibrary(handle);
        return std::unexpected("Export-Funktionen nicht gefunden in: " + path.string());
    }

    IModule* instance = createFn();
    if (!instance) {
        CloseLibrary(handle);
        return std::unexpected("CreateModule() returned nullptr");
    }

    if (!instance->Initialize(config)) {
        destroyFn(instance);
        CloseLibrary(handle);
        return std::unexpected("Modul-Initialisierung fehlgeschlagen: " + std::string(instance->GetName()));
    }

    auto modHandle = std::make_unique<ModuleHandle>();
    modHandle->path = path;
    modHandle->instance = instance;
    modHandle->osHandle = handle;
    modHandle->config = config;

    std::string name = instance->GetName();
    m_nameIndex[name] = m_modules.size();
    m_modules.push_back(std::move(modHandle));

    instance->PostInitialize();
    spdlog::info("Modul geladen: {} v{} von {}", name, instance->GetVersion(), instance->GetAuthor());
    return instance;
}

bool ModuleLoader::Unload(IModule* module) {
    if (!module) return false;

    std::string name = module->GetName();
    auto it = m_nameIndex.find(name);
    if (it == m_nameIndex.end()) return false;

    size_t idx = it->second;
    auto& handle = m_modules[idx];

    module->Shutdown();

    if (handle->osHandle) {
        auto destroyFn = reinterpret_cast<DestroyModuleFunc>(GetSymbol(handle->osHandle, "DestroyModule"));
        if (destroyFn) {
            destroyFn(module);
        }
        CloseLibrary(handle->osHandle);
    }

    m_modules.erase(m_modules.begin() + idx);
    m_nameIndex.erase(it);

    // Indizes neu aufbauen
    for (size_t i = 0; i < m_modules.size(); ++i) {
        m_nameIndex[m_modules[i]->instance->GetName()] = i;
    }

    spdlog::info("Modul entladen: {}", name);
    return true;
}

std::expected<IModule*, std::string> ModuleLoader::HotReload(IModule* module) {
    if (!module) return std::unexpected("Nullptr-Modul");

    std::string name = module->GetName();
    auto it = m_nameIndex.find(name);
    if (it == m_nameIndex.end()) return std::unexpected("Modul nicht gefunden");

    auto& handle = m_modules[it->second];
    auto path = handle->path;
    auto config = handle->config;

    // State serialisieren
    // (Für Demo: einfach nur Shutdown/Unload/Load)
    module->Shutdown();
    Unload(module);

    return Load(path, config);
}

IModule* ModuleLoader::GetModule(const std::string& name) const {
    auto it = m_nameIndex.find(name);
    if (it == m_nameIndex.end()) return nullptr;
    return m_modules[it->second]->instance;
}

std::vector<IModule*> ModuleLoader::GetAllModules() const {
    std::vector<IModule*> result;
    for (auto& m : m_modules) {
        result.push_back(m->instance);
    }
    return result;
}

void ModuleLoader::TickAll(float deltaTime) {
    for (auto& m : m_modules) {
        if (m->instance) m->instance->Tick(deltaTime);
    }
}

void ModuleLoader::FixedTickAll(float fixedDelta) {
    for (auto& m : m_modules) {
        if (m->instance) m->instance->FixedTick(fixedDelta);
    }
}

} // namespace TheSeed::Modules
