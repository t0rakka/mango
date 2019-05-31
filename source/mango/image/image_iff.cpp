/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "[ImageDecoder.IFF] "

namespace
{
    using namespace mango;

	// ------------------------------------------------------------
	// iff
	// ------------------------------------------------------------

	void unpackBits(u8* buffer, const u8* input, int scansize, int insize)
	{
		u8* buffer_end = buffer + scansize;
		const u8* input_end = input + insize;

		for (; buffer < buffer_end && input < input_end;)
		{
			u8 v = *input++;

			if (v > 128)
			{
				const int n = 257 - v;
				std::memset(buffer, *input++, n);
				buffer += n;
			}
			else if (v < 128)
			{
				const int n = v + 1;
				std::memcpy(buffer, input, n);
				input += n;
				buffer += n;
			}
			else
			{
				// 0x80
				break;
			}
		}
	}

	void p2c_raw(u8* image, const u8* temp, int xsize, int ysize, int nplanes, int mask)
	{
		const int bpp = (nplanes + 7) >> 3;
		const int scansize = xsize * bpp;
		const int planesize = ((xsize + 15) & ~15) / 8;
        const int mplanes = nplanes + (mask == 1);

		for (int y = 0; y < ysize; ++y)
		{
			std::memset(image, 0, scansize);

			for (int x = 0; x < xsize; ++x)
			{
				const int shift = ((x ^ 7) & 7);

				const u8* src = temp + x / 8;
				u8* dest = image + x * bpp;

				for (int n = 0; n < nplanes; ++n)
				{
					int v = src[n * planesize] >> shift;
					dest[n / 8] |= (v & 1) << (n & 7);
				}
			}

			image += scansize;
			temp += planesize * mplanes;
		}
	}

	void p2c_ham(u8* dest, const u8* workptr, int width, int height, int nplanes, const Palette& palette)
	{
		bool hamcode2b = (nplanes == 6 || nplanes == 8);
		int ham_shift = 8 - (nplanes - (hamcode2b ? 2 : 1));
		//int ham_mask = (1 << ham_shift) - 1;

		int lineskip = ((width + 15) >> 4) << 1;

		for (int y = 0; y < height; ++y)
		{
			u32 r = palette[0].r;
			u32 g = palette[0].g;
			u32 b = palette[0].b;

			u32 bitmask = 0x80;
			const u8* workptr2 = workptr;

			for (int x = 0; x < width; ++x)
			{
				const u8* workptr3 = workptr2;

				// read value
				u32 v = 0;
				u32 colorbit = 1;

				for (int plane = 2; plane < nplanes; ++plane)
				{
					if (*workptr3 & bitmask)
					{
						v |= colorbit;
					}
					workptr3 += lineskip;
					colorbit += colorbit;
				}

				// read hamcode
				u32 hamcode = 0;

				if (*workptr3 & bitmask)
				{
					hamcode = 1;
				}
				workptr3 += lineskip;

				if (hamcode2b)
				{
					if (*workptr3 & bitmask)
					{
						hamcode |= 2;
					}
					workptr3 += lineskip;
				}

				// hold-and-modify
				switch (hamcode)
				{
					case 0:
						r = palette[v].r;
						g = palette[v].g;
						b = palette[v].b;
						break;

					case 1:
                        b = v << ham_shift;
                        break;

					case 2:
                        r = v << ham_shift;
                        break;

					case 3:
                        g = v << ham_shift;
                        break;
				}

                dest[0] = u8(b);
                dest[1] = u8(g);
                dest[2] = u8(r);
                dest[3] = 0xff;
                dest += 4;

				bitmask >>= 1;

				if (!bitmask)
				{
					bitmask = 0x80;
					++workptr2;
				}
			}

			workptr += lineskip * nplanes;
		}
	}

    void expand_palette(u8* dest, const u8* src, int xsize, int ysize, const Palette& palette)
    {
        ColorBGRA* image = reinterpret_cast<ColorBGRA*>(dest);
        int count = xsize * ysize;

        for (int i = 0; i < count; ++i)
        {
            image[i] = palette[src[i]];
        }
    }

    bool read_signature(const u8*& data)
    {
        BigEndianConstPointer p = data;

		u32 v0 = p.read32(); p += 4;
		u32 v1 = p.read32();
        data = p;

		if (v0 != u32_mask_rev('F','O','R','M'))
            MANGO_EXCEPTION(ID"Incorrect signature.");

		if (v1 != u32_mask_rev('I','L','B','M') && v1 != u32_mask_rev('P','B','M',' '))
            MANGO_EXCEPTION(ID"Incorrect signature.");

        return v1 == u32_mask_rev('P','B','M',' ');
    }

    Format select_format(int nplanes, bool ham)
    {
        // choose pixelformat
        Format format;

        if (ham)
        {
            // always decode Hold And Modify modes into 32 bpp
            format = FORMAT_B8G8R8A8;
        }
        else
        {
            int bpp = (nplanes + 7) >> 3;
            switch (bpp)
            {
                case 1:
                    // expand palette
                    format = FORMAT_B8G8R8A8;
                    break;
                case 2:
                    format = FORMAT_B5G6R5;
                    break;
                case 3:
                    format = FORMAT_B8G8R8;
                    break;
                case 4:
                    format = FORMAT_R8G8B8A8;
                    break;
            }
        }

        return format;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            const u8* data = m_memory.address;
            const u8* end = m_memory.address + m_memory.size - 12;

            bool is_pbm = read_signature(data);
            MANGO_UNREFERENCED_PARAMETER(is_pbm);

            bool ham = false;
            u8 nplanes = 0;

            ImageHeader header;

            header.width   = 0;
            header.height  = 0;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.format  = Format();
            header.compression = TextureCompression::NONE;

            // chunk reader
            while (data < end)
            {
                // chunk header
                BigEndianConstPointer p = data;

                u32 id = p.read32();
                u32 size = p.read32();

                // next chunk
                data = p + size + (size & 1);

                switch (id)
                {
                    case u32_mask_rev('B','M','H','D'):
                    {
                        header.width  = p.read16();
                        header.height = p.read16();
                        p += 4;
                        nplanes = p.read8();
                        p += 12;
                        break;
                    }

                    case u32_mask_rev('C','A','M','G'):
                    {
                        u32 v = p.read32();
                        ham = (v & 0x0800) != 0;
                        break;
                    }

                    default:
                        break;
                }
            }

            header.palette = nplanes <= 8 && !ham;
            header.format = select_format(nplanes, ham);

            return header;
        }

        void decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(ptr_palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            const u8* data = m_memory.address;
            const u8* end = m_memory.address + m_memory.size - 12;

            bool is_pbm = read_signature(data);

            Palette palette;

            u8* buffer_allocated = nullptr;
            const u8* buffer = nullptr;

            bool ham = false;
            bool ehb = false;
            int nplanes = 0;
            int compression = 0;
            int xsize = 0;
            int ysize = 0;

            // misc
            int xorigin = 0;
            int yorigin = 0;
            int mask = 0;
            int transcolor = 0;
            int xaspect = 0;
            int yaspect = 0;
            int xscreen = 0;
            int yscreen = 0;

            // chunk reader
            while (data < end)
            {
                // chunk header
                BigEndianConstPointer p = data;

                u32 id = p.read32();
                u32 size = p.read32();

                // next chunk
                data = p + size + (size & 1);

                switch (id)
                {
                    case u32_mask_rev('A','N','N','O'):
                    {
                        break;
                    }

                    case u32_mask_rev('B','M','H','D'):
                    {
                        xsize       = p.read16();
                        ysize       = p.read16();
                        xorigin     = p.read16();
                        yorigin     = p.read16();
                        nplanes     = p.read8();
                        mask        = p.read8();
                        compression = p.read16();
                        transcolor  = p.read16();
                        xaspect     = p.read8();
                        yaspect     = p.read8();
                        xscreen     = p.read16();
                        yscreen     = p.read16();

                        // alignment
                        xsize = xsize + (xsize & 1);
                        break;
                    }

                    case u32_mask_rev('C','M','A','P'):
                    {
                        palette.size = size / 3;
                        for (u32 i = 0; i < palette.size; ++i)
                        {
                            palette[i] = ColorBGRA(p[0], p[1], p[2], 0xff);
                            p += 3;
                        }
                        break;
                    }

                    case u32_mask_rev('C','A','M','G'):
                    {
                        u32 v = p.read32();
                        ham = (v & 0x0800) != 0;
                        ehb = (v & 0x0080) != 0;
                        break;
                    }

                    case u32_mask_rev('B','O','D','Y'):
                    {
                        if (compression)
                        {
                            int scansize = ((xsize + 15) & ~15) / 8 * (nplanes + (mask == 1));
                            int bytes = scansize * ysize;
                            buffer_allocated = new u8[bytes];

                            unpackBits(buffer_allocated, p, bytes, size);
                            buffer = buffer_allocated;
                        }
                        else
                        {
                            buffer = p;
                        }

                        break;
                    }

                    default:
                        break;
                }

                // silence compiler warnings about unused symbols
                MANGO_UNREFERENCED_PARAMETER(xorigin);
                MANGO_UNREFERENCED_PARAMETER(yorigin);
                MANGO_UNREFERENCED_PARAMETER(transcolor);
                MANGO_UNREFERENCED_PARAMETER(xaspect);
                MANGO_UNREFERENCED_PARAMETER(yaspect);
                MANGO_UNREFERENCED_PARAMETER(xscreen);
                MANGO_UNREFERENCED_PARAMETER(yscreen);
            }

            // fix ehb palette
            if (ehb && (palette.size == 32 || palette.size == 64))
            {
                for (int i = 0; i < 32; ++i)
                {
                    palette[i + 32].r = palette[i].r >> 1;
                    palette[i + 32].g = palette[i].g >> 1;
                    palette[i + 32].b = palette[i].b >> 1;
                    palette[i + 32].a = 0xff;
                }
            }

            if (palette.size > 0 && !ham && ptr_palette)
            {
                // client requests for palette and the image has one
                *ptr_palette = palette;

                if (is_pbm)
                {
                    // linear
                    std::memcpy(dest.image, buffer, xsize * ysize);
                }
                else
                {
                    // interlaced
                    Bitmap raw(xsize, ysize, FORMAT_L8);
                    p2c_raw(raw.image, buffer, xsize, ysize, nplanes, mask);
                    std::memcpy(dest.image, raw.image, xsize * ysize);
                }
            }
            else
            {
                // choose pixelformat
                Format format = select_format(nplanes, ham);
                Bitmap temp(xsize, ysize, format);

                // planar-to-chunky conversion
                if (ham)
                {
                    p2c_ham(temp.image, buffer, xsize, ysize, nplanes, palette);
                }
                else
                {
                    if (is_pbm)
                    {
                        // linear
                        if (nplanes <= 8)
                        {
                            expand_palette(temp.image, buffer, xsize, ysize, palette);
                        }
                        else
                        {
                            std::memcpy(temp.image, buffer, temp.stride * ysize);
                        }
                    }
                    else
                    {
                        // interlaced
                        if (nplanes <= 8)
                        {
                            Bitmap raw(xsize, ysize, FORMAT_L8);
                            p2c_raw(raw.image, buffer, xsize, ysize, nplanes, mask);
                            expand_palette(temp.image, raw.image, xsize, ysize, palette);
                        }
                        else
                        {
                            p2c_raw(temp.image, buffer, xsize, ysize, nplanes, mask);
                        }
                    }
                }

			    // NOTE: we could directly decode into dest if the formats match.
                dest.blit(0, 0, temp);
            }

            delete[] buffer_allocated;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango
{

    void registerImageDecoderIFF()
    {
        registerImageDecoder(createInterface, ".iff");
        registerImageDecoder(createInterface, ".lbm");
        registerImageDecoder(createInterface, ".ham");
        registerImageDecoder(createInterface, ".ham8");
        registerImageDecoder(createInterface, ".ilbm");
        registerImageDecoder(createInterface, ".ehb");
    }

} // namespace mango
