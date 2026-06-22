/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstdio>
#include <vector>
#include <cmath>
#include <cstring>

#define MANGO_IMPLEMENT_MAIN
#include <mango/core/core.hpp>
#include <mango/math/math.hpp>
#include <mango/vulkan/vulkan.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::vulkan;

// ------------------------------------------------------------------------------

static const char* g_vertexShader = R"(#version 450
    layout(location = 0) in vec3 inPosition;
    layout(location = 1) in vec3 inColor;

    layout(location = 0) out vec3 fragColor;

    layout(binding = 0) uniform UniformBufferObject
    {
        layout(row_major) mat4 mvp;
    } ubo;

    void main()
    {
        gl_Position = vec4(inPosition, 1.0) * ubo.mvp;
        fragColor = inColor;
    }
)";

static const char* g_fragmentShader = R"(#version 450
    layout(location = 0) in vec3 fragColor;
    layout(location = 0) out vec4 outColor;

    void main()
    {
        outColor = vec4(fragColor, 1.0);
    }
)";

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
    VkRenderPass m_renderPass = VK_NULL_HANDLE;

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
    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;

    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;

    u64 m_startTime = 0;

    struct Vertex
    {
        float position[3];
        float color[3];
    };

    struct UniformBufferObject
    {
        Matrix4x4 mvp;
    };

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        printLine(Print::Error, "Failed to find suitable memory type.");
        return 0;
    }

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

        VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateBuffer: {}", getString(result));
            return;
        }

        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(m_device, buffer, &requirements);

        VkMemoryAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirements.size,
            .memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties),
        };

        result = vkAllocateMemory(m_device, &allocInfo, nullptr, &memory);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkAllocateMemory: {}", getString(result));
            return;
        }

        vkBindBufferMemory(m_device, buffer, memory, 0);
    }

    VkFormat findDepthFormat()
    {
        const VkFormat candidates[] =
        {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
        };

        for (VkFormat format : candidates)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &properties);

            if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                return format;
            }
        }

        return VK_FORMAT_D32_SFLOAT;
    }

    void createDepthResources(VkExtent2D extent)
    {
        m_depthFormat = findDepthFormat();

        VkImageCreateInfo imageInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = m_depthFormat,
            .extent = { extent.width, extent.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        VkResult result = vkCreateImage(m_device, &imageInfo, nullptr, &m_depthImage);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateImage: {}", getString(result));
            return;
        }

        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(m_device, m_depthImage, &requirements);

        VkMemoryAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirements.size,
            .memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        };

        result = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_depthImageMemory);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkAllocateMemory: {}", getString(result));
            return;
        }

        vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0);

        VkImageViewCreateInfo viewInfo =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_depthImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_depthFormat,
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        result = vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthImageView);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateImageView: {}", getString(result));
        }
    }

    void destroyDepthResources()
    {
        if (m_depthImageView)
        {
            vkDestroyImageView(m_device, m_depthImageView, nullptr);
            m_depthImageView = VK_NULL_HANDLE;
        }

        if (m_depthImage)
        {
            vkDestroyImage(m_device, m_depthImage, nullptr);
            m_depthImage = VK_NULL_HANDLE;
        }

        if (m_depthImageMemory)
        {
            vkFreeMemory(m_device, m_depthImageMemory, nullptr);
            m_depthImageMemory = VK_NULL_HANDLE;
        }
    }

    void createShaders()
    {
        Compiler compiler;

        Shader vertexShader = compiler.compile(g_vertexShader, ShaderStage::Vertex);
        Shader fragmentShader = compiler.compile(g_fragmentShader, ShaderStage::Fragment);

        printLine(Print::Info, "");
        vertexShader.print();
        fragmentShader.print();

        if (!vertexShader.valid() || !fragmentShader.valid())
        {
            printLine(Print::Error, "Shader compilation failed.");
            if (!vertexShader.log.empty())
            {
                printLine(Print::Error, "{}", vertexShader.log);
            }
            if (!fragmentShader.log.empty())
            {
                printLine(Print::Error, "{}", fragmentShader.log);
            }
            return;
        }

        m_vertexShader = Compiler::createShaderModule(m_device, vertexShader);
        m_fragmentShader = Compiler::createShaderModule(m_device, fragmentShader);
    }

    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding binding =
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &binding,
        };

        vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);
    }

    void createDescriptorPoolAndSet()
    {
        VkDescriptorPoolSize poolSize =
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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
            .pSetLayouts = &m_descriptorSetLayout,
        };

        vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet);

        VkDescriptorBufferInfo bufferInfo =
        {
            .buffer = m_uniformBuffer,
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };

        VkWriteDescriptorSet descriptorWrite =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo,
        };

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }

    void destroyPipeline()
    {
        if (m_graphicsPipeline)
        {
            vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
            m_graphicsPipeline = VK_NULL_HANDLE;
        }

        if (m_pipelineLayout)
        {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }
    }

    void createPipeline(VkExtent2D extent)
    {
        destroyPipeline();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptorSetLayout,
        };

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

        VkPipelineShaderStageCreateInfo shaderStages[] =
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

        VkVertexInputAttributeDescription attributeDescriptions[] =
        {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, position),
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color),
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

        VkViewport viewport =
        {
            .width = float(extent.width),
            .height = float(extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D scissor =
        {
            .extent = extent,
        };

        VkPipelineViewportStateCreateInfo viewportState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor,
        };

        VkPipelineRasterizationStateCreateInfo rasterizer =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .lineWidth = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisampling =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };

        VkPipelineDepthStencilStateCreateInfo depthStencil =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
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

        VkGraphicsPipelineCreateInfo pipelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .layout = m_pipelineLayout,
            .renderPass = m_renderPass,
            .subpass = 0,
        };

        vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
    }

    void createGeometry()
    {
        const Vertex vertices[] =
        {
            { { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
            { {  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
            { {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } },
            { { -1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f } },
            { { -1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f, 1.0f } },
            { {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f, 1.0f } },
            { {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f } },
            { { -1.0f,  1.0f,  1.0f }, { 0.2f, 0.2f, 0.2f } },
        };

        const u16 indices[] =
        {
            0, 1, 2, 2, 3, 0,
            1, 5, 6, 6, 2, 1,
            5, 4, 7, 7, 6, 5,
            4, 0, 3, 3, 7, 4,
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4,
        };

        const VkDeviceSize vertexBufferSize = sizeof(vertices);
        const VkDeviceSize indexBufferSize = sizeof(indices);

        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_vertexBuffer, m_vertexBufferMemory);

        createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_indexBuffer, m_indexBufferMemory);

        void* data = nullptr;
        vkMapMemory(m_device, m_vertexBufferMemory, 0, vertexBufferSize, 0, &data);
        std::memcpy(data, vertices, sizeof(vertices));
        vkUnmapMemory(m_device, m_vertexBufferMemory);

        vkMapMemory(m_device, m_indexBufferMemory, 0, indexBufferSize, 0, &data);
        std::memcpy(data, indices, sizeof(indices));
        vkUnmapMemory(m_device, m_indexBufferMemory);

        createBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffer, m_uniformBufferMemory);
    }

    void createRenderPass()
    {
        VkFormat colorFormat = m_swapchain->getFormat();

        VkAttachmentDescription attachments[] =
        {
            {
                .format = colorFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
            {
                .format = m_depthFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
        };

        VkAttachmentReference colorRef = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        VkAttachmentReference depthRef = { .attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpass =
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorRef,
            .pDepthStencilAttachment = &depthRef,
        };

        VkRenderPassCreateInfo renderPassInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .subpassCount = 1,
            .pSubpasses = &subpass,
        };

        vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
    }

    void createFramebuffers(VkExtent2D extent)
    {
        m_framebuffers.resize(m_swapchain->getImageCount());

        for (u32 i = 0; i < m_swapchain->getImageCount(); ++i)
        {
            VkImageView attachments[] =
            {
                m_swapchain->getImageView(i),
                m_depthImageView,
            };

            VkFramebufferCreateInfo framebufferInfo =
            {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_renderPass,
                .attachmentCount = 2,
                .pAttachments = attachments,
                .width = extent.width,
                .height = extent.height,
                .layers = 1,
            };

            vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffers[i]);
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex, VkExtent2D extent)
    {
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkClearValue clearValues[2] = {};
        clearValues[0].color = {{ 0.1f, 0.14f, 0.23f, 1.0f }};
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = m_renderPass,
            .framebuffer = m_framebuffers[imageIndex],
            .renderArea = { .extent = extent },
            .clearValueCount = 2,
            .pClearValues = clearValues,
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

        VkBuffer vertexBuffers[] = { m_vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);
    }

    void updateUniformBuffer()
    {
        const VkExtent2D extent = m_swapchain->getExtent();
        const float aspect = float(extent.width) / float(extent.height);

        const float time = float(mango::Time::us() - m_startTime) / 1000000.0f;

        Matrix4x4 model = Matrix4x4::rotateXYZ(time * 1.00f, time * 1.22f, time * 0.30f);
        Matrix4x4 view = Matrix4x4::translate(0.0f, 0.0f, -4.0f);

        float xfov = 1.2f;
        float yfov = xfov;
        if (aspect > 1.0f)
        {
            yfov /= aspect;
        }
        else
        {
            xfov *= aspect;
        }

        Matrix4x4 projection = Matrix4x4::perspectiveVK(xfov, yfov, 0.1f, 100.0f);

        UniformBufferObject ubo;
        ubo.mvp = model * view * projection;

        void* data = nullptr;
        vkMapMemory(m_device, m_uniformBufferMemory, 0, sizeof(ubo), 0, &data);
        std::memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(m_device, m_uniformBufferMemory);
    }

public:
    TestWindow(VkInstance instance, int width, int height, u32 flags)
        : VulkanWindow(instance, width, height, flags)
    {
        m_startTime = mango::Time::us();
        m_physicalDevice = selectPhysicalDevice(instance);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties = getPhysicalDeviceQueueFamilyProperties(m_physicalDevice);

        u32 selectedQueueFamilyIndex = UINT32_MAX;

        for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
        {
            const VkQueueFamilyProperties& properties = queueFamilyProperties[i];
            bool presentationSupport = getPresentationSupport(m_physicalDevice, u32(i), *this);
            bool graphics = properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;

            if (graphics && presentationSupport)
            {
                selectedQueueFamilyIndex = u32(i);
            }
        }

        if (selectedQueueFamilyIndex == UINT32_MAX)
        {
            printLine(Print::Error, "Couldn't find suitable queue.");
            return;
        }

        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = selectedQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        std::vector<const char*> deviceExtensions = vulkan::requiredDeviceExtensions();

        VkDeviceCreateInfo deviceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = u32(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
        };

        VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
        if (result != VK_SUCCESS)
        {
            printLine(Print::Error, "vkCreateDevice: {}", getString(result));
            return;
        }

        m_graphicsQueueFamilyIndex = selectedQueueFamilyIndex;
        vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);


        //VkBool32 supported = VK_FALSE;
        //vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsQueueFamilyIndex, m_surface, &supported);
        //printLine("vkGetPhysicalDeviceSurfaceSupportKHR: {}", supported);

        /*
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);
        printLine("PhysicalDeviceSurface.Extent: {} x {}", caps.currentExtent.width, caps.currentExtent.height);

        m_extent = caps.currentExtent;
        */

        const VkSurfaceFormatKHR formatSDR =
        {
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };

        const VkSurfaceFormatKHR formatHDR =
        {
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .colorSpace = VK_COLOR_SPACE_BT2020_LINEAR_EXT
        };

        MANGO_UNREFERENCED(formatSDR);
        MANGO_UNREFERENCED(formatHDR);

        VkSurfaceFormatKHR preferredFormat = formatSDR;

        std::vector<VkSurfaceFormatKHR> surfaceFormats = getSurfaceFormats(m_physicalDevice, m_surface);

        size_t selectedFormatIndex = 0;
        VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];

        for (size_t i = 0; i < surfaceFormats.size(); ++i)
        {
            if (surfaceFormats[i].format == preferredFormat.format &&
                surfaceFormats[i].colorSpace == preferredFormat.colorSpace)
            {
                selectedFormatIndex = i;
                selectedFormat = surfaceFormats[i];
                break;
            }
        }

        printLine(Print::Info, "");
        printLine(Print::Info, "PhysicalDeviceSurfaceFormats:");

        for (size_t i = 0; i < surfaceFormats.size(); ++i)
        {
            std::string_view prefix = i == selectedFormatIndex ? ">" : " ";
            printLine(Print::Info, "  {} {} | {}", prefix, getString(surfaceFormats[i].format), getString(surfaceFormats[i].colorSpace));
        }

        m_swapchain = std::make_unique<vulkan::Swapchain>(m_device, m_physicalDevice, m_surface, selectedFormat, m_graphicsQueue, this);

        VkCommandPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
        };

        vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

        m_commandBuffers.resize(m_swapchain->getImageCount());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = m_swapchain->getImageCount(),
        };

        vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, m_commandBuffers.data());

        VkExtent2D extent = m_swapchain->getExtent();

        createDepthResources(extent);
        createRenderPass();
        createShaders();
        createGeometry();
        createDescriptorSetLayout();
        createDescriptorPoolAndSet();
        createPipeline(extent);
        createFramebuffers(extent);

        setVisible(true);
    }

    ~TestWindow()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);

            m_swapchain.reset();

            destroyPipeline();

            if (m_descriptorPool)
            {
                vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
            }

            if (m_descriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
            }

            if (m_vertexShader)
            {
                vkDestroyShaderModule(m_device, m_vertexShader, nullptr);
            }

            if (m_fragmentShader)
            {
                vkDestroyShaderModule(m_device, m_fragmentShader, nullptr);
            }

            for (auto framebuffer : m_framebuffers)
            {
                vkDestroyFramebuffer(m_device, framebuffer, nullptr);
            }

            if (m_vertexBuffer)
            {
                vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
            }

            if (m_vertexBufferMemory)
            {
                vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
            }

            if (m_indexBuffer)
            {
                vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
            }

            if (m_indexBufferMemory)
            {
                vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
            }

            if (m_uniformBuffer)
            {
                vkDestroyBuffer(m_device, m_uniformBuffer, nullptr);
            }

            if (m_uniformBufferMemory)
            {
                vkFreeMemory(m_device, m_uniformBufferMemory, nullptr);
            }

            destroyDepthResources();

            if (m_renderPass)
            {
                vkDestroyRenderPass(m_device, m_renderPass, nullptr);
            }

            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            vkDestroyDevice(m_device, nullptr);
        }
    }

    void rebuildSwapchainResources()
    {
        vkDeviceWaitIdle(m_device);

        for (auto framebuffer : m_framebuffers)
        {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        m_framebuffers.clear();

        destroyPipeline();

        destroyDepthResources();

        VkExtent2D extent = m_swapchain->getExtent();
        createDepthResources(extent);
        createPipeline(extent);
        createFramebuffers(extent);
    }

    void updateSwapchain()
    {
        if (!m_swapchain->recreateSwapchain())
        {
            return;
        }

        rebuildSwapchainResources();
    }

    void render()
    {
        auto frame = m_swapchain->beginFrame();
        if (!frame)
        {
            return;
        }

        updateUniformBuffer();

        VkExtent2D extent = m_swapchain->getExtent();
        VkCommandBuffer commandBuffer = m_commandBuffers[frame.imageIndex()];
        recordCommandBuffer(commandBuffer, frame.imageIndex(), extent);
        frame.submitAndPresent(m_graphicsQueue, commandBuffer);
    }

    void onResize(int width, int height) override
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
        updateSwapchain();
    }

    void onDraw() override
    {
        u64 time0 = mango::Time::us();

        render();

        u64 time1 = mango::Time::us();
        setTitle(fmt::format("vkbasic: {:.2f} ms", (time1 - time0) / 1000.0));
    }

    void onIdle() override
    {
        onDraw();
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        MANGO_UNREFERENCED(mask);

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
    MANGO_UNREFERENCED(commands);

    printEnable(Print::Info, true);

    InstanceExtensionProperties instanceExtensionProperties;

    printLine(Print::Info, "InstanceExtensionProperties:");
    instanceExtensionProperties.print();

    std::vector<const char*> enabledLayers = { "VK_LAYER_KHRONOS_validation" };
    std::vector<const char*> enabledExtensions = vulkan::requiredSurfaceExtensions();

    if (instanceExtensionProperties.contains(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME))
    {
        enabledExtensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    }

    VkApplicationInfo applicationInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vkbasic",
        .applicationVersion = 1,
        .pEngineName = "mango",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 2, 0),
    };

    Instance instance(applicationInfo, enabledLayers, enabledExtensions);

    TestWindow window(instance, 1280, 720, 0);
    window.setTitle("vkbasic");
    window.enterEventLoop();

    return 0;
}
