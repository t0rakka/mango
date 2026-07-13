/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/image/image.hpp>
#include <mango/vulkan/vulkan.hpp>

namespace mango::vulkan
{

    // -------------------------------------------------------------------
    // VulkanFramebuffer
    // -------------------------------------------------------------------

    class VulkanFramebuffer : public VulkanWindow
    {
    public:
        enum BufferMode
        {
            RGBA_DIRECT,
            BGRA_DIRECT,
            RGBA_FLOAT,
            BGRA_FLOAT,
            RGBA_PALETTE,
            BGRA_PALETTE,
        };

        enum Filter
        {
            FILTER_NEAREST,
            FILTER_BILINEAR,
        };

    private:
        int m_width = 0;
        int m_height = 0;
        image::Format m_format;
        size_t m_stride = 0;
        bool m_is_rgba = true;
        bool m_is_float = false;
        bool m_is_palette = false;
        float m_exposure = 1.0f;
        u32 m_palette[256] = {};

        VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_stagingMemory = VK_NULL_HANDLE;

        VkImage m_indexTexture = VK_NULL_HANDLE;
        VkDeviceMemory m_indexTextureMemory = VK_NULL_HANDLE;
        VkImageView m_indexTextureView = VK_NULL_HANDLE;

        VkBuffer m_paletteBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_paletteMemory = VK_NULL_HANDLE;

        VkImage m_texture = VK_NULL_HANDLE;
        VkDeviceMemory m_textureMemory = VK_NULL_HANDLE;
        VkImageView m_textureView = VK_NULL_HANDLE;

        VkSampler m_samplerNearest = VK_NULL_HANDLE;
        VkSampler m_samplerLinear = VK_NULL_HANDLE;

        VkShaderModule m_vertexShader = VK_NULL_HANDLE;
        VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
        VkShaderModule m_resolveFragmentShader = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_resolvePipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_resolvePipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_resolveDescriptorLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorPool m_resolveDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSet m_resolveDescriptorSet = VK_NULL_HANDLE;

        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

        VkCommandPool m_uploadPool = VK_NULL_HANDLE;
        VkCommandBuffer m_uploadCommand = VK_NULL_HANDLE;
        VkFence m_uploadFence = VK_NULL_HANDLE;

        bool m_gpu_ready = false;
        bool m_texture_uploaded = false;
        bool m_index_texture_uploaded = false;

        void createStagingBuffer();
        void createIndexTexture();
        void createTexture();
        void createPaletteBuffer();
        void createSamplers();
        void createPresentPipeline();
        void createResolvePipeline();
        void createQuadGeometry();
        void createUploadCommands();
        void destroyGpuResources();
        void uploadTexture();
        void uploadIndexTexture();
        void uploadPalette();
        void recordResolve(VkCommandBuffer commandBuffer);
        void recordPresent(VkCommandBuffer commandBuffer, u32 imageIndex, Filter filter);

    protected:
        void onDeviceReady() override;

    public:
        VulkanFramebuffer(VkInstance instance, int width, int height, BufferMode buffermode = RGBA_DIRECT);
        ~VulkanFramebuffer();

        int contentWidth() const { return m_width; }
        int contentHeight() const { return m_height; }
        bool isFloat() const { return m_is_float; }

        void setExposure(float exposure) { m_exposure = exposure; }
        float exposure() const { return m_exposure; }

        void setPalette(const image::Palette& palette);

        image::Surface lock();
        void unlock();
        void present(Filter filter = FILTER_NEAREST);
    };

} // namespace mango::vulkan
