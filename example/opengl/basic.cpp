/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;

static
const char* vertex_shader_source = R"(
    #version 330

    layout(location = 0) in vec2 a_Position;

    void main()
    {
        gl_Position = vec4(a_Position, 0.0, 1.0);
    }
)";

static
const char* fragment_shader_source = R"(
    #version 330

    out vec4 outFragment0;

    void main()
    {
        vec3 color = vec3(1.0, 1.0, 1.0) * 16.0f;
        outFragment0 = vec4(color, 1.0);
    }
)";

class TestWindow : public OpenGLContext
{
protected:
    GLuint m_program = 0;

public:
    TestWindow(const OpenGLContext::Config* config)
        : OpenGLContext(1280, 800, 0, config)
    {
        m_program = opengl::createProgram(vertex_shader_source, fragment_shader_source);
        glLinkProgram(m_program);
        opengl::getLinkStatus(m_program);
    }

    void onKeyPress(Keycode code, u32 mask) override
    {
        switch (code)
        {
        case KEYCODE_ESC:
            breakEventLoop();
            break;

        case KEYCODE_F11:
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
        glClearColor(0.1f, 0.14f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const float positions[] =
        {
            0.0f, 0.9f,
            -0.7f, -0.9f,
            0.7f, -0.9f,
        };

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, positions);
        glEnableVertexAttribArray(0);

        glUseProgram(m_program);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        swapBuffers();
    }
};

int main(int argc, const char* argv[])
{
    OpenGLContext::Config config;

    if (argc == 2)
    {
        if (std::string_view(argv[1]) == "--hdr")
        {
            config.red = 16;
            config.green = 16;
            config.blue = 16;
            config.alpha = 16;
            config.hdr = true;
        }
    }

    TestWindow window(&config);
    window.enterEventLoop();
}
