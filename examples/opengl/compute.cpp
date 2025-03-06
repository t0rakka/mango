/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#define MANGO_IMPLEMENT_MAIN
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::image;

namespace
{

const char* vs_render = R"(
    #version 430 core

    layout (location = 0) in vec2 aPosition;
    layout (location = 1) in vec2 aTexcoord;

    out vec2 texcoord;

    void main()
    {
        texcoord = aTexcoord;
        gl_Position = vec4(aPosition, 0.0, 1.0);
    }
)";

const char* fs_render = R"(
    #version 430 core

    uniform sampler2D uTexture;

    in vec2 texcoord;
    out vec4 FragColor;

    void main()
    {
        FragColor = texture(uTexture, texcoord);
    }
)";

} // namespace

GLuint createComputeProgram()
{
    const char* source = R"(
        #version 430 core

        uniform float time;
        layout(rgba8, binding = 0) uniform image2D uTexture;

        layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

        void main()
        {
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy) * 8 + ivec2(x, y);
                    float width = 512;

                    float s = mod(float(texelCoord.x), width) / (gl_NumWorkGroups.x * 8 + x);
                    float t = float(texelCoord.y) / (gl_NumWorkGroups.y * 8 + y);

                    vec4 color = vec4(0.5 + cos(time / 100.0) * s + sin(time / 100.0) * t,
                                    0.5 + sin(time / 100.0) * s - cos(time / 100.0) * t,
                                    sin(time / 40.0) * 0.5 + 0.5,
                                    1.0);
                    imageStore(uTexture, texelCoord, color);
                }
            }
        }
    )";

    GLuint shader = opengl::createShader(GL_COMPUTE_SHADER, source);
    GLuint program = glCreateProgram();

    glAttachShader(program, shader);
    glLinkProgram(program);

    opengl::getLinkStatus(program);

    glDeleteShader(shader);

    return program;
}

class DemoWindow : public OpenGLContext
{
protected:
    GLuint renderVAO = 0;
    GLuint renderVBO = 0;
    GLuint renderProgram = 0;
    GLuint computeProgram = 0;
    GLuint texture = 0;

public:
    DemoWindow()
        : OpenGLContext(1280, 800)
    {
        setTitle("OpenGL Compute Shader");

        int version = getVersion();
        if (version < 430)
        {
            printLine("OpenGL 4.3 required (you have: {}.{})", version / 100, (version % 100) / 10);
            return;
        }

        static const float vertices [] =
        {
            // position  texcoord
            -1.0f,-1.0f,  0.0f, 0.0f,
             1.0f,-1.0f,  1.0f, 0.0f,
             1.0f, 1.0f,  1.0f, 1.0f,
            -1.0f, 1.0f,  0.0f, 1.0f,
        };

        glGenVertexArrays(1, &renderVAO);
        glGenBuffers(1, &renderVBO);
        glBindVertexArray(renderVAO);
        glBindBuffer(GL_ARRAY_BUFFER, renderVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindImageTexture(0, texture, 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA8);

        computeProgram = createComputeProgram();
        if (!computeProgram)
        {
            printLine("createComputeProgram() failed.");
            return;
        }

        glUseProgram(computeProgram);
        glUniform1i(glGetUniformLocation(computeProgram, "uTexture"), 0);

        renderProgram = opengl::createProgram(vs_render, fs_render);
        if (!renderProgram)
        {
            printLine("createProgram() failed.");
            return;
        }

        glUseProgram(renderProgram);
        glUniform1i(glGetUniformLocation(renderProgram, "uTexture"), 0);
    }

    ~DemoWindow()
    {
        if (renderVAO)
        {
            glDeleteVertexArrays(1, &renderVAO);
        }

        if (renderVBO)
        {
            glDeleteBuffers(1, &renderVBO);
        }

        if (renderProgram)
        {
            glDeleteProgram(renderProgram);
        }

        if (computeProgram)
        {
            glDeleteProgram(computeProgram);
        }

        if (texture)
        {
            glDeleteTextures(1, &texture);
        }
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

    void onIdle() override
    {
        onDraw();
    }

    void onDraw() override
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float time = mango::Time::ms() * 0.1f;

        glUseProgram(computeProgram);
        glUniform1f(glGetUniformLocation(computeProgram, "time"), time);
        glDispatchCompute(512 / 8, 512 / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glUseProgram(renderProgram);

        glBindVertexArray(renderVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);

        swapBuffers();
    }
};

int mangoMain(const mango::CommandLine& commands)
{
    DemoWindow window;
    window.enterEventLoop();
    return 0;
}
