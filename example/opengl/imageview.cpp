/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
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

        // compute window size
        int32x2 screen = getScreenSize();
        int32x2 window(bitmap.width, bitmap.height);

        if (window.x > screen.x)
        {
            // fit horizontally
            int scale = div_ceil(window.x, screen.x);
            window.x = window.x / scale;
            window.y = window.y / scale;
        }

        if (window.y > screen.y)
        {
            // fit vertically
            int scale = div_ceil(window.y, screen.y);
            window.x = window.x / scale;
            window.y = window.y / scale;
        }

        if (window.y < screen.y)
        {
            // enlarge tiny windows
            int scale = std::max(1, (screen.y / std::max(1, window.y)) / 2);
            window.x *= scale;
            window.y *= scale;
        }

        setWindowSize(window.x, window.y);

        // upload image into the framebuffer
        Surface s = lock();
        s.blit(0, 0, m_bitmap);
        unlock();

        printf("screen: %d x %d\n", screen.x, screen.y);
        printf("Image: %d x %d\n", bitmap.width, bitmap.height);
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

int main(int argc, const char* argv[])
{
    std::string filename = "data/hanselun.png";
    if (argc == 2)
    {
        filename = argv[1];
    }

    debugPrintEnable(true);

    u64 time0 = mango::Time::ms();

    Bitmap bitmap(filename, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

    u64 time1 = mango::Time::ms();
    printf("decode: %d ms\n", u32(time1 - time0));

    TestWindow window(bitmap);
    window.enterEventLoop();
}
