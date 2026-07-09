/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>

#include <mango/core/bits.hpp>
#include <mango/core/exception.hpp>
#include <mango/vulkan/framebuffer.hpp>

namespace
{

    using namespace mango;
    using namespace mango::image;
    using namespace mango::math;
    using namespace mango::vulkan;

    const char* g_vertex_shader = R"(#version 450
        layout(location = 0) in vec2 inPosition;
        layout(location = 1) in vec2 inTexcoord;
        layout(location = 0) out vec2 vTexcoord;

        layout(push_constant) uniform PushConstants
        {
            vec4 u_Transform;
        } pc;

        void main()
        {
            vTexcoord = inTexcoord;
            gl_Position = vec4((inPosition + pc.u_Transform.xy) * pc.u_Transform.zw, 0.0, 1.0);
        }
    )";

    const char* g_fragment_shader = R"(#version 450
        layout(location = 0) in vec2 vTexcoord;
        layout(location = 0) out vec4 outColor;

        layout(set = 0, binding = 0) uniform sampler2D u_Texture;

        void main()
        {
            outColor = texture(u_Texture, vTexcoord);
        }
    )";

    const char* g_fragment_shader_float = R"(#version 450
        layout(location = 0) in vec2 vTexcoord;
        layout(location = 0) out vec4 outColor;

        layout(set = 0, binding = 0) uniform sampler2D u_Texture;

        layout(push_constant) uniform PushConstants
        {
            layout(offset = 16) float u_Exposure;
        } pc;

        vec3 tonemapSdr(vec3 color)
        {
            color = max(color, 0.0) * pc.u_Exposure;
            return color / (1.0 + color);
        }

        void main()
        {
            vec4 src = texture(u_Texture, vTexcoord);
            outColor = vec4(tonemapSdr(src.rgb), 1.0);
        }
    )";

    const char* g_resolve_fragment_shader = R"(#version 450
        layout(location = 0) in vec2 vTexcoord;
        layout(location = 0) out vec4 outColor;

        layout(set = 0, binding = 0) uniform usampler2D u_Index;
        layout(set = 0, binding = 1) readonly buffer Palette { uint u_Palette[]; };

        void main()
        {
            uint index = texture(u_Index, vTexcoord).r;
            uint color = u_Palette[index];
            float r = float((color >>  0) & 0xffu) / 255.0;
            float g = float((color >>  8) & 0xffu) / 255.0;
            float b = float((color >> 16) & 0xffu) / 255.0;
            float a = float((color >> 24) & 0xffu) / 255.0;
            outColor = vec4(r, g, b, a);
        }
    )";

    static
    int32x2 adjustWindowSizeToContent(int width, int height, int screenIndex)
    {
        int32x2 screen = Window::getScreenSize(screenIndex);
        int32x2 content(width, height);

        if (screen.x <= 0 || screen.y <= 0)
        {
            constexpr int max_dim = 1280;
            const int max_side = std::max(content.x, content.y);

            if (max_side > max_dim)
            {
                const float scale = float(max_dim) / float(max_side);
                content.x = std::max(1, int(float(content.x) * scale + 0.5f));
                content.y = std::max(1, int(float(content.y) * scale + 0.5f));
            }

            return content;
        }

        if (content.x > screen.x)
        {
            int scale = div_ceil(content.x, screen.x);
            content.x = content.x / scale;
            content.y = content.y / scale;
        }

        if (content.y > screen.y)
        {
            int scale = div_ceil(content.y, screen.y);
            content.x = content.x / scale;
            content.y = content.y / scale;
        }

        if (content.y < screen.y)
        {
            int scale = std::max(1, (screen.y / std::max(1, content.y)) / 2);
            content.x *= scale;
            content.y *= scale;
        }

        return content;
    }

} // namespace

namespace mango::vulkan
{

    using namespace mango::image;
    using namespace mango::math;

    // -------------------------------------------------------------------
    // VulkanFramebuffer
    // -------------------------------------------------------------------

    VulkanFramebuffer::VulkanFramebuffer(VkInstance instance, int width, int height, BufferMode buffermode)
        : VulkanWindow(instance,
            adjustWindowSizeToContent(width, height, 0).x,
            adjustWindowSizeToContent(width, height, 0).y,
            0)
        , m_width(width)
        , m_height(height)
    {
        switch (buffermode)
        {
            case RGBA_DIRECT:
                m_format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                m_is_rgba = true;
                m_is_float = false;
                m_is_palette = false;
                break;

            case BGRA_DIRECT:
                m_format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                m_is_rgba = false;
                m_is_float = false;
                m_is_palette = false;
                break;

            case RGBA_FLOAT:
                m_format = Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
                m_is_rgba = true;
                m_is_float = true;
                m_is_palette = false;
                break;

            case BGRA_FLOAT:
                m_format = Format(128, Format::FLOAT32, Format::BGRA, 32, 32, 32, 32);
                m_is_rgba = false;
                m_is_float = true;
                m_is_palette = false;
                break;

            case RGBA_PALETTE:
                m_format = IndexedFormat(8);
                m_is_rgba = true;
                m_is_float = false;
                m_is_palette = true;
                break;

            case BGRA_PALETTE:
                m_format = IndexedFormat(8);
                m_is_rgba = false;
                m_is_float = false;
                m_is_palette = true;
                break;
        }

        m_stride = size_t(m_width) * m_format.bytes();
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        destroyGpuResources();
    }

    void VulkanFramebuffer::onDeviceReady()
    {
        createStagingBuffer();
        createTexture();
        createSamplers();
        createPresentPipeline();
        if (m_is_palette)
        {
            createIndexTexture();
            createPaletteBuffer();
            createResolvePipeline();
        }
        createQuadGeometry();
        createUploadCommands();
        m_gpu_ready = true;
    }

    void VulkanFramebuffer::destroyGpuResources()
    {
        if (m_device == VK_NULL_HANDLE)
        {
            return;
        }

        vkDeviceWaitIdle(m_device);

        if (m_uploadFence)
        {
            vkDestroyFence(m_device, m_uploadFence, nullptr);
            m_uploadFence = VK_NULL_HANDLE;
        }

        if (m_uploadPool)
        {
            vkDestroyCommandPool(m_device, m_uploadPool, nullptr);
            m_uploadPool = VK_NULL_HANDLE;
        }

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

        if (m_resolveFragmentShader)
        {
            vkDestroyShaderModule(m_device, m_resolveFragmentShader, nullptr);
            m_resolveFragmentShader = VK_NULL_HANDLE;
        }

        if (m_paletteBuffer)
        {
            vkDestroyBuffer(m_device, m_paletteBuffer, nullptr);
            m_paletteBuffer = VK_NULL_HANDLE;
        }

        if (m_paletteMemory)
        {
            vkFreeMemory(m_device, m_paletteMemory, nullptr);
            m_paletteMemory = VK_NULL_HANDLE;
        }

        if (m_indexTextureView)
        {
            vkDestroyImageView(m_device, m_indexTextureView, nullptr);
            m_indexTextureView = VK_NULL_HANDLE;
        }

        if (m_indexTexture)
        {
            vkDestroyImage(m_device, m_indexTexture, nullptr);
            m_indexTexture = VK_NULL_HANDLE;
        }

        if (m_indexTextureMemory)
        {
            vkFreeMemory(m_device, m_indexTextureMemory, nullptr);
            m_indexTextureMemory = VK_NULL_HANDLE;
        }

        if (m_pipeline)
        {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }

        if (m_pipelineLayout)
        {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }

        if (m_descriptorPool)
        {
            vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }

        if (m_descriptorLayout)
        {
            vkDestroyDescriptorSetLayout(m_device, m_descriptorLayout, nullptr);
            m_descriptorLayout = VK_NULL_HANDLE;
        }

        if (m_vertexShader)
        {
            vkDestroyShaderModule(m_device, m_vertexShader, nullptr);
            m_vertexShader = VK_NULL_HANDLE;
        }

        if (m_fragmentShader)
        {
            vkDestroyShaderModule(m_device, m_fragmentShader, nullptr);
            m_fragmentShader = VK_NULL_HANDLE;
        }

        if (m_vertexBuffer)
        {
            vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
            m_vertexBuffer = VK_NULL_HANDLE;
        }

        if (m_vertexBufferMemory)
        {
            vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
            m_vertexBufferMemory = VK_NULL_HANDLE;
        }

        if (m_textureView)
        {
            vkDestroyImageView(m_device, m_textureView, nullptr);
            m_textureView = VK_NULL_HANDLE;
        }

        if (m_texture)
        {
            vkDestroyImage(m_device, m_texture, nullptr);
            m_texture = VK_NULL_HANDLE;
        }

        if (m_textureMemory)
        {
            vkFreeMemory(m_device, m_textureMemory, nullptr);
            m_textureMemory = VK_NULL_HANDLE;
        }

        if (m_stagingBuffer)
        {
            vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
            m_stagingBuffer = VK_NULL_HANDLE;
        }

        if (m_stagingMemory)
        {
            vkFreeMemory(m_device, m_stagingMemory, nullptr);
            m_stagingMemory = VK_NULL_HANDLE;
        }

        if (m_samplerNearest)
        {
            vkDestroySampler(m_device, m_samplerNearest, nullptr);
            m_samplerNearest = VK_NULL_HANDLE;
        }

        if (m_samplerLinear)
        {
            vkDestroySampler(m_device, m_samplerLinear, nullptr);
            m_samplerLinear = VK_NULL_HANDLE;
        }

        m_gpu_ready = false;
        m_texture_uploaded = false;
        m_index_texture_uploaded = false;
    }

    void VulkanFramebuffer::createStagingBuffer()
    {
        const VkDeviceSize size = VkDeviceSize(m_stride * m_height);

        VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_stagingBuffer);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(m_device, m_stagingBuffer, &memReq);

        VkMemoryAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReq.size,
            .memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        };

        vkAllocateMemory(m_device, &allocInfo, nullptr, &m_stagingMemory);
        vkBindBufferMemory(m_device, m_stagingBuffer, m_stagingMemory, 0);
    }

    void VulkanFramebuffer::createTexture()
    {
        const VkFormat vkFormat = m_is_float ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;

        const VkImageUsageFlags usage = m_is_palette
            ? (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
            : (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        VkImageCreateInfo imageInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = vkFormat,
            .extent = { u32(m_width), u32(m_height), 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        vkCreateImage(m_device, &imageInfo, nullptr, &m_texture);

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(m_device, m_texture, &memReq);

        VkMemoryAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReq.size,
            .memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        };

        vkAllocateMemory(m_device, &allocInfo, nullptr, &m_textureMemory);
        vkBindImageMemory(m_device, m_texture, m_textureMemory, 0);

        VkComponentMapping swizzle =
        {
            .r = m_is_rgba ? VK_COMPONENT_SWIZZLE_R : VK_COMPONENT_SWIZZLE_B,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = m_is_rgba ? VK_COMPONENT_SWIZZLE_B : VK_COMPONENT_SWIZZLE_R,
            .a = VK_COMPONENT_SWIZZLE_A,
        };

        VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_texture,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vkFormat,
            .components = swizzle,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCreateImageView(m_device, &viewInfo, nullptr, &m_textureView);
    }

    void VulkanFramebuffer::createIndexTexture()
    {
        VkImageCreateInfo imageInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8_UINT,
            .extent = { u32(m_width), u32(m_height), 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        vkCreateImage(m_device, &imageInfo, nullptr, &m_indexTexture);

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(m_device, m_indexTexture, &memReq);

        VkMemoryAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReq.size,
            .memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        };

        vkAllocateMemory(m_device, &allocInfo, nullptr, &m_indexTextureMemory);
        vkBindImageMemory(m_device, m_indexTexture, m_indexTextureMemory, 0);

        VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_indexTexture,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R8_UINT,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCreateImageView(m_device, &viewInfo, nullptr, &m_indexTextureView);
    }

    void VulkanFramebuffer::createPaletteBuffer()
    {
        const VkDeviceSize size = sizeof(m_palette);

        VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_paletteBuffer);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(m_device, m_paletteBuffer, &memReq);

        VkMemoryAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReq.size,
            .memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        };

        vkAllocateMemory(m_device, &allocInfo, nullptr, &m_paletteMemory);
        vkBindBufferMemory(m_device, m_paletteBuffer, m_paletteMemory, 0);
        uploadPalette();
    }

    void VulkanFramebuffer::setPalette(const Palette& palette)
    {
        if (!m_is_palette)
        {
            return;
        }

        for (u32 i = 0; i < 256; ++i)
        {
            m_palette[i] = palette[i];
        }

        uploadPalette();
    }

    void VulkanFramebuffer::uploadPalette()
    {
        if (!m_paletteBuffer)
        {
            return;
        }

        void* data = nullptr;
        vkMapMemory(m_device, m_paletteMemory, 0, sizeof(m_palette), 0, &data);
        std::memcpy(data, m_palette, sizeof(m_palette));
        vkUnmapMemory(m_device, m_paletteMemory);
    }

    void VulkanFramebuffer::createSamplers()
    {
        VkSamplerCreateInfo samplerInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        };

        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_samplerNearest);

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_samplerLinear);
    }

    void VulkanFramebuffer::createPresentPipeline()
    {
        Compiler compiler;
        Shader vs = compiler.compile(g_vertex_shader, ShaderStage::Vertex);
        Shader fs = compiler.compile(m_is_float ? g_fragment_shader_float : g_fragment_shader, ShaderStage::Fragment);

        if (!vs.valid() || !fs.valid())
        {
            MANGO_EXCEPTION("[VulkanFramebuffer] shader compilation failed.");
        }

        m_vertexShader = Compiler::createShaderModule(m_device, vs);
        m_fragmentShader = Compiler::createShaderModule(m_device, fs);

        VkDescriptorSetLayoutBinding samplerBinding =
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
            .pBindings = &samplerBinding,
        };

        vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorLayout);

        VkPushConstantRange pushRanges[2];
        u32 pushRangeCount = 1;

        pushRanges[0] =
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(float) * 4,
        };

        if (m_is_float)
        {
            pushRanges[1] =
            {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = sizeof(float) * 4,
                .size = sizeof(float),
            };
            pushRangeCount = 2;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptorLayout,
            .pushConstantRangeCount = pushRangeCount,
            .pPushConstantRanges = pushRanges,
        };

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

        VkDescriptorPoolSize poolSize =
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
        };

        VkDescriptorPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
        };

        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);

        VkDescriptorSetAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_descriptorLayout,
        };

        vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet);

        VkDescriptorImageInfo imageInfo =
        {
            .sampler = m_samplerNearest,
            .imageView = m_textureView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet write =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
        };

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        VkPipelineShaderStageCreateInfo stages[] =
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

        VkVertexInputBindingDescription binding =
        {
            .binding = 0,
            .stride = sizeof(float) * 4,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkVertexInputAttributeDescription attributes[] =
        {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = 0,
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = sizeof(float) * 2,
            },
        };

        VkPipelineVertexInputStateCreateInfo vertexInput =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &binding,
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions = attributes,
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
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

        VkPipelineColorBlendAttachmentState blendAttachment =
        {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        VkPipelineColorBlendStateCreateInfo colorBlending =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &blendAttachment,
        };

        VkFormat colorFormat = swapchain().getFormat();

        VkPipelineRenderingCreateInfo renderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat,
        };

        VkGraphicsPipelineCreateInfo pipelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingInfo,
            .stageCount = 2,
            .pStages = stages,
            .pVertexInputState = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = m_pipelineLayout,
        };

        vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    }

    void VulkanFramebuffer::createResolvePipeline()
    {
        Compiler compiler;
        Shader fs = compiler.compile(g_resolve_fragment_shader, ShaderStage::Fragment);

        if (!fs.valid())
        {
            MANGO_EXCEPTION("[VulkanFramebuffer] resolve shader compilation failed.");
        }

        m_resolveFragmentShader = Compiler::createShaderModule(m_device, fs);

        VkDescriptorSetLayoutBinding bindings[] =
        {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
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

        vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_resolveDescriptorLayout);

        VkPushConstantRange pushRange =
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(float) * 4,
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_resolveDescriptorLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushRange,
        };

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_resolvePipelineLayout);

        VkDescriptorPoolSize poolSizes[] =
        {
            { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1 },
            { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1 },
        };

        VkDescriptorPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = 2,
            .pPoolSizes = poolSizes,
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

        VkDescriptorImageInfo imageInfo =
        {
            .sampler = m_samplerNearest,
            .imageView = m_indexTextureView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorBufferInfo bufferInfo =
        {
            .buffer = m_paletteBuffer,
            .offset = 0,
            .range = sizeof(m_palette),
        };

        VkWriteDescriptorSet writes[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_resolveDescriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_resolveDescriptorSet,
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &bufferInfo,
            },
        };

        vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);

        VkPipelineShaderStageCreateInfo stages[] =
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
                .module = m_resolveFragmentShader,
                .pName = "main",
            },
        };

        VkVertexInputBindingDescription binding =
        {
            .binding = 0,
            .stride = sizeof(float) * 4,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };

        VkVertexInputAttributeDescription attributes[] =
        {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = 0,
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = sizeof(float) * 2,
            },
        };

        VkPipelineVertexInputStateCreateInfo vertexInput =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &binding,
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions = attributes,
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
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

        VkPipelineColorBlendAttachmentState blendAttachment =
        {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        VkPipelineColorBlendStateCreateInfo colorBlending =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &blendAttachment,
        };

        VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

        VkPipelineRenderingCreateInfo renderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat,
        };

        VkGraphicsPipelineCreateInfo pipelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingInfo,
            .stageCount = 2,
            .pStages = stages,
            .pVertexInputState = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = m_resolvePipelineLayout,
        };

        vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_resolvePipeline);
    }

    void VulkanFramebuffer::createQuadGeometry()
    {
        const float vertices[] =
        {
            // position   texcoord
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
        };

        const VkDeviceSize size = sizeof(vertices);

        VkBufferCreateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexBuffer);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &memReq);

        VkMemoryAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReq.size,
            .memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        };

        vkAllocateMemory(m_device, &allocInfo, nullptr, &m_vertexBufferMemory);
        vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);

        void* data = nullptr;
        vkMapMemory(m_device, m_vertexBufferMemory, 0, size, 0, &data);
        std::memcpy(data, vertices, sizeof(vertices));
        vkUnmapMemory(m_device, m_vertexBufferMemory);
    }

    void VulkanFramebuffer::createUploadCommands()
    {
        VkCommandPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
        };

        vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_uploadPool);

        VkCommandBufferAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_uploadPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        vkAllocateCommandBuffers(m_device, &allocInfo, &m_uploadCommand);

        VkFenceCreateInfo fenceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        };

        vkCreateFence(m_device, &fenceInfo, nullptr, &m_uploadFence);
    }

    Surface VulkanFramebuffer::lock()
    {
        if (!m_gpu_ready)
        {
            MANGO_EXCEPTION("[VulkanFramebuffer] lock() before GPU initialization.");
        }

        void* data = nullptr;
        vkMapMemory(m_device, m_stagingMemory, 0, VK_WHOLE_SIZE, 0, &data);
        return Surface(m_width, m_height, m_format, m_stride, data);
    }

    void VulkanFramebuffer::unlock()
    {
        vkUnmapMemory(m_device, m_stagingMemory);

        if (m_is_palette)
        {
            uploadIndexTexture();
        }
        else
        {
            uploadTexture();
        }
    }

    void VulkanFramebuffer::uploadTexture()
    {
        vkResetCommandBuffer(m_uploadCommand, 0);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(m_uploadCommand, &beginInfo);

        VkImageMemoryBarrier toTransfer =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = m_texture_uploaded ? VK_ACCESS_SHADER_READ_BIT : VkAccessFlags(0),
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = m_texture_uploaded ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = m_texture,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(m_uploadCommand,
            m_texture_uploaded ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
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
                .layerCount = 1,
            },
            .imageExtent = { u32(m_width), u32(m_height), 1 },
        };

        vkCmdCopyBufferToImage(m_uploadCommand, m_stagingBuffer, m_texture,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier toShaderRead =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image = m_texture,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(m_uploadCommand,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &toShaderRead);

        vkEndCommandBuffer(m_uploadCommand);

        VkSubmitInfo submitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_uploadCommand,
        };

        vkResetFences(m_device, 1, &m_uploadFence);
        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_uploadFence);
        vkWaitForFences(m_device, 1, &m_uploadFence, VK_TRUE, UINT64_MAX);
        m_texture_uploaded = true;
    }

    void VulkanFramebuffer::uploadIndexTexture()
    {
        vkResetCommandBuffer(m_uploadCommand, 0);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(m_uploadCommand, &beginInfo);

        VkImageMemoryBarrier toTransfer =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = m_index_texture_uploaded ? VK_ACCESS_SHADER_READ_BIT : VkAccessFlags(0),
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = m_index_texture_uploaded ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = m_indexTexture,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(m_uploadCommand,
            m_index_texture_uploaded ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &toTransfer);

        VkBufferImageCopy region =
        {
            .bufferOffset = 0,
            .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
            },
            .imageExtent = { u32(m_width), u32(m_height), 1 },
        };

        vkCmdCopyBufferToImage(m_uploadCommand, m_stagingBuffer, m_indexTexture,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier toShaderRead =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image = m_indexTexture,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(m_uploadCommand,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &toShaderRead);

        vkEndCommandBuffer(m_uploadCommand);

        VkSubmitInfo submitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_uploadCommand,
        };

        vkResetFences(m_device, 1, &m_uploadFence);
        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_uploadFence);
        vkWaitForFences(m_device, 1, &m_uploadFence, VK_TRUE, UINT64_MAX);
        m_index_texture_uploaded = true;
    }

    void VulkanFramebuffer::recordResolve(VkCommandBuffer cmd)
    {
        VkImageMemoryBarrier textureBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = m_texture_uploaded ? VK_ACCESS_SHADER_READ_BIT : VkAccessFlags(0),
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = m_texture_uploaded ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = m_texture,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(cmd,
            m_texture_uploaded ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 1, &textureBarrier);

        const float transform[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

        VkRenderingAttachmentInfo colorAttachment =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_textureView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };

        VkRenderingInfo renderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { .extent = { u32(m_width), u32(m_height) } },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };

        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport =
        {
            .width = float(m_width),
            .height = float(m_height),
            .maxDepth = 1.0f,
        };

        VkRect2D scissor =
        {
            .extent = { u32(m_width), u32(m_height) },
        };

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resolvePipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resolvePipelineLayout, 0, 1, &m_resolveDescriptorSet, 0, nullptr);
        vkCmdPushConstants(cmd, m_resolvePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform), transform);

        VkBuffer buffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        vkCmdEndRendering(cmd);

        textureBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        textureBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        textureBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        textureBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &textureBarrier);

        m_texture_uploaded = true;
    }

    void VulkanFramebuffer::recordPresent(VkCommandBuffer cmd, u32 imageIndex, Filter filter)
    {
        swapchain().cmdTransitionImageToColorAttachment(cmd, imageIndex);

        VkExtent2D extent = swapchainExtent();
        u32 width = std::max(1u, extent.width);
        u32 height = std::max(1u, extent.height);

        float32x2 aspect;
        aspect.x = float(width) / float(m_width);
        aspect.y = float(height) / float(m_height);

        if (aspect.x < aspect.y)
        {
            aspect.y = aspect.x / aspect.y;
            aspect.x = 1.0f;
        }
        else
        {
            aspect.x = aspect.y / aspect.x;
            aspect.y = 1.0f;
        }

        const float transform[4] =
        {
            0.0f, 0.0f, aspect.x, aspect.y,
        };

        VkRenderingAttachmentInfo colorAttachment =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain().getImageView(imageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = {{ 0.0f, 0.0f, 0.0f, 1.0f }} },
        };

        VkRenderingInfo renderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { .extent = { width, height } },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };

        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport =
        {
            .width = float(width),
            .height = float(height),
            .maxDepth = 1.0f,
        };

        VkRect2D scissor =
        {
            .extent = { width, height },
        };

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        VkSampler sampler = filter == FILTER_BILINEAR ? m_samplerLinear : m_samplerNearest;

        VkDescriptorImageInfo imageInfo =
        {
            .sampler = sampler,
            .imageView = m_textureView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet write =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
        };

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

        if (m_is_float)
        {
            const float pushConstants[5] =
            {
                transform[0], transform[1], transform[2], transform[3], m_exposure,
            };

            vkCmdPushConstants(cmd, m_pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(pushConstants), pushConstants);
        }
        else
        {
            vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform), transform);
        }

        VkBuffer buffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        vkCmdEndRendering(cmd);

        swapchain().cmdTransitionImageToPresent(cmd, imageIndex);
    }

    void VulkanFramebuffer::present(Filter filter)
    {
        auto frame = beginDraw();
        if (!frame)
        {
            return;
        }

        VkCommandBuffer cmd = commandBuffer(frame.imageIndex());
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(cmd, &beginInfo);

        if (m_is_palette)
        {
            recordResolve(cmd);
        }

        recordPresent(cmd, frame.imageIndex(), filter);
        vkEndCommandBuffer(cmd);

        frame.submitAndPresent(m_graphicsQueue, cmd);
    }

} // namespace mango::vulkan
