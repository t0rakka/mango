/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        : Window(width, height, flags)
        , m_context(nullptr)
    {
        m_context = createOpenGLContext(this, width, height, flags, configPtr, shared);
        if (!m_context)
        {
            MANGO_EXCEPTION("[OpenGLContext] context creation failed.");
        }

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

        // parse extension string
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        if (extensions)
        {
            // parse extensions
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

        // initialize extension mask
        initExtensionMask();

        // renderer information
        const GLubyte* s0 = glGetString(GL_VENDOR);
        const GLubyte* s1 = glGetString(GL_RENDERER);
        const GLubyte* s2 = glGetString(GL_VERSION);

        debugPrint("Vendor:   \"%s\"\n", reinterpret_cast<const char *>(s0));
        debugPrint("Renderer: \"%s\"\n", reinterpret_cast<const char *>(s1));
        debugPrint("Version:  \"%s\"\n", reinterpret_cast<const char *>(s2));
    }

    OpenGLContext::~OpenGLContext()
    {
        delete m_context;
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

        if (ext.EXT_texture_compression_rgtc || ext.ARB_texture_compression_rgtc)
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
        core.texture_compression_astc = ext.KHR_texture_compression_astc_hdr || ext.KHR_texture_compression_astc_ldr;

        if (m_is_gles)
        {
            core.texture_compression_etc2 = m_version >= 300;
            core.texture_compression_eac = m_version >= 300;
        }
        else
        {
            core.texture_compression_etc2 = m_version >= 430;
            core.texture_compression_eac = m_version >= 430;
        }
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

            case TextureCompression::ASTC_RGBA_4x4:
            case TextureCompression::ASTC_RGBA_5x4:
            case TextureCompression::ASTC_RGBA_5x5:
            case TextureCompression::ASTC_RGBA_6x5:
            case TextureCompression::ASTC_RGBA_6x6:
            case TextureCompression::ASTC_RGBA_8x5:
            case TextureCompression::ASTC_RGBA_8x6:
            case TextureCompression::ASTC_RGBA_8x8:
            case TextureCompression::ASTC_RGBA_10x5:
            case TextureCompression::ASTC_RGBA_10x6:
            case TextureCompression::ASTC_RGBA_10x8:
            case TextureCompression::ASTC_RGBA_10x10:
            case TextureCompression::ASTC_RGBA_12x10:
            case TextureCompression::ASTC_RGBA_12x12:
            case TextureCompression::ASTC_SRGB_ALPHA_4x4:
            case TextureCompression::ASTC_SRGB_ALPHA_5x4:
            case TextureCompression::ASTC_SRGB_ALPHA_5x5:
            case TextureCompression::ASTC_SRGB_ALPHA_6x5:
            case TextureCompression::ASTC_SRGB_ALPHA_6x6:
            case TextureCompression::ASTC_SRGB_ALPHA_8x5:
            case TextureCompression::ASTC_SRGB_ALPHA_8x6:
            case TextureCompression::ASTC_SRGB_ALPHA_8x8:
            case TextureCompression::ASTC_SRGB_ALPHA_10x5:
            case TextureCompression::ASTC_SRGB_ALPHA_10x6:
            case TextureCompression::ASTC_SRGB_ALPHA_10x8:
            case TextureCompression::ASTC_SRGB_ALPHA_10x10:
            case TextureCompression::ASTC_SRGB_ALPHA_12x10:
            case TextureCompression::ASTC_SRGB_ALPHA_12x12:
                supported = ext.KHR_texture_compression_astc_hdr || ext.KHR_texture_compression_astc_ldr;
                break;

            case TextureCompression::ASTC_RGBA_3x3x3:
            case TextureCompression::ASTC_RGBA_4x3x3:
            case TextureCompression::ASTC_RGBA_4x4x3:
            case TextureCompression::ASTC_RGBA_4x4x4:
            case TextureCompression::ASTC_RGBA_5x4x4:
            case TextureCompression::ASTC_RGBA_5x5x4:
            case TextureCompression::ASTC_RGBA_5x5x5:
            case TextureCompression::ASTC_RGBA_6x5x5:
            case TextureCompression::ASTC_RGBA_6x6x5:
            case TextureCompression::ASTC_RGBA_6x6x6:
            case TextureCompression::ASTC_SRGB_ALPHA_3x3x3:
            case TextureCompression::ASTC_SRGB_ALPHA_4x3x3:
            case TextureCompression::ASTC_SRGB_ALPHA_4x4x3:
            case TextureCompression::ASTC_SRGB_ALPHA_4x4x4:
            case TextureCompression::ASTC_SRGB_ALPHA_5x4x4:
            case TextureCompression::ASTC_SRGB_ALPHA_5x5x4:
            case TextureCompression::ASTC_SRGB_ALPHA_5x5x5:
            case TextureCompression::ASTC_SRGB_ALPHA_6x5x5:
            case TextureCompression::ASTC_SRGB_ALPHA_6x6x5:
            case TextureCompression::ASTC_SRGB_ALPHA_6x6x6:
                // TODO: OES_texture_compression_astc
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

#ifdef MANGO_OPENGL_FRAMEBUFFER

namespace
{

    // -------------------------------------------------------------------
    // built-in shader sources
    // -------------------------------------------------------------------

    const char* vertex_shader_source = R"(
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
        uniform sampler2D uTexture;

        in vec2 texcoord;
        out vec4 outFragment0;

        void main()
        {
            outFragment0 = texture(uTexture, texcoord);
        }
    )";

    const char* vertex_shader_source_bicubic = R"(
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

    GLuint create_shader(GLenum type, const char* source)
    {
        GLuint shader = glCreateShader(type);

        const char* table[] =
        {
            R"(
                #version 330
            )",
            source
        };

        glShaderSource(shader, 2, table, NULL);
        glCompileShader(shader);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status)
        {
            GLint size;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);

            GLchar* buffer = new GLchar[size + 1];
            GLsizei length;
            glGetShaderInfoLog(shader, size + 1, &length, buffer);

            printf("glCompileShader() ERROR:\n%s\n", buffer); // TODO: exception
            delete[] buffer;

            glDeleteShader(shader);
            shader = 0;
        }

        return shader;
    }

    GLuint create_program(const char* vertex_source, const char* fragment_source)
    {
        GLuint vs = create_shader(GL_VERTEX_SHADER, vertex_source);
        GLuint fs = create_shader(GL_FRAGMENT_SHADER, fragment_source);

        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glBindFragDataLocation(program, 0, "outFragment0");
        glLinkProgram(program);

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status)
        {
            GLint size;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);

            GLchar* buffer = new GLchar[size + 1];
            GLsizei length;
            glGetProgramInfoLog(program, size + 1, &length, buffer);

            printf("glLinkProgram() ERROR:\n%s\n", buffer); // TODO: exception
            delete[] buffer;

            glDeleteProgram(program);
            program = 0;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);

        return program;
    }

} // namespace

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

            m_index_program = create_program(vertex_shader_source, fragment_shader_source_index);
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

        m_bilinear.program = create_program(vertex_shader_source, fragment_shader_source);
        m_bilinear.transform = glGetUniformLocation(m_bilinear.program, "uTransform");
        m_bilinear.texture = glGetUniformLocation(m_bilinear.program, "uTexture");
        m_bilinear.position = glGetAttribLocation(m_bilinear.program, "inPosition");

        // create bicubic program

        m_bicubic.program = create_program(vertex_shader_source_bicubic, fragment_shader_source_bicubic);
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

#endif // MANGO_OPENGL_FRAMEBUFFER

} // namespace mango

#endif // MANGO_OPENGL_CONTEXT_NONE
