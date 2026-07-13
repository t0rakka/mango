/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/math/math.hpp>
#include <mango/vulkan/vulkan.hpp>
#include <mango/vulkan/render_target.hpp>

#include <cstring>
#include <memory>

#include <mango/core/exception.hpp>
#include <mango/core/print.hpp>
#include <mango/vulkan/compiler.hpp>

namespace mango::vulkan
{

    using mango::math::float32x4;

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

        const char* g_pass_fragment_shader = R"(#version 450
layout(location = 0) in vec2 vTexcoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTexture;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    outColor = texelFetch(uTexture, pixel, 0);
}
)";

        const char* g_linear_fragment_preamble = R"(#version 450
layout(location = 0) in vec2 vTexcoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTexture;
)";

        const char* g_linear_fragment_body = R"(

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 src = texelFetch(uTexture, pixel, 0);
    outColor = encodeOutput(src);
}
)";

        VkFormat toVkFormat(RenderTargetFormat format)
        {
            switch (format)
            {
                case RenderTargetFormat::UNORM8:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case RenderTargetFormat::SRGB8:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case RenderTargetFormat::Float16:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
            }

            return VK_FORMAT_UNDEFINED;
        }

        bool surfaceFormatEquals(VkSurfaceFormatKHR a, VkSurfaceFormatKHR b)
        {
            return a.format == b.format && a.colorSpace == b.colorSpace;
        }

        void imageBarrier(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout& trackedLayout,
                          VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                          VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
        {
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

            vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            trackedLayout = newLayout;
        }

    } // namespace

    struct RenderTarget::Impl
    {
        VkDevice device = VK_NULL_HANDLE;
        Allocator* allocator = nullptr;
        VkQueue queue = VK_NULL_HANDLE;
        u32 queueFamily = 0;
        RenderTargetFormat targetFormat = RenderTargetFormat::UNORM8;
        VkFormat imageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D extent {};

        ImageAllocation imageAllocation;
        VkImageView imageView = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkShaderModule resolveVertexShader = VK_NULL_HANDLE;
        VkShaderModule resolveFragmentShader = VK_NULL_HANDLE;
        VkDescriptorSetLayout resolveDescriptorLayout = VK_NULL_HANDLE;
        VkPipelineLayout resolvePipelineLayout = VK_NULL_HANDLE;
        VkPipeline resolvePipeline = VK_NULL_HANDLE;
        VkDescriptorPool resolveDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet resolveDescriptorSet = VK_NULL_HANDLE;
        VkSampler resolveSampler = VK_NULL_HANDLE;

        VkFormat resolveSwapchainFormat = VK_FORMAT_UNDEFINED;
        VkSurfaceFormatKHR resolveSurfaceFormat {};

        Impl(const CreateInfo& info)
            : device(info.device)
            , allocator(info.allocator)
            , queue(info.queue)
            , queueFamily(info.queueFamily)
            , targetFormat(info.format)
            , imageFormat(toVkFormat(info.format))
            , extent(info.extent)
        {
            if (!device || !allocator || !queue || imageFormat == VK_FORMAT_UNDEFINED)
            {
                MANGO_EXCEPTION("[RenderTarget] Invalid CreateInfo.");
            }

            if (extent.width > 0 && extent.height > 0)
            {
                createImage();
            }
        }

        ~Impl()
        {
            if (device == VK_NULL_HANDLE)
            {
                return;
            }

            vkDeviceWaitIdle(device);
            destroyResolvePipeline();
            destroyImage();
        }

        bool valid() const noexcept
        {
            return imageAllocation && imageView != VK_NULL_HANDLE;
        }

        void resize(VkExtent2D newExtent)
        {
            if (newExtent.width == 0 || newExtent.height == 0)
            {
                return;
            }

            if (newExtent.width == extent.width && newExtent.height == extent.height && valid())
            {
                return;
            }

            vkDeviceWaitIdle(device);
            destroyResolvePipeline();
            destroyImage();

            extent = newExtent;
            createImage();
        }

        void createImage()
        {
            VkImageCreateInfo imageInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = imageFormat,
                .extent = { extent.width, extent.height, 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            imageAllocation = allocator->createImage(imageInfo, MemoryUsage::GpuOnly);
            if (!imageAllocation)
            {
                printLine(Print::Error, "RenderTarget: image allocation failed.");
                return;
            }

            VkImageViewCreateInfo viewInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = imageAllocation.image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = imageFormat,
                .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
            if (result != VK_SUCCESS)
            {
                printLine(Print::Error, "RenderTarget: vkCreateImageView failed: {}", getString(result));
                allocator->destroyImage(imageAllocation);
                imageAllocation = {};
                return;
            }

            imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkCommandPoolCreateInfo poolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
            };

            VkCommandPool pool = VK_NULL_HANDLE;
            vkCreateCommandPool(device, &poolInfo, nullptr, &pool);

            VkCommandBufferAllocateInfo allocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            VkCommandBuffer cmd = VK_NULL_HANDLE;
            vkAllocateCommandBuffers(device, &allocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };

            vkBeginCommandBuffer(cmd, &beginInfo);
            imageBarrier(cmd, imageAllocation.image, imageLayout,
                VK_IMAGE_LAYOUT_GENERAL, 0, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            vkEndCommandBuffer(cmd);

            VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            VkFence fence = VK_NULL_HANDLE;
            vkCreateFence(device, &fenceInfo, nullptr, &fence);

            VkSubmitInfo submitInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &cmd,
            };

            vkQueueSubmit(queue, 1, &submitInfo, fence);
            vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

            vkDestroyFence(device, fence, nullptr);
            vkFreeCommandBuffers(device, pool, 1, &cmd);
            vkDestroyCommandPool(device, pool, nullptr);
        }

        void destroyImage()
        {
            if (imageView)
            {
                vkDestroyImageView(device, imageView, nullptr);
                imageView = VK_NULL_HANDLE;
            }

            if (imageAllocation)
            {
                allocator->destroyImage(imageAllocation);
                imageAllocation = {};
            }

            extent = {};
            imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        void destroyResolvePipeline()
        {
            if (resolvePipeline)
            {
                vkDestroyPipeline(device, resolvePipeline, nullptr);
                resolvePipeline = VK_NULL_HANDLE;
            }

            if (resolvePipelineLayout)
            {
                vkDestroyPipelineLayout(device, resolvePipelineLayout, nullptr);
                resolvePipelineLayout = VK_NULL_HANDLE;
            }

            if (resolveDescriptorPool)
            {
                vkDestroyDescriptorPool(device, resolveDescriptorPool, nullptr);
                resolveDescriptorPool = VK_NULL_HANDLE;
            }

            if (resolveDescriptorLayout)
            {
                vkDestroyDescriptorSetLayout(device, resolveDescriptorLayout, nullptr);
                resolveDescriptorLayout = VK_NULL_HANDLE;
            }

            if (resolveSampler)
            {
                vkDestroySampler(device, resolveSampler, nullptr);
                resolveSampler = VK_NULL_HANDLE;
            }

            if (resolveVertexShader)
            {
                vkDestroyShaderModule(device, resolveVertexShader, nullptr);
                resolveVertexShader = VK_NULL_HANDLE;
            }

            if (resolveFragmentShader)
            {
                vkDestroyShaderModule(device, resolveFragmentShader, nullptr);
                resolveFragmentShader = VK_NULL_HANDLE;
            }

            resolveDescriptorSet = VK_NULL_HANDLE;
            resolveSwapchainFormat = VK_FORMAT_UNDEFINED;
            resolveSurfaceFormat = {};
        }

        void updateResolveDescriptor()
        {
            if (!resolveDescriptorSet || !imageView)
            {
                return;
            }

            VkDescriptorImageInfo imageInfo =
            {
                .sampler = resolveSampler,
                .imageView = imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VkWriteDescriptorSet write =
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = resolveDescriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }

        void ensureResolvePipeline(Swapchain& swapchain, const OutputTransformOptions* colorOptions)
        {
            const VkSurfaceFormatKHR surfaceFormat = swapchain.getSurfaceFormat();
            const VkFormat swapchainFormat = swapchain.getFormat();

            if (resolvePipeline)
            {
                if (targetFormat == RenderTargetFormat::Float16)
                {
                    if (surfaceFormatEquals(resolveSurfaceFormat, surfaceFormat))
                    {
                        return;
                    }
                }
                else if (resolveSwapchainFormat == swapchainFormat)
                {
                    return;
                }
            }

            destroyResolvePipeline();

            Compiler compiler;
            Shader vs = compiler.compile(g_resolve_vertex_shader, ShaderStage::Vertex);
            if (!vs)
            {
                printLine(Print::Error, "RenderTarget: resolve vertex shader failed.");
                return;
            }

            std::string fragmentSource;
            if (targetFormat == RenderTargetFormat::Float16)
            {
                OutputTransformOptions options = colorOptions
                    ? *colorOptions
                    : defaultOutputOptions(surfaceFormat);
                fragmentSource = g_linear_fragment_preamble;
                fragmentSource += getOutputTransformGLSL(surfaceFormat, options);
                fragmentSource += g_linear_fragment_body;
            }
            else
            {
                fragmentSource = g_pass_fragment_shader;
            }

            Shader fs = compiler.compile(fragmentSource, ShaderStage::Fragment);
            if (!fs)
            {
                printLine(Print::Error, "RenderTarget: resolve fragment shader failed.");
                if (!fs.log.empty())
                {
                    printLine(Print::Error, "{}", fs.log);
                }
                return;
            }

            resolveVertexShader = Compiler::createShaderModule(device, vs);
            resolveFragmentShader = Compiler::createShaderModule(device, fs);

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

            vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &resolveDescriptorLayout);

            VkPipelineLayoutCreateInfo pipelineLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = &resolveDescriptorLayout,
            };

            vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &resolvePipelineLayout);

            VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
            VkDescriptorPoolCreateInfo poolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1,
                .poolSizeCount = 1,
                .pPoolSizes = &poolSize,
            };

            vkCreateDescriptorPool(device, &poolInfo, nullptr, &resolveDescriptorPool);

            VkDescriptorSetAllocateInfo allocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = resolveDescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &resolveDescriptorLayout,
            };

            vkAllocateDescriptorSets(device, &allocInfo, &resolveDescriptorSet);

            VkSamplerCreateInfo samplerInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            };

            vkCreateSampler(device, &samplerInfo, nullptr, &resolveSampler);
            updateResolveDescriptor();

            VkPipelineShaderStageCreateInfo stages[] =
            {
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = resolveVertexShader, .pName = "main" },
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = resolveFragmentShader, .pName = "main" },
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

            resolveSwapchainFormat = swapchainFormat;
            resolveSurfaceFormat = surfaceFormat;

            VkFormat colorFormat = swapchainFormat;
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
                .layout = resolvePipelineLayout,
            };

            vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resolvePipeline);
        }

        void clear(VkCommandBuffer commandBuffer, float32x4 color)
        {
            if (!valid())
            {
                return;
            }

            VkClearColorValue clearValue {};
            clearValue.float32[0] = color.x;
            clearValue.float32[1] = color.y;
            clearValue.float32[2] = color.z;
            clearValue.float32[3] = color.w;

            VkImageSubresourceRange range =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };

            vkCmdClearColorImage(commandBuffer, imageAllocation.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &range);

            VkImageMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .image = imageAllocation.image,
                .subresourceRange = range,
            };

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);

            imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        }

        void beginColorAttachment(VkCommandBuffer commandBuffer)
        {
            if (!valid())
            {
                return;
            }

            const bool undefinedLayout = imageLayout == VK_IMAGE_LAYOUT_UNDEFINED;
            const bool shaderReadLayout = imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            const VkPipelineStageFlags srcStage = undefinedLayout
                ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                : shaderReadLayout
                ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            const VkAccessFlags srcAccess = undefinedLayout
                ? VkAccessFlags(0)
                : shaderReadLayout
                ? VK_ACCESS_SHADER_READ_BIT
                : VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

            imageBarrier(commandBuffer, imageAllocation.image, imageLayout,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                srcAccess, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                srcStage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        }

        void endColorAttachment(VkCommandBuffer commandBuffer)
        {
            if (!valid())
            {
                return;
            }

            imageBarrier(commandBuffer, imageAllocation.image, imageLayout,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

        void resolve(VkCommandBuffer commandBuffer, Swapchain& swapchain, u32 imageIndex,
                        const OutputTransformOptions* colorOptions)
        {
            if (!valid())
            {
                return;
            }

            const VkExtent2D swapchainExtent = swapchain.getExtent();
            if (swapchainExtent.width != extent.width || swapchainExtent.height != extent.height)
            {
                printLine(Print::Warning,
                    "RenderTarget: extent {}x{} does not match swapchain {}x{}",
                    extent.width, extent.height, swapchainExtent.width, swapchainExtent.height);
            }

            ensureResolvePipeline(swapchain, colorOptions);
            if (!resolvePipeline)
            {
                return;
            }

            if (imageLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                VkAccessFlags srcAccess = VK_ACCESS_SHADER_WRITE_BIT;
                VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                if (imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                {
                    srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }

                imageBarrier(commandBuffer, imageAllocation.image, imageLayout,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    srcAccess, VK_ACCESS_SHADER_READ_BIT,
                    srcStage, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            }

            swapchain.transitionImageToColorAttachment(commandBuffer, imageIndex);

            VkRenderingAttachmentInfo colorAttachment =
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = swapchain.getImageView(imageIndex),
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

            vkCmdBeginRendering(commandBuffer, &renderingInfo);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, resolvePipeline);

            VkViewport viewport =
            {
                .x = 0.0f,
                .y = 0.0f,
                .width = float(extent.width),
                .height = float(extent.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };

            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &renderArea);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, resolvePipelineLayout, 0, 1, &resolveDescriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
            vkCmdEndRendering(commandBuffer);

            swapchain.transitionImageToPresent(commandBuffer, imageIndex);

            imageBarrier(commandBuffer, imageAllocation.image, imageLayout,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
    };

    RenderTarget::RenderTarget(const CreateInfo& info)
        : m_impl(std::make_unique<Impl>(info))
    {
    }

    RenderTarget::~RenderTarget() = default;

    void RenderTarget::resize(VkExtent2D extent)
    {
        m_impl->resize(extent);
    }

    RenderTarget::operator bool () const noexcept
    {
        return m_impl->valid();
    }

    RenderTargetFormat RenderTarget::targetFormat() const
    {
        return m_impl->targetFormat;
    }

    VkFormat RenderTarget::format() const
    {
        return m_impl->imageFormat;
    }

    VkExtent2D RenderTarget::extent() const
    {
        return m_impl->extent;
    }

    VkImage RenderTarget::image() const
    {
        return m_impl->imageAllocation.image;
    }

    VkImageView RenderTarget::view() const
    {
        return m_impl->imageView;
    }

    void RenderTarget::clear(VkCommandBuffer commandBuffer, float32x4 color)
    {
        m_impl->clear(commandBuffer, color);
    }

    void RenderTarget::beginColorAttachment(VkCommandBuffer commandBuffer)
    {
        m_impl->beginColorAttachment(commandBuffer);
    }

    void RenderTarget::endColorAttachment(VkCommandBuffer commandBuffer)
    {
        m_impl->endColorAttachment(commandBuffer);
    }

    RenderTarget::ColorAttachmentScope::ColorAttachmentScope(RenderTarget& target, VkCommandBuffer commandBuffer)
        : m_target(&target)
        , m_commandBuffer(commandBuffer)
    {
        target.beginColorAttachment(commandBuffer);
    }

    RenderTarget::ColorAttachmentScope::~ColorAttachmentScope()
    {
        if (m_target)
        {
            m_target->endColorAttachment(m_commandBuffer);
            m_target = nullptr;
        }
    }

    RenderTarget::ColorAttachmentScope::ColorAttachmentScope(ColorAttachmentScope&& other) noexcept
        : m_target(other.m_target)
        , m_commandBuffer(other.m_commandBuffer)
    {
        other.m_target = nullptr;
    }

    RenderTarget::ColorAttachmentScope RenderTarget::beginColorAttachmentScope(VkCommandBuffer commandBuffer)
    {
        return ColorAttachmentScope(*this, commandBuffer);
    }

    void RenderTarget::resolve(VkCommandBuffer commandBuffer, Swapchain& swapchain, u32 imageIndex,
                               const OutputTransformOptions* color)
    {
        m_impl->resolve(commandBuffer, swapchain, imageIndex, color);
    }

} // namespace mango::vulkan
