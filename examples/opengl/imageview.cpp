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
    const Bitmap& m_bitmap;
    Filter m_filter = OpenGLFramebuffer::FILTER_NEAREST;

public:
    TestWindow(const Bitmap& bitmap)
        : OpenGLFramebuffer(bitmap.width, bitmap.height)
        , m_bitmap(bitmap)
    {
        setTitle("OpenGLFramebuffer");

        // upload image into the framebuffer
        Surface s = lock();
        s.blit(0, 0, m_bitmap);
        unlock();

        int32x2 screen = getScreenSize();
        printLine("screen: {} x {}", screen.x, screen.y);
        printLine("Image: {} x {}", bitmap.width, bitmap.height);
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
};

int mangoMain(const mango::CommandLine& commands)
{
    std::string filename = "data/hanselun.png";
    if (commands.size() == 2)
    {
        filename = commands[1];
    }

    printEnable(Print::Info, true);

    u64 time0 = mango::Time::ms();

    Bitmap bitmap(filename, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

    u64 time1 = mango::Time::ms();
    printLine("decode: {} ms", time1 - time0);

    if (bitmap.width * bitmap.height > 0)
    {
        TestWindow window(bitmap);
        window.enterEventLoop();
    }

    return 0;
}
