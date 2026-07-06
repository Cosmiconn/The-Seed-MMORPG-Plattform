#pragma once
#include "core/renderer.hpp"
#include "core/material.hpp"
#include <array>

namespace TheSeed::Render {

// G-Buffer attachments
struct GBuffer {
    static constexpr uint32_t ATTACHMENT_COUNT = 4;

    VkImage images[ATTACHMENT_COUNT] = {};
    VkDeviceMemory memories[ATTACHMENT_COUNT] = {};
    VkImageView views[ATTACHMENT_COUNT] = {};
    VkFormat formats[ATTACHMENT_COUNT] = {
        VK_FORMAT_R16G16B16A16_SFLOAT,  // Position
        VK_FORMAT_R16G16B16A16_SFLOAT,  // Normal
        VK_FORMAT_R8G8B8A8_UNORM,       // Albedo + Metallic
        VK_FORMAT_R8G8B8A8_UNORM        // Roughness + AO + Emissive
    };
};

// Light types
struct DirectionalLight {
    float dirX, dirY, dirZ;
    float colorR, colorG, colorB;
    float intensity;
};

struct PointLight {
    float posX, posY, posZ;
    float colorR, colorG, colorB;
    float intensity;
    float radius;
};

// Deferred Renderer extends basic VulkanRenderer
class DeferredRenderer {
public:
    DeferredRenderer();
    ~DeferredRenderer();

    bool Initialize(SDL_Window* window, uint32_t width, uint32_t height);
    void Shutdown();

    bool BeginFrame();
    void EndFrame();

    // Geometry Pass
    void BeginGeometryPass();
    void EndGeometryPass();
    void SubmitMesh(const Mesh& mesh, const Material& material);

    // Lighting Pass
    void BeginLightingPass();
    void EndLightingPass();
    void SetDirectionalLight(const DirectionalLight& light);
    void AddPointLight(const PointLight& light);
    void ClearLights();

    // Tonemapping
    void Tonemap();

    bool IsInitialized() const { return m_initialized; }

private:
    bool m_initialized = false;
    SDL_Window* m_window = nullptr;
    uint32_t m_width = 0, m_height = 0;

    // Vulkan core (reused from VulkanRenderer)
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkCommandPool m_cmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBuffer = VK_NULL_HANDLE;
    VkSemaphore m_imageAvailable = VK_NULL_HANDLE;
    VkSemaphore m_renderFinished = VK_NULL_HANDLE;
    VkFence m_inFlight = VK_NULL_HANDLE;

    // Swapchain
    std::vector<VkImage> m_swapImages;
    std::vector<VkImageView> m_swapViews;
    std::vector<VkFramebuffer> m_framebuffers;
    VkExtent2D m_extent{};
    uint32_t m_imageIndex = 0;

    // G-Buffer
    GBuffer m_gbuffer;
    VkRenderPass m_geometryRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_lightingRenderPass = VK_NULL_HANDLE;
    VkFramebuffer m_geometryFramebuffer = VK_NULL_HANDLE;

    // Pipelines
    VkPipeline m_geometryPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_geometryLayout = VK_NULL_HANDLE;
    VkPipeline m_lightingPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_lightingLayout = VK_NULL_HANDLE;

    // Descriptor sets for lighting
    VkDescriptorSetLayout m_descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    // Lights
    DirectionalLight m_dirLight{0.0f, -1.0f, 0.5f, 1.0f, 0.95f, 0.8f, 1.0f};
    std::vector<PointLight> m_pointLights;

    // Material
    MaterialLibrary m_materials;

    // Helpers
    uint32_t m_graphicsQueueFamily = 0;

    bool CreateInstance();
    bool SelectPhysicalDevice();
    bool CreateDevice();
    bool CreateSurface();
    bool CreateSwapchain();
    bool CreateGBuffer();
    bool CreateGeometryRenderPass();
    bool CreateLightingRenderPass();
    bool CreateGeometryPipeline();
    bool CreateLightingPipeline();
    bool CreateFramebuffers();
    bool CreateCommandPool();
    bool CreateSyncObjects();
    bool CreateDescriptors();

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props);
    VkShaderModule CreateShaderModule(std::span<const uint32_t> code);
    void CleanupSwapchain();
    void CleanupGBuffer();
};

} // namespace TheSeed::Render
