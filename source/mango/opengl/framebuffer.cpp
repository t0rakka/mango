/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <mango/opengl/opengl.hpp>

#ifdef MANGO_OPENGL_FRAMEBUFFER

namespace
{

    // -------------------------------------------------------------------
    // built-in shader sources
    // -------------------------------------------------------------------

    const char* vertex_shader_source = R"(
        #version 330

        uniform vec4 uTransform = vec4(0.0, 0.0, 1.0, 1.0);

        in vec2 inPosition;
        out vec2 texcoord;

        void main()
        {
            texcoord = inPosition * vec2(0.5, -0.5) + vec2(0.5);
            gl_Position = vec4((inPosition + uTransform.xy) * uTransform.zw, 0.0, 1.0);
        }
    )";

    const char* fragment_shader_source = R"(
        #version 330

        uniform sampler2D uTexture;

        in vec2 texcoord;
        out vec4 outFragment0;

        void main()
        {
            outFragment0 = texture(uTexture, texcoord);
        }
    )";

    const char* vertex_shader_source_bicubic = R"(
        #version 330

        uniform vec4 uTransform = vec4(0.0, 0.0, 1.0, 1.0);

        in vec2 inPosition;
        out vec2 texcoord;

        void main()
        {
            texcoord = inPosition * vec2(0.5, -0.5) + vec2(0.5);
            gl_Position = vec4((inPosition + uTransform.xy) * uTransform.zw, 0.0, 1.0);
        }
    )";

    const char* fragment_shader_source_bicubic = R"(
        #version 330

        vec4 cubic(float v)
        {
            vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
            vec4 s = n * n * n;
            float x = s.x;
            float y = s.y - 4.0 * s.x;
            float z = s.z - 4.0 * s.y + 6.0 * s.x;
            float w = 6.0 - x - y - z;
            return vec4(x, y, z, w);
        }

        vec4 texture_filter(sampler2D uTexture, vec2 texcoord, vec2 texscale)
        {
            // hack to bring unit texcoords to integer pixel coords
            texcoord /= texscale;
            texcoord -= vec2(0.5, 0.5f);
            float fx = fract(texcoord.x);
            float fy = fract(texcoord.y);
            texcoord.x -= fx;
            texcoord.y -= fy;
            vec4 cx = cubic(fx);
            vec4 cy = cubic(fy);
            vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y - 0.5, texcoord.y + 1.5);
            vec4 s = vec4(cx.x + cx.y, cx.z + cx.w, cy.x + cy.y, cy.z + cy.w);
            vec4 offset = c + vec4(cx.y, cx.w, cy.y, cy.w) / s;
            vec4 sample0 = texture(uTexture, vec2(offset.x, offset.z) * texscale);
            vec4 sample1 = texture(uTexture, vec2(offset.y, offset.z) * texscale);
            vec4 sample2 = texture(uTexture, vec2(offset.x, offset.w) * texscale);
            vec4 sample3 = texture(uTexture, vec2(offset.y, offset.w) * texscale);
            float sx = s.x / (s.x + s.y);
            float sy = s.z / (s.z + s.w);
            return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
        }

        uniform sampler2D uTexture;
        uniform vec2 uTexScale;

        in vec2 texcoord;
        out vec4 outFragment0;

        void main()
        {
            outFragment0 = texture_filter(uTexture, texcoord, uTexScale);
        }
    )";

    const char* fragment_shader_source_index = R"(
        #version 330

        uniform isampler2D uTexture;
        uniform uint uPalette[256];

        in vec2 texcoord;
        out vec4 outFragment0;

        void main()
        {
            uint color = uPalette[texture(uTexture, texcoord).r];
            float r = ((color << 24) >> 24) / 255.0;
            float g = ((color << 16) >> 24) / 255.0;
            float b = ((color <<  8) >> 24) / 255.0;
            float a = ((color <<  0) >> 24) / 255.0;
            outFragment0 = vec4(r, g, b, a);
        }
    )";

} // namespace

namespace mango
{

    using namespace mango::opengl;
    using namespace mango::image;
    using namespace mango::math;

    // -------------------------------------------------------------------
    // OpenGLFramebuffer
    // -------------------------------------------------------------------

    OpenGLFramebuffer::OpenGLFramebuffer(int width, int height, BufferMode buffermode)
        : OpenGLContext(width, height, 0, nullptr)
        , m_width(width)
        , m_height(height)
    {

        switch (buffermode)
        {
            case RGBA_DIRECT:
                m_format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                m_is_rgba = true;
                m_is_palette = false;
                break;

            case BGRA_DIRECT:
                m_format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                m_is_rgba = false;
                m_is_palette = false;
                break;

            case RGBA_PALETTE:
                m_format = IndexedFormat(8);
                m_is_rgba = true;
                m_is_palette = true;
                break;

            case BGRA_PALETTE:
                m_format = IndexedFormat(8);
                m_is_rgba = false;
                m_is_palette = true;
                break;
        }

        m_stride = size_t(m_width) * m_format.bytes();

        // create texture

        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // create framebuffer

        if (m_is_palette)
        {
            glGenTextures(1, &m_index_texture);
            glBindTexture(GL_TEXTURE_2D, m_index_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glGenFramebuffers(1, &m_framebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);

            m_index_program = createProgram(vertex_shader_source, fragment_shader_source_index);
        }

        // create pixelbuffer

        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_buffer);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, m_stride * height, nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        // create vertex buffers

        const GLfloat vertex_buffer_data[] =
        {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };

        const GLushort element_buffer_data[] =
        {
            0, 1, 2, 3
        };

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

        glGenBuffers(1, &m_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element_buffer_data), element_buffer_data, GL_STATIC_DRAW);

        // create bilinear program

        m_bilinear.program = createProgram(vertex_shader_source, fragment_shader_source);
        m_bilinear.transform = glGetUniformLocation(m_bilinear.program, "uTransform");
        m_bilinear.texture = glGetUniformLocation(m_bilinear.program, "uTexture");
        m_bilinear.position = glGetAttribLocation(m_bilinear.program, "inPosition");

        // create bicubic program

        m_bicubic.program = createProgram(vertex_shader_source_bicubic, fragment_shader_source_bicubic);
        m_bicubic.transform = glGetUniformLocation(m_bicubic.program, "uTransform");
        m_bicubic.texture = glGetUniformLocation(m_bicubic.program, "uTexture");
        m_bicubic.scale = glGetUniformLocation(m_bicubic.program, "uTexScale");
        m_bicubic.position = glGetAttribLocation(m_bicubic.program, "inPosition");
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        if (m_index_program)
        {
            glDeleteProgram(m_index_program);
        }

        if (m_bilinear.program)
        {
            glDeleteProgram(m_bilinear.program);
        }

        if (m_bicubic.program)
        {
            glDeleteProgram(m_bicubic.program);
        }

        if (m_ibo)
        {
            glDeleteBuffers(1, &m_ibo);
        }

        if (m_vbo)
        {
            glDeleteBuffers(1, &m_vbo);
        }

        if (m_vao)
        {
            glDeleteBuffers(1, &m_vao);
        }

        if (m_buffer)
        {
            glDeleteBuffers(1, &m_buffer);
        }

        if (m_texture)
        {
            glDeleteTextures(1, &m_texture);
        }

        if (m_index_texture)
        {
            glDeleteTextures(1, &m_index_texture);
        }

        if (m_framebuffer)
        {
            glDeleteFramebuffers(1, &m_framebuffer);
        }
    }

    Surface OpenGLFramebuffer::lock()
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_buffer);
        void* data = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        return Surface(m_width, m_height, m_format, m_stride, data);
    }

    void OpenGLFramebuffer::unlock()
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_buffer);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (m_framebuffer)
        {
            glBindTexture(GL_TEXTURE_2D, m_index_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, m_width, m_height, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, m_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    void OpenGLFramebuffer::setPalette(const u32* palette)
    {
        if (m_is_palette)
        {
            GLint location = glGetUniformLocation(m_index_program, "uPalette");
            glProgramUniform1uiv(m_index_program, location, 256, palette);
        }
    }

    void OpenGLFramebuffer::present(Filter filter)
    {
        glDisable(GL_BLEND);

        // resolve palette

        if (m_framebuffer)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

            glViewport(0, 0, m_width, m_height);
            glScissor(0, 0, m_width, m_height);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_index_texture);

            glUseProgram(m_index_program);

            glBindVertexArray(m_vao);
            glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr);
            glBindVertexArray(0);
        }

        // compute aspect ratio

        int32x2 window = getWindowSize();

        float32x2 aspect;
        aspect.x = float(window.x) / float(m_width);
        aspect.y = float(window.y) / float(m_height);

        if (aspect.x < aspect.y)
        {
            aspect.y = aspect.x / aspect.y;
            aspect.x = 1.0f;
        }
        else
        {
            aspect.x = aspect.y / aspect.x;
            aspect.y = 1.0f;
        }

        float32x2 scale = aspect;
        float32x2 translate(0.0f, 0.0f);

        // render

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, window.x, window.y);
        glScissor(0, 0, window.x, window.y);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);

        glBindVertexArray(m_vao);

        Program p;

        switch (filter)
        {
            case FILTER_NEAREST:
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                p = m_bilinear;
                break;

            case FILTER_BILINEAR:
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                p = m_bilinear;
                break;

            case FILTER_BICUBIC:
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                p = m_bicubic;
                break;
        }

        if (p.program)
        {
            glUseProgram(p.program);

            glUniform1i(p.texture, 0);
            glUniform4f(p.transform, translate.x, -translate.y, scale.x, scale.y);
            glUniform2f(p.scale, 1.0f / m_width, 1.0f / m_height);

            if (p.position != -1)
            {
                glVertexAttribPointer(p.position, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, nullptr);
                glEnableVertexAttribArray(p.position);
            }
        }

        if (m_is_rgba)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R_EXT, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G_EXT, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B_EXT, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A_EXT, GL_ALPHA);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R_EXT, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G_EXT, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B_EXT, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A_EXT, GL_ALPHA);
        }

        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);

        swapBuffers();
    }

} // namespace mango

#endif // MANGO_OPENGL_FRAMEBUFFER
