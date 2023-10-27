/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "opengl.hpp"

namespace mango
{

    struct OpenGLJPEGDecoder
    {
        OpenGLJPEGDecoder();
        ~OpenGLJPEGDecoder();

        GLuint decode(ConstMemory memory);
    };

} // namespace mango
