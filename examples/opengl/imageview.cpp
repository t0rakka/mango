/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        : OpenGLFramebuffer(bitmap.width, bitmap.height, bitmap.palette ? RGBA_PALETTE : RGBA_DIRECT)
        , m_bitmap(bitmap)
    {
    }

    void onContextReady() override
    {
        setTitle("OpenGLFramebuffer");

        // upload image into the framebuffer
        Surface s = lock();
        s.blit(0, 0, m_bitmap);
        unlock();

        if (m_bitmap.palette)
        {
            setPalette(*m_bitmap.palette);
        }

        int32x2 screen = getScreenSize();
        printLine("screen: {} x {}", screen.x, screen.y);
        printLine("Image: {} x {}", m_bitmap.width, m_bitmap.height);
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
            invalidate();
            break;

        case KEYCODE_1:
            m_filter = OpenGLFramebuffer::FILTER_NEAREST;
            invalidate();
            break;

        case KEYCODE_2:
            m_filter = OpenGLFramebuffer::FILTER_BILINEAR;
            invalidate();
            break;

        case KEYCODE_3:
            m_filter = OpenGLFramebuffer::FILTER_BICUBIC;
            invalidate();
            break;

        default:
            break;
        }
    }

    void onFrame(const FrameInfo& info) override
    {
        MANGO_UNREFERENCED(info);
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

    u64 time0 = mango::Time::us();

    // The decoding below is more complicated than necessary because it is also testing indexed images.

    std::unique_ptr<Bitmap> bitmap;

    filesystem::File file(filename);
    ImageDecoder decoder(file, filename);
    if (decoder.isDecoder())
    {
        ImageHeader header = decoder.header();
        if (header.format.isIndexed())
        {
            printLine("Decoding INDEXED image.\n");
        }
        else
        {
            printLine("Decoding RGBA image.");
            printLine("  Format.bits: {}\n", header.format.bits);
        }

        bitmap = std::make_unique<Bitmap>(header);
        ImageDecodeStatus status = decoder.decode(*bitmap, ImageDecodeOptions(), 0, 0, 0);
        MANGO_UNREFERENCED(status);
    }
    else
    {
        printLine("Incorrect decoder.");
        return 1;
    }

    if (bitmap->palette)
    {
        printLine("  Palette size: {}", bitmap->palette->size);
    }

    u64 time1 = mango::Time::us();

    ConstMemory icc = decoder.icc();
    if (icc.size)
    {
        transform(*bitmap, icc);

        u64 time2 = mango::Time::us();
        u64 time = time2 - time1;
        printLine("icc transform: {}.{} ms", time / 1000, time % 1000);
    }

    u64 time = time1 - time0;
    printLine("decode: {}.{} ms", time / 1000, time % 1000);

    if (bitmap->width * bitmap->height > 0)
    {
        TestWindow window(*bitmap);

        EventLoopConfig config;
        config.mode = FrameMode::OnDemand;

        window.enterEventLoop(config);
    }

    return 0;
}
