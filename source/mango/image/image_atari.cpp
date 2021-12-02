/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    ATARI decoders copyright (C) 2011 Toni Lönnberg. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;

        Interface(ConstMemory memory)
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

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_header.success)
            {
                status.setError(m_header.info);
                return status;
            }

            status.direct = dest.format == m_header.format &&
                            dest.width >= m_header.width &&
                            dest.height >= m_header.height;

            if (status.direct)
            {
                decodeImage(dest);
            }
            else
            {
                Bitmap temp(m_header.width, m_header.height, m_header.format);
                decodeImage(temp);
                dest.blit(0, 0, temp);
            }

            return status;
        }

        virtual void decodeImage(const Surface& dest) = 0;
    };

    // ------------------------------------------------------------
    // ST helper functions
    // ------------------------------------------------------------

    Color convert_atari_color(u16 atari_color)
    {
        Color color;

        color.r = ((atari_color & 0x700) >> 3) | ((atari_color & 0x800) >> 7) | ((atari_color & 0x700) >> 7) | ((atari_color & 0x800) >> 11);
        color.g = ((atari_color & 0x70 ) << 1) | ((atari_color & 0x80 ) >> 3) | ((atari_color & 0x70 ) >> 3) | ((atari_color & 0x80 ) >> 7);
        color.b = ((atari_color & 0x7  ) << 5) | ((atari_color & 0x8  ) << 1) | ((atari_color & 0x7  ) << 1) | ((atari_color & 0x8  ) >> 3);
        color.a = 0xff;

        return color;
    }

    void resolve_palette(const Surface& s, int width, int height, const u8* image, const Palette& palette)
    {
        for (int y = 0; y < height; ++y)
        {
            Color* scan = s.address<Color>(0, y);
            const u8* src = image + y * width;

            for (int x = 0; x < width; ++x)
            {
                scan[x] = palette[src[x]];
            }
        }
    }

    // ------------------------------------------------------------
	// ImageDecoder: Degas/Degas Elite
	// ------------------------------------------------------------

	void degas_decompress(u8* buffer, const u8* input, const u8* input_end, int scansize)
	{
		u8* buffer_end = buffer + scansize;

		for ( ; buffer < buffer_end && input < input_end; )
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

	struct header_degas
	{
		int width = 0;
		int height = 0;
        int bitplanes = 0;
        bool compressed = false;

        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            BigEndianConstPointer p = data;

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
                error = "[ImageDecoder.ATARI] unsupported resolution.";
                return nullptr;
            }

            compressed = (resolution_data & 0x8000) != 0;

            if (!compressed)
            {
                if (size < 32034)
                {
                    error = "[ImageDecoder.ATARI] Out of data.";
                    return nullptr;
                }
            }

            return p;
        }

        void decode(const Surface& s, Palette& palette, const u8* data, const u8* end)
        {
            Buffer tempImage(width * height, 0);

            const int words_per_scan = bitplanes == 1 ? 40 : 80;

            if (compressed)
            {
                Buffer buffer(32000);
			    degas_decompress(buffer, data, end, 32000);

                BigEndianConstPointer p = buffer.data();

                for (int y = 0; y < height; ++y)
                {
					u8* image = tempImage + y * width;

					for (int j = 0; j < bitplanes; ++j)
                    {
                        for (int k = 0; k < words_per_scan / bitplanes; ++k)
                        {
                            u16 word = p.read16();

                            for (int l = 15; l >= 0; --l)
                            {
                                image[k * 16 + (15 - l)] |= (((word & (1 << l)) >> l) << j);
                            }
                        }
                    }
                }
			}
            else
            {
                const u16be* buffer = reinterpret_cast<const u16be *>(data);

                if (bitplanes == 1)
                {
                    palette.color[0] = 0xffeeeeee;
                    palette.color[1] = 0xff000000;
                }

                for (int y = 0; y < height; ++y)
                {
					u8* image = tempImage.data() + y * width;
					int yoffset = y * (bitplanes == 1 ? 40 : 80);

					for (int x = 0; x < width; ++x)
                    {
                        int x_offset = 15 - (x & 15);
                        int word_offset = (x >> 4) * bitplanes + yoffset;

                        u8 index = 0;
                        for (int i = 0; i < bitplanes; ++i)
                        {
                            u16 v = buffer[word_offset + i];
                            int bit_pattern = (v >> x_offset) & 0x1;
                            index |= (bit_pattern << i);
                        }

						image[x] = index;
                    }
                }
			}

            resolve_palette(s, width, height, tempImage, palette);
        }
	};

    struct InterfaceDEGAS : Interface
    {
        header_degas m_degas_header;
        const u8* m_data;

        InterfaceDEGAS(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_degas_header.parse(memory.address, memory.size);
            if (m_data)
            {
                m_header.width  = m_degas_header.width;
                m_header.height = m_degas_header.height;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                m_header.setError(m_degas_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
			if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;
            BigEndianConstPointer p = m_data;

            // read palette
            Palette palette;
            palette.size = 1 << m_degas_header.bitplanes;

            for (int i = 0; i < 16; ++i)
            {
                u16 color = p.read16();
                palette[i] = convert_atari_color(color);
            }

            // decode image
            m_degas_header.decode(s, palette, p, end);
        }
    };

    ImageDecoderInterface* createInterfaceDEGAS(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceDEGAS(memory);
        return x;
    }

	// ------------------------------------------------------------
    // ImageDecoder: NEOchrome
	// ------------------------------------------------------------

	struct header_neo
	{
		int width = 0;
		int height = 0;
        int bitplanes;

        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            if (size != 32128)
            {
                error = "[ImageDecoder.ATARI] Incorrect number of bytes.";
                return nullptr;
            }

            BigEndianConstPointer p = data;

            u16 flag = p.read16();
            u16 resolution_data = p.read16();

            if (flag)
            {
                error = "[ImageDecoder.ATARI] Incorrect flags.";
                return nullptr;
            }
            else
            {
                if (resolution_data == 0)
                {
                    width = 320;
                    height = 200;
                    bitplanes = 4;
                }
                else if (resolution_data == 1)
                {
                    width = 640;
                    height = 200;
                    bitplanes = 2;
                }
                else if (resolution_data == 2)
                {
                    width = 640;
                    height = 400;
                    bitplanes = 1;
                }
                else
                {
                    error = "[ImageDecoder.ATARI] Unsupported resolution.";
                    return nullptr;
                }
            }

            return p;
        }

        void decode(const Surface& s, const u8* data, const u8* end)
        {
            BigEndianConstPointer p = data;

            // read palette
            Palette palette;
            palette.size = 16;

            for (int i = 0; i < 16; ++i)
            {
                u16 palette_color = p.read16();
                palette[i] = convert_atari_color(palette_color);
            }

            // patch palette
            if (bitplanes == 1)
            {
                palette.color[0] = 0xffeeeeee;
                palette.color[1] = 0xff000000;
            }

            // Skip unnecessary data
            p += 12 + 2 + 2 + 2 + 2 + 2 + 2 + 2 + 66;

            const int num_words = 16000;
            Buffer buffer(width * height);

            for (int i = 0; i < (num_words / bitplanes); ++i)
            {
                u16 word[4];

                for (int j = 0; j < bitplanes; ++j)
                {
                    word[j] = p.read16();

                    if (p > end)
                    {
                        return;
                    }
                }

                for (int j = 15; j >= 0; --j)
                {
                    u8 index = 0;

                    for (int k = 0; k < bitplanes; ++k)
                    {
                        index |= (((word[k] & (1 << j)) >> j) << k);
                    }

                    buffer[(i * 16) + (15 - j)] = index;
                }
            }

            resolve_palette(s, width, height, buffer, palette);
        }
	};

    struct InterfaceNEO : Interface
    {
        header_neo m_neo_header;
        const u8* m_data;

        InterfaceNEO(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_neo_header.parse(memory.address, memory.size);
            if (m_data)
            {
                m_header.width  = m_neo_header.width;
                m_header.height = m_neo_header.height;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                m_header.setError(m_neo_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;

            m_neo_header.decode(s, m_data, end);
        }
    };

    ImageDecoderInterface* createInterfaceNEO(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceNEO(memory);
        return x;
    }

	// ------------------------------------------------------------
    // ImageDecoder: Spectrum 512
	// ------------------------------------------------------------

    u8 find_spectrum_palette_index(u32 x, u8 c)
    {
        u32 t = 10 * c;

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

	void spu_decompress(u8* buffer, const u8* input, int scansize, int insize)
	{
		u8* buffer_end = buffer + scansize;
		const u8* input_end = input + insize;

		for ( ; buffer < buffer_end && input < input_end; )
		{
			u8 v = *input++;

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
		int width = 0;
		int height = 0;
        int bitplanes = 4;
        bool compressed = false;
        int length_of_data_bit_map = 0;
        int length_of_color_bit_map = 0;
        
        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            if (size < 12)
            {
                error = "[ImageDecoder.ATARI] Incorrect header size.";
                return nullptr;
            }

            BigEndianConstPointer p = data;

            u16 flag = p.read16();

            if (flag == 0x5350)
            {
                compressed = true;
                p += 2; // skip reserved
                length_of_data_bit_map = p.read32();
                length_of_color_bit_map = p.read32();

                const size_t total_size = 12 + length_of_data_bit_map + length_of_color_bit_map;
                const size_t total_size_other = total_size + 78; // HACK: some files also have this size..

                if (size != total_size && size != total_size_other)
                {
                    error = "[ImageDecoder.ATARI] Incorrect number of bytes.";
                    return nullptr;
                }
            }
            else
            {
                if (size != 51104)
                {
                    error = "[ImageDecoder.ATARI] Incorrect number of bytes.";
                    return nullptr;
                }

                p -= sizeof(u16);
            }

            width = 320;
            height = 200;

            return p;
        }

        void decode(const Surface& s, const u8* data, const u8* end)
        {
            BigEndianConstPointer p = data;

            Buffer bitmap(width * height, 0);
            std::vector<Color> palette(16 * 3 * (height - 1), Color(0, 0, 0, 0xff));

            int num_words = 16000;
            int words_per_scan = 20;

            if (compressed)
            {
                Buffer buffer(31840);
                spu_decompress(buffer.data(), p, 31840, length_of_data_bit_map);

                p = buffer.data();
                end = buffer.data() + 31840;

                for (int i = 0; i < bitplanes; ++i)
                {
                    for (int j = 1; j < height; ++j)
                    {
                        for (int k = 0; k < words_per_scan; ++k)
                        {
                            u16 word = p.read16();

                            if (p > end)
                            {
                                return;
                            }

                            for (int l = 15; l >= 0; --l)
                            {
                                bitmap[(j * width) + (k * 16) + (15 - l)] |= (((word & (1 << l)) >> l) << i);
                            }
                        }
                    }
                }

                p = data + length_of_data_bit_map;
                end = p + length_of_color_bit_map;

                int palette_set = 0;
                while (p < end)
                {
                    u16 vector = p.read16();

                    for (int i = 0; i < 16; ++i)
                    {
                        if (vector & (1 << i))
                        {
                            int index = (palette_set * 16) + i;
                            u16 palette_color = p.read16();
                            palette[index] = convert_atari_color(palette_color);
                        }
                    }

                    ++palette_set;
                }
            }
            else
            {
                for (int i = 0; i < (num_words / bitplanes); ++i)
                {
                    u16 word[4] = { 0 };

                    for (int j = 0; j < bitplanes; ++j)
                    {
                        word[j] = p.read16();

                        if (p > end)
                        {
                            return;
                        }
                    }

                    for (int j = 15; j >= 0; --j)
                    {
                        u8 index = 0;

                        for (int k = 0; k < bitplanes; ++k)
                        {
                            index |= (((word[k] & (1 << j)) >> j) << k);
                        }

                        bitmap[(i * 16) + (15 - j)] = index;
                    }
                }

                for (int i = 0; i < (height - 1); ++i)
                {
                    for (int j = 0; j < 16 * 3; ++j)
                    {
                        u16 palette_color = p.read16();
                        int index = (i * 16 * 3) + j;

                        if (p > end)
                        {
                            return;
                        }

                        palette[index] = convert_atari_color(palette_color);
                    }
                }
            }

            // HACK: clear first scanline - not sure what we should do here..
            std::memset(s.address<u8>(0, 0), 0, width * 4);

            // Resolve palette
            for (int y = 1; y < height; ++y)
            {
                Color* image = s.address<Color>(0, y);

                for (int x = 0; x < width; ++x)
                {
                    u8 palette_index = bitmap[y * width + x];
                    palette_index = find_spectrum_palette_index(x, palette_index);

                    int index = (y - 1) * 16 * 3 + palette_index;
                    image[x] = palette[index];
                }
            }
        }
	};

    struct InterfaceSPU : Interface
    {
        header_spu m_spu_header;
        const u8* m_data;

        InterfaceSPU(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_spu_header.parse(memory.address, memory.size);
            if (m_data)
            {
                m_header.width  = m_spu_header.width;
                m_header.height = m_spu_header.height;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                m_header.setError(m_spu_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* data = m_data;
            const u8* end = m_memory.address + m_memory.size;

            m_spu_header.decode(s, data, end);
        }
    };

    ImageDecoderInterface* createInterfaceSPU(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceSPU(memory);
        return x;
    }

	// ------------------------------------------------------------
    // ImageDecoder: Crack Art
	// ------------------------------------------------------------

	void ca_decompress(u8* buffer, const u8* input, const int scansize, const u8 escape_char, const u16 offset)
	{
		u8* buffer_start = buffer;
		u8* buffer_end = buffer + scansize;

        int count = scansize;

		while (count > 0)
		{
			u8 v = *input++;
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
		int width = 0;
		int height = 0;
        int bitplanes = 0;
        bool compressed = false;

        std::string error;

        const u8* parse(const u8* data, size_t size)
        {
            BigEndianConstPointer p = data;

            if (std::memcmp(p, "CA", 2))
            {
                error = "[ImageDecoder.ATARI] Incorrect header.";
                return nullptr;
            }

            p += 2;

            compressed = p.read8() != 0;
            u8 resolution = p.read8();

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
                error = "[ImageDecoder.ATARI] Unsupported resolution.";
                return nullptr;
            }

            if (!compressed)
            {
                if (size < size_t(32000 + 2 + 1 + 1 + (1 << bitplanes)))
                {
                    error = "[ImageDecoder.ATARI] Out of data.";
                    return nullptr;
                }
            }

            return p;
        }

        void decode(const Surface& s, const u8* data, const u8* end)
        {
            BigEndianConstPointer p = data;

            Palette palette;
            palette.size = 1 << bitplanes;

            for (int i = 0; i < int(palette.size); ++i)
            {
                u16 palette_color = p.read16();
                palette[i] = convert_atari_color(palette_color);
            }

            Buffer temp;
            const u8* buffer = p;

            if (compressed)
            {
                const u8 escape_char = p.read8();
                const u8 initial_value = p.read8();
                const u16 offset = p.read16() & 0x7fff;

                temp.reset(32000, initial_value);
			    ca_decompress(temp.data(), p, 32000, escape_char, offset);

                buffer = temp.data();
                end = temp.data() + 32000;
            }

            p = buffer;

            const int num_words = 16000;
            Buffer image(width * height);

            for (int i = 0; i < (num_words / bitplanes); ++i)
            {
                u16 word[4] = { 0 };

                for (int j = 0; j < bitplanes; ++j)
                {
                    word[j] = p.read16();

                    if (p > end)
                    {
                        return;
                    }
                }

                for (int j = 15; j >= 0; --j)
                {
                    u8 index = 0;

                    for (int k = 0; k < bitplanes; ++k)
                    {
                        index |= (((word[k] & (1 << j)) >> j) << k);
                    }

                    image[(i * 16) + (15 - j)] = index;
                }
            }

            resolve_palette(s, width, height, image, palette);
        }
	};

    struct InterfaceCA : Interface
    {
        header_ca m_ca_header;
        const u8* m_data;

        InterfaceCA(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_ca_header.parse(memory.address, memory.size);
            if (m_data)
            {
                m_header.width  = m_ca_header.width;
                m_header.height = m_ca_header.height;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                m_header.setError(m_ca_header.error);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;
            m_ca_header.decode(s, m_data, end);
		}
    };

    ImageDecoderInterface* createInterfaceCA(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceCA(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderATARI()
    {
        // Degas/Degas Elite
        registerImageDecoder(createInterfaceDEGAS, ".pi1");
        registerImageDecoder(createInterfaceDEGAS, ".pi2");
        registerImageDecoder(createInterfaceDEGAS, ".pi3");
        registerImageDecoder(createInterfaceDEGAS, ".pc1");
        registerImageDecoder(createInterfaceDEGAS, ".pc2");
        registerImageDecoder(createInterfaceDEGAS, ".pc3");

        // NEOchrome
        registerImageDecoder(createInterfaceNEO, ".neo");

        // Spectrum 512
        registerImageDecoder(createInterfaceSPU, ".spu");
        registerImageDecoder(createInterfaceSPU, ".spc");

        // Crack Art
        registerImageDecoder(createInterfaceCA, ".ca1");
        registerImageDecoder(createInterfaceCA, ".ca2");
        registerImageDecoder(createInterfaceCA, ".ca3");
    }

} // namespace mango::image
