/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
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
    : m_bits(bits), m_type(UNORM)
    {
        m_size[0] = uint8(u32_count_bits(luminanceMask));
        m_size[1] = uint8(u32_count_bits(luminanceMask));
        m_size[2] = uint8(u32_count_bits(luminanceMask));
        m_size[3] = uint8(u32_count_bits(alphaMask));
        m_offset[0] = uint8(u32_index_of_lsb(luminanceMask));
        m_offset[1] = uint8(u32_index_of_lsb(luminanceMask));
        m_offset[2] = uint8(u32_index_of_lsb(luminanceMask));
        m_offset[3] = uint8(u32_index_of_lsb(alphaMask));

        if (bits & 7 || bits < 8 || bits > 32) {
            MANGO_EXCEPTION("Incorrect number of bits.");
        }
    }

    Format::Format(int bits, uint32 redMask, uint32 greenMask, uint32 blueMask, uint32 alphaMask)
    : m_bits(bits), m_type(UNORM)
    {
        m_size[0] = uint8(u32_count_bits(redMask));
        m_size[1] = uint8(u32_count_bits(greenMask));
        m_size[2] = uint8(u32_count_bits(blueMask));
        m_size[3] = uint8(u32_count_bits(alphaMask));
        m_offset[0] = uint8(u32_index_of_lsb(redMask));
        m_offset[1] = uint8(u32_index_of_lsb(greenMask));
        m_offset[2] = uint8(u32_index_of_lsb(blueMask));
        m_offset[3] = uint8(u32_index_of_lsb(alphaMask));

        if (bits & 7 || bits < 8 || bits > 32) {
            MANGO_EXCEPTION("Incorrect number of bits.");
        }
    }

    Format::Format(int bits, Type type, Order order, int s0, int s1, int s2, int s3)
    : m_bits(bits), m_type(type)
    {
        // compute component indices from order mask
		const int c0 = (order >> 0) & 3;
		const int c1 = (order >> 2) & 3;
		const int c2 = (order >> 4) & 3;
		const int c3 = (order >> 6) & 3;

		// compute component offset
        m_offset[c0] = uint8(0);
        m_offset[c1] = uint8(s0);
        m_offset[c2] = uint8(s0 + s1);
        m_offset[c3] = uint8(s0 + s1 + s2);

		// compute component size
        m_size[c0] = uint8(s0);
        m_size[c1] = uint8(s1);
        m_size[c2] = uint8(s2);
        m_size[c3] = uint8(s3);
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

    int Format::bits() const
    {
        return m_bits;
    }

    int Format::bytes() const
    {
        return m_bits / 8;
    }

	int Format::float_bits() const
	{
		uint32 bits = 0;

		switch (m_type)
		{
			case FP16:
				bits = 16;
				break;
			case FP32:
				bits = 32;
				break;
			case FP64:
				bits = 64;
				break;
		}

		return bits;
	}

    Format::Type Format::type() const
    {
        return static_cast<Type>(m_type);
    }

    bool Format::alpha() const
    {
        return m_size[3] > 0;
    }

    PackedColor Format::size() const
    {
        return m_size;
    }

    PackedColor Format::offset() const
    {
        return m_offset;
    }

    int Format::size(int component) const
    {
        return m_size[component];
    }

    int Format::offset(int component) const
    {
        return m_offset[component];
    }

    uint32 Format::mask(int component) const
    {
        uint32 mask = uint32((1UL << m_size[component]) - 1) << m_offset[component];
        return mask;
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

        switch (m_type)
        {
            case UNORM:
                for (int i = 0; i < 4; ++i)
                {
					const uint32 mask = (1 << m_size[i]) - 1;
                    uint32 component = uint32(mask * scale[i] + 0.5f) << m_offset[i];
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

