/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <cstdio>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define MANGO_IMPLEMENT_MAIN
#include <mango/core/core.hpp>
#include <mango/filesystem/filesystem.hpp>
#include <mango/image/image.hpp>
#include <mango/vulkan/vulkan.hpp>

using namespace mango;
using namespace mango::vulkan;

namespace
{

    struct Args
    {
        std::vector<std::string> files;
        bool info = false;
        bool validate = false;
        bool cpu = false;
        bool no_wait = false;
        bool all = false;
        bool quit = false;
        bool verify = false;
        float min_mp = 1.0f;
        int warmup = 0;   // discarded runs per file before the timed ones
        int count = 1;    // timed runs per file (GPU stays warm across them)
    };

    std::string megabytes(size_t bytes)
    {
        return fmt::format("{:.1f} MB", double(bytes) / (1024.0 * 1024.0));
    }

    bool isSrgbFormat(VkFormat format)
    {
        return getString(format).find("SRGB") != std::string_view::npos;
    }

    std::string optionalMs(double ms)
    {
        return ms > 0.0 ? fmt::format("{:.2f} ms", ms) : std::string("-");
    }

    void printReport(const std::string& name, size_t index, size_t count, const TextureResult& result,
                     bool warmup_run = false)
    {
        const TextureReport& r = result.report;
        const TextureInfo& info = result.info;

        printLine("[{}/{}] {}{}", index + 1, count, name, warmup_run ? "  (warmup)" : "");
        printLine("  status    : {}{}", getString(result.status),
            result.error.empty() ? std::string() : " (" + result.error + ")");
        printLine("  path      : {}{}", getString(r.path),
            r.fallback_reason.empty() ? std::string() : " (fallback: " + r.fallback_reason + ")");
        printLine("  source    : {}, {} x {}", r.source_format, info.source_width, info.source_height);
        printLine("  texture   : {} x {} {} (image {}){}", info.width, info.height,
            getString(info.format), getString(info.image_format),
            r.downscaled ? ", downscaled to fit maxImageDimension2D" : "");
        printLine("  color     : {}, transform: {}, hdr: {}, tonemap: {}{}", r.color_source,
            r.color_transform ? "linearize to BT.709 fp16" : "none",
            r.hdr ? "yes" : "no", info.needs_tonemap ? "yes" : "no",
            r.ultrahdr ? ", UltraHDR gain map applied" : "");
        if (r.icc_present)
            printLine("  icc       : \"{}\", {}", r.icc_description, r.icc_handling);
        else
            printLine("  icc       : none");
        printLine("  stages    : header {}, image {}, prepare {} (compile {}), decode {}, convert {}, downscale {}, staging {}",
            optionalMs(r.header_ms), optionalMs(r.image_ms), optionalMs(r.prepare_ms), optionalMs(r.compile_ms),
            optionalMs(r.decode_ms), optionalMs(r.convert_ms), optionalMs(r.downscale_ms), optionalMs(r.staging_ms));
        printLine("  queue     : submitted at {}, gpu {}", optionalMs(r.submit_ms), optionalMs(r.gpu_ms));

        // DECODE / READY match gpujpeg: GPU timestamps and submit→fence wall.
        // VISIBLE is when the callback ran (owner thread); irrelevant to how many
        // textures are already sampleable.
        const double mp = double(info.source_width) * double(info.source_height) / 1e6;
        auto mps = [mp] (double ms) { return ms > 0.0 ? mp / (ms / 1000.0) : 0.0; };

        double decode_ms = 0.0;
        if (r.path == TextureDecodePath::Compute)
        {
            decode_ms = r.gpu_ms;
            const double host_ms = r.prepare_ms - r.compile_ms;
            if (r.compile_ms > 0.05)
                printLine("  DECODE    : {:.2f} ms (+ {:.2f} ms shader compile)", r.gpu_ms, r.compile_ms);
            else
                printLine("  DECODE    : {:.2f} ms", r.gpu_ms);
            printLine("  HOST      : {:.2f} ms prepare", host_ms);
        }
        else
        {
            decode_ms = r.decode_ms + r.convert_ms + r.downscale_ms;
            printLine("  DECODE    : {:.2f} ms", decode_ms);
        }
        printLine("  READY     : {:.2f} ms   (VkImage usable, submit→fence)", r.ready_ms);
        printLine("  VISIBLE   : {:.2f} ms   (callback / first chance to draw)", r.visible_ms);
        printLine("  RATE      : {:.0f} MP/s decode, {:.0f} MP/s ready ({:.1f} MP)",
            mps(decode_ms), mps(r.ready_ms), mp);
        printLine("  memory    : host peak {}, staging {}, image {}, process peak RSS {}",
            megabytes(r.peak_host_bytes), megabytes(r.staging_bytes),
            megabytes(r.image_bytes), megabytes(processPeakBytes()));
        printLine("");
    }

} // namespace

// ------------------------------------------------------------------------------

static const char* g_vertexShader = R"(#version 450
    layout(push_constant) uniform Push
    {
        vec4 rect;      // NDC x, y, width, height
        int tonemap;
        int encode;
    } pc;

    layout(location = 0) out vec2 uv;

    void main()
    {
        vec2 p = vec2(gl_VertexIndex & 1, gl_VertexIndex >> 1);
        uv = p;
        gl_Position = vec4(pc.rect.xy + p * pc.rect.zw, 0.0, 1.0);
    }
)";

static const char* g_fragmentShader = R"(#version 450
    layout(push_constant) uniform Push
    {
        vec4 rect;
        int tonemap;
        int encode;
    } pc;

    layout(binding = 0) uniform sampler2D tex;

    layout(location = 0) in vec2 uv;
    layout(location = 0) out vec4 outColor;

    vec3 srgbEncode(vec3 c)
    {
        c = clamp(c, 0.0, 1.0);
        return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
    }

    void main()
    {
        // The sampler returns linear light: _SRGB views decode in hardware,
        // the other formats store linear values.
        vec4 c = texture(tex, uv);
        vec3 rgb = c.rgb;
        if (pc.tonemap != 0)
            rgb = rgb / (1.0 + rgb);

        ivec2 cell = ivec2(gl_FragCoord.xy) / 16;
        vec3 checker = vec3(((cell.x + cell.y) & 1) != 0 ? 0.20 : 0.10);
        rgb = mix(checker, rgb, clamp(c.a, 0.0, 1.0));

        if (pc.encode != 0)
            rgb = srgbEncode(rgb);
        outColor = vec4(rgb, 1.0);
    }
)";

// ------------------------------------------------------------------------------

class ImageViewWindow : public VulkanWindow
{
protected:
    struct Push
    {
        float rect[4];
        s32 tonemap;
        s32 encode;
    };

    struct Texture
    {
        ImageAllocation image;
        VkImageView view = VK_NULL_HANDLE;
        VkDescriptorSet sets[2] {};
        TextureInfo info;
        std::string name;

        explicit operator bool () const
        {
            return image.image != VK_NULL_HANDLE;
        }
    };

    struct Retired
    {
        Texture texture;
        u64 frame = 0;
    };

    const Args& m_args;

    std::unique_ptr<Allocator> m_allocator;
    std::unique_ptr<TextureDecoder> m_decoder;

    VkShaderModule m_vertexShader = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkSampler m_samplers[2] {};
    int m_filter = 1;

    Texture m_current;
    std::deque<Retired> m_retired;
    u64 m_frame = 0;

    size_t m_index = 0;
    TextureDecoder::Handle m_wanted = 0;
    bool m_advance_after_present = false;
    std::unordered_map<TextureDecoder::Handle, std::unique_ptr<filesystem::File>> m_files;

    void createShaders()
    {
        Compiler compiler;
        Shader vs = compiler.compile(g_vertexShader, ShaderStage::Vertex);
        Shader fs = compiler.compile(g_fragmentShader, ShaderStage::Fragment);
        if (!vs || !fs)
        {
            printLine(Print::Error, "shader compile failed:\n{}\n{}", vs.log, fs.log);
            return;
        }
        m_vertexShader = Compiler::createShaderModule(device(), vs);
        m_fragmentShader = Compiler::createShaderModule(device(), fs);
    }

    void createPipeline()
    {
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
        vkCreateDescriptorSetLayout(device(), &layoutInfo, nullptr, &m_setLayout);

        VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32 };
        VkDescriptorPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 32,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
        };
        vkCreateDescriptorPool(device(), &poolInfo, nullptr, &m_descriptorPool);

        VkPushConstantRange range = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Push) };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_setLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &range,
        };
        vkCreatePipelineLayout(device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

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

        VkPipelineVertexInputStateCreateInfo vertexInput =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
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
        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
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
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
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
        vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

        for (int i = 0; i < 2; ++i)
        {
            const VkFilter filter = i ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
            VkSamplerCreateInfo samplerInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = filter,
                .minFilter = filter,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .maxLod = 0.0f,
            };
            vkCreateSampler(device(), &samplerInfo, nullptr, &m_samplers[i]);
        }
    }

    void destroyTexture(Texture& texture)
    {
        if (texture.sets[0])
            vkFreeDescriptorSets(device(), m_descriptorPool, 2, texture.sets);
        if (texture.view)
            vkDestroyImageView(device(), texture.view, nullptr);
        if (texture.image)
            m_allocator->destroyImage(texture.image);
        texture = Texture();
    }

    // Frames still in flight may sample the old texture; the swapchain never
    // runs more than a few frames ahead.
    void retire(Texture& texture)
    {
        if (texture)
            m_retired.push_back({ std::move(texture), m_frame });
        texture = Texture();
    }

    void collectRetired(bool all)
    {
        while (!m_retired.empty() && (all || m_frame >= m_retired.front().frame + 4))
        {
            destroyTexture(m_retired.front().texture);
            m_retired.pop_front();
        }
    }

    void request(size_t index)
    {
        if (m_args.files.empty())
            return;

        m_index = index % m_args.files.size();
        const std::string& filename = m_args.files[m_index];

        if (m_wanted)
            m_decoder->cancel(m_wanted);

        // Clear any in-flight present before the decode timer starts. Compute
        // shares the graphics queue; a submit behind vsync inflates READY by a
        // full refresh (the ~8 ms gap vs gpujpeg).
        vkQueueWaitIdle(graphicsQueue());

        std::unique_ptr<filesystem::File> file;
        try
        {
            file = std::make_unique<filesystem::File>(filename);
        }
        catch (const std::exception& e)
        {
            printLine(Print::Error, "{}: {}", filename, e.what());
            advance();
            return;
        }

        const size_t requested = m_index;
        TextureDecoder::Texture texture = m_decoder->decode(*file, filename,
            [this, filename, requested] (const TextureResult& result)
            {
                onDecoded(filename, requested, result);
            });

        if (!texture)
        {
            printLine("[{}/{}] {}", m_index + 1, m_args.files.size(), filename);
            printLine("  status    : header failed ({})\n", texture.error);
            m_wanted = 0;
            advance();
            return;
        }

        m_wanted = texture.handle;
        m_files[texture.handle] = std::move(file);
    }

    void advance()
    {
        // Defer the next decode until after present so it does not land behind
        // this frame's vsync on the shared graphics queue.
        if (!m_args.all)
            return;
        m_advance_after_present = true;
    }

    void advanceNow()
    {
        m_advance_after_present = false;
        if (!m_args.all)
            return;

        if (m_index + 1 < m_args.files.size())
            request(m_index + 1);
        else if (m_args.quit)
            requestQuit();
    }

    // Reads the texture back and compares it with ImageDecoder's RGBA8 output.
    void verify(const std::string& filename, const TextureResult& result)
    {
        const TextureInfo& info = result.info;
        if (info.image_format != VK_FORMAT_R8G8B8A8_UNORM && info.image_format != VK_FORMAT_R8G8B8A8_SRGB)
        {
            printLine("  verify    : skipped ({})\n", getString(info.image_format));
            return;
        }
        if (info.width != info.source_width || info.height != info.source_height)
        {
            printLine("  verify    : skipped (downscaled)\n");
            return;
        }

        const size_t bytes = size_t(info.width) * info.height * 4;
        BufferAllocation buffer = m_allocator->createBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, MemoryUsage::Readback, true);

        VkCommandBufferAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(device(), &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(cmd, &beginInfo);

        auto transition = [&] (VkImageLayout from, VkImageLayout to)
        {
            VkImageMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .oldLayout = from,
                .newLayout = to,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = result.image.image,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);
        };

        transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkBufferImageCopy region =
        {
            .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
            .imageExtent = { info.width, info.height, 1 },
        };
        vkCmdCopyImageToBuffer(cmd, result.image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer.buffer, 1, &region);
        transition(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        };
        vkQueueSubmit(graphicsQueue(), 1, &submit, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue());
        vkFreeCommandBuffers(device(), commandPool(), 1, &cmd);
        m_allocator->invalidate(buffer.allocation, 0, bytes);

        filesystem::File file(filename);
        image::Bitmap reference(file, image::Format(32, image::Format::UNORM, image::Format::RGBA, 8, 8, 8, 8));

        const u8* gpu = static_cast<const u8*>(buffer.mapped);
        int maxdiff = 0;
        size_t over = 0;
        for (u32 y = 0; y < info.height; ++y)
        {
            const u8* a = gpu + size_t(y) * info.width * 4;
            const u8* b = reference.address(0, int(y));
            for (u32 x = 0; x < info.width * 4; ++x)
            {
                const int d = std::abs(int(a[x]) - int(b[x]));
                maxdiff = std::max(maxdiff, d);
                over += d > 4;
            }
        }
        m_allocator->destroyBuffer(buffer);

        printLine("  verify    : max |texture - cpu| = {}, {} samples over 4 {}\n", maxdiff, over,
            maxdiff <= 4 ? "(OK)" : "(MISMATCH)");
    }

    void onDecoded(const std::string& filename, size_t index, const TextureResult& result)
    {
        m_files.erase(result.handle);
        const int stride = m_args.warmup + m_args.count;
        const bool warmup_run = stride > 1 && m_args.warmup > 0 &&
            int(index % size_t(stride)) < m_args.warmup;
        printReport(filename, index, m_args.files.size(), result, warmup_run);

        if (m_args.verify && result.status == TextureStatus::Ready)
            verify(filename, result);

        const bool wanted = result.handle == m_wanted;
        if (wanted)
            m_wanted = 0;

        if (result.status != TextureStatus::Ready || !wanted)
        {
            m_allocator->destroyImage(result.image);
            if (wanted)
                advance();
            return;
        }

        Texture texture;
        texture.image = result.image;
        texture.info = result.info;
        texture.name = filename;
        texture.view = createTextureView(device(), result.image, result.info);

        VkDescriptorSetLayout layouts[2] = { m_setLayout, m_setLayout };
        VkDescriptorSetAllocateInfo allocInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 2,
            .pSetLayouts = layouts,
        };
        vkAllocateDescriptorSets(device(), &allocInfo, texture.sets);

        for (int i = 0; i < 2; ++i)
        {
            VkDescriptorImageInfo imageInfo =
            {
                .sampler = m_samplers[i],
                .imageView = texture.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkWriteDescriptorSet write =
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = texture.sets[i],
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            };
            vkUpdateDescriptorSets(device(), 1, &write, 0, nullptr);
        }

        retire(m_current);
        m_current = std::move(texture);

        setTitle(fmt::format("vkimageview: [{}/{}] {} ({} x {})", index + 1, m_args.files.size(),
            filesystem::removePath(filename), m_current.info.width, m_current.info.height));

        advance();
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

        swapchain().transitionImageToColorAttachment(cmd, imageIndex);

        VkRenderingAttachmentInfo colorAttachment =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain().getImageView(imageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = {{ 0.05f, 0.05f, 0.05f, 1.0f }} },
        };
        VkRenderingInfo renderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { .extent = extent },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };
        vkCmdBeginRendering(cmd, &renderingInfo);

        if (m_current && m_pipeline)
        {
            VkViewport viewport = { 0.0f, 0.0f, float(extent.width), float(extent.height), 0.0f, 1.0f };
            VkRect2D scissor = { { 0, 0 }, extent };
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            // Fit the image inside the window, never magnified past 1:1.
            const float ww = float(extent.width);
            const float wh = float(extent.height);
            const float iw = float(m_current.info.width);
            const float ih = float(m_current.info.height);
            const float scale = std::min({ ww / iw, wh / ih, 1.0f });
            const float w = 2.0f * iw * scale / ww;
            const float h = 2.0f * ih * scale / wh;

            Push push = {};
            push.rect[0] = -w * 0.5f;
            push.rect[1] = -h * 0.5f;
            push.rect[2] = w;
            push.rect[3] = h;
            push.tonemap = m_current.info.needs_tonemap ? 1 : 0;
            push.encode = isSrgbFormat(swapchain().getFormat()) ? 0 : 1;

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                &m_current.sets[m_filter], 0, nullptr);
            vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(Push), &push);
            vkCmdDraw(cmd, 4, 1, 0, 0);
        }

        vkCmdEndRendering(cmd);
        swapchain().transitionImageToPresent(cmd, imageIndex);
        vkEndCommandBuffer(cmd);
    }

public:
    ImageViewWindow(VulkanContext& context, const Args& args)
        : VulkanWindow(context, 1280, 800, 0)
        , m_args(args)
    {
    }

    ~ImageViewWindow()
    {
        if (device() == VK_NULL_HANDLE)
            return;

        // Fires the remaining callbacks (Cancelled) before the window goes away.
        m_decoder.reset();

        vkDeviceWaitIdle(device());

        retire(m_current);
        collectRetired(true);

        for (VkSampler sampler : m_samplers)
            if (sampler)
                vkDestroySampler(device(), sampler, nullptr);
        if (m_pipeline)
            vkDestroyPipeline(device(), m_pipeline, nullptr);
        if (m_pipelineLayout)
            vkDestroyPipelineLayout(device(), m_pipelineLayout, nullptr);
        if (m_descriptorPool)
            vkDestroyDescriptorPool(device(), m_descriptorPool, nullptr);
        if (m_setLayout)
            vkDestroyDescriptorSetLayout(device(), m_setLayout, nullptr);
        if (m_vertexShader)
            vkDestroyShaderModule(device(), m_vertexShader, nullptr);
        if (m_fragmentShader)
            vkDestroyShaderModule(device(), m_fragmentShader, nullptr);

        m_allocator.reset();
    }

    void onDeviceReady() override
    {
        m_allocator = std::make_unique<Allocator>(instance(), physicalDevice(), device(), VK_API_VERSION_1_3);

        TextureDecoderConfig config;
        config.compute = !m_args.cpu;
        config.compute_min_pixels = u64(double(m_args.min_mp) * 1e6);
        config.wait_for_pipelines = !m_args.no_wait;
        config.wake = [this]
        {
            // invalidate alone can wait for the next vsync-aligned pass; a due
            // deadline wakes the Cocoa loop on the next iteration.
            invalidate();
            requestFrameIn(0);
        };
        m_decoder = std::make_unique<TextureDecoder>(context(), *m_allocator, config);

        createShaders();
        createPipeline();

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(physicalDevice(), &props);
        printLine("device    : {}, maxImageDimension2D {}", props.deviceName, props.limits.maxImageDimension2D);
        printLine("swapchain : {}", getString(swapchain().getFormat()));
        printLine("decoder   : compute {}, compute_min_pixels {}, wait_for_pipelines {}\n",
            config.compute ? "on" : "off", config.compute_min_pixels, config.wait_for_pipelines);

        request(0);
    }

    void onFrame(const FrameInfo& info) override
    {
        MANGO_UNREFERENCED(info);

        if (!m_decoder)
            return;

        m_decoder->process();

        // Drain in-flight decodes before acquiring the swapchain so present does
        // not share the queue with compute.
        if (m_decoder->pending())
            m_decoder->wait();

        auto frame = beginDraw();
        if (!frame)
            return;

        ++m_frame;
        collectRetired(false);

        const VkExtent2D extent = swapchainExtent();
        VkCommandBuffer cmd = commandBuffer(frame.imageIndex());
        recordCommandBuffer(cmd, frame.imageIndex(), extent);
        frame.submitAndPresent(graphicsQueue(), cmd);

        // Start the next file after present, then drain it in this turn so
        // prepare→submit does not wait for another vsync-aligned onFrame.
        if (m_advance_after_present)
        {
            advanceNow();
            if (m_decoder->pending())
                m_decoder->wait();
        }

        // On-demand loop: keep drawing until retired textures are released.
        if (!m_retired.empty())
        {
            invalidate();
            requestFrameIn(0);
        }
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        MANGO_UNREFERENCED(mask);

        const size_t count = std::max<size_t>(m_args.files.size(), 1);

        switch (code)
        {
            case KEYCODE_ESC:
                requestQuit();
                break;

            case KEYCODE_F:
                toggleFullscreen();
                break;

            case KEYCODE_1:
                m_filter = 0;
                invalidate();
                break;

            case KEYCODE_2:
                m_filter = 1;
                invalidate();
                break;

            case KEYCODE_R:
                request(m_index);
                if (m_decoder->pending())
                    m_decoder->wait();
                invalidate();
                break;

            case KEYCODE_LEFT:
                request(m_index + count - 1);
                break;

            case KEYCODE_RIGHT:
            case KEYCODE_SPACE:
                request(m_index + 1);
                break;

            default:
                break;
        }
    }
};

// ------------------------------------------------------------------------------

int mangoMain(const mango::CommandLine& commands)
{
    Args args;

    commands.usage("[options] file [file ...]");
    commands.flag("--info", "enable verbose info output", [&] { args.info = true; });
    commands.flag("--validate", "enable Khronos validation layer", [&] { args.validate = true; });
    commands.flag("--cpu", "decode everything on the CPU path", [&] { args.cpu = true; });
    commands.flag("--no-wait", "take the CPU path while compute shaders compile (application default)", [&] { args.no_wait = true; });
    commands.flag("--all", "step through every file automatically", [&] { args.all = true; });
    commands.flag("--quit", "with --all: exit after the last file", [&] { args.quit = true; });
    commands.flag("--verify", "compare 8-bit textures against a CPU decode", [&] { args.verify = true; });
    commands.optionFloat("--min-mp", "smallest JPEG (megapixels) for the compute path (default 1.0)",
        [&] (float value) { args.min_mp = value; });
    commands.optionInt("--warmup", "discarded warm-up decodes per file before timing (default 0)",
        [&] (int value) { args.warmup = std::max(value, 0); });
    commands.optionInt("--count", "timed decodes per file after warmup (default 1); keeps the GPU warm",
        [&] (int value) { args.count = std::max(value, 1); });
    commands.positional([&] (std::string_view token)
    {
        args.files.emplace_back(token);
    });

    if (!commands.parse())
    {
        return 1;
    }

    if (args.files.empty())
    {
        printLine("usage: vkimageview [options] file [file ...]");
        printLine("  keys: left/right/space = previous/next, R = reload, 1/2 = nearest/linear, F = fullscreen, Esc = quit");
        printLine("  tip:  --warmup 1 --count 8 --quit file.jpg   (gpujpeg-style warm timing)");
        return 1;
    }

    // Expand each file into warmup + count back-to-back decodes so the GPU stays
    // clocked up — same idea as typing the filename N times, without the typing.
    if (args.warmup > 0 || args.count > 1)
    {
        std::vector<std::string> playlist;
        playlist.reserve(args.files.size() * size_t(args.warmup + args.count));
        for (const std::string& file : args.files)
        {
            for (int i = 0; i < args.warmup; ++i)
                playlist.push_back(file);
            for (int i = 0; i < args.count; ++i)
                playlist.push_back(file);
        }
        args.files = std::move(playlist);
        args.all = true;
        args.quit = true;
    }

    if (args.info)
    {
        printEnable(Print::Info, true);
    }

    std::vector<const char*> layers;
    if (args.validate)
    {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    InstanceExtensionProperties instanceExtensions;
    std::vector<const char*> extensions = vulkan::requiredSurfaceExtensions();
    if (instanceExtensions.contains(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME))
    {
        extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    }

    VkApplicationInfo applicationInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vkimageview",
        .applicationVersion = 1,
        .pEngineName = "mango",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    Instance instance(applicationInfo, layers, extensions);

    VulkanContext context(instance);
    ImageViewWindow window(context, args);
    window.setTitle("vkimageview");

    EventLoopConfig config;
    config.mode = FrameMode::OnDemand;

    EventLoop loop;
    loop.attach(window, config);
    loop.run();

    return 0;
}
