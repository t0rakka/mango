/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;

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
        setWindowSize(bitmap.width * 2, bitmap.height * 2);
        setTitle("OpenGLFramebuffer");
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

        case KEYCODE_F1:
            m_filter = OpenGLFramebuffer::FILTER_NEAREST;
            onDraw();
            break;

        case KEYCODE_F2:
            m_filter = OpenGLFramebuffer::FILTER_BILINEAR;
            onDraw();
            break;

        case KEYCODE_F3:
            m_filter = OpenGLFramebuffer::FILTER_BICUBIC;
            onDraw();
            break;

        default:
            break;
        }
    }

    void onResize(int width, int height) override
    {
    }

    void onIdle() override
    {
    }

    void onDraw() override
    {
        Surface s = lock();
        s.blit(0, 0, m_bitmap);

        unlock();
        present(m_filter);
    }
};

int main(int argc, const char* argv[])
{
    Bitmap bitmap("hanselun.png", Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
    TestWindow window(bitmap);
    window.enterEventLoop();
}
