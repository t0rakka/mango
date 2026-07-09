/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#define MANGO_IMPLEMENT_MAIN
#include <mango/mango.hpp>
#include <mango/vulkan/framebuffer.hpp>

#include <cmath>
#include <vector>

using namespace mango;
using namespace mango::image;
using namespace mango::math;
using namespace mango::vulkan;

struct Wave
{
    float amplitude;
    float frequency;
    float ampRate;
    float freqRate;
    float phase;
};

static
float waveHeight(int x, double time, const Wave& wave)
{
    const float amp = wave.amplitude * (0.55f + 0.45f * std::sin(float(time) * wave.ampRate + wave.phase));
    const float freq = wave.frequency * (0.65f + 0.35f * std::cos(float(time) * wave.freqRate));
    return amp * std::sin(x * freq + wave.phase + float(time) * 1.7f);
}

static
float32x4 lerpColor(float32x4 a, float32x4 b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

static
void phosphorGreen(float intensity, float& r, float& g, float& b)
{
    r = intensity * (0.18f + 0.10f * intensity);
    g = intensity * (1.12f + 0.12f * intensity);
    b = intensity * 0.22f;
}

static
void hsvTint(float& r, float& g, float& b, float shift)
{
    const float lum = std::max(r, std::max(g, b));
    phosphorGreen(lum * (1.0f + shift * 0.12f), r, g, b);
}

class WobbleWindow : public VulkanFramebuffer
{
protected:
    static constexpr int kGridX = 40;
    static constexpr int kGridY = 32;

    static constexpr Wave kWaves[] =
    {
        { 72.0f, 0.028f, 0.91f, 1.13f, 0.00f },
        { 38.0f, 0.051f, 1.37f, 0.84f, 1.20f },
        { 22.0f, 0.089f, 1.05f, 1.46f, 2.40f },
        { 14.0f, 0.137f, 1.62f, 0.73f, 3.80f },
    };

    int m_fbWidth = 0;
    int m_fbHeight = 0;
    std::vector<float> m_phosphorR;
    std::vector<float> m_phosphorG;
    std::vector<float> m_phosphorB;

    u32 m_frames = 0;
    double m_elapsed = 0.0;

    void ensurePhosphor(int width, int height)
    {
        const size_t pixels = size_t(width) * size_t(height);
        if (width != m_fbWidth || height != m_fbHeight)
        {
            m_fbWidth = width;
            m_fbHeight = height;
            m_phosphorR.assign(pixels, 0.0f);
            m_phosphorG.assign(pixels, 0.0f);
            m_phosphorB.assign(pixels, 0.0f);
        }
    }

    void decayPhosphor(float retention)
    {
        ConcurrentQueue q;
        const size_t count = m_phosphorR.size();
        constexpr size_t chunk = 64 * 1024;

        for (size_t offset = 0; offset < count; offset += chunk)
        {
            const size_t end = std::min(offset + chunk, count);
            q.enqueue([&, offset, end]
            {
                for (size_t i = offset; i < end; ++i)
                {
                    m_phosphorR[i] *= retention;
                    m_phosphorG[i] *= retention;
                    m_phosphorB[i] *= retention;
                }
            });
        }

        q.wait();
    }

    void stampPhosphor(const std::vector<float>& curve, double time)
    {
        const int width = m_fbWidth;
        const int height = m_fbHeight;

        const float sweep = float(std::fmod(time * 120.0, double(width + 64)) - 32.0);

        for (int x = 0; x < width; ++x)
        {
            const int cx = x;
            const int cy = int(curve[x]);

            const float sweepGlow = std::exp(-std::abs(float(x) - sweep) * 0.035f) * 0.42f;
            const float pulse = 0.80f + 0.20f * std::sin(float(time) * 3.1f + x * 0.04f);

            for (int dy = -10; dy <= 10; ++dy)
            {
                const int py = cy + dy;
                if (py < 0 || py >= height)
                {
                    continue;
                }

                const float dist = std::abs(float(dy));
                float intensity = std::exp(-dist * dist * 0.10f) * pulse;
                intensity += sweepGlow * std::exp(-dist * dist * 0.05f);

                if (intensity < 0.01f)
                {
                    continue;
                }

                const float shimmer = 0.94f + 0.06f * std::sin(float(time) * 1.3f + x * 0.02f);
                float tr, tg, tb;
                phosphorGreen(intensity * shimmer, tr, tg, tb);

                const size_t i = size_t(py) * size_t(width) + size_t(cx);
                m_phosphorR[i] = std::min(4.0f, m_phosphorR[i] + tr);
                m_phosphorG[i] = std::min(4.0f, m_phosphorG[i] + tg);
                m_phosphorB[i] = std::min(4.0f, m_phosphorB[i] + tb);
            }
        }
    }

    float32x4 backgroundColor(int x, int y, float waveY, double time) const
    {
        MANGO_UNREFERENCED(x);
        const float tint = 0.05f * std::sin(float(time) * 0.45f);

        const float32x4 solidBelow(2.0f / 255.0f, 10.0f / 255.0f, 6.0f / 255.0f, 1.0f);
        const float32x4 gradTop(6.0f / 255.0f, 22.0f / 255.0f, 18.0f / 255.0f, 1.0f);
        const float32x4 gradMid(16.0f / 255.0f, 72.0f / 255.0f, 32.0f / 255.0f, 1.0f);
        const float32x4 gradWave(72.0f / 255.0f, 210.0f / 255.0f, 88.0f / 255.0f, 1.0f);

        if (float(y) < waveY)
        {
            const float t = float(y) / std::max(waveY, 1.0f);
            float32x4 c = lerpColor(gradTop, gradMid, std::min(t * 1.3f, 1.0f));
            c = lerpColor(c, gradWave, std::max(0.0f, (t - 0.45f) / 0.55f));

            if (t > 0.72f)
            {
                const float32x4 warm(140.0f / 255.0f, 220.0f / 255.0f, 70.0f / 255.0f, 1.0f);
                c = lerpColor(c, warm, (t - 0.72f) / 0.28f * 0.40f);
            }

            float r = c.x;
            float g = c.y;
            float b = c.z;
            hsvTint(r, g, b, tint);
            return float32x4(r, g, b, 1.0f);
        }

        return solidBelow;
    }

    float32x4 applyCrt(int x, int y, float32x4 color, double time) const
    {
        float r = color.x;
        float g = color.y;
        float b = color.z;

        const bool majorX = (x % kGridX) == 0;
        const bool majorY = (y % kGridY) == 0;
        const bool minorX = (x % (kGridX / 2)) == 0;
        const bool minorY = (y % (kGridY / 2)) == 0;

        float grid = 0.0f;
        if (majorX || majorY)
        {
            grid = 0.14f;
        }
        else if (minorX || minorY)
        {
            grid = 0.06f;
        }

        const float gridPulse = 0.5f + 0.5f * std::sin(float(time) * 2.0f + x * 0.02f + y * 0.03f);
        r += grid * 0.06f * gridPulse;
        g += grid * (0.26f + 0.14f * gridPulse);
        b += grid * (0.10f + 0.08f * gridPulse);

        if (y & 1)
        {
            r *= 0.93f;
            g *= 0.93f;
            b *= 0.93f;
        }

        const float w = float(m_fbWidth);
        const float h = float(m_fbHeight);
        const float dx = (float(x) - w * 0.5f) / (w * 0.5f);
        const float dy = (float(y) - h * 0.5f) / (h * 0.5f);
        const float vig = 1.0f - 0.32f * (dx * dx + dy * dy);
        r *= vig;
        g *= vig;
        b *= vig;

        const float corner = std::exp(-float(x) * 0.004f - float(y) * 0.006f);
        r += corner * 0.03f;
        g += corner * 0.09f;
        b += corner * 0.04f;

        return float32x4(std::max(r, 0.0f), std::max(g, 0.0f), std::max(b, 0.0f), 1.0f);
    }

public:
    WobbleWindow(VkInstance instance, int width, int height)
        : VulkanFramebuffer(instance, width, height, RGBA_FLOAT)
    {
        setExposure(1.35f);
    }

    void onKeyPress(Keycode key, u32 mask) override
    {
        MANGO_UNREFERENCED(mask);

        if (key == KEYCODE_ESC)
        {
            breakEventLoop();
        }
        else if (key == KEYCODE_F)
        {
            toggleFullscreen();
        }
    }

    void onFrame(const FrameInfo& info) override
    {
        ++m_frames;
        m_elapsed += info.dt;

        if (m_elapsed > 0.25)
        {
            const double avg_dt = m_elapsed / m_frames;
            const int fps = int(0.5 + 1.0 / avg_dt);
            setTitle(fmt::format("[wobble] {:.2f} ms ({} fps)", avg_dt * 1000.0, fps));
            m_frames = 0;
            m_elapsed = 0.0;
        }

        Surface s = lock();
        render(s, info.time, float(info.dt));
        unlock();
        present(VulkanFramebuffer::FILTER_NEAREST);
    }

    void render(Surface s, double time, float dt)
    {
        const int width = s.width;
        const int height = s.height;
        const float center = float(height) * 0.5f;

        ensurePhosphor(width, height);

        std::vector<float> curve(width);
        for (int x = 0; x < width; ++x)
        {
            float y = center;
            for (const Wave& wave : kWaves)
            {
                y += waveHeight(x, time, wave);
            }
            curve[x] = y;
        }

        const float retention = std::pow(0.86f, dt * 60.0f);
        decayPhosphor(retention);
        stampPhosphor(curve, time);

        ConcurrentQueue q;

        const int block = 8;

        for (int y0 = 0; y0 < height; y0 += block)
        {
            q.enqueue([&, y0]
            {
                const int y1 = std::min(y0 + block, height);
                for (int y = y0; y < y1; ++y)
                {
                    float32x4* scan = s.address<float32x4>(0, y);

                    for (int x = 0; x < width; ++x)
                    {
                        const float waveY = curve[x];
                        float32x4 color = backgroundColor(x, y, waveY, time);

                        const size_t i = size_t(y) * size_t(width) + size_t(x);
                        color.x += m_phosphorR[i] * 0.82f;
                        color.y += m_phosphorG[i] * 0.82f;
                        color.z += m_phosphorB[i] * 0.82f;

                        const float dy = std::abs(float(y) - waveY);
                        if (dy < 2.5f)
                        {
                            const float core = std::exp(-dy * dy * 0.85f);
                            float cr, cg, cb;
                            phosphorGreen(core, cr, cg, cb);
                            color.x += cr * 1.4f;
                            color.y += cg * 1.4f;
                            color.z += cb * 1.4f;
                        }

                        scan[x] = applyCrt(x, y, color, time);
                    }
                }
            });
        }

        q.wait();
    }
};

int mangoMain(const mango::CommandLine& commands)
{
    MANGO_UNREFERENCED(commands);

    std::vector<const char*> enabledLayers;
    std::vector<const char*> enabledExtensions = requiredSurfaceExtensions();

    InstanceExtensionProperties instanceExtensionProperties;
    if (instanceExtensionProperties.contains(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME))
    {
        enabledExtensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    }

    VkApplicationInfo applicationInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "wobble",
        .applicationVersion = 1,
        .pEngineName = "mango",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    Instance instance(applicationInfo, enabledLayers, enabledExtensions);
    WobbleWindow window(instance, 360, 200);

    EventLoopConfig config;
    config.mode = FrameMode::Continuous;
    config.trackDisplayRefreshRate = true;
    config.waitForFrame = true;

    window.enterEventLoop(config);

    return 0;
}
