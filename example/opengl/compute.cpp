/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
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

GLint getCompileStatus(GLuint shader)
{
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        printf("%s", infoLog);
    }

    return success;
}

GLint getLinkStatus(GLuint program)
{
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        printf("%s", infoLog);
    }

    return success;
}

GLuint createShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    getCompileStatus(shader);

    return shader;
}

GLuint createProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    GLuint vertex = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragment = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    getLinkStatus(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

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

    GLuint compute = createShader(GL_COMPUTE_SHADER, source);

    GLuint program = glCreateProgram();

    glAttachShader(program, compute);
    glLinkProgram(program);

    getLinkStatus(program);

    glDeleteShader(compute);

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

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindImageTexture(0, texture, 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA8);

        computeProgram = createComputeProgram();
        if (!computeProgram)
        {
            printf("createComputeProgram() failed.\n");
            return;
        }

        glUseProgram(computeProgram);
        glUniform1i(glGetUniformLocation(computeProgram, "uTexture"), 0);

        renderProgram = createProgram(vs_render, fs_render);
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

int main()
{
    DemoWindow window;
}
