/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/bits.hpp>
#include <mango/image/format.hpp>

namespace mango
{

    // ----------------------------------------------------------------------------
    // Format
    // ----------------------------------------------------------------------------

    Format::Format()
        : bits(0)
        , type(NONE)
        , size(0)
        , offset(0)
    {
    }

    Format::Format(int bits, u32 luminanceMask, u32 alphaMask)
        : bits(bits)
        , type(UNORM)
    {
        assert(!(bits & 7));
        assert(bits >= 8 && bits <= 32);

        size[0] = u8(u32_count_bits(luminanceMask));
        size[1] = u8(u32_count_bits(luminanceMask));
        size[2] = u8(u32_count_bits(luminanceMask));
        size[3] = u8(u32_count_bits(alphaMask));
        offset[0] = u8(u32_index_of_lsb(luminanceMask));
        offset[1] = u8(u32_index_of_lsb(luminanceMask));
        offset[2] = u8(u32_index_of_lsb(luminanceMask));
        offset[3] = u8(u32_index_of_lsb(alphaMask));
    }

    Format::Format(int bits, u32 redMask, u32 greenMask, u32 blueMask, u32 alphaMask)
        : bits(bits)
        , type(UNORM)
    {
        assert(!(bits & 7));
        assert(bits >= 8 && bits <= 32);

        size[0] = u8(u32_count_bits(redMask));
        size[1] = u8(u32_count_bits(greenMask));
        size[2] = u8(u32_count_bits(blueMask));
        size[3] = u8(u32_count_bits(alphaMask));
        offset[0] = u8(u32_index_of_lsb(redMask));
        offset[1] = u8(u32_index_of_lsb(greenMask));
        offset[2] = u8(u32_index_of_lsb(blueMask));
        offset[3] = u8(u32_index_of_lsb(alphaMask));
    }

    Format::Format(int bits, Type type, ColorRGBA size, ColorRGBA offset)
        : bits(bits)
        , type(type)
        , size(size)
        , offset(offset)
    {
    }

    Format::Format(int bits, Type type, Order order, int s0, int s1, int s2, int s3)
        : bits(bits)
        , type(type)
    {
        assert(!(bits & 7));
        assert(bits >= 8 && bits <= 128);

        // compute component indices from order mask
		const int c0 = (order >> 0) & 3;
		const int c1 = (order >> 2) & 3;
		const int c2 = (order >> 4) & 3;
		const int c3 = (order >> 6) & 3;

		// compute component offset
        offset[c0] = u8(0);
        offset[c1] = u8(s0);
        offset[c2] = u8(s0 + s1);
        offset[c3] = u8(s0 + s1 + s2);

		// compute component size
        size[c0] = u8(s0);
        size[c1] = u8(s1);
        size[c2] = u8(s2);
        size[c3] = u8(s3);
    }

    const Format& Format::operator = (const Format& format)
    {
        bits = format.bits;
        type = format.type;
        size = format.size;
        offset = format.offset;
        return *this;
    }

    bool Format::operator == (const Format& format) const
    {
        return std::memcmp(this, &format, sizeof(Format)) == 0;
    }

    bool Format::operator != (const Format& format) const
    {
        return std::memcmp(this, &format, sizeof(Format)) != 0;
    }

    bool Format::operator < (const Format& format) const
    {
        return std::memcmp(this, &format, sizeof(Format)) < 0;
    }

    int Format::bytes() const
    {
        return bits >> 3;
    }

    bool Format::isAlpha() const
    {
        // check alpha channel size
        return size[ALPHA] > 0;
    }

    bool Format::isLuminance() const
    {
        // check if red, green and blue channels are identical
        u32 red   = mask(RED);
        u32 green = mask(GREEN);
        u32 blue  = mask(BLUE);
        return (red != 0) && (red == green) && (red == blue);
    }

    bool Format::isFloat() const
    {
        return (type & 0x0100) != 0;
    }

    u32 Format::mask(int component) const
    {
        return u32((1u << size[component]) - 1) << offset[component];
    }

    u32 Format::pack(float red, float green, float blue, float alpha) const
    {
        const float scale[] =
        {
            clamp(red,   0.0f, 1.0f),
            clamp(green, 0.0f, 1.0f),
            clamp(blue,  0.0f, 1.0f),
            clamp(alpha, 0.0f, 1.0f)
        };

        u32 color = 0;

        switch (type)
        {
            case UNORM:
                for (int i = 0; i < 4; ++i)
                {
					const u32 mask = (1 << size[i]) - 1;
                    u32 component = u32(mask * scale[i] + 0.5f) << offset[i];
                    color |= component;
                }
                break;

            case NONE:
            case SRGB:
            case SNORM:
            case UINT:
            case SINT:
            case FLOAT16:
			case FLOAT32:
			case FLOAT64:
				// not supported
                break;
        }

        return color;
    }

} // namespace mango
