/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#define MANGO_IMPLEMENT_MAIN

#include <mango/core/core.hpp>
#include <mango/math/math.hpp>
#include <mango/vulkan/vulkan.hpp>
#include <mango/vulkan/allocator.hpp>
#include <mango/vulkan/compiler.hpp>
#include <mango/vulkan/font.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::vulkan;

namespace
{

    const char* g_resolve_vertex_shader = R"(#version 450
layout(location = 0) out vec2 vTexcoord;

vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

vec2 texcoords[3] = vec2[](
    vec2(0.0, 0.0),
    vec2(2.0, 0.0),
    vec2(0.0, 2.0)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vTexcoord = texcoords[gl_VertexIndex];
}
)";

    const char* g_resolve_fragment_shader = R"(#version 450
layout(location = 0) in vec2 vTexcoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTexture;

void main()
{
    outColor = texture(uTexture, vTexcoord);
}
)";

    void cmdImageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout& trackedLayout,
                         VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                         VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
    {
        if (trackedLayout == newLayout && srcAccess == 0 && dstAccess == 0)
        {
            return;
        }

        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = srcAccess,
            .dstAccessMask = dstAccess,
            .oldLayout = trackedLayout,
            .newLayout = newLayout,
            .image = image,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        trackedLayout = newLayout;
    }

} // namespace

class FontWindow : public VulkanWindow
{
protected:
    static constexpr VkFormat kRenderTargetFormat = VK_FORMAT_R8G8B8A8_UNORM;

    std::unique_ptr<Allocator> m_allocator;
    std::unique_ptr<FontRenderer> m_renderer;
    Font m_body;
    Font m_hud;

    float m_fontPixelHeight = 32.0f;

    static constexpr size_t kFrameTimeHistory = 60;

    float m_frameTimeMs = 0.0f;
    float m_queueTimeMs = 0.0f;
    float m_encodeTimeMs = 0.0f;
    std::array<float, kFrameTimeHistory> m_frameTimeHistory {};
    size_t m_frameTimeIndex = 0;
    size_t m_frameTimeCount = 0;
    std::string m_hudLine;

    std::vector<std::string> m_lines =
    {
        "An ancient hero from the waters, from the waves he came while singing.",
        "Ready now I bring my singing, ready now my incantations.",
        "Ilmarinen, smith immortal, forged the wonder-grinder Sampo.",
        "Wainamoinen, old and truthful, sang the birth of ancient wisdom.",
        "Let the dead rest in the darkness, let the living wander light-foot.",
        "Thus began the ancient legends, thus began the old traditions.",
        "Runo after runo he counted, spell on spell he laid in order.",
        "From the forge of the Creator, from the hammer of the Maker.",
        "Never yet was born a singer, never will there be his equal.",
        "Ahti, ruler of the waters, lord of every lake and island.",
    };

    std::string m_bodyFontPath;

    static constexpr const char* kHudFontPath = "data/NotoSans-Regular.ttf";

    ImageAllocation m_renderTarget;
    VkImageView m_renderTargetView = VK_NULL_HANDLE;
    VkExtent2D m_renderExtent {};
    VkImageLayout m_renderTargetLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkShaderModule m_resolveVertexShader = VK_NULL_HANDLE;
    VkShaderModule m_resolveFragmentShader = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_resolveDescriptorLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_resolvePipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_resolvePipeline = VK_NULL_HANDLE;
    VkDescriptorPool m_resolveDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_resolveDescriptorSet = VK_NULL_HANDLE;
    VkSampler m_resolveSampler = VK_NULL_HANDLE;
    VkFormat m_swapchainFormat = VK_FORMAT_UNDEFINED;

public:
    FontWindow(VkInstance instance, int width, int height, u32 flags, const std::string& bodyFontPath)
        : VulkanWindow(instance, width, height, flags)
        , m_bodyFontPath(bodyFontPath)
    {
    }

    void onDeviceReady() override
    {
        m_allocator = std::make_unique<Allocator>(instance(), m_physicalDevice, m_device, VK_API_VERSION_1_3);

        FontRenderer::CreateInfo fontInfo =
        {
            .physicalDevice = m_physicalDevice,
            .device = m_device,
            .queue = m_graphicsQueue,
            .queueFamily = m_graphicsQueueFamilyIndex,
            .allocator = m_allocator.get(),
        };

        m_renderer = std::make_unique<FontRenderer>(fontInfo);

        m_hud = m_renderer->load(kHudFontPath);
        if (!m_hud)
        {
            printLine(Print::Error, "Failed to load HUD font: {}", kHudFontPath);
        }

        m_body = m_renderer->load(m_bodyFontPath);
        if (!m_body)
        {
            printLine(Print::Error, "Failed to load body font: {}", m_bodyFontPath);
        }
        else
        {
            m_renderer->setSize(m_body, m_fontPixelHeight);
        }

        m_swapchainFormat = swapchain().getFormat();
        recreateRenderResources(swapchainExtent());
    }

    void onSwapchainResize(VkExtent2D extent) override
    {
        if (extent.width > 0 && extent.height > 0)
        {
            m_swapchainFormat = swapchain().getFormat();
            recreateRenderResources(extent);
        }
    }

    ~FontWindow()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
            destroyResolvePass();
            destroyRenderTarget();
            m_renderer.reset();
            m_allocator.reset();
        }
    }

    void recreateRenderResources(VkExtent2D extent)
    {
        if (!m_allocator || extent.width == 0 || extent.height == 0)
        {
            return;
        }

        vkDeviceWaitIdle(m_device);

        destroyResolvePass();
        destroyRenderTarget();

        m_renderExtent = extent;
        m_swapchainFormat = swapchain().getFormat();
        createRenderTarget(extent);
        createResolvePass();

        if (m_renderer)
        {
            m_renderer->resize(extent);
        }
    }

    void createRenderTarget(VkExtent2D extent)
    {
        VkImageCreateInfo imageInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = kRenderTargetFormat,
            .extent = { extent.width, extent.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        m_renderTarget = m_allocator->createImage(imageInfo, MemoryUsage::GpuOnly);

        VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_renderTarget.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = kRenderTargetFormat,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCreateImageView(m_device, &viewInfo, nullptr, &m_renderTargetView);
        m_renderTargetLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkCommandPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
        };

        VkCommandPool pool = VK_NULL_HANDLE;
        vkCreateCommandPool(m_device, &poolInfo, nullptr, &pool);

        VkCommandBufferAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(m_device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(cmd, &beginInfo);
        cmdImageBarrier(cmd, m_renderTarget.image, m_renderTargetLayout,
            VK_IMAGE_LAYOUT_GENERAL, 0, VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        vkEndCommandBuffer(cmd);

        VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VkFence fence = VK_NULL_HANDLE;
        vkCreateFence(m_device, &fenceInfo, nullptr, &fence);

        VkSubmitInfo submitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        };

        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, fence);
        vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);

        vkDestroyFence(m_device, fence, nullptr);
        vkFreeCommandBuffers(m_device, pool, 1, &cmd);
        vkDestroyCommandPool(m_device, pool, nullptr);
    }

    void destroyRenderTarget()
    {
        if (m_renderTargetView)
        {
            vkDestroyImageView(m_device, m_renderTargetView, nullptr);
            m_renderTargetView = VK_NULL_HANDLE;
        }

        if (m_renderTarget)
        {
            m_allocator->destroyImage(m_renderTarget);
            m_renderTarget = {};
        }

        m_renderExtent = {};
        m_renderTargetLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void createResolvePass()
    {
        if (m_swapchainFormat == VK_FORMAT_UNDEFINED)
        {
            printLine(Print::Error, "Font example: swapchain format not available for resolve pass.");
            return;
        }

        Compiler compiler;
        Shader vs = compiler.compile(g_resolve_vertex_shader, ShaderStage::Vertex);
        Shader fs = compiler.compile(g_resolve_fragment_shader, ShaderStage::Fragment);

        if (!vs.valid() || !fs.valid())
        {
            printLine(Print::Error, "Font example resolve shader compilation failed.");
            return;
        }

        m_resolveVertexShader = Compiler::createShaderModule(m_device, vs);
        m_resolveFragmentShader = Compiler::createShaderModule(m_device, fs);

        VkDescriptorSetLayoutBinding binding =
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &binding,
        };

        vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_resolveDescriptorLayout);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_resolveDescriptorLayout,
        };

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_resolvePipelineLayout);

        VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
        VkDescriptorPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
        };

        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_resolveDescriptorPool);

        VkDescriptorSetAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_resolveDescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_resolveDescriptorLayout,
        };

        vkAllocateDescriptorSets(m_device, &allocInfo, &m_resolveDescriptorSet);

        VkSamplerCreateInfo samplerInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        };

        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_resolveSampler);

        VkDescriptorImageInfo imageInfo =
        {
            .sampler = m_resolveSampler,
            .imageView = m_renderTargetView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet write =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_resolveDescriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
        };

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        VkPipelineShaderStageCreateInfo stages[] =
        {
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = m_resolveVertexShader, .pName = "main" },
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = m_resolveFragmentShader, .pName = "main" },
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
        VkPipelineViewportStateCreateInfo viewportState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1 };
        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = 2, .pDynamicStates = dynamicStates };
        VkPipelineRasterizationStateCreateInfo rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE, .lineWidth = 1.0f };
        VkPipelineMultisampleStateCreateInfo multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT };
        VkPipelineColorBlendAttachmentState blendAttachment { .colorWriteMask = 0xF };
        VkPipelineColorBlendStateCreateInfo blending = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &blendAttachment };

        VkFormat colorFormat = m_swapchainFormat;
        VkPipelineRenderingCreateInfo renderingCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat,
        };

        VkGraphicsPipelineCreateInfo pipelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingCreateInfo,
            .stageCount = 2,
            .pStages = stages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &blending,
            .pDynamicState = &dynamicState,
            .layout = m_resolvePipelineLayout,
        };

        vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_resolvePipeline);
    }

    void destroyResolvePass()
    {
        if (m_resolvePipeline)
        {
            vkDestroyPipeline(m_device, m_resolvePipeline, nullptr);
            m_resolvePipeline = VK_NULL_HANDLE;
        }

        if (m_resolvePipelineLayout)
        {
            vkDestroyPipelineLayout(m_device, m_resolvePipelineLayout, nullptr);
            m_resolvePipelineLayout = VK_NULL_HANDLE;
        }

        if (m_resolveDescriptorPool)
        {
            vkDestroyDescriptorPool(m_device, m_resolveDescriptorPool, nullptr);
            m_resolveDescriptorPool = VK_NULL_HANDLE;
        }

        if (m_resolveDescriptorLayout)
        {
            vkDestroyDescriptorSetLayout(m_device, m_resolveDescriptorLayout, nullptr);
            m_resolveDescriptorLayout = VK_NULL_HANDLE;
        }

        if (m_resolveSampler)
        {
            vkDestroySampler(m_device, m_resolveSampler, nullptr);
            m_resolveSampler = VK_NULL_HANDLE;
        }

        if (m_resolveVertexShader)
        {
            vkDestroyShaderModule(m_device, m_resolveVertexShader, nullptr);
            m_resolveVertexShader = VK_NULL_HANDLE;
        }

        if (m_resolveFragmentShader)
        {
            vkDestroyShaderModule(m_device, m_resolveFragmentShader, nullptr);
            m_resolveFragmentShader = VK_NULL_HANDLE;
        }

        m_resolveDescriptorSet = VK_NULL_HANDLE;
    }

    void buildText()
    {
        m_renderer->beginFrame();

        TextStyle hudStyle { .color = float32x4(0.75f, 0.9f, 1.0f, 1.0f), .pixelHeight = 32.0f };
        TextCursor hud = m_renderer->cursorTopLeft(m_hud, 8.0f, 32.0f, hudStyle);
        m_renderer->draw(hud, m_hud, m_hudLine, hudStyle);

        TextStyle bodyStyle { .color = float32x4(1.0f, 1.0f, 1.0f, 1.0f), .pixelHeight = m_fontPixelHeight };
        TextCursor body = m_renderer->cursorTopLeft(m_body, 40.0f, 132.0f, bodyStyle);

        for (const std::string& line : m_lines)
        {
            m_renderer->drawLine(body, m_body, line, bodyStyle);
        }
    }

    void clearRenderTarget(VkCommandBuffer cmd)
    {
        if (!m_renderTarget)
        {
            return;
        }

        VkClearColorValue clearValue {};
        clearValue.float32[0] = 0.1f;
        clearValue.float32[1] = 0.14f;
        clearValue.float32[2] = 0.23f;
        clearValue.float32[3] = 1.0f;

        VkImageSubresourceRange range =
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        vkCmdClearColorImage(cmd, m_renderTarget.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &range);

        VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = m_renderTarget.image,
            .subresourceRange = range,
        };

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void resolveToSwapchain(VkCommandBuffer cmd, u32 imageIndex, VkExtent2D extent)
    {
        if (!m_renderTarget || !m_resolvePipeline)
        {
            return;
        }

        cmdImageBarrier(cmd, m_renderTarget.image, m_renderTargetLayout,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        swapchain().cmdTransitionImageToColorAttachment(cmd, imageIndex);

        VkRenderingAttachmentInfo colorAttachment =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain().getImageView(imageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };

        VkRect2D renderArea = { .extent = extent };

        VkRenderingInfo renderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };

        vkCmdBeginRendering(cmd, &renderingInfo);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resolvePipeline);

        VkViewport viewport =
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = float(extent.width),
            .height = float(extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &renderArea);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resolvePipelineLayout, 0, 1, &m_resolveDescriptorSet, 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);

        swapchain().cmdTransitionImageToPresent(cmd, imageIndex);

        cmdImageBarrier(cmd, m_renderTarget.image, m_renderTargetLayout,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }

    void recordCommandBuffer(VkCommandBuffer cmd, u32 imageIndex, VkExtent2D extent)
    {
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(cmd, &beginInfo);

        const u64 queue_begin = Time::us();

        buildText();

        m_queueTimeMs = float(Time::us() - queue_begin) / 1000.0f;

        clearRenderTarget(cmd);

        const u64 encode_begin = Time::us();
        m_renderer->encode(cmd,
        {
            .imageView = m_renderTargetView,
            .extent = extent,
        });
        m_encodeTimeMs = float(Time::us() - encode_begin) / 1000.0f;

        resolveToSwapchain(cmd, imageIndex, extent);

        vkEndCommandBuffer(cmd);
    }

    void render()
    {
        auto frame = beginDraw();
        if (!frame || !m_renderTarget)
        {
            return;
        }

        const VkExtent2D extent = swapchainExtent();
        VkCommandBuffer cmd = commandBuffer(frame.imageIndex());
        recordCommandBuffer(cmd, frame.imageIndex(), extent);
        frame.submitAndPresent(m_graphicsQueue, cmd);
    }

    void onFrame(const FrameInfo& info) override
    {
        constexpr float min_size = 16.0f;
        constexpr float max_size = 128.0f;
        constexpr float cycle_seconds = 18.0f;

        const float phase = float(std::fmod(info.time, double(cycle_seconds))) / cycle_seconds;
        const float t = 0.5f + 0.5f * std::sin(phase * float(2.0 * 3.14159265358979323846) - float(3.14159265358979323846) * 0.5f);
        m_fontPixelHeight = min_size + t * (max_size - min_size);
        m_renderer->setSize(m_body, m_fontPixelHeight);

        const float frame_ms = float(info.dt * 1000.0);
        m_frameTimeHistory[m_frameTimeIndex] = frame_ms;
        m_frameTimeIndex = (m_frameTimeIndex + 1) % kFrameTimeHistory;
        m_frameTimeCount = std::min(m_frameTimeCount + 1, kFrameTimeHistory);

        float frame_sum = 0.0f;
        for (size_t i = 0; i < m_frameTimeCount; ++i)
        {
            frame_sum += m_frameTimeHistory[i];
        }
        m_frameTimeMs = frame_sum / float(m_frameTimeCount);

        const float fps = 1000.0f / std::max(m_frameTimeMs, 0.001f);
        m_hudLine = fmt::format("frame: {:6.3f} ms ({:3.0f} fps)  queue: {:5.3f} ms  encode: {:5.3f} ms",
            m_frameTimeMs, fps, m_queueTimeMs, m_encodeTimeMs);

        render();
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        MANGO_UNREFERENCED(mask);

        if (code == KEYCODE_ESC)
        {
            breakEventLoop();
        }
        else if (code == KEYCODE_F)
        {
            toggleFullscreen();
        }
    }
};

int mangoMain(const mango::CommandLine& commands)
{
    std::string bodyFontPath = "data/GreatVibes-Regular.ttf";

    std::vector<const char*> enabledLayers;

    for (size_t i = 1; i < commands.size(); ++i)
    {
        std::string arg = std::string(commands[i]);
        if (arg[0] != '-')
        {
            bodyFontPath = arg;
        }
        else if (arg == "--info")
        {
            printEnable(Print::Info, true);
        }
        else if (arg == "--validate")
        {
            enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
    }

    std::vector<const char*> enabledExtensions = vulkan::requiredSurfaceExtensions();

    InstanceExtensionProperties instanceExtensionProperties;
    if (instanceExtensionProperties.contains(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME))
    {
        enabledExtensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    }

    VkApplicationInfo applicationInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "font",
        .applicationVersion = 1,
        .pEngineName = "mango",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    Instance instance(applicationInfo, enabledLayers, enabledExtensions);

    FontWindow window(instance, 1280, 720, 0, bodyFontPath);
    window.setTitle("Scanline Sweeper Font (Vulkan)");

    EventLoopConfig config;
    config.mode = FrameMode::Continuous;
    config.waitForFrame = true;

    window.enterEventLoop(config);

    return 0;
}
