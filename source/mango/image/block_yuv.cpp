/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/bits.hpp>
#include <mango/image/compression.hpp>

namespace
{
    using namespace mango;

    union RGB9E5
    {
        enum
        {
            EXPONENT = 5,
            MANTISSA = 9,
            BIAS = 15
        };

        u32 u;
        struct
        {
            u32 Red : MANTISSA;
            u32 Green : MANTISSA;
            u32 Blue : MANTISSA;
            u32 Exponent : EXPONENT;
        };

        RGB9E5()
        {
        }

        explicit RGB9E5(u32 exponent, u32 red, u32 green, u32 blue)
            : Red(red)
            , Green(green)
            , Blue(blue)
            , Exponent(exponent)
        {
        }

        explicit RGB9E5(u32 bits)
            : u(bits)
        {
        }

        ~RGB9E5()
        {
        }
    };

    inline u32 pack_yrgb(int y, int r, int g, int b)
    {
        r = byteclamp((y + r) >> 8);
        g = byteclamp((y + g) >> 8);
        b = byteclamp((y + b) >> 8);
        return 0xff000000 | (b << 16) | (g << 8) | r;
    }

} // namespace

namespace mango::image
{

    void decode_block_uyvy(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        MANGO_UNREFERENCED(stride);

        int y0 = (input[1] - 16) * 298 + 128;
        int y1 = (input[3] - 16) * 298 + 128;
        int u = input[0] - 128;
        int v = input[2] - 128;

        int r = v * 409;
        int g = u * -100 + v * -208;
        int b = u * 516;

        u32* dest = reinterpret_cast<u32*>(output);
        dest[0] = pack_yrgb(y0, r, g, b);
        dest[1] = pack_yrgb(y1, r, g, b);
    }

    void decode_block_yuy2(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        MANGO_UNREFERENCED(stride);

        int y0 = (input[0] - 16) * 298 + 128;
        int y1 = (input[2] - 16) * 298 + 128;
        int u = input[1] - 128;
        int v = input[3] - 128;

        int r = v * 409;
        int g = u * -100 + v * -208;
        int b = u * 516;

        u32* dest = reinterpret_cast<u32*>(output);
        dest[0] = pack_yrgb(y0, r, g, b);
        dest[1] = pack_yrgb(y1, r, g, b);
    }

    void decode_block_grgb8(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        MANGO_UNREFERENCED(stride);

        output[0] = input[1];
        output[1] = input[0];
        output[2] = input[3];
        output[3] = 0xff;

        output[4] = input[1];
        output[5] = input[2];
        output[6] = input[3];
        output[7] = 0xff;
    }

    void decode_block_rgbg8(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        MANGO_UNREFERENCED(stride);

        output[0] = input[0];
        output[1] = input[1];
        output[2] = input[2];
        output[3] = 0xff;

        output[4] = input[0];
        output[5] = input[3];
        output[6] = input[2];
        output[7] = 0xff;
    }

    void decode_block_rgb9e5(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        MANGO_UNREFERENCED(stride);

        u32 data = uload32le(input);
        RGB9E5 sample(data);

        const int exponent = sample.Exponent - (RGB9E5::BIAS + RGB9E5::MANTISSA);
        const float scale = exponent < 0 ? 1.0f / float(1 << -exponent) : float(1 << exponent);

        float* dest = reinterpret_cast<float*>(output);
        dest[0] = scale * sample.Red;
        dest[1] = scale * sample.Green;
        dest[2] = scale * sample.Blue;
        dest[3] = 1.0f;
    }

    /*
    void encode_block_r11f_g11f_b10f(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        const float* source = reinterpret_cast<const float*>(input);
        u32* dest = reinterpret_cast<u32*>(output);

        const u32 red   = packFloat<0, 5, 6>(source[0]);
        const u32 green = packFloat<0, 5, 6>(source[1]);
        const u32 blue  = packFloat<0, 5, 5>(source[2]);
        dest[0] = (blue << 22) | (green << 11) | red;
    }
    */

    void decode_block_r11f_g11f_b10f(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        MANGO_UNREFERENCED(stride);

        u32 data = uload32le(input);

        // extract components
        const u32 red_mantissa = (data >> 0) & 0x3f;
        const u32 red_exponent = (data >> 6) & 0x1f;
        const u32 green_mantissa = (data >> 11) & 0x3f;
        const u32 green_exponent = (data >> 17) & 0x1f;
        const u32 blue_mantissa = (data >> 22) & 0x1f;
        const u32 blue_exponent = (data >> 27) & 0x1f;

        float* dest = reinterpret_cast<float*>(output);
        dest[0] = Float::unpack<0, 5, 6>(0, red_exponent, red_mantissa);
        dest[1] = Float::unpack<0, 5, 6>(0, green_exponent, green_mantissa);
        dest[2] = Float::unpack<0, 5, 5>(0, blue_exponent, blue_mantissa);
        dest[3] = 1.0f;
    }

    void decode_block_r10f_g11f_b11f(const TextureCompressionInfo& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);
        MANGO_UNREFERENCED(stride);

        u32 data = uload32le(input);

        // extract components
        const u32 red_mantissa = (data >> 0) & 0x1f;
        const u32 red_exponent = (data >> 5) & 0x1f;
        const u32 green_mantissa = (data >> 10) & 0x3f;
        const u32 green_exponent = (data >> 16) & 0x1f;
        const u32 blue_mantissa = (data >> 21) & 0x3f;
        const u32 blue_exponent = (data >> 27) & 0x1f;

        float* dest = reinterpret_cast<float*>(output);
        dest[0] = Float::unpack<0, 5, 5>(0, red_exponent, red_mantissa);
        dest[1] = Float::unpack<0, 5, 6>(0, green_exponent, green_mantissa);
        dest[2] = Float::unpack<0, 5, 6>(0, blue_exponent, blue_mantissa);
        dest[3] = 1.0f;
    }

} // namespace mango::image
