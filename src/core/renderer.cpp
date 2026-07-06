#include "core/renderer.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>

// SDL3 Vulkan integration
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace TheSeed::Render {

// --- Inline SPIR-V Shaders (compiled offline, embedded as uint32_t arrays) ---
// Vertex shader: passthrough pos + color
static const uint32_t s_vertShader[] = {
    0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
    0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00090004,0x415f4c47,0x735f4252,
    0x72617065,0x5f657461,0x00000000,0x00050004,0x4f4c4743,0x6f736972,0x00000000,0x00050005,
    0x00000004,0x6e69616d,0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,
    0x0000000b,0x00000000,0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00050006,
    0x0000000b,0x00000001,0x56530000,0x00000050,0x00050006,0x0000000b,0x00000002,0x65746e49,
    0x00000000,0x00060006,0x0000000b,0x00000003,0x6e695f75,0x65726170,0x00000000,0x00030005,
    0x0000000d,0x00000000,0x00040005,0x0000000f,0x6c6f4376,0x0000726f,0x00030005,0x00000015,
    0x00786574,0x00050005,0x0000001b,0x78655476,0x6f6f6c43,0x00000072,0x00040047,0x0000000b,
    0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000000,0x00040047,0x00000015,
    0x0000001e,0x00000000,0x00040047,0x0000001b,0x0000001e,0x00000000,0x00020013,0x00000002,
    0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,
    0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,
    0x00000009,0x00000003,0x00040015,0x0000000a,0x00000020,0x00000001,0x0004002b,0x0000000a,
    0x0000000c,0x00000000,0x0006001c,0x0000000d,0x00000006,0x0000000a,0x0000000b,0x0000000c,
    0x00040020,0x0000000e,0x00000001,0x0000000d,0x0004003b,0x0000000e,0x0000000f,0x00000001,
    0x00040017,0x00000011,0x00000006,0x00000002,0x00040020,0x00000012,0x00000001,0x00000011,
    0x0004003b,0x00000012,0x00000015,0x00000001,0x00040017,0x00000019,0x00000006,0x00000003,
    0x00040020,0x0000001a,0x00000001,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000001,
    0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,
    0x0000000d,0x00000010,0x0000000f,0x00050041,0x00000015,0x00000016,0x00000010,0x0000000c,
    0x0003003e,0x00000016,0x00000015,0x0004003d,0x00000019,0x0000001c,0x0000001b,0x00050041,
    0x00000009,0x0000001d,0x0000001c,0x0000000c,0x0003003e,0x0000001d,0x00000009,0x000100fd,
    0x00010038
};

// Fragment shader: output color
static const uint32_t s_fragShader[] = {
    0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030003,
    0x00000002,0x000001c2,0x00090004,0x415f4c47,0x735f4252,0x72617065,0x5f657461,0x00000000,
    0x00050004,0x4f4c4743,0x6f736972,0x00000000,0x00040005,0x00000004,0x6e69616d,0x00000000,
    0x00040005,0x00000009,0x6f6c6f43,0x00000072,0x00030005,0x0000000b,0x00000000,0x00050006,
    0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00050006,0x0000000b,0x00000001,0x56530000,
    0x00000050,0x00050006,0x0000000b,0x00000002,0x65746e49,0x00000000,0x00060006,0x0000000b,
    0x00000003,0x6e695f75,0x65726170,0x00000000,0x00030005,0x0000000d,0x00000000,0x00040047,
    0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,0x00000000,0x00020013,
    0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,
    0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,
    0x00000008,0x00000009,0x00000003,0x00040015,0x0000000a,0x00000020,0x00000001,0x0004002b,
    0x0000000a,0x0000000c,0x00000000,0x0006001c,0x0000000d,0x00000006,0x0000000a,0x0000000b,
    0x0000000c,0x00040020,0x0000000e,0x00000001,0x0000000d,0x0004003b,0x0000000e,0x0000000d,
    0x00000001,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
    0x0004003d,0x0000000d,0x0000000f,0x0000000d,0x00050041,0x00000009,0x00000010,0x0000000f,
    0x0000000c,0x0003003e,0x00000010,0x00000009,0x000100fd,0x00010038
};

VulkanRenderer::VulkanRenderer() = default;
VulkanRenderer::~VulkanRenderer() { Shutdown(); }

bool VulkanRenderer::Initialize(SDL_Window* window, uint32_t width, uint32_t height) {
    m_window = window;

    if (!CreateInstance()) return false;
    if (!CreateSurface()) return false;
    if (!SelectPhysicalDevice()) return false;
    if (!CreateDevice()) return false;
    if (!CreateSwapchain(width, height)) return false;
    if (!CreateRenderPass()) return false;
    if (!CreatePipeline()) return false;
    if (!CreateFramebuffers()) return false;
    if (!CreateCommandPool()) return false;
    if (!CreateSyncObjects()) return false;

    m_initialized = true;
    spdlog::info("VulkanRenderer initialisiert: {}x{}", width, height);
    return true;
}

void VulkanRenderer::Shutdown() {
    if (!m_initialized && m_device == VK_NULL_HANDLE) return;

    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }

    CleanupSwapchain();

    if (m_indexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    if (m_indexMemory != VK_NULL_HANDLE) vkFreeMemory(m_device, m_indexMemory, nullptr);
    if (m_vertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    if (m_vertexMemory != VK_NULL_HANDLE) vkFreeMemory(m_device, m_vertexMemory, nullptr);

    if (m_inFlight != VK_NULL_HANDLE) vkDestroyFence(m_device, m_inFlight, nullptr);
    if (m_renderFinished != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_renderFinished, nullptr);
    if (m_imageAvailable != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_imageAvailable, nullptr);
    if (m_cmdPool != VK_NULL_HANDLE) vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
    if (m_pipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_device, m_pipeline, nullptr);
    if (m_pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    if (m_renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    if (m_device != VK_NULL_HANDLE) vkDestroyDevice(m_device, nullptr);
    if (m_surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    if (m_instance != VK_NULL_HANDLE) vkDestroyInstance(m_instance, nullptr);

    m_initialized = false;
    spdlog::info("VulkanRenderer heruntergefahren");
}

bool VulkanRenderer::CreateInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "TheSeed";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "TheSeed";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // SDL3 liefert Vulkan-Extensions automatisch
    // Wir nutzen nur die minimalen Extensions
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

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        spdlog::error("vkCreateInstance fehlgeschlagen");
        return false;
    }
    return true;
}

bool VulkanRenderer::CreateSurface() {
    if (!SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface)) {
        spdlog::error("SDL_Vulkan_CreateSurface fehlgeschlagen");
        return false;
    }
    return true;
}

bool VulkanRenderer::SelectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        spdlog::error("Keine Vulkan-faehige GPU gefunden");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

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
                    spdlog::info("GPU gewaehlt: {}", props.deviceName);
                    return true;
                }
            }
        }
    }

    spdlog::error("Keine GPU mit Graphics+Present gefunden");
    return false;
}

bool VulkanRenderer::CreateDevice() {
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

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        spdlog::error("vkCreateDevice fehlgeschlagen");
        return false;
    }

    vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
    return true;
}

bool VulkanRenderer::CreateSwapchain(uint32_t width, uint32_t height) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            break;
        }
    }

    VkExtent2D extent = { width, height };
    extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    m_extent = extent;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        spdlog::error("vkCreateSwapchainKHR fehlgeschlagen");
        return false;
    }

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

bool VulkanRenderer::CreateRenderPass() {
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

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        spdlog::error("vkCreateRenderPass fehlgeschlagen");
        return false;
    }
    return true;
}

VkShaderModule VulkanRenderer::CreateShaderModule(std::span<const uint32_t> code) {
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

bool VulkanRenderer::CreatePipeline() {
    VkShaderModule vertModule = CreateShaderModule(std::span(s_vertShader));
    VkShaderModule fragModule = CreateShaderModule(std::span(s_fragShader));
    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        spdlog::error("Shader-Module Erstellung fehlgeschlagen");
        return false;
    }

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
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        spdlog::error("vkCreatePipelineLayout fehlgeschlagen");
        return false;
    }

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
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        spdlog::error("vkCreateGraphicsPipelines fehlgeschlagen");
        return false;
    }

    vkDestroyShaderModule(m_device, vertModule, nullptr);
    vkDestroyShaderModule(m_device, fragModule, nullptr);
    return true;
}

bool VulkanRenderer::CreateFramebuffers() {
    m_framebuffers.resize(m_swapViews.size());
    for (size_t i = 0; i < m_swapViews.size(); ++i) {
        VkImageView attachments[] = { m_swapViews[i] };
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = m_extent.width;
        fbInfo.height = m_extent.height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &fbInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            spdlog::error("vkCreateFramebuffer fehlgeschlagen");
            return false;
        }
    }
    return true;
}

bool VulkanRenderer::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_cmdPool) != VK_SUCCESS) {
        spdlog::error("vkCreateCommandPool fehlgeschlagen");
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_cmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_cmdBuffer) != VK_SUCCESS) {
        spdlog::error("vkAllocateCommandBuffers fehlgeschlagen");
        return false;
    }
    return true;
}

bool VulkanRenderer::CreateSyncObjects() {
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinished) != VK_SUCCESS ||
        vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlight) != VK_SUCCESS) {
        spdlog::error("Sync-Object Erstellung fehlgeschlagen");
        return false;
    }
    return true;
}

uint32_t VulkanRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return 0;
}

bool VulkanRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                                   VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_device, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, props);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        return false;
    }

    vkBindBufferMemory(m_device, buffer, memory, 0);
    return true;
}

bool VulkanRenderer::CreateVertexBuffer(const std::vector<Vertex>& vertices) {
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        vkFreeMemory(m_device, m_vertexMemory, nullptr);
    }

    VkDeviceSize size = vertices.size() * sizeof(Vertex);
    if (!CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       m_vertexBuffer, m_vertexMemory)) {
        return false;
    }

    void* data;
    vkMapMemory(m_device, m_vertexMemory, 0, size, 0, &data);
    std::memcpy(data, vertices.data(), size);
    vkUnmapMemory(m_device, m_vertexMemory);
    m_vertexCount = vertices.size();
    return true;
}

bool VulkanRenderer::CreateIndexBuffer(const std::vector<uint32_t>& indices) {
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        vkFreeMemory(m_device, m_indexMemory, nullptr);
    }

    VkDeviceSize size = indices.size() * sizeof(uint32_t);
    if (!CreateBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       m_indexBuffer, m_indexMemory)) {
        return false;
    }

    void* data;
    vkMapMemory(m_device, m_indexMemory, 0, size, 0, &data);
    std::memcpy(data, indices.data(), size);
    vkUnmapMemory(m_device, m_indexMemory);
    m_indexCount = indices.size();
    return true;
}

void VulkanRenderer::CleanupSwapchain() {
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

bool VulkanRenderer::RecreateSwapchain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(m_device);
    CleanupSwapchain();
    if (!CreateSwapchain(width, height)) return false;
    if (!CreateFramebuffers()) return false;
    return true;
}

void VulkanRenderer::SetClearColor(float r, float g, float b, float a) {
    m_clearR = r; m_clearG = g; m_clearB = b; m_clearA = a;
}

bool VulkanRenderer::BeginFrame() {
    vkWaitForFences(m_device, 1, &m_inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlight);

    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailable, VK_NULL_HANDLE, &m_imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        uint32_t w, h;
        SDL_GetWindowSizeInPixels(m_window, reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h));
        RecreateSwapchain(w, h);
        return false;
    }

    vkResetCommandBuffer(m_cmdBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_cmdBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[m_imageIndex];
    renderPassInfo.renderArea.extent = m_extent;

    VkClearValue clearColor{};
    clearColor.color.float32[0] = m_clearR;
    clearColor.color.float32[1] = m_clearG;
    clearColor.color.float32[2] = m_clearB;
    clearColor.color.float32[3] = m_clearA;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(m_cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport viewport{};
    viewport.width = static_cast<float>(m_extent.width);
    viewport.height = static_cast<float>(m_extent.height);
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = m_extent;
    vkCmdSetScissor(m_cmdBuffer, 0, 1, &scissor);

    return true;
}

void VulkanRenderer::EndFrame() {
    vkCmdEndRenderPass(m_cmdBuffer);
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

    VkResult result = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        uint32_t w, h;
        SDL_GetWindowSizeInPixels(m_window, reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h));
        RecreateSwapchain(w, h);
    }
}

void VulkanRenderer::SubmitMesh(const Mesh& mesh) {
    CreateVertexBuffer(mesh.vertices);
    CreateIndexBuffer(mesh.indices);

    VkBuffer vb = m_vertexBuffer;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_cmdBuffer, 0, 1, &vb, &offset);
    vkCmdBindIndexBuffer(m_cmdBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(m_cmdBuffer, static_cast<uint32_t>(m_indexCount), 1, 0, 0, 0);
}

void VulkanRenderer::DrawTriangle() {
    Mesh tri;
    tri.vertices = {
        { 0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f },
        { 0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f },
        { -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f },
    };
    tri.indices = { 0, 1, 2 };
    SubmitMesh(tri);
}

void VulkanRenderer::DrawQuad() {
    Mesh quad;
    quad.vertices = {
        { -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f },
        {  0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f },
        {  0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f },
        { -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f },
    };
    quad.indices = { 0, 1, 2, 0, 2, 3 };
    SubmitMesh(quad);
}

void VulkanRenderer::DrawCube() {
    Mesh cube;
    cube.vertices = {
        // Front face (red)
        { -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f },
        {  0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f },
        {  0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f },
        { -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f },
        // Back face (green)
        { -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f },
        {  0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f },
        {  0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f },
        { -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f },
    };
    cube.indices = {
        // Front
        0, 1, 2, 0, 2, 3,
        // Back
        4, 6, 5, 4, 7, 6,
        // Left
        4, 0, 3, 4, 3, 7,
        // Right
        1, 5, 6, 1, 6, 2,
        // Top
        3, 2, 6, 3, 6, 7,
        // Bottom
        4, 5, 1, 4, 1, 0,
    };
    SubmitMesh(cube);
}

std::unique_ptr<VulkanRenderer> CreateRenderer() {
    return std::make_unique<VulkanRenderer>();
}

} // namespace TheSeed::Render
