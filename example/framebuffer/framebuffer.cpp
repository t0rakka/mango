/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <mango/framebuffer/framebuffer.hpp>

using namespace mango;
using namespace mango::framebuffer;

class DemoWindow : public Framebuffer
{
protected:
    u64 prev_time;
    u64 frames = 0;

public:
    DemoWindow(int width, int height)
        : Framebuffer(width, height)
    {
        setVisible(true);
        setTitle("[DemoWindow] Initializing...");
        prev_time = Time::us();
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

    void updateTitle()
    {
        u64 time = Time::us();
        u64 diff = time - prev_time;
        ++frames;
        if (diff > 1000000 / 4)
        {
            diff = diff / frames;
            frames = 0;
            prev_time = time;
            std::string text = makeString("[DemoWindow]  time: %.2f ms (%d Hz)", diff / 1000.0f, diff ? 1000000 / diff : 0);
            setTitle(text);
        }
    }

    void render(const Surface& s)
    {
        s.clear(0xff808080);

        Surface canvas(s, 120, 120, 640 - 240, 480 - 240);
        canvas.clear(0xff80c080);

        // NOTE: this should be premade
        Bitmap red(100, 100, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
        red.clear(0xff0000ff);

        u64 time = Time::ms();
        int x = sin(time * 0.003f) * 50.0f + 25;
        int y = cos(time * 0.003f) * 50.0f + 25;
        canvas.blit(x, y, red);
    }

    void onDraw() override
    {
        updateTitle();

        Surface s = lock();

        render(s);

        // draw debugging rectangles (should be from the left: red, green, blue, white)
        Surface(s,  0, 0, 32, 32).clear(1.0f, 0.0f, 0.0f, 1.0);
        Surface(s, 32, 0, 32, 32).clear(0.0f, 1.0f, 0.0f, 1.0);
        Surface(s, 64, 0, 32, 32).clear(0.0f, 0.0f, 1.0f, 1.0);
        Surface(s, 96, 0, 32, 32).clear(1.0f, 1.0f, 1.0f, 1.0);

        unlock();
        present();
    }
};

int main(int argc, const char* argv[])
{
    DemoWindow demo(640, 480);
    demo.enterEventLoop();
}
