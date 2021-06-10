/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;
using namespace mango::image;

class DemoWindow : public OpenGLFramebuffer
{
protected:
    Timer timer;
    u64 prev_time;
    u64 frames = 0;
    float color = 0;

public:
    DemoWindow(int width, int height)
        : OpenGLFramebuffer(width, height)
    {
        setVisible(true);
        setTitle("[DemoWindow] Initializing...");
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
        u64 time = timer.us();
        u64 diff = time - prev_time;
        ++frames;
        if (diff > 1000000 / 4)
        {
            diff = diff / frames;
            frames = 0;
            prev_time = time;
            std::string text = makeString("[Mandelbrot]  time: %.2f ms (%d fps)", diff / 1000.0f, diff ? 1000000 / diff : 0);
            setTitle(text);
        }

        Surface s = lock();
        mandelbrot(s);
        unlock();
        present();
    }

    int compute(double x, double y) const
    {
        double x0 = x;
        double y0 = y;
        int n = 0;
        const int nmax = 255;
        while (x * x + y * y <= 4.0 && n < nmax)
        {
            double temp = x * x - y * y + x0;
            y = 2.0 * x * y + y0;
            x = temp;
            ++n;
        }
        return n;
    }

    void mandelbrot(Surface s)
    {
        int width = s.width;
        int height = s.height;

        double px = -0.156653458;
        double py = 1.039128122;

        static double scale = 4.0;
        static double angle = 0.0f;
        scale *= 0.993f;
        angle -= 0.003f;

        double ax = sin(angle) * scale;
        double ay = cos(angle) * scale;
        double bx = cos(angle) * scale;
        double by =-sin(angle) * scale;

        double u0 = px - ax * 0.5 - ay * 0.5;
        double v0 = py - bx * 0.5 - by * 0.5;
        double dxdu = ax / width;
        double dxdv = ay / width;
        double dydu = bx / height;
        double dydv = by / height;

        ConcurrentQueue q;

        for (int y = 0; y < height; ++y)
        {
            q.enqueue([this,&s,width,y,u0,v0,dxdu,dxdv,dydu,dydv]
            {
                u32* scan = s.address<u32>(0, y);

                for (int x = 0; x < width; ++x)
                {
                    double u = u0 + dxdu * x + dydu * y;
                    double v = v0 + dxdv * x + dydv * y;
                    int n = compute(u, v);
                    n = 255 -n;
                    scan[x] = makeRGBA(n & 0x0f, n, n & 0xf0, 0xff);
                }
            });
        }

        q.wait();
    }
};

int main(int argc, const char* argv[])
{
    DemoWindow demo(640, 640);
    demo.enterEventLoop();
}
