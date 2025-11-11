/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstdio>
#include <vector>

#define MANGO_IMPLEMENT_MAIN
#include <mango/core/core.hpp>
#include <mango/vulkan/vulkan.hpp>
#include <mango/image/image.hpp>

using namespace mango;
using namespace mango::vulkan;
using namespace mango::image;

// ------------------------------------------------------------------------------

class TestWindow : public vulkan::VulkanWindow
{
protected:
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device = VK_NULL_HANDLE;
    VkCommandPool m_commandPool;
    VkQueue m_graphicsQueue;
    uint32_t m_graphicsQueueFamilyIndex;

    std::unique_ptr<vulkan::Swapchain> m_swapchain;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkRenderPass m_renderPass;

    // Texture support
    VkImage m_textureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
    VkImageView m_textureImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    // Pipeline support
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    // Vertex buffer
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    // Image data
    std::unique_ptr<Bitmap> m_bitmap;

    // Vertex structure for full-screen quad
    struct Vertex {
        float pos[2];
        float texCoord[2];
    };

    // Helper functions
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        printLine(Print::Error, "Failed to find suitable memory type!");
        return 0;
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer);
        if (result != VK_SUCCESS) {
            printLine(Print::Error, "Failed to create buffer!");
            return;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        result = vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory);
        if (result != VK_SUCCESS) {
            printLine(Print::Error, "Failed to allocate buffer memory!");
            return;
        }

        vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphicsQueue);

        vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
    }

public:
    TestWindow(VkInstance instance, int width, int height, u32 flags)
        : VulkanWindow(instance, width, height, flags)
    {
        m_physicalDevice = selectPhysicalDevice(instance);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties = getPhysicalDeviceQueueFamilyProperties(m_physicalDevice);

        u32 selectedQueueFamilyIndex = -1;

        for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
        {
            const VkQueueFamilyProperties& properties = queueFamilyProperties[i];

            bool presentationSupport = getPresentationSupport(m_physicalDevice, i);
            bool graphics = properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;

            if (graphics && presentationSupport)
            {
                selectedQueueFamilyIndex = i;
            }
        }

        printLine("selectedQueueFamilyIndex: {}", selectedQueueFamilyIndex);
        printLine("");

        if (selectedQueueFamilyIndex == -1u)
        {
            printLine(Print::Error, "Couldn't find suitable queue.");
            return;
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(m_physicalDevice, &supportedFeatures);

        VkPhysicalDeviceFeatures enabledFeatures = {};
        enabledFeatures.multiDrawIndirect = supportedFeatures.multiDrawIndirect;

        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = selectedQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        const char* enabledExtensions [] =
        {
            "VK_KHR_swapchain"
        };

        VkDeviceCreateInfo deviceCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = enabledExtensions,
            .pEnabledFeatures = &enabledFeatures
        };

        VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
        MANGO_UNREFERENCED(result);

        m_graphicsQueueFamilyIndex = selectedQueueFamilyIndex;
        vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);


        VkQueue presentQueue = m_graphicsQueue; // TODO

        m_swapchain = std::make_unique<vulkan::Swapchain>(m_device, m_physicalDevice, m_surface, presentQueue);

        VkCommandPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
        };

        result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
        MANGO_UNREFERENCED(result);


        m_commandBuffers.resize(m_swapchain->getImageCount());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = m_swapchain->getImageCount(),
        };

        result = vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, m_commandBuffers.data());
        MANGO_UNREFERENCED(result);

        createRenderPass();

        u32 imageCount = m_swapchain->getImageCount();
        m_framebuffers.resize(imageCount);

        VkExtent2D extent = m_swapchain->getExtent();

        for (u32 i = 0; i < m_swapchain->getImageCount(); ++i)
        {
            VkImageView imageView = m_swapchain->getImageView(i);
            VkImageView attachments[] = { imageView };

            VkFramebufferCreateInfo framebufferCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .flags = 0,
                .renderPass = m_renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = extent.width,
                .height = extent.height,
                .layers = 1
            };

            result = vkCreateFramebuffer(m_device, &framebufferCreateInfo, NULL, &m_framebuffers[i]);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "vkCreateFramebuffer: {}", getString(result));
                return;
            }
        }

        createCommandBuffers(extent);
        printLine("");
    }

    ~TestWindow()
    {
        m_swapchain.reset();

        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);

            for (auto framebuffer : m_framebuffers)
            {
                vkDestroyFramebuffer(m_device, framebuffer, nullptr);
            }

            vkDestroyRenderPass(m_device, m_renderPass, nullptr);

            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            vkDestroyDevice(m_device, nullptr);
        }
    }

    void createRenderPass()
    {
        VkFormat format = m_swapchain->getFormat();

        VkAttachmentDescription colorAttachment =
        {
            .format = format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        VkAttachmentReference colorRef = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpass =
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorRef
        };

        VkRenderPassCreateInfo renderPassInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass
        };

        vkCreateRenderPass(m_device, &renderPassInfo, NULL, &m_renderPass);
    }

    void createCommandBuffers(VkExtent2D extent)
    {
        u32 imageCount = m_swapchain->getImageCount();

        for (u32 i = 0; i < imageCount; ++i)
        {
            VkFramebuffer framebuffer = m_framebuffers[i];
            VkCommandBuffer commandBuffer = m_commandBuffers[i];

            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            };

            vkResetCommandBuffer(commandBuffer, 0);
            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            VkClearValue clearColor =
            {
                .color = {{ 0.5f, 1.0f, 0.5f, 1.0f }}
            };

            VkRenderPassBeginInfo renderPassInfo =
            {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = m_renderPass,
                .framebuffer = framebuffer,
                .renderArea = { .offset = {0, 0}, .extent = extent },
                .clearValueCount = 1,
                .pClearValues = &clearColor
            };

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdEndRenderPass(commandBuffer);
            vkEndCommandBuffer(commandBuffer);
        }
    }

    void updateSwapchain()
    {
        if (!m_swapchain->recreateSwapchain())
        {
            return;
        }

        VkExtent2D extent = m_swapchain->getExtent();

        for (u32 i = 0; i < m_swapchain->getImageCount(); ++i)
        {
            vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);

            VkImageView imageView = m_swapchain->getImageView(i);
            VkImageView attachments[] = { imageView };

            VkFramebufferCreateInfo framebufferCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .flags = 0,
                .renderPass = m_renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = extent.width,
                .height = extent.height,
                .layers = 1
            };

            VkResult result = vkCreateFramebuffer(m_device, &framebufferCreateInfo, NULL, &m_framebuffers[i]);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "vkCreateFramebuffer: {}", getString(result));
                return;
            }
        }

        createCommandBuffers(extent);
    }

    void render()
    {
        VkExtent2D extent = m_swapchain->getExtent();
        if (extent.width <= 1 || extent.height <= 1)
        {
            return;
        }

        uint32_t imageIndex = 0;
        VkResult result = m_swapchain->acquireNextImage(imageIndex);
        if (result == VK_SUBOPTIMAL_KHR || result == VK_SUCCESS)
        {
        }
        else
        {
            return;
        }

        VkSemaphore imageAvailableSemaphore = m_swapchain->getImageAvailableSemaphore();
        VkSemaphore renderFinishedSemaphore = m_swapchain->getRenderFinishedSemaphore();
        VkFence fence = m_swapchain->getFence();

        VkCommandBuffer commandBuffer = m_commandBuffers[imageIndex];

        VkPipelineStageFlags waitStages [] =
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };

        VkSubmitInfo submitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &imageAvailableSemaphore,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &renderFinishedSemaphore,
        };

        vkResetFences(m_device, 1, &fence);
        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, fence);

        m_swapchain->present(imageIndex);
    }

    void onResize(int width, int height) override
    {
        printLine("onResize: {} x {}", width, height);
        updateSwapchain();
    }

    void onDraw() override
    {
        u64 time0 = mango::Time::us();

        render();

        u64 time1 = mango::Time::us();
        std::string title = fmt::format("Frame: {}.{} ms", (time1 - time0) / 1000, (time1 - time0) % 1000);
        setTitle(title);
        //printLine("Frame: {}.{} ms", (time1 - time0) / 1000, (time1 - time0) % 1000);
    }

    void onIdle() override
    {
        onDraw();
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        switch (code)
        {
        case KEYCODE_ESC:
            breakEventLoop();
            break;

        case KEYCODE_F:
            toggleFullscreen();
            break;

        default:
            break;
        }
    }
};

// ------------------------------------------------------------------------------

int mangoMain(const mango::CommandLine& commands)
{
    printEnable(Print::Info, true);

    // CreateInstance
    std::vector<VkLayerProperties> layerProperties = enumerateInstanceLayerProperties();
    std::vector<VkExtensionProperties> extensionProperties = enumerateInstanceExtensionProperties();

    std::vector<const char*> enabledLayers;
    std::vector<const char*> enabledExtensions = vulkan::getSurfaceExtensions();

    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");

    VkApplicationInfo applicationInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VulkanTest",
        .applicationVersion = 1,
        .pEngineName = "CustomEngine",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 2, 0),
    };

    Instance instance(applicationInfo, enabledLayers, enabledExtensions);

    // Create Window
    TestWindow window(instance, 1280, 720, 0);

    window.setTitle("Testing!");
    window.enterEventLoop();

    return 0;
}
