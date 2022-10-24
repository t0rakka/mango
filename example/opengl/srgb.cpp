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

        // generate test pattern
        gradient(bitmap);

        // upload image into the framebuffer
        Surface s = lock();
        s.blit(0, 0, m_bitmap);
        unlock();

        printf("screen: %d x %d (scale: %dx)\n", screen.x, screen.y, scale);

        GLint encoding = -1;
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_FRONT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
        if (encoding == GL_LINEAR)
            printf("Framebuffer attachment color encoding is linear.\n");
        if (encoding == GL_SRGB)
            printf("Framebuffer attachment color encoding is sRGB.\n");
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
                    float nonlinear = srgbEncode(linear);
                    s = u8(nonlinear * 255.0f);
                }
                dest[x] = Color(s, s, s, 0xff);
            }
        }
    }

};

int main(int argc, const char* argv[])
{
    Bitmap bitmap(256, 128, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
    TestWindow window(bitmap);
    window.enterEventLoop();
}
