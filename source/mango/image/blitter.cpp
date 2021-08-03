/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <cassert>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/half.hpp>
#include <mango/image/blitter.hpp>
#include <mango/math/vector.hpp>

namespace
{
    using namespace mango;
    using namespace mango::math;
    using namespace mango::image;

    // ----------------------------------------------------------------------------
    // configure
    // ----------------------------------------------------------------------------

#if defined(MANGO_COMPILER_MICROSOFT)

    // The compiler optimizes the scalar loops really well with ICC/CLANG/GCC so these custom
    // conversions are only enabled for MSVC, where the speed up varies between 2x and 4x.

    // TODO: fine-tune the detection if some combination is found that improves the performance. :)

    #if defined(MANGO_ENABLE_SSE4_1)
        #define BLITTER_ENABLE_SSE4
    #endif

    #if defined(MANGO_ENABLE_AVX2)
        #define BLITTER_ENABLE_AVX2
    #endif

    // NOTE: this one is questionable, very little pay-off for requiring a rare ISA extension
    #if defined(MANGO_ENABLE_AVX512)
        #define BLITTER_ENABLE_AVX512
    #endif

#endif

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
    // conversion table
    // ----------------------------------------------------------------------------

    struct ConversionTable
    {
        u32 scale;
        u32 bias;
        u32 shift;
    };

    static
    const ConversionTable g_conversion_table[] =
    {
        // 1 bit source
        { 1, 0, 0 }, { 3, 0, 0 }, { 7, 0, 0 }, { 15, 0, 0 },
        { 31, 0, 0 }, { 63, 0, 0 }, { 127, 0, 0 }, { 255, 0, 0 },
        { 511, 0, 0 }, { 1023, 0, 0 }, { 2047, 0, 0 }, { 4095, 0, 0 },
        { 8191, 0, 0 }, { 16383, 0, 0 }, { 32767, 0, 0 }, { 65535, 0, 0 },

        // 2 bit source
        { 1, 0, 1 }, { 1, 0, 0 }, { 9, 2, 2 }, { 5, 0, 0 },
        { 41, 2, 2 }, { 21, 0, 0 }, { 169, 2, 2 }, { 85, 0, 0 },
        { 681, 2, 2 }, { 341, 0, 0 }, { 2729, 2, 2 }, { 1365, 0, 0 },
        { 10921, 2, 2 }, { 5461, 0, 0 }, { 43689, 2, 2 }, { 21845, 0, 0 },

        // 3 bit source
        { 1, 0, 2 }, { 2, 1, 2 }, { 1, 0, 0 }, { 17, 4, 3 },
        { 18, 1, 2 }, { 9, 0, 0 }, { 145, 4, 3 }, { 146, 1, 2 },
        { 73, 0, 0 }, { 1169, 4, 3 }, { 1170, 1, 2 }, { 585, 0, 0 },
        { 9361, 4, 3 }, { 9362, 1, 2 }, { 4681, 0, 0 }, { 74897, 4, 3 },

        // 4 bit source
        { 1, 0, 3 }, { 3, 9, 4 }, { 2, 1, 2 }, { 1, 0, 0 },
        { 33, 8, 4 }, { 67, 9, 4 }, { 34, 1, 2 }, { 17, 0, 0 },
        { 545, 8, 4 }, { 1091, 9, 4 }, { 546, 1, 2 }, { 273, 0, 0 },
        { 8737, 8, 4 }, { 17475, 9, 4 }, { 8738, 1, 2 }, { 4369, 0, 0 },

        // 5 bit source
        { 1, 0, 4 }, { 13, 56, 7 }, { 15, 23, 6 }, { 2, 1, 2 },
        { 1, 0, 0 }, { 65, 16, 5 }, { 525, 56, 7 }, { 527, 23, 6 },
        { 66, 1, 2 }, { 33, 0, 0 }, { 2113, 16, 5 }, { 16909, 56, 7 },
        { 16911, 23, 6 }, { 2114, 1, 2 }, { 1057, 0, 0 }, { 67649, 16, 5 },

        // 6 bit source
        { 1, 0, 5 }, { 3, 33, 6 }, { 7, 35, 6 }, { 61, 128, 8 },
        { 2, 1, 2 }, { 1, 0, 0 }, { 129, 32, 6 }, { 259, 33, 6 },
        { 519, 35, 6 }, { 4157, 128, 8 }, { 130, 1, 2 }, { 65, 0, 0 },
        { 8321, 32, 6 }, { 16643, 33, 6 }, { 33287, 35, 6 }, { 266301, 128, 8 },

        // 7 bit source
        { 1, 0, 6 }, { 49, 992, 11 }, { 57, 480, 10 }, { 121, 512, 10 },
        { 125, 256, 9 }, { 2, 1, 2 }, { 1, 0, 0 }, { 257, 64, 7 },
        { 8241, 992, 11 }, { 8249, 480, 10 }, { 16505, 512, 10 }, { 16509, 256, 9 },
        { 258, 1, 2 }, { 129, 0, 0 }, { 33025, 64, 7 }, { 1056817, 992, 11 },

        // 8 bit source
        { 1, 0, 7 }, { 3, 129, 8 }, { 225, 4096, 13 }, { 15, 135, 8 },
        { 249, 1024, 11 }, { 253, 512, 10 }, { 2, 1, 2 }, { 1, 0, 0 },
        { 513, 128, 8 }, { 1027, 129, 8 }, { 65761, 4096, 13 }, { 4111, 135, 8 },
        { 65785, 1024, 11 }, { 65789, 512, 10 }, { 514, 1, 2 }, { 257, 0, 0 },

        // 9 bit source
        { 1, 0, 8 }, { 193, 16256, 15 }, { 7, 259, 9 }, { 241, 3968, 13 },
        { 497, 4096, 13 }, { 505, 2048, 12 }, { 509, 1024, 11 }, { 2, 1, 2 },
        { 1, 0, 0 }, { 1025, 256, 9 }, { 131265, 16256, 15 }, { 4103, 259, 9 },
        { 131313, 3968, 13 }, { 262641, 4096, 13 }, { 262649, 2048, 12 }, { 262653, 1028, 11 },

        // 10 bit source
        { 1, 0, 9 }, { 3, 513, 10 }, { 449, 32512, 16 }, { 961, 32768, 16 },
        { 31, 527, 10 }, { 1009, 8192, 14 }, { 1017, 4096, 13 }, { 1021, 2048, 12 },
        { 2, 1, 2 }, { 1, 0, 0 }, { 2049, 512, 10 }, { 4099, 513, 10 },
        { 524737, 32512, 16 }, { 1049537, 32768, 16 }, { 32799, 527, 10 }, { 1049585, 8208, 14 },

        // 11 bit source
        { 1, 0, 10 }, { 769, 261632, 19 }, { 1793, 262144, 19 }, { 1921, 131072, 18 },
        { 993, 32256, 16 }, { 2017, 32768, 16 }, { 2033, 16384, 15 }, { 2041, 8192, 14 },
        { 2045, 4096, 13 }, { 2, 1, 2 }, { 1, 0, 0 }, { 4097, 1024, 11 },
        { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },

        // 12 bit source
        { 1, 0, 11 }, { 3, 2049, 12 }, { 7, 2051, 12 }, { 15, 2055, 12 },
        { 3969, 262144, 19 }, { 63, 2079, 12 }, { 4065, 65536, 17 }, { 4081, 32768, 16 },
        { 4089, 16384, 15 }, { 4093, 8192, 14 }, { 2, 1, 2 }, { 1, 0, 0 },
        { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },

        // 13 bit source
        { 1, 0, 12 }, { 3073, 4192256, 23 }, { 3585, 2095104, 22 }, { 3841, 1046528, 21 },
        { 7937, 1048576, 21 }, { 4033, 260096, 19 }, { 8129, 262144, 19 }, { 8161, 131072, 18 },
        { 8177, 65536, 17 }, { 8185, 32768, 16 }, { 8189, 16384, 15 }, { 2, 1, 2 },
        { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },

        // 14 bit source
        { 1, 0, 13 }, { 3, 8193, 14 }, { 14337, 16777216, 25 }, { 15361, 8388608, 24 },
        { 15873, 4194304, 23 }, { 16129, 2097152, 22 }, { 127, 8255, 14 }, { 16321, 524288, 20 },
        { 16353, 262144, 19 }, { 16369, 131072, 18 }, { 16377, 65536, 17 }, { 16381, 32772, 16 },
        { 2, 1, 2 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },

        // 15 bit source
        { 1, 0, 14 }, { 12289, 67100672, 27 }, { 7, 16387, 15 }, { 30721, 33554432, 26 },
        { 31, 16399, 15 }, { 32257, 8388608, 24 }, { 16257, 2088960, 22 }, { 32641, 2097152, 22 },
        { 32705, 1048576, 21 }, { 32737, 524288, 20 }, { 32753, 262144, 19 }, { 32761, 131096, 18 },
        { 0, 0, 0 }, { 2, 1, 2 }, { 1, 0, 0 }, { 0, 0, 0 },

        // 16 bit source
        { 1, 0, 15 }, { 3, 32769, 16 }, { 28673, 134201344, 28 }, { 15, 32775, 16 },
        { 31745, 33538048, 26 }, { 0, 0, 0 }, { 0, 0, 0 }, { 255, 32895, 16 },
        { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
        { 0, 0, 0 }, { 0, 0, 0 }, { 2, 1, 2 }, { 1, 0, 0 },
    };

    static
    ConversionTable getConversionTable(int dst, int src)
    {
        if (dst > 16 || src > 16)
        {
            // Conversion is not supported
            return { 0, 0, 0 };
        }

        if (dst < 1 || src < 1)
        {
            // Conversion is disabled
            return { 1, 0, 0 };
        }

        const ConversionTable& table = g_conversion_table[(src - 1) * 16 + (dst - 1)];
        return table;
    }

    static
    bool isConversionTableSupported(const Format& dest, const Format& source)
    {
        if (dest.type != Format::UNORM || source.type != Format::UNORM)
        {
            return false;
        }

        for (int i = 0; i < 4; ++i)
        {
            ConversionTable table = getConversionTable(dest.size[i], source.size[i]);
            if (!table.scale)
            {
                return false;
            }
        }

        return true;
    }

    // ----------------------------------------------------------------------------
    // memory access
    // ----------------------------------------------------------------------------

    // scalar_load

    template <typename T>
    u32 scalar_load(const u8* p);

    template <>
    u32 scalar_load<u8>(const u8* p)
    {
        return u32(*p);
    }

    template <>
    u32 scalar_load<u16>(const u8* p)
    {
        return u32(uload16(p));
    }

    template <>
    u32 scalar_load<u24>(const u8* p)
    {
        return u32((p[2] << 16) | (p[1] << 8) | p[0]);
    }

    template <>
    u32 scalar_load<u32>(const u8* p)
    {
        return uload32(p);
    }

    // store

    template <typename T>
    void scalar_store(u8* p, u32 v);

    template <>
    void scalar_store<u8>(u8* p, u32 v)
    {
        *p = u8(v);
    }

    template <>
    void scalar_store<u16>(u8* p, u32 v)
    {
        ustore16(p, u16(v));
    }

    template <>
    void scalar_store<u24>(u8* p, u32 v)
    {
        p[0] = (v >> 0) & 0xff;
        p[1] = (v >> 8) & 0xff;
        p[2] = (v >> 16) & 0xff;
    }

    template <>
    void scalar_store<u32>(u8* p, u32 v)
    {
        ustore32(p, v);
    }

    // ----------------------------------------------------------------------------
    // conversion template
    // ----------------------------------------------------------------------------

    template <typename DestType, typename SourceType>
    void table_convert_template_unorm_unorm(const Blitter& blitter, const BlitRect& rect)
    {
        struct Component
        {
            u32 srcMask;
            u32 srcOffset;
            u32 destOffset;
            u32 scale;
            u32 bias;
            u32 shift;
        } component[4];

        u32 alphaMask = 0;

        for (int i = 0; i < 4; ++i)
        {
            Component& c = component[i];

            // disable component
            c.srcMask = 0;
            c.bias = 0;

            const Format& source = blitter.srcFormat;
            const Format& dest = blitter.destFormat;

            if (dest.size[i])
            {
                if (source.size[i])
                {
                    // source and destination are different: add channel to component array
                    c.srcMask = (1 << source.size[i]) - 1;
                    c.srcOffset = source.offset[i];
                    c.destOffset = dest.offset[i];

                    const ConversionTable& table = getConversionTable(dest.size[i], source.size[i]);
                    c.scale = table.scale;
                    c.bias = table.bias;
                    c.shift = table.shift;
                }
                else
                {
                    if (i == 3)
                    {
                        // alpha defaults to 1.0 (all bits of destination are set)
                        alphaMask |= dest.mask(i);
                    }
                }
            }
        }

        const u32 mask0 = component[0].srcMask;
        const u32 mask1 = component[1].srcMask;
        const u32 mask2 = component[2].srcMask;
        const u32 mask3 = component[3].srcMask;

        const u32 right0 = component[0].srcOffset;
        const u32 right1 = component[1].srcOffset;
        const u32 right2 = component[2].srcOffset;
        const u32 right3 = component[3].srcOffset;

        const u32 left0 = component[0].destOffset;
        const u32 left1 = component[1].destOffset;
        const u32 left2 = component[2].destOffset;
        const u32 left3 = component[3].destOffset;

        const u32 scale0 = component[0].scale;
        const u32 scale1 = component[1].scale;
        const u32 scale2 = component[2].scale;
        const u32 scale3 = component[3].scale;

        const u32 bias0 = component[0].bias;
        const u32 bias1 = component[1].bias;
        const u32 bias2 = component[2].bias;
        const u32 bias3 = component[3].bias;

        const u32 shift0 = component[0].shift;
        const u32 shift1 = component[1].shift;
        const u32 shift2 = component[2].shift;
        const u32 shift3 = component[3].shift;

        int width = rect.width;
        int height = rect.height;

        auto load = scalar_load<SourceType>;
        auto store = scalar_store<DestType>;

        for (int y = 0; y < height; ++y)
        {
            const u8* s = rect.src_address + rect.src_stride * y;
            u8* d = rect.dest_address + rect.dest_stride * y;;

            for (int x = 0; x < width; ++x)
            {
                u32 v = load(s);
                u32 color = alphaMask;
                color |= ((((v >> right0) & mask0) * scale0 + bias0) >> shift0) << left0;
                color |= ((((v >> right1) & mask1) * scale1 + bias1) >> shift1) << left1;
                color |= ((((v >> right2) & mask2) * scale2 + bias2) >> shift2) << left2;
                color |= ((((v >> right3) & mask3) * scale3 + bias3) >> shift3) << left3;
                store(d, color);
                s += sizeof(SourceType);
                d += sizeof(DestType);
            }
        }
    }

#if defined(BLITTER_ENABLE_SSE4)

    // ----------------------------------------------------------------------------
    // memory access
    // ----------------------------------------------------------------------------

    // load

    template <typename T>
    __m128i sse4_load(const u8* p);

    template <>
    __m128i sse4_load<u8>(const u8* p)
    {
        u32 x = uload32(p + 0);
        __m128i value = _mm_cvtsi32_si128(x);
        //__m128i value = _mm_loadu_si32(p + 0); // broken in gcc-11
        return _mm_cvtepu8_epi32(value);
    }

    template <>
    __m128i sse4_load<u16>(const u8* p)
    {
        __m128i value = _mm_loadu_si64(p);
        return _mm_cvtepu16_epi32(value);
    }

    template <>
    __m128i sse4_load<u24>(const u8* p)
    {
        u32 z = uload32(p + 8);
        __m128i value = _mm_loadu_si64(p);
        value = _mm_insert_epi32(value, z, 2);
        constexpr u8 n = 0x80;
        __m128i mask = _mm_setr_epi8(0, 1, 2, n, 3, 4, 5, n, 6, 7, 8, n, 9, 10, 11, n);
        return _mm_shuffle_epi8(value, mask);
    }

    template <>
    __m128i sse4_load<u32>(const u8* p)
    {
        __m128i value = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        return value;
    }

    // store

    template <typename T>
    void sse4_store(u8* p, __m128i value);

    template <>
    void sse4_store<u8>(u8* p, __m128i value)
    {
        value = _mm_packus_epi32(value, value);
        value = _mm_packus_epi16(value, value);
        u32 x = _mm_extract_epi32(value, 0);
        ustore32(p, x);
    }

    template <>
    void sse4_store<u16>(u8* p, __m128i value)
    {
        value = _mm_packus_epi32(value, value);
        u64 x = _mm_extract_epi64(value, 0);
        ustore64(p, x);
    }

    template <>
    void sse4_store<u24>(u8* p, __m128i value)
    {
        constexpr u8 n = 0x80;
        const __m128i mask = _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, n, n, n, n);
        value = _mm_shuffle_epi8(value, mask);
        _mm_storeu_si64(p + 0, value);
        u32 z = _mm_extract_epi32(value, 2);
        ustore32(p + 8, z);
    }

    template <>
    void sse4_store<u32>(u8* p, __m128i value)
    {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p), value);
    }

    // ----------------------------------------------------------------------------
    // conversion template
    // ----------------------------------------------------------------------------

    template <typename DestType, typename SourceType>
    void sse4_table_convert_template_unorm_unorm(const Blitter& blitter, const BlitRect& rect)
    {
        struct Component
        {
            u32 srcMask;
            u32 srcOffset;
            u32 destOffset;
            u32 scale;
            u32 bias;
            u32 shift;
        } component[4];

        u32 alphaMask = 0;

        for (int i = 0; i < 4; ++i)
        {
            Component& c = component[i];

            const Format& source = blitter.srcFormat;
            const Format& dest = blitter.destFormat;

            if (dest.size[i])
            {
                if (source.size[i])
                {
                    // source and destination are different: add channel to component array
                    c.srcMask = (1 << source.size[i]) - 1;
                    c.srcOffset = source.offset[i];
                    c.destOffset = dest.offset[i];

                    const ConversionTable& table = getConversionTable(dest.size[i], source.size[i]);
                    c.scale = table.scale;
                    c.bias = table.bias;
                    c.shift = table.shift;
                }
                else
                {
                    // disable component
                    c.srcMask = 0;
                    c.bias = 0;

                    const bool is_alpha_channel = (i == 3);
                    if (is_alpha_channel)
                    {
                        // alpha defaults to 1.0 (all bits of destination are set)
                        alphaMask |= dest.mask(i);
                    }
                }
            }
            else
            {
                // disable component
                c.srcMask = 0;
                c.bias = 0;
            }
        }

        const __m128i mask0 = _mm_set1_epi32(component[0].srcMask);
        const __m128i mask1 = _mm_set1_epi32(component[1].srcMask);
        const __m128i mask2 = _mm_set1_epi32(component[2].srcMask);
        const __m128i mask3 = _mm_set1_epi32(component[3].srcMask);

        const __m128i scale0 = _mm_set1_epi32(component[0].scale);
        const __m128i scale1 = _mm_set1_epi32(component[1].scale);
        const __m128i scale2 = _mm_set1_epi32(component[2].scale);
        const __m128i scale3 = _mm_set1_epi32(component[3].scale);

        const __m128i bias0 = _mm_set1_epi32(component[0].bias);
        const __m128i bias1 = _mm_set1_epi32(component[1].bias);
        const __m128i bias2 = _mm_set1_epi32(component[2].bias);
        const __m128i bias3 = _mm_set1_epi32(component[3].bias);

        const __m128i right01 = _mm_set_epi64x(component[1].srcOffset, component[0].srcOffset);
        const __m128i right23 = _mm_set_epi64x(component[3].srcOffset, component[2].srcOffset);

        const __m128i left01 = _mm_set_epi64x(component[1].destOffset, component[0].destOffset);
        const __m128i left23 = _mm_set_epi64x(component[3].destOffset, component[2].destOffset);

        const __m128i shift01 = _mm_set_epi64x(component[1].shift, component[0].shift);
        const __m128i shift23 = _mm_set_epi64x(component[3].shift, component[2].shift);

        const __m128i alpha = _mm_set1_epi32(alphaMask);

        int width = rect.width;
        int height = rect.height;

        auto load = sse4_load<SourceType>;
        auto store = sse4_store<DestType>;

        for (int y = 0; y < height; ++y)
        {
            const u8* s = rect.src_address + rect.src_stride * y;
            u8* d = rect.dest_address + rect.dest_stride * y;;

            int xcount = width;

            while (xcount >= 4)
            {
                __m128i v = load(s);
                __m128i color = alpha;

                __m128i c0 = _mm_srl_epi32(v, right01);
                __m128i c1 = _mm_srl_epi32(v, _mm_unpackhi_epi64(right01, right01));
                __m128i c2 = _mm_srl_epi32(v, right23);
                __m128i c3 = _mm_srl_epi32(v, _mm_unpackhi_epi64(right23, right23));

                c0 = _mm_and_si128(c0, mask0);
                c1 = _mm_and_si128(c1, mask1);
                c2 = _mm_and_si128(c2, mask2);
                c3 = _mm_and_si128(c3, mask3);

                c0 = _mm_mullo_epi32(c0, scale0);
                c1 = _mm_mullo_epi32(c1, scale1);
                c2 = _mm_mullo_epi32(c2, scale2);
                c3 = _mm_mullo_epi32(c3, scale3);

                c0 = _mm_srl_epi32(_mm_add_epi32(c0, bias0), shift01);
                c1 = _mm_srl_epi32(_mm_add_epi32(c1, bias1), _mm_unpackhi_epi64(shift01, shift01));
                c2 = _mm_srl_epi32(_mm_add_epi32(c2, bias2), shift23);
                c3 = _mm_srl_epi32(_mm_add_epi32(c3, bias3), _mm_unpackhi_epi64(shift23, shift23));

                color = _mm_or_si128(color, _mm_sll_epi32(c0, left01));
                color = _mm_or_si128(color, _mm_sll_epi32(c1, _mm_unpackhi_epi64(left01, left01)));
                color = _mm_or_si128(color, _mm_sll_epi32(c2, left23));
                color = _mm_or_si128(color, _mm_sll_epi32(c3, _mm_unpackhi_epi64(left23, left23)));

                store(d, color);
                s += sizeof(SourceType) * 4;
                d += sizeof(DestType) * 4;
                xcount -= 4;
            }
        }

        const int xremain = width % 4;
        if (xremain)
        {
            BlitRect patch = rect;

            patch.width = xremain;
            patch.src_address += (width - xremain) * sizeof(SourceType);
            patch.dest_address += (width - xremain) * sizeof(DestType);

            table_convert_template_unorm_unorm<DestType, SourceType>(blitter, patch);
        }
    }

#endif // defined(BLITTER_ENABLE_SSE4)

#if defined(BLITTER_ENABLE_AVX2)

    // ----------------------------------------------------------------------------
    // memory access
    // ----------------------------------------------------------------------------

    // load

    template <typename T>
    __m256i avx2_load(const u8* p);

    template <>
    __m256i avx2_load<u8>(const u8* p)
    {
        u64 x = uload64(p);
        __m128i value = _mm_cvtsi64_si128(x);
        return _mm256_cvtepu8_epi32(value);
    }

    template <>
    __m256i avx2_load<u16>(const u8* p)
    {
        __m128i value = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        return _mm256_cvtepu16_epi32(value);
    }

    template <>
    __m256i avx2_load<u24>(const u8* p)
    {
        __m128i value0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 0));
        __m128i value1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 8));
        __m256i value = _mm256_setr_m128i(value0, value1);
        constexpr u8 n = 0x80;
        const __m256i mask = _mm256_setr_epi8(
            0, 1, 2, n, 3, 4, 5, n, 6, 7, 8, n, 9, 10, 11, n,
            4, 5, 6, n, 7, 8, 9, n, 10, 11, 12, n, 13, 14, 15, n);
        value = _mm256_shuffle_epi8(value, mask);
        return value;
    }

    template <>
    __m256i avx2_load<u32>(const u8* p)
    {
        __m256i value = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        return value;
    }

    // store

    template <typename T>
    void avx2_store(u8* p, __m256i value);

    template <>
    void avx2_store<u8>(u8* p, __m256i value)
    {
        __m128i value0 = _mm256_extracti128_si256(value, 0);
        __m128i value1 = _mm256_extracti128_si256(value, 1);
        __m128i temp = _mm_packus_epi32(value0, value1);
        temp = _mm_packus_epi16(temp, temp);
        _mm_storeu_si64(reinterpret_cast<__m128i*>(p), temp);
    }

    template <>
    void avx2_store<u16>(u8* p, __m256i value)
    {
        __m128i value0 = _mm256_extracti128_si256(value, 0);
        __m128i value1 = _mm256_extracti128_si256(value, 1);
        __m128i temp = _mm_packus_epi32(value0, value1);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p), temp);
    }

    template <>
    void avx2_store<u24>(u8* p, __m256i value)
    {
        __m128i value0 = _mm256_extracti128_si256(value, 0);
        __m128i value1 = _mm256_extracti128_si256(value, 1);
        constexpr u8 n = 0x80;
        const __m128i mask0 = _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, n, n, n, n);
        const __m128i mask1 = _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, n, n, 0, 1, 2, 4);
        const __m128i mask2 = _mm_setr_epi8(5, 6, 8, 9, 10, 12, 13, 14, n, n, n, n, n, n, n, n);
        __m128i s0 = _mm_shuffle_epi8(value0, mask0);
        __m128i s1 = _mm_shuffle_epi8(value1, mask1);
        __m128i s2 = _mm_shuffle_epi8(value1, mask2);
        s0 = _mm_or_si128(s0, s1);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p + 0), s0);
        _mm_storeu_si64(reinterpret_cast<__m128i*>(p + 16), s2);
    }

    template <>
    void avx2_store<u32>(u8* p, __m256i value)
    {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(p), value);
    }

    // ----------------------------------------------------------------------------
    // conversion template
    // ----------------------------------------------------------------------------

    template <typename DestType, typename SourceType>
    void avx2_table_convert_template_unorm_unorm(const Blitter& blitter, const BlitRect& rect)
    {
        struct Component
        {
            u32 srcMask;
            u32 srcOffset;
            u32 destOffset;
            u32 scale;
            u32 bias;
            u32 shift;
        } component[4];

        u32 alphaMask = 0;

        for (int i = 0; i < 4; ++i)
        {
            Component& c = component[i];

            const Format& source = blitter.srcFormat;
            const Format& dest = blitter.destFormat;

            if (dest.size[i])
            {
                if (source.size[i])
                {
                    // source and destination are different: add channel to component array
                    c.srcMask = (1 << source.size[i]) - 1;
                    c.srcOffset = source.offset[i];
                    c.destOffset = dest.offset[i];

                    const ConversionTable& table = getConversionTable(dest.size[i], source.size[i]);
                    c.scale = table.scale;
                    c.bias = table.bias;
                    c.shift = table.shift;
                }
                else
                {
                    // disable component
                    c.srcMask = 0;
                    c.bias = 0;

                    const bool is_alpha_channel = (i == 3);
                    if (is_alpha_channel)
                    {
                        // alpha defaults to 1.0 (all bits of destination are set)
                        alphaMask |= dest.mask(i);
                    }
                }
            }
            else
            {
                // disable component
                c.srcMask = 0;
                c.bias = 0;
            }
        }

        const __m256i mask0 = _mm256_set1_epi32(component[0].srcMask);
        const __m256i mask1 = _mm256_set1_epi32(component[1].srcMask);
        const __m256i mask2 = _mm256_set1_epi32(component[2].srcMask);
        const __m256i mask3 = _mm256_set1_epi32(component[3].srcMask);

        const __m256i scale0 = _mm256_set1_epi32(component[0].scale);
        const __m256i scale1 = _mm256_set1_epi32(component[1].scale);
        const __m256i scale2 = _mm256_set1_epi32(component[2].scale);
        const __m256i scale3 = _mm256_set1_epi32(component[3].scale);

        const __m256i bias0 = _mm256_set1_epi32(component[0].bias);
        const __m256i bias1 = _mm256_set1_epi32(component[1].bias);
        const __m256i bias2 = _mm256_set1_epi32(component[2].bias);
        const __m256i bias3 = _mm256_set1_epi32(component[3].bias);

        const __m128i right01 = _mm_set_epi64x(component[1].srcOffset, component[0].srcOffset);
        const __m128i right23 = _mm_set_epi64x(component[3].srcOffset, component[2].srcOffset);

        const __m128i left01 = _mm_set_epi64x(component[1].destOffset, component[0].destOffset);
        const __m128i left23 = _mm_set_epi64x(component[3].destOffset, component[2].destOffset);

        const __m128i shift01 = _mm_set_epi64x(component[1].shift, component[0].shift);
        const __m128i shift23 = _mm_set_epi64x(component[3].shift, component[2].shift);

        const __m256i alpha = _mm256_set1_epi32(alphaMask);

        int width = rect.width;
        int height = rect.height;

        auto load = avx2_load<SourceType>;
        auto store = avx2_store<DestType>;

        for (int y = 0; y < height; ++y)
        {
            const u8* s = rect.src_address + rect.src_stride * y;
            u8* d = rect.dest_address + rect.dest_stride * y;;

            int xcount = width;

            while (xcount >= 8)
            {
                __m256i v = load(s);
                __m256i color = alpha;

                __m256i c0 = _mm256_srl_epi32(v, right01);
                __m256i c1 = _mm256_srl_epi32(v, _mm_unpackhi_epi64(right01, right01));
                __m256i c2 = _mm256_srl_epi32(v, right23);
                __m256i c3 = _mm256_srl_epi32(v, _mm_unpackhi_epi64(right23, right23));

                c0 = _mm256_and_si256(c0, mask0);
                c1 = _mm256_and_si256(c1, mask1);
                c2 = _mm256_and_si256(c2, mask2);
                c3 = _mm256_and_si256(c3, mask3);

                c0 = _mm256_mullo_epi32(c0, scale0);
                c1 = _mm256_mullo_epi32(c1, scale1);
                c2 = _mm256_mullo_epi32(c2, scale2);
                c3 = _mm256_mullo_epi32(c3, scale3);

                c0 = _mm256_srl_epi32(_mm256_add_epi32(c0, bias0), shift01);
                c1 = _mm256_srl_epi32(_mm256_add_epi32(c1, bias1), _mm_unpackhi_epi64(shift01, shift01));
                c2 = _mm256_srl_epi32(_mm256_add_epi32(c2, bias2), shift23);
                c3 = _mm256_srl_epi32(_mm256_add_epi32(c3, bias3), _mm_unpackhi_epi64(shift23, shift23));

                color = _mm256_or_si256(color, _mm256_sll_epi32(c0, left01));
                color = _mm256_or_si256(color, _mm256_sll_epi32(c1, _mm_unpackhi_epi64(left01, left01)));
                color = _mm256_or_si256(color, _mm256_sll_epi32(c2, left23));
                color = _mm256_or_si256(color, _mm256_sll_epi32(c3, _mm_unpackhi_epi64(left23, left23)));

                store(d, color);
                s += sizeof(SourceType) * 8;
                d += sizeof(DestType) * 8;
                xcount -= 8;
            }
        }

        const int xremain = width % 8;
        if (xremain)
        {
            BlitRect patch = rect;

            patch.width = xremain;
            patch.src_address += (width - xremain) * sizeof(SourceType);
            patch.dest_address += (width - xremain) * sizeof(DestType);

            table_convert_template_unorm_unorm<DestType, SourceType>(blitter, patch);
        }
    }

#endif // defined(BLITTER_ENABLE_AVX2)

#if defined(BLITTER_ENABLE_AVX512)

    // ----------------------------------------------------------------------------
    // memory access
    // ----------------------------------------------------------------------------

    // load

    template <typename T>
    __m512i avx512_load(const u8* p);

    template <>
    __m512i avx512_load<u8>(const u8* p)
    {
        __m128i value = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        return _mm512_cvtepu8_epi32(value);
    }

    template <>
    __m512i avx512_load<u16>(const u8* p)
    {
        __m256i value = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        return _mm512_cvtepu16_epi32(value);
    }

#if 0

    // requires AVX512VBMI
    template <>
    __m512i avx512_load<u24>(const u8* p)
    {
        __m512i value = _mm512_setzero_si512();
        value = _mm512_mask_loadu_epi32(value, 0x0fff, p);
        constexpr u8 n = 0x80;
        const __m512i idx = _mm512_set_epi8(
            n, 47, 46, 45, n, 44, 43, 42, n, 41, 40 39, n, 38, 37, 36,
            n, 35, 34, 33, n, 32, 31, 30, n, 29., 28, 27, n, 26, 25, 24,
            n, 23, 22, 21, n, 20, 19, 18, n, 17, 16, 15, n, 14, 13, 12,
            n, 11, 10, 9, n, 8, 7, 6, n, 5, 4, 3, n, 2, 1, 0);
        value = _mm512_permutexvar_epi8(idx, value);
        return value;
    }

#else

    template <>
    __m512i avx512_load<u24>(const u8* p)
    {
        // ++++++++ ++++----
        // 00011122 23334445 55666777 888999aa abbbcccd ddeeefff
        //          ----++++ ++++++++
        // 00011122 23334445 55666777 888999aa abbbcccd ddeeefff
        //                            ++++++++ ++++----
        // 00011122 23334445 55666777 888999aa abbbcccd ddeeefff
        //                                     ----++++ ++++++++
        // 00011122 23334445 55666777 888999aa abbbcccd ddeeefff
        __m128i value0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 0));
        __m128i value1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 8));
        __m128i value2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 24));
        __m128i value3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 32));
        __m256i lo = _mm256_setr_m128i(value0, value1);
        __m256i hi = _mm256_setr_m128i(value2, value3);
        __m512i value = _mm512_castsi256_si512(lo);
        value = _mm512_inserti32x8(value, hi, 1);
        constexpr u8 n = 0x80;
        const __m512i idx = _mm512_set_epi8(
            n, 15, 14, 13, n, 12, 11, 10, n, 9, 8, 7, n, 6, 5, 4,
            n, 11, 10, 9, n, 8, 7, 6, n, 5, 4, 3, n, 2, 1, 0,
            n, 15, 14, 13, n, 12, 11, 10, n, 9, 8, 7, n, 6, 5, 4, 
            n, 11, 10, 9, n, 8, 7, 6, n, 5, 4, 3, n, 2, 1, 0);
        value = _mm512_shuffle_epi8(value, idx);
        return value;
    }

#endif

    template <>
    __m512i avx512_load<u32>(const u8* p)
    {
        __m512i value = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(p));
        return value;
    }

    // store

    template <typename T>
    void avx512_store(u8* p, __m512i value);

    template <>
    void avx512_store<u8>(u8* p, __m512i value)
    {
        __m512i xz = _mm512_shuffle_i64x2(value, value, 0x08);
        __m512i yw = _mm512_shuffle_i64x2(value, value, 0x0d);
        value = _mm512_packus_epi32(xz, yw);
        __m128i xy = _mm512_extracti32x4_epi32(value, 0);
        __m128i zw = _mm512_extracti32x4_epi32(value, 1);
        __m128i temp128 = _mm_packus_epi16(xy, zw);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p), temp128);
    }

    template <>
    void avx512_store<u16>(u8* p, __m512i value)
    {
        __m512i xz = _mm512_shuffle_i64x2(value, value, 0x08);
        __m512i yw = _mm512_shuffle_i64x2(value, value, 0x0d);
        value = _mm512_packus_epi32(xz, yw);
        _mm512_mask_storeu_epi32(p, 0x00ff, value);
    }

#if 0

    // requires AVX512VBMI
    template <>
    void avx512_store<u24>(u8* p, __m512i value)
    {
        constexpr u8 n = 0x80;
        const __m512i idx = _mm512_set_epi8(
            n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n,
            62, 61, 60, 58, 57, 56, 54, 53, 52, 50, 49, 48,
            46, 45, 44, 42, 41, 40, 38, 37, 36, 34, 33, 32,
            30, 29, 28, 26, 25, 24, 22, 21, 20, 18, 17, 16,
            14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0);
        value = _mm512_permutexvar_epi8(idx, value);
        _mm512_mask_storeu_epi32(p, 0x0fff, value);
    }

#else

    template <>
    void avx512_store<u24>(u8* p, __m512i value)
    {
        // input:  000x111x222x333x 444x555x666x777x 888x999xaaaxbbbx cccxdddxeeexfffx
        // output: 0001112223334445 55666777888999aa abbbcccdddeeefff
        //
        //        0                1                2
        // 000111222333---- 55666777-------- abbb------------
        // ------------4445 --------888999aa ----cccdddeeefff
        //        3                4                5

        __m128i x = _mm512_extracti32x4_epi32(value, 0);
        __m128i y = _mm512_extracti32x4_epi32(value, 1);
        __m128i z = _mm512_extracti32x4_epi32(value, 2);
        __m128i w = _mm512_extracti32x4_epi32(value, 3);

        constexpr u8 n = 0x80;
        const __m128i mask0 = _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, n, n, n, n);
        const __m128i mask1 = _mm_setr_epi8(5, 6, 8, 9, 10, 12, 13, 14, n, n, n, n, n, n, n, n);
        const __m128i mask2 = _mm_setr_epi8(10, 12, 13, 14, n, n, n, n, n, n, n, n, n, n, n, n);
        const __m128i mask3 = _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, n, n, 0, 1, 2, 4);
        const __m128i mask4 = _mm_setr_epi8(n, n, n, n, n, n, n, n, 0, 1, 2, 4, 5, 6, 8, 9);
        const __m128i mask5 = _mm_setr_epi8(n, n, n, n, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14);

        __m128i value0 = _mm_shuffle_epi8(x, mask0);
        __m128i value1 = _mm_shuffle_epi8(y, mask1);
        __m128i value2 = _mm_shuffle_epi8(z, mask2);
        __m128i value3 = _mm_shuffle_epi8(y, mask3);
        __m128i value4 = _mm_shuffle_epi8(z, mask4);
        __m128i value5 = _mm_shuffle_epi8(w, mask5);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(p +  0), _mm_or_si128(value0, value3));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p + 16), _mm_or_si128(value1, value4));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p + 32), _mm_or_si128(value2, value5));

#if 0
        // input:  000x111x222x333x 444x555x666x777x 888x999xaaaxbbbx cccxdddxeeexfffx
        // output: 0001112223334445 55666777888999aa abbbcccdddeeefff
        //
        //        0                1                2
        // 000111222333---- 55666777-------- abbb------------
        // ------------4445 --------888999aa ----cccdddeeefff
        //        1                2                3

        __m128i y = _mm512_extracti32x4_epi32(value, 1);
        __m128i z = _mm512_extracti32x4_epi32(value, 2);
        __m128i w = _mm512_extracti32x4_epi32(value, 3);
        __m256i value01 = _mm512_extracti32x8_epi32(value, 0);
        __m256i value12 = _mm256_setr_m128i(y, z);
        __m128i value2 = z;
        __m128i value3 = w;

        constexpr u8 n = 0x80;
        const __m256i mask01 = _mm256_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, n, n, n, n,
                                                5, 6, 8, 9, 10, 12, 13, 14, n, n, n, n, n, n, n, n);
        const __m256i mask12 = _mm256_setr_epi8(n, n, n, n, n, n, n, n, n, n, n, n, 0, 1, 2, 4,
                                                n, n, n, n, n, n, n, n, 0, 1, 2, 4, 5, 6, 8, 9);
        const __m128i mask2 = _mm_setr_epi8(10, 12, 13, 14, n, n, n, n, n, n, n, n, n, n, n, n);
        const __m128i mask3 = _mm_setr_epi8(n, n, n, n, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14);

        value01 = _mm256_shuffle_epi8(value01, mask01);
        value12 = _mm256_shuffle_epi8(value12, mask12);
        value2 = _mm_shuffle_epi8(value2, mask2);
        value3 = _mm_shuffle_epi8(value3, mask3);

        _mm256_storeu_si256(reinterpret_cast<__m256i*>(p + 0), _mm256_or_si256(value01, value12));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(p + 32), _mm_or_si128(value2, value3));
#endif
    }

#endif

    template <>
    void avx512_store<u32>(u8* p, __m512i value)
    {
        _mm512_storeu_si512(reinterpret_cast<__m512i*>(p), value);
    }

    // ----------------------------------------------------------------------------
    // conversion template
    // ----------------------------------------------------------------------------

    template <typename DestType, typename SourceType>
    void avx512_table_convert_template_unorm_unorm(const Blitter& blitter, const BlitRect& rect)
    {
        struct Component
        {
            u32 srcMask;
            u32 srcOffset;
            u32 destOffset;
            u32 scale;
            u32 bias;
            u32 shift;
        } component[4];

        u32 alphaMask = 0;

        for (int i = 0; i < 4; ++i)
        {
            Component& c = component[i];

            const Format& source = blitter.srcFormat;
            const Format& dest = blitter.destFormat;

            if (dest.size[i])
            {
                if (source.size[i])
                {
                    // source and destination are different: add channel to component array
                    c.srcMask = (1 << source.size[i]) - 1;
                    c.srcOffset = source.offset[i];
                    c.destOffset = dest.offset[i];

                    const ConversionTable& table = getConversionTable(dest.size[i], source.size[i]);
                    c.scale = table.scale;
                    c.bias = table.bias;
                    c.shift = table.shift;
                }
                else
                {
                    // disable component
                    c.srcMask = 0;
                    c.bias = 0;

                    const bool is_alpha_channel = (i == 3);
                    if (is_alpha_channel)
                    {
                        // alpha defaults to 1.0 (all bits of destination are set)
                        alphaMask |= dest.mask(i);
                    }
                }
            }
            else
            {
                // disable component
                c.srcMask = 0;
                c.bias = 0;
            }
        }

        const __m512i mask0 = _mm512_set1_epi32(component[0].srcMask);
        const __m512i mask1 = _mm512_set1_epi32(component[1].srcMask);
        const __m512i mask2 = _mm512_set1_epi32(component[2].srcMask);
        const __m512i mask3 = _mm512_set1_epi32(component[3].srcMask);

        const __m512i scale0 = _mm512_set1_epi32(component[0].scale);
        const __m512i scale1 = _mm512_set1_epi32(component[1].scale);
        const __m512i scale2 = _mm512_set1_epi32(component[2].scale);
        const __m512i scale3 = _mm512_set1_epi32(component[3].scale);

        const __m512i bias0 = _mm512_set1_epi32(component[0].bias);
        const __m512i bias1 = _mm512_set1_epi32(component[1].bias);
        const __m512i bias2 = _mm512_set1_epi32(component[2].bias);
        const __m512i bias3 = _mm512_set1_epi32(component[3].bias);

        const __m128i right01 = _mm_set_epi64x(component[1].srcOffset, component[0].srcOffset);
        const __m128i right23 = _mm_set_epi64x(component[3].srcOffset, component[2].srcOffset);

        const __m128i left01 = _mm_set_epi64x(component[1].destOffset, component[0].destOffset);
        const __m128i left23 = _mm_set_epi64x(component[3].destOffset, component[2].destOffset);

        const __m128i shift01 = _mm_set_epi64x(component[1].shift, component[0].shift);
        const __m128i shift23 = _mm_set_epi64x(component[3].shift, component[2].shift);

        const __m512i alpha = _mm512_set1_epi32(alphaMask);

        int width = rect.width;
        int height = rect.height;

        auto load = avx512_load<SourceType>;
        auto store = avx512_store<DestType>;

        for (int y = 0; y < height; ++y)
        {
            const u8* s = rect.src_address + rect.src_stride * y;
            u8* d = rect.dest_address + rect.dest_stride * y;;

            int xcount = width;

            while (xcount >= 16)
            {
                __m512i v = load(s);
                __m512i color = alpha;

                __m512i c0 = _mm512_srl_epi32(v, right01);
                __m512i c1 = _mm512_srl_epi32(v, _mm_unpackhi_epi64(right01, right01));
                __m512i c2 = _mm512_srl_epi32(v, right23);
                __m512i c3 = _mm512_srl_epi32(v, _mm_unpackhi_epi64(right23, right23));

                c0 = _mm512_and_si512(c0, mask0);
                c1 = _mm512_and_si512(c1, mask1);
                c2 = _mm512_and_si512(c2, mask2);
                c3 = _mm512_and_si512(c3, mask3);

                c0 = _mm512_mullo_epi32(c0, scale0);
                c1 = _mm512_mullo_epi32(c1, scale1);
                c2 = _mm512_mullo_epi32(c2, scale2);
                c3 = _mm512_mullo_epi32(c3, scale3);

                c0 = _mm512_srl_epi32(_mm512_add_epi32(c0, bias0), shift01);
                c1 = _mm512_srl_epi32(_mm512_add_epi32(c1, bias1), _mm_unpackhi_epi64(shift01, shift01));
                c2 = _mm512_srl_epi32(_mm512_add_epi32(c2, bias2), shift23);
                c3 = _mm512_srl_epi32(_mm512_add_epi32(c3, bias3), _mm_unpackhi_epi64(shift23, shift23));

                color = _mm512_or_si512(color, _mm512_sll_epi32(c0, left01));
                color = _mm512_or_si512(color, _mm512_sll_epi32(c1, _mm_unpackhi_epi64(left01, left01)));
                color = _mm512_or_si512(color, _mm512_sll_epi32(c2, left23));
                color = _mm512_or_si512(color, _mm512_sll_epi32(c3, _mm_unpackhi_epi64(left23, left23)));

                store(d, color);
                s += sizeof(SourceType) * 16;
                d += sizeof(DestType) * 16;
                xcount -= 16;
            }
        }

        const int xremain = width % 16;
        if (xremain)
        {
            BlitRect patch = rect;

            patch.width = xremain;
            patch.src_address += (width - xremain) * sizeof(SourceType);
            patch.dest_address += (width - xremain) * sizeof(DestType);

            table_convert_template_unorm_unorm<DestType, SourceType>(blitter, patch);
        }
    }

#endif // defined(BLITTER_ENABLE_AVX512)

    // ----------------------------------------------------------------------------
    // conversion templates
    // ----------------------------------------------------------------------------

    // unorm <- unorm

    template <typename DestType, typename SourceType>
    void convert_template_unorm_unorm(const Blitter& blitter, const BlitRect& rect)
    {
        struct Component
        {
            u32 srcMask;
            u32 srcOffset;
            u32 destMask;
            u32 destOffset;
            float scale;
        } component[4];

        u32 alphaMask = 0;

        for (int i = 0; i < 4; ++i)
        {
            Component& c = component[i];

            // disable component
            c.srcMask = 0;
            c.scale = 0.0f;

            const Format& source = blitter.srcFormat;
            const Format& dest = blitter.destFormat;

            if (dest.size[i])
            {
                if (source.size[i])
                {
                    c.srcMask = (1 << source.size[i]) - 1;
                    c.srcOffset = source.offset[i];

                    c.destMask = (1 << dest.size[i]) - 1;
                    c.destOffset = dest.offset[i];

                    c.scale = float(c.destMask) / float(c.srcMask);
                }
                else
                {
                    if (i == 3)
                    {
                        // alpha defaults to 1.0 (all bits of destination are set)
                        alphaMask |= dest.mask(i);
                    }
                }
            }
        }

        const u32 mask0 = component[0].srcMask;
        const u32 mask1 = component[1].srcMask;
        const u32 mask2 = component[2].srcMask;
        const u32 mask3 = component[3].srcMask;

        const u32 right0 = component[0].srcOffset;
        const u32 right1 = component[1].srcOffset;
        const u32 right2 = component[2].srcOffset;
        const u32 right3 = component[3].srcOffset;

        const u32 left0 = component[0].destOffset;
        const u32 left1 = component[1].destOffset;
        const u32 left2 = component[2].destOffset;
        const u32 left3 = component[3].destOffset;

        const float scale0 = component[0].scale;
        const float scale1 = component[1].scale;
        const float scale2 = component[2].scale;
        const float scale3 = component[3].scale;

        int width = rect.width;
        int height = rect.height;

        auto load = scalar_load<SourceType>;
        auto store = scalar_store<DestType>;

        for (int y = 0; y < height; ++y)
        {
            const u8* s = rect.src_address + rect.src_stride * y;
            u8* d = rect.dest_address + rect.dest_stride * y;;

            for (int x = 0; x < width; ++x)
            {
                u32 v = load(s);
                u32 color = alphaMask;
                color |= (u32(((v >> right3) & mask3) * scale3 + 0.5f)) << left3;
                color |= (u32(((v >> right2) & mask2) * scale2 + 0.5f)) << left2;
                color |= (u32(((v >> right1) & mask1) * scale1 + 0.5f)) << left1;
                color |= (u32(((v >> right0) & mask0) * scale0 + 0.5f)) << left0;
                store(d, color);
                s += sizeof(SourceType);
                d += sizeof(DestType);
            }
        }
    }

    // unorm <- fp

    template <typename DestType, typename SourceType>
    void convert_template_unorm_fp(const Blitter& blitter, const BlitRect& rect)
    {
        const Format& sf = blitter.srcFormat;
        const Format& df = blitter.destFormat;

        u32 mask[4];
        int offset[4];
        int components = 0;

        for (int i = 0; i < 4; ++i)
        {
            if (sf.size[i])
            {
                if (df.size[i])
                {
                    mask[i] = df.mask(i);
                    offset[i] = sf.offset[i] / (sizeof(SourceType) * 8);
                }
                else
                {
                    mask[i] = 0;
                    offset[i] = 0;
                }

                ++components;
            }
        }

        const u32 alphaMask = sf.isAlpha() ? 0 : df.mask(3);

        const u32 mask0 = mask[0];
        const u32 mask1 = mask[1];
        const u32 mask2 = mask[2];
        const u32 mask3 = mask[3];

        const float scale0 = float(mask[0]);
        const float scale1 = float(mask[1]);
        const float scale2 = float(mask[2]);
        const float scale3 = float(mask[3]);

        const int offset0 = offset[0];
        const int offset1 = offset[1];
        const int offset2 = offset[2];
        const int offset3 = offset[3];

        u8* source = rect.src_address;
        u8* dest = rect.dest_address;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                u32 v = alphaMask;
                v |= u32(clamp(float(src[offset0]), 0.0f, 1.0f) * scale0) & mask0;
                v |= u32(clamp(float(src[offset1]), 0.0f, 1.0f) * scale1) & mask1;
                v |= u32(clamp(float(src[offset2]), 0.0f, 1.0f) * scale2) & mask2;
                v |= u32(clamp(float(src[offset3]), 0.0f, 1.0f) * scale3) & mask3;
                dst[x] = DestType(v);
                src += components;
            }

            source += rect.src_stride;
            dest += rect.dest_stride;
        }
    }

    // fp <- unorm

    template <typename DestType, typename SourceType>
    void convert_template_fp_unorm(const Blitter& blitter, const BlitRect& rect)
    {
        const Format& sf = blitter.srcFormat;
        const Format& df = blitter.destFormat;

        u32 mask[4] = { 0, 0, 0, 0 };
        float scale[4] = { 0, 0, 0, 0 };
        float alpha[4] = { 0, 0, 0, 0 };
        int components = 0;

        for (int i = 0; i < 4; ++i)
        {
            if (df.size[i])
            {
                if (sf.size[i])
                {
                    mask[components] = sf.mask(i);
                    scale[components] = 1.0f / float(mask[components]);
                    alpha[components] = 0.0f;
                }
                else
                {
                    mask[components] = 0;
                    scale[components] = 0.0f;
                    alpha[components] = 1.0f;
                }

                ++components;
            }
        }

        const u32 mask0 = mask[0];
        const u32 mask1 = mask[1];
        const u32 mask2 = mask[2];
        const u32 mask3 = mask[3];

        const float scale0 = scale[0];
        const float scale1 = scale[1];
        const float scale2 = scale[2];
        const float scale3 = scale[3];

        const float alpha0 = alpha[0];
        const float alpha1 = alpha[1];
        const float alpha2 = alpha[2];
        const float alpha3 = alpha[3];

        u8* source = rect.src_address;
        u8* dest = rect.dest_address;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int x = 0; x < rect.width; ++x)
            {
                u32 v = src[x];
                switch (components)
                {
                    case 4:
                        dst[3] = DestType((v & mask3) * scale3 + alpha3);
                    case 3:
                        dst[2] = DestType((v & mask2) * scale2 + alpha2);
                    case 2:
                        dst[1] = DestType((v & mask1) * scale1 + alpha1);
                    case 1:
                        dst[0] = DestType((v & mask0) * scale0 + alpha0);
                }
                dst += components;
            }

            source += rect.src_stride;
            dest += rect.dest_stride;
        }
    }

    // fp <- fp

    template <typename DestType, typename SourceType>
    void convert_template_fp_fp(const Blitter& blitter, const BlitRect& rect)
    {
        const int source_float_shift = blitter.srcFormat.type - Format::FLOAT16 + 4;

        int sample_size = blitter.srcFormat.bytes() / sizeof(SourceType);
        int components = 0;

        SourceType constant[4];
        const SourceType* input[4];
        int stride[4];
        int offset[4];

        for (int i = 0; i < 4; ++i)
        {
            if (blitter.destFormat.size[i])
            {
                offset[components] = blitter.srcFormat.size[i] ? blitter.srcFormat.offset[i] >> source_float_shift : -1;
                if (offset[components] < 0)
                {
                    constant[components] = SourceType(i == 3 ? 1.0f : 0.0f);
                    input[components] = &constant[components];
                    stride[components] = 0;
                }
                else
                {
                    stride[components] = sample_size;
                }

                ++components;
            }
        }

        u8* source = rect.src_address;
        u8* dest = rect.dest_address;

        for (int y = 0; y < rect.height; ++y)
        {
            const SourceType* src = reinterpret_cast<const SourceType*>(source);
            DestType* dst = reinterpret_cast<DestType*>(dest);

            for (int i = 0; i < components; ++i)
            {
                if (offset[i] >= 0)
                {
                    input[i] = src + offset[i];
                }
            }

            const SourceType* input0 = input[0];
            const SourceType* input1 = input[1];
            const SourceType* input2 = input[2];
            const SourceType* input3 = input[3];

            const int stride0 = stride[0];
            const int stride1 = stride[1];
            const int stride2 = stride[2];
            const int stride3 = stride[3];

            for (int x = 0; x < rect.width; ++x)
            {
                switch (components)
                {
                    case 4:
                        dst[3] = DestType(*input3);
                        input3 += stride3;
                    case 3:
                        dst[2] = DestType(*input2);
                        input2 += stride2;
                    case 2:
                        dst[1] = DestType(*input1);
                        input1 += stride1;
                    case 1:
                        dst[0] = DestType(*input0);
                        input0 += stride0;
                }
                dst += components;
            }

            source += rect.src_stride;
            dest += rect.dest_stride;
        }
    }

    void convert_none(const Blitter& blitter, const BlitRect& rect)
    {
        MANGO_UNREFERENCED(blitter);
        MANGO_UNREFERENCED(rect);
    }

    void convert_custom(const Blitter& blitter, const BlitRect& rect)
    {
        u8* src = rect.src_address;
        u8* dest = rect.dest_address;

        for (int y = 0; y < rect.height; ++y)
        {
            blitter.scan_convert(dest, src, rect.width);
            src += rect.src_stride;
            dest += rect.dest_stride;
        }
    }

    Blitter::RectFunc get_rect_convert(const Format& dest, const Format& source)
    {
        int destBits = modeBits(dest);
        int sourceBits = modeBits(source);
        int modeMask = MAKE_MODEMASK(destBits, sourceBits);

        Blitter::RectFunc func = convert_none;

        if (isConversionTableSupported(dest, source))
        {
            switch (modeMask)
            {
                case MAKE_MODEMASK( 8,  8): func = table_convert_template_unorm_unorm<u8, u8>; break;
                case MAKE_MODEMASK( 8, 16): func = table_convert_template_unorm_unorm<u8, u16>; break;
                case MAKE_MODEMASK( 8, 24): func = table_convert_template_unorm_unorm<u8, u24>; break;
                case MAKE_MODEMASK( 8, 32): func = table_convert_template_unorm_unorm<u8, u32>; break;
                case MAKE_MODEMASK(16,  8): func = table_convert_template_unorm_unorm<u16, u8>; break;
                case MAKE_MODEMASK(16, 16): func = table_convert_template_unorm_unorm<u16, u16>; break;
                case MAKE_MODEMASK(16, 24): func = table_convert_template_unorm_unorm<u16, u24>; break;
                case MAKE_MODEMASK(16, 32): func = table_convert_template_unorm_unorm<u16, u32>; break;
                case MAKE_MODEMASK(24,  8): func = table_convert_template_unorm_unorm<u24, u8>; break;
                case MAKE_MODEMASK(24, 16): func = table_convert_template_unorm_unorm<u24, u16>; break;
                case MAKE_MODEMASK(24, 24): func = table_convert_template_unorm_unorm<u24, u24>; break;
                case MAKE_MODEMASK(24, 32): func = table_convert_template_unorm_unorm<u24, u32>; break;
                case MAKE_MODEMASK(32,  8): func = table_convert_template_unorm_unorm<u32, u8>; break;
                case MAKE_MODEMASK(32, 16): func = table_convert_template_unorm_unorm<u32, u16>; break;
                case MAKE_MODEMASK(32, 24): func = table_convert_template_unorm_unorm<u32, u24>; break;
                case MAKE_MODEMASK(32, 32): func = table_convert_template_unorm_unorm<u32, u32>; break;
            }

#if defined(BLITTER_ENABLE_SSE4)
            switch (modeMask)
            {
                case MAKE_MODEMASK( 8,  8): func = sse4_table_convert_template_unorm_unorm<u8, u8>; break;
                case MAKE_MODEMASK( 8, 16): func = sse4_table_convert_template_unorm_unorm<u8, u16>; break;
                case MAKE_MODEMASK( 8, 24): func = sse4_table_convert_template_unorm_unorm<u8, u24>; break;
                case MAKE_MODEMASK( 8, 32): func = sse4_table_convert_template_unorm_unorm<u8, u32>; break;
                case MAKE_MODEMASK(16,  8): func = sse4_table_convert_template_unorm_unorm<u16, u8>; break;
                case MAKE_MODEMASK(16, 16): func = sse4_table_convert_template_unorm_unorm<u16, u16>; break;
                case MAKE_MODEMASK(16, 24): func = sse4_table_convert_template_unorm_unorm<u16, u24>; break;
                case MAKE_MODEMASK(16, 32): func = sse4_table_convert_template_unorm_unorm<u16, u32>; break;
                case MAKE_MODEMASK(24,  8): func = sse4_table_convert_template_unorm_unorm<u24, u8>; break;
                case MAKE_MODEMASK(24, 16): func = sse4_table_convert_template_unorm_unorm<u24, u16>; break;
                case MAKE_MODEMASK(24, 24): func = sse4_table_convert_template_unorm_unorm<u24, u24>; break;
                case MAKE_MODEMASK(24, 32): func = sse4_table_convert_template_unorm_unorm<u24, u32>; break;
                case MAKE_MODEMASK(32,  8): func = sse4_table_convert_template_unorm_unorm<u32, u8>; break;
                case MAKE_MODEMASK(32, 16): func = sse4_table_convert_template_unorm_unorm<u32, u16>; break;
                case MAKE_MODEMASK(32, 24): func = sse4_table_convert_template_unorm_unorm<u32, u24>; break;
                case MAKE_MODEMASK(32, 32): func = sse4_table_convert_template_unorm_unorm<u32, u32>; break;
            }
#endif // defined(BLITTER_ENABLE_SSE4)

#if defined(BLITTER_ENABLE_AVX2)
            switch (modeMask)
            {
                case MAKE_MODEMASK( 8,  8): func = avx2_table_convert_template_unorm_unorm<u8, u8>; break;
                case MAKE_MODEMASK( 8, 16): func = avx2_table_convert_template_unorm_unorm<u8, u16>; break;
                case MAKE_MODEMASK( 8, 24): func = avx2_table_convert_template_unorm_unorm<u8, u24>; break;
                case MAKE_MODEMASK( 8, 32): func = avx2_table_convert_template_unorm_unorm<u8, u32>; break;
                case MAKE_MODEMASK(16,  8): func = avx2_table_convert_template_unorm_unorm<u16, u8>; break;
                case MAKE_MODEMASK(16, 16): func = avx2_table_convert_template_unorm_unorm<u16, u16>; break;
                case MAKE_MODEMASK(16, 24): func = avx2_table_convert_template_unorm_unorm<u16, u24>; break;
                case MAKE_MODEMASK(16, 32): func = avx2_table_convert_template_unorm_unorm<u16, u32>; break;
                case MAKE_MODEMASK(24,  8): func = avx2_table_convert_template_unorm_unorm<u24, u8>; break;
                case MAKE_MODEMASK(24, 16): func = avx2_table_convert_template_unorm_unorm<u24, u16>; break;
                case MAKE_MODEMASK(24, 24): func = avx2_table_convert_template_unorm_unorm<u24, u24>; break;
                case MAKE_MODEMASK(24, 32): func = avx2_table_convert_template_unorm_unorm<u24, u32>; break;
                case MAKE_MODEMASK(32,  8): func = avx2_table_convert_template_unorm_unorm<u32, u8>; break;
                case MAKE_MODEMASK(32, 16): func = avx2_table_convert_template_unorm_unorm<u32, u16>; break;
                case MAKE_MODEMASK(32, 24): func = avx2_table_convert_template_unorm_unorm<u32, u24>; break;
                case MAKE_MODEMASK(32, 32): func = avx2_table_convert_template_unorm_unorm<u32, u32>; break;
            }
#endif // defined(BLITTER_ENABLE_AVX2)

#if defined(BLITTER_ENABLE_AVX512)
            switch (modeMask)
            {
                case MAKE_MODEMASK( 8,  8): func = avx512_table_convert_template_unorm_unorm<u8, u8>; break;
                case MAKE_MODEMASK( 8, 16): func = avx512_table_convert_template_unorm_unorm<u8, u16>; break;
                case MAKE_MODEMASK( 8, 24): func = avx512_table_convert_template_unorm_unorm<u8, u24>; break;
                case MAKE_MODEMASK( 8, 32): func = avx512_table_convert_template_unorm_unorm<u8, u32>; break;
                case MAKE_MODEMASK(16,  8): func = avx512_table_convert_template_unorm_unorm<u16, u8>; break;
                case MAKE_MODEMASK(16, 16): func = avx512_table_convert_template_unorm_unorm<u16, u16>; break;
                case MAKE_MODEMASK(16, 24): func = avx512_table_convert_template_unorm_unorm<u16, u24>; break;
                case MAKE_MODEMASK(16, 32): func = avx512_table_convert_template_unorm_unorm<u16, u32>; break;
                case MAKE_MODEMASK(24,  8): func = avx512_table_convert_template_unorm_unorm<u24, u8>; break;
                case MAKE_MODEMASK(24, 16): func = avx512_table_convert_template_unorm_unorm<u24, u16>; break;
                case MAKE_MODEMASK(24, 24): func = avx512_table_convert_template_unorm_unorm<u24, u24>; break;
                case MAKE_MODEMASK(24, 32): func = avx512_table_convert_template_unorm_unorm<u24, u32>; break;
                case MAKE_MODEMASK(32,  8): func = avx512_table_convert_template_unorm_unorm<u32, u8>; break;
                case MAKE_MODEMASK(32, 16): func = avx512_table_convert_template_unorm_unorm<u32, u16>; break;
                case MAKE_MODEMASK(32, 24): func = avx512_table_convert_template_unorm_unorm<u32, u24>; break;
                case MAKE_MODEMASK(32, 32): func = avx512_table_convert_template_unorm_unorm<u32, u32>; break;
            }
#endif // defined(BLITTER_ENABLE_AVX512)

        }
        else
        {
            switch (modeMask)
            {
                case MAKE_MODEMASK( 8,  8): func = convert_template_unorm_unorm<u8, u8>; break;
                case MAKE_MODEMASK( 8, 16): func = convert_template_unorm_unorm<u8, u16>; break;
                case MAKE_MODEMASK( 8, 24): func = convert_template_unorm_unorm<u8, u24>; break;
                case MAKE_MODEMASK( 8, 32): func = convert_template_unorm_unorm<u8, u32>; break;
                case MAKE_MODEMASK(16,  8): func = convert_template_unorm_unorm<u16, u8>; break;
                case MAKE_MODEMASK(16, 16): func = convert_template_unorm_unorm<u16, u16>; break;
                case MAKE_MODEMASK(16, 24): func = convert_template_unorm_unorm<u16, u24>; break;
                case MAKE_MODEMASK(16, 32): func = convert_template_unorm_unorm<u16, u32>; break;
                case MAKE_MODEMASK(24,  8): func = convert_template_unorm_unorm<u24, u8>; break;
                case MAKE_MODEMASK(24, 16): func = convert_template_unorm_unorm<u24, u16>; break;
                case MAKE_MODEMASK(24, 24): func = convert_template_unorm_unorm<u24, u24>; break;
                case MAKE_MODEMASK(24, 32): func = convert_template_unorm_unorm<u24, u32>; break;
                case MAKE_MODEMASK(32,  8): func = convert_template_unorm_unorm<u32, u8>; break;
                case MAKE_MODEMASK(32, 16): func = convert_template_unorm_unorm<u32, u16>; break;
                case MAKE_MODEMASK(32, 24): func = convert_template_unorm_unorm<u32, u24>; break;
                case MAKE_MODEMASK(32, 32): func = convert_template_unorm_unorm<u32, u32>; break;
                case MAKE_MODEMASK( 8, BITS_FP16): func = convert_template_unorm_fp<u8, float16>; break;
                case MAKE_MODEMASK(16, BITS_FP16): func = convert_template_unorm_fp<u16, float16>; break;
                case MAKE_MODEMASK(24, BITS_FP16): func = convert_template_unorm_fp<u24, float16>; break;
                case MAKE_MODEMASK(32, BITS_FP16): func = convert_template_unorm_fp<u32, float16>; break;
                case MAKE_MODEMASK( 8, BITS_FP32): func = convert_template_unorm_fp<u8, float>; break;
                case MAKE_MODEMASK(16, BITS_FP32): func = convert_template_unorm_fp<u16, float>; break;
                case MAKE_MODEMASK(24, BITS_FP32): func = convert_template_unorm_fp<u24, float>; break;
                case MAKE_MODEMASK(32, BITS_FP32): func = convert_template_unorm_fp<u32, float>; break;
                case MAKE_MODEMASK(BITS_FP16,  8): func = convert_template_fp_unorm<float16, u8>; break;
                case MAKE_MODEMASK(BITS_FP16, 16): func = convert_template_fp_unorm<float16, u16>; break;
                case MAKE_MODEMASK(BITS_FP16, 24): func = convert_template_fp_unorm<float16, u24>; break;
                case MAKE_MODEMASK(BITS_FP16, 32): func = convert_template_fp_unorm<float16, u32>; break;
                case MAKE_MODEMASK(BITS_FP32,  8): func = convert_template_fp_unorm<float, u8>; break;
                case MAKE_MODEMASK(BITS_FP32, 16): func = convert_template_fp_unorm<float, u16>; break;
                case MAKE_MODEMASK(BITS_FP32, 24): func = convert_template_fp_unorm<float, u24>; break;
                case MAKE_MODEMASK(BITS_FP32, 32): func = convert_template_fp_unorm<float, u32>; break;
                case MAKE_MODEMASK(BITS_FP16, BITS_FP16): func = convert_template_fp_fp<float16, float16>; break;
                case MAKE_MODEMASK(BITS_FP16, BITS_FP32): func = convert_template_fp_fp<float16, float32>; break;
                case MAKE_MODEMASK(BITS_FP16, BITS_FP64): func = convert_template_fp_fp<float16, float64>; break;
                case MAKE_MODEMASK(BITS_FP32, BITS_FP16): func = convert_template_fp_fp<float32, float16>; break;
                case MAKE_MODEMASK(BITS_FP32, BITS_FP32): func = convert_template_fp_fp<float32, float32>; break;
                case MAKE_MODEMASK(BITS_FP32, BITS_FP64): func = convert_template_fp_fp<float32, float64>; break;
                case MAKE_MODEMASK(BITS_FP64, BITS_FP16): func = convert_template_fp_fp<float64, float16>; break;
                case MAKE_MODEMASK(BITS_FP64, BITS_FP32): func = convert_template_fp_fp<float64, float32>; break;
                case MAKE_MODEMASK(BITS_FP64, BITS_FP64): func = convert_template_fp_fp<float64, float64>; break;
            }
        }

        return func;
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
            color = (color & 0xff00ff00) | ((color & 0x00ff) << 16) | ((color >> 16) & 0x00ff);
            simd::u32x4_ustore(d, color);
            s += 4;
            d += 4;
            count -= 4;
        }
#endif

        while (count-- > 0)
        {
            u32 color = *s++;
            *d++ = (color & 0xff00ff00) | ((color & 0x00ff) << 16) | ((color >> 16) & 0x00ff);
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
        Blitter::ScanFunc func;
    }
    const g_scan_func_table[] =
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

    using ScanConversionMap = std::map< std::pair<Format, Format>, Blitter::ScanFunc >;

    // initialize map of custom conversion functions
    ScanConversionMap g_scan_func_map = []
    {
        const u64 cpuFlags = getCPUFlags();

        ScanConversionMap map;

        for (auto& node : g_scan_func_table)
        {
            u64 required = node.requireCpuFeature;
            if ((cpuFlags & required) == required)
            {
                map[std::make_pair(node.dest, node.source)] = node.func;
            }
        }

        return map;
    } ();

    Blitter::ScanFunc find_scan_blitter(const Format& dest, const Format& source)
    {
        Blitter::ScanFunc func = nullptr; // default: no conversion function

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
            auto i = g_scan_func_map.find(std::make_pair(dest, source));
            if (i != g_scan_func_map.end())
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
        , scan_convert(nullptr)
        , rect_convert(nullptr)
    {
        scan_convert = find_scan_blitter(dest, source);
        if (scan_convert)
        {
            // found custom blitter
            rect_convert = convert_custom;
            return;
        }

        rect_convert = get_rect_convert(dest, source);
    }

    Blitter::~Blitter()
    {
    }

    void Blitter::convert(const BlitRect& rect) const
    {
        rect_convert(*this, rect);
    }

} // namespace mango::image
