/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    GIF decoder source: ImageMagick (readBits function).
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;

	// ------------------------------------------------------------
	// decoder
	// ------------------------------------------------------------

	// Specification:
	// https://www.w3.org/Graphics/GIF/spec-gif89a.txt

	enum
	{
		GIF_IMAGE      = 0x2c,
		GIF_EXTENSION  = 0x21,
		GIF_TERMINATE  = 0x3b,

		PLAIN_TEXT_EXTENSION = 0x01,
		GRAPHICS_CONTROL_EXTENSION = 0xf9,
		COMMENT_EXTENSION = 0xfe,
		APPLICATION_EXTENSION = 0xff,
	};

	struct gif_logical_screen_descriptor
	{
		u16	width = 0;
		u16	height = 0;
		u8	packed = 0;
		u8	background = 0;
		u8  aspect = 0;
		const u8* palette = nullptr;

        const u8* read(const u8* data, const u8* end)
		{
			LittleEndianConstPointer p = data;

			if (p + 7 <= end)
			{
                width      = p.read16();
                height     = p.read16();
                packed     = p.read8();
                background = p.read8();
                aspect     = p.read8();

                if (color_table_flag())
                {
                    palette = p;
                    p += color_table_size() * 3;
                }
			}

			return p;
		}

		int  color_table_size() const { return 1 << ((packed & 0x07) + 1); }
		bool sort_flag()        const { return (packed & 0x08) == 0x08; }
		int  color_resolution() const { return (packed >> 4) & 0x07; }
		bool color_table_flag() const { return (packed & 0x80) == 0x80; }
	};

	struct gif_image_descriptor
	{
		u16	left = 0;
		u16	top = 0;
		u16	width = 0;
		u16	height = 0;
		u8	field = 0;
		const u8* palette = nullptr;

        const u8* read(const u8* data, const u8* end)
		{
            LittleEndianConstPointer p = data;

            if (p + 9 <= end)
            {
                left   = p.read16();
                top    = p.read16();
                width  = p.read16();
                height = p.read16();
                field  = p.read8();

                if (local_color_table())
                {
                    palette = p;
                    p += color_table_size() * 3;
                }
            }

			return p;
		}

		bool interlaced()        const { return (field & 0x40) != 0; }
		bool local_color_table() const { return (field & 0x80) != 0; }
		int  color_table_size()  const { return 1 << ((field & 0x07) + 1); }
	};

	const u8* readBits(u8* dest, int samples, const u8* data, const u8* data_end)
	{
        const u8* p = data;

		const int MAX_STACK_SIZE = 4096;

		u8 data_size = *p++;

		int clear = 1 << data_size;
		int end_of_information = clear + 1;
		int available = clear + 2;

		int code_size = data_size + 1;
		int code_mask = (1 << code_size) - 1;
		int old_code = -1;

		u16 prefix[MAX_STACK_SIZE];
		u8 suffix[MAX_STACK_SIZE];

		u8 pixel_stack[MAX_STACK_SIZE + 1];
		u8* top_stack = pixel_stack;

		for (int code = 0; code < clear; ++code)
		{
			prefix[code] = 0;
			suffix[code] = u8(code);
		}

		// decode gif pixel stream
		int bits = 0;
		u8 count = 0;
		u32 datum = 0;
		u8 first = 0;

		u8* q = dest;
		u8* qend = dest + samples;

		const u8* c = nullptr;

		while (q < qend)
		{
			if (top_stack == pixel_stack)
			{
				if (bits < code_size)
				{
					// load bytes until there is enough bits for a code
					if (!count)
					{
						// read a new data block
						count = *p++;
						if (!count)
						{
							// terminator
							return p;
						}

						c = p;
						p += count;

					}

					datum += (*c++) << bits;
					bits += 8;
					--count;
					continue;
				}

				// get the next code
				int code = datum & code_mask;
				datum >>= code_size;
				bits -= code_size;

				// interpret the code
				if ((code > available) || (code == end_of_information))
				{
					break;
				}

				if (code == clear)
				{
					// reset decoder
					code_size = data_size + 1;
					code_mask = (1 << code_size) - 1;
					available = clear + 2;
					old_code = -1;
					continue;
				}

				if (old_code == -1)
				{
					*top_stack++ = suffix[code];
					old_code = code;
					first = u8(code);
					continue;
				}

				int in_code = code;

				if (code >= available)
				{
					*top_stack++ = first;
					code = old_code;
				}

				while (code >= clear)
				{
					*top_stack++ = suffix[code];
					code = prefix[code];
				}

				first = suffix[code];

				// add a new string to the string table
				if (available >= MAX_STACK_SIZE)
				{
					break;
				}

				*top_stack++ = first;
				prefix[available] = u16(old_code);
				suffix[available] = first;
				++available;

				if (!(available & code_mask) && (available < MAX_STACK_SIZE))
				{
					++code_size;
					code_mask += available;
				}

				old_code = in_code;
			}

			// write sample
			*q++ = *(--top_stack);
		}

		// skip junk in case we early-terminated
		for ( ; p < data_end; )
		{
			u8 s = *p++;
			if (!s)
				break;
			p += s;
		}

		return p;
	}

	void deinterlace(u8* dest, u8* buffer, int width, int height)
	{
		static const int interlace_rate[] = { 8, 8, 4, 2 };
		static const int interlace_start[] = { 0, 4, 2, 1 };

		for (int pass = 0; pass < 4; ++pass)
		{
			const int rate = interlace_rate[pass];
			int j =  interlace_start[pass];

			while (j < height)
			{
				u8* d = dest + j * width;
				std::memcpy(d, buffer, width);
				buffer += width;
				j += rate;
			}
		}
	}

    void blit_raw(Surface& surface, const u8* bits)
	{
		int width = surface.width;
		int height = surface.height;

        for (int y = 0; y < height; ++y)
        {
            u8* dest = surface.address<u8>(0, y);
			std::memcpy(dest, bits, width);
            bits += width;
        }
	}

    void blit_palette(Surface& surface, const u8* bits, const Palette& palette)
    {
		int width = surface.width;
		int height = surface.height;

        for (int y = 0; y < height; ++y)
        {
            u32* dest = surface.address<u32>(0, y);
            for (int x = 0; x < width; ++x)
            {
				ColorBGRA color = palette[bits[x]];
				dest[x] = color;
            }
            bits += width;
        }
    }

    const u8* read_image(const u8* data, const u8* end, const gif_logical_screen_descriptor& screen_desc, Surface& surface, Palette* ptr_palette)
    {
		gif_image_descriptor image_desc;
        data = image_desc.read(data, end);

		Palette palette;

		// choose palette
		if (image_desc.local_color_table())
		{
			// local palette
			palette.size = image_desc.color_table_size();

			for (u32 i = 0; i < palette.size; ++i)
			{
            	u32 r = image_desc.palette[i * 3 + 0];
            	u32 g = image_desc.palette[i * 3 + 1];
            	u32 b = image_desc.palette[i * 3 + 2];
            	palette[i] = ColorBGRA(r, g, b, 0xff);
			}
		}
		else
		{
			// global palette
			palette.size = screen_desc.color_table_size();

			for (u32 i = 0; i < palette.size; ++i)
			{
            	u32 r = screen_desc.palette[i * 3 + 0];
            	u32 g = screen_desc.palette[i * 3 + 1];
            	u32 b = screen_desc.palette[i * 3 + 2];
            	palette[i] = ColorBGRA(r, g, b, 0xff);
			}
		}

		// translucent color
		/* NOTE: transparent samples will be rendered incorrectly if alpha blending is enabled
		palette[screen_desc.background].a = 0;
		*/

        int width = image_desc.width;
        int height = image_desc.height;

		// decode gif bit stream
		int samples = width * height;
		std::unique_ptr<u8[]> bits(new u8[samples]);
		data = readBits(bits.get(), samples, data, end);

        // deinterlace
		if (image_desc.interlaced())
		{
            u8* temp = new u8[width * height];
			deinterlace(temp, bits.get(), width, height);
			bits.reset(temp);
		}

		int x = image_desc.left;
		int y = image_desc.top;
		Surface rect(surface, x, y, width, height);

		if (ptr_palette)
		{
			*ptr_palette = palette;
			blit_raw(rect, bits.get());
		}
		else
		{
			blit_palette(rect, bits.get(), palette);
		}

		return data;
    }

	void read_graphics_control_extension(const u8* p)
	{
		LittleEndianConstPointer x = p;

		u8 packed = *x++;
		int disposal_method = (packed >> 2) & 0x07; // 2 - restore background color, 3 - restore previous
		int user_input_flag = (packed >> 1) & 0x01; // 0 - user input is not expected, 1 - user input is expected
		int transparent_color_flag = packed & 0x01; // pixels with this value are not to be touched

		u16 delay = x.read16(); // delay between frames in 1/100th of seconds (50 = .5 seconds, 100 = 1.0 seconds, etc)
		u8 transparent_color = transparent_color_flag ? *x : 0;

        MANGO_UNREFERENCED(delay);
        MANGO_UNREFERENCED(transparent_color);
        MANGO_UNREFERENCED(user_input_flag);
        MANGO_UNREFERENCED(disposal_method);
	}

	void read_application_extension(const u8* p)
	{
		std::string identifier(reinterpret_cast<const char*>(p), 8);
		MANGO_UNREFERENCED(identifier);
	}

	const u8* read_extension(const u8* p)
	{
		u8 label = *p++;
		u8 size = *p++;
		//printf("    label: %x, size: %d\n", int(label), int(size));

		switch (label)
		{
			case PLAIN_TEXT_EXTENSION:
				break;
			case GRAPHICS_CONTROL_EXTENSION:
				read_graphics_control_extension(p);
				break;
			case COMMENT_EXTENSION:
				break;
			case APPLICATION_EXTENSION:
				read_application_extension(p);
				break;
		}

		for ( ; size; )
		{
			p += size;
			size = *p++;
		}

		return p;
	}

    const u8* read_magic(ImageHeader& header, const u8* data, const u8* end)
    {
		if (data + 6 >= end)
		{
			header.setError("[ImageDecoder.GIF] Incorrect header.");
			return nullptr;
		}

		const char* magic = reinterpret_cast<const char*>(data);
        data += 6;

		if (std::strncmp(magic, "GIF87a", 6) && std::strncmp(magic, "GIF89a", 6))
		{
			header.setError("[ImageDecoder.GIF] Header is missing GIF87a or GIF89a identifier.");
			return nullptr;
		}

		return data;
    }

    const u8* read_chunks(const u8* data, const u8* end, const gif_logical_screen_descriptor& screen_desc, Surface& surface, Palette* ptr_palette)
    {
        while (data < end)
		{
			u8 chunkID = *data++;
			//printf("  chunkID: %x\n", (int)chunkID);
			switch (chunkID)
			{
				case GIF_EXTENSION:
					data = read_extension(data);
					break;

				case GIF_IMAGE:
                    data = read_image(data, end, screen_desc, surface, ptr_palette);
                    return data;

				case GIF_TERMINATE:
                    return nullptr;
			}
		}

		return nullptr;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        Memory m_memory;
		ImageHeader m_header;
        gif_logical_screen_descriptor m_screen_desc;

		std::unique_ptr<u8[]> m_image;
		int m_frame_counter = 0;

		const u8* m_start;
		const u8* m_end;
		const u8* m_data;

        Interface(Memory memory)
        	: m_memory(memory)
            , m_image(nullptr)
        {
			m_start = nullptr;
			m_end = m_memory.address + m_memory.size;
            m_data = m_memory.address;

			m_data = read_magic(m_header, m_data, m_end);
			if (m_header.success)
			{
				m_data = m_screen_desc.read(m_data, m_end);

				m_start = m_data;

				m_header.width   = m_screen_desc.width;
				m_header.height  = m_screen_desc.height;
				m_header.depth   = 0;
				m_header.levels  = 0;
				m_header.faces   = 0;
				m_header.palette = true;
				m_header.format  = FORMAT_B8G8R8A8;
				m_header.compression = TextureCompression::NONE;

				m_image.reset(new u8[m_header.width * m_header.height * 4]);
			}
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ImageDecodeStatus decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

			ImageDecodeStatus status;

            if (!m_header.success)
            {
                status.setError(m_header.info);
                return status;
            }

			Format format = ptr_palette ? FORMAT_L8 : FORMAT_B8G8R8A8;

			int stride = m_header.width * format.bytes();
			Surface target(m_header.width, m_header.height, format, stride, m_image.get());

			status.current_frame_index = m_frame_counter;

			if (m_data)
			{
				m_data = read_chunks(m_data, m_end, m_screen_desc, target, ptr_palette);
				m_frame_counter += (m_data != nullptr);
			}

			dest.blit(0, 0, target);

			if (!m_data)
			{
				// out of data - we reached the end of file
				if (m_frame_counter > 1)
				{
					// there was more than 1 frame so it is animation -> restart animation
					m_frame_counter = 0;
					m_data = m_start;
				}
			}

			status.next_frame_index = m_frame_counter;

            return status;
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

	// ------------------------------------------------------------
	// encoder
	// ------------------------------------------------------------

	struct EncoderState
	{
		u8 data = 0;
		int index = 0;

		int chunkIndex = 0;
		u8 chunk[256];

		void writeBits(LittleEndianStream& s, u32 code, int numbits)
		{
			while (numbits > 0)
			{
				// how many bits still fit into the register
				int size = std::min(numbits, 8 - index);

				// insert the bits into the register
				u32 mask = (1 << size) - 1;
				data |= ((code & mask) << index);

				code >>= size;
				index += size;
				numbits -= size;

				if (index > 7)
				{
					chunk[chunkIndex++] = data;
					if (chunkIndex == 255)
					{
						flushChunk(s);
					}

					data = 0;
					index = 0;
				}
			}
		}

		void flushChunk(LittleEndianStream& s)
		{
			s.write8(chunkIndex);
			s.write(chunk, chunkIndex);
			chunkIndex = 0;
		}

		void terminate(LittleEndianStream& s)
		{
			if (index)
			{
				chunk[chunkIndex++] = data;
			}

			if (chunkIndex > 0)
			{
				flushChunk(s);
			}
		}
	};

	void gif_encode_image_block(LittleEndianStream& s, int depth, int width, int height, int stride, u8* image)
	{
		const int minCodeSize = depth;
		const u32 clearCode = 1 << depth;

		s.write8(minCodeSize);

		std::vector<u16> codetree(4096 * 256, 0);

		s32 curCode = -1;
		u32 codeSize = u32(minCodeSize + 1);
		u32 maxCode = clearCode + 1;

		EncoderState state;

		state.writeBits(s, clearCode, codeSize); // start with a fresh LZW dictionary

		for (int y = 0; y < height; ++y)
		{
			u8* scan = image + y * stride;

			for (int x = 0; x < width; ++x)
			{
				u8 nextValue = scan[x];

				if (curCode < 0)
				{
					// first value in a new run
					curCode = nextValue;
				}
				else if (codetree[curCode * 256 + nextValue])
				{
					// current run already in the dictionary
					curCode = codetree[curCode * 256 + nextValue];
				}
				else
				{
					// finish the current run, write a code
					state.writeBits(s, curCode, codeSize);

					// insert the new run into the dictionary
					codetree[curCode * 256 + nextValue] = u16(++maxCode);

					if (maxCode >= (1ul << codeSize))
					{
						// dictionary entry count has broken a size barrier,
						// we need more bits for codes
						codeSize++;
					}
					if (maxCode == 4095)
					{
						// the dictionary is full, clear it out and begin anew
						state.writeBits(s, clearCode, codeSize); // clear tree
						
						std::memset(codetree.data(), 0, 4096 * 256 * sizeof(u16));
						codeSize = u32(minCodeSize + 1);
						maxCode = clearCode + 1;
					}

					curCode = nextValue;
				}
			}
		}

		// compression footer
		state.writeBits(s, curCode, codeSize);
		state.writeBits(s, clearCode, codeSize);
		state.writeBits(s, clearCode + 1, minCodeSize + 1);
		state.terminate(s);

		s.write8(0); // image block terminator
	}

	void gif_encode_file(Stream& stream, const Surface& surface, const Palette& palette)
	{
		int width = surface.width;
		int height = surface.height;
		int stride = surface.stride;
		u8* image = surface.image;

		LittleEndianStream s = stream;

		// identifier
		s.write("GIF89a", 6);

		// screen descriptor
		s.write16(width);
		s.write16(height);

		u8 packed = 0;
		packed |= 0x7; // color table size as log2(size) - 1 (0 -> 2 colors, 7 -> 256 colors)
		packed |= 0x80; // color table present
		packed |= (0x7 << 4); // color resolution as bits - 1 (0 -> 1 bit, 7 -> 8 bits)
		s.write8(packed);

		s.write8(255); // background color
		s.write8(0); // aspect ratio

		// palette
		for (int i = 0; i < 256; ++i)
		{
			s.write8(palette[i].r);
			s.write8(palette[i].g);
			s.write8(palette[i].b);
		}

		// TODO: write graphics_control_extension to disable translucent color
		// TODO: support the extension in decoder so that we don't have one index being invisible

		// image descriptor
		s.write8(0x2c);

		s.write16(0);
		s.write16(0);
		s.write16(width);
		s.write16(height);

		// local palette
		u8 field = 0;
		s.write8(field);

		gif_encode_image_block(s, 8, width, height, stride, image);

		// end of file
		s.write8(0x3b);
	}

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

		if (options.palette.size > 0)
		{
			if (options.palette.size != 256)
			{
				status.setError("[ImageEncoder.GIF] Incorrect palette size - must be 0 or 256 (size: %d).", options.palette.size);
				return status;
			}

			if (surface.format.isIndexed() || surface.format.bits != 8)
			{
				status.setError("[ImageEncoder.GIF] Incorrect format - must be 8 bit INDEXED (bits: %d).", surface.format.bits);
				return status;
			}

			gif_encode_file(stream, surface, options.palette);
		}
		else
		{
    		Bitmap temp(surface.width, surface.height, IndexedFormat(8));

			image::ColorQuantizeOptions quantize_options;
			quantize_options.dithering = options.dithering;
			quantize_options.quality = options.quality;

			image::ColorQuantizer quantizer;
			quantizer.quantize(temp, surface, quantize_options);

			gif_encode_file(stream, temp, quantizer.palette);
		}

        return status;
    }

} // namespace

namespace mango
{

    void registerImageDecoderGIF()
    {
        registerImageDecoder(createInterface, ".gif");
        registerImageEncoder(imageEncode, ".gif");
    }

} // namespace mango
