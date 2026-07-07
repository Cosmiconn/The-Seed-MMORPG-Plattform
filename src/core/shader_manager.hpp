#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <span>
#include <chrono>
#include <mutex>
#include <functional>
#include <vulkan/vulkan.h>

namespace TheSeed::Render {

struct ShaderEntry {
    std::filesystem::path path;
    VkShaderStageFlagBits stage;
    VkShaderModule module = VK_NULL_HANDLE;
    std::filesystem::file_time_type lastWrite;
    std::vector<uint32_t> spirv;
    bool isCustom = false;
};

using ShaderReloadCallback = std::function<void(const std::string& shaderName)>;

class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();
    bool Initialize(VkDevice device);
    void Shutdown();
    VkShaderModule LoadSPIRV(const std::filesystem::path& path, VkShaderStageFlagBits stage, const std::string& name = "");
    VkShaderModule LoadEmbedded(std::span<const uint32_t> spirv, VkShaderStageFlagBits stage, const std::string& name);
    VkShaderModule GetShader(const std::string& name) const;
    bool HasShader(const std::string& name) const;
    const ShaderEntry* GetShaderEntry(const std::string& name) const;
    void EnableHotReload(const std::filesystem::path& watchDir, std::chrono::milliseconds pollInterval = std::chrono::milliseconds(500));
    void DisableHotReload();
    void PollFileChanges();
    bool ReloadShader(const std::string& name);
    size_t ReloadAllChanged();
    void OnShaderReloaded(ShaderReloadCallback callback);
    void ClearCallbacks();
    std::vector<std::string> GetShaderNames() const;
    size_t GetShaderCount() const;
    size_t GetReloadCount() const;
    bool IsInitialized() const { return m_device != VK_NULL_HANDLE; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    std::unordered_map<std::string, ShaderEntry> m_shaders;
    std::vector<ShaderReloadCallback> m_callbacks;
    mutable std::mutex m_mutex;
    std::filesystem::path m_watchDir;
    bool m_hotReloadEnabled = false;
    std::chrono::milliseconds m_pollInterval{500};
    std::chrono::steady_clock::time_point m_lastPoll;
    size_t m_reloadCount = 0;
    VkShaderModule CreateShaderModule(std::span<const uint32_t> code);
    bool LoadSPIRVFile(const std::filesystem::path& path, std::vector<uint32_t>& out);
    bool HasFileChanged(const ShaderEntry& entry) const;
    void NotifyReloaded(const std::string& name);
    void DestroyShaderModule(VkShaderModule module);
};

} // namespace TheSeed::Render
