/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include "context.hpp"

#ifdef MANGO_OPENGL_JPEG

namespace mango
{

    struct OpenGLJPEGDecoder : image::ComputeDecoder
    {
        GLuint program = 0;
        GLuint texture = 0;

        int width = 0;
        int height = 0;

        void decode(s16* data, int xmcu, int ymcu) override;
        void decode(const image::Surface& surface) override;

        OpenGLJPEGDecoder();
        ~OpenGLJPEGDecoder();

        GLuint decode(ConstMemory memory);
    };

} // namespace mango

#endif // MANGO_OPENGL_JPEG
