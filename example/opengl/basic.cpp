/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;

class TestWindow : public OpenGLContext
{
public:
    TestWindow()
        : OpenGLContext(1280, 800)
    {
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

        default:
            break;
        }
    }

    void onResize(int width, int height) override
    {
        glViewport(0, 0, width, height);
        glScissor(0, 0, width, height);
    }

    void onDraw() override
    {
        glClearColor(0.2, 0.3, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        swapBuffers();
    }
};

int main(int argc, const char* argv[])
{
    TestWindow window;
    window.enterEventLoop();
}
