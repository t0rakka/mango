/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/opengl/context.hpp>

#ifdef MANGO_OPENGL_JPEG

namespace mango
{

    struct OpenGLJPEGDecoder
    {
        GLuint program = 0;

        OpenGLJPEGDecoder();
        ~OpenGLJPEGDecoder();

        GLuint decode(ConstMemory memory);
    };

} // namespace mango

#endif // MANGO_OPENGL_JPEG
