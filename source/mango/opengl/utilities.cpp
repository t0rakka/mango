/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
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

            printLine(Print::Error, "{}", buffer.data());
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

            printLine(Print::Error, "{}", buffer.data());
        }

        return success;
    }

    GLuint createShader(GLenum type, std::string_view source)
    {
        GLuint shader = glCreateShader(type);

        const GLchar* data = source.data();
        GLint length = GLint(source.length());

        glShaderSource(shader, 1, &data, &length);
        glCompileShader(shader);
        getCompileStatus(shader);

        return shader;
    }

    GLuint createShader(GLenum type, const std::vector<std::string_view>& sources)
    {
        GLuint shader = glCreateShader(type);

        std::vector<const GLchar*> strings;
        std::vector<GLint> lengths;

        for (auto view : sources)
        {
            strings.emplace_back(view.data());
            lengths.emplace_back(GLint(view.length()));
        }

        glShaderSource(shader, GLsizei(sources.size()), strings.data(), lengths.data());
        glCompileShader(shader);
        getCompileStatus(shader);

        return shader;
    }

    GLuint createProgram(std::string_view vertexShaderSource, std::string_view fragmentShaderSource)
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

    GLuint createProgram(const std::vector<std::string_view>& vertexShaderSources, const std::vector<std::string_view>& fragmentShaderSources)
    {
        GLuint vertex = createShader(GL_VERTEX_SHADER, vertexShaderSources);
        GLuint fragment = createShader(GL_FRAGMENT_SHADER, fragmentShaderSources);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);

        getLinkStatus(program);

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        return program;
    }

    GLuint createProgram(filesystem::Path& path, const std::string& vertexShaderFilename, const std::string& fragmentShaderFilename)
    {
        filesystem::File vertexShaderFile(path, vertexShaderFilename);
        filesystem::File fragmentShaderFile(path, fragmentShaderFilename);

        std::string_view vertexShaderSource(reinterpret_cast<const char*>(vertexShaderFile.data()), vertexShaderFile.size());
        std::string_view fragmentShaderSource(reinterpret_cast<const char*>(fragmentShaderFile.data()), fragmentShaderFile.size());

        return createProgram(vertexShaderSource, fragmentShaderSource);
    }

    GLuint createProgram(filesystem::Path& path, const std::vector<std::string>& vertexShaderFilenames, const std::vector<std::string>& fragmentShaderFilenames)
    {
        struct SourceInfo
        {
            size_t offset;
            u64 size;
        };

        std::vector<SourceInfo> vertexShaderInfo;
        std::vector<SourceInfo> fragmentShaderInfo;

        // load shader sources into shared buffer and store where the files were loaded

        Buffer buffer;

        for (const std::string& filename : vertexShaderFilenames)
        {
            filesystem::File file(path, filename);

            SourceInfo info { buffer.size(), file.size() };
            vertexShaderInfo.push_back(info);

            buffer.append(file);
        }

        for (const std::string& filename : fragmentShaderFilenames)
        {
            filesystem::File file(path, filename);

            SourceInfo info { buffer.size(), file.size() };
            fragmentShaderInfo.push_back(info);

            buffer.append(file);
        }

        // resolve buffer into string views

        std::vector<std::string_view> vertexShaderSources;
        std::vector<std::string_view> fragmentShaderSources;

        const char* data = reinterpret_cast<const char*>(buffer.data());

        for (const SourceInfo& info : vertexShaderInfo)
        {
            vertexShaderSources.emplace_back(data + info.offset, info.size);
        }

        for (const SourceInfo& info : fragmentShaderInfo)
        {
            fragmentShaderSources.emplace_back(data + info.offset, info.size);
        }

        return createProgram(vertexShaderSources, fragmentShaderSources);
    }

} // namespace mango::opengl

#endif // MANGO_OPENGL_CONTEXT_NONE
