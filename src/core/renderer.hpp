#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <memory>
#include <string>
#include <functional>

// Vulkan C-API (kein vulkan.hpp um Build-Zeit zu sparen)
#include <vulkan/vulkan.h>

struct SDL_Window;

namespace TheSeed::Render {

struct Vertex {
    float x, y, z;
    float r, g, b;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    bool Initialize(SDL_Window* window, uint32_t width, uint32_t height);
    void Shutdown();

    bool BeginFrame();
    void EndFrame();

    void SetClearColor(float r, float g, float b, float a);

    // Mesh API
    void SubmitMesh(const Mesh& mesh);
    void DrawTriangle();
    void DrawQuad();
    void DrawCube();

    bool IsInitialized() const { return m_initialized; }

private:
    bool m_initialized = false;
    SDL_Window* m_window = nullptr;

    // Vulkan Handles
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkCommandPool m_cmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBuffer = VK_NULL_HANDLE;
    VkSemaphore m_imageAvailable = VK_NULL_HANDLE;
    VkSemaphore m_renderFinished = VK_NULL_HANDLE;
    VkFence m_inFlight = VK_NULL_HANDLE;

    // Buffers
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;

    // Swapchain Images
    std::vector<VkImage> m_swapImages;
    std::vector<VkImageView> m_swapViews;
    std::vector<VkFramebuffer> m_framebuffers;
    VkExtent2D m_extent{};
    uint32_t m_imageIndex = 0;

    // Clear color
    float m_clearR = 0.0f, m_clearG = 0.0f, m_clearB = 0.0f, m_clearA = 1.0f;

    uint32_t m_graphicsQueueFamily = 0;

    // Helpers
    bool CreateInstance();
    bool SelectPhysicalDevice();
    bool CreateDevice();
    bool CreateSurface();
    bool CreateSwapchain(uint32_t width, uint32_t height);
    bool CreateRenderPass();
    bool CreatePipeline();
    bool CreateFramebuffers();
    bool CreateCommandPool();
    bool CreateSyncObjects();
    bool CreateVertexBuffer(const std::vector<Vertex>& vertices);
    bool CreateIndexBuffer(const std::vector<uint32_t>& indices);
    bool RecreateSwapchain(uint32_t width, uint32_t height);

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props);
    bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                      VkBuffer& buffer, VkDeviceMemory& memory);
    VkShaderModule CreateShaderModule(std::span<const uint32_t> code);

    void CleanupSwapchain();
};

// Factory
std::unique_ptr<VulkanRenderer> CreateRenderer();

} // namespace TheSeed::Render
