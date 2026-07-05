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

    bool m_decoded = false; // the still-image case decodes exactly once
    u64 m_target_us = 0;    // absolute deadline for the next frame (drift-free pacing)

public:
    DemoWindow(ImageAnimation& animation)
        : OpenGLFramebuffer(animation.m_bitmap.width, animation.m_bitmap.height)
        , m_animation(animation)
    {
    }

    ~DemoWindow()
    {
    }

    void onContextReady() override
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

    void onKeyPress(Keycode key, u32 mask) override
    {
        if (key == KEYCODE_ESC)
            breakEventLoop();
    }

    void onFrame(const FrameInfo& info) override
    {
        // onFrame is dispatched with a trigger that says *why* it fired. Only a timed
        // wake (our requestFrameIn) should advance the animation; an Invalidate (resize
        // / expose) must re-blit the *current* frame so a burst of redraw requests can't
        // run the animation ahead of its schedule. The very first frame is the exception:
        // it arrives as an Invalidate but must decode and start the clock.
        const bool firstFrame = !m_decoded;
        const bool timedWake = info.trigger == FrameTrigger::Timed;
        const bool advance = m_animation.isAnimation() ? (firstFrame || timedWake)
                                                       : firstFrame;

        Surface s = lock();

        if (advance)
        {
            m_animation.decodeFrame();
            m_decoded = true;
        }

        s.blit(0, 0, m_animation.m_bitmap);

        unlock();
        present();

        if (m_animation.isAnimation())
        {
            // Advance the target on an absolute timeline only when we actually showed a
            // new frame; resize/expose keeps the existing schedule intact.
            if (advance)
            {
                if (firstFrame)
                {
                    m_target_us = info.time_us;
                }

                m_target_us += m_animation.m_delay * 1000; // milliseconds -> microseconds
            }

            const u64 now = Time::us();
            if (m_target_us < now)
            {
                // fell behind (e.g. window drag stalled the loop): resync instead of
                // bursting through a backlog of frames
                m_target_us = now;
            }

            // Re-arm the wake. The deadline is consumed on every dispatch, so even an
            // invalidate-driven frame must re-arm to keep the animation running.
            requestFrameIn(double(m_target_us - now) / 1'000'000.0);
        }
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

    // OnDemand: the first frame fires automatically, after which animations re-arm a
    // timed wake via requestFrameIn() and still images simply idle until invalidated.
    EventLoopConfig config;
    config.mode = FrameMode::OnDemand;
    config.trackDisplayRefreshRate = false;

    demo.enterEventLoop(config);

    return 0;
}
