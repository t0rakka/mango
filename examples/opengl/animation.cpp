/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#define MANGO_IMPLEMENT_MAIN
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;
using namespace mango::image;
using namespace mango::filesystem;

struct ImageAnimation
{
    File m_file;
    ImageDecoder m_decoder;
    ImageHeader m_header;
    Bitmap m_bitmap;

    int m_frame = 0;  // index of the frame currently in m_bitmap
    u64 m_delay = 0;  // how long the current frame should be shown, in milliseconds

    ImageAnimation(const std::string& filename)
        : m_file(filename)
        , m_decoder(m_file, filename)
        , m_header(m_decoder.header())
        , m_bitmap(m_header.width, m_header.height, m_header.format)
    {
    }

    // ImageHeader::frames: 0 means not animated; a count of 1 is a degenerate
    // animation (a single still image) and aliases with 0.
    bool isAnimation() const
    {
        return m_header.frames > 1;
    }

    // Decode the next frame into m_bitmap and remember its display duration.
    void decodeFrame()
    {
        ImageDecodeStatus status = m_decoder.decode(m_bitmap);
        m_delay = (1000 * status.frame_delay_numerator) / status.frame_delay_denominator;

        if (isAnimation())
        {
            m_frame = (m_frame + 1) % m_header.frames;
        }
    }
};

class DemoWindow : public OpenGLFramebuffer
{
protected:
    ImageAnimation& m_animation;
    bool m_presented = false;

public:
    DemoWindow(ImageAnimation& animation)
        : OpenGLFramebuffer(animation.m_bitmap.width, animation.m_bitmap.height)
        , m_animation(animation)
    {
        std::string name = filesystem::removePath(m_animation.m_file.filename());

        if (m_animation.isAnimation())
        {
            setTitle(fmt::format("[ {} ]  {} frames", name, m_animation.m_header.frames));
        }
        else
        {
            setTitle(fmt::format("[ {} ]", name));
        }
    }

    ~DemoWindow()
    {
    }

    void onKeyPress(Keycode key, u32 mask) override
    {
        if (key == KEYCODE_ESC)
            breakEventLoop();
    }

    void onFrame(const FrameInfo& info) override
    {
        MANGO_UNREFERENCED(info);

        // A still image only needs to be decoded and presented once; an animation
        // decodes a new frame every time and paces itself with the frame delay.
        if (!m_animation.isAnimation() && m_presented)
            return;

        Surface s = lock();

        m_animation.decodeFrame();

        if (m_animation.m_delay > 0)
        {
            setMaxFrameRate(1000.0 / double(m_animation.m_delay));
        }

        s.blit(0, 0, m_animation.m_bitmap);

        unlock();
        present();

        m_presented = true;
    }
};

int mangoMain(const mango::CommandLine& commands)
{
    std::string filename = "data/dude.gif"; // default filename

    if (commands.size() < 2)
    {
        printLine("Too few arguments. Usage: {} <filename>", commands[0]);
        printLine("We play the default animation for your convenience.");
    }
    else
    {
        // overide from command line
        filename = commands[1];
    }

    ImageAnimation animation(filename);
    DemoWindow demo(animation);

    EventLoopConfig config;
    config.trackDisplayRefreshRate = false;

    demo.enterEventLoop(config);

    return 0;
}
