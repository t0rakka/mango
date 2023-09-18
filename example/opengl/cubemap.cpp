/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <mango/opengl/opengl.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::math;
using namespace mango::image;

namespace
{

const char* vs_mesh = R"(
    #version 330 core

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    layout (location = 0) in vec3 aPosition;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main()
    {
        TexCoord = aTexCoord;
        gl_Position = projection * view * model * vec4(aPosition, 1.0);
    }
)";

const char* fs_mesh = R"(
    #version 330 core

    uniform sampler2D texture0;

    in vec2 TexCoord;
    out vec4 FragColor;

    void main()
    {    
        FragColor = texture(texture0, TexCoord);
    }
)";

const char* vs_skybox = R"(
    #version 330 core

    uniform mat4 view;
    uniform mat4 projection;

    layout (location = 0) in vec3 aPosition;

    out vec3 Normal;

    void main()
    {
        Normal = aPosition;
        vec4 position = projection * view * vec4(aPosition, 1.0);
        gl_Position = position.xyww;
    }
)";

const char* fs_skybox_cubemap = R"(
    #version 330 core

    uniform samplerCube skybox;

    in vec3 Normal;
    out vec4 FragColor;

    void main()
    {
        FragColor = texture(skybox, Normal);
    }
)";

const char* fs_skybox_latlong = R"(
    #version 330 core
    #define PI 3.14159265359

    uniform sampler2D skybox;

    in vec3 Normal;
    out vec4 FragColor;

    vec2 latlong(vec3 direction)
    {
        direction = normalize(direction);
        float s = atan(-direction.z, direction.x) / (2.0 * PI);
        float t = acos(direction.y) / PI;
        return vec2(s + 0.25, t);
    }

    void main()
    {
        FragColor = texture(skybox, latlong(Normal));
    }
)";

const float cubeVertices [] =
{
    // positions          texture coords
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

const float skyboxVertices [] =
{
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

} // namespace

static inline
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const Matrix4x4& value)
{
    glUniformMatrix4fv(location, count, transpose, value[0].data());
}

GLuint createShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        printf("%s", infoLog);
    }

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

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        printf("%s", infoLog);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

GLuint createTexture2D(const std::string& filename, bool mipmap)
{
    GLuint texture = 0;

    File file(filename);

    ImageDecoder decoder(file, filename);
    if (decoder.isDecoder())
    {
        ImageHeader header = decoder.header();

        int width = header.width;
        int height = header.height;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        Bitmap bitmap(width, height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

        ImageDecodeOptions options;
        ImageDecodeStatus status = decoder.decode(bitmap, options, 0, 0, 0); 

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.image);

        if (mipmap)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    return texture;
}

GLuint createTextureCube(const std::string& filename)
{
    GLuint texture = 0;

    File file(filename);

    ImageDecoder decoder(file, filename);
    if (decoder.isDecoder())
    {
        ImageHeader header = decoder.header();
        if (header.faces == 6)
        {
            int width = header.width;
            int height = header.height;

            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

            Bitmap bitmap(width, height, Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16));

            for (int face = 0; face < 6; ++face)
            {
                ImageDecodeOptions options;
                ImageDecodeStatus status = decoder.decode(bitmap, options, 0, 0, face);

                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_HALF_FLOAT, bitmap.image);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
    }

    return texture;
}

class DemoWindow : public OpenGLContext
{
protected:
    GLuint meshVAO = 0;
    GLuint meshVBO = 0;
    GLuint meshTexture = 0;
    GLuint meshProgram = 0;

    GLuint skyboxVAO = 0;
    GLuint skyboxVBO = 0;
    GLuint skyboxTexture = 0;
    GLuint skyboxProgram = 0;

    GLenum skyboxSamplerType;

public:
    DemoWindow()
        : OpenGLContext(1280, 800)
    {
        // mesh
        glGenVertexArrays(1, &meshVAO);
        glGenBuffers(1, &meshVBO);
        glBindVertexArray(meshVAO);
        glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        // skybox
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        meshTexture = createTexture2D("data/hanselun.png", true);
        meshProgram = createProgram(vs_mesh, fs_mesh);

        bool cubemap = false;

        if (cubemap)
        {
            skyboxTexture = createTextureCube("data/KernerEnvCube.exr");
            skyboxProgram = createProgram(vs_skybox, fs_skybox_cubemap);
            skyboxSamplerType = GL_TEXTURE_CUBE_MAP;
        }
        else
        {
            skyboxTexture = createTexture2D("data/KernerEnvLatLong.exr", false);
            skyboxProgram = createProgram(vs_skybox, fs_skybox_latlong);
            skyboxSamplerType = GL_TEXTURE_2D;
        }

        glUseProgram(meshProgram);
        glUniform1i(glGetUniformLocation(meshProgram, "texture0"), 0);

        glUseProgram(skyboxProgram);
        glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);
    }

    ~DemoWindow()
    {
        glDeleteVertexArrays(1, &meshVAO);
        glDeleteVertexArrays(1, &skyboxVAO);
        glDeleteBuffers(1, &meshVBO);
        glDeleteBuffers(1, &skyboxVBO);
        glDeleteTextures(1, &meshTexture);
        glDeleteTextures(1, &skyboxTexture);
        glDeleteProgram(meshProgram);
        glDeleteProgram(skyboxProgram);
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

        glEnable(GL_DEPTH_TEST);

        float xfov = 1.8f;
        float yfov = xfov;

        int32x2 window = getWindowSize();
        if (window.x > window.y)
        {
            float s = float(window.y) / float(window.x);
            yfov *= s;
        }
        else
        {
            float s = float(window.x) / float(window.y);
            xfov *= s;
        }

        float time = mango::Time::us() / 10000000.0f * 3.0f;

        Matrix4x4 model = Matrix4x4::identity();
        Matrix4x4 view = Matrix4x4::rotateXYZ(time, time * 2, -time*0.5) * Matrix4x4::translate(0.0f, 0.0f, -2.0f);
        Matrix4x4 projection = Matrix4x4::perspectiveGL(xfov, yfov, 1.0f, 100.0f);

        // cube

        glUseProgram(meshProgram);

        glUniformMatrix4fv(glGetUniformLocation(meshProgram, "model"), 1, GL_FALSE, model);
        glUniformMatrix4fv(glGetUniformLocation(meshProgram, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(meshProgram, "projection"), 1, GL_FALSE, projection);

        glBindVertexArray(meshVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, meshTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // skybox

        glDepthFunc(GL_LEQUAL);

        glUseProgram(skyboxProgram);

        view[3] = float32x4(0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(skyboxSamplerType, skyboxTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS);

        swapBuffers();
    }
};

int main()
{
    DemoWindow window;
    window.enterEventLoop();
}
