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

    static constexpr size_t kFrameTimeHistory = 60;

    float m_frameTimeMs = 0.0f;
    float m_textTimeMs = 0.0f;
    std::array<float, kFrameTimeHistory> m_frameTimeHistory {};
    size_t m_frameTimeIndex = 0;
    size_t m_frameTimeCount = 0;
    std::string m_hudPrefix;
    std::string m_hudTextTime;

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
    }

    void onSwapchainResize(VkExtent2D extent) override
    {
        if (m_renderer && extent.width > 0 && extent.height > 0)
        {
            m_renderer->resize(extent);
        }
    }

    ~FontWindow()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
            m_renderer.reset();
            m_allocator.reset();
        }
    }

    void buildText()
    {
        m_renderer->beginFrame(float32x4(0.1f, 0.14f, 0.23f, 1.0f));

        TextStyle hudStyle { .color = float32x4(0.75f, 0.9f, 1.0f, 1.0f), .pixelHeight = 32.0f };
        TextStyle hudValueStyle { .color = float32x4(1.0f, 0.98f, 1.0f, 1.0f), .pixelHeight = 32.0f };

        TextCursor hud = m_renderer->cursorTopLeft(m_body, 8.0f, 32.0f, hudStyle);
        m_renderer->draw(hud, m_body, m_hudPrefix, hudStyle);
        hud.x += m_renderer->textWidth(m_body, m_hudPrefix, hudStyle);
        m_renderer->draw(hud, m_body, m_hudTextTime, hudValueStyle);

        TextStyle bodyStyle { .color = float32x4(1.0f, 1.0f, 1.0f, 1.0f), .pixelHeight = m_fontPixelHeight };
        TextCursor body = m_renderer->cursorTopLeft(m_body, 40.0f, 132.0f, bodyStyle);

        for (const std::string& line : m_lines)
        {
            m_renderer->drawLine(body, m_body, line, bodyStyle);
        }
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

        const u64 text_begin = Time::us();

        buildText();

        swapchain().cmdTransitionImageToColorAttachment(cmd, imageIndex);
        m_renderer->encode(cmd,
        {
            .imageView = swapchain().getImageView(imageIndex),
            .extent = extent,
            .mode = ResolveMode::Overlay,
            .clearTarget = true,
        });
        swapchain().cmdTransitionImageToPresent(cmd, imageIndex);

        m_textTimeMs = float(Time::us() - text_begin) / 1000.0f;

        vkEndCommandBuffer(cmd);
    }

    void render()
    {
        auto frame = beginDraw();
        if (!frame)
        {
            return;
        }

        VkCommandBuffer cmd = commandBuffer(frame.imageIndex());
        recordCommandBuffer(cmd, frame.imageIndex(), swapchainExtent());
        frame.submit(m_graphicsQueue, cmd);
        frame.present();
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
        m_hudPrefix = fmt::format("frame: {:6.3f} ms ({:3.0f} fps)  text: ", m_frameTimeMs, fps);
        m_hudTextTime = fmt::format("{:6.3f} ms", m_textTimeMs);

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
