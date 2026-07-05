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
using namespace mango::vulkan;

struct Wave
{
    float amplitude;
    float frequency;
    float ampRate;
    float freqRate;
    float phase;
    float hue; // trace tint offset
};

static
float waveHeight(int x, double time, const Wave& wave)
{
    const float amp = wave.amplitude * (0.55f + 0.45f * std::sin(float(time) * wave.ampRate + wave.phase));
    const float freq = wave.frequency * (0.65f + 0.35f * std::cos(float(time) * wave.freqRate));
    return amp * std::sin(x * freq + wave.phase + float(time) * 1.7f);
}

static
void unpack(u32 c, float& r, float& g, float& b)
{
    r = float((c >>  0) & 0xff) / 255.0f;
    g = float((c >>  8) & 0xff) / 255.0f;
    b = float((c >> 16) & 0xff) / 255.0f;
}

static
u32 packColor(float r, float g, float b)
{
    r = std::clamp(r, 0.0f, 1.0f);
    g = std::clamp(g, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);
    return makeRGBA(u32(r * 255.0f + 0.5f), u32(g * 255.0f + 0.5f), u32(b * 255.0f + 0.5f), 255);
}

static
u32 lerpColor(u32 a, u32 b, float t)
{
    float ar, ag, ab;
    float br, bg, bb;
    unpack(a, ar, ag, ab);
    unpack(b, br, bg, bb);
    t = std::clamp(t, 0.0f, 1.0f);
    const float inv = 1.0f - t;
    return packColor(ar * inv + br * t, ag * inv + bg * t, ab * inv + bb * t);
}

static
void hsvTint(float& r, float& g, float& b, float shift)
{
    // cheap RGB swirl — good enough for demoscene vibes
    const float nr = r + shift * (g - b);
    const float ng = g + shift * (b - r);
    const float nb = b + shift * (r - g);
    r = nr;
    g = ng;
    b = nb;
}

class WobbleWindow : public VulkanFramebuffer
{
protected:
    static constexpr int kGridX = 40;
    static constexpr int kGridY = 32;

    static constexpr Wave kWaves[] =
    {
        { 72.0f, 0.028f, 0.91f, 1.13f, 0.00f,  0.00f },
        { 38.0f, 0.051f, 1.37f, 0.84f, 1.20f,  0.18f },
        { 22.0f, 0.089f, 1.05f, 1.46f, 2.40f, -0.12f },
        { 14.0f, 0.137f, 1.62f, 0.73f, 3.80f,  0.25f },
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

            const float sweepGlow = std::exp(-std::abs(float(x) - sweep) * 0.035f) * 0.35f;
            const float pulse = 0.75f + 0.25f * std::sin(float(time) * 3.1f + x * 0.04f);

            for (int dy = -10; dy <= 10; ++dy)
            {
                const int py = cy + dy;
                if (py < 0 || py >= height)
                {
                    continue;
                }

                const float dist = std::abs(float(dy));
                float intensity = std::exp(-dist * dist * 0.11f) * pulse;
                intensity += sweepGlow * std::exp(-dist * dist * 0.05f);

                if (intensity < 0.01f)
                {
                    continue;
                }

                float tr = 1.0f;
                float tg = 0.55f + 0.35f * std::sin(float(time) * 0.7f + x * 0.01f);
                float tb = 0.15f;

                // Orange demogroup accent on the hot trace
                tr = 1.0f;
                tg = 0.42f + 0.18f * std::sin(float(time) * 1.3f);
                tb = 0.05f;

                hsvTint(tr, tg, tb, 0.12f * std::sin(float(time) * 0.5f + x * 0.02f));

                const size_t i = size_t(py) * size_t(width) + size_t(cx);
                m_phosphorR[i] = std::min(2.5f, m_phosphorR[i] + tr * intensity);
                m_phosphorG[i] = std::min(2.5f, m_phosphorG[i] + tg * intensity);
                m_phosphorB[i] = std::min(2.5f, m_phosphorB[i] + tb * intensity);
            }
        }

        // ghost harmonics in the phosphor only
        for (const Wave& wave : kWaves)
        {
            const float hue = wave.hue;
            for (int x = 0; x < width; x += 3)
            {
                const int py = int(m_fbHeight * 0.5f + waveHeight(x, time, wave) * 0.55f);
                if (py < 1 || py >= height - 1)
                {
                    continue;
                }

                float tr = 0.15f + hue;
                float tg = 0.35f;
                float tb = 0.9f - hue;
                const size_t i = size_t(py) * size_t(width) + size_t(x);
                m_phosphorR[i] = std::min(1.2f, m_phosphorR[i] + tr * 0.08f);
                m_phosphorG[i] = std::min(1.2f, m_phosphorG[i] + tg * 0.08f);
                m_phosphorB[i] = std::min(1.2f, m_phosphorB[i] + tb * 0.08f);
            }
        }
    }

    u32 backgroundColor(int x, int y, float waveY, double time) const
    {
        const float tint = 0.10f * std::sin(float(time) * 0.45f);

        const u32 solidBelow = makeRGBA(4, 3, 18, 255);
        const u32 gradTop = makeRGBA(22, 2, 42, 255);
        const u32 gradMid = makeRGBA(255, 88, 12, 255);  // Orange
        const u32 gradWave = makeRGBA(0, 220, 255, 255);

        if (float(y) < waveY)
        {
            const float t = float(y) / std::max(waveY, 1.0f);
            u32 c = lerpColor(gradTop, gradMid, std::min(t * 1.6f, 1.0f));
            c = lerpColor(c, gradWave, std::max(0.0f, (t - 0.55f) / 0.45f));

            float r, g, b;
            unpack(c, r, g, b);
            hsvTint(r, g, b, tint);
            return packColor(r, g, b);
        }

        return solidBelow;
    }

    u32 applyCrt(int x, int y, u32 color, double time) const
    {
        float r, g, b;
        unpack(color, r, g, b);

        // grid
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
        r += grid * (0.25f + 0.75f * gridPulse);
        g += grid * (0.45f + 0.40f * gridPulse);
        b += grid * 0.20f;

        // scanlines
        if (y & 1)
        {
            r *= 0.90f;
            g *= 0.90f;
            b *= 0.90f;
        }

        // vignette
        const float w = float(m_fbWidth);
        const float h = float(m_fbHeight);
        const float dx = (float(x) - w * 0.5f) / (w * 0.5f);
        const float dy = (float(y) - h * 0.5f) / (h * 0.5f);
        const float vig = 1.0f - 0.42f * (dx * dx + dy * dy);
        r *= vig;
        g *= vig;
        b *= vig;

        // corner orange glow (demogroup nod)
        const float corner = std::exp(-float(x) * 0.004f - float(y) * 0.006f);
        r += corner * 0.10f;
        g += corner * 0.03f;

        return packColor(r, g, b);
    }

public:
    WobbleWindow(VkInstance instance, int width, int height)
        : VulkanFramebuffer(instance, width, height)
    {
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
        present(VulkanFramebuffer::FILTER_BILINEAR);
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
                    u32* scan = s.address<u32>(0, y);

                    for (int x = 0; x < width; ++x)
                    {
                        const float waveY = curve[x];
                        u32 color = backgroundColor(x, y, waveY, time);
    
                        float r, g, b;
                        unpack(color, r, g, b);
    
                        const size_t i = size_t(y) * size_t(width) + size_t(x);
                        r = std::min(1.0f, r + m_phosphorR[i] * 0.65f);
                        g = std::min(1.0f, g + m_phosphorG[i] * 0.65f);
                        b = std::min(1.0f, b + m_phosphorB[i] * 0.65f);
    
                        // hot core on the trace
                        const float dy = std::abs(float(y) - waveY);
                        if (dy < 2.5f)
                        {
                            const float core = std::exp(-dy * dy * 0.9f);
                            r = std::min(1.0f, r + core * 0.9f);
                            g = std::min(1.0f, g + core * 0.55f);
                            b = std::min(1.0f, b + core * 0.15f);
                        }
    
                        color = packColor(r, g, b);
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
        .pApplicationName = "vkfb_wobble",
        .applicationVersion = 1,
        .pEngineName = "mango",
        .engineVersion = 1,
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    Instance instance(applicationInfo, enabledLayers, enabledExtensions);

    WobbleWindow window(instance, 800, 480);
    window.setTitle("vkfb_wobble — mr.ai/orange");

    EventLoopConfig config;
    config.mode = FrameMode::Continuous;
    config.waitForFrame = true;
    window.enterEventLoop(config);

    return 0;
}
