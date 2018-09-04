/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    ATARI decoders copyright (C) 2011 Toni Lönnberg. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "ImageStream.ATARI: "

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        Memory m_memory;
        ImageHeader m_header;

        Interface(Memory memory)
            : m_memory(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            if (dest.format == m_header.format && dest.width >= m_header.width && dest.height >= m_header.height)
            {
                decodeImage(dest);
            }
            else
            {
                Bitmap temp(m_header.width, m_header.height, m_header.format);
                decodeImage(temp);
                dest.blit(0, 0, temp);
            }
        }

        virtual void decodeImage(Surface& dest) = 0;
    };

    // ------------------------------------------------------------
	// ImageDecoder: Degas/Degas Elite
	// ------------------------------------------------------------

	void depack_packbits(u8* buffer, const u8* input, int scansize, int insize)
	{
		u8* buffer_end = buffer + scansize;
		const u8* input_end = input + insize;

		for ( ; buffer < buffer_end && input < input_end; )
		{
			uint8 v = *input++;

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

    BGRA convert_atari_color(u16 atari_color)
    {
        BGRA color;

        color.b = ((atari_color & 0x7  ) << 5) | ((atari_color & 0x8  ) << 1) | ((atari_color & 0x7  ) << 1) | ((atari_color & 0x8  ) >> 3);
        color.g = ((atari_color & 0x70 ) << 1) | ((atari_color & 0x80 ) >> 3) | ((atari_color & 0x70 ) >> 3) | ((atari_color & 0x80 ) >> 7);
        color.r = ((atari_color & 0x700) >> 3) | ((atari_color & 0x800) >> 7) | ((atari_color & 0x700) >> 7) | ((atari_color & 0x800) >> 11);
        color.a = 0xff;

        return color;
    }

	struct header_degas
	{
		int width = 0;
		int height = 0;
        int bitplanes = 0;
        bool compressed = false;

        u8* parse(u8* data, size_t size)
        {
            BigEndianPointer p = data;

            u16 resolution_data = p.read16();
            u8 resolution = (resolution_data & 0x3);

            if (resolution == 0)
            {
                width = 320;
                height = 200;
                bitplanes = 4;
            }
            else if (resolution == 1)
            {
                width = 640;
                height = 200;
                bitplanes = 2;
            }
            else if (resolution == 2)
            {
                width = 640;
                height = 400;
                bitplanes = 1;
            }
            else
            {
                MANGO_EXCEPTION(ID"unsupported resolution.");
            }

            compressed = (resolution_data & 0x8000) != 0;

            if (!compressed)
            {
                if (size < 32034)
                {
                    return nullptr;
                }
            }

            return p;
        }
	};

    struct InterfaceDEGAS : Interface
    {
        header_degas m_degas_header;
        u8* m_data;

        InterfaceDEGAS(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_degas_header.parse(memory.address, memory.size);
            if (m_data)
            {
                m_header.width  = m_degas_header.width;
                m_header.height = m_degas_header.height;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& dest) override
        {
            if (!m_data)
                return;

            u8* data = m_data;
            u8* end = m_memory.address + m_memory.size;

            BigEndianPointer p = data;

            Palette palette;
            palette.size = 1 << m_degas_header.bitplanes;

            for (int i = 0; i < 16; ++i)
            {
                u16 palette_color = p.read16();
                palette[i] = convert_atari_color(palette_color);
            }

            std::vector<u8> imageVector(m_header.width * m_header.height);
            u8* image = imageVector.data();
            std::memset(image, 0, m_header.width * m_header.height);

            const int words_per_scan = m_degas_header.bitplanes == 1 ? 40 : 80;
            printf("-- compressed: %d, planes: %d \n", m_degas_header.compressed, m_degas_header.bitplanes);

            if (m_degas_header.compressed)
            {
                std::vector<u8> buffer(32000);
			    depack_packbits(buffer.data(), p, 32000, int(end - p));

                p = buffer.data();

                for (int y = 0; y < m_header.height; ++y)
                {
                    for (int j = 0; j < m_degas_header.bitplanes; ++j)
                    {
                        for (int k = 0; k < words_per_scan / m_degas_header.bitplanes; ++k)
                        {
                            u16 word = p.read16();

                            if (p > end)
                                return;

                            for (int l = 15; l >= 0; --l)
                            {
                                image[(y * m_header.width) + (k * 16) + (15 - l)] |= (((word & (1 << l)) >> l) << j);
                            }
                        }
                    }
                }
            }
            else
            {
                data = p;
                const uint16be* buffer = reinterpret_cast<const uint16be *>(data);
                
                if (m_degas_header.bitplanes == 1)
                {
                    palette.color[0] = 0xffeeeeee;
                    palette.color[1] = 0xff000000;
                }

                for (int y = 0; y < m_header.height; ++y)
                {
                    int yoffset = y * (m_degas_header.bitplanes == 1 ? 40 : 80);
                    for (int x = 0; x < m_header.width; ++x)
                    {
                        int x_offset = 15 - (x & 15);
                        int word_offset = (x >> 4) * m_degas_header.bitplanes + yoffset;

                        u8 index = 0;
                        for (int i = 0; i < m_degas_header.bitplanes; ++i)
                        {
                            u16 v = buffer[word_offset + i];
                            int bit_pattern = (v >> x_offset) & 0x1;
                            index |= (bit_pattern << i);
                        }
                        image[x + y * m_header.width] = index;
                    }
                }
            }

            // Resolve palette
            for (int y = 0; y < m_header.height; ++y)
            {
                u32* dst = dest.address<u32>(0, y);
                u8* src = image + y * m_header.width;
                for (int x = 0; x < m_header.width; ++x)
                {
                    u8 index = src[x];
                    dst[x] = palette.color[index];
                }
            }
        }
    };

    ImageDecoderInterface* createInterfaceDEGAS(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceDEGAS(memory);
        return x;
    }

	// ------------------------------------------------------------
    // ImageDecoder: NEOchrome
	// ------------------------------------------------------------

#if 0

	struct header_neo
	{
		int width;
		int height;
        int bitplanes;
	};

	const uint8* read_header_neo(header_neo& header, const uint8* data, int size)
	{
		header.width = 0;
		header.height = 0;

        if (size != 32128)
        {
            return NULL;
        }

        infilter xf(data);

        uint16 flag = xf.read<uint16>();
        uint16 resolution_data = xf.read<uint16>();

        if (flag)
        {
            return NULL;
        }
        else
        {
            if (resolution_data == 0)
            {
                header.width = 320;
                header.height = 200;
                header.bitplanes = 4;
            }
            else if (resolution_data == 1)
            {
                header.width = 640;
                header.height = 200;
                header.bitplanes = 2;
            }
            else if (resolution_data == 2)
            {
                header.width = 640;
                header.height = 400;
                header.bitplanes = 1;
            }
            else
            {
                FUSIONCORE_EXCEPTION("Neochrome header: unsupported resolution.");
            }
        }

        return xf;
	}

	imageheader neo_header(stream* s)
	{
		imageheader image_header;

		int size = int(s->size());
		const uint8* data = s->read(size);

		header_neo header;
		data = read_header_neo(header, data, size);

        if (data)
        {
		    image_header.width = header.width;
		    image_header.height = header.height;

            ucolor *palette = NULL;
            image_header.format = pixelformat(palette);
        }
        else
        {
            image_header.width = 0;
            image_header.height = 0;
        }

		return image_header;
	}

	surface* neo_load(stream* s, bool thumbnail)
	{
		(void) thumbnail;

        int i, j, k;
		int size = int(s->size());
		const uint8* data = s->read(size);
        const uint8* end = data + size;

		header_neo header;
        data = read_header_neo(header, data, size);

        if (data)
        {
            infilter xf(data);

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));

            for (i = 0; i < 16; ++i)
            {
                uint16 palette_color = xf.read<uint16>();
                palette[i] = convert_atari_color(palette_color);
            }

            // Skip unnecessary data
            xf += 12 + 2 + 2 + 2 + 2 + 2 + 2 + 2 + 66;

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
		    uint8* image = so->lock<uint8>();

            int num_words = 16000;
            for (i = 0; i < (num_words / header.bitplanes); ++i)
            {
                uint16 word[4];

                for (j = 0; j < header.bitplanes; ++j)
                {
                    word[j] = xf.read<uint16>();

                    if (xf > end)
                    {
                        so->unlock();
                        so->release();
                        return NULL;
                    }
                }

                for (j = 15; j >= 0; --j)
                {
                    uint8 index = 0;

                    for (k = 0; k < header.bitplanes; ++k)
                    {
                        index |= (((word[k] & (1 << j)) >> j) << k);
                    }

                    image[(i * 16) + (15 - j)] = index;
                }
            }

		    so->unlock();

		    return so;
	    }

        return NULL;
    }

#endif

    struct InterfaceNEO : Interface
    {
        InterfaceNEO(Memory memory)
            : Interface(memory)
        {
            m_header.width   = 0; // TODO
            m_header.height  = 0; // TODO
            m_header.format  = Format(); // TODO
        }

        void decodeImage(Surface& dest) override
        {
            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceNEO(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceNEO(memory);
        return x;
    }

	// ------------------------------------------------------------
    // ImageDecoder: Spectrum 512
	// ------------------------------------------------------------

#if 0

	void depack_spu(uint8* buffer, const uint8* input, int scansize, int insize)
	{
		uint8* buffer_end = buffer + scansize;
		const uint8* input_end = input + insize;

		for (; buffer < buffer_end && input < input_end;)
		{
			uint8 v = *input++;

			if (v >= 128)
			{
				const int n = 258 - v;
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
		}
	}

	struct header_spu
	{
		int width;
		int height;
        int bitplanes;
        bool compressed;
        int length_of_data_bit_map;
        int length_of_color_bit_map;
	};

	const uint8* read_header_spu(header_spu& header, const uint8* data, int size)
	{
		header.width = 0;
		header.height = 0;
        header.bitplanes = 4;
        header.compressed = false;
        header.length_of_data_bit_map = 0;
        header.length_of_color_bit_map = 0;

        infilter xf(data);

        uint16 flag = xf.read<uint16>();

        if (flag == 0x5350)
        {
            header.compressed = true;
            xf.read<uint16>();  // skip reserved word
            header.length_of_data_bit_map = xf.read<uint32>();
            header.length_of_color_bit_map = xf.read<uint32>();

            if (size != 12 + header.length_of_data_bit_map + header.length_of_color_bit_map)
            {
                return NULL;
            }
        }
        else
        {
            if (size != 51104)
            {
                return NULL;
            }

            xf -= sizeof(uint16);
        }

        header.width = 320;
        header.height = 200;

        return xf;
	}

	imageheader spu_header(stream* s)
	{
		imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;
        image_header.format = pixelformat::argb8888;

		int size = int(s->size());
		const uint8* data = s->read(size);

        if (size > 12)
        {
		    header_spu header;
		    data = read_header_spu(header, data, size);

            if (data)
            {
		        image_header.width = header.width;
		        image_header.height = header.height;
            }
        }

		return image_header;
	}

    uint8 find_spectrum_palette_index(uint32 x, uint8 c)
    {
        uint32 t = 10 * c;

        if (c & 1)
            t -= 5;
        else
            t++;

        if (x < t)
            return c;
        if (x >= t + 160)
            return c + 32;

        return c + 16;
    }

	surface* spu_load(stream* s, bool thumbnail)
	{
		(void) thumbnail;

        int i, j, k, l;
		int size = int(s->size());
		const uint8* data = s->read(size);
        const uint8* end = data + size;

		header_spu header;
        data = read_header_spu(header, data, size);

        if (data)
        {
            infilter xf(data);

            surface* so = bitmap::create(header.width, header.height, pixelformat::argb8888);
		    ucolor* image = so->lock<ucolor>();
            std::memset(image, 0, header.width * header.height * sizeof(ucolor));

            uint8* bitmap = new uint8[header.width * header.height];
            ucolor* palette = new ucolor[16 * 3 * (header.height - 1)];
            std::memset(bitmap, 0, header.width * header.height);

            for (i = 0; i < 16 * 3 * (header.height - 1); ++i)
            {
                palette[i].a = 0xff;
                palette[i].r = 0;
                palette[i].g = 0;
                palette[i].b = 0;
            }

            int num_words = 16000;
            int words_per_scan = 20;
            if (header.compressed)
            {
			    uint8* buffer = new uint8[31840];
                depack_spu(buffer, xf, 31840, header.length_of_data_bit_map);

                xf = buffer;
                end = buffer + 31840;

                for (i = 0; i < header.bitplanes; ++i)
                {
                    for (j = 1; j < header.height; ++j)
                    {
                        for (k = 0; k < words_per_scan; ++k)
                        {
                            uint16 word = xf.read<uint16>();

                            if (xf > end)
                            {
                                so->unlock();
                                so->release();
                                delete [] bitmap;
                                delete [] palette;
                                delete [] buffer;
                                return NULL;
                            }

                            for (l = 15; l >= 0; --l)
                            {
                                bitmap[(j * header.width) + (k * 16) + (15 - l)] |= (((word & (1 << l)) >> l) << i);
                            }
                        }
                    }
                }

                xf = data + header.length_of_data_bit_map;
                end = xf + header.length_of_color_bit_map;

                int palette_set = 0;
                while (xf < end)
                {
                    uint16 vector = xf.read<uint16>();

                    for (i = 0; i < 16; ++i)
                    {
                        if (vector & (1 << i))
                        {
                            int index = (palette_set * 16) + i;
                            uint16 palette_color = xf.read<uint16>();
                            palette[index] = convert_atari_color(palette_color);
                        }
                    }

                    ++palette_set;
                }

                delete [] buffer;
            }
            else
            {
                for (i = 0; i < (num_words / header.bitplanes); ++i)
                {
                    uint16 word[4] = {0};

                    for (j = 0; j < header.bitplanes; ++j)
                    {
                        word[j] = xf.read<uint16>();

                        if (xf > end)
                        {
                            so->unlock();
                            so->release();
                            delete [] bitmap;
                            delete [] palette;
                            return NULL;
                        }
                    }

                    for (j = 15; j >= 0; --j)
                    {
                        uint8 index = 0;

                        for (k = 0; k < header.bitplanes; ++k)
                        {
                            index |= (((word[k] & (1 << j)) >> j) << k);
                        }

                        bitmap[(i * 16) + (15 - j)] = index;
                    }
                }

                for (i = 0; i < (header.height - 1); ++i)
                {
                    for (j = 0; j < 16 * 3; ++j)
                    {
                        uint16 palette_color = xf.read<uint16>();
                        int index = (i * 16 * 3) + j;

                        if (xf > end)
                        {
                            so->unlock();
                            so->release();
                            delete [] bitmap;
                            delete [] palette;
                            return NULL;
                        }

                        palette[index] = convert_atari_color(palette_color);
                    }
                }
            }

            for (i = 1; i < header.height; ++i)
            {
                for (j = 0; j < header.width; ++j)
                {
                    uint8 palette_index = bitmap[(i * header.width) + j];
                    palette_index = find_spectrum_palette_index(j, palette_index);

                    int offset = (i * header.width) + j;
                    int index = (i - 1) * 16 * 3 + palette_index;

                    image[offset].a = 0xff;
                    image[offset].r = palette[index].r;
                    image[offset].g = palette[index].g;
                    image[offset].b = palette[index].b;
                }
            }

            delete [] bitmap;
            delete [] palette;

		    so->unlock();

		    return so;
	    }

        return NULL;
    }

#endif

    struct InterfaceSPU : Interface
    {
        InterfaceSPU(Memory memory)
            : Interface(memory)
        {
            m_header.width   = 0; // TODO
            m_header.height  = 0; // TODO
            m_header.format  = Format(); // TODO
        }

        void decodeImage(Surface& dest) override
        {
            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceSPU(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceSPU(memory);
        return x;
    }

	// ------------------------------------------------------------
    // ImageDecoder: Crack Art
	// ------------------------------------------------------------

#if 0

	void depack_ca(uint8* buffer, const uint8* input, const int scansize, const int insize, const uint8 escape_char, const uint16 offset)
	{
        (void)insize;

		uint8* buffer_start = buffer;
		uint8* buffer_end = buffer + scansize;

        int count = scansize;
		while (count > 0)
		{
			uint8 v = *input++;
			if (v != escape_char)
			{
				*buffer = v;
				--count;

			    buffer += offset;
			    if (buffer >= buffer_end)
			    {
				    ++buffer_start;
				    buffer = buffer_start;
			    }
			}
			else
			{
				v = *input++;
				if (v == 0)
				{
					int n = *input++;
					++n;

					v = *input++;

					if (n > count)
					{
						n = count;
					}

					count -= n;

					while (n > 0)
					{
						*buffer = v;
						--n;

			            buffer += offset;
			            if (buffer >= buffer_end)
			            {
				            ++buffer_start;
				            buffer = buffer_start;
			            }
					}
				}
				else if (v == 1)
				{
					int n = *input++;
					n <<= 8;
					n += *input++;
					++n;

					v = *input++;

					if (n > count)
					{
						n = count;
					}

					count -= n;

					while (n > 0)
					{
						*buffer = v;
						--n;

			            buffer += offset;
			            if (buffer >= buffer_end)
			            {
				            ++buffer_start;
				            buffer = buffer_start;
			            }
					}
				}
				else if (v == 2)
				{
					int n;
					v = *input++;

					if (v == 0)
					{
						n = count;
					}
					else
					{
						n = v;
						n <<= 8;
						n += *input++;
                        ++n;

						if (n > count)
						{
							n = count;
						}
					}

					count -= n;

					while (n > 0)
					{
						--n;

			            buffer += offset;
			            if (buffer >= buffer_end)
			            {
				            ++buffer_start;
				            buffer = buffer_start;
			            }
					}
				}
				else if (v == escape_char)
				{
					*buffer = v;
					--count;

		            buffer += offset;
		            if (buffer >= buffer_end)
		            {
			            ++buffer_start;
			            buffer = buffer_start;
		            }
                }
				else
				{
					int n = v;
					++n;

					v = *input++;

					if (n > count)
					{
						n = count;
					}

					count -= n;

					while (n > 0)
					{
						*buffer = v;
						--n;

			            buffer += offset;
			            if (buffer >= buffer_end)
			            {
				            ++buffer_start;
				            buffer = buffer_start;
			            }
					}
				}
			}
		}
	}

	struct header_ca
	{
		int width;
		int height;
        int bitplanes;
        bool compressed;
	};

	const uint8* read_header_ca(header_ca& header, const uint8* data, int size)
	{
		header.width = 0;
		header.height = 0;

        infilter xf(data);

        uint8 keyword[] = "CA";
        if (std::memcmp(keyword, xf, sizeof(keyword) - 1) == 0)
        {
            xf += 2;

            header.compressed = xf.read<uint8>() ? true : false;
            uint8 resolution = xf.read<uint8>();

            if (resolution == 0)
            {
                header.width = 320;
                header.height = 200;
                header.bitplanes = 4;
            }
            else if (resolution == 1)
            {
                header.width = 640;
                header.height = 200;
                header.bitplanes = 2;
            }
            else if (resolution == 2)
            {
                header.width = 640;
                header.height = 400;
                header.bitplanes = 1;
            }
            else
            {
                FUSIONCORE_EXCEPTION("Crack Art: unsupported resolution.");
            }

            if (!header.compressed)
            {
                if (size < 32000 + 2 + 1 + 1 + (1 << header.bitplanes))
                {
                    return NULL;
                }
            }

            return xf;
        }

        return NULL;
	}

	imageheader ca_header(stream* s)
	{
		imageheader image_header;

		int size = int(s->size());
		const uint8* data = s->read(size);

		header_ca header;
		data = read_header_ca(header, data, size);

        if (data)
        {
		    image_header.width = header.width;
		    image_header.height = header.height;

            ucolor *palette = NULL;
            image_header.format = pixelformat(palette);
        }
        else
        {
            image_header.width = 0;
            image_header.height = 0;
        }

		return image_header;
	}

	surface* ca_load(stream* s, bool thumbnail)
	{
		(void) thumbnail;

        int i, j, k;
		int size = int(s->size());
		const uint8* data = s->read(size);
        const uint8* end = data + size;
        uint8* temp = NULL;

		header_ca header;
        data = read_header_ca(header, data, size);

        if (data)
        {
            infilter xf(data);

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));

            for (i = 0; i < (1 << header.bitplanes); ++i)
            {
                uint16 palette_color = xf.read<uint16>();
                palette[i] = convert_atari_color(palette_color);
            }

            const uint8* buffer = xf;

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
		    uint8* image = so->lock<uint8>();

            if (header.compressed)
            {
                const uint8 escape_char = xf.read<uint8>();
                const uint8 initial_value = xf.read<uint8>();
                const uint16 offset = xf.read<uint16>() & 0x7fff;

			    temp = new uint8[32000];
                std::memset(temp, initial_value, 32000);

			    depack_ca(temp, xf, 32000, int(end - xf), escape_char, offset);

                buffer = temp;
                end = temp + 32000;
            }

            xf = buffer;

            int num_words = 16000;
            for (i = 0; i < (num_words / header.bitplanes); ++i)
            {
                uint16 word[4];

                for (j = 0; j < header.bitplanes; ++j)
                {
                    word[j] = xf.read<uint16>();

                    if (xf > end)
                    {
                        so->unlock();
                        so->release();
                        return NULL;
                    }
                }

                for (j = 15; j >= 0; --j)
                {
                    uint8 index = 0;

                    for (k = 0; k < header.bitplanes; ++k)
                    {
                        index |= (((word[k] & (1 << j)) >> j) << k);
                    }

                    image[(i * 16) + (15 - j)] = index;
                }
            }

            if (temp)
            {
                delete [] temp;
            }

		    so->unlock();

		    return so;
        }

        return NULL;
	}
    
#endif

    struct InterfaceCA : Interface
    {
        InterfaceCA(Memory memory)
            : Interface(memory)
        {
            m_header.width   = 0; // TODO
            m_header.height  = 0; // TODO
            m_header.format  = Format(); // TODO
        }

        void decodeImage(Surface& dest) override
        {
            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceCA(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceCA(memory);
        return x;
    }

} // namespace

namespace mango
{

    void registerImageDecoderATARI()
    {
        // Degas/Degas Elite
        registerImageDecoder(createInterfaceDEGAS, "pi1");
        registerImageDecoder(createInterfaceDEGAS, "pi2");
        registerImageDecoder(createInterfaceDEGAS, "pi3");
        registerImageDecoder(createInterfaceDEGAS, "pc1");
        registerImageDecoder(createInterfaceDEGAS, "pc2");
        registerImageDecoder(createInterfaceDEGAS, "pc3");

        // NEOchrome
        registerImageDecoder(createInterfaceNEO, "neo");

        // Spectrum 512
        registerImageDecoder(createInterfaceSPU, "spu");
        registerImageDecoder(createInterfaceSPU, "spc");

        // Crack Art
        registerImageDecoder(createInterfaceCA, "ca1");
        registerImageDecoder(createInterfaceCA, "ca2");
        registerImageDecoder(createInterfaceCA, "ca3");
    }

} // namespace mango
