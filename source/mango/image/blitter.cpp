/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/system.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/half.hpp>
#include <mango/image/blitter.hpp>
#include <mango/simd/simd.hpp>
#include <mango/math/vector.hpp>
#include <mango/math/srgb.hpp>

namespace
{
    using namespace mango;

    // ----------------------------------------------------------------------------
    // prototype
    // ----------------------------------------------------------------------------

#if 0

    NONE   = 0, // --
    SRGB   = 1, // 8
    UNORM  = 2, // 8, 16, 24, 32
    SNORM  = 3, // 8, 16, 24, 32
    UINT   = 4, // 8, 16, 24, 32
    SINT   = 5, // 8, 16, 24, 32
    HALF   = 6, // 16
    FLOAT  = 7  // 32

#endif

    struct MaskProcessor
    {
        uint32 m_mask[4];
        double m_scale[4];
        double m_inv_scale[4];
        double m_alpha;

        MaskProcessor(const Format& format)
        {
            for (int i = 0; i < 4; ++i)
            {
                m_mask[i] = format.mask(i);
                if (m_mask[i])
                {
                    m_scale[i] = m_mask[i];
                    m_inv_scale[i] = 1.0 / m_mask[i];
                }
                else
                {
                    m_scale[i] = 0.0;
                    m_inv_scale[i] = 0.0;
                }
            }

            m_alpha = m_mask[3] ? 0.0 : 1.0;
        }

        double4 unpack(uint32 sample) const
        {
            double4 v;
            v[0] = u32_to_f64(sample & m_mask[0]) * m_inv_scale[0];
            v[1] = u32_to_f64(sample & m_mask[1]) * m_inv_scale[1];
            v[2] = u32_to_f64(sample & m_mask[2]) * m_inv_scale[2];
            v[3] = u32_to_f64(sample & m_mask[3]) * m_inv_scale[3];
            v[3] += m_alpha;
            return v;
        }

        uint32 pack(const double4& value) const
        {
            uint32 s = 0;
            s |= f64_to_u32(value[0] * m_scale[0]) & m_mask[0];
            s |= f64_to_u32(value[1] * m_scale[1]) & m_mask[1];
            s |= f64_to_u32(value[2] * m_scale[2]) & m_mask[2];
            s |= f64_to_u32(value[3] * m_scale[3]) & m_mask[3];
            return s;
        }
    };

    // ----------------------------------------------------------------------------
    // abstract
    // ----------------------------------------------------------------------------

    /*

    The Blitter interface supports cross conversion between any pixel format that can
    be presented with the Format interface. The main use case for the code is asset loading
    pipeline and image decoding back-end.

    The Format can store either luminance or color information with optional alpha component.
    More exotic formats such as compressed surfaces, YUV and others will use "packed pixel" or
    planar storage extensions and are not covered by the Blitter conversion.

    The number of supported data types is currently limited to number of legacy formats:

    - Unsigned Normalized Integer (8, 16, 24 and 32 bit)
    - Floating Point (16 and 32 bit)

    Signed normalized and non-normalized integer formats are not supported. The floatToInt
    conversions will clamp the values to normalized range so there will be significant color
    loss. Such conversions are supported for symmetry only, application which knows the mapping
    of HDR to LDR should do the tone mapping using more advanced algorithm.

    The design is a trade-off between code size and performance. The Optimum solution would be to
    generate the conversion code using JIT techniques. LLVM offers a great infrastructure for this.
    The generated conversion functions can also have more state to affect the conversion so it
    is practical to support wider range of formats than these basic facilities we offer here.

    TODO:
    - blitter can handle all UNORM conversions
    - other types will be handled by generic/slow path
    - generic/slow path will have optional LLVM JIT optimizer
    - palette to rgba conversions
    - ui16 and ui32 component formats
    - 1, 2 and 4 bit packed luminance formats (alignment to at least 8 bits)

    */

    // ----------------------------------------------------------------------------
    // macros
    // ----------------------------------------------------------------------------

    /*

    The MODEMASK system below requires little explanation. The pixelformat conversion
    code uses a hash value to classify the different conversion routines. The MAKE_MODEMASK
    macro computes the hash using # of bits for color components. The UNORM formats use bits
    from 1 to 32. Thus, the FP16 and FP32 are assigned synthetic bit values of 40 and 48.

    The hash is computed using multiplies of 8 bits and begins from zero, resulting in hash codes
    to be computed according to the following table:

    ------------------------------------
    Hash     Format
    ------------------------------------
     0       8 bit unorm colors
     1       16 bit unorm colors
     2       24 bit unorm colors
     3       32 bit unorm colors
     4       16 bit half components
     5       32 bit float components
     6       16 bit unorm components
     7       32 bit unorm components

    Colors store all components in the specified number of bits, for example, 16 bit unorm color will pack
    the red, green, blue and alpha into 16 bit unsigned integer. The component formats use exactly specified
    number of bits per color component. This arrangement allows efficient memory access patterns for reading
    and writing data in the conversion loops.

    The hash allows efficient conversion function lookup. All possible  conversions are not yet supported. :_(

    */

	// TODO: support FP64 in the blitter

#define BITS_FP16 40  /* (32 + 8), half is at index 4    */
#define BITS_FP32 48  /* (32 + 16), float is at index 5  */
#define BITS_UI16  56 /* (32 + 24), uint16 is at index 6 */
#define BITS_UI32  64 /* (32 + 32), uint32 is at index 7 */
#define MAKE_MODEMASK(destBits, sourceBits) \
    ((((destBits - 8) / 8) * 8) + ((sourceBits - 8) / 8))

    int modeBits(const Format& format)
    {
        int bits = 0;

        switch (format.type())
        {
			case Format::UNORM:
				bits = format.bits();
				break;
		
			case Format::FP16:
				bits = BITS_FP16;
				break;

			case Format::FP32:
				bits = BITS_FP32;
				break;

			case Format::FP64:
				// TODO: support FP64
				break;

            default:
                break;
        }

        return bits;
    }

    template <typename FloatType>
    inline uint32 floatToByte(FloatType sample)
    {
        float v = float(sample);
        v = clamp(v, 0.0f, 1.0f);
        return uint32(v * 255.0f + 0.5f);
    }

    inline uint32 packFloat(uint32 mask, float v)
    {
        v = clamp(v, 0.0f, 1.0f);
        const uint32 lsb = mask ^ (mask - 1);
        const float bias = lsb * 0.5f; // The rounding bias should be precomputed
        return uint32(v * mask + bias) & mask;
    }

    // ----------------------------------------------------------------------------
    // debug print
    // ----------------------------------------------------------------------------
#if 0
    void print(const Blitter& blitter)
    {
        printf("Blitter \n");
        printf("  components: %d\n", blitter.components);
        for (int i = 0; i < blitter.components; ++i)
        {
            printf("    srcMask: 0x%x\n", blitter.component[i].srcMask);
            printf("    destMask: 0x%x\n", blitter.component[i].destMask);
            printf("    scale: %f\n", blitter.component[i].scale);
            printf("    constant: %f\n", blitter.component[i].constant);
            printf("    offset: %d\n", blitter.component[i].offset);
        }
        printf("  sampleSize: %d\n", blitter.sampleSize);
        printf("  initMask: 0x%x\n", blitter.initMask);
        printf("  copyMask: 0x%x\n", blitter.copyMask);
    }
#endif

    // ----------------------------------------------------------------------------
    // conversion templates
    // ----------------------------------------------------------------------------

    // unorm <- unorm

    template <typename DestType, typename SourceType>
    void convert_template_unorm_unorm_fpu(const Blitter& blitter, const BlitRect& rect)
    {
        uint8* source = rect.srcImage;
        uint8* dest = rect.destImage;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                uint32 s = src[x];
                uint32 v = blitter.initMask | (s & blitter.copyMask);
                switch (blitter.components)
                {
                    case 4: v |= blitter.component[3].computePack(s);
                    case 3: v |= blitter.component[2].computePack(s);
                    case 2: v |= blitter.component[1].computePack(s);
                    case 1: v |= blitter.component[0].computePack(s);
                }
                dst[x] = DestType(v);
            }

            source += rect.srcStride;
            dest += rect.destStride;
        }
    }

    // unorm <- fp

    template <typename DestType, typename SourceType>
    void convert_template_unorm_fp_fpu(const Blitter& blitter, const BlitRect& rect)
    {
        uint8* source = rect.srcImage;
        uint8* dest = rect.destImage;

        const Format& sf = blitter.srcFormat;
        const Format& df = blitter.destFormat;

        uint32 mask[4];
        int offset[4];
        int components = 0;

        for (int i = 0; i < 4; ++i)
        {
            if (df.size(i) && sf.size(i))
            {
                mask[components] = df.mask(i);
                offset[components] = sf.offset(i) / (sizeof(SourceType) * 8);
                ++components;
            }
        }

        // default alpha is 1.0 (if destination does not have alpha the mask is 0)
        uint32 alphaMask = df.mask(3);

        // if source format has alpha channel use computed alpha instead of the default
        if (sf.size(3) > 0)
            alphaMask = 0;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                uint32 v = alphaMask;

                switch (components)
                {
                    case 4: v |= packFloat(mask[3], src[offset[3]]);
                    case 3: v |= packFloat(mask[2], src[offset[2]]);
                    case 2: v |= packFloat(mask[1], src[offset[1]]);
                    case 1: v |= packFloat(mask[0], src[offset[0]]);
                }

                src += components;
                dst[x] = DestType(v);
            }

            source += rect.srcStride;
            dest += rect.destStride;
        }
    }

    // fp <- unorm

    template <typename DestType, typename SourceType>
    void convert_template_fp_unorm_fpu(const Blitter& blitter, const BlitRect& rect)
    {
        MANGO_UNREFERENCED_PARAMETER(blitter);

        uint8* source = rect.srcImage;
        uint8* dest = rect.destImage;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                // TODO
                MANGO_UNREFERENCED_PARAMETER(src);
                MANGO_UNREFERENCED_PARAMETER(dst);
            }

            source += rect.srcStride;
            dest += rect.destStride;
        }
    }

    // fp <- fp

    template <typename DestType, typename SourceType>
    void convert_template_fp_fp_fpu(const Blitter& blitter, const BlitRect& rect)
    {
        uint8* source = rect.srcImage;
        uint8* dest = rect.destImage;

        SourceType constant[4];
        const SourceType* input[4];
        int stride[4];

        const int components = std::min(4, blitter.components);

        // TODO: initialize blitter variables; they are not configured for FP formats in the code yet!
        for (int i = 0; i < components; ++i)
        {
            int offset = blitter.component[i].offset;
            if (offset < 0)
            {
                constant[i] = SourceType(blitter.component[i].constant);
                input[i] = &constant[i];
                stride[i] = 0;
            }
            else
            {
                stride[i] = blitter.sampleSize;
            }
        }

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int i = 0; i < components; ++i)
            {
                int offset = blitter.component[i].offset;
                if (offset >= 0)
                {
                    input[i] = src + offset;
                }
            }

            for (int x = 0; x < rect.width; ++x)
            {
                switch (components)
                {
                    case 4: dst[3] = *input[3]; input[3] += stride[3];
                    case 3: dst[2] = *input[2]; input[2] += stride[2];
                    case 2: dst[1] = *input[1]; input[1] += stride[1];
                    case 1: dst[0] = *input[0]; input[0] += stride[0];
                }
                dst += components;
            }

            source += rect.srcStride;
            dest += rect.destStride;
        }
    }

    Blitter::ConvertFunc convert_fpu(int modeMask)
    {
        Blitter::ConvertFunc func = NULL;

        switch (modeMask)
        {
            case MAKE_MODEMASK( 8,  8): func = convert_template_unorm_unorm_fpu<uint8, uint8>; break;
            case MAKE_MODEMASK( 8, 16): func = convert_template_unorm_unorm_fpu<uint8, uint16>; break;
            case MAKE_MODEMASK( 8, 24): func = convert_template_unorm_unorm_fpu<uint8, uint24>; break;
            case MAKE_MODEMASK( 8, 32): func = convert_template_unorm_unorm_fpu<uint8, uint32>; break;
            case MAKE_MODEMASK(16,  8): func = convert_template_unorm_unorm_fpu<uint16, uint8>; break;
            case MAKE_MODEMASK(16, 16): func = convert_template_unorm_unorm_fpu<uint16, uint16>; break;
            case MAKE_MODEMASK(16, 24): func = convert_template_unorm_unorm_fpu<uint16, uint24>; break;
            case MAKE_MODEMASK(16, 32): func = convert_template_unorm_unorm_fpu<uint16, uint32>; break;
            case MAKE_MODEMASK(24,  8): func = convert_template_unorm_unorm_fpu<uint24, uint8>; break;
            case MAKE_MODEMASK(24, 16): func = convert_template_unorm_unorm_fpu<uint24, uint16>; break;
            case MAKE_MODEMASK(24, 24): func = convert_template_unorm_unorm_fpu<uint24, uint24>; break;
            case MAKE_MODEMASK(24, 32): func = convert_template_unorm_unorm_fpu<uint24, uint32>; break;
            case MAKE_MODEMASK(32,  8): func = convert_template_unorm_unorm_fpu<uint32, uint8>; break;
            case MAKE_MODEMASK(32, 16): func = convert_template_unorm_unorm_fpu<uint32, uint16>; break;
            case MAKE_MODEMASK(32, 24): func = convert_template_unorm_unorm_fpu<uint32, uint24>; break;
            case MAKE_MODEMASK(32, 32): func = convert_template_unorm_unorm_fpu<uint32, uint32>; break;
            case MAKE_MODEMASK( 8, BITS_FP16): func = convert_template_unorm_fp_fpu<uint8, half>; break;
            case MAKE_MODEMASK(16, BITS_FP16): func = convert_template_unorm_fp_fpu<uint16, half>; break;
            case MAKE_MODEMASK(24, BITS_FP16): func = convert_template_unorm_fp_fpu<uint24, half>; break;
            case MAKE_MODEMASK(32, BITS_FP16): func = convert_template_unorm_fp_fpu<uint32, half>; break;
            case MAKE_MODEMASK( 8, BITS_FP32): func = convert_template_unorm_fp_fpu<uint8, float>; break;
            case MAKE_MODEMASK(16, BITS_FP32): func = convert_template_unorm_fp_fpu<uint16, float>; break;
            case MAKE_MODEMASK(24, BITS_FP32): func = convert_template_unorm_fp_fpu<uint24, float>; break;
            case MAKE_MODEMASK(32, BITS_FP32): func = convert_template_unorm_fp_fpu<uint32, float>; break;
            case MAKE_MODEMASK(BITS_FP16,  8): func = convert_template_fp_unorm_fpu<half, uint8>; break;
            case MAKE_MODEMASK(BITS_FP16, 16): func = convert_template_fp_unorm_fpu<half, uint16>; break;
            case MAKE_MODEMASK(BITS_FP16, 24): func = convert_template_fp_unorm_fpu<half, uint24>; break;
            case MAKE_MODEMASK(BITS_FP16, 32): func = convert_template_fp_unorm_fpu<half, uint32>; break;
            case MAKE_MODEMASK(BITS_FP32,  8): func = convert_template_fp_unorm_fpu<float, uint8>; break;
            case MAKE_MODEMASK(BITS_FP32, 16): func = convert_template_fp_unorm_fpu<float, uint16>; break;
            case MAKE_MODEMASK(BITS_FP32, 24): func = convert_template_fp_unorm_fpu<float, uint24>; break;
            case MAKE_MODEMASK(BITS_FP32, 32): func = convert_template_fp_unorm_fpu<float, uint32>; break;
            case MAKE_MODEMASK(BITS_FP16, BITS_FP16): func = convert_template_fp_fp_fpu<half, half>; break;
            case MAKE_MODEMASK(BITS_FP16, BITS_FP32): func = convert_template_fp_fp_fpu<half, float>; break;
            case MAKE_MODEMASK(BITS_FP32, BITS_FP16): func = convert_template_fp_fp_fpu<float, half>; break;
            case MAKE_MODEMASK(BITS_FP32, BITS_FP32): func = convert_template_fp_fp_fpu<float, float>; break;
        }

        return func;
    }

#ifdef MANGO_ENABLE_SSE2
    template <typename DestType, typename SourceType>
    void convert_template_sse2(const Blitter& blitter, const BlitRect& rect)
    {
        uint8* source = rect.srcImage;
        uint8* dest = rect.destImage;

        __m128 scale = blitter.sseScale;
        __m128i src_mask = blitter.sseSrcMask;
        __m128i dest_mask = blitter.sseDestMask;
        __m128i shift_mask = blitter.sseShiftMask;

        for (int y = 0; y < rect.height; ++y)
        {
            SourceType* src = reinterpret_cast<SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                uint32 s = src[x];
                __m128i r = _mm_set1_epi32(s);
                __m128 m = _mm_cvtepi32_ps(_mm_and_si128(r, src_mask));
                r = _mm_cvtps_epi32(_mm_mul_ps(m, scale));
                r = _mm_and_si128(r, dest_mask);
                r = _mm_add_epi32(r, _mm_and_si128(r, shift_mask));
                r = _mm_or_si128(_mm_shuffle_epi32(r, 0x44), _mm_shuffle_epi32(r, 0xee));
                r = _mm_or_si128(_mm_shuffle_epi32(r, 0x00), _mm_shuffle_epi32(r, 0x55));
                uint32 v = blitter.initMask | _mm_cvtsi128_si32(r);
                dst[x] = DestType(v);
            }

            source += rect.srcStride;
            dest += rect.destStride;
        }
    }

    Blitter::ConvertFunc convert_sse2(int modeMask)
    {
        Blitter::ConvertFunc func = NULL;

        switch (modeMask)
        {
            case MAKE_MODEMASK( 8,  8): func = convert_template_sse2<uint8, uint8>; break;
            case MAKE_MODEMASK( 8, 16): func = convert_template_sse2<uint8, uint16>; break;
            case MAKE_MODEMASK( 8, 24): func = convert_template_sse2<uint8, uint24>; break;
            case MAKE_MODEMASK( 8, 32): func = convert_template_sse2<uint8, uint32>; break;
            case MAKE_MODEMASK(16,  8): func = convert_template_sse2<uint16, uint8>; break;
            case MAKE_MODEMASK(16, 16): func = convert_template_sse2<uint16, uint16>; break;
            case MAKE_MODEMASK(16, 24): func = convert_template_sse2<uint16, uint24>; break;
            case MAKE_MODEMASK(16, 32): func = convert_template_sse2<uint16, uint32>; break;
            case MAKE_MODEMASK(24,  8): func = convert_template_sse2<uint24, uint8>; break;
            case MAKE_MODEMASK(24, 16): func = convert_template_sse2<uint24, uint16>; break;
            case MAKE_MODEMASK(24, 24): func = convert_template_sse2<uint24, uint24>; break;
            case MAKE_MODEMASK(24, 32): func = convert_template_sse2<uint24, uint32>; break;
            case MAKE_MODEMASK(32,  8): func = convert_template_sse2<uint32, uint8>; break;
            case MAKE_MODEMASK(32, 16): func = convert_template_sse2<uint32, uint16>; break;
            case MAKE_MODEMASK(32, 24): func = convert_template_sse2<uint32, uint24>; break;
            case MAKE_MODEMASK(32, 32): func = convert_template_sse2<uint32, uint32>; break;
        }

        return func;
    }
#endif

    void convert_custom(const Blitter& blitter, const BlitRect& rect)
    {
        uint8* src = rect.srcImage;
        uint8* dest = rect.destImage;

        for (int y = 0; y < rect.height; ++y)
        {
            blitter.custom(dest, src, rect.width);
            src += rect.srcStride;
            dest += rect.destStride;
        }
    }

    // ----------------------------------------------------------------------------
    // custom conversion functions
    // ----------------------------------------------------------------------------

#define INIT_POINTERS(DEST, SRC) \
    const SRC* s = reinterpret_cast<const SRC*>(src); \
    DEST* d = reinterpret_cast<DEST*>(dest);

    template <int bytes>
    void blit_memcpy(uint8* dest, const uint8* src, int count)
    {
        std::memcpy(dest, src, count * bytes);
    }

    void blit_bgra8888_from_bgrx8888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint32);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            d[x] = v | 0xff000000;
        }
    }

    void blit_bgra8888_to_and_from_rgba8888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint32);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            d[x] = (v & 0xff00ff00) | (((v << 16) | (v >> 16)) & 0x00ff00ff);
        }
    }

    void blit_bgra8888_from_bgra4444(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            uint32 u = ((v & 0xf000) << 16) | ((v & 0x0f00) << 12) | ((v & 0x00f0) << 8) | ((v & 0x000f) << 4);
            d[x] = u | (u >> 4);
        }
    }

    void blit_bgra8888_from_bgra5551(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            uint32 u = v & 0x8000 ? 0xff000000 : 0;
            u |= ((v & 0x7c00) << 9) | ((v & 0x03e0) << 6) | ((v & 0x001f) << 3);
            d[x] = u | ((u & 0x00e0e0e0) >> 5);
        }
    }

    void blit_bgra8888_from_bgr888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint24);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            d[x] = 0xff000000 | v;
        }
    }

    void blit_rgba8888_from_bgr888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint8, uint8);
        for (int x = 0; x < count; ++x)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            d[3] = 0xff;
            s += 3;
            d += 4;
        }
    }

    void blit_bgra8888_from_bgr565(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            uint32 u = ((v & 0xf800) << 8) | ((v & 0x07e0) << 5) | ((v & 0x001f) << 3);
            u |= ((v & 0xe000) << 3) | ((v & 0x0600) >> 1) | ((v & 0x001c) >> 2);
            d[x] = 0xff000000 | u;
        }
    }

    void blit_bgr888_from_bgra8888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint24, uint32);
        for (int x = 0; x < count; ++x)
        {
            d[x] = s[x];
        }
    }

    void blit_rgb888_from_bgra8888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint8, uint8);
        for (int x = 0; x < count; ++x)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            s += 4;
            d += 3;
        }
    }

    void blit_bgr888_to_and_from_rgb888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint8, uint8);
        for (int x = 0; x < count; ++x)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            s += 3;
            d += 3;
        }
    }

    void blit_bgr565_from_bgra8888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint16, uint32);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            uint32 u = ((v >> 8) & 0xf800) | ((v >> 5) & 0x07e0) | ((v >> 3) & 0x001f);
            d[x] = static_cast<uint16>(u);
        }
    }

    void blit_bgra5551_from_bgra8888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint16, uint32);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            uint32 u = ((v >> 16) & 0x8000) | ((v >> 9) & 0x7c00) | ((v >> 6) & 0x03e0) | ((v >> 3) & 0x001f);
            d[x] = static_cast<uint16>(u);
        }
    }

    void blit_bgra4444_from_bgra8888(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint16, uint32);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            uint32 u = ((v >> 16) & 0xf000) | ((v >> 12) & 0x0f00) | ((v >> 8) & 0x00f0) | ((v >> 4) & 0x000f);
            d[x] = static_cast<uint16>(u);
        }
    }

    void blit_yyy888_from_y8(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint24, uint8);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            d[x] = static_cast<uint24>(v * 65793);
        }
    }

    void blit_yyya8888_from_y8(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint8);
        for (int x = 0; x < count; ++x)
        {
            uint32 v = s[x];
            d[x] = 0xff000000 | v * 65793;
        }
    }

    void blit_rgba8888_from_y16ui(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            const uint32 i = s[x] >> 8;
            d[x] = 0xff000000 | i * 0x010101;
        }
    }

    void blit_bgra8888_from_y16ui(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            const uint32 i = s[x] >> 8;
            d[x] = 0xff000000 | i * 0x010101;
        }
    }

    void blit_rgba8888_from_ya16ui(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint32);
        for (int x = 0; x < count; ++x)
        {
            const uint32 a = s[x] & 0xff000000;
            const uint32 i = (s[x] & 0x0000ff00) >> 8;
            d[x] = a | i * 0x010101;
        }
    }

    void blit_bgra8888_from_ya16ui(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint32);
        for (int x = 0; x < count; ++x)
        {
            const uint32 a = s[x] & 0xff000000;
            const uint32 i = (s[x] & 0x0000ff00) >> 8;
            d[x] = a | i * 0x010101;
        }
    }

    void blit_rgba8888_from_rgb16ui(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            const uint32 r = s[0];
            const uint32 g = s[1] & 0x0000ff00;
            const uint32 b = s[2] & 0x0000ff00;
            d[x] = 0xff000000 | (b << 8) | g | (r >> 8);
            s += 3;
        }
    }

    void blit_bgra8888_from_rgb16ui(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            const uint32 r = s[0] & 0x0000ff00;
            const uint32 g = s[1] & 0x0000ff00;
            const uint32 b = s[2];
            d[x] = 0xff000000 | (r << 8) | g | (b >> 8);
            s += 3;
        }
    }

    void blit_rgba8888_from_rgba16ui(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            const uint32 r = s[0];
            const uint32 g = s[1] & 0x0000ff00;
            const uint32 b = s[2] & 0x0000ff00;
            const uint32 a = s[3] & 0x0000ff00;
            d[x] = (a << 16) | (b << 8) | g | (r >> 8);
            s += 4;
        }
    }

    void blit_bgra8888_from_rgba16ui(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, uint16);
        for (int x = 0; x < count; ++x)
        {
            const uint32 r = s[0] & 0x0000ff00;
            const uint32 g = s[1] & 0x0000ff00;
            const uint32 b = s[2];
            const uint32 a = s[3] & 0x0000ff00;
            d[x] = (a << 16) | (r << 8) | g | (b >> 8);
            s += 4;
        }
    }

    void blit_rgba8888_from_rgba16f(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, simd::float16x4);
        for (int x = 0; x < count; ++x)
        {
            simd::float32x4 f = simd::convert<simd::float32x4>(s[x]);
            f = simd::clamp(f, 0.0f, 1.0f);
            f = simd::mul(f, 255.0f);
            f = simd::add(f, 0.5f);
            simd::int32x4 i = simd::convert<simd::int32x4>(f);
            d[x] = simd::pack(i);
        }
    }

    void blit_bgra8888_from_rgba16f(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, simd::float16x4);
        for (int x = 0; x < count; ++x)
        {
            simd::float32x4 f = simd::convert<simd::float32x4>(s[x]);
            f = simd::shuffle<2, 1, 0, 3>(f);
            f = simd::clamp(f, 0.0f, 1.0f);
            f = simd::mul(f, 255.0f);
            f = simd::add(f, 0.5f);
            simd::int32x4 i = simd::convert<simd::int32x4>(f);
            d[x] = simd::pack(i);
        }
    }

    void blit_rgba8888_from_rgba32f(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, simd::float32x4);
        for (int x = 0; x < count; ++x)
        {
            simd::float32x4 f = s[x];
            f = simd::clamp(f, 0.0f, 1.0f);
            f = simd::mul(f, 255.0f);
            f = simd::add(f, 0.5f);
            simd::int32x4 i = simd::convert<simd::int32x4>(f);
            d[x] = simd::pack(i);
        }
    }

    void blit_bgra8888_from_rgba32f(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(uint32, simd::float32x4);
        for (int x = 0; x < count; ++x)
        {
            simd::float32x4 f = s[x];
            f = simd::shuffle<2, 1, 0, 3>(f);
            f = simd::clamp(f, 0.0f, 1.0f);
            f = simd::mul(f, 255.0f);
            f = simd::add(f, 0.5f);
            simd::int32x4 i = simd::convert<simd::int32x4>(f);
            d[x] = simd::pack(i);
        }
    }

    void blit_rgba16f_from_rgba32f(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(simd::float16x4, simd::float32x4);
        for (int x = 0; x < count; ++x)
        {
            d[x] = simd::convert<simd::float16x4>(s[x]);
        }
    }

    void blit_rgba32f_from_rgba16f(uint8* dest, const uint8* src, int count)
    {
        INIT_POINTERS(simd::float32x4, simd::float16x4);
        for (int x = 0; x < count; ++x)
        {
            d[x] = simd::convert<simd::float32x4>(s[x]);
        }
    }

    // ----------------------------------------------------------------------------
    // custom conversion function lookup
    // ----------------------------------------------------------------------------

    // list of custom conversion functions
    struct
    {
        Format dest;
        Format source;
        uint32 requireCpuFeature;
        Blitter::FastFunc func;
    }
    const g_custom_func_table[] =
    {
        { FORMAT_B8G8R8X8, FORMAT_B8G8R8A8,   0, blit_memcpy<4> },
        { FORMAT_R8G8B8X8, FORMAT_R8G8B8A8,   0, blit_memcpy<4> },
        { FORMAT_B8G8R8A8, FORMAT_B8G8R8X8,   0, blit_bgra8888_from_bgrx8888 },
        { FORMAT_R8G8B8A8, FORMAT_R8G8B8X8,   0, blit_bgra8888_from_bgrx8888 },
        { FORMAT_B8G8R8X8, FORMAT_R8G8B8X8,   0, blit_bgra8888_to_and_from_rgba8888 },
        { FORMAT_R8G8B8X8, FORMAT_B8G8R8X8,   0, blit_bgra8888_to_and_from_rgba8888 },
        { FORMAT_B8G8R8A8, FORMAT_R8G8B8A8,   0, blit_bgra8888_to_and_from_rgba8888 },
        { FORMAT_R8G8B8A8, FORMAT_B8G8R8A8,   0, blit_bgra8888_to_and_from_rgba8888 },
        { FORMAT_B8G8R8A8, FORMAT_B4G4R4A4,   0, blit_bgra8888_from_bgra4444 },
        { FORMAT_B8G8R8A8, FORMAT_B5G5R5A1,   0, blit_bgra8888_from_bgra5551 },
        { FORMAT_B8G8R8A8, FORMAT_B8G8R8,     0, blit_bgra8888_from_bgr888 },
        { FORMAT_B8G8R8A8, FORMAT_R8G8B8,     0, blit_rgba8888_from_bgr888 },
        { FORMAT_B8G8R8A8, FORMAT_B5G6R5,     0, blit_bgra8888_from_bgr565 },
        { FORMAT_B8G8R8,   FORMAT_B8G8R8A8,   0, blit_bgr888_from_bgra8888 },
        { FORMAT_R8G8B8,   FORMAT_B8G8R8A8,   0, blit_rgb888_from_bgra8888 },
        { FORMAT_R8G8B8,   FORMAT_B8G8R8,     0, blit_bgr888_to_and_from_rgb888 },//
        { FORMAT_B8G8R8,   FORMAT_R8G8B8,     0, blit_bgr888_to_and_from_rgb888 },
        { FORMAT_B5G6R5,   FORMAT_B8G8R8A8,   0, blit_bgr565_from_bgra8888 },
        { FORMAT_B5G5R5A1, FORMAT_B8G8R8A8,   0, blit_bgra5551_from_bgra8888 },
        { FORMAT_B4G4R4A4, FORMAT_B8G8R8A8,   0, blit_bgra4444_from_bgra8888 },
        { FORMAT_B8G8R8,   FORMAT_L8,         0, blit_yyy888_from_y8 },
        { FORMAT_R8G8B8,   FORMAT_L8,         0, blit_yyy888_from_y8 },
        { FORMAT_B8G8R8A8, FORMAT_L8,         0, blit_yyya8888_from_y8 },
        { FORMAT_R8G8B8A8, FORMAT_L8,         0, blit_yyya8888_from_y8 },
        { FORMAT_R8G8B8A8, FORMAT_L16,        0, blit_rgba8888_from_y16ui },
        { FORMAT_B8G8R8A8, FORMAT_L16,        0, blit_bgra8888_from_y16ui },
        { FORMAT_R8G8B8A8, FORMAT_L16A16,     0, blit_rgba8888_from_ya16ui },
        { FORMAT_B8G8R8A8, FORMAT_L16A16,     0, blit_bgra8888_from_ya16ui },
        { FORMAT_R8G8B8A8, FORMAT_RGB16,      0, blit_rgba8888_from_rgb16ui },
        { FORMAT_B8G8R8A8, FORMAT_RGB16,      0, blit_bgra8888_from_rgb16ui },
        { FORMAT_R8G8B8A8, FORMAT_RGBA16,     0, blit_rgba8888_from_rgba16ui },
        { FORMAT_B8G8R8A8, FORMAT_RGBA16,     0, blit_bgra8888_from_rgba16ui },
        { FORMAT_R8G8B8A8, FORMAT_RGBA16F,    0, blit_rgba8888_from_rgba16f },
        { FORMAT_B8G8R8A8, FORMAT_RGBA16F,    0, blit_bgra8888_from_rgba16f },
        { FORMAT_R8G8B8A8, FORMAT_RGBA32F,    0, blit_rgba8888_from_rgba32f },
        { FORMAT_B8G8R8A8, FORMAT_RGBA32F,    0, blit_bgra8888_from_rgba32f },
        { FORMAT_RGBA16F,  FORMAT_RGBA32F,    0, blit_rgba16f_from_rgba32f },
        { FORMAT_RGBA32F,  FORMAT_RGBA16F,    0, blit_rgba32f_from_rgba16f },
    };

    typedef std::map< std::pair<Format, Format>, Blitter::FastFunc > FastConversionMap;

    // initialize map of custom conversion functions
    FastConversionMap g_custom_func_map = [] {
        FastConversionMap map;

        uint64 cpuFlags = getCPUFlags();

        const int table_size = sizeof(g_custom_func_table) / sizeof(g_custom_func_table[0]);

        for (int i = 0; i < table_size; ++i)
        {
            const auto& node = g_custom_func_table[i];

            if (!node.requireCpuFeature || (cpuFlags & node.requireCpuFeature) != 0)
            {
                map[std::make_pair(node.dest, node.source)] = node.func;
            }
        }

        return map;
    } ();

    Blitter::FastFunc find_custom_blitter(const Format& dest, const Format& source)
    {
        Blitter::FastFunc func = NULL; // default: no conversion function

        if (dest == source)
        {
            // no conversion required
            switch (dest.bytes())
            {
                case 1: func = blit_memcpy<1>; break;
                case 2: func = blit_memcpy<2>; break;
                case 3: func = blit_memcpy<3>; break;
                case 4: func = blit_memcpy<4>; break;
                case 6: func = blit_memcpy<6>; break;
                case 8: func = blit_memcpy<8>; break;
                case 12: func = blit_memcpy<12>; break;
                case 16: func = blit_memcpy<16>; break;
            }
        }
        else
        {
            // find custom conversion function
            auto i = g_custom_func_map.find(std::make_pair(dest, source));
            if (i != g_custom_func_map.end())
            {
                func = i->second;
            }
        }

        return func;
    }

} // namespace

namespace mango
{

    // ----------------------------------------------------------------------------
    // Blitter
    // ----------------------------------------------------------------------------

    Blitter::Blitter(const Format& dest, const Format& source)
    : srcFormat(source), destFormat(dest), custom(NULL), convertFunc(NULL)
    {
        custom = find_custom_blitter(dest, source);
        if (custom)
        {
            // found custom blitter
            convertFunc = convert_custom;
            return;
        }

        components = 0;
        initMask = 0;
        copyMask = 0;

        sampleSize = 0; // TODO

        uint64 cpuFlags = getCPUFlags();
        bool sse2 = (cpuFlags & CPU_SSE2) != 0;

        for (int i = 0; i < 4; ++i)
        {
            uint32 src_mask = source.mask(i);
            uint32 dest_mask = dest.mask(i);

            if (dest_mask)
            {
                if (src_mask != dest_mask)
                {
                    if (src_mask)
                    {
                        // isolate least significant bit of mask; this is used for correct rounding
                        const uint32 lsb = dest_mask ^ (dest_mask - 1);

                        // source and destination are different: add channel to component array
                        component[components].srcMask = src_mask;
                        component[components].destMask = dest_mask;
                        component[components].scale = float(dest_mask) / float(src_mask);
                        component[components].bias = lsb * 0.5f;
                        ++components;
                    }
                    else
                    {
                        // no source channel
                        const bool is_alpha_channel = (i == 3);
                        if (is_alpha_channel)
                        {
                            // alpha defaults to 1.0 (all bits of destination are set)
                            initMask |= dest_mask;
                        }
                        else
                        {
                            // color defaults to 0.0
                        }
                    }
                }
                else
                {
                    // identical masks: pass-through w/o expensive processing
                    copyMask |= src_mask;
                }
            }
        }

        if (components < 2)
        {
            // the fpu loop is faster when we process only single channel
            // the fpu can handle mask 0xffffffff where SSE will corrupt LSB when using 32 bit mask
            sse2 = false; // force fpu conversion
        }

        //float constant; // TODO: configure
        //int offset; // TODO: configure

        // select innerloop
        int destBits = modeBits(dest);
        int sourceBits = modeBits(source);
        int modeMask = MAKE_MODEMASK(destBits, sourceBits);

        convertFunc = convert_fpu(modeMask);

#ifdef MANGO_ENABLE_SSE2
        if (sse2)
        {
            // select innerloop
            ConvertFunc func = convert_sse2(modeMask);

            if (func)
            {
                convertFunc = func;

                initMask = 0;

                uint32 shift_mask[4] = { 0, 0, 0, 0 };
                uint32 dest_mask[4];
                uint32 src_mask[4];
                float scale[4];

                for (int i = 0; i < 4; ++i)
                {
                    src_mask[i] = source.mask(i);
                    dest_mask[i] = dest.mask(i);
                    scale[i] = 0;

                    if (dest_mask[i])
                    {
                        if (src_mask[i])
                        {
                            scale[i] = float(dest_mask[i]) / float(src_mask[i]);
                        }
                        else if (i == 3)
                        {
                            initMask |= dest_mask[i];
                        }
                    }

                    // SSE floatToInt conversion is signed
                    if (dest_mask[i] & 0x80000000)
                    {
                        dest_mask[i] >>= 1;
                        shift_mask[i] = dest_mask[i];
                        scale[i] *= 0.5f;
                    }
                }

                // convert components into SSE registers
                sseScale = _mm_set_ps(scale[0], scale[1], scale[2], scale[3]);
                sseSrcMask = _mm_set_epi32(src_mask[0], src_mask[1], src_mask[2], src_mask[3]);
                sseDestMask = _mm_set_epi32(dest_mask[0], dest_mask[1], dest_mask[2], dest_mask[3]);
                sseShiftMask = _mm_set_epi32(shift_mask[0], shift_mask[1], shift_mask[2], shift_mask[3]);
            }
        }
#else
        MANGO_UNREFERENCED_PARAMETER(sse2);
#endif
    }

    Blitter::~Blitter()
    {
    }

} // namespace mango
