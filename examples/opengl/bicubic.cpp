/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
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
    }

    void onContextReady() override
    {
        setTitle("Bicubic");

        int32x2 screen = getScreenSize();
        printLine("screen: {} x {}", screen.x, screen.y);
        printLine("Image: {} x {}", m_bitmap.width, m_bitmap.height);
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        switch (code)
        {
        case KEYCODE_ESC:
            requestQuit();
            break;

        case KEYCODE_F:
            toggleFullscreen();
            break;

        default:
            break;
        }
    }

    void onFrame(const FrameInfo& info) override
    {
        Surface s = lock();
        render(s, info.time);
        unlock();
        present();
    }

    void render(Surface s, double time)
    {
        u64 time0 = mango::Time::us();

        const float t = std::sin(float(time)) * 0.5f + 0.5f;

        float width = (s.width - 1) * t + 1.0f;
        float height = (s.height - 1) * t + 1.0f;
        float x = (s.width - width) * 0.5f;
        float y = (s.height - height) * 0.5f;

        u32_bicubic_blit(s, m_bitmap, x + 0.5f, y + 0.5f, width - 1.0f, height - 1.0f);

        u64 time1 = mango::Time::us();
        u64 duration = time1 - time0;
        std::string title = fmt::format("time: {}.{} ms", duration / 1000, duration % 1000);
        setTitle(title);
    }
};

int mangoMain(const mango::CommandLine& commands)
{
    struct Args
    {
        std::string filename = "data/tech_helmet_2005.jpg";
    } args;

    commands.usage("[filename]");
    commands.positional([&](std::string_view token)
    {
        args.filename = token;
    });

    if (!commands.parse())
    {
        return 1;
    }

    Bitmap bitmap(args.filename, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

    TestWindow window(bitmap);

    EventLoop loop;
    loop.attach(window);
    loop.run();

    return 0;
}
