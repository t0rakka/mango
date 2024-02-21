/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string_view>
#include <mango/filesystem/filesystem.hpp>
#include <mango/opengl/context.hpp>

#ifndef MANGO_OPENGL_CONTEXT_NONE

namespace mango::opengl
{

    GLint getCompileStatus(GLuint shader);
    GLint getLinkStatus(GLuint program);
    GLuint createShader(GLenum type, std::string_view source);
    GLuint createShader(GLenum type, const std::vector<std::string_view>& sources);
    GLuint createProgram(std::string_view vertexShaderSource, std::string_view fragmentShaderSource);
    GLuint createProgram(const std::vector<std::string_view>& vertexShaderSources, const std::vector<std::string_view>& fragmentShaderSources);
    GLuint createProgram(filesystem::Path& path, const std::string& vertexShaderFilename, const std::string& fragmentShaderFilename);
    GLuint createProgram(filesystem::Path& path, const std::vector<std::string>& vertexShaderFilenames, const std::vector<std::string>& fragmentShaderFilenames);

} // namespace mango::opengl

#endif // MANGO_OPENGL_CONTEXT_NONE
