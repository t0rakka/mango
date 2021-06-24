/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#if !defined(__ppc__)

#include <mango/core/exception.hpp>
#include <mango/opengl/opengl.hpp>

/* TODO: (type, format) mapping to Format, example:

    glTexImage2D(..., GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, ...)
        { 0x8032, "UNSIGNED_BYTE_3_3_2" },
        { 0x8362, "UNSIGNED_BYTE_2_3_3_REV" },
        { 0x8363, "UNSIGNED_SHORT_5_6_5" },
        { 0x8364, "UNSIGNED_SHORT_5_6_5_REV" },
        { 0x8033, "UNSIGNED_SHORT_4_4_4_4" },
        { 0x8365, "UNSIGNED_SHORT_4_4_4_4_REV" },
        { 0x8034, "UNSIGNED_SHORT_5_5_5_1" },
        { 0x8366, "UNSIGNED_SHORT_1_5_5_5_REV" },
        { 0x8035, "UNSIGNED_INT_8_8_8_8" },
        { 0x8367, "UNSIGNED_INT_8_8_8_8_REV" },
        { 0x8036, "UNSIGNED_INT_10_10_10_2" },
        { 0x8368, "UNSIGNED_INT_2_10_10_10_REV" },
*/

#define MAKE_FORMAT(bits, type, order, s0, s1, s2, s3) \
    image::Format(bits, image::Format::type, image::Format::order, s0, s1, s2, s3)

namespace
{
    using namespace mango;

    const OpenGLContext::InternalFormat g_format_table[] =
    {
        // 1.0
        { 0x1903, MAKE_FORMAT(  8, UNORM,    R,  8, 0, 0, 0), false, "RED" },
        { 0x1904, MAKE_FORMAT(  8, UNORM,    G,  8, 0, 0, 0), false, "GREEN" },
        { 0x1905, MAKE_FORMAT(  8, UNORM,    B,  8, 0, 0, 0), false, "BLUE" },
        { 0x1906, MAKE_FORMAT(  8, UNORM,    A,  8, 0, 0, 0), false, "ALPHA" },
        { 0x1907, MAKE_FORMAT( 24, UNORM,  RGB,  8, 8, 8, 0), false, "RGB" },
        { 0x1908, MAKE_FORMAT( 32, UNORM, RGBA,  8, 8, 8, 8), false, "RGBA" },

        // 1.1
        { 0x2A10, MAKE_FORMAT(  8, UNORM, RGB,   3,  3,  2,  0), false, "R3_G3_B2" },
        { 0x804F, MAKE_FORMAT( 16, UNORM, RGB,   4,  4,  4,  0), false, "RGB4" },
        { 0x8050, MAKE_FORMAT( 16, UNORM, RGB,   5,  5,  5,  0), false, "RGB5" },
        { 0x8051, MAKE_FORMAT( 24, UNORM, RGB,   8,  8,  8,  0), false, "RGB8" },
        { 0x8052, MAKE_FORMAT( 32, UNORM, RGB,  10, 10, 10,  0), false, "RGB10" },
        { 0x8053, MAKE_FORMAT( 48, UNORM, RGB,  12, 12, 12,  0), false, "RGB12" },
        { 0x8054, MAKE_FORMAT( 48, UNORM, RGB,  16, 16, 16,  0), false, "RGB16" },
        { 0x8055, MAKE_FORMAT(  8, UNORM, RGBA,  2,  2,  2,  2), false, "RGBA2" },
        { 0x8056, MAKE_FORMAT( 16, UNORM, RGBA,  4,  4,  4,  4), false, "RGBA4" },
        { 0x8057, MAKE_FORMAT( 16, UNORM, RGBA,  5,  5,  5,  1), false, "RGB5_A1" },
        { 0x8058, MAKE_FORMAT( 32, UNORM, RGBA,  8,  8,  8,  8), false, "RGBA8" },
        { 0x8059, MAKE_FORMAT( 32, UNORM, RGBA, 10, 10, 10,  2), false, "RGB10_A2" },
        { 0x805A, MAKE_FORMAT( 48, UNORM, RGBA, 12, 12, 12, 12), false, "RGBA12" },
        { 0x805B, MAKE_FORMAT( 64, UNORM, RGBA, 16, 16, 16, 16), false, "RGBA16" },

        // 1.2
        { 0x8032, MAKE_FORMAT(  8, UNORM,  BGR, 2, 3, 3, 0), false, "UNSIGNED_BYTE_3_3_2" },
        { 0x8033, MAKE_FORMAT( 16, UNORM, ABGR, 4, 4, 4, 4), false, "UNSIGNED_SHORT_4_4_4_4" },
        { 0x8034, MAKE_FORMAT( 16, UNORM, ABGR, 1, 5, 5, 5), false, "UNSIGNED_SHORT_5_5_5_1" },
        { 0x8035, MAKE_FORMAT( 32, UNORM, ABGR, 8, 8, 8, 8), false, "UNSIGNED_INT_8_8_8_8" },
        { 0x8036, MAKE_FORMAT( 32, UNORM, ABGR, 2, 10, 10, 10), false, "UNSIGNED_INT_10_10_10_2" },
        { 0x8362, MAKE_FORMAT(  8, UNORM,  RGB, 3, 3, 2, 0), false, "UNSIGNED_BYTE_2_3_3_REV" },
        { 0x8363, MAKE_FORMAT( 16, UNORM,  BGR, 5, 6, 5, 0), false, "UNSIGNED_SHORT_5_6_5" },
        { 0x8364, MAKE_FORMAT( 16, UNORM,  RGB, 5, 6, 5, 0), false, "UNSIGNED_SHORT_5_6_5_REV" },
        { 0x8365, MAKE_FORMAT( 16, UNORM, RGBA, 4, 4, 4, 4), false, "UNSIGNED_SHORT_4_4_4_4_REV" },
        { 0x8366, MAKE_FORMAT( 16, UNORM, RGBA, 5, 5, 5, 1), false, "UNSIGNED_SHORT_1_5_5_5_REV" },
        { 0x8367, MAKE_FORMAT( 32, UNORM, RGBA, 8, 8, 8, 8), false, "UNSIGNED_INT_8_8_8_8_REV" },
        { 0x8368, MAKE_FORMAT( 32, UNORM, RGBA, 10, 10, 10, 2), false, "UNSIGNED_INT_2_10_10_10_REV" },
        { 0x80E0, MAKE_FORMAT( 24, UNORM,  BGR,  8, 8, 8, 0), false, "BGR" },
        { 0x80E1, MAKE_FORMAT( 32, UNORM, BGRA,  8, 8, 8, 8), false, "BGRA" },

        // 2.1
        { 0x8C40, MAKE_FORMAT( 24, UNORM, RGB,   8,  8,  8,  0),  true, "SRGB" },
        { 0x8C41, MAKE_FORMAT( 24, UNORM, RGB,   8,  8,  8,  0),  true, "SRGB8" },
        { 0x8C42, MAKE_FORMAT( 32, UNORM, RGBA,  8,  8,  8,  8),  true, "SRGB_ALPHA" },
        { 0x8C43, MAKE_FORMAT( 32, UNORM, RGBA,  8,  8,  8,  8),  true, "SRGB8_ALPHA8" },

        // 3.0
        { 0x8814, MAKE_FORMAT(128, FLOAT32, RGBA, 32, 32, 32, 32), false, "RGBA32F" },
        { 0x8815, MAKE_FORMAT( 96, FLOAT32, RGB,  32, 32, 32,  0), false, "RGB32F" },
        { 0x881A, MAKE_FORMAT( 64, FLOAT16, RGBA, 16, 16, 16, 16), false, "RGBA16F" },
        { 0x881B, MAKE_FORMAT( 48, FLOAT16, RGB,  16, 16, 16,  0), false, "RGB16F" },
        //{ 0x8C3A, MAKE_FORMAT(), false, "R11F_G11F_B10F" },
        //{ 0x8C3B, MAKE_FORMAT(), false, "UNSIGNED_INT_10F_11F_11F_REV" },
        //{ 0x8C3D, MAKE_FORMAT(), false, "RGB9_E5" },
        { 0x8C3E, MAKE_FORMAT( 32, UNORM, RGBA, 9, 9, 9, 5), false, "UNSIGNED_INT_5_9_9_9_REV" },
        { 0x8D70, MAKE_FORMAT(128, UINT,  RGBA, 32, 32, 32, 32), false, "RGBA32UI" },
        { 0x8D71, MAKE_FORMAT( 96, UINT,  RGB,  32, 32, 32,  0), false, "RGB32UI" },
        { 0x8D76, MAKE_FORMAT( 64, UINT,  RGBA, 16, 16, 16, 16), false, "RGBA16UI" },
        { 0x8D77, MAKE_FORMAT( 48, UINT,  RGB,  16, 16, 16,  0), false, "RGB16UI" },
        { 0x8D7C, MAKE_FORMAT( 32, UINT,  RGBA,  8,  8,  8,  8), false, "RGBA8UI" },
        { 0x8D7D, MAKE_FORMAT( 24, UINT,  RGB,   8,  8,  8,  0), false, "RGB8UI" },
        { 0x8D82, MAKE_FORMAT(128, SINT,  RGBA, 32, 32, 32, 32), false, "RGBA32I" },
        { 0x8D83, MAKE_FORMAT( 96, SINT,  RGB,  32, 32, 32,  0), false, "RGB32I" },
        { 0x8D88, MAKE_FORMAT( 64, SINT,  RGBA, 16, 16, 16, 16), false, "RGBA16I" },
        { 0x8D89, MAKE_FORMAT( 48, SINT,  RGB,  16, 16, 16,  0), false, "RGB16I" },
        { 0x8D8E, MAKE_FORMAT( 32, SINT,  RGBA,  8,  8,  8,  8), false, "RGBA8I" },
        { 0x8D8F, MAKE_FORMAT( 24, SINT,  RGB,   8,  8,  8,  0), false, "RGB8I" },
        { 0x8227, MAKE_FORMAT( 16, UNORM, RG,    8,  8,  0,  0), false, "RG" },
        { 0x8229, MAKE_FORMAT(  8, UNORM, R,     8,  0,  0,  0), false, "R8" },
        { 0x822A, MAKE_FORMAT( 16, UNORM, R,    16,  0,  0,  0), false, "R16" },
        { 0x822B, MAKE_FORMAT( 16, UNORM, RG,    8,  8,  0,  0), false, "RG8" },
        { 0x822C, MAKE_FORMAT( 32, UNORM, RG,   16, 16,  0,  0), false, "RG16" },
        { 0x822D, MAKE_FORMAT( 16, FLOAT16, R,    16,  0,  0,  0), false, "R16F" },
        { 0x822E, MAKE_FORMAT( 32, FLOAT32, R,    32,  0,  0,  0), false, "R32F" },
        { 0x822F, MAKE_FORMAT( 32, FLOAT16, RG,   16, 16,  0,  0), false, "RG16F" },
        { 0x8230, MAKE_FORMAT( 64, FLOAT32, RG,   32, 32,  0,  0), false, "RG32F" },
        { 0x8231, MAKE_FORMAT(  8, SINT,  R,     8,  0,  0,  0), false, "R8I" },
        { 0x8232, MAKE_FORMAT(  8, UINT,  R,     8,  0,  0,  0), false, "R8UI" },
        { 0x8233, MAKE_FORMAT( 16, SINT,  R,    16,  0,  0,  0), false, "R16I" },
        { 0x8234, MAKE_FORMAT( 16, UINT,  R,    16,  0,  0,  0), false, "R16UI" },
        { 0x8235, MAKE_FORMAT( 32, SINT,  R,    32,  0,  0,  0), false, "R32I" },
        { 0x8236, MAKE_FORMAT( 32, UINT,  R,    32,  0,  0,  0), false, "R32UI" },
        { 0x8237, MAKE_FORMAT( 16, SINT,  RG,    8,  8,  0,  0), false, "RG8I" },
        { 0x8238, MAKE_FORMAT( 16, UINT,  RG,    8,  8,  0,  0), false, "RG8UI" },
        { 0x8239, MAKE_FORMAT( 32, SINT,  RG,   16, 16,  0,  0), false, "RG16I" },
        { 0x823A, MAKE_FORMAT( 32, UINT,  RG,   16, 16,  0,  0), false, "RG16UI" },
        { 0x823B, MAKE_FORMAT( 64, SINT,  RG,   32, 32,  0,  0), false, "RG32I" },
        { 0x823C, MAKE_FORMAT( 64, UINT,  RG,   32, 32,  0,  0), false, "RG32UI" },

        // 3.1
        { 0x8F94, MAKE_FORMAT(  8, SNORM, R,     8,  0,  0,  0), false, "R8_SNORM" },
        { 0x8F95, MAKE_FORMAT( 16, SNORM, RG,    8,  8,  0,  0), false, "RG8_SNORM" },
        { 0x8F96, MAKE_FORMAT( 24, SNORM, RGB,   8,  8,  8,  0), false, "RGB8_SNORM" },
        { 0x8F97, MAKE_FORMAT( 32, SNORM, RGBA,  8,  8,  8,  8), false, "RGBA8_SNORM" },
        { 0x8F98, MAKE_FORMAT( 16, SNORM, R,    16,  0,  0,  0), false, "R16_SNORM" },
        { 0x8F99, MAKE_FORMAT( 32, SNORM, RG,   16, 16,  0,  0), false, "RG16_SNORM" },
        { 0x8F9A, MAKE_FORMAT( 48, SNORM, RGB,  16, 16, 16,  0), false, "RGB16_SNORM" },
        { 0x8F9B, MAKE_FORMAT( 64, SNORM, RGBA, 16, 16, 16, 16), false, "RGBA16_SNORM" },

        // 3.3
        { 0x906F, MAKE_FORMAT( 32, UINT,  RGBA, 10, 10, 10,  2), false, "RGB10_A2UI" },
        { 0x8D9F, MAKE_FORMAT( 32, UNORM, RGBA, 10, 10, 10,  2), false, "INT_2_10_10_10_REV" },

        // 4.1
        { 0x8D62, MAKE_FORMAT( 16, UNORM, RGB,   5,  6,  5,  0), false, "RGB565" },

        // GL_EXT_texture
        { 0x803B, MAKE_FORMAT(  8, UNORM,   A,  4, 0, 0, 0), false, "ALPHA4" },
        { 0x803C, MAKE_FORMAT(  8, UNORM,   A,  8, 0, 0, 0), false, "ALPHA8" },
        { 0x803D, MAKE_FORMAT( 16, UNORM,   A, 12, 0, 0, 0), false, "ALPHA12" },
        { 0x803D, MAKE_FORMAT( 16, UNORM,   A, 16, 0, 0, 0), false, "ALPHA16" },

        // GL_EXT_texture_integer
        { 0x8D90, MAKE_FORMAT(  8, SINT,    A,  8, 0, 0, 0), false, "ALPHA8I" },
        { 0x8D7E, MAKE_FORMAT(  8, UINT,    A,  8, 0, 0, 0), false, "ALPHA8UI" },
        { 0x8D8A, MAKE_FORMAT( 16, SINT,    A, 16, 0, 0, 0), false, "ALPHA16I" },
        { 0x8D78, MAKE_FORMAT( 16, UINT,    A, 16, 0, 0, 0), false, "ALPHA16UI" },
        { 0x8D84, MAKE_FORMAT( 32, SINT,    A, 32, 0, 0, 0), false, "ALPHA32I" },
        { 0x8D72, MAKE_FORMAT( 32, UINT,    A, 32, 0, 0, 0), false, "ALPHA32UI" },

        // GL_{ATI,ARB,APPLE}_texture_float
        { 0x881C, MAKE_FORMAT( 16, FLOAT16, A, 16, 0, 0, 0), false, "ALPHA16F" },
        { 0x8816, MAKE_FORMAT( 32, FLOAT32, A, 32, 0, 0, 0), false, "ALPHA32F" },
    };

} // namespace

namespace mango
{
    using namespace math;
    using namespace image;

    // -----------------------------------------------------------------------
    // extension masks
    // -----------------------------------------------------------------------

    static void init_ext(OpenGLContext& context)
    {
        const char* names[] =
        {
#define GL_EXTENSION(Name) "GL_"#Name,
#include <mango/opengl/func/glext.hpp>
#undef GL_EXTENSION
        };
        const int size = sizeof(names) / sizeof(names[0]);

        u32* mask = reinterpret_cast<u32*>(&context.ext);
        std::memset(mask, 0, sizeof(context.ext));

        for (int i = 0; i < size; ++i)
        {
            if (context.isExtension(names[i]))
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

    static void init_core(OpenGLContext& context, int version)
    {
        int versions[] =
        {
#define CORE_EXTENSION(Version, Name)  Version,
#include <mango/opengl/func/glcorearb.hpp>
#undef CORE_EXTENSION
        };
        const int size = sizeof(versions) / sizeof(versions[0]);

        u32* mask = reinterpret_cast<u32*>(&context.core);
        std::memset(mask, 0, sizeof(context.core));

        for (int i = 0; i < size; ++i)
        {
            if (version >= versions[i])
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

#if defined(MANGO_OPENGL_CONTEXT_WGL)

    static void init_wgl(OpenGLContext& context)
    {
        const char* names[] =
        {
#define WGL_EXTENSION(Name) "WGL_"#Name,
#include <mango/opengl/func/wglext.hpp>
#undef WGL_EXTENSION
        };
        const int size = sizeof(names) / sizeof(names[0]);

        u32* mask = reinterpret_cast<u32*>(&context.wgl);
        std::memset(mask, 0, sizeof(context.wgl));

        for (int i = 0; i < size; ++i)
        {
            if (context.isExtension(names[i]))
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

#endif

#if defined(MANGO_OPENGL_CONTEXT_GLX)

    static void init_glx(OpenGLContext& context)
    {
        const char* names[] =
        {
#define GLX_EXTENSION(Name) "GLX_"#Name,
#include <mango/opengl/func/glxext.hpp>
#undef GLX_EXTENSION
        };
        const int size = sizeof(names) / sizeof(names[0]);

        u32* mask = reinterpret_cast<u32*>(&context.glx);
        std::memset(mask, 0, sizeof(context.glx));

        for (int i = 0; i < size; ++i)
        {
            if (context.isExtension(names[i]))
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

#endif

    // -----------------------------------------------------------------------
    // OpenGLContext
    // -----------------------------------------------------------------------

    void OpenGLContext::initExtensionMask()
    {
        const int version = getVersion();
        const bool gles = isGLES();

        // TODO: GLES version number

        init_ext(*this);
        init_core(*this, version);

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

        if (gles)
        {
            core.texture_compression_etc2 = version >= 300;
            core.texture_compression_eac = version >= 300;
        }
        else
        {
            core.texture_compression_etc2 = version >= 430;
            core.texture_compression_eac = version >= 430;
        }
    }

    bool OpenGLContext::isExtension(const std::string& name) const
    {
        return m_extensions.find(name) != m_extensions.end();
    }

    bool OpenGLContext::isGLES() const
    {
        return false; // TODO: add GLES support
    }

    int OpenGLContext::getVersion() const
    {
        int major;
        int minor;
        const char* str = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        std::sscanf(str, "%d.%d", &major, &minor);
        int version = major * 100 + minor * 10;
        return version;
    }

    bool OpenGLContext::isCompressedTextureSupported(TextureCompression compression) const
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
            if (node.iformat == internalFormat)
                return &node;
        }

        return nullptr;
    }

#ifdef MANGO_OPENGL_FRAMEBUFFER

namespace {

    // -------------------------------------------------------------------
    // built-in shader sources
    // -------------------------------------------------------------------

    const char* vertex_shader_source =
        "uniform vec4 uTransform = vec4(0.0, 0.0, 1.0, 1.0); \n"
        "in vec2 inPosition; \n"
        "out vec2 texcoord; \n"
        "void main() { \n"
        "    texcoord = inPosition * vec2(0.5, -0.5) + vec2(0.5); \n"
        "    gl_Position = vec4((inPosition + uTransform.xy) * uTransform.zw, 0.0, 1.0); \n"
        "}\n";

    const char* fragment_shader_source =
        "uniform sampler2D uTexture; \n"
        "in vec2 texcoord; \n"
        "out vec4 outFragment0; \n"
        "void main() { \n"
        "    outFragment0 = texture(uTexture, texcoord); \n"
        "} \n";

    const char* vertex_shader_source_bicubic =
        "uniform vec4 uTransform = vec4(0.0, 0.0, 1.0, 1.0); \n"
        "in vec2 inPosition; \n"
        "out vec2 texcoord; \n"
        "void main() { \n"
        "    texcoord = inPosition * vec2(0.5, -0.5) + vec2(0.5); \n"
        "    gl_Position = vec4((inPosition + uTransform.xy) * uTransform.zw, 0.0, 1.0); \n"
        "}\n";

    const char* fragment_shader_source_bicubic =
        "vec4 cubic(float v) \n"
        "{ \n"
        "    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v; \n"
        "    vec4 s = n * n * n; \n"
        "    float x = s.x; \n"
        "    float y = s.y - 4.0 * s.x; \n"
        "    float z = s.z - 4.0 * s.y + 6.0 * s.x; \n"
        "    float w = 6.0 - x - y - z; \n"
        "    return vec4(x, y, z, w); \n"
        "} \n"
        "vec4 texture_filter(sampler2D uTexture, vec2 texcoord, vec2 texscale) \n"
        "{ \n"
        "    // hack to bring unit texcoords to integer pixel coords \n"
        "    texcoord /= texscale; \n"
        "    texcoord -= vec2(0.5, 0.5f); \n"
        "    float fx = fract(texcoord.x); \n"
        "    float fy = fract(texcoord.y); \n"
        "    texcoord.x -= fx; \n"
        "    texcoord.y -= fy; \n"
        "    vec4 cx = cubic(fx); \n"
        "    vec4 cy = cubic(fy); \n"
        "    vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y - 0.5, texcoord.y + 1.5); \n"
        "    vec4 s = vec4(cx.x + cx.y, cx.z + cx.w, cy.x + cy.y, cy.z + cy.w); \n"
        "    vec4 offset = c + vec4(cx.y, cx.w, cy.y, cy.w) / s; \n"
        "    vec4 sample0 = texture(uTexture, vec2(offset.x, offset.z) * texscale); \n"
        "    vec4 sample1 = texture(uTexture, vec2(offset.y, offset.z) * texscale); \n"
        "    vec4 sample2 = texture(uTexture, vec2(offset.x, offset.w) * texscale); \n"
        "    vec4 sample3 = texture(uTexture, vec2(offset.y, offset.w) * texscale); \n"
        "    float sx = s.x / (s.x + s.y); \n"
        "    float sy = s.z / (s.z + s.w); \n"
        "    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy); \n"
        "} \n"
        "uniform sampler2D uTexture; \n"
        "uniform vec2 uTexScale; \n"
        "in vec2 texcoord; \n"
        "out vec4 outFragment0; \n"
        "void main() { \n"
        "    outFragment0 = texture_filter(uTexture, texcoord, uTexScale); \n"
        "} \n";

    const char* fragment_shader_source_index =
        "uniform isampler2D uTexture; \n"
        "uniform uint uPalette[256];\n"
        "in vec2 texcoord; \n"
        "out vec4 outFragment0; \n"
        "void main() { \n"
        "    uint color = uPalette[texture(uTexture, texcoord).r]; \n"
        "    float r = ((color << 24) >> 24) / 255.0; \n"
        "    float g = ((color << 16) >> 24) / 255.0; \n"
        "    float b = ((color <<  8) >> 24) / 255.0; \n"
        "    float a = ((color <<  0) >> 24) / 255.0; \n"
        "    outFragment0 = vec4(r, g, b, a); \n"
        "} \n";

    std::string getShadingLanguageVersionString()
    {
        std::string s = "#version 110\n"; // default for OpenGL 2.0

        const char* version = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
        if (version)
        {
            int major;
            int minor;
            std::sscanf(version, "%d.%d", &major, &minor);
            char buffer[80];
            sprintf(buffer, "#version %d%d\n", major, minor);
            s = buffer;
        }

        return s;
    }

    GLuint createShader(GLenum type, const char* source)
    {
        GLuint shader = glCreateShader(type);

        std::string header = getShadingLanguageVersionString();

        const GLchar* string[] =
        {
            reinterpret_cast<const GLchar*>(header.c_str()),
            reinterpret_cast<const GLchar*>(source)
        };

        glShaderSource(shader, 2, string, NULL);
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

    GLuint createProgram(const char* vertex_source, const char* fragment_source)
    {
        GLuint vs = createShader(GL_VERTEX_SHADER, vertex_source);
        GLuint fs = createShader(GL_FRAGMENT_SHADER, fragment_source);

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

#endif
