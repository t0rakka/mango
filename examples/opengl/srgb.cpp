/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#define MANGO_IMPLEMENT_MAIN
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::image;

class TestWindow : public OpenGLFramebuffer
{
protected:
    Bitmap m_bitmap;
    Filter m_filter = OpenGLFramebuffer::FILTER_NEAREST;
    bool m_srgb = false;

public:
    TestWindow()
        : OpenGLFramebuffer(256, 128)
        , m_bitmap(256, 128, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8))
    {
        updateTitle();

        // compute window size
        int32x2 screen = getScreenSize();
        int scale = std::max(1, (screen.y / std::max(1, m_bitmap.height)) / 2);
        setWindowSize(m_bitmap.width * scale, m_bitmap.height * scale);

        // generate test pattern
        gradient(m_bitmap);

        // upload image into the framebuffer
        Surface s = lock();
        s.blit(0, 0, m_bitmap);
        unlock();

        printLine("screen: {} x {} (scale: {}x)\n", screen.x, screen.y, scale);
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        switch (code)
        {
        case KEYCODE_ESC:
            breakEventLoop();
            break;

        case KEYCODE_F:
            toggleFullscreen();
            break;

        case KEYCODE_1:
            m_filter = OpenGLFramebuffer::FILTER_NEAREST;
            onDraw();
            break;

        case KEYCODE_2:
            m_filter = OpenGLFramebuffer::FILTER_BILINEAR;
            onDraw();
            break;

        case KEYCODE_3:
            m_filter = OpenGLFramebuffer::FILTER_BICUBIC;
            onDraw();
            break;

        case KEYCODE_S:
            m_srgb = !m_srgb;
            updateTitle();
            if (m_srgb)
                glEnable(GL_FRAMEBUFFER_SRGB);
            else
                glDisable(GL_FRAMEBUFFER_SRGB);
            onDraw();
            break;

        default:
            break;
        }
    }

    void onIdle() override
    {
    }

    void onDraw() override
    {
        present(m_filter);
    }

    void updateTitle()
    {
        if (m_srgb)
            setTitle("[OpenGLFramebuffer] top: LINEAR, sRGB: ENABLE");
        else
            setTitle("[OpenGLFramebuffer] top: LINEAR, sRGB: DISABLE");
    }

    void gradient(const Surface& surface)
    {
        for (int y = 0; y < surface.height; ++y)
        {
            u32* dest = surface.address<u32>(0, y);
            for (int x = 0; x < surface.width; ++x)
            {
                u8 s = x;
                if (y > surface.height / 2)
                {
                    float linear = s / 255.0f;
                    float nonlinear = linear_to_srgb(linear);
                    s = u8(nonlinear * 255.0f);
                }
                dest[x] = Color(s, s, s, 0xff);
            }
        }
    }

};

int mangoMain(const mango::CommandLine& commands)
{
    TestWindow window;
    window.enterEventLoop();
    return 0;
}
