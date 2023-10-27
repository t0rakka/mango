/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "context.hpp"

namespace mango::opengl
{

    GLint getCompileStatus(GLuint shader);
    GLint getLinkStatus(GLuint program);
    GLuint createShader(GLenum type, const char* source);
    GLuint createProgram(const char* vertexShaderSource, const char* fragmentShaderSource);

} // namespace mango::opengl
