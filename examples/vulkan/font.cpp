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
#include <mango/vulkan/font.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::vulkan;

class FontWindow : public VulkanWindow
{
protected:
    std::unique_ptr<Allocator> m_allocator;
    std::unique_ptr<FontRenderer> m_renderer;
    Font m_body;

    float m_fontPixelHeight = 32.0f;
    float m_frameTimeMs = 0.0f;
    static constexpr size_t kFrameTimeHistory = 60;
    std::array<float, kFrameTimeHistory> m_frameTimeHistory {};
    size_t m_frameTimeIndex = 0;
    size_t m_frameTimeCount = 0;

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

    std::string m_fontPath;
    std::string m_hudFramePrefix;
    std::string m_hudGpuTextTime;

    static constexpr float kHudPixelHeight = 32.0f;
    static constexpr u32 kTimestampsPerFrame = 4;

    enum TimestampSlot : u32
    {
        TS_FrameBegin = 0,
        TS_AfterClear = 1,
        TS_AfterText = 2,
        TS_AfterBlit = 3,
    };

    struct FrameTimings
    {
        float wait_ms = 0.0f;
        float record_ms = 0.0f;
        float record_clear_ms = 0.0f;
        float record_text_ms = 0.0f;
        float record_blit_ms = 0.0f;
        float submit_ms = 0.0f;
        float present_ms = 0.0f;
        float gpu_clear_ms = 0.0f;
        float gpu_text_ms = 0.0f;
        float gpu_blit_ms = 0.0f;
        float gpu_total_ms = 0.0f;
        float total_ms = 0.0f;
    };

    VkQueryPool m_timestampPool = VK_NULL_HANDLE;
    u32 m_timestampImageCount = 0;
    bool m_timestampsEnabled = false;
    float m_timestampPeriod = 1.0f;
    std::vector<bool> m_timestampQueryReady;
    FrameTimings m_timings {};
    static constexpr size_t kTimingHistory = 30;
    std::array<FrameTimings, kTimingHistory> m_timingHistory {};
    size_t m_timingIndex = 0;
    size_t m_timingCount = 0;

    u32 timestamp_base(u32 imageIndex) const
    {
        return imageIndex * kTimestampsPerFrame;
    }

    void createTimestampQueryPool()
    {
        VkPhysicalDeviceProperties properties {};
        vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

        std::vector<VkQueueFamilyProperties> queueFamilies =
            getPhysicalDeviceQueueFamilyProperties(m_physicalDevice);

        u32 timestamp_valid_bits = 0;
        if (m_graphicsQueueFamilyIndex < queueFamilies.size())
        {
            timestamp_valid_bits = queueFamilies[m_graphicsQueueFamilyIndex].timestampValidBits;
        }

        m_timestampsEnabled = properties.limits.timestampComputeAndGraphics && timestamp_valid_bits > 0;
        m_timestampPeriod = properties.limits.timestampPeriod;

        if (!m_timestampsEnabled)
        {
            printLine(Print::Warning, "GPU timestamps unavailable; CPU split timings only.");
            return;
        }

        const u32 query_count = swapchain().getImageCount() * kTimestampsPerFrame;
        VkQueryPoolCreateInfo poolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            .queryType = VK_QUERY_TYPE_TIMESTAMP,
            .queryCount = query_count,
        };

        vkCreateQueryPool(m_device, &poolInfo, nullptr, &m_timestampPool);
        m_timestampQueryReady.assign(swapchain().getImageCount(), false);
    }

    void destroyTimestampQueryPool()
    {
        m_timestampQueryReady.clear();
        if (m_timestampPool)
        {
            vkDestroyQueryPool(m_device, m_timestampPool, nullptr);
            m_timestampPool = VK_NULL_HANDLE;
        }
    }

    void write_timestamp(VkCommandBuffer cmd, u32 imageIndex, TimestampSlot slot)
    {
        if (!m_timestampsEnabled)
        {
            return;
        }

        VkPipelineStageFlagBits stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        switch (slot)
        {
            case TS_AfterClear: stage = VK_PIPELINE_STAGE_TRANSFER_BIT; break;
            case TS_AfterText: stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT; break;
            case TS_AfterBlit: stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; break;
            default: break;
        }

        vkCmdWriteTimestamp(cmd, stage, m_timestampPool, timestamp_base(imageIndex) + slot);
    }

    void read_gpu_timings(u32 imageIndex)
    {
        if (!m_timestampsEnabled)
        {
            return;
        }

        if (imageIndex >= m_timestampQueryReady.size() || !m_timestampQueryReady[imageIndex])
        {
            return;
        }

        u64 values[kTimestampsPerFrame] = {};
        VkResult result = vkGetQueryPoolResults(
            m_device,
            m_timestampPool,
            timestamp_base(imageIndex),
            kTimestampsPerFrame,
            sizeof(values),
            values,
            sizeof(u64),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

        if (result != VK_SUCCESS)
        {
            return;
        }

        auto delta_ms = [this](u64 a, u64 b) -> float
        {
            return float(double(b - a) * double(m_timestampPeriod) / 1'000'000.0);
        };

        m_timings.gpu_clear_ms = delta_ms(values[TS_FrameBegin], values[TS_AfterClear]);
        m_timings.gpu_text_ms = delta_ms(values[TS_AfterClear], values[TS_AfterText]);
        m_timings.gpu_blit_ms = delta_ms(values[TS_AfterText], values[TS_AfterBlit]);
        m_timings.gpu_total_ms = delta_ms(values[TS_FrameBegin], values[TS_AfterBlit]);
    }

    void push_timing_sample()
    {
        m_timingHistory[m_timingIndex] = m_timings;
        m_timingIndex = (m_timingIndex + 1) % kTimingHistory;
        m_timingCount = std::min(m_timingCount + 1, kTimingHistory);
    }

    FrameTimings averaged_timings() const
    {
        FrameTimings avg;
        if (m_timingCount == 0)
        {
            return avg;
        }

        for (size_t i = 0; i < m_timingCount; ++i)
        {
            const FrameTimings& t = m_timingHistory[i];
            avg.wait_ms += t.wait_ms;
            avg.record_ms += t.record_ms;
            avg.record_clear_ms += t.record_clear_ms;
            avg.record_text_ms += t.record_text_ms;
            avg.record_blit_ms += t.record_blit_ms;
            avg.submit_ms += t.submit_ms;
            avg.present_ms += t.present_ms;
            avg.gpu_clear_ms += t.gpu_clear_ms;
            avg.gpu_text_ms += t.gpu_text_ms;
            avg.gpu_blit_ms += t.gpu_blit_ms;
            avg.gpu_total_ms += t.gpu_total_ms;
            avg.total_ms += t.total_ms;
        }

        const float inv = 1.0f / float(m_timingCount);
        avg.wait_ms *= inv;
        avg.record_ms *= inv;
        avg.record_clear_ms *= inv;
        avg.record_text_ms *= inv;
        avg.record_blit_ms *= inv;
        avg.submit_ms *= inv;
        avg.present_ms *= inv;
        avg.gpu_clear_ms *= inv;
        avg.gpu_text_ms *= inv;
        avg.gpu_blit_ms *= inv;
        avg.gpu_total_ms *= inv;
        avg.total_ms *= inv;
        return avg;
    }

    std::string timing_title() const
    {
        const FrameTimings t = averaged_timings();
        if (m_timestampsEnabled)
        {
            return fmt::format(
                "font total {:.2f} | wait {:.2f} rec {:.2f} (clr {:.2f} txt {:.2f} blt {:.2f}) submit {:.2f} present {:.2f} | GPU {:.2f} (clr {:.2f} txt {:.2f} blt {:.2f})",
                t.total_ms,
                t.wait_ms,
                t.record_ms,
                t.record_clear_ms,
                t.record_text_ms,
                t.record_blit_ms,
                t.submit_ms,
                t.present_ms,
                t.gpu_total_ms,
                t.gpu_clear_ms,
                t.gpu_text_ms,
                t.gpu_blit_ms);
        }

        return fmt::format(
            "font total {:.2f} | wait {:.2f} rec {:.2f} (clr {:.2f} txt {:.2f} blt {:.2f}) submit {:.2f} present {:.2f} | GPU n/a",
            t.total_ms,
            t.wait_ms,
            t.record_ms,
            t.record_clear_ms,
            t.record_text_ms,
            t.record_blit_ms,
            t.submit_ms,
            t.present_ms);
    }

public:
    FontWindow(VkInstance instance, int width, int height, u32 flags, const std::string& fontPath)
        : VulkanWindow(instance, width, height, flags)
        , m_fontPath(fontPath)
    {
    }

    void onDeviceReady() override
    {
        m_allocator = std::make_unique<Allocator>(instance(), m_physicalDevice, m_device, VK_API_VERSION_1_3);

        FontRenderer::CreateInfo fontInfo =
        {
            .device = m_device,
            .queue = m_graphicsQueue,
            .queueFamily = m_graphicsQueueFamilyIndex,
            .allocator = m_allocator.get(),
            .targetFormat = surfaceFormat().format,
        };

        m_renderer = std::make_unique<FontRenderer>(fontInfo);

        m_body = m_renderer->load(m_fontPath);
        if (!m_body)
        {
            printLine(Print::Error, "Failed to load font: {}", m_fontPath);
        }
        else
        {
            m_renderer->setSize(m_body, m_fontPixelHeight);
        }

        m_renderer->resize(swapchainExtent());
        createTimestampQueryPool();
        m_timestampImageCount = swapchain().getImageCount();
    }

    void onSwapchainResize(VkExtent2D extent) override
    {
        if (m_renderer && extent.width > 0 && extent.height > 0)
        {
            m_renderer->resize(extent);
        }

        const u32 image_count = swapchain().getImageCount();
        if (image_count != m_timestampImageCount)
        {
            destroyTimestampQueryPool();
            createTimestampQueryPool();
            m_timestampImageCount = image_count;
        }
    }

    ~FontWindow()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
            destroyTimestampQueryPool();
            m_renderer.reset();
            m_allocator.reset();
        }
    }

    void recordCommandBuffer(VkCommandBuffer cmd, u32 imageIndex, VkExtent2D extent)
    {
        read_gpu_timings(imageIndex);

        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(cmd, &beginInfo);

        if (m_timestampsEnabled)
        {
            vkCmdResetQueryPool(cmd, m_timestampPool, timestamp_base(imageIndex), kTimestampsPerFrame);
            write_timestamp(cmd, imageIndex, TS_FrameBegin);
        }

        u64 t_section = Time::us();
        m_renderer->clear(cmd, float32x4(0.1f, 0.14f, 0.23f, 1.0f));
        m_timings.record_clear_ms = float(Time::us() - t_section) / 1000.0f;
        write_timestamp(cmd, imageIndex, TS_AfterClear);

        t_section = Time::us();

        m_renderer->setSize(m_body, kHudPixelHeight);
        const float hud_baseline = 32.0f + m_renderer->ascender(m_body);
        TextStyle hudPrefixStyle { .color = float32x4(0.75f, 0.9f, 1.0f, 1.0f) };
        TextStyle hudValueStyle { .color = float32x4(1.0f, 0.98f, 1.0f, 1.0f) };

        m_renderer->draw(cmd, m_body, 8.0f, hud_baseline, m_hudFramePrefix, hudPrefixStyle);
        m_renderer->draw(cmd, m_body, 8.0f + m_renderer->textWidth(m_body, m_hudFramePrefix),
            hud_baseline, m_hudGpuTextTime, hudValueStyle);

        m_renderer->setSize(m_body, m_fontPixelHeight);

        const float line_step = m_renderer->lineHeight(m_body) * 1.12f;
        float baseline_y = 132.0f;
        TextStyle bodyStyle { .color = float32x4(1.0f, 1.0f, 1.0f, 1.0f) };

        for (const std::string& line : m_lines)
        {
            m_renderer->draw(cmd, m_body, 40.0f, baseline_y, line, bodyStyle);
            baseline_y += line_step;
        }

        swapchain().cmdTransitionImageToColorAttachment(cmd, imageIndex);
        m_renderer->resolve(cmd, swapchain().getImageView(imageIndex), extent);
        swapchain().cmdTransitionImageToPresent(cmd, imageIndex);

        m_timings.record_text_ms = float(Time::us() - t_section) / 1000.0f;
        write_timestamp(cmd, imageIndex, TS_AfterText);
        write_timestamp(cmd, imageIndex, TS_AfterBlit);

        vkEndCommandBuffer(cmd);
    }

    void render()
    {
        const u64 frame_begin = Time::us();

        const u64 wait_begin = Time::us();
        auto frame = beginDraw();
        m_timings.wait_ms = float(Time::us() - wait_begin) / 1000.0f;

        if (!frame)
        {
            return;
        }

        VkExtent2D extent = swapchainExtent();

        const u64 record_begin = Time::us();
        VkCommandBuffer cmd = commandBuffer(frame.imageIndex());
        recordCommandBuffer(cmd, frame.imageIndex(), extent);
        m_timings.record_ms = float(Time::us() - record_begin) / 1000.0f;

        const u64 submit_begin = Time::us();
        frame.submit(m_graphicsQueue, cmd);
        m_timings.submit_ms = float(Time::us() - submit_begin) / 1000.0f;

        if (m_timestampsEnabled && frame.imageIndex() < m_timestampQueryReady.size())
        {
            m_timestampQueryReady[frame.imageIndex()] = true;
        }

        const u64 present_begin = Time::us();
        frame.present();
        m_timings.present_ms = float(Time::us() - present_begin) / 1000.0f;

        m_timings.total_ms = float(Time::us() - frame_begin) / 1000.0f;
        push_timing_sample();
    }

    void onFrame(const FrameInfo& info) override
    {
        constexpr float min_size = 8.0f;
        constexpr float max_size = 82.0f;
        constexpr float cycle_seconds = 20.0f;

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
        m_hudFramePrefix = fmt::format("frame: {:6.3f} ms ({:3.0f} fps)  text: ", m_frameTimeMs, fps);

        const FrameTimings avg = averaged_timings();
        const float text_ms = (m_timestampsEnabled && m_timingCount > 0) ? avg.gpu_text_ms : avg.record_text_ms;
        m_hudGpuTextTime = fmt::format("{:6.3f} ms", text_ms);

        render();

        setTitle(timing_title());
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
    std::string fontPath = "data/NotoSans-Regular.ttf";

    for (size_t i = 1; i < commands.size(); ++i)
    {
        std::string arg = std::string(commands[i]);
        if (arg[0] != '-')
        {
            fontPath = arg;
        }
    }

    std::vector<const char*> enabledLayers;
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

    FontWindow window(instance, 1280, 720, 0, fontPath);
    window.setTitle("Scanline Sweeper Font (Vulkan)");

    EventLoopConfig config;
    config.mode = FrameMode::Continuous;
    config.waitForFrame = true;

    window.enterEventLoop(config);

    return 0;
}
