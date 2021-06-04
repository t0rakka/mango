/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
	The lzw_decode() function is based on Jean-Marc Lienher / STB decoder.
	The symbol resolver is iterative instead of recursive as in the original.
*/
#include <algorithm>
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

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

		PLAIN_TEXT_EXTENSION       = 0x01,
		GRAPHICS_CONTROL_EXTENSION = 0xf9,
		COMMENT_EXTENSION          = 0xfe,
		APPLICATION_EXTENSION      = 0xff,
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

		bool color_table_flag() const { return (packed & 0x80) == 0x80; }
		int  color_resolution() const { return (packed >> 4) & 0x07; }
		bool sort_flag()        const { return (packed & 0x08) == 0x08; }
		int  color_table_size() const { return 1 << ((packed & 0x07) + 1); }
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

                if (color_table_flag())
                {
                    palette = p;
                    p += color_table_size() * 3;
                }
            }

			return p;
		}

		bool color_table_flag() const { return (field & 0x80) != 0; }
		bool interlaced()       const { return (field & 0x40) != 0; }
		int  color_table_size() const { return 1 << ((field & 0x07) + 1); }
	};

	struct gif_state
	{
		gif_logical_screen_descriptor screen_desc;
		gif_image_descriptor image_desc;

		bool first_frame = true;

		u16 delay = 2; // default: 50 Hz
		int disposal_method = 0;
		int user_input_flag = 0;
		int transparent_color_flag = 0;
		u8 transparent_color = 0;
	};

	const u8* lzw_decode(u8* dest, u8* dest_end, const u8* src, const u8* src_end)
	{
		constexpr int MAX_STACK_SIZE = 8192;
		constexpr int MAX_AVAILABLE = 0xfff;

		struct lzw
		{
			s16 prefix;
			u8 first;
			u8 suffix;
		} codes[MAX_STACK_SIZE];

		const u8 minimum_codesize = *src++;
		if (minimum_codesize > 12)
		{
			return nullptr;
		}

		const s32 code_clear = 1 << minimum_codesize;
		const s32 code_eoi = code_clear + 1;

		for (int i = 0; i < code_clear; ++i)
		{
			const u8 initcode = i;
			codes[i].prefix = -1;
			codes[i].first = initcode;
			codes[i].suffix = initcode;
		}

		s32 codesize = minimum_codesize + 1;
		s32 codemask = (1 << codesize) - 1;

		s32 available = code_clear + 2;
		s32 oldcode = -1;

		s32 data = 0;
		s32 data_bits = 0;
		s32 length = 0;

		for (;;)
		{
			if (data_bits < codesize)
			{
				if (!length)
				{
					// start compression block
					length = *src++;
					if (!length)
					{
						// terminator
						return src;
					}

					if (src + length >= src_end)
					{
						// overflow
						return nullptr;
					}
				}

				// fill register
				--length;
				data |= s32(*src++) << data_bits;
				data_bits += 8;
			}
			else
			{
				// consume register
				s32 code = data & codemask;
				data >>= codesize;
				data_bits -= codesize;

				if (code == code_clear)
				{
					// clear code
					codesize = minimum_codesize + 1;
					codemask = (1 << codesize) - 1;
					available = code_clear + 2;
					oldcode = -1;
				}
				else if (code == code_eoi)
				{
					// end of information
					src += length;
					while ((length = *src++) > 0)
					{
						src += length;
					}
					return src;
				}
				else if (code <= available)
				{
					if (oldcode >= 0)
					{
						lzw* p = &codes[available++];
						if (available >= MAX_STACK_SIZE)
						{
							// too many codes
							return nullptr;
						}

						p->prefix = s16(oldcode);
						p->first = codes[oldcode].first;
						p->suffix = (code == available) ? p->first : codes[code].first;
					}
					else if (code == available)
					{
						// illegal code
						return nullptr;
					}

					oldcode = code;

					// remember current address
					u8* start = dest;

					// resolve symbols
					for (;;)
					{
						if (dest < dest_end)
						{
							u8 sample = codes[code].suffix;
							*dest++ = sample;
						}

						if (codes[code].prefix < 0)
							break;

						code = codes[code].prefix;
					}

					// reverse symbols
					std::reverse(start, dest);

					if (available > codemask && available < MAX_AVAILABLE)
					{
						++codesize;
						codemask |= (codemask + 1);
					}
				}
				else
				{
					// illegal code
					return nullptr;
				}
			}
		}

		return src;
	}

	void deinterlace(u8* dest, const u8* buffer, int width, int height)
	{
		for (int pass = 0; pass < 4; ++pass)
		{
			const int rate = std::min(8, 16 >> pass); // 8, 8, 4, 2
			const int start = (8 >> pass) & 0x7;      // 0, 4, 2, 1

			for (int y = start; y < height; y += rate)
			{
				std::memcpy(dest + y * width, buffer, width);
				buffer += width;
			}
		}
	}

	void scanline_copy_indices(u8* dest, const u8* src, int width, const Palette& palette, u8 transparent)
	{
		MANGO_UNREFERENCED(palette);
		MANGO_UNREFERENCED(transparent);

		std::memcpy(dest, src, width);
	}

	void scanline_blend_indices(u8* dest, const u8* src, int width, const Palette& palette, u8 transparent)
	{
		MANGO_UNREFERENCED(palette);

		for (int x = 0; x < width; ++x)
		{
			u8 sample = src[x];
			if (sample != transparent)
			{
				dest[x] = sample;
			}
		}
	}

	void scanline_copy_palette(u8* dest8, const u8* src, int width, const Palette& palette, u8 transparent)
	{
		MANGO_UNREFERENCED(transparent);

		u32* dest = reinterpret_cast<u32*>(dest8);

		for (int x = 0; x < width; ++x)
		{
			u8 sample = src[x];
			dest[x] = palette[sample];
		}
	}

	void scanline_blend_palette(u8* dest8, const u8* src, int width, const Palette& palette, u8 transparent)
	{
		u32* dest = reinterpret_cast<u32*>(dest8);

		for (int x = 0; x < width; ++x)
		{
			u8 sample = src[x];
			if (sample != transparent)
			{
				dest[x] = palette[sample];
			}
		}
	}

    const u8* read_image(const u8* data, const u8* end, 
	                     gif_state& state, 
	                     Surface& surface, Palette* ptr_palette)
    {
		gif_image_descriptor image_desc;
        data = image_desc.read(data, end);

		Palette palette;

		// choose palette
		if (image_desc.color_table_flag())
		{
			// local palette
			palette.size = image_desc.color_table_size();

			for (u32 i = 0; i < palette.size; ++i)
			{
            	u32 r = image_desc.palette[i * 3 + 0];
            	u32 g = image_desc.palette[i * 3 + 1];
            	u32 b = image_desc.palette[i * 3 + 2];
            	palette[i] = Color(r, g, b, 0xff);
			}
		}
		else
		{
			// global palette
			palette.size = state.screen_desc.color_table_size();

			for (u32 i = 0; i < palette.size; ++i)
			{
            	u32 r = state.screen_desc.palette[i * 3 + 0];
            	u32 g = state.screen_desc.palette[i * 3 + 1];
            	u32 b = state.screen_desc.palette[i * 3 + 2];
            	palette[i] = Color(r, g, b, 0xff);
			}
		}

		// transparency
		u8 background = state.screen_desc.background;
		u8 transparent = state.transparent_color;

		if (state.transparent_color_flag)
		{
			palette[transparent].r = palette[background].r;
			palette[transparent].g = palette[background].g;
			palette[transparent].b = palette[background].b;
			palette[transparent].a = 0;
		}

		bool blend = !state.first_frame && state.transparent_color_flag;

		int x = image_desc.left;
		int y = image_desc.top;
        int width = image_desc.width;
        int height = image_desc.height;

		// decode gif bit stream
		int bytes = width * height;
		std::unique_ptr<u8[]> bits(new u8[bytes]);
		data = lzw_decode(bits.get(), bits.get() + bytes, data, end);

		// deinterlace
		if (image_desc.interlaced())
		{
			u8* temp = new u8[bytes];
			deinterlace(temp, bits.get(), width, height);
			bits.reset(temp);
		}

		void (*func)(u8*, const u8*, int , const Palette& , u8) = nullptr;

		if (ptr_palette)
		{
			*ptr_palette = palette;
			func = blend ? scanline_blend_indices : scanline_copy_indices;
		}
		else
		{
			func = blend ? scanline_blend_palette : scanline_copy_palette;
		}

		// NOTE: clipping happens with some image files; don't be too clever and "optimize" this later :)
		Surface rect(surface, x, y, width, height);
		u8* src = bits.get();

		for (int y = 0; y < rect.height; ++y)
		{
			u8* dest = rect.address<u8>(0, y);
			func(dest, src, rect.width, palette, transparent);
			src += width;
		}

		return data;
    }

	void read_graphics_control_extension(const u8* p, gif_state& state)
	{
		LittleEndianConstPointer x = p;

		u8 packed = *x++;

		state.disposal_method = (packed >> 2) & 0x07; // 1 - do not dispose, 2 - restore background color, 3 - restore previous
		state.user_input_flag = (packed >> 1) & 0x01; // 0 - user input is not expected, 1 - user input is expected
		state.transparent_color_flag = packed & 0x01; // pixels with this value are not to be touched

		state.delay = x.read16(); // delay between frames in 1/100th of seconds (50 = .5 seconds, 100 = 1.0 seconds, etc)
		state.transparent_color = state.transparent_color_flag ? *x : 0;

		debugPrint("      delay: %d, dispose: %d, transparent: %s (%d)\n",
			state.delay,
			state.disposal_method,
			state.transparent_color_flag ? "YES" : "NO",
			state.transparent_color);
	}

	void read_application_extension(const u8* p)
	{
		std::string identifier(reinterpret_cast<const char*>(p), 8);
		MANGO_UNREFERENCED(identifier);
	}

	const u8* read_extension(const u8* p, gif_state& state)
	{
		u8 label = *p++;
		u8 size = *p++;
		debugPrint("    label: %x, size: %d\n", int(label), int(size));

		switch (label)
		{
			case PLAIN_TEXT_EXTENSION:
				break;
			case GRAPHICS_CONTROL_EXTENSION:
				read_graphics_control_extension(p, state);
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

    const u8* read_chunks(const u8* data, const u8* end, gif_state& state, Surface& surface, Palette* ptr_palette)
    {
        while (data < end)
		{
			u8 chunkID = *data++;
			debugPrint("  chunkID: %x\n", int(chunkID));
			switch (chunkID)
			{
				case GIF_EXTENSION:
					data = read_extension(data, state);
					break;

				case GIF_IMAGE:
                    data = read_image(data, end, state, surface, ptr_palette);
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
        ConstMemory m_memory;
		ImageHeader m_header;
		gif_state m_state;

		std::unique_ptr<u8[]> m_image;
		int m_frame_counter = 0;

		const u8* m_start;
		const u8* m_end;
		const u8* m_data;

        Interface(ConstMemory memory)
        	: m_memory(memory)
            , m_image(nullptr)
        {
			m_start = nullptr;
			m_end = m_memory.address + m_memory.size;
            m_data = m_memory.address;

			m_data = read_magic(m_header, m_data, m_end);
			if (m_header.success)
			{
				m_data = m_state.screen_desc.read(m_data, m_end);

				m_start = m_data;

				m_header.width   = m_state.screen_desc.width;
				m_header.height  = m_state.screen_desc.height;
				m_header.depth   = 0;
				m_header.levels  = 0;
				m_header.faces   = 0;
				m_header.palette = true;
				m_header.format  = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
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

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
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

			Format format = options.palette ? LuminanceFormat(8, Format::UNORM, 8, 0)
			                                : Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

			size_t stride = m_header.width * format.bytes();
			Surface target(m_header.width, m_header.height, format, stride, m_image.get());

			status.current_frame_index = m_frame_counter;

			if (m_data)
			{
				m_state.first_frame = status.current_frame_index == 0;
				m_data = read_chunks(m_data, m_end, m_state, target, options.palette);
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

			status.frame_delay_numerator = m_state.delay;
			status.frame_delay_denominator = 100;

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
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

	void gif_encode_image_block(LittleEndianStream& s, int depth, Surface surface)
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

		for (int y = 0; y < surface.height; ++y)
		{
			u8* scan = surface.address(0, y);

			for (int x = 0; x < surface.width; ++x)
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
		LittleEndianStream s = stream;

		// identifier
		s.write("GIF89a", 6);

		// screen descriptor
		s.write16(surface.width);
		s.write16(surface.height);

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

		// TODO: gif animations will repeat this section
		{
			// image descriptor
			s.write8(GIF_IMAGE);

			s.write16(0);
			s.write16(0);
			s.write16(surface.width);
			s.write16(surface.height);

			// local palette
			u8 field = 0;
			s.write8(field);

			gif_encode_image_block(s, 8, surface);
		}

		// end of file
		s.write8(GIF_TERMINATE);
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

			image::ColorQuantizer quantizer(surface, options.quality);
			quantizer.quantize(temp, surface, options.dithering);

			gif_encode_file(stream, temp, quantizer.getPalette());
		}

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderGIF()
    {
        registerImageDecoder(createInterface, ".gif");
        registerImageEncoder(imageEncode, ".gif");
    }

} // namespace mango::image
