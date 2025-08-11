/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/image/image.hpp>
#include <mango/opengl/context.hpp>

#ifdef MANGO_OPENGL_FRAMEBUFFER

namespace mango
{

    // -------------------------------------------------------------------
    // OpenGLFramebuffer
    // -------------------------------------------------------------------

    class OpenGLFramebuffer : public OpenGLContext
    {
    protected:
        int m_width;
        int m_height;
        image::Format m_format;
        size_t m_stride;

        bool m_is_rgba;
        bool m_is_palette;

        GLuint m_framebuffer = 0;
        GLuint m_index_texture = 0;
        GLuint m_index_program = 0;
        GLint m_index_position = -1;
        GLint m_index_texcoord = -1;

        GLuint m_texture = 0;
        GLuint m_buffer = 0;

        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_ibo = 0;

        struct Program
        {
            GLuint program = 0;
            GLint transform = -1;
            GLint texture = -1;
            GLint scale = -1;
            GLint position = -1;
            GLint texcoord = -1;
        };

        Program m_bilinear;
        Program m_bicubic;

    public:
        enum BufferMode
        {
            RGBA_DIRECT,
            BGRA_DIRECT,
            RGBA_PALETTE,
            BGRA_PALETTE,
        };

        enum Filter
        {
            FILTER_NEAREST,
            FILTER_BILINEAR,
            FILTER_BICUBIC,
        };

        OpenGLFramebuffer(int width, int height, BufferMode buffermode = RGBA_DIRECT);
        ~OpenGLFramebuffer();

        void setPalette(const image::Palette& palette);

        image::Surface lock();
        void unlock();
        void present(Filter filter = FILTER_NEAREST);
    };

} // namespace mango

#endif // MANGO_OPENGL_FRAMEBUFFER
