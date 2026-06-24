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
    u64 m_delay = 20;

    ImageAnimation(const std::string& filename)
        : m_file(filename)
        , m_decoder(m_file, filename)
        , m_header(m_decoder.header())
        , m_bitmap(m_header.width, m_header.height, m_header.format)
    {
    }

    void decode()
    {
        ImageDecodeStatus status = m_decoder.decode(m_bitmap);
        m_delay = (1000 * status.frame_delay_numerator) / status.frame_delay_denominator;
        printLine("current: {}, next: {} ({} ms)", 
            status.current_frame_index, status.next_frame_index, m_delay);
    }
};

class DemoWindow : public OpenGLFramebuffer
{
protected:
    ImageAnimation& m_animation;

public:
    DemoWindow(ImageAnimation& animation)
        : OpenGLFramebuffer(animation.m_bitmap.width, animation.m_bitmap.height)
        , m_animation(animation)
    {
        setTitle(fmt::format("[ {} ]", filesystem::removePath(m_animation.m_file.filename())));
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

        Surface s = lock();

        m_animation.decode();

        if (m_animation.m_delay > 0)
        {
            setMaxFrameRate(1000.0 / double(m_animation.m_delay));
        }

        s.blit(0, 0, m_animation.m_bitmap);

        unlock();
        present();
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
