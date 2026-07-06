#include "core/deferred_renderer.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace TheSeed::Render {

// Inline SPIR-V: Simple Vertex Shader
static const uint32_t s_pbrVert[] = {
    0x07230203,0x00010000,0x00080001,0x0000004a,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x000f000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000a,0x00000016,0x0000001c,
    0x00000022,0x0000002c,0x00000036,0x0000003e,0x00000043,0x00030003,0x00000002,0x000001c2,
    0x00090004,0x415f4c47,0x735f4252,0x72617065,0x5f657461,0x00000000,0x00050004,0x4f4c4743,
    0x6f736972,0x00000000,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,
    0x6f6c6f43,0x0000726f,0x00040005,0x0000000a,0x6c6f4376,0x0000726f,0x00040005,0x0000000e,
    0x6d726f4e,0x00006c61,0x00050005,0x00000016,0x78655476,0x6f6f6c43,0x00000072,0x00060006,
    0x00000016,0x00000000,0x6f6c6f43,0x00000072,0x00060006,0x00000016,0x00000001,0x56530000,
    0x00000050,0x00060006,0x00000016,0x00000002,0x65746e49,0x00000000,0x00070006,0x00000016,
    0x00000003,0x6e695f75,0x65726170,0x00000000,0x00040005,0x00000018,0x00786574,0x00040005,
    0x0000001c,0x6c6f4376,0x00000000,0x00050005,0x00000022,0x6d726f4e,0x00000000,0x00040005,
    0x0000002c,0x6c61744d,0x00000000,0x00060006,0x0000002c,0x00000000,0x6c62616c,0x6f446f64,
    0x00000000,0x00060006,0x0000002c,0x00000001,0x6c696174,0x69754363,0x00000000,0x00060006,
    0x0000002c,0x00000002,0x68676965,0x6f52746c,0x00006867,0x00060006,0x0000002c,0x00000003,
    0x69736f50,0x6e6f6974,0x00000000,0x00050005,0x0000002e,0x00786574,0x00040047,0x00000009,
    0x0000001e,0x00000000,0x00040047,0x0000000a,0x0000001e,0x00000000,0x00040047,0x0000000e,
    0x0000001e,0x00000000,0x00040047,0x00000016,0x0000001e,0x00000000,0x00040047,0x0000001c,
    0x0000001e,0x00000000,0x00040047,0x00000022,0x0000001e,0x00000000,0x00040047,0x0000002c,
    0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,
    0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,
    0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000b,
    0x00000006,0x00000003,0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,0x0000000c,
    0x0000000d,0x00000001,0x00040020,0x0000000f,0x00000001,0x00000007,0x0004003b,0x0000000f,
    0x00000010,0x00000001,0x00040017,0x00000012,0x00000006,0x00000002,0x00040020,0x00000013,
    0x00000001,0x00000012,0x0004003b,0x00000013,0x00000014,0x00000001,0x00040015,0x00000019,
    0x00000020,0x00000001,0x0004002b,0x00000019,0x0000001a,0x00000000,0x0006001c,0x0000001b,
    0x00000006,0x00000019,0x00000016,0x0000001a,0x00040020,0x0000001c,0x00000001,0x0000001b,
    0x0004003b,0x0000001c,0x0000001d,0x00000001,0x00040020,0x0000001e,0x00000001,0x0000000b,
    0x0004003b,0x0000001e,0x0000001f,0x00000001,0x00040017,0x00000024,0x00000006,0x00000004,
    0x00040020,0x00000025,0x00000001,0x00000024,0x0004003b,0x00000025,0x00000026,0x00000001,
    0x00040015,0x0000002d,0x00000020,0x00000001,0x0004002b,0x0000002d,0x0000002f,0x00000000,
    0x0006001c,0x00000030,0x00000006,0x0000002d,0x0000002c,0x0000002f,0x00040020,0x00000031,
    0x00000001,0x00000030,0x0004003b,0x00000031,0x00000032,0x00000001,0x00050036,0x00000002,
    0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x0000000b,0x00000011,
    0x0000000d,0x00050041,0x00000010,0x00000015,0x00000011,0x0000001a,0x0003003e,0x00000015,
    0x00000010,0x0004003d,0x0000001b,0x00000020,0x0000001d,0x00050041,0x0000001f,0x00000021,
    0x00000020,0x0000001a,0x0003003e,0x00000021,0x0000001f,0x0004003d,0x00000030,0x00000033,
    0x00000032,0x00050041,0x00000026,0x00000034,0x00000033,0x0000002f,0x0003003e,0x00000034,
    0x00000026,0x000100fd,0x00010038
};

// Inline SPIR-V: Simple Fragment Shader (material color output)
static const uint32_t s_pbrFrag[] = {
    0x07230203,0x00010000,0x00080001,0x0000002a,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0008000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000009,0x00000015,0x0000001b,
    0x00030003,0x00000002,0x000001c2,0x00090004,0x415f4c47,0x735f4252,0x72617065,0x5f657461,
    0x00000000,0x00050004,0x4f4c4743,0x6f736972,0x00000000,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00040005,0x00000009,0x6f6c6f43,0x00000000,0x00040005,0x0000000d,0x6c61744d,
    0x00000000,0x00060006,0x0000000d,0x00000000,0x6c62616c,0x6f446f64,0x00000000,0x00060006,
    0x0000000d,0x00000001,0x6c696174,0x69754363,0x00000000,0x00060006,0x0000000d,0x00000002,
    0x68676965,0x6f52746c,0x00006867,0x00060006,0x0000000d,0x00000003,0x69736f50,0x6e6f6974,
    0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
    0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
    0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
    0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000b,0x00000006,
    0x00000003,0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,0x0000000c,0x0000000d,
    0x00000001,0x00040015,0x00000011,0x00000020,0x00000001,0x0004002b,0x00000011,0x00000013,
    0x00000000,0x0006001c,0x00000014,0x00000006,0x00000011,0x0000000d,0x00000013,0x00040020,
    0x00000015,0x00000001,0x00000014,0x0004003b,0x00000015,0x00000016,0x00000001,0x0004002b,
    0x00000006,0x0000001c,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,
    0x000200f8,0x00000005,0x0004003d,0x00000014,0x00000017,0x00000016,0x00050041,0x00000009,
    0x00000018,0x00000017,0x00000013,0x0003003e,0x00000018,0x00000009,0x000100fd,0x00010038
};

DeferredRenderer::DeferredRenderer() = default;
DeferredRenderer::~DeferredRenderer() { Shutdown(); }

bool DeferredRenderer::Initialize(SDL_Window* window, uint32_t width, uint32_t height) {
    m_window = window;
    m_width = width;
    m_height = height;

    if (!CreateInstance()) return false;
    if (!CreateSurface()) return false;
    if (!SelectPhysicalDevice()) return false;
    if (!CreateDevice()) return false;
    if (!CreateSwapchain()) return false;
    if (!CreateCommandPool()) return false;
    if (!CreateSyncObjects()) return false;
    if (!CreateGeometryRenderPass()) return false;
    if (!CreateGeometryPipeline()) return false;
    if (!CreateFramebuffers()) return false;

    m_initialized = true;
    spdlog::info("DeferredRenderer initialisiert: {}x{}", width, height);
    return true;
}

void DeferredRenderer::Shutdown() {
    if (!m_initialized && m_device == VK_NULL_HANDLE) return;
    if (m_device != VK_NULL_HANDLE) vkDeviceWaitIdle(m_device);

    CleanupSwapchain();

    if (m_geometryPipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_device, m_geometryPipeline, nullptr);
    if (m_geometryLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_device, m_geometryLayout, nullptr);
    if (m_geometryRenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_device, m_geometryRenderPass, nullptr);

    if (m_inFlight != VK_NULL_HANDLE) vkDestroyFence(m_device, m_inFlight, nullptr);
    if (m_renderFinished != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_renderFinished, nullptr);
    if (m_imageAvailable != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_imageAvailable, nullptr);
    if (m_cmdPool != VK_NULL_HANDLE) vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
    if (m_device != VK_NULL_HANDLE) vkDestroyDevice(m_device, nullptr);
    if (m_surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    if (m_instance != VK_NULL_HANDLE) vkDestroyInstance(m_instance, nullptr);

    m_initialized = false;
    spdlog::info("DeferredRenderer heruntergefahren");
}

bool DeferredRenderer::BeginFrame() {
    vkWaitForFences(m_device, 1, &m_inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlight);

    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailable, VK_NULL_HANDLE, &m_imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) return false;

    vkResetCommandBuffer(m_cmdBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_cmdBuffer, &beginInfo);

    return true;
}

void DeferredRenderer::EndFrame() {
    vkEndCommandBuffer(m_cmdBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_imageAvailable;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_cmdBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_renderFinished;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlight);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_imageIndex;

    vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
}

void DeferredRenderer::BeginGeometryPass() {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_geometryRenderPass;
    renderPassInfo.framebuffer = m_framebuffers[m_imageIndex];
    renderPassInfo.renderArea.extent = m_extent;

    VkClearValue clearColor{};
    clearColor.color.float32[0] = 0.05f;
    clearColor.color.float32[1] = 0.05f;
    clearColor.color.float32[2] = 0.05f;
    clearColor.color.float32[3] = 1.0f;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(m_cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_geometryPipeline);

    VkViewport viewport{};
    viewport.width = static_cast<float>(m_extent.width);
    viewport.height = static_cast<float>(m_extent.height);
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = m_extent;
    vkCmdSetScissor(m_cmdBuffer, 0, 1, &scissor);
}

void DeferredRenderer::EndGeometryPass() {
    vkCmdEndRenderPass(m_cmdBuffer);
}

void DeferredRenderer::SubmitMesh(const Mesh& mesh, const Material& material) {
    std::vector<Vertex> coloredVerts = mesh.vertices;
    for (auto& v : coloredVerts) {
        v.r = material.albedoR;
        v.g = material.albedoG;
        v.b = material.albedoB;
    }

    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = coloredVerts.size() * sizeof(Vertex);
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer vb;
    vkCreateBuffer(m_device, &bufInfo, nullptr, &vb);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, vb, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory mem;
    vkAllocateMemory(m_device, &allocInfo, nullptr, &mem);
    vkBindBufferMemory(m_device, vb, mem, 0);

    void* data;
    vkMapMemory(m_device, mem, 0, bufInfo.size, 0, &data);
    std::memcpy(data, coloredVerts.data(), bufInfo.size);
    vkUnmapMemory(m_device, mem);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_cmdBuffer, 0, 1, &vb, &offset);

    if (!mesh.indices.empty()) {
        VkBufferCreateInfo ibInfo{};
        ibInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ibInfo.size = mesh.indices.size() * sizeof(uint32_t);
        ibInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        ibInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer ib;
        vkCreateBuffer(m_device, &ibInfo, nullptr, &ib);

        VkMemoryRequirements ibMemReqs;
        vkGetBufferMemoryRequirements(m_device, ib, &ibMemReqs);

        VkMemoryAllocateInfo ibAlloc{};
        ibAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ibAlloc.allocationSize = ibMemReqs.size;
        ibAlloc.memoryTypeIndex = FindMemoryType(ibMemReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkDeviceMemory ibMem;
        vkAllocateMemory(m_device, &ibAlloc, nullptr, &ibMem);
        vkBindBufferMemory(m_device, ib, ibMem, 0);

        void* ibData;
        vkMapMemory(m_device, ibMem, 0, ibInfo.size, 0, &ibData);
        std::memcpy(ibData, mesh.indices.data(), ibInfo.size);
        vkUnmapMemory(m_device, ibMem);

        vkCmdBindIndexBuffer(m_cmdBuffer, ib, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(m_cmdBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

        vkDestroyBuffer(m_device, ib, nullptr);
        vkFreeMemory(m_device, ibMem, nullptr);
    } else {
        vkCmdDraw(m_cmdBuffer, static_cast<uint32_t>(coloredVerts.size()), 1, 0, 0);
    }

    vkDestroyBuffer(m_device, vb, nullptr);
    vkFreeMemory(m_device, mem, nullptr);
}

void DeferredRenderer::SetDirectionalLight(const DirectionalLight& light) {
    m_dirLight = light;
}

void DeferredRenderer::AddPointLight(const PointLight& light) {
    m_pointLights.push_back(light);
}

void DeferredRenderer::ClearLights() {
    m_pointLights.clear();
}

bool DeferredRenderer::CreateInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "TheSeed";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 2, 0);
    appInfo.pEngineName = "TheSeed";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 2, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    #if defined(_WIN32)
        extensions.push_back("VK_KHR_win32_surface");
    #elif defined(__linux__)
        extensions.push_back("VK_KHR_xlib_surface");
        extensions.push_back("VK_KHR_wayland_surface");
    #elif defined(__APPLE__)
        extensions.push_back("VK_EXT_metal_surface");
    #endif

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    #ifdef __APPLE__
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    return vkCreateInstance(&createInfo, nullptr, &m_instance) == VK_SUCCESS;
}

bool DeferredRenderer::CreateSurface() {
    return SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface);
}

bool DeferredRenderer::SelectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) return false;

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (auto& dev : devices) {
        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queues(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueCount, queues.data());

        for (uint32_t i = 0; i < queueCount; ++i) {
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &presentSupport);
                if (presentSupport) {
                    m_physicalDevice = dev;
                    m_graphicsQueueFamily = i;
                    return true;
                }
            }
        }
    }
    return false;
}

bool DeferredRenderer::CreateDevice() {
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures features{};
    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.pEnabledFeatures = &features;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) return false;
    vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
    return true;
}

bool DeferredRenderer::CreateSwapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f; break;
        }
    }

    m_extent = { m_width, m_height };
    m_extent.width = std::clamp(m_extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    m_extent.height = std::clamp(m_extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

    uint32_t imageCount = std::min(caps.minImageCount + 1, caps.maxImageCount > 0 ? caps.maxImageCount : UINT32_MAX);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) return false;

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapImages.data());

    m_swapViews.resize(m_swapImages.size());
    for (size_t i = 0; i < m_swapImages.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_swapImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = surfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(m_device, &viewInfo, nullptr, &m_swapViews[i]);
    }
    return true;
}

bool DeferredRenderer::CreateGeometryRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    return vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_geometryRenderPass) == VK_SUCCESS;
}

VkShaderModule DeferredRenderer::CreateShaderModule(std::span<const uint32_t> code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule module;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

bool DeferredRenderer::CreateGeometryPipeline() {
    VkShaderModule vertModule = CreateShaderModule(std::span<const uint32_t>(s_pbrVert, sizeof(s_pbrVert)/sizeof(uint32_t)));
    VkShaderModule fragModule = CreateShaderModule(std::span<const uint32_t>(s_pbrFrag, sizeof(s_pbrFrag)/sizeof(uint32_t)));
    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) return false;

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, x);
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(Vertex, r);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.width = static_cast<float>(m_extent.width);
    viewport.height = static_cast<float>(m_extent.height);
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent = m_extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_geometryLayout) != VK_SUCCESS) return false;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_geometryLayout;
    pipelineInfo.renderPass = m_geometryRenderPass;
    pipelineInfo.subpass = 0;

    bool result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_geometryPipeline) == VK_SUCCESS;

    vkDestroyShaderModule(m_device, vertModule, nullptr);
    vkDestroyShaderModule(m_device, fragModule, nullptr);
    return result;
}

bool DeferredRenderer::CreateFramebuffers() {
    m_framebuffers.resize(m_swapViews.size());
    for (size_t i = 0; i < m_swapViews.size(); ++i) {
        VkImageView attachments[] = { m_swapViews[i] };
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_geometryRenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = m_extent.width;
        fbInfo.height = m_extent.height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &fbInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) return false;
    }
    return true;
}

bool DeferredRenderer::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_cmdPool) != VK_SUCCESS) return false;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_cmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    return vkAllocateCommandBuffers(m_device, &allocInfo, &m_cmdBuffer) == VK_SUCCESS;
}

bool DeferredRenderer::CreateSyncObjects() {
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    return vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailable) == VK_SUCCESS &&
           vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinished) == VK_SUCCESS &&
           vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlight) == VK_SUCCESS;
}

uint32_t DeferredRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return 0;
}

void DeferredRenderer::CleanupSwapchain() {
    for (auto& fb : m_framebuffers) {
        if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(m_device, fb, nullptr);
    }
    m_framebuffers.clear();
    for (auto& view : m_swapViews) {
        if (view != VK_NULL_HANDLE) vkDestroyImageView(m_device, view, nullptr);
    }
    m_swapViews.clear();
    m_swapImages.clear();
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

void DeferredRenderer::CleanupGBuffer() {
    for (uint32_t i = 0; i < GBuffer::ATTACHMENT_COUNT; ++i) {
        if (m_gbuffer.views[i] != VK_NULL_HANDLE) vkDestroyImageView(m_device, m_gbuffer.views[i], nullptr);
        if (m_gbuffer.images[i] != VK_NULL_HANDLE) vkDestroyImage(m_device, m_gbuffer.images[i], nullptr);
        if (m_gbuffer.memories[i] != VK_NULL_HANDLE) vkFreeMemory(m_device, m_gbuffer.memories[i], nullptr);
        m_gbuffer.views[i] = VK_NULL_HANDLE;
        m_gbuffer.images[i] = VK_NULL_HANDLE;
        m_gbuffer.memories[i] = VK_NULL_HANDLE;
    }
}

} // namespace TheSeed::Render
