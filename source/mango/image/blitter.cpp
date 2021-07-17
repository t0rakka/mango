/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/system.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/half.hpp>
#include <mango/image/blitter.hpp>
#include <mango/math/vector.hpp>
#include <mango/math/srgb.hpp>

namespace
{
    using namespace mango;
    using namespace mango::math;
    using namespace mango::image;

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
     6       64 bit double components
     7       16 bit unorm components
     8       32 bit unorm components

    Colors store all components in the specified number of bits, for example, 16 bit unorm color will pack
    the red, green, blue and alpha into 16 bit unsigned integer. The component formats use exactly specified
    number of bits per color component. This arrangement allows efficient memory access patterns for reading
    and writing data in the conversion loops.

    The hash allows efficient conversion function lookup. All possible  conversions are not yet supported. :_(

    */

    constexpr u32 BITS_FP16 = (8 * 5);
    constexpr u32 BITS_FP32 = (8 * 6);
    constexpr u32 BITS_FP64 = (8 * 7);
    //constexpr u32 BITS_UI16 = (8 * 8);
    //constexpr u32 BITS_UI32 = (8 * 9);

    constexpr u32 MAKE_MODEMASK(u32 destBits, u32 sourceBits)
    {
        u32 destIndex = (destBits / 8) - 1;
        u32 sourceIndex = (sourceBits / 8) - 1;
        return (destIndex << 4) | sourceIndex;
    }

    int modeBits(const Format& format)
    {
        int bits = 0;

        switch (format.type)
        {
			case Format::UNORM:
				bits = format.bits; // 8, 16, 24, 32
				break;

			case Format::FLOAT16:
				bits = BITS_FP16;
				break;

			case Format::FLOAT32:
				bits = BITS_FP32;
				break;

			case Format::FLOAT64:
                bits = BITS_FP64;
				break;

            default:
                break;
        }

        return bits;
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
        u8* source = rect.src.address;
        u8* dest = rect.dest.address;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                u32 s = src[x];
                u32 v = blitter.initMask | (s & blitter.copyMask);
                switch (blitter.components)
                {
                    case 4:
                        v |= blitter.component[3].computePack(s);
                        // fall-through
                    case 3:
                        v |= blitter.component[2].computePack(s);
                        // fall-through
                    case 2:
                        v |= blitter.component[1].computePack(s);
                        // fall-through
                    case 1:
                        v |= blitter.component[0].computePack(s);
                        // fall-through
                }
                dst[x] = DestType(v);
            }

            source += rect.src.stride;
            dest += rect.dest.stride;
        }
    }

    // unorm <- fp

    template <typename DestType, typename SourceType>
    void convert_template_unorm_fp_fpu(const Blitter& blitter, const BlitRect& rect)
    {
        u8* source = rect.src.address;
        u8* dest = rect.dest.address;

        const Format& sf = blitter.srcFormat;
        const Format& df = blitter.destFormat;

        u32 mask[4];
        int offset[4];
        int components = 0;

        for (int i = 0; i < 4; ++i)
        {
            if (df.size[i] && sf.size[i])
            {
                mask[components] = df.mask(i);
                offset[components] = sf.offset[i] / (sizeof(SourceType) * 8);
                ++components;
            }
        }

        // default alpha is 1.0 (if destination does not have alpha the mask is 0)
        u32 alphaMask = df.mask(3);

        // if source format has alpha channel use computed alpha instead of the default
        if (sf.isAlpha())
            alphaMask = 0;

        const float scale0 = float(mask[0]);
        const float scale1 = float(mask[1]);
        const float scale2 = float(mask[2]);
        const float scale3 = float(mask[3]);
        const float bias0 = float(mask[0] ^ (mask[0] - 1)) * 0.5f; // least-significant-bit * 0.5
        const float bias1 = float(mask[1] ^ (mask[1] - 1)) * 0.5f;
        const float bias2 = float(mask[2] ^ (mask[2] - 1)) * 0.5f;
        const float bias3 = float(mask[3] ^ (mask[3] - 1)) * 0.5f;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                u32 v = alphaMask;

                switch (components)
                {
                    case 4:
                        v |= u32(clamp(float(src[offset[3]]), 0.0f, 1.0f) * scale3 + bias3) & mask[3];
                        // fall-through
                    case 3:
                        v |= u32(clamp(float(src[offset[2]]), 0.0f, 1.0f) * scale2 + bias2) & mask[2];
                        // fall-through
                    case 2:
                        v |= u32(clamp(float(src[offset[1]]), 0.0f, 1.0f) * scale1 + bias1) & mask[1];
                        // fall-through
                    case 1:
                        v |= u32(clamp(float(src[offset[0]]), 0.0f, 1.0f) * scale0 + bias0) & mask[0];
                        // fall-through
                }

                src += components;
                dst[x] = DestType(v);
            }

            source += rect.src.stride;
            dest += rect.dest.stride;
        }
    }

    // fp <- unorm

    template <typename DestType, typename SourceType>
    void convert_template_fp_unorm_fpu(const Blitter& blitter, const BlitRect& rect)
    {
        MANGO_UNREFERENCED(blitter);

        u8* source = rect.src.address;
        u8* dest = rect.dest.address;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                // TODO
                MANGO_UNREFERENCED(src);
                MANGO_UNREFERENCED(dst);
            }

            source += rect.src.stride;
            dest += rect.dest.stride;
        }
    }

    // fp <- fp

    template <typename DestType, typename SourceType>
    void convert_template_fp_fp_fpu(const Blitter& blitter, const BlitRect& rect)
    {
        u8* source = rect.src.address;
        u8* dest = rect.dest.address;

        SourceType constant[4];
        const SourceType* input[4];
        int stride[4];

        const int components = std::min(4, blitter.components);

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

#if 0
        printf("components: %d\n", components);
        for (int i = 0; i < components; ++i)
        {
            printf("  component[%d]:  .offset: %d,  .stride: %d \n",
                i,
                blitter.component[i].offset,
                stride[i]);
        }
#endif

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
                    case 4:
                        dst[3] = DestType(*input[3]);
                        input[3] += stride[3];
                        // fall-through
                    case 3:
                        dst[2] = DestType(*input[2]);
                        input[2] += stride[2];
                        // fall-through
                    case 2:
                        dst[1] = DestType(*input[1]);
                        input[1] += stride[1];
                        // fall-through
                    case 1:
                        dst[0] = DestType(*input[0]);
                        input[0] += stride[0];
                        // fall-through
                }
                dst += components;
            }

            source += rect.src.stride;
            dest += rect.dest.stride;
        }
    }

    Blitter::ConvertFunc convert_fpu(int modeMask)
    {
        Blitter::ConvertFunc func = nullptr;

        switch (modeMask)
        {
            case MAKE_MODEMASK( 8,  8): func = convert_template_unorm_unorm_fpu<u8, u8>; break;
            case MAKE_MODEMASK( 8, 16): func = convert_template_unorm_unorm_fpu<u8, u16>; break;
            case MAKE_MODEMASK( 8, 24): func = convert_template_unorm_unorm_fpu<u8, u24>; break;
            case MAKE_MODEMASK( 8, 32): func = convert_template_unorm_unorm_fpu<u8, u32>; break;
            case MAKE_MODEMASK(16,  8): func = convert_template_unorm_unorm_fpu<u16, u8>; break;
            case MAKE_MODEMASK(16, 16): func = convert_template_unorm_unorm_fpu<u16, u16>; break;
            case MAKE_MODEMASK(16, 24): func = convert_template_unorm_unorm_fpu<u16, u24>; break;
            case MAKE_MODEMASK(16, 32): func = convert_template_unorm_unorm_fpu<u16, u32>; break;
            case MAKE_MODEMASK(24,  8): func = convert_template_unorm_unorm_fpu<u24, u8>; break;
            case MAKE_MODEMASK(24, 16): func = convert_template_unorm_unorm_fpu<u24, u16>; break;
            case MAKE_MODEMASK(24, 24): func = convert_template_unorm_unorm_fpu<u24, u24>; break;
            case MAKE_MODEMASK(24, 32): func = convert_template_unorm_unorm_fpu<u24, u32>; break;
            case MAKE_MODEMASK(32,  8): func = convert_template_unorm_unorm_fpu<u32, u8>; break;
            case MAKE_MODEMASK(32, 16): func = convert_template_unorm_unorm_fpu<u32, u16>; break;
            case MAKE_MODEMASK(32, 24): func = convert_template_unorm_unorm_fpu<u32, u24>; break;
            case MAKE_MODEMASK(32, 32): func = convert_template_unorm_unorm_fpu<u32, u32>; break;
            case MAKE_MODEMASK( 8, BITS_FP16): func = convert_template_unorm_fp_fpu<u8, float16>; break;
            case MAKE_MODEMASK(16, BITS_FP16): func = convert_template_unorm_fp_fpu<u16, float16>; break;
            case MAKE_MODEMASK(24, BITS_FP16): func = convert_template_unorm_fp_fpu<u24, float16>; break;
            case MAKE_MODEMASK(32, BITS_FP16): func = convert_template_unorm_fp_fpu<u32, float16>; break;
            case MAKE_MODEMASK( 8, BITS_FP32): func = convert_template_unorm_fp_fpu<u8, float>; break;
            case MAKE_MODEMASK(16, BITS_FP32): func = convert_template_unorm_fp_fpu<u16, float>; break;
            case MAKE_MODEMASK(24, BITS_FP32): func = convert_template_unorm_fp_fpu<u24, float>; break;
            case MAKE_MODEMASK(32, BITS_FP32): func = convert_template_unorm_fp_fpu<u32, float>; break;
            case MAKE_MODEMASK(BITS_FP16,  8): func = convert_template_fp_unorm_fpu<float16, u8>; break;
            case MAKE_MODEMASK(BITS_FP16, 16): func = convert_template_fp_unorm_fpu<float16, u16>; break;
            case MAKE_MODEMASK(BITS_FP16, 24): func = convert_template_fp_unorm_fpu<float16, u24>; break;
            case MAKE_MODEMASK(BITS_FP16, 32): func = convert_template_fp_unorm_fpu<float16, u32>; break;
            case MAKE_MODEMASK(BITS_FP32,  8): func = convert_template_fp_unorm_fpu<float, u8>; break;
            case MAKE_MODEMASK(BITS_FP32, 16): func = convert_template_fp_unorm_fpu<float, u16>; break;
            case MAKE_MODEMASK(BITS_FP32, 24): func = convert_template_fp_unorm_fpu<float, u24>; break;
            case MAKE_MODEMASK(BITS_FP32, 32): func = convert_template_fp_unorm_fpu<float, u32>; break;
            case MAKE_MODEMASK(BITS_FP16, BITS_FP16): func = convert_template_fp_fp_fpu<float16, float16>; break;
            case MAKE_MODEMASK(BITS_FP16, BITS_FP32): func = convert_template_fp_fp_fpu<float16, float32>; break;
            case MAKE_MODEMASK(BITS_FP16, BITS_FP64): func = convert_template_fp_fp_fpu<float16, float64>; break;
            case MAKE_MODEMASK(BITS_FP32, BITS_FP16): func = convert_template_fp_fp_fpu<float32, float16>; break;
            case MAKE_MODEMASK(BITS_FP32, BITS_FP32): func = convert_template_fp_fp_fpu<float32, float32>; break;
            case MAKE_MODEMASK(BITS_FP32, BITS_FP64): func = convert_template_fp_fp_fpu<float32, float64>; break;
            case MAKE_MODEMASK(BITS_FP64, BITS_FP16): func = convert_template_fp_fp_fpu<float64, float16>; break;
            case MAKE_MODEMASK(BITS_FP64, BITS_FP32): func = convert_template_fp_fp_fpu<float64, float32>; break;
            case MAKE_MODEMASK(BITS_FP64, BITS_FP64): func = convert_template_fp_fp_fpu<float64, float64>; break;
        }

        return func;
    }

    void convert_custom(const Blitter& blitter, const BlitRect& rect)
    {
        u8* src = rect.src.address;
        u8* dest = rect.dest.address;

        for (int y = 0; y < rect.height; ++y)
        {
            blitter.custom(dest, src, rect.width);
            src += rect.src.stride;
            dest += rect.dest.stride;
        }
    }

    // ----------------------------------------------------------------------------
    // custom conversion functions
    // ----------------------------------------------------------------------------

    template <int bytes>
    void blit_memcpy(u8* dest, const u8* src, int count)
    {
        std::memcpy(dest, src, count * bytes);
    }

    void blit_32bit_generate_alpha(u8* dest, const u8* src, int count)
    {
        u32* d = reinterpret_cast<u32*>(dest);
        const u32* s = reinterpret_cast<const u32*>(src);
        for (int x = 0; x < count; ++x)
        {
            u32 v = s[x];
            d[x] = v | 0xff000000;
        }
    }

    void blit_32bit_swap_rg(u8* dest, const u8* src, int count)
    {
        u32* d = reinterpret_cast<u32*>(dest);
        const u32* s = reinterpret_cast<const u32*>(src);

#if 0
        while (count >= 4)
        {
            uint32x4 color = simd::u32x4_uload(s);
            color = (color & 0xff00ff00) | (color << 16) | ((color >> 16) & 0x00ff);
            simd::u32x4_ustore(d, color);
            s += 4;
            d += 4;
            count -= 4;
        }
#endif

        while (count-- > 0)
        {
            u32 color = *s++;
            *d++ = (color & 0xff00ff00) | (color << 16) | ((color >> 16) & 0x00ff);
        }
    }

    void blit_24bit_swap_rg(u8* dest, const u8* src, int count)
    {
        u8* d = reinterpret_cast<u8*>(dest);
        const u8* s = reinterpret_cast<const u8*>(src);

#if 0
        while (count >= 4)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            d[3] = s[5];
            d[4] = s[4];
            d[5] = s[3];
            d[6] = s[8];
            d[7] = s[7];
            d[8] = s[6];
            d[9] = s[11];
            d[10] = s[10];
            d[11] = s[9];
            s += 12;
            d += 12;
            count -= 4;
        }
#endif

        while (count-- > 0)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            s += 3;
            d += 3;
        }
    }

#if defined(MANGO_ENABLE_SSE4_1)

    // ----------------------------------------------------------------------------
    // SSE4.1
    // ----------------------------------------------------------------------------

    void sse4_32bit_swap_rg(u8* d, const u8* s, int count)
    {
        while (count >= 4)
        {
            __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(s + 0));
            a = _mm_shuffle_epi8(a, _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15));
            _mm_storeu_si128(reinterpret_cast<__m128i *>(d +  0), a);
            s += 16;
            d += 16;
            count -= 4;
        }

        while (count-- > 0)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            d[3] = s[3];
            s += 4;
            d += 4;
        }
    }

    void sse4_24bit_swap_rg(u8* d, const u8* s, int count)
    {
        while (count >= 8)
        {
            constexpr u8 n = 0x80;
            __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(s + 0));
            __m128i b = _mm_loadu_si64(reinterpret_cast<const __m128i *>(s + 16));
            __m128i v0 = _mm_shuffle_epi8(a, _mm_setr_epi8(2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, n));
            __m128i v1 = _mm_shuffle_epi8(b, _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, 1));
            __m128i v2 = _mm_shuffle_epi8(a, _mm_setr_epi8(n, 15, n, n, n, n, n, n, n, n, n, n, n, n, n, n));
            __m128i v3 = _mm_shuffle_epi8(b, _mm_setr_epi8(0, n, 4, 3, 2, 7, 6, 5, n, n, n, n, n, n, n, n));
            a = _mm_or_si128(v0, v1);
            b = _mm_or_si128(v2, v3);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(d +  0), a);
            _mm_storel_epi64(reinterpret_cast<__m128i *>(d + 16), b);
            s += 24;
            d += 24;
            count -= 8;
        }

        while (count-- > 0)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            s += 3;
            d += 3;
        }
    }

#endif // defined(MANGO_ENABLE_SSE4_1)

#if defined(MANGO_ENABLE_AVX2)

    // ----------------------------------------------------------------------------
    // AVX2
    // ----------------------------------------------------------------------------

    void avx2_32bit_swap_rg(u8* d, const u8* s, int count)
    {
        while (count >= 8)
        {
            __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(s + 0));
            a = _mm256_shuffle_epi8(a, _mm256_setr_epi8(
                2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15,
                2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15));
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(d +  0), a);
            s += 32;
            d += 32;
            count -= 8;
        }

        while (count-- > 0)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            d[3] = s[3];
            s += 4;
            d += 4;
        }
    }

#endif // defined(MANGO_ENABLE_AVX2)

#if defined(MANGO_ENABLE_NEON)

    // ----------------------------------------------------------------------------
    // NEON
    // ----------------------------------------------------------------------------

    void neon_32bit_swap_rg(u8* d, const u8* s, int count)
    {
        while (count >= 16)
        {
            const uint8x16x4_t a = vld4q_u8(s);
            uint8x16x4_t b;
            b.val[0] = a.val[2];
            b.val[1] = a.val[1];
            b.val[2] = a.val[0];
            b.val[3] = a.val[3];
            vst4q_u8(d, b);
            s += 64;
            d += 64;
            count -= 16;
        }

        while (count-- > 0)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            d[3] = s[3];
            s += 4;
            d += 4;
        }
    }

    void neon_24bit_swap_rg(u8* d, const u8* s, int count)
    {
        while (count >= 16)
        {
            const uint8x16x3_t a = vld3q_u8(s);
            uint8x16x3_t b;
            b.val[0] = a.val[2];
            b.val[1] = a.val[1];
            b.val[2] = a.val[0];
            vst3q_u8(d, b);
            s += 48;
            d += 48;
            count -= 16;
        }

        while (count-- > 0)
        {
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            s += 3;
            d += 3;
        }
    }

#endif // defined(MANGO_ENABLE_NEON)

    // ----------------------------------------------------------------------------
    // custom conversion function lookup
    // ----------------------------------------------------------------------------

    struct
    {
        Format dest;
        Format source;
        u64 requireCpuFeature;
        Blitter::CustomFunc func;
    }
    const g_custom_func_table[] =
    {

    // rgba.u8 <-> rgbx.u8

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            std::memcpy(dest, src, count * 4);
        } 
    },

    {
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            std::memcpy(dest, src, count * 4);
        } 
    },

    // bgra.u8 <-> bgrx.u8

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        0,
        [] (u8* dest, const u8* src, int count) -> void
        {
            std::memcpy(dest, src, count * 4);
        } 
    },

    {
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        0,
        [] (u8* dest, const u8* src, int count) -> void
        {
            std::memcpy(dest, src, count * 4);
        } 
    },

    // rgba.u8 <- rgbx.u8
    // bgra.u8 <- bgrx.u8

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        0, 
        blit_32bit_generate_alpha 
    },
    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        0, 
        blit_32bit_generate_alpha 
    },

    // rgbx.u8 <-> bgrx.u8

    {
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        0, 
        blit_32bit_swap_rg 
    },

    {
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        0, 
        blit_32bit_swap_rg
    },

    // rgba.u8 <-> bgra.u8

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        0, 
        blit_32bit_swap_rg 
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        0, 
        blit_32bit_swap_rg 
    },

    // rgba.u8 <-> rgb.u8

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            while (count-- > 0)
            {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = 0xff;
                src += 3;
                dest += 4;
            }
        }
    },

    {
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u24* d = reinterpret_cast<u24*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);
            for (int x = 0; x < count; ++x)
            {
                d[x] = s[x];
            }
        }
    },

    // bgrx.u8 <-> rgb.u8

    {
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            while (count-- > 0)
            {
                dest[0] = src[2];
                dest[1] = src[1];
                dest[2] = src[0];
                dest[3] = 0xff;
                src += 3;
                dest += 4;
            }
        }
    },

    {
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            while (count-- > 0)
            {
                dest[0] = src[2];
                dest[1] = src[1];
                dest[2] = src[0];
                src += 4;
                dest += 3;
            }
        }
    },

    // bgra.u8 <-> bgr.u8

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u24* s = reinterpret_cast<const u24*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                d[x] = 0xff000000 | v;
            }
        }
    },

    {
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u24* d = reinterpret_cast<u24*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);
            for (int x = 0; x < count; ++x)
            {
                d[x] = s[x];
            }
        }
    },

    // bgra.u8 <-> rgb.u8

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            while (count-- > 0)
            {
                dest[0] = src[2];
                dest[1] = src[1];
                dest[2] = src[0];
                dest[3] = 0xff;
                src += 3;
                dest += 4;
            }
        }
    },

    {
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u8* d = reinterpret_cast<u8*>(dest);
            const u8* s = reinterpret_cast<const u8*>(src);
            for (int x = 0; x < count; ++x)
            {
                d[0] = s[2];
                d[1] = s[1];
                d[2] = s[0];
                s += 4;
                d += 3;
            }
        }
    },

    // bgr.u8 <-> rgb.u8

    {
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        0, 
        blit_24bit_swap_rg 
    },

    {
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        0, 
        blit_24bit_swap_rg 
    },

    // bgra.u8888 <-> bgr.u565

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(16, Format::UNORM, Format::BGR, 5, 6, 5),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                u32 u = ((v & 0xf800) << 8) | ((v & 0x07e0) << 5) | ((v & 0x001f) << 3);
                u |= ((v & 0xe000) << 3) | ((v & 0x0600) >> 1) | ((v & 0x001c) >> 2);
                d[x] = 0xff000000 | u;
            }
        }
    },

    {
        Format(16, Format::UNORM, Format::BGR, 5, 6, 5),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u16* d = reinterpret_cast<u16*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                u32 u = ((v >> 8) & 0xf800) | ((v >> 5) & 0x07e0) | ((v >> 3) & 0x001f);
                d[x] = u16(u);
            }
        }
    },

    // bgra.u8888 <-> bgra.u5551

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                u32 u = v & 0x8000 ? 0xff000000 : 0;
                u |= ((v & 0x7c00) << 9) | ((v & 0x03e0) << 6) | ((v & 0x001f) << 3);
                d[x] = u | ((u & 0x00e0e0e0) >> 5);
            }
        } 
    },

    {
        Format(16, Format::UNORM, Format::BGRA, 5, 5, 5, 1),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u16* d = reinterpret_cast<u16*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                u32 u = ((v >> 16) & 0x8000) | ((v >> 9) & 0x7c00) |
                        ((v >> 6) & 0x03e0) | ((v >> 3) & 0x001f);
                d[x] = u16(u);
            }
        }
    },

    // bgra.u8888 <-> bgra.u4444

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                u32 u = ((v & 0xf000) << 16) | ((v & 0x0f00) << 12) |
                        ((v & 0x00f0) <<  8) | ((v & 0x000f) << 4);
                d[x] = u | (u >> 4);
            }
        } 
    },

    {
        Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u16* d = reinterpret_cast<u16*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                u32 u = ((v >> 16) & 0xf000) | ((v >> 12) & 0x0f00) |
                        ((v >> 8) & 0x00f0) | ((v >> 4) & 0x000f);
                d[x] = u16(u);
            }
        }
    },

    // rgba.u8888 <-> bgra.u4444

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                u32 u = ((v & 0xf000) << 16) | ((v & 0x000f) << 20) |
                        ((v & 0x00f0) <<  8) | ((v & 0x0f00) >> 4);
                d[x] = u | (u >> 4);
            }
        } 
    },

    {
        Format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u16* d = reinterpret_cast<u16*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                u32 u = ((v >> 16) & 0xf000) | ((v << 4) & 0x0f00) |
                        ((v >> 8) & 0x00f0) | ((v >> 20) & 0x000f);
                d[x] = u16(u);
            }
        }
    },

    // rgb.u8 <- l.u8

    {
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        LuminanceFormat(8, Format::UNORM, 8, 0),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u24* d = reinterpret_cast<u24*>(dest);
            const u8* s = reinterpret_cast<const u8*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                d[x] = u24(v * 0x010101);
            }
        }
    },

    {
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        LuminanceFormat(8, Format::UNORM, 8, 0),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u24* d = reinterpret_cast<u24*>(dest);
            const u8* s = reinterpret_cast<const u8*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                d[x] = u24(v * 0x010101);
            }
        }
    },

    // rgba.u8 <- l.u8

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        LuminanceFormat(8, Format::UNORM, 8, 0),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u8* s = reinterpret_cast<const u8*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                d[x] = 0xff000000 | v * 0x010101;
            }
        }
    },

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        LuminanceFormat(8, Format::UNORM, 8, 0),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u8* s = reinterpret_cast<const u8*>(src);
            for (int x = 0; x < count; ++x)
            {
                u32 v = s[x];
                d[x] = 0xff000000 | v * 0x010101;
            }
        }
    },

    // rgba.u8 <- l.u16

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        LuminanceFormat(16, Format::UNORM, 16, 0),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                const u32 i = s[x] >> 8;
                d[x] = 0xff000000 | i * 0x010101;
            }
        }
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        LuminanceFormat(16, Format::UNORM, 16, 0),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                const u32 i = s[x] >> 8;
                d[x] = 0xff000000 | i * 0x010101;
            }
        }
    },

    // rgba.u8 <- la.u16

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        LuminanceFormat(32, Format::UNORM, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);
            for (int x = 0; x < count; ++x)
            {
                const u32 a = s[x] & 0xff000000;
                const u32 i = (s[x] & 0x0000ff00) >> 8;
                d[x] = a | i * 0x010101;
            }
        }
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        LuminanceFormat(32, Format::UNORM, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);
            for (int x = 0; x < count; ++x)
            {
                const u32 a = s[x] & 0xff000000;
                const u32 i = (s[x] & 0x0000ff00) >> 8;
                d[x] = a | i * 0x010101;
            }
        }
    },

    // rgba.u8 <- rgb.u16

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(48, Format::UNORM, Format::RGB, 16, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                const u32 r = s[0];
                const u32 g = s[1] & 0x0000ff00;
                const u32 b = s[2] & 0x0000ff00;
                d[x] = 0xff000000 | (b << 8) | g | (r >> 8);
                s += 3;
            }
        }
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(48, Format::UNORM, Format::RGB, 16, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                const u32 r = s[0] & 0x0000ff00;
                const u32 g = s[1] & 0x0000ff00;
                const u32 b = s[2];
                d[x] = 0xff000000 | (r << 8) | g | (b >> 8);
                s += 3;
            }
        }
    },

    // rgba.u8 <- rgba.u16

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                const u32 r = s[0];
                const u32 g = s[1] & 0x0000ff00;
                const u32 b = s[2] & 0x0000ff00;
                const u32 a = s[3] & 0x0000ff00;
                d[x] = (a << 16) | (b << 8) | g | (r >> 8);
                s += 4;
            }
        }
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const u16* s = reinterpret_cast<const u16*>(src);
            for (int x = 0; x < count; ++x)
            {
                const u32 r = s[0] & 0x0000ff00;
                const u32 g = s[1] & 0x0000ff00;
                const u32 b = s[2];
                const u32 a = s[3] & 0x0000ff00;
                d[x] = (a << 16) | (r << 8) | g | (b >> 8);
                s += 4;
            }
        }
    },

    // rgba.u8 <-> rgba.f16

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const float16x4* s = reinterpret_cast<const float16x4*>(src);
            for (int x = 0; x < count; ++x)
            {
                float32x4 f = convert<float32x4>(s[x]);
                f = clamp(f, 0.0f, 1.0f);
                f = f * 255.0f + 0.5f;
                int32x4 i = convert<int32x4>(f);
                d[x] = i.pack();
            }
        } 
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const float16x4* s = reinterpret_cast<const float16x4*>(src);
            for (int x = 0; x < count; ++x)
            {
                float32x4 f = convert<float32x4>(s[x]);
                f = f.zyxw;
                f = clamp(f, 0.0f, 1.0f);
                f = f * 255.0f + 0.5f;
                int32x4 i = convert<int32x4>(f);
                d[x] = i.pack();
            }
        } 
    },

    {
        Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            float16x4* d = reinterpret_cast<float16x4*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);

            while (count >= 4)
            {
                uint32x4 color = simd::u32x4_uload(s);

                uint32x4 r = (color >>  0) & 0xff;
                uint32x4 g = (color >>  8) & 0xff;
                uint32x4 b = (color >> 16) & 0xff;
                uint32x4 a = (color >> 24) & 0xff;

                uint32x4 rb01 = unpacklo(r, b);
                uint32x4 ga01 = unpacklo(g, a);
                uint32x4 rb23 = unpackhi(r, b);
                uint32x4 ga23 = unpackhi(g, a);

                uint32x4 rgba0 = unpacklo(rb01, ga01);
                uint32x4 rgba1 = unpackhi(rb01, ga01);
                uint32x4 rgba2 = unpacklo(rb23, ga23);
                uint32x4 rgba3 = unpackhi(rb23, ga23);

                float32x4 v0 = convert<float32x4>(rgba0) / 255.0f;
                float32x4 v1 = convert<float32x4>(rgba1) / 255.0f;
                float32x4 v2 = convert<float32x4>(rgba2) / 255.0f;
                float32x4 v3 = convert<float32x4>(rgba3) / 255.0f;

                d[0] = convert<float16x4>(v0);
                d[1] = convert<float16x4>(v1);
                d[2] = convert<float16x4>(v2);
                d[3] = convert<float16x4>(v3);
                s += 4;
                d += 4;
                count -= 4;
            }

            while (count-- > 0)
            {
                u32 color = s[0];
                float r = float((color >>  0) & 0xff) / 255.0f;
                float g = float((color >>  8) & 0xff) / 255.0f;
                float b = float((color >> 16) & 0xff) / 255.0f;
                float a = float((color >> 24) & 0xff) / 255.0f;
                float32x4 v(r, g, b, a);
                d[0] = convert<float16x4>(v);
                s += 1;
                d += 1;
            }
        } 
    },

    // rgba.u8 <-> rgba.f32

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const float32x4* s = reinterpret_cast<const float32x4*>(src);
            for (int x = 0; x < count; ++x)
            {
                float32x4 f = s[x];
                f = clamp(f, 0.0f, 1.0f);
                f = f * 255.0f + 0.5f;
                int32x4 i = convert<int32x4>(f);
                d[x] = i.pack();
            }
        } 
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            u32* d = reinterpret_cast<u32*>(dest);
            const float32x4* s = reinterpret_cast<const float32x4*>(src);
            for (int x = 0; x < count; ++x)
            {
                float32x4 f = s[x];
                f = f.zyxw;
                f = clamp(f, 0.0f, 1.0f);
                f = f * 255.0f + 0.5f;
                int32x4 i = convert<int32x4>(f);
                d[x] = i.pack();
            }
        } 
    },

    {
        Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            float* d = reinterpret_cast<float*>(dest);
            const u32* s = reinterpret_cast<const u32*>(src);

            while (count >= 4)
            {
                uint32x4 color = simd::u32x4_uload(s);

                uint32x4 r = (color >>  0) & 0xff;
                uint32x4 g = (color >>  8) & 0xff;
                uint32x4 b = (color >> 16) & 0xff;
                uint32x4 a = (color >> 24) & 0xff;

                uint32x4 rb01 = unpacklo(r, b);
                uint32x4 ga01 = unpacklo(g, a);
                uint32x4 rb23 = unpackhi(r, b);
                uint32x4 ga23 = unpackhi(g, a);

                uint32x4 rgba0 = unpacklo(rb01, ga01);
                uint32x4 rgba1 = unpackhi(rb01, ga01);
                uint32x4 rgba2 = unpacklo(rb23, ga23);
                uint32x4 rgba3 = unpackhi(rb23, ga23);

                simd::f32x4_ustore(d + 4 * 0, convert<float32x4>(rgba0) / 255.0f);
                simd::f32x4_ustore(d + 4 * 1, convert<float32x4>(rgba1) / 255.0f);
                simd::f32x4_ustore(d + 4 * 2, convert<float32x4>(rgba2) / 255.0f);
                simd::f32x4_ustore(d + 4 * 3, convert<float32x4>(rgba3) / 255.0f);
                s += 4;
                d += 16;
                count -= 4;
            }

            while (count-- > 0)
            {
                u32 color = s[0];
                d[0] = float((color >>  0) & 0xff) / 255.0f;
                d[1] = float((color >>  8) & 0xff) / 255.0f;
                d[2] = float((color >> 16) & 0xff) / 255.0f;
                d[3] = float((color >> 24) & 0xff) / 255.0f;
                s += 1;
                d += 4;
            }
        } 
    },

    // rgba.f16 <-> rgba.f32

    {
        Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
        Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            float16x4* d = reinterpret_cast<float16x4*>(dest);
            const float32x4* s = reinterpret_cast<const float32x4*>(src);
            for (int x = 0; x < count; ++x)
            {
                d[x] = convert<float16x4>(s[x]);
            }
        } 
    },

    {
        Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32),
        Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16),
        0, 
        [] (u8* dest, const u8* src, int count) -> void
        {
            float32x4* d = reinterpret_cast<float32x4*>(dest);
            const float16x4* s = reinterpret_cast<const float16x4*>(src);
            for (int x = 0; x < count; ++x)
            {
                d[x] = convert<float32x4>(s[x]);
            }
        } 
    },

#if defined(MANGO_ENABLE_SSE2)

    // ----------------------------------------------------------------------------
    // SSE2
    // ----------------------------------------------------------------------------

    /* placeholder
    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        INTEL_SSE2,
        [] (u8* dest, const u8* src, int count) -> void
        {
        }
    },
    */

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_SSE4_1)

    // ----------------------------------------------------------------------------
    // SSE4.1
    // ----------------------------------------------------------------------------

    {
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        INTEL_SSE4_1, 
        sse4_32bit_swap_rg 
    },

    {
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        INTEL_SSE4_1, 
        sse4_32bit_swap_rg
    },

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        INTEL_SSE4_1, 
        sse4_32bit_swap_rg 
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        INTEL_SSE4_1, 
        sse4_32bit_swap_rg 
    },

    {
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        INTEL_SSE4_1, 
        sse4_24bit_swap_rg
    },

    {
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        INTEL_SSE4_1,
        sse4_24bit_swap_rg
    },

#endif // MANGO_ENABLE_SSE4_1

#if defined(MANGO_ENABLE_AVX)

    // ----------------------------------------------------------------------------
    // AVX
    // ----------------------------------------------------------------------------

    /* placeholder
    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        INTEL_AVX,
        [] (u8* dest, const u8* src, int count) -> void
        {
        }
    },
    */

#endif // MANGO_ENABLE_AVX

#if defined(MANGO_ENABLE_AVX2)

    // ----------------------------------------------------------------------------
    // AVX2
    // ----------------------------------------------------------------------------

    {
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        INTEL_AVX2, 
        avx2_32bit_swap_rg 
    },

    {
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        INTEL_AVX2, 
        avx2_32bit_swap_rg
    },

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        INTEL_AVX2, 
        avx2_32bit_swap_rg 
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        INTEL_AVX2, 
        avx2_32bit_swap_rg 
    },

#endif // MANGO_ENABLE_AVX2

#if defined(MANGO_ENABLE_NEON)

    // ----------------------------------------------------------------------------
    // NEON
    // ----------------------------------------------------------------------------

    {
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        ARM_NEON,
        neon_32bit_swap_rg 
    },

    {
        Format(32, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGB, 8, 8, 8),
        ARM_NEON,
        neon_32bit_swap_rg
    },

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        ARM_NEON,
        neon_32bit_swap_rg 
    },

    {
        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8),
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        ARM_NEON,
        neon_32bit_swap_rg 
    },

    {
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        ARM_NEON, 
        neon_24bit_swap_rg
    },

    {
        Format(24, Format::UNORM, Format::BGR, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        ARM_NEON,
        neon_24bit_swap_rg
    },

    {
        Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8),
        Format(24, Format::UNORM, Format::RGB, 8, 8, 8),
        ARM_NEON,
        [] (u8* dest, const u8* src, int count) -> void
        {
            while (count >= 16)
            {
                const uint8x16x3_t rgb = vld3q_u8(src);
                uint8x16x4_t rgba;
                rgba.val[0] = rgb.val[0];
                rgba.val[1] = rgb.val[1];
                rgba.val[2] = rgb.val[2];
                rgba.val[3] = vdupq_n_u8(0xff);
                vst4q_u8(dest, rgba);
                src += 48;
                dest += 64;
                count -= 16;
            }

            while (count-- > 0)
            {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = 0xff;
                src += 3;
                dest += 4;
            }
        }
    },

#endif // MANGO_ENABLE_NEON

    }; // end of custom blitter

    using CustomConversionMap = std::map< std::pair<Format, Format>, Blitter::CustomFunc >;

    // initialize map of custom conversion functions
    CustomConversionMap g_custom_func_map = []
    {
        const u64 cpuFlags = getCPUFlags();

        CustomConversionMap map;

        for (auto& node : g_custom_func_table)
        {
            u64 required = node.requireCpuFeature;
            if ((cpuFlags & required) == required)
            {
                map[std::make_pair(node.dest, node.source)] = node.func;
            }
        }

        return map;
    } ();

    Blitter::CustomFunc find_custom_blitter(const Format& dest, const Format& source)
    {
        Blitter::CustomFunc func = nullptr; // default: no conversion function

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
                case 24: func = blit_memcpy<24>; break;
                case 32: func = blit_memcpy<32>; break;
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

namespace mango::image
{

    // ----------------------------------------------------------------------------
    // Blitter
    // ----------------------------------------------------------------------------

    Blitter::Blitter(const Format& dest, const Format& source)
        : srcFormat(source)
        , destFormat(dest)
        , custom(nullptr)
        , convertFunc(nullptr)
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

        sampleSize = 0;

        const int source_float_shift = source.type - Format::FLOAT16 + 4;

        if (dest.isFloat() && source.isFloat())
        {
            for (int i = 0; i < 4; ++i)
            {
                if (dest.size[i])
                {
                    // not used in float to float blitting
                    component[components].srcMask = 0;
                    component[components].destMask = 0;
                    component[components].scale = 1.0f;
                    component[components].bias = 0.0f;

                    component[components].constant = i == 3 ? 1.0f : 0.0f;
                    component[components].offset = source.size[i] ? source.offset[i] >> source_float_shift : -1;

                    if (source.size[i])
                        ++sampleSize;

                    ++components;
                }
            }

            // select innerloop
            int destBits = modeBits(dest);
            int sourceBits = modeBits(source);
            int modeMask = MAKE_MODEMASK(destBits, sourceBits);

            convertFunc = convert_fpu(modeMask);

            return;
        }

        for (int i = 0; i < 4; ++i)
        {
            u32 src_mask = source.mask(i);
            u32 dest_mask = dest.mask(i);

            if (dest_mask)
            {
                if (src_mask != dest_mask)
                {
                    if (src_mask)
                    {
                        // isolate least significant bit of mask; this is used for correct rounding
                        const u32 lsb = dest_mask ^ (dest_mask - 1);

                        // source and destination are different: add channel to component array
                        component[components].srcMask = src_mask;
                        component[components].destMask = dest_mask;
                        component[components].scale = float(dest_mask) / float(src_mask);
                        component[components].bias = lsb * 0.5f;

                        component[components].constant = i == 3 ? 1.0f : 0.0f;
                        component[components].offset = source.offset[i] >> source_float_shift;

                        if (source.size[i])
                        {
                            ++sampleSize;
                        }

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

        // select innerloop
        int destBits = modeBits(dest);
        int sourceBits = modeBits(source);
        int modeMask = MAKE_MODEMASK(destBits, sourceBits);

        convertFunc = convert_fpu(modeMask);
    }

    Blitter::~Blitter()
    {
    }

    void Blitter::convert(const BlitRect& rect) const
    {
        if (convertFunc)
        {
            convertFunc(*this, rect);
        }
    }

} // namespace mango::image
