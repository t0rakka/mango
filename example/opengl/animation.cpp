/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
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
        printf("current: %d, next: %d (%d ms)\n", 
            status.current_frame_index, status.next_frame_index, int(m_delay));
    }
};

class DemoWindow : public OpenGLFramebuffer
{
protected:
    ImageAnimation& m_animation;
    Timer timer;
    u64 prev_time;

public:
    DemoWindow(ImageAnimation& animation)
        : OpenGLFramebuffer(animation.m_bitmap.width, animation.m_bitmap.height)
        , m_animation(animation)
    {
        setVisible(true);
        setTitle("[DemoWindow]");
        prev_time = timer.us();
    }

    ~DemoWindow()
    {
    }

    void onKeyPress(Keycode key, u32 mask) override
    {
        if (key == KEYCODE_ESC)
            breakEventLoop();
    }

    void onIdle() override
    {
        onDraw();
    }

    void onDraw() override
    {
        u64 time = timer.ms();
        u64 diff = time - prev_time;
        if (diff > m_animation.m_delay)
        {
            prev_time = time;

            Surface s = lock();

            m_animation.decode();
            s.blit(0, 0, m_animation.m_bitmap);

            unlock();
            present();
        }
    }
};

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Too few arguments. Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    std::string filename = argv[1];

    ImageAnimation animation(filename);
    DemoWindow demo(animation);
    demo.enterEventLoop();
}
