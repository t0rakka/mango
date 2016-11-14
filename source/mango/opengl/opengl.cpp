/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/opengl/opengl.hpp>

// Enable this for validating extension mask -- WARNING: compiling will slow down
//#define ENABLE_CONTEXT_VALIDATE

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

    { 0x1907, "RGB" },
    { 0x1908, "RGBA" },
    { 0x80E0, "BGR" },
    { 0x80E1, "BGRA" },
*/

namespace
{
    using namespace mango;
    using namespace mango::opengl;

    const InternalFormat g_formatTable[] =
    {
        { 0x803B, MAKE_FORMAT(  8, UNORM, A,  4, 0, 0, 0), false, "ALPHA4" },
        { 0x803C, MAKE_FORMAT(  8, UNORM, A,  8, 0, 0, 0), false, "ALPHA8" },
        { 0x803D, MAKE_FORMAT( 16, UNORM, A, 12, 0, 0, 0), false, "ALPHA12" },
        { 0x803D, MAKE_FORMAT( 16, UNORM, A, 16, 0, 0, 0), false, "ALPHA16" },
        { 0x8D90, MAKE_FORMAT(  8, SINT,  A,  8, 0, 0, 0), false, "ALPHA8I" },
        { 0x8D7E, MAKE_FORMAT(  8, UINT,  A,  8, 0, 0, 0), false, "ALPHA8UI" },
        { 0x8D8A, MAKE_FORMAT( 16, SINT,  A, 16, 0, 0, 0), false, "ALPHA16I" },
        { 0x8D78, MAKE_FORMAT( 16, UINT,  A, 16, 0, 0, 0), false, "ALPHA16UI" },
        { 0x881C, MAKE_FORMAT( 16, FP16,  A, 16, 0, 0, 0), false, "ALPHA16F" },
        { 0x8D84, MAKE_FORMAT( 32, SINT,  A, 32, 0, 0, 0), false, "ALPHA32I" },
        { 0x8D72, MAKE_FORMAT( 32, UINT,  A, 32, 0, 0, 0), false, "ALPHA32UI" },
        { 0x8816, MAKE_FORMAT( 32, FP32,  A, 32, 0, 0, 0), false, "ALPHA32F" },

        { 0x8229, MAKE_FORMAT(  8, UNORM, R,     8,  0,  0,  0), false, "R8" },
        { 0x822A, MAKE_FORMAT( 16, UNORM, R,    16,  0,  0,  0), false, "R16" },
        { 0x8227, MAKE_FORMAT( 16, UNORM, RG,    8,  8,  0,  0), false, "RG" },
        { 0x822B, MAKE_FORMAT( 16, UNORM, RG,    8,  8,  0,  0), false, "RG8" },
        { 0x822C, MAKE_FORMAT( 32, UNORM, RG,   16, 16,  0,  0), false, "RG16" },
        { 0x2A10, MAKE_FORMAT(  8, UNORM, RGB,   3,  3,  2,  0), false, "R3_G3_B2" },
        { 0x804F, MAKE_FORMAT( 16, UNORM, RGB,   4,  4,  4,  0), false, "RGB4" },
        { 0x8050, MAKE_FORMAT( 16, UNORM, RGB,   5,  5,  5,  0), false, "RGB5" },
        { 0x8051, MAKE_FORMAT( 24, UNORM, RGB,   8,  8,  8,  0), false, "RGB8" },
        { 0x8052, MAKE_FORMAT( 32, UNORM, RGB,  10, 10, 10,  0), false, "RGB10" },
        { 0x8053, MAKE_FORMAT( 48, UNORM, RGB,  12, 12, 12,  0), false, "RGB12" },
        { 0x8054, MAKE_FORMAT( 48, UNORM, RGB,  16, 16, 16,  0), false, "RGB16" },
        { 0x8C40, MAKE_FORMAT( 24, UNORM, RGB,   8,  8,  8,  0),  true, "SRGB" },
        { 0x8C41, MAKE_FORMAT( 24, UNORM, RGB,   8,  8,  8,  0),  true, "SRGB8" },
        { 0x8D62, MAKE_FORMAT( 16, UNORM, RGB,   5,  6,  5,  0), false, "RGB565" },
        { 0x8055, MAKE_FORMAT(  8, UNORM, RGBA,  2,  2,  2,  2), false, "RGBA2" },
        { 0x8056, MAKE_FORMAT( 16, UNORM, RGBA,  4,  4,  4,  4), false, "RGBA4" },
        { 0x8057, MAKE_FORMAT( 16, UNORM, RGBA,  5,  5,  5,  1), false, "RGB5_A1" },
        { 0x8058, MAKE_FORMAT( 32, UNORM, RGBA,  8,  8,  8,  8), false, "RGBA8" },
        { 0x8059, MAKE_FORMAT( 32, UNORM, RGBA, 10, 10, 10,  2), false, "RGB10_A2" },
        { 0x8C42, MAKE_FORMAT( 32, UNORM, RGBA,  8,  8,  8,  8),  true, "SRGB_ALPHA" },
        { 0x8C43, MAKE_FORMAT( 32, UNORM, RGBA,  8,  8,  8,  8),  true, "SRGB8_ALPHA8" },
        { 0x805A, MAKE_FORMAT( 48, UNORM, RGBA, 12, 12, 12, 12), false, "RGBA12" },
        { 0x805B, MAKE_FORMAT( 64, UNORM, RGBA, 16, 16, 16, 16), false, "RGBA16" },

        { 0x8F94, MAKE_FORMAT(  8, SNORM, R,     8,  0,  0,  0), false, "R8_SNORM" },
        { 0x8F95, MAKE_FORMAT( 16, SNORM, RG,    8,  8,  0,  0), false, "RG8_SNORM" },
        { 0x8F96, MAKE_FORMAT( 24, SNORM, RGB,   8,  8,  8,  0), false, "RGB8_SNORM" },
        { 0x8F97, MAKE_FORMAT( 32, SNORM, RGBA,  8,  8,  8,  8), false, "RGBA8_SNORM" },

        { 0x8F98, MAKE_FORMAT( 16, SNORM, R,    16,  0,  0,  0), false, "R16_SNORM" },
        { 0x8F99, MAKE_FORMAT( 32, SNORM, RG,   16, 16,  0,  0), false, "RG16_SNORM" },
        { 0x8F9A, MAKE_FORMAT( 48, SNORM, RGB,  16, 16, 16,  0), false, "RGB16_SNORM" },
        { 0x8F9B, MAKE_FORMAT( 64, SNORM, RGBA, 16, 16, 16, 16), false, "RGBA16_SNORM" },

        { 0x8232, MAKE_FORMAT(  8, UINT,  R,     8,  0,  0,  0), false, "R8UI" },
        { 0x8238, MAKE_FORMAT( 16, UINT,  RG,    8,  8,  0,  0), false, "RG8UI" },
        { 0x8D7D, MAKE_FORMAT( 24, UINT,  RGB,   8,  8,  8,  0), false, "RGB8UI" },
        { 0x8D7C, MAKE_FORMAT( 32, UINT,  RGBA,  8,  8,  8,  8), false, "RGBA8UI" },

        { 0x8234, MAKE_FORMAT( 16, UINT,  R,    16,  0,  0,  0), false, "R16UI" },
        { 0x823A, MAKE_FORMAT( 32, UINT,  RG,   16, 16,  0,  0), false, "RG16UI" },
        { 0x8D77, MAKE_FORMAT( 48, UINT,  RGB,  16, 16, 16,  0), false, "RGB16UI" },
        { 0x8D76, MAKE_FORMAT( 64, UINT,  RGBA, 16, 16, 16, 16), false, "RGBA16UI" },

        { 0x8236, MAKE_FORMAT( 32, UINT,  R,    32,  0,  0,  0), false, "R32UI" },
        { 0x823C, MAKE_FORMAT( 64, UINT,  RG,   32, 32,  0,  0), false, "RG32UI" },
        { 0x8D71, MAKE_FORMAT( 96, UINT,  RGB,  32, 32, 32,  0), false, "RGB32UI" },
        { 0x8D70, MAKE_FORMAT(128, UINT,  RGBA, 32, 32, 32, 32), false, "RGBA32UI" },

        { 0x906F, MAKE_FORMAT( 32, UINT,  RGBA, 10, 10, 10,  2), false, "RGB10_A2UI" },

        { 0x8231, MAKE_FORMAT(  8, SINT,  R,     8,  0,  0,  0), false, "R8I" },
        { 0x8237, MAKE_FORMAT( 16, SINT,  RG,    8,  8,  0,  0), false, "RG8I" },
        { 0x8D8F, MAKE_FORMAT( 24, SINT,  RGB,   8,  8,  8,  0), false, "RGB8I" },
        { 0x8D8E, MAKE_FORMAT( 32, SINT,  RGBA,  8,  8,  8,  8), false, "RGBA8I" },

        { 0x8233, MAKE_FORMAT( 16, SINT,  R,    16,  0,  0,  0), false, "R16I" },
        { 0x8239, MAKE_FORMAT( 32, SINT,  RG,   16, 16,  0,  0), false, "RG16I" },
        { 0x8D89, MAKE_FORMAT( 48, SINT,  RGB,  16, 16, 16,  0), false, "RGB16I" },
        { 0x8D88, MAKE_FORMAT( 64, SINT,  RGBA, 16, 16, 16, 16), false, "RGBA16I" },

        { 0x8235, MAKE_FORMAT( 32, SINT,  R,    32,  0,  0,  0), false, "R32I" },
        { 0x823B, MAKE_FORMAT( 64, SINT,  RG,   32, 32,  0,  0), false, "RG32I" },
        { 0x8D83, MAKE_FORMAT( 96, SINT,  RGB,  32, 32, 32,  0), false, "RGB32I" },
        { 0x8D82, MAKE_FORMAT(128, SINT,  RGBA, 32, 32, 32, 32), false, "RGBA32I" },

        { 0x822D, MAKE_FORMAT( 16, FP16,  R,    16,  0,  0,  0), false, "R16F" },
        { 0x822F, MAKE_FORMAT( 32, FP16,  RG,   16, 16,  0,  0), false, "RG16F" },
        { 0x881B, MAKE_FORMAT( 48, FP16,  RGB,  16, 16, 16,  0), false, "RGB16F" },
        { 0x881A, MAKE_FORMAT( 64, FP16,  RGBA, 16, 16, 16, 16), false, "RGBA16F" },

        { 0x822E, MAKE_FORMAT( 32, FP32,  R,    32,  0,  0,  0), false, "R32F" },
        { 0x8230, MAKE_FORMAT( 64, FP32,  RG,   32, 32,  0,  0), false, "RG32F" },
        { 0x8815, MAKE_FORMAT( 96, FP32,  RGB,  32, 32, 32,  0), false, "RGB32F" },
        { 0x8814, MAKE_FORMAT(128, FP32,  RGBA, 32, 32, 32, 32), false, "RGBA32F" },
    };

} // namespace

namespace mango {
namespace opengl {

    // -----------------------------------------------------------------------
    // ExtensionMask
    // -----------------------------------------------------------------------

    glExtensionMask glext;

#ifdef ENABLE_CONTEXT_VALIDATE
    static void check(uint32 mask, bool is, const char* name)
    {
        bool isMask = mask != 0;
        printf("Feature: %s --> %s (%d)\n", name, isMask == is ? "OK" : "FAIL", is);
        if (isMask != is)
        {
            MANGO_EXCEPTION("ExtensionMask check failed.");
        }
    }
#endif

    static void init_glext(Context& context)
    {
        static const char* names[] =
        {
#define GL_EXTENSION(Name) "GL_"#Name,
#include <mango/opengl/func/glext.hpp>
#undef GL_EXTENSION
        };
        static const int size = sizeof(names) / sizeof(names[0]);

        uint32* mask = reinterpret_cast<uint32*>(&glext);
        std::memset(mask, 0, sizeof(glext));

        for (int i = 0; i < size; ++i)
        {
            if (context.isExtension(names[i]))
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }

#ifdef ENABLE_CONTEXT_VALIDATE
        printf("Validating glext mask...\n");
#define GL_EXTENSION(Name) check(glext.Name, context.isExtension("GL_"#Name), "GL_"#Name);
#include <mango/opengl/func/glext.hpp>
#undef GL_EXTENSION
#endif
    }

    coreExtensionMask core;

    static void init_core(int version)
    {
        static int versions[] =
        {
#define CORE_EXTENSION(Version, Name)  Version,
#include <mango/opengl/func/glcorearb.hpp>
#undef CORE_EXTENSION
        };
        static const int size = sizeof(versions) / sizeof(versions[0]);

        uint32* mask = reinterpret_cast<uint32*>(&core);
        std::memset(mask, 0, sizeof(core));

        for (int i = 0; i < size; ++i)
        {
            if (version >= versions[i])
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }

#ifdef MANGO_CONTEXT_WGL
    wglExtensionMask wglext;

    static void init_wglext(Context& context)
    {
        static const char* names[] =
        {
#define WGL_EXTENSION(Name) "WGL_"#Name,
#include <mango/opengl/func/wglext.hpp>
#undef WGL_EXTENSION
        };
        static const int size = sizeof(names) / sizeof(names[0]);

        uint32* mask = reinterpret_cast<uint32*>(&wglext);
        std::memset(mask, 0, sizeof(wglext));

        for (int i = 0; i < size; ++i)
        {
            if (context.isExtension(names[i]))
            {
                mask[i / 32] |= (1 << (i % 32));
            }
        }
    }
#endif

#ifdef MANGO_CONTEXT_GLX
    glxExtensionMask glxext;

    static void init_glxext(Context& context)
    {
        static const char* names[] =
        {
#define GLX_EXTENSION(Name) "GLX_"#Name,
#include <mango/opengl/func/glxext.hpp>
#undef GLX_EXTENSION
        };
        static const int size = sizeof(names) / sizeof(names[0]);

        uint32* mask = reinterpret_cast<uint32*>(&glxext);
        std::memset(mask, 0, sizeof(glxext));

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
    // Context
    // -----------------------------------------------------------------------

    void Context::initExtensionMask()
    {
        const int version = getVersion();
        const bool gles = isGLES();

        // TODO: GLES version number

        init_glext(*this);
        init_core(version);

#ifdef MANGO_CONTEXT_WGL
        init_wglext(*this);
#endif

#ifdef MANGO_CONTEXT_GLX
        init_glxext(*this);
#endif

        core.texture_compression_dxt1 = false;
        core.texture_compression_dxt3 = false;
        core.texture_compression_dxt5 = false;

        if (glext.EXT_texture_compression_s3tc || glext.NV_texture_compression_s3tc)
        {
            core.texture_compression_dxt1 = true;
            core.texture_compression_dxt3 = true;
            core.texture_compression_dxt5 = true;
        }

        if (glext.ANGLE_texture_compression_dxt1 || glext.EXT_texture_compression_dxt1)
        {
            core.texture_compression_dxt1 = true;
        }

        if (glext.ANGLE_texture_compression_dxt3)
        {
            core.texture_compression_dxt1 = true;
            core.texture_compression_dxt3 = true;
        }

        if (glext.ANGLE_texture_compression_dxt5)
        {
            core.texture_compression_dxt1 = true;
            core.texture_compression_dxt5 = true;
        }

        if (glext.EXT_texture_compression_rgtc || glext.ARB_texture_compression_rgtc)
        {
            core.texture_compression_rgtc = true;
        }

        if (glext.EXT_texture_rg || glext.ARB_texture_rg)
        {
            core.texture_rg = true;
        }

        if (glext.EXT_texture_snorm)
        {
            core.texture_snorm = true;
        }

		core.texture_compression_latc = glext.NV_texture_compression_latc || glext.EXT_texture_compression_latc;
        core.texture_compression_atc = glext.AMD_compressed_ATC_texture || glext.ATI_texture_compression_atitc;

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

    bool Context::isExtension(const std::string& name) const
    {
        return m_extensions.find(name) != m_extensions.end();
    }

    bool Context::isGLES() const
    {
        return false; // TODO: add GLES support
    }

    int Context::getVersion() const
    {
        int major;
        int minor;
        const char* str = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        std::sscanf(str, "%d.%d", &major, &minor);
        int version = major * 100 + minor * 10;
        return version;
    }

    bool isCompressedTextureSupported(TextureCompression compression)
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
                supported = glext.AMD_compressed_3DC_texture;
                break;

			case TextureCompression::LATC1_LUMINANCE:
			case TextureCompression::LATC1_SIGNED_LUMINANCE:
			case TextureCompression::LATC2_LUMINANCE_ALPHA:
			case TextureCompression::LATC2_SIGNED_LUMINANCE_ALPHA:
				supported = glext.EXT_texture_compression_latc;
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

            case TextureCompression::PVRTC_RGB_4BPPV1:
            case TextureCompression::PVRTC_RGB_2BPPV1:
            case TextureCompression::PVRTC_RGBA_4BPPV1:
            case TextureCompression::PVRTC_RGBA_2BPPV1:
                supported = glext.IMG_texture_compression_pvrtc;
                break;

            case TextureCompression::PVRTC_RGBA_2BPPV2:
            case TextureCompression::PVRTC_RGBA_4BPPV2:
                supported = glext.IMG_texture_compression_pvrtc2;
                break;

            case TextureCompression::PVRTC_SRGB_2BPPV1:
            case TextureCompression::PVRTC_SRGB_4BPPV1:
            case TextureCompression::PVRTC_SRGB_ALPHA_2BPPV1:
            case TextureCompression::PVRTC_SRGB_ALPHA_4BPPV1:
                supported = glext.EXT_pvrtc_sRGB;
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
                supported = glext.OES_compressed_ETC1_RGB8_texture;
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
                supported = glext.KHR_texture_compression_astc_hdr;
                // TODO: || glext.KHR_texture_compression_astc_ldr;
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

    const InternalFormat* getInternalFormat(GLenum internalFormat)
    {
        for (auto& node : g_formatTable)
        {
            if (node.internalFormat == internalFormat)
                return &node;
        }

        return NULL;
    }

} // namespace opengl
} // namespace mango
