/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/bits.hpp>
#include <mango/image/format.hpp>

namespace mango
{

    // ----------------------------------------------------------------------------
    // Format
    // ----------------------------------------------------------------------------

    Format::Format(int bits, uint32 luminanceMask, uint32 alphaMask)
        : bits(bits)
        , type(UNORM)
    {
        assert(!(bits & 7));
        assert(bits >= 8 && bits <= 32);

        size[0] = uint8(u32_count_bits(luminanceMask));
        size[1] = uint8(u32_count_bits(luminanceMask));
        size[2] = uint8(u32_count_bits(luminanceMask));
        size[3] = uint8(u32_count_bits(alphaMask));
        offset[0] = uint8(u32_index_of_lsb(luminanceMask));
        offset[1] = uint8(u32_index_of_lsb(luminanceMask));
        offset[2] = uint8(u32_index_of_lsb(luminanceMask));
        offset[3] = uint8(u32_index_of_lsb(alphaMask));
    }

    Format::Format(int bits, uint32 redMask, uint32 greenMask, uint32 blueMask, uint32 alphaMask)
        : bits(bits)
        , type(UNORM)
    {
        assert(!(bits & 7));
        assert(bits >= 8 && bits <= 32);

        size[0] = uint8(u32_count_bits(redMask));
        size[1] = uint8(u32_count_bits(greenMask));
        size[2] = uint8(u32_count_bits(blueMask));
        size[3] = uint8(u32_count_bits(alphaMask));
        offset[0] = uint8(u32_index_of_lsb(redMask));
        offset[1] = uint8(u32_index_of_lsb(greenMask));
        offset[2] = uint8(u32_index_of_lsb(blueMask));
        offset[3] = uint8(u32_index_of_lsb(alphaMask));
    }

    Format::Format(int bits, Type type, Order order, int s0, int s1, int s2, int s3)
        : bits(bits)
        , type(type)
    {
        assert(!(bits & 7));
        assert(bits >= 8 && bits <= 32);

        // compute component indices from order mask
		const int c0 = (order >> 0) & 3;
		const int c1 = (order >> 2) & 3;
		const int c2 = (order >> 4) & 3;
		const int c3 = (order >> 6) & 3;

		// compute component offset
        offset[c0] = uint8(0);
        offset[c1] = uint8(s0);
        offset[c2] = uint8(s0 + s1);
        offset[c3] = uint8(s0 + s1 + s2);

		// compute component size
        size[c0] = uint8(s0);
        size[c1] = uint8(s1);
        size[c2] = uint8(s2);
        size[c3] = uint8(s3);
    }

    const Format& Format::operator = (const Format& format)
    {
        std::memcpy(this, &format, sizeof(Format));
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

	int Format::float_bits() const
	{
		uint32 f_bits = 0;

		switch (type)
		{
			case FP16:
				f_bits = 16;
				break;
			case FP32:
				f_bits = 32;
				break;
			case FP64:
				f_bits = 64;
				break;
            default:
                break;
		}

		return f_bits;
	}

    uint32 Format::pack(float red, float green, float blue, float alpha) const
    {
        const float scale[] =
        {
            clamp(red, 0.0f, 1.0f),
            clamp(green, 0.0f, 1.0f),
            clamp(blue, 0.0f, 1.0f),
            clamp(alpha, 0.0f, 1.0f)
        };

        uint32 color = 0;

        switch (type)
        {
            case UNORM:
                for (int i = 0; i < 4; ++i)
                {
					const uint32 mask = (1 << size[i]) - 1;
                    uint32 component = uint32(mask * scale[i] + 0.5f) << offset[i];
                    color |= component;
                }
                break;

            case NONE:
            case SRGB:
            case SNORM:
            case UINT:
            case SINT:
            case FP16:
			case FP32:
			case FP64:
				// not supported
                break;
        }

        return color;
    }

} // namespace mango
