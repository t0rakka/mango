/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;
using namespace mango::math;
using namespace mango::image;

/*
    WARNING!

    This code is for TESTING purposes only; it does only support baseline 8x8 MCU JPEGs.
    The decoder is still WIP; it will be more feature-complete after the Compute Huffman decoder is done.

*/

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

class DemoWindow : public OpenGLContext
{
protected:
    GLuint renderVAO = 0;
    GLuint renderVBO = 0;
    GLuint renderProgram = 0;
    GLuint texture = 0;

public:
    DemoWindow()
        : OpenGLContext(1280, 800)
    {
        setTitle("OpenGL Compute Shader");

        int version = getVersion();
        if (version < 430)
        {
            printf("OpenGL 4.3 required (you have: %d.%d)\n", version / 100, (version % 100) / 10);
            return;
        }

        static const float vertices[] =
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

        debugPrintEnable(true);

        OpenGLJPEGDecoder decoder;
        filesystem::File file("../image/conquer.jpg");
        texture = decoder.decode(file);

        glBindImageTexture(0, texture, 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA8);

        renderProgram = opengl::createProgram(vs_render, fs_render);
        if (!renderProgram)
        {
            printf("createProgram() failed.\n");
            return;
        }

        glUseProgram(renderProgram);
        glUniform1i(glGetUniformLocation(renderProgram, "uTexture"), 0);

        enterEventLoop();
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

        glUseProgram(renderProgram);

        glBindVertexArray(renderVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);

        swapBuffers();
    }
};

int main()
{
    DemoWindow window;
}
