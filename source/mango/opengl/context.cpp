/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/opengl/opengl.hpp>

#ifndef MANGO_OPENGL_CONTEXT_NONE

namespace
{
    using namespace mango;
    using namespace mango::image;

    const OpenGLContext::InternalFormat g_format_table[] =
    {
        // 1.0
        { 0x1903, Format(  8, Format::UNORM, Format::R,    8, 0, 0, 0), "RED" },
        { 0x1904, Format(  8, Format::UNORM, Format::G,    8, 0, 0, 0), "GREEN" },
        { 0x1905, Format(  8, Format::UNORM, Format::B,    8, 0, 0, 0), "BLUE" },
        { 0x1906, Format(  8, Format::UNORM, Format::A,    8, 0, 0, 0), "ALPHA" },
        { 0x1907, Format( 24, Format::UNORM, Format::RGB,  8, 8, 8, 0), "RGB" },
        { 0x1908, Format( 32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), "RGBA" },

        // 1.1
        { 0x2A10, Format(  8, Format::UNORM, Format::RGB,   3,  3,  2,  0), "R3_G3_B2" },
        { 0x804F, Format( 16, Format::UNORM, Format::RGB,   4,  4,  4,  0), "RGB4" },
        { 0x8050, Format( 16, Format::UNORM, Format::RGB,   5,  5,  5,  0), "RGB5" },
        { 0x8051, Format( 24, Format::UNORM, Format::RGB,   8,  8,  8,  0), "RGB8" },
        { 0x8052, Format( 32, Format::UNORM, Format::RGB,  10, 10, 10,  0), "RGB10" },
        { 0x8053, Format( 48, Format::UNORM, Format::RGB,  12, 12, 12,  0), "RGB12" },
        { 0x8054, Format( 48, Format::UNORM, Format::RGB,  16, 16, 16,  0), "RGB16" },
        { 0x8055, Format(  8, Format::UNORM, Format::RGBA,  2,  2,  2,  2), "RGBA2" },
        { 0x8056, Format( 16, Format::UNORM, Format::RGBA,  4,  4,  4,  4), "RGBA4" },
        { 0x8057, Format( 16, Format::UNORM, Format::RGBA,  5,  5,  5,  1), "RGB5_A1" },
        { 0x8058, Format( 32, Format::UNORM, Format::RGBA,  8,  8,  8,  8), "RGBA8" },
        { 0x8059, Format( 32, Format::UNORM, Format::RGBA, 10, 10, 10,  2), "RGB10_A2" },
        { 0x805A, Format( 48, Format::UNORM, Format::RGBA, 12, 12, 12, 12), "RGBA12" },
        { 0x805B, Format( 64, Format::UNORM, Format::RGBA, 16, 16, 16, 16), "RGBA16" },

        // 1.2
        { 0x8032, Format(  8, Format::UNORM, Format::BGR,   2, 3, 3, 0), "UNSIGNED_BYTE_3_3_2" },
        { 0x8033, Format( 16, Format::UNORM, Format::ABGR,  4, 4, 4, 4), "UNSIGNED_SHORT_4_4_4_4" },
        { 0x8034, Format( 16, Format::UNORM, Format::ABGR,  1, 5, 5, 5), "UNSIGNED_SHORT_5_5_5_1" },
        { 0x8035, Format( 32, Format::UNORM, Format::ABGR,  8, 8, 8, 8), "UNSIGNED_INT_8_8_8_8" },
        { 0x8036, Format( 32, Format::UNORM, Format::ABGR,  2, 10, 10, 10), "UNSIGNED_INT_10_10_10_2" },
        { 0x8362, Format(  8, Format::UNORM, Format::RGB,   3, 3, 2, 0), "UNSIGNED_BYTE_2_3_3_REV" },
        { 0x8363, Format( 16, Format::UNORM, Format::BGR,   5, 6, 5, 0), "UNSIGNED_SHORT_5_6_5" },
        { 0x8364, Format( 16, Format::UNORM, Format::RGB,   5, 6, 5, 0), "UNSIGNED_SHORT_5_6_5_REV" },
        { 0x8365, Format( 16, Format::UNORM, Format::RGBA,  4, 4, 4, 4), "UNSIGNED_SHORT_4_4_4_4_REV" },
        { 0x8366, Format( 16, Format::UNORM, Format::RGBA,  5, 5, 5, 1), "UNSIGNED_SHORT_1_5_5_5_REV" },
        { 0x8367, Format( 32, Format::UNORM, Format::RGBA,  8, 8, 8, 8), "UNSIGNED_INT_8_8_8_8_REV" },
        { 0x8368, Format( 32, Format::UNORM, Format::RGBA, 10, 10, 10, 2), "UNSIGNED_INT_2_10_10_10_REV" },
        { 0x80E0, Format( 24, Format::UNORM, Format::BGR,   8, 8, 8, 0), "BGR" },
        { 0x80E1, Format( 32, Format::UNORM, Format::BGRA,  8, 8, 8, 8), "BGRA" },

        // 2.1
        { 0x8C40, Format( 24, Format::UNORM, Format::RGB,   8,  8,  8,  0), "SRGB" },
        { 0x8C41, Format( 24, Format::UNORM, Format::RGB,   8,  8,  8,  0), "SRGB8" },
        { 0x8C42, Format( 32, Format::UNORM, Format::RGBA,  8,  8,  8,  8), "SRGB_ALPHA" },
        { 0x8C43, Format( 32, Format::UNORM, Format::RGBA,  8,  8,  8,  8), "SRGB8_ALPHA8" },

        // 3.0
        { 0x8814, Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32), "RGBA32F" },
        { 0x8815, Format( 96, Format::FLOAT32, Format::RGB,  32, 32, 32,  0), "RGB32F" },
        { 0x881A, Format( 64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16), "RGBA16F" },
        { 0x881B, Format( 48, Format::FLOAT16, Format::RGB,  16, 16, 16,  0), "RGB16F" },
        // NOTE: these don't have blitter support but can be handled as compressed blocks
        /*
        { 0x8C3A, Format(), "R11F_G11F_B10F" },
        { 0x8C3B, Format(), "UNSIGNED_INT_10F_11F_11F_REV" },
        { 0x8C3D, Format(), "RGB9_E5" },
        */
        { 0x8C3E, Format( 32, Format::UNORM, Format::RGBA, 9, 9, 9, 5), "UNSIGNED_INT_5_9_9_9_REV" },
        { 0x8D70, Format(128, Format::UINT,  Format::RGBA, 32, 32, 32, 32), "RGBA32UI" },
        { 0x8D71, Format( 96, Format::UINT,  Format::RGB,  32, 32, 32,  0), "RGB32UI" },
        { 0x8D76, Format( 64, Format::UINT,  Format::RGBA, 16, 16, 16, 16), "RGBA16UI" },
        { 0x8D77, Format( 48, Format::UINT,  Format::RGB,  16, 16, 16,  0), "RGB16UI" },
        { 0x8D7C, Format( 32, Format::UINT,  Format::RGBA,  8,  8,  8,  8), "RGBA8UI" },
        { 0x8D7D, Format( 24, Format::UINT,  Format::RGB,   8,  8,  8,  0), "RGB8UI" },
        { 0x8D82, Format(128, Format::SINT,  Format::RGBA, 32, 32, 32, 32), "RGBA32I" },
        { 0x8D83, Format( 96, Format::SINT,  Format::RGB,  32, 32, 32,  0), "RGB32I" },
        { 0x8D88, Format( 64, Format::SINT,  Format::RGBA, 16, 16, 16, 16), "RGBA16I" },
        { 0x8D89, Format( 48, Format::SINT,  Format::RGB,  16, 16, 16,  0), "RGB16I" },
        { 0x8D8E, Format( 32, Format::SINT,  Format::RGBA,  8,  8,  8,  8), "RGBA8I" },
        { 0x8D8F, Format( 24, Format::SINT,  Format::RGB,   8,  8,  8,  0), "RGB8I" },
        { 0x8227, Format( 16, Format::UNORM, Format::RG,    8,  8,  0,  0), "RG" },
        { 0x8229, Format(  8, Format::UNORM, Format::R,     8,  0,  0,  0), "R8" },
        { 0x822A, Format( 16, Format::UNORM, Format::R,    16,  0,  0,  0), "R16" },
        { 0x822B, Format( 16, Format::UNORM, Format::RG,    8,  8,  0,  0), "RG8" },
        { 0x822C, Format( 32, Format::UNORM, Format::RG,   16, 16,  0,  0), "RG16" },
        { 0x822D, Format( 16, Format::FLOAT16, Format::R,    16,  0,  0,  0), "R16F" },
        { 0x822E, Format( 32, Format::FLOAT32, Format::R,    32,  0,  0,  0), "R32F" },
        { 0x822F, Format( 32, Format::FLOAT16, Format::RG,   16, 16,  0,  0), "RG16F" },
        { 0x8230, Format( 64, Format::FLOAT32, Format::RG,   32, 32,  0,  0), "RG32F" },
        { 0x8231, Format(  8, Format::SINT,  Format::R,     8,  0,  0,  0), "R8I" },
        { 0x8232, Format(  8, Format::UINT,  Format::R,     8,  0,  0,  0), "R8UI" },
        { 0x8233, Format( 16, Format::SINT,  Format::R,    16,  0,  0,  0), "R16I" },
        { 0x8234, Format( 16, Format::UINT,  Format::R,    16,  0,  0,  0), "R16UI" },
        { 0x8235, Format( 32, Format::SINT,  Format::R,    32,  0,  0,  0), "R32I" },
        { 0x8236, Format( 32, Format::UINT,  Format::R,    32,  0,  0,  0), "R32UI" },
        { 0x8237, Format( 16, Format::SINT,  Format::RG,    8,  8,  0,  0), "RG8I" },
        { 0x8238, Format( 16, Format::UINT,  Format::RG,    8,  8,  0,  0), "RG8UI" },
        { 0x8239, Format( 32, Format::SINT,  Format::RG,   16, 16,  0,  0), "RG16I" },
        { 0x823A, Format( 32, Format::UINT,  Format::RG,   16, 16,  0,  0), "RG16UI" },
        { 0x823B, Format( 64, Format::SINT,  Format::RG,   32, 32,  0,  0), "RG32I" },
        { 0x823C, Format( 64, Format::UINT,  Format::RG,   32, 32,  0,  0), "RG32UI" },

        // 3.1
        { 0x8F94, Format(  8, Format::SNORM, Format::R,     8,  0,  0,  0), "R8_SNORM" },
        { 0x8F95, Format( 16, Format::SNORM, Format::RG,    8,  8,  0,  0), "RG8_SNORM" },
        { 0x8F96, Format( 24, Format::SNORM, Format::RGB,   8,  8,  8,  0), "RGB8_SNORM" },
        { 0x8F97, Format( 32, Format::SNORM, Format::RGBA,  8,  8,  8,  8), "RGBA8_SNORM" },
        { 0x8F98, Format( 16, Format::SNORM, Format::R,    16,  0,  0,  0), "R16_SNORM" },
        { 0x8F99, Format( 32, Format::SNORM, Format::RG,   16, 16,  0,  0), "RG16_SNORM" },
        { 0x8F9A, Format( 48, Format::SNORM, Format::RGB,  16, 16, 16,  0), "RGB16_SNORM" },
        { 0x8F9B, Format( 64, Format::SNORM, Format::RGBA, 16, 16, 16, 16), "RGBA16_SNORM" },

        // 3.3
        { 0x906F, Format( 32, Format::UINT,  Format::RGBA, 10, 10, 10,  2), "RGB10_A2UI" },
        { 0x8D9F, Format( 32, Format::UNORM, Format::RGBA, 10, 10, 10,  2), "INT_2_10_10_10_REV" },

        // 4.1
        { 0x8D62, Format( 16, Format::UNORM, Format::RGB,   5,  6,  5,  0), "RGB565" },

        // GL_EXT_texture
        { 0x803B, Format(  8, Format::UNORM,   Format::A,  4, 0, 0, 0), "ALPHA4" },
        { 0x803C, Format(  8, Format::UNORM,   Format::A,  8, 0, 0, 0), "ALPHA8" },
        { 0x803D, Format( 16, Format::UNORM,   Format::A, 12, 0, 0, 0), "ALPHA12" },
        { 0x803D, Format( 16, Format::UNORM,   Format::A, 16, 0, 0, 0), "ALPHA16" },

        // GL_EXT_texture_integer
        { 0x8D90, Format(  8, Format::SINT,    Format::A,  8, 0, 0, 0), "ALPHA8I" },
        { 0x8D7E, Format(  8, Format::UINT,    Format::A,  8, 0, 0, 0), "ALPHA8UI" },
        { 0x8D8A, Format( 16, Format::SINT,    Format::A, 16, 0, 0, 0), "ALPHA16I" },
        { 0x8D78, Format( 16, Format::UINT,    Format::A, 16, 0, 0, 0), "ALPHA16UI" },
        { 0x8D84, Format( 32, Format::SINT,    Format::A, 32, 0, 0, 0), "ALPHA32I" },
        { 0x8D72, Format( 32, Format::UINT,    Format::A, 32, 0, 0, 0), "ALPHA32UI" },

        // GL_{ATI,ARB,APPLE}_texture_float
        { 0x881C, Format( 16, Format::FLOAT16, Format::A, 16, 0, 0, 0), "ALPHA16F" },
        { 0x8816, Format( 32, Format::FLOAT32, Format::A, 32, 0, 0, 0), "ALPHA32F" },
    };

} // namespace

namespace mango
{
    using namespace math;
    using namespace image;

    // -----------------------------------------------------------------------
    // extension masks
    // -----------------------------------------------------------------------

    static
    void init_ext(OpenGLContext& context)
    {
        const char* names[] =
        {
#define GL_EXTENSION(Name) "GL_"#Name,
#include <mango/opengl/func/glext.hpp>
#undef GL_EXTENSION
        };

        u32* mask = reinterpret_cast<u32*>(&context.ext);
        std::memset(mask, 0, sizeof(context.ext));

        for (size_t i = 0; i < std::size(names); ++i)
        {
            if (context.isExtension(names[i]))
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

    static
    void init_core(OpenGLContext& context, int version)
    {
        int table[] =
        {
#define CORE_EXTENSION(Version, Name)  Version,
#include <mango/opengl/func/glcorearb.hpp>
#undef CORE_EXTENSION
        };

        u32* mask = reinterpret_cast<u32*>(&context.core);
        std::memset(mask, 0, sizeof(context.core));

        for (size_t i = 0; i < std::size(table); ++i)
        {
            if (version >= table[i])
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

#if defined(MANGO_OPENGL_CONTEXT_WGL)

    static
    void init_wgl(OpenGLContext& context)
    {
        const char* names[] =
        {
#define WGL_EXTENSION(Name) "WGL_"#Name,
#include <mango/opengl/func/wglext.hpp>
#undef WGL_EXTENSION
        };

        u32* mask = reinterpret_cast<u32*>(&context.wgl);
        std::memset(mask, 0, sizeof(context.wgl));

        for (size_t i = 0; i < std::size(names); ++i)
        {
            if (context.isExtension(names[i]))
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

#endif

#if defined(MANGO_OPENGL_CONTEXT_GLX)

    static
    void init_glx(OpenGLContext& context)
    {
        const char* names[] =
        {
#define GLX_EXTENSION(Name) "GLX_"#Name,
#include <mango/opengl/func/glxext.hpp>
#undef GLX_EXTENSION
        };

        u32* mask = reinterpret_cast<u32*>(&context.glx);
        std::memset(mask, 0, sizeof(context.glx));

        for (size_t i = 0; i < std::size(names); ++i)
        {
            if (context.isExtension(names[i]))
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

#endif

    // -----------------------------------------------------------------------
    // context creation
    // -----------------------------------------------------------------------

    OpenGLContextHandle* createOpenGLContextWGL(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared);
    OpenGLContextHandle* createOpenGLContextCocoa(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared);
    OpenGLContextHandle* createOpenGLContextGLX(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared);
    OpenGLContextHandle* createOpenGLContextEGL(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared);

    using CreateContext = OpenGLContextHandle* (*)(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared);

    static
    OpenGLContextHandle* createOpenGLContext(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        CreateContext create = nullptr;

#if defined(MANGO_OPENGL_CONTEXT_WGL)
        create = createOpenGLContextWGL;
#endif

#if defined(MANGO_OPENGL_CONTEXT_COCOA)
        create = createOpenGLContextCocoa;
#endif

#if defined(MANGO_OPENGL_CONTEXT_GLX)
        create = createOpenGLContextGLX;
#endif

#if defined(MANGO_OPENGL_CONTEXT_EGL)
        if ((flags & OpenGLContext::EGL) || !create)
        {
            create = createOpenGLContextEGL;
        }
#endif

        OpenGLContextHandle* context = nullptr;
        if (create)
        {
            context = create(parent, width, height, flags, configPtr, shared);
        }

        return context;
    }

    // -----------------------------------------------------------------------
    // OpenGLContext
    // -----------------------------------------------------------------------

    OpenGLContext::OpenGLContext(int width, int height, u32 flags, const Config* configPtr, OpenGLContext* shared)
        : Window(width, height, flags | API_OPENGL)
    {
        initContext(width, height, flags, configPtr, shared);
    }

    OpenGLContext::OpenGLContext(math::int32x2 extent, u32 flags, const Config* configPtr, OpenGLContext* shared)
        : Window(extent.x, extent.y, flags | API_OPENGL)
    {
        initContext(extent.x, extent.y, flags, configPtr, shared);
    }

    OpenGLContext::~OpenGLContext()
    {
    }

    void OpenGLContext::initContext(int width, int height, u32 flags, const Config* configPtr, OpenGLContext* shared)
    {
        OpenGLContextHandle* context = createOpenGLContext(this, width, height, flags, configPtr, shared);
        if (!context)
        {
            MANGO_EXCEPTION("[OpenGLContext] context creation failed.");
        }

        m_context.reset(context);
        setVisible(true);

        // initialize version
        const char* str_version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

        int major = 0;
        int minor = 0;

        if (!std::strncmp(str_version, "OpenGL ES", 9))
        {
            m_is_gles = true;
            std::sscanf(str_version, "%*s %*s %d.%d", &major, &minor);
        }
        else
        {
            std::sscanf(str_version, "%d.%d", &major, &minor);
        }

        m_version = major * 100 + minor * 10;

        // query OpenGL extensions
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        if (extensions)
        {
            // parse extension string
            for (const GLubyte* s = extensions; *s; ++s)
            {
                if (*s == ' ')
                {
                    const std::ptrdiff_t length = s - extensions;
                    if (length > 0)
                    {
                        m_extensions.emplace(reinterpret_cast<const char*>(extensions), length);
                    }

                    extensions = s + 1;
                }
            }
        }
        else
        {
            GLint numExtensions = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

            for (GLint i = 0; i < numExtensions; ++i)
            {
                const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
                m_extensions.emplace(extension);
            }
        }

        // initialize extension mask
        initExtensionMask();

        // renderer information
        const GLubyte* s0 = glGetString(GL_VENDOR);
        const GLubyte* s1 = glGetString(GL_RENDERER);
        const GLubyte* s2 = glGetString(GL_VERSION);

        printLine("Vendor:   \"{}\"", reinterpret_cast<const char *>(s0));
        printLine("Renderer: \"{}\"", reinterpret_cast<const char *>(s1));
        printLine("Version:  \"{}\"", reinterpret_cast<const char *>(s2));
    }

    void OpenGLContext::makeCurrent()
    {
        m_context->makeCurrent();
    }

    void OpenGLContext::swapBuffers()
    {
        m_context->swapBuffers();
    }

    void OpenGLContext::swapInterval(int interval)
    {
        m_context->swapInterval(interval);
    }

    void OpenGLContext::toggleFullscreen()
    {
        m_context->toggleFullscreen();
    }

    bool OpenGLContext::isFullscreen() const
    {
        return m_context->isFullscreen();
    }

    math::int32x2 OpenGLContext::getWindowSize() const
    {
        return m_context->getWindowSize();
    }

    void OpenGLContext::initExtensionMask()
    {
        init_ext(*this);
        init_core(*this, m_version);

#if defined(MANGO_OPENGL_CONTEXT_WGL)
        init_wgl(*this);
#endif

#if defined(MANGO_OPENGL_CONTEXT_GLX)
        init_glx(*this);
#endif

        int gl_version = 0;
        int gles_version = 0;

        if (m_is_gles)
        {
            gles_version = m_version;
        }
        else
        {
            gl_version = m_version;
        }

        // detect compressed texture format support using extensions

        core.texture_compression_dxt1 = false;
        core.texture_compression_dxt3 = false;
        core.texture_compression_dxt5 = false;

        if (ext.EXT_texture_compression_s3tc || ext.NV_texture_compression_s3tc)
        {
            core.texture_compression_dxt1 = true;
            core.texture_compression_dxt3 = true;
            core.texture_compression_dxt5 = true;
        }

        if (ext.ANGLE_texture_compression_dxt1 || ext.EXT_texture_compression_dxt1)
        {
            core.texture_compression_dxt1 = true;
        }

        if (ext.ANGLE_texture_compression_dxt3)
        {
            core.texture_compression_dxt1 = true;
            core.texture_compression_dxt3 = true;
        }

        if (ext.ANGLE_texture_compression_dxt5)
        {
            core.texture_compression_dxt1 = true;
            core.texture_compression_dxt5 = true;
        }

        if (gl_version >= 420 || ext.ARB_texture_compression_bptc || ext.EXT_texture_compression_bptc)
        {
            core.texture_compression_bptc = true;
        }

        if (gl_version >= 300 || ext.EXT_texture_compression_rgtc || ext.ARB_texture_compression_rgtc)
        {
            core.texture_compression_rgtc = true;
        }

        if (ext.EXT_texture_rg || ext.ARB_texture_rg)
        {
            core.texture_rg = true;
        }

        if (ext.EXT_texture_snorm)
        {
            core.texture_snorm = true;
        }

        core.texture_compression_latc = ext.NV_texture_compression_latc || ext.EXT_texture_compression_latc;
        core.texture_compression_atc = ext.AMD_compressed_ATC_texture || ext.ATI_texture_compression_atitc;

        core.texture_compression_astc_ldr = ext.KHR_texture_compression_astc_ldr || gles_version >= 320;
        core.texture_compression_astc_hdr = ext.KHR_texture_compression_astc_hdr;

        if (gl_version >= 430 || gles_version >= 300)
        {
            core.texture_compression_etc2 = true;
            core.texture_compression_eac = true;
        }

#if 0
        // detect compressed texture format support using API

        GLint numCompressedFormats = 0;
        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numCompressedFormats);

        if (numCompressedFormats > 0)
        {
            std::vector<GLint> compressedFormats(numCompressedFormats);
            glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compressedFormats.data());

            for (auto format : compressedFormats)
            {
                switch (format)
                {
                    // GL_EXT_texture_compression_s3tc
                    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
                    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
                    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
                    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                        break;

                    // GL_ARB_texture_compression_bptc
                    case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
                    case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
                    case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB:
                    case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB:
                        break;

                    // GL_3DFX_texture_compression_FXT1
                    case GL_COMPRESSED_RGB_FXT1_3DFX:
                    case GL_COMPRESSED_RGBA_FXT1_3DFX:
                        break;

                    // GL_EXT_texture_compression_latc
                    case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
                    case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
                    case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
                    case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
                        break;

                    // GL_EXT_texture_compression_rgtc
                    case GL_COMPRESSED_RED_RGTC1_EXT:
                    case GL_COMPRESSED_SIGNED_RED_RGTC1_EXT:
                    case GL_COMPRESSED_RED_GREEN_RGTC2_EXT:
                    case GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT:
                        break;

                    // GL_IMG_texture_compression_pvrtc
                    case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
                    case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
                    case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
                    case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
                        break;

                    // GL_IMG_texture_compression_pvrtc2
                    case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG:
                    case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG:
                        break;

                    // GL_EXT_texture_compression_s3tc_srgb
                    case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
                    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
                    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
                    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
                        break;
                }
            }
        }
#endif
    }

    bool OpenGLContext::isExtension(const std::string& name) const
    {
        return m_extensions.find(name) != m_extensions.end();
    }

    bool OpenGLContext::isGLES() const
    {
        return m_is_gles;
    }

    int OpenGLContext::getVersion() const
    {
        return m_version;
    }

    bool OpenGLContext::isCompressedTextureSupported(u32 compression) const
    {
        bool supported = false;

        switch (compression)
        {
            case TextureCompression::ATC_RGB:
            case TextureCompression::ATC_RGBA_EXPLICIT_ALPHA:
            case TextureCompression::ATC_RGBA_INTERPOLATED_ALPHA:
                supported = core.texture_compression_atc;
                break;

            case TextureCompression::AMD_3DC_X:
            case TextureCompression::AMD_3DC_XY:
                supported = ext.AMD_compressed_3DC_texture;
                break;

            case TextureCompression::LATC1_LUMINANCE:
            case TextureCompression::LATC1_SIGNED_LUMINANCE:
            case TextureCompression::LATC2_LUMINANCE_ALPHA:
            case TextureCompression::LATC2_SIGNED_LUMINANCE_ALPHA:
                supported = ext.EXT_texture_compression_latc;
                break;

            case TextureCompression::DXT1:
            case TextureCompression::DXT1_ALPHA1:
                supported = core.texture_compression_dxt1;
                break;

            case TextureCompression::DXT3:
                supported = core.texture_compression_dxt3;
                break;

            case TextureCompression::DXT5:
                supported = core.texture_compression_dxt5;
                break;

            case TextureCompression::RGTC1_RED:
            case TextureCompression::RGTC1_SIGNED_RED:
            case TextureCompression::RGTC2_RG:
            case TextureCompression::RGTC2_SIGNED_RG:
                supported = core.texture_compression_rgtc;
                break;

            case TextureCompression::BPTC_RGBA_UNORM:
            case TextureCompression::BPTC_SRGB_ALPHA_UNORM:
            case TextureCompression::BPTC_RGB_SIGNED_FLOAT:
            case TextureCompression::BPTC_RGB_UNSIGNED_FLOAT:
                supported = core.texture_compression_bptc;
                break;

            case TextureCompression::PVRTC_RGB_4BPP:
            case TextureCompression::PVRTC_RGB_2BPP:
            case TextureCompression::PVRTC_RGBA_4BPP:
            case TextureCompression::PVRTC_RGBA_2BPP:
                supported = ext.IMG_texture_compression_pvrtc;
                break;

            case TextureCompression::PVRTC2_RGBA_2BPP:
            case TextureCompression::PVRTC2_RGBA_4BPP:
                supported = ext.IMG_texture_compression_pvrtc2;
                break;

            case TextureCompression::PVRTC_SRGB_2BPP:
            case TextureCompression::PVRTC_SRGB_4BPP:
            case TextureCompression::PVRTC_SRGB_ALPHA_2BPP:
            case TextureCompression::PVRTC_SRGB_ALPHA_4BPP:
                supported = ext.EXT_pvrtc_sRGB;
                break;

            case TextureCompression::EAC_R11:
            case TextureCompression::EAC_SIGNED_R11:
            case TextureCompression::EAC_RG11:
            case TextureCompression::EAC_SIGNED_RG11:
            case TextureCompression::ETC2_RGB:
            case TextureCompression::ETC2_SRGB:
            case TextureCompression::ETC2_RGB_ALPHA1:
            case TextureCompression::ETC2_SRGB_ALPHA1:
            case TextureCompression::ETC2_RGBA:
            case TextureCompression::ETC2_SRGB_ALPHA8:
                supported = core.texture_compression_etc2;
                break;

            case TextureCompression::ETC1_RGB:
                supported = ext.OES_compressed_ETC1_RGB8_texture;
                break;

            case TextureCompression::ASTC_UNORM_4x4:
            case TextureCompression::ASTC_UNORM_5x4:
            case TextureCompression::ASTC_UNORM_5x5:
            case TextureCompression::ASTC_UNORM_6x5:
            case TextureCompression::ASTC_UNORM_6x6:
            case TextureCompression::ASTC_UNORM_8x5:
            case TextureCompression::ASTC_UNORM_8x6:
            case TextureCompression::ASTC_UNORM_8x8:
            case TextureCompression::ASTC_UNORM_10x5:
            case TextureCompression::ASTC_UNORM_10x6:
            case TextureCompression::ASTC_UNORM_10x8:
            case TextureCompression::ASTC_UNORM_10x10:
            case TextureCompression::ASTC_UNORM_12x10:
            case TextureCompression::ASTC_UNORM_12x12:
            case TextureCompression::ASTC_SRGB_4x4:
            case TextureCompression::ASTC_SRGB_5x4:
            case TextureCompression::ASTC_SRGB_5x5:
            case TextureCompression::ASTC_SRGB_6x5:
            case TextureCompression::ASTC_SRGB_6x6:
            case TextureCompression::ASTC_SRGB_8x5:
            case TextureCompression::ASTC_SRGB_8x6:
            case TextureCompression::ASTC_SRGB_8x8:
            case TextureCompression::ASTC_SRGB_10x5:
            case TextureCompression::ASTC_SRGB_10x6:
            case TextureCompression::ASTC_SRGB_10x8:
            case TextureCompression::ASTC_SRGB_10x10:
            case TextureCompression::ASTC_SRGB_12x10:
            case TextureCompression::ASTC_SRGB_12x12:
                supported = core.texture_compression_astc_ldr;
                break;

            case TextureCompression::ASTC_FLOAT_4x4:
            case TextureCompression::ASTC_FLOAT_5x4:
            case TextureCompression::ASTC_FLOAT_5x5:
            case TextureCompression::ASTC_FLOAT_6x5:
            case TextureCompression::ASTC_FLOAT_6x6:
            case TextureCompression::ASTC_FLOAT_8x5:
            case TextureCompression::ASTC_FLOAT_8x6:
            case TextureCompression::ASTC_FLOAT_8x8:
            case TextureCompression::ASTC_FLOAT_10x5:
            case TextureCompression::ASTC_FLOAT_10x6:
            case TextureCompression::ASTC_FLOAT_10x8:
            case TextureCompression::ASTC_FLOAT_10x10:
            case TextureCompression::ASTC_FLOAT_12x10:
            case TextureCompression::ASTC_FLOAT_12x12:
                supported = core.texture_compression_astc_hdr;
                break;

            case TextureCompression::ASTC_UNORM_3x3x3:
            case TextureCompression::ASTC_UNORM_4x3x3:
            case TextureCompression::ASTC_UNORM_4x4x3:
            case TextureCompression::ASTC_UNORM_4x4x4:
            case TextureCompression::ASTC_UNORM_5x4x4:
            case TextureCompression::ASTC_UNORM_5x5x4:
            case TextureCompression::ASTC_UNORM_5x5x5:
            case TextureCompression::ASTC_UNORM_6x5x5:
            case TextureCompression::ASTC_UNORM_6x6x5:
            case TextureCompression::ASTC_UNORM_6x6x6:
            case TextureCompression::ASTC_SRGB_3x3x3:
            case TextureCompression::ASTC_SRGB_4x3x3:
            case TextureCompression::ASTC_SRGB_4x4x3:
            case TextureCompression::ASTC_SRGB_4x4x4:
            case TextureCompression::ASTC_SRGB_5x4x4:
            case TextureCompression::ASTC_SRGB_5x5x4:
            case TextureCompression::ASTC_SRGB_5x5x5:
            case TextureCompression::ASTC_SRGB_6x5x5:
            case TextureCompression::ASTC_SRGB_6x6x5:
            case TextureCompression::ASTC_SRGB_6x6x6:
                supported = ext.OES_texture_compression_astc;
                break;

            default:
                break;
        }

        return supported;
    }

    const OpenGLContext::InternalFormat* OpenGLContext::getInternalFormat(GLenum internalFormat) const
    {
        for (auto& node : g_format_table)
        {
            if (node.internalFormat == internalFormat)
                return &node;
        }

        return nullptr;
    }

} // namespace mango

#endif // MANGO_OPENGL_CONTEXT_NONE
