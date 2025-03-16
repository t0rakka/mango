/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#define MANGO_IMPLEMENT_MAIN
#include <mango/mango.hpp>
#include <mango/opengl/framebuffer.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::image;

class TestWindow : public OpenGLFramebuffer
{
protected:
    const Bitmap& m_bitmap;

public:
    TestWindow(const Bitmap& bitmap)
        : OpenGLFramebuffer(bitmap.width, bitmap.height)
        , m_bitmap(bitmap)
    {
        setTitle("Bicubic");

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

        default:
            break;
        }
    }

    void onIdle() override
    {
        onDraw();
    }

    void onDraw() override
    {
        Surface s = lock();
        render(s);
        unlock();
        present();
    }

    void render(Surface s)
    {
        u64 time0 = mango::Time::us();

        float t = sin(time0 / 1000000.0f) * 0.5f + 0.5f;

        float width = (m_width - 1) * t + 1.0f;
        float height = (m_height - 1) * t + 1.0f;
        float x = (m_width - width) * 0.5f;
        float y = (m_height - height) * 0.5f;

        u32_bicubic_blit(s, m_bitmap, x + 0.5, y + 0.5f, width - 1.0f, height - 1.0f);

        u64 time1 = mango::Time::us();
        u64 time = time1 - time0;
        std::string title = fmt::format("time: {}.{} ms", time / 1000, time % 1000);
        setTitle(title);
    }
};

int mangoMain(const mango::CommandLine& commands)
{
    std::string filename = "data/tech_helmet_2005.jpg";

    if (commands.size() == 2)
    {
        filename = commands[1];
    }

    Bitmap bitmap(filename, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

    TestWindow window(bitmap);
    window.enterEventLoop();

    return 0;
}
