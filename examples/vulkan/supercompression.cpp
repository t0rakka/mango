/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#define MANGO_IMPLEMENT_MAIN
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

#include <mango/mango.hpp>
#include <mango/vulkan/vulkan.hpp>
#include <mango/vulkan/compiler.hpp>

using namespace mango;
using namespace mango::image;
using namespace mango::vulkan;

namespace
{

    std::string formatSizeKb(size_t bytes)
    {
        char text[32];
        if (bytes < 1024)
        {
            std::snprintf(text, sizeof(text), "%zu B", bytes);
        }
        else
        {
            std::snprintf(text, sizeof(text), "%.1f KB", double(bytes) / 1024.0);
        }

        return text;
    }

    struct Args
    {
        std::string filename;
        std::string gpu_format = "bc7";
        bool validate = false;
    };

    struct GpuTexture
    {
        std::vector<u8> data;
        int width = 0;
        int height = 0;
        u32 compression = TextureCompression::NONE;
        bool hdr = false;
        std::string format_name;
    };

    u32 compressionFromCli(const std::string& name)
    {
        if (name == "native")
        {
            return TextureCompression::NONE;
        }
        if (name == "bc7")
        {
            return TextureCompression::BC7_UNORM;
        }
        if (name == "bc3")
        {
            return TextureCompression::BC3_UNORM;
        }
        if (name == "bc1")
        {
            return TextureCompression::BC1_UNORM;
        }
        if (name == "bc4")
        {
            return TextureCompression::BC4_UNORM;
        }
        if (name == "bc5")
        {
            return TextureCompression::BC5_UNORM;
        }
        if (name == "etc2")
        {
            return TextureCompression::ETC2_RGBA;
        }
        if (name == "astc4x4")
        {
            return TextureCompression::ASTC_UNORM_4x4;
        }
        if (name == "bc6h")
        {
            return TextureCompression::BC6H_UF16;
        }
        if (name == "astc_hdr4")
        {
            return TextureCompression::ASTC_FLOAT_4x4;
        }
        return TextureCompression::NONE;
    }

    void adjustDefaultGpuFormat(const ImageHeader& header, std::string& gpu_format)
    {
        if (gpu_format != "bc7")
        {
            return;
        }

        if (header.supercompression & SUPERCOMPRESS_BASISU_UASTC_HDR)
        {
            gpu_format = "bc6h";
        }
    }

    bool isBlockCompressed(u32 compression)
    {
        return compression != TextureCompression::NONE;
    }

    bool isHdrCompression(u32 compression)
    {
        TextureCompression tc(compression);
        return tc.format.isFloat();
    }

    void logFormatSupport(VkPhysicalDevice physicalDevice, VkFormat format)
    {
        VkFormatProperties props {};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        const auto f = props.optimalTilingFeatures;
        printLine("  Vulkan {} ({}): sampled={} linear_filter={} transfer_dst={}",
            int(format), getString(format),
            (f & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0,
            (f & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0,
            (f & VK_FORMAT_FEATURE_TRANSFER_DST_BIT) != 0);
    }

    bool formatIsSampleable(VkPhysicalDevice physicalDevice, VkFormat format)
    {
        VkFormatProperties props {};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
    }

    bool formatSupportsLinearFilter(VkPhysicalDevice physicalDevice, VkFormat format)
    {
        VkFormatProperties props {};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
    }

    static const char* g_vertexShader = R"(#version 450
    layout(location = 0) in vec2 inPosition;
    layout(location = 1) in vec2 inUV;

    layout(location = 0) out vec2 fragUV;

    void main()
    {
        gl_Position = vec4(inPosition, 0.0, 1.0);
        fragUV = inUV;
    }
)";

    static const char* g_fragmentShader = R"(#version 450
    layout(location = 0) in vec2 fragUV;
    layout(location = 0) out vec4 outColor;

    layout(binding = 0) uniform sampler2D uTexture;

    layout(binding = 1) uniform ExposureBlock
    {
        float exposure;
    } uExposure;

    void main()
    {
        // KTX2 level data is top-first; our fullscreen quad is bottom-first (Vulkan clip space).
        vec2 uv = vec2(fragUV.x, 1.0 - fragUV.y);
        vec3 color = texture(uTexture, uv).rgb;
        float e = uExposure.exposure;
        if (e > 0.0)
        {
            color = vec3(1.0) - exp(-color * e);
        }
        outColor = vec4(color, 1.0);
    }
)";

    class SupercompressionWindow : public VulkanWindow
    {
        GpuTexture m_texture;
        std::string m_summary;
        float m_exposure = 0.0f;

        VkShaderModule m_vertexShader = VK_NULL_HANDLE;
        VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
        VkBuffer m_exposureBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_exposureBufferMemory = VK_NULL_HANDLE;

        VkImage m_textureImage = VK_NULL_HANDLE;
        VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
        VkImageView m_textureImageView = VK_NULL_HANDLE;
        VkSampler m_textureSampler = VK_NULL_HANDLE;
        VkFormat m_textureVkFormat = VK_FORMAT_UNDEFINED;

        bool m_ready = false;

        struct Vertex
        {
            float position[2];
            float uv[2];
        };

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                          VkBuffer& buffer, VkDeviceMemory& memory)
        {
            VkBufferCreateInfo bufferInfo =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = size,
                .usage = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            vkCreateBuffer(device(), &bufferInfo, nullptr, &buffer);

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device(), buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
            };

            vkAllocateMemory(device(), &allocInfo, nullptr, &memory);
            vkBindBufferMemory(device(), buffer, memory, 0);
        }

        void submitOneTimeCommands(const std::function<void(VkCommandBuffer)>& record)
        {
            VkCommandBufferAllocateInfo allocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = commandPool(),
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(device(), &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };

            vkBeginCommandBuffer(commandBuffer, &beginInfo);
            record(commandBuffer);
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer,
            };

            vkQueueSubmit(graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue());

            vkFreeCommandBuffers(device(), commandPool(), 1, &commandBuffer);
        }

        void uploadCompressedTexture()
        {
            TextureCompression tc(m_texture.compression);
            m_textureVkFormat = static_cast<VkFormat>(tc.vulkan);

            if (m_textureVkFormat == VK_FORMAT_UNDEFINED)
            {
                printLine(Print::Error, "No Vulkan format mapping for TextureCompression.");
                return;
            }

            if (!isBlockCompressed(m_texture.compression))
            {
                printLine(Print::Error, "GPU blob is not block-compressed; this demo samples compressed VkImages only.");
                return;
            }

            printLine("Physical device format probe:");
            logFormatSupport(physicalDevice(), m_textureVkFormat);

            if (!formatIsSampleable(physicalDevice(), m_textureVkFormat))
            {
                printLine(Print::Error,
                    "Decoder produced {} but this GPU cannot sample {} — try another --format or ktx2_blocks --all.",
                    m_texture.format_name, getString(m_textureVkFormat));
                return;
            }

            const VkDeviceSize imageSize = m_texture.data.size();

            VkBuffer stagingBuffer = VK_NULL_HANDLE;
            VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
            createBuffer(imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingMemory);

            void* mapped = nullptr;
            vkMapMemory(device(), stagingMemory, 0, imageSize, 0, &mapped);
            std::memcpy(mapped, m_texture.data.data(), imageSize);
            vkUnmapMemory(device(), stagingMemory);

            VkImageCreateInfo imageInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = m_textureVkFormat,
                .extent =
                {
                    .width = u32(m_texture.width),
                    .height = u32(m_texture.height),
                    .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            vkCreateImage(device(), &imageInfo, nullptr, &m_textureImage);

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device(), m_textureImage, &memRequirements);

            VkMemoryAllocateInfo allocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
            };

            vkAllocateMemory(device(), &allocInfo, nullptr, &m_textureImageMemory);
            vkBindImageMemory(device(), m_textureImage, m_textureImageMemory, 0);

            submitOneTimeCommands([&](VkCommandBuffer cmd)
            {
                VkImageMemoryBarrier toTransfer =
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = m_textureImage,
                    .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };

                vkCmdPipelineBarrier(cmd,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &toTransfer);

                VkBufferImageCopy region =
                {
                    .bufferOffset = 0,
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,
                    .imageSubresource =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .imageOffset = { 0, 0, 0 },
                    .imageExtent =
                    {
                        .width = u32(m_texture.width),
                        .height = u32(m_texture.height),
                        .depth = 1,
                    },
                };

                vkCmdCopyBufferToImage(cmd, stagingBuffer, m_textureImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                VkImageMemoryBarrier toShader =
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = m_textureImage,
                    .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };

                vkCmdPipelineBarrier(cmd,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &toShader);
            });

            vkDestroyBuffer(device(), stagingBuffer, nullptr);
            vkFreeMemory(device(), stagingMemory, nullptr);

            VkImageViewCreateInfo viewInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_textureImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_textureVkFormat,
                .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            vkCreateImageView(device(), &viewInfo, nullptr, &m_textureImageView);

            const bool linearOk = formatSupportsLinearFilter(physicalDevice(), m_textureVkFormat);

            VkSamplerCreateInfo samplerInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = linearOk ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
                .minFilter = linearOk ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .maxLod = 1.0f,
            };

            vkCreateSampler(device(), &samplerInfo, nullptr, &m_textureSampler);

            const u64 expected = tc.getBlockBytes(m_texture.width, m_texture.height);
            if (u64(imageSize) != expected)
            {
                printLine(Print::Warning, "Blob size {} != TextureCompression expected {} — upload may still work for mip0.",
                    formatSizeKb(imageSize), formatSizeKb(expected));
            }

            m_ready = true;
        }

        void createShaders()
        {
            Compiler compiler;
            Shader vertexShader = compiler.compile(g_vertexShader, ShaderStage::Vertex);
            Shader fragmentShader = compiler.compile(g_fragmentShader, ShaderStage::Fragment);

            if (!vertexShader || !fragmentShader)
            {
                printLine(Print::Error, "Shader compilation failed.");
                return;
            }

            m_vertexShader = Compiler::createShaderModule(device(), vertexShader);
            m_fragmentShader = Compiler::createShaderModule(device(), fragmentShader);
        }

        void createDescriptorSetLayout()
        {
            VkDescriptorSetLayoutBinding bindings[2] =
            {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
                {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
            };

            VkDescriptorSetLayoutCreateInfo layoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 2,
                .pBindings = bindings,
            };

            vkCreateDescriptorSetLayout(device(), &layoutInfo, nullptr, &m_descriptorSetLayout);
        }

        void createDescriptorPoolAndSet()
        {
            VkDescriptorPoolSize poolSizes[2] =
            {
                { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1 },
                { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1 },
            };

            VkDescriptorPoolCreateInfo poolInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1,
                .poolSizeCount = 2,
                .pPoolSizes = poolSizes,
            };

            vkCreateDescriptorPool(device(), &poolInfo, nullptr, &m_descriptorPool);

            VkDescriptorSetAllocateInfo allocInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = m_descriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &m_descriptorSetLayout,
            };

            vkAllocateDescriptorSets(device(), &allocInfo, &m_descriptorSet);

            VkDescriptorImageInfo imageInfo =
            {
                .sampler = m_textureSampler,
                .imageView = m_textureImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VkWriteDescriptorSet writes[2] =
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_descriptorSet,
                    .dstBinding = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &imageInfo,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = m_descriptorSet,
                    .dstBinding = 1,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = nullptr,
                },
            };

            VkDescriptorBufferInfo exposureInfo =
            {
                .buffer = m_exposureBuffer,
                .offset = 0,
                .range = sizeof(float),
            };

            writes[1].pBufferInfo = &exposureInfo;
            vkUpdateDescriptorSets(device(), 2, writes, 0, nullptr);
        }

        void createGeometry()
        {
            const Vertex vertices[] =
            {
                { { -1.0f, -1.0f }, { 0.0f, 1.0f } },
                { {  1.0f, -1.0f }, { 1.0f, 1.0f } },
                { {  1.0f,  1.0f }, { 1.0f, 0.0f } },
                { { -1.0f,  1.0f }, { 0.0f, 0.0f } },
            };

            const u16 indices[] = { 0, 1, 2, 2, 3, 0 };

            createBuffer(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_vertexBuffer, m_vertexBufferMemory);

            createBuffer(sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_indexBuffer, m_indexBufferMemory);

            void* data = nullptr;
            vkMapMemory(device(), m_vertexBufferMemory, 0, sizeof(vertices), 0, &data);
            std::memcpy(data, vertices, sizeof(vertices));
            vkUnmapMemory(device(), m_vertexBufferMemory);

            vkMapMemory(device(), m_indexBufferMemory, 0, sizeof(indices), 0, &data);
            std::memcpy(data, indices, sizeof(indices));
            vkUnmapMemory(device(), m_indexBufferMemory);

            createBuffer(sizeof(float), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_exposureBuffer, m_exposureBufferMemory);

            vkMapMemory(device(), m_exposureBufferMemory, 0, sizeof(float), 0, &data);
            std::memcpy(data, &m_exposure, sizeof(float));
            vkUnmapMemory(device(), m_exposureBufferMemory);
        }

        void createPipeline()
        {
            VkPipelineShaderStageCreateInfo shaderStages[2] =
            {
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = m_vertexShader,
                    .pName = "main",
                },
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = m_fragmentShader,
                    .pName = "main",
                },
            };

            VkVertexInputBindingDescription bindingDescription =
            {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };

            VkVertexInputAttributeDescription attributeDescriptions[2] =
            {
                {
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(Vertex, position),
                },
                {
                    .location = 1,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(Vertex, uv),
                },
            };

            VkPipelineVertexInputStateCreateInfo vertexInputInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = &bindingDescription,
                .vertexAttributeDescriptionCount = 2,
                .pVertexAttributeDescriptions = attributeDescriptions,
            };

            VkPipelineInputAssemblyStateCreateInfo inputAssembly =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            };

            VkPipelineViewportStateCreateInfo viewportState =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .scissorCount = 1,
            };

            VkDynamicState dynamicStates[] =
            {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
            };

            VkPipelineDynamicStateCreateInfo dynamicState =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = 2,
                .pDynamicStates = dynamicStates,
            };

            VkPipelineRasterizationStateCreateInfo rasterizer =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_NONE,
                .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .lineWidth = 1.0f,
            };

            VkPipelineMultisampleStateCreateInfo multisampling =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            };

            VkPipelineColorBlendAttachmentState colorBlendAttachment =
            {
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            };

            VkPipelineColorBlendStateCreateInfo colorBlending =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachment,
            };

            VkFormat colorFormat = swapchain().getFormat();

            VkPipelineRenderingCreateInfo renderingCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &colorFormat,
            };

            VkPipelineLayoutCreateInfo pipelineLayoutInfo =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = &m_descriptorSetLayout,
            };

            vkCreatePipelineLayout(device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

            VkGraphicsPipelineCreateInfo pipelineInfo =
            {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &renderingCreateInfo,
                .stageCount = 2,
                .pStages = shaderStages,
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssembly,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterizer,
                .pMultisampleState = &multisampling,
                .pColorBlendState = &colorBlending,
                .pDynamicState = &dynamicState,
                .layout = m_pipelineLayout,
            };

            vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
        }

        void destroyPipelineResources()
        {
            if (m_graphicsPipeline)
            {
                vkDestroyPipeline(device(), m_graphicsPipeline, nullptr);
                m_graphicsPipeline = VK_NULL_HANDLE;
            }

            if (m_pipelineLayout)
            {
                vkDestroyPipelineLayout(device(), m_pipelineLayout, nullptr);
                m_pipelineLayout = VK_NULL_HANDLE;
            }

            if (m_descriptorPool)
            {
                vkDestroyDescriptorPool(device(), m_descriptorPool, nullptr);
                m_descriptorPool = VK_NULL_HANDLE;
            }

            if (m_descriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(device(), m_descriptorSetLayout, nullptr);
                m_descriptorSetLayout = VK_NULL_HANDLE;
            }

            if (m_vertexShader)
            {
                vkDestroyShaderModule(device(), m_vertexShader, nullptr);
                m_vertexShader = VK_NULL_HANDLE;
            }

            if (m_fragmentShader)
            {
                vkDestroyShaderModule(device(), m_fragmentShader, nullptr);
                m_fragmentShader = VK_NULL_HANDLE;
            }

            if (m_vertexBuffer)
            {
                vkDestroyBuffer(device(), m_vertexBuffer, nullptr);
                m_vertexBuffer = VK_NULL_HANDLE;
            }

            if (m_vertexBufferMemory)
            {
                vkFreeMemory(device(), m_vertexBufferMemory, nullptr);
                m_vertexBufferMemory = VK_NULL_HANDLE;
            }

            if (m_indexBuffer)
            {
                vkDestroyBuffer(device(), m_indexBuffer, nullptr);
                m_indexBuffer = VK_NULL_HANDLE;
            }

            if (m_indexBufferMemory)
            {
                vkFreeMemory(device(), m_indexBufferMemory, nullptr);
                m_indexBufferMemory = VK_NULL_HANDLE;
            }

            if (m_exposureBuffer)
            {
                vkDestroyBuffer(device(), m_exposureBuffer, nullptr);
                m_exposureBuffer = VK_NULL_HANDLE;
            }

            if (m_exposureBufferMemory)
            {
                vkFreeMemory(device(), m_exposureBufferMemory, nullptr);
                m_exposureBufferMemory = VK_NULL_HANDLE;
            }

            if (m_textureSampler)
            {
                vkDestroySampler(device(), m_textureSampler, nullptr);
                m_textureSampler = VK_NULL_HANDLE;
            }

            if (m_textureImageView)
            {
                vkDestroyImageView(device(), m_textureImageView, nullptr);
                m_textureImageView = VK_NULL_HANDLE;
            }

            if (m_textureImage)
            {
                vkDestroyImage(device(), m_textureImage, nullptr);
                m_textureImage = VK_NULL_HANDLE;
            }

            if (m_textureImageMemory)
            {
                vkFreeMemory(device(), m_textureImageMemory, nullptr);
                m_textureImageMemory = VK_NULL_HANDLE;
            }
        }

        void recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex, VkExtent2D extent)
        {
            vkResetCommandBuffer(commandBuffer, 0);

            VkCommandBufferBeginInfo beginInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            };

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            swapchain().transitionImageToColorAttachment(commandBuffer, imageIndex);

            VkRenderingAttachmentInfo colorAttachment =
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = swapchain().getImageView(imageIndex),
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .color = {{ 0.05f, 0.05f, 0.08f, 1.0f }} },
            };

            VkRenderingInfo renderingInfo =
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = { .extent = extent },
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachment,
            };

            vkCmdBeginRendering(commandBuffer, &renderingInfo);

            if (m_ready && m_graphicsPipeline)
            {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

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

                VkRect2D scissor =
                {
                    .offset = { 0, 0 },
                    .extent = extent,
                };
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

                VkBuffer vertexBuffers[] = { m_vertexBuffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
            }

            vkCmdEndRendering(commandBuffer);

            swapchain().transitionImageToPresent(commandBuffer, imageIndex);

            vkEndCommandBuffer(commandBuffer);
        }

    public:
        SupercompressionWindow(VulkanContext& context, GpuTexture texture, const std::string& summary)
            : VulkanWindow(context, 1280, 720, 0)
            , m_texture(std::move(texture))
            , m_summary(summary)
            , m_exposure(texture.hdr ? 1.8f : 0.0f)
        {
        }

        void onDeviceReady() override
        {
            setTitle("Supercompression — " + m_summary);
            printLine("Uploading {} block texture ({} x {}, {})",
                m_texture.format_name, m_texture.width, m_texture.height, formatSizeKb(m_texture.data.size()));

            uploadCompressedTexture();
            if (!m_ready)
            {
                requestQuit();
                return;
            }

            createShaders();
            createGeometry();
            createDescriptorSetLayout();
            createDescriptorPoolAndSet();
            createPipeline();

            printLine("Rendering hardware-sampled block texture (ImageDecoder::memory → VkImage).");
        }

        ~SupercompressionWindow()
        {
            if (device() != VK_NULL_HANDLE)
            {
                vkDeviceWaitIdle(device());
                destroyPipelineResources();
            }
        }

        void onFrame(const FrameInfo& info) override
        {
            MANGO_UNREFERENCED(info);

            auto frame = beginDraw();
            if (!frame)
            {
                return;
            }

            const VkExtent2D extent = swapchainExtent();
            VkCommandBuffer commandBuffer = this->commandBuffer(frame.imageIndex());
            recordCommandBuffer(commandBuffer, frame.imageIndex(), extent);
            frame.submitAndPresent(graphicsQueue(), commandBuffer);
        }

        void onKeyPress(Keycode code, u32 mask) override
        {
            MANGO_UNREFERENCED(mask);

            if (code == KEYCODE_ESC)
            {
                requestQuit();
            }
            else if (code == KEYCODE_F)
            {
                toggleFullscreen();
            }
        }
    };

} // namespace

int mangoMain(const CommandLine& commands)
{
    Args args;

    commands.usage("[options] <file.ktx2>");
    commands.option("--format", "Transcode target: bc7, bc6h, bc3, bc1, bc4, bc5, etc2, astc4x4, astc_hdr4, native (default: bc7; UASTC HDR → bc6h)",
        [&](std::string_view value)
        {
            args.gpu_format = std::string(value);
        });
    commands.flag("--validate", "enable Vulkan validation layer",
        [&]()
        {
            args.validate = true;
        });
    commands.positional([&](std::string_view token)
    {
        args.filename = token;
    });

    if (!commands.parse())
    {
        return 1;
    }

    if (args.filename.empty())
    {
        commands.printHelp();
        printLine("");
        printLine("Transcode via ImageDecoder::memory(), upload block data to a compressed VkImage, sample in the GPU.");
        printLine("Matrix / CI probes: ktx2_blocks --list-formats / --all <file.ktx2>");
        return 1;
    }

    filesystem::File file(args.filename);
    ImageDecoder decoder(file, args.filename);
    if (!decoder.isDecoder())
    {
        printLine(Print::Error, "Not a KTX2 image.");
        return 1;
    }

    ImageHeader header = decoder.header();
    if (!header.success)
    {
        printLine(Print::Error, "{}", header.info);
        return 1;
    }

    adjustDefaultGpuFormat(header, args.gpu_format);

    ImageDecodeOptions gpu_options;
    gpu_options.compression = compressionFromCli(args.gpu_format);

    ConstMemory gpu = decoder.memory(0, 0, 0, gpu_options);
    const ImageHeader after_gpu = decoder.header();

    if (!gpu.size)
    {
        printLine(Print::Error, "ImageDecoder::memory(\"{}\") returned no data — run ktx2_blocks --all {}",
            args.gpu_format, args.filename);
        return 1;
    }

    const u32 output_compression = after_gpu.compression != TextureCompression::NONE
        ? after_gpu.compression
        : gpu_options.compression;

    if (!isBlockCompressed(output_compression))
    {
        printLine(Print::Error, "No block compression type for this transcode. Pick another --format.");
        return 1;
    }

    TextureCompression tc(output_compression);

    GpuTexture texture;
    texture.data.assign(gpu.address, gpu.address + gpu.size);
    texture.width = header.width;
    texture.height = header.height;
    texture.compression = output_compression;
    texture.hdr = isHdrCompression(texture.compression);
    texture.format_name = args.gpu_format;

    std::string summary = format("{} x {} | {} → {} | {}",
        header.width, header.height, args.gpu_format, getString(static_cast<VkFormat>(tc.vulkan)),
        formatSizeKb(texture.data.size()));

    printLine("supercompression: {}", summary);
    printLine("  supercompression flags: 0x{:08x}", header.supercompression);

    std::vector<const char*> enabled_layers;
    if (args.validate)
    {
        enabled_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    std::vector<const char*> enabled_extensions = requiredSurfaceExtensions();

    InstanceExtensionProperties instanceExtensionProperties;
    if (instanceExtensionProperties.contains(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME))
    {
        enabled_extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    }

    VkApplicationInfo applicationInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "supercompression",
        .applicationVersion = 1,
        .pEngineName = "mango",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    Instance instance(applicationInfo, enabled_layers, enabled_extensions);
    VulkanContext ctx(instance);

    SupercompressionWindow window(ctx, std::move(texture), summary);
    if (!window.isDeviceReady())
    {
        printLine(Print::Error, "Vulkan device/swapchain setup failed (check MoltenVK / GPU drivers).");
        return 1;
    }

    EventLoopConfig config;
    config.mode = FrameMode::Continuous;
    config.trackDisplayRefreshRate = true;
    config.waitForFrame = true;

    EventLoop loop;
    loop.attach(window, config);
    loop.run();

    return 0;
}
