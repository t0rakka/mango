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
#include <mango/vulkan/render_target.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::vulkan;

class FontWindow : public VulkanWindow
{
protected:
    std::unique_ptr<Allocator> m_allocator;
    std::unique_ptr<FontRenderer> m_renderer;
    std::unique_ptr<RenderTarget> m_renderTarget;
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

    static constexpr const char* kHudFontPath = "data/fonts/NotoSans-Regular.ttf";
    static constexpr const char* kSampleText = "The quick brown fox jumps over the lazy dog.";
    static constexpr const char* kLigatureText = "ff fi fl ffi ffl office affine";

    static constexpr float kHudTop = 4.0f;
    static constexpr float kTestLeft = 8.0f;
    static constexpr float kLigatureLeft = 640.0f;
    static constexpr float kTestTop = 22.0f;
    static constexpr float kBodyTop = 180.0f;
    static constexpr float kCompareSize = 11.0f;

    static constexpr std::array<float, 5> kTestSizes { 8.0f, 10.0f, 12.0f, 14.0f, 16.0f };

    struct HintingCompareRow
    {
        FontHinting hinting;
        const char* label;
    };

    // Add rows here when new FontHinting modes are implemented.
    static constexpr std::array<HintingCompareRow, 2> kHintingCompareRows
    {{
        { FontHinting::None, "No hinting: " },
        { FontHinting::Light, "Hinting (light): " },
    }};

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

        recreateRenderTarget(swapchainExtent());
    }

    void onSwapchainResize(VkExtent2D extent) override
    {
        recreateRenderTarget(extent);
    }

    ~FontWindow()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
            m_renderTarget.reset();
            m_renderer.reset();
            m_allocator.reset();
        }
    }

    void recreateRenderTarget(VkExtent2D extent)
    {
        if (!m_allocator || extent.width == 0 || extent.height == 0)
        {
            return;
        }

        m_renderTarget = std::make_unique<RenderTarget>(RenderTarget::CreateInfo
        {
            .device = m_device,
            .allocator = m_allocator.get(),
            .queue = m_graphicsQueue,
            .queueFamily = m_graphicsQueueFamilyIndex,
            .format = RenderTargetFormat::UNORM8,
            .extent = extent,
        });

        if (m_renderer)
        {
            m_renderer->resize(extent);
            m_renderer->bindTarget(m_renderTarget->view());
        }
    }

    TextStyle labelStyle(FontHinting hinting = FontHinting::None) const
    {
        return TextStyle
        {
            .color = float32x4(0.55f, 0.65f, 0.78f, 1.0f),
            .pixelHeight = 11.0f,
            .hinting = hinting,
        };
    }

    TextStyle sampleStyle(float pixelHeight, FontHinting hinting = FontHinting::None) const
    {
        return TextStyle
        {
            .color = float32x4(0.92f, 0.94f, 0.98f, 1.0f),
            .pixelHeight = pixelHeight,
            .hinting = hinting,
        };
    }

    void drawAtBaseline(Font font, float x, float baseline_y, std::string_view utf8, const TextStyle& style)
    {
        m_renderer->draw(font, x, baseline_y, utf8, style);
    }

    void drawSizedSampleLine(TextCursor& cursor, float pixelHeight, FontHinting hinting)
    {
        if (!m_hud)
        {
            return;
        }

        const TextStyle tagStyle = labelStyle(hinting);
        const TextStyle textStyle = sampleStyle(pixelHeight, hinting);
        const std::string tag = fmt::format("{:2.0f} px: ", pixelHeight);

        drawAtBaseline(m_hud, cursor.x, cursor.y, tag, tagStyle);
        const float sample_x = cursor.x + m_renderer->textWidth(m_hud, tag, tagStyle);
        drawAtBaseline(m_hud, sample_x, cursor.y, kSampleText, textStyle);
        m_renderer->newline(cursor, m_hud, textStyle);
    }

    void drawHintingCompareLine(TextCursor& cursor, FontHinting hinting, std::string_view label)
    {
        if (!m_hud)
        {
            return;
        }

        // Labels use a fixed style; sample text uses the same pixel height for every mode.
        const TextStyle tagStyle = labelStyle(FontHinting::None);
        const TextStyle textStyle = sampleStyle(kCompareSize, hinting);

        drawAtBaseline(m_hud, cursor.x, cursor.y, label, tagStyle);
        const float sample_x = cursor.x + m_renderer->textWidth(m_hud, label, tagStyle);
        drawAtBaseline(m_hud, sample_x, cursor.y, kSampleText, textStyle);

        const TextStyle spacingStyle { .pixelHeight = kCompareSize };
        m_renderer->newline(cursor, m_hud, spacingStyle);
    }

    void buildHintingTests()
    {
        if (!m_hud)
        {
            return;
        }

        TextStyle headerStyle = sampleStyle(16.0f, FontHinting::None);
        headerStyle.color = float32x4(0.7f, 0.82f, 0.95f, 1.0f);

        TextCursor test = m_renderer->cursorTopLeft(m_hud, kTestLeft, kTestTop, headerStyle);
        m_renderer->drawLine(test, m_hud, "Hinting test font: NotoSans-Regular.ttf", headerStyle);

        for (float size : kTestSizes)
        {
            drawSizedSampleLine(test, size, FontHinting::None);
        }

        for (const HintingCompareRow& row : kHintingCompareRows)
        {
            drawHintingCompareLine(test, row.hinting, row.label);
        }

        buildLigatureTests();
    }

    void buildLigatureTests()
    {
        if (!m_hud)
        {
            return;
        }

        TextStyle headerStyle = sampleStyle(14.0f, FontHinting::None);
        headerStyle.color = float32x4(0.7f, 0.82f, 0.95f, 1.0f);

        TextCursor test = m_renderer->cursorTopLeft(m_hud, kLigatureLeft, kTestTop, headerStyle);
        m_renderer->drawLine(test, m_hud, "Ligatures: NotoSans-Regular.ttf", headerStyle);

        const TextStyle textStyle = sampleStyle(18.0f, FontHinting::None);
        drawAtBaseline(m_hud, test.x, test.y, kLigatureText, textStyle);
    }

    void buildText()
    {
        m_renderer->beginFrame();

        TextStyle hudStyle { .color = float32x4(0.75f, 0.9f, 1.0f, 1.0f), .pixelHeight = 16.0f };
        TextCursor hud = m_renderer->cursorTopLeft(m_hud, 8.0f, kHudTop, hudStyle);
        m_renderer->draw(hud, m_hud, m_hudLine, hudStyle);

        buildHintingTests();

        // Animated body: smooth fractional scale (hinted modes snap to integer ppem).
        TextStyle bodyStyle
        {
            .color = float32x4(1.0f, 1.0f, 1.0f, 1.0f),
            .pixelHeight = m_fontPixelHeight,
            .hinting = FontHinting::None, // smooth fractional scale, do not use hinting
        };
        TextCursor body = m_renderer->cursorTopLeft(m_body, 40.0f, kBodyTop, bodyStyle);

        for (const std::string& line : m_lines)
        {
            m_renderer->drawLine(body, m_body, line, bodyStyle);
        }
    }

    void recordCommandBuffer(VkCommandBuffer cmd, u32 imageIndex, u32 frameIndex, VkExtent2D extent)
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

        m_renderTarget->clear(cmd, float32x4(0.1f, 0.14f, 0.23f, 1.0f));

        const u64 encode_begin = Time::us();
        m_renderer->encode(cmd,
        {
            .imageView = m_renderTarget->view(),
            .extent = extent,
            .frameIndex = frameIndex,
        });
        m_encodeTimeMs = float(Time::us() - encode_begin) / 1000.0f;

        m_renderTarget->resolve(cmd, swapchain(), imageIndex);

        vkEndCommandBuffer(cmd);
    }

    void render()
    {
        auto frame = beginDraw();
        if (!frame || !m_renderTarget || !(*m_renderTarget))
        {
            return;
        }

        const VkExtent2D extent = swapchainExtent();
        const u32 frameIndex = swapchain().frameIndex();
        VkCommandBuffer cmd = commandBuffer(frame.imageIndex());
        recordCommandBuffer(cmd, frame.imageIndex(), frameIndex, extent);
        frame.submitAndPresent(m_graphicsQueue, cmd);
    }

    void onFrame(const FrameInfo& info) override
    {
        constexpr float body_min_size = 16.0f;
        constexpr float body_max_size = 128.0f;
        constexpr float cycle_seconds = 18.0f;

        const float phase = float(std::fmod(info.time, double(cycle_seconds))) / cycle_seconds;
        const float t = 0.5f + 0.5f * std::sin(phase * float(2.0 * 3.14159265358979323846) - float(3.14159265358979323846) * 0.5f);
        m_fontPixelHeight = body_min_size + t * (body_max_size - body_min_size);
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
    std::string bodyFontPath = "data/fonts/GreatVibes-Regular.ttf";

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
