/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;

static
const char* vertex_shader_source = R"(
    #version 410

    layout(location = 0) in vec2 a_Position;

    void main()
    {
        gl_Position = vec4(a_Position, 0.0, 1.0);
    }
)";

static
const char* fragment_shader_source = R"(
    #version 410

    layout(location = 0) out vec4 fragColor0;

    void main()
    {
        vec3 color = vec3(0.8, 1.0, 0.8) * 16.0f;
        fragColor0 = vec4(color, 1.0);
    }
)";

class TestWindow : public OpenGLContext
{
protected:
    GLuint m_program = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;

public:
    TestWindow(const OpenGLContext::Config& config, bool sRGB)
        : OpenGLContext(1280, 800, 0, &config)
    {
        if (sRGB)
        {
            glEnable(GL_FRAMEBUFFER_SRGB);
        }

        m_program = opengl::createProgram(vertex_shader_source, fragment_shader_source);

        const float positions [] =
        {
             0.0f,  0.9f,
            -0.7f, -0.9f,
             0.7f, -0.9f,
        };

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, reinterpret_cast<const void*>(0));
    }

    ~TestWindow()
    {
        glDeleteProgram(m_program);
        glDeleteBuffers(1, &m_vbo);
        glDeleteVertexArrays(1, &m_vao);
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
        glClearColor(0.1f, 0.14f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_program);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        swapBuffers();
    }
};

int custom_main(int argc, const char* argv[])
{
    OpenGLContext::Config config;

    bool sRGB = false;

    if (argc == 2)
    {
        std::string_view p1 = argv[1];

        if (p1 == "--hdr")
        {
            config.red = 16;
            config.green = 16;
            config.blue = 16;
            config.alpha = 16;
        }
        else if (p1 == "--srgb")
        {
            sRGB = true;
        }
    }

    printEnable(Print::Info, true);

    TestWindow window(config, sRGB);
    window.enterEventLoop();

    return 0;
}

#ifdef _WIN32
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
        const char* argv[] =
        {
            "",
        };
        custom_main(sizeof(argv), argv);
    }
#else
    int main(int argc, const char* argv[])
    {
        custom_main(argc, argv);
    }
#endif
