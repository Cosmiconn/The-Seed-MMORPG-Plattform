#include "core/shader_manager.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>

namespace TheSeed::Render {

ShaderManager::ShaderManager() = default;
ShaderManager::~ShaderManager() { Shutdown(); }

bool ShaderManager::Initialize(VkDevice device) {
    if (device == VK_NULL_HANDLE) { spdlog::error("ShaderManager: Invalid device"); return false; }
    m_device = device;
    spdlog::info("ShaderManager initialisiert");
    return true;
}

void ShaderManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [name, entry] : m_shaders) { if (entry.module != VK_NULL_HANDLE) DestroyShaderModule(entry.module); }
    m_shaders.clear(); m_callbacks.clear(); m_hotReloadEnabled = false; m_device = VK_NULL_HANDLE;
    spdlog::info("ShaderManager heruntergefahren");
}

VkShaderModule ShaderManager::CreateShaderModule(std::span<const uint32_t> code) {
    if (code.empty()) { spdlog::error("ShaderManager: Empty SPIR-V code"); return VK_NULL_HANDLE; }
    if (m_device == VK_NULL_HANDLE) { spdlog::error("ShaderManager: Device not initialized"); return VK_NULL_HANDLE; }
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    VkShaderModule module;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &module) != VK_SUCCESS) {
        spdlog::error("ShaderManager: Failed to create shader module"); return VK_NULL_HANDLE;
    }
    return module;
}

void ShaderManager::DestroyShaderModule(VkShaderModule module) {
    if (module != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) vkDestroyShaderModule(m_device, module, nullptr);
}

bool ShaderManager::LoadSPIRVFile(const std::filesystem::path& path, std::vector<uint32_t>& out) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) { spdlog::error("ShaderManager: Failed to open SPIR-V file: {}", path.string()); return false; }
    auto size = file.tellg(); file.seekg(0, std::ios::beg);
    if (size % sizeof(uint32_t) != 0) { spdlog::error("ShaderManager: SPIR-V file size not aligned: {}", path.string()); return false; }
    out.resize(size / sizeof(uint32_t));
    if (!file.read(reinterpret_cast<char*>(out.data()), size)) { spdlog::error("ShaderManager: Failed to read SPIR-V file: {}", path.string()); return false; }
    if (out.empty() || out[0] != 0x07230203) { spdlog::error("ShaderManager: Invalid SPIR-V magic: {}", path.string()); out.clear(); return false; }
    return true;
}

VkShaderModule ShaderManager::LoadSPIRV(const std::filesystem::path& path, VkShaderStageFlagBits stage, const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_device == VK_NULL_HANDLE) { spdlog::error("ShaderManager: Device not initialized"); return VK_NULL_HANDLE; }
    std::string shaderName = name.empty() ? path.stem().string() : name;
    auto it = m_shaders.find(shaderName);
    if (it != m_shaders.end()) { spdlog::debug("ShaderManager: '{}' already loaded", shaderName); return it->second.module; }
    std::vector<uint32_t> spirv;
    if (!LoadSPIRVFile(path, spirv)) return VK_NULL_HANDLE;
    VkShaderModule module = CreateShaderModule(spirv);
    if (module == VK_NULL_HANDLE) return VK_NULL_HANDLE;
    ShaderEntry entry; entry.path = path; entry.stage = stage; entry.module = module;
    entry.spirv = std::move(spirv); entry.isCustom = true;
    if (std::filesystem::exists(path)) entry.lastWrite = std::filesystem::last_write_time(path);
    m_shaders[shaderName] = std::move(entry);
    spdlog::info("ShaderManager: Loaded SPIR-V '{}' from {}", shaderName, path.string());
    return module;
}

VkShaderModule ShaderManager::LoadEmbedded(std::span<const uint32_t> spirv, VkShaderStageFlagBits stage, const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_device == VK_NULL_HANDLE) { spdlog::error("ShaderManager: Device not initialized"); return VK_NULL_HANDLE; }
    if (name.empty()) { spdlog::error("ShaderManager: Embedded shader requires a name"); return VK_NULL_HANDLE; }
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) { spdlog::debug("ShaderManager: Embedded '{}' already loaded", name); return it->second.module; }
    VkShaderModule module = CreateShaderModule(spirv);
    if (module == VK_NULL_HANDLE) return VK_NULL_HANDLE;
    ShaderEntry entry; entry.path.clear(); entry.stage = stage; entry.module = module;
    entry.spirv.assign(spirv.begin(), spirv.end()); entry.isCustom = false;
    m_shaders[name] = std::move(entry);
    spdlog::info("ShaderManager: Loaded embedded '{}'", name);
    return module;
}

VkShaderModule ShaderManager::GetShader(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) return it->second.module;
    return VK_NULL_HANDLE;
}

bool ShaderManager::HasShader(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_shaders.contains(name);
}

const ShaderEntry* ShaderManager::GetShaderEntry(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) return &it->second;
    return nullptr;
}

void ShaderManager::EnableHotReload(const std::filesystem::path& watchDir, std::chrono::milliseconds pollInterval) {
    if (!std::filesystem::exists(watchDir) || !std::filesystem::is_directory(watchDir)) {
        spdlog::warn("ShaderManager: Watch directory does not exist: {}", watchDir.string()); return;
    }
    m_watchDir = watchDir; m_pollInterval = pollInterval; m_hotReloadEnabled = true;
    m_lastPoll = std::chrono::steady_clock::now();
    spdlog::info("ShaderManager: Hot-Reload aktiviert fuer '{}' (Poll: {}ms)", watchDir.string(), pollInterval.count());
}

void ShaderManager::DisableHotReload() {
    m_hotReloadEnabled = false; m_watchDir.clear();
    spdlog::info("ShaderManager: Hot-Reload deaktiviert");
}

bool ShaderManager::HasFileChanged(const ShaderEntry& entry) const {
    if (entry.path.empty() || !std::filesystem::exists(entry.path)) return false;
    try { auto current = std::filesystem::last_write_time(entry.path); return current != entry.lastWrite; }
    catch (const std::filesystem::filesystem_error&) { return false; }
}

void ShaderManager::PollFileChanges() {
    if (!m_hotReloadEnabled) return;
    auto now = std::chrono::steady_clock::now();
    if (now - m_lastPoll < m_pollInterval) return;
    m_lastPoll = now;
    ReloadAllChanged();
}

bool ShaderManager::ReloadShader(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_shaders.find(name);
    if (it == m_shaders.end()) { spdlog::warn("ShaderManager: Cannot reload unknown '{}'", name); return false; }
    auto& entry = it->second;
    if (entry.path.empty() || !entry.isCustom) { spdlog::warn("ShaderManager: '{}' not reloadable", name); return false; }
    std::vector<uint32_t> newSpirv;
    if (!LoadSPIRVFile(entry.path, newSpirv)) { spdlog::error("ShaderManager: Failed to reload '{}'", name); return false; }
    VkShaderModule oldModule = entry.module;
    VkShaderModule newModule = CreateShaderModule(newSpirv);
    if (newModule == VK_NULL_HANDLE) { spdlog::error("ShaderManager: Failed to create new module for '{}'", name); return false; }
    entry.module = newModule; entry.spirv = std::move(newSpirv);
    entry.lastWrite = std::filesystem::last_write_time(entry.path);
    DestroyShaderModule(oldModule);
    m_reloadCount++;
    spdlog::info("ShaderManager: Reloaded '{}' (reload #{})", name, m_reloadCount);
    NotifyReloaded(name);
    return true;
}

size_t ShaderManager::ReloadAllChanged() {
    std::vector<std::string> toReload;
    { std::lock_guard<std::mutex> lock(m_mutex);
      for (const auto& [name, entry] : m_shaders) { if (entry.isCustom && !entry.path.empty() && HasFileChanged(entry)) toReload.push_back(name); } }
    size_t reloaded = 0;
    for (const auto& name : toReload) { if (ReloadShader(name)) reloaded++; }
    if (reloaded > 0) spdlog::info("ShaderManager: {} Shader(s) hot-reloaded", reloaded);
    return reloaded;
}

void ShaderManager::OnShaderReloaded(ShaderReloadCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.push_back(std::move(callback));
}

void ShaderManager::ClearCallbacks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.clear();
}

void ShaderManager::NotifyReloaded(const std::string& name) {
    std::vector<ShaderReloadCallback> callbacks;
    { std::lock_guard<std::mutex> lock(m_mutex); callbacks = m_callbacks; }
    for (auto& cb : callbacks) cb(name);
}

std::vector<std::string> ShaderManager::GetShaderNames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names; names.reserve(m_shaders.size());
    for (const auto& [name, _] : m_shaders) names.push_back(name);
    return names;
}

size_t ShaderManager::GetShaderCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_shaders.size();
}

size_t ShaderManager::GetReloadCount() const { return m_reloadCount; }

} // namespace TheSeed::Render
