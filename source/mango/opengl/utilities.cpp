/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <mango/core/core.hpp>
#include <mango/opengl/opengl.hpp>

#ifndef MANGO_OPENGL_CONTEXT_NONE

namespace mango::opengl
{

    GLint getCompileStatus(GLuint shader)
    {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            GLint size;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
            std::vector<GLchar> buffer(size);

            GLsizei length;
            glGetShaderInfoLog(shader, size, &length, buffer.data());
            debugPrint("%s", buffer.data());
        }

        return success;
    }

    GLint getLinkStatus(GLuint program)
    {
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            GLint size;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
            std::vector<GLchar> buffer(size);

            GLsizei length;
            glGetProgramInfoLog(program, size, &length, buffer.data());
            debugPrint("%s", buffer.data());
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

} // namespace mango::opengl

#endif // MANGO_OPENGL_CONTEXT_NONE
