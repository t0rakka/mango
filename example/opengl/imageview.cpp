/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        int scale = std::max(1, (screen.y / std::max(1, bitmap.height)) / 2);
        setWindowSize(bitmap.width * scale, bitmap.height * scale);

        // upload image into the framebuffer
        Surface s = lock();
        s.blit(0, 0, m_bitmap);
        unlock();

        printf("screen: %d x %d (scale: %dx)\n", screen.x, screen.y, scale);
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
    Bitmap bitmap("data/hanselun.png", Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
    TestWindow window(bitmap);
    window.enterEventLoop();
}
