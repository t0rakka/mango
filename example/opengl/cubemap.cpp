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

const char* mesh_vs =
R"(
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

const char* mesh_fs =
R"(
    #version 330 core

    uniform sampler2D texture0;

    in vec2 TexCoord;
    out vec4 FragColor;

    void main()
    {    
        FragColor = texture(texture0, TexCoord);
    }
)";

const char* skybox_vs =
R"(
    #version 330 core

    uniform mat4 view;
    uniform mat4 projection;

    layout (location = 0) in vec3 aPosition;

    out vec3 TexCoord;

    void main()
    {
        TexCoord = aPosition;
        vec4 position = projection * view * vec4(aPosition, 1.0);
        gl_Position = position.xyww;
    }
)";

const char* skybox_fs =
R"(
    #version 330 core

    uniform samplerCube skybox;

    in vec3 TexCoord;
    out vec4 FragColor;

    void main()
    {
        FragColor = texture(skybox, TexCoord);
    }
)";

float cubeVertices [] =
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

float skyboxVertices [] =
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

GLuint createProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    GLint success;
    GLchar infoLog[1024];

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, sizeof(infoLog), NULL, infoLog);
        printf("%s", infoLog);
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, sizeof(infoLog), NULL, infoLog);
        printf("%s", infoLog);
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        printf("%s", infoLog);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

static inline
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const Matrix4x4& value)
{
    glUniformMatrix4fv(location, count, transpose, value[0].data());
}

GLuint createTexture(const std::string& filename)
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
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    return texture;
}

GLuint createCubeTexture(const std::string& filename)
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
    GLuint cubeVAO = 0;
    GLuint cubeVBO = 0;
    GLuint skyboxVAO = 0;
    GLuint skyboxVBO = 0;
    GLuint cubeTexture = 0;
    GLuint cubemapTexture = 0;

    GLuint shader;
    GLuint skyboxShader;

public:
    DemoWindow()
        : OpenGLContext(1280, 800)
    {
        // cube VAO
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        // skybox VAO
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        cubeTexture = createTexture("data/hanselun.png");
        cubemapTexture = createCubeTexture("data/KernerEnvCube.exr");

        shader = createProgram(mesh_vs, mesh_fs);
        skyboxShader = createProgram(skybox_vs, skybox_fs);

        glUseProgram(shader);
        glUniform1i(glGetUniformLocation(shader, "texture0"), 0);

        glUseProgram(skyboxShader);
        glUniform1i(glGetUniformLocation(skyboxShader, "skybox"), 0);
    }

    ~DemoWindow()
    {
        glDeleteVertexArrays(1, &cubeVAO);
        glDeleteVertexArrays(1, &skyboxVAO);
        glDeleteBuffers(1, &cubeVBO);
        glDeleteBuffers(1, &skyboxVBO);
        glDeleteTextures(1, &cubeTexture);
        glDeleteTextures(1, &cubemapTexture);
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

        glUseProgram(shader);

        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, model);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, projection);

        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // skybox

        glDepthFunc(GL_LEQUAL);

        glUseProgram(skyboxShader);

        view[3] = float32x4(0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
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
