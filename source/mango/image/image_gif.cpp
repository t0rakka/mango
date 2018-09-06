/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    GIF decoder source: ImageMagick.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "ImageDecoderGIF: "

namespace
{
    using namespace mango;

	// ------------------------------------------------------------
	// decoder
	// ------------------------------------------------------------

	enum
	{
		GIF_IMAGE      = 0x2c,
		GIF_EXTENSION  = 0x21,
		GIF_TERMINATE  = 0x3b
	};

	struct gif_logical_screen_descriptor
	{
		uint16	width = 0;
		uint16	height = 0;
		uint8	packed = 0;
		uint8	background = 0;
		uint8   aspect = 0;
		uint8* palette = nullptr;

        void read(uint8*& data, uint8* end)
		{
			LittleEndianPointer p = data;

			if (p + 7 < end)
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

            data = p;
		}

		int  color_table_size() const { return 1 << ((packed & 0x07) + 1); }
		bool sort_flag()        const { return (packed & 0x08) == 0x08; }
		int  color_resolution() const { return (packed >> 4) & 0x07; }
		bool color_table_flag() const { return (packed & 0x80) == 0x80; }
	};

	struct gif_image_descriptor
	{
		uint16	left = 0;
		uint16	top = 0;
		uint16	width = 0;
		uint16	height = 0;
		uint8	field = 0;
		uint8* palette = nullptr;

        void read(uint8*& data, uint8* end)
		{
            LittleEndianPointer p = data;

            if (p + 9 < end)
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

            data = p;
		}

		bool interlaced()        const { return (field & 0x40) != 0; }
		bool local_color_table() const { return (field & 0x80) != 0; }
		int  color_table_size()  const { return 1 << ((field & 0x07) + 1); }
	};

	uint8* readBits(uint8*& data, int width, int height)
	{
        uint8* p = data;

		// initialize gif data stream decoder
		const int samples = width * height;
		uint8* q_buffer = new uint8[samples];
		uint8* q_buffer_end = q_buffer + samples;

		const int MaxStackSize = 4096;

		uint8 data_size = *p++;

		int clear = 1 << data_size;
		int end_of_information = clear + 1;
		int available = clear + 2;

		int code_size = data_size + 1;
		int code_mask = (1 << code_size) - 1;
		int old_code = -1;

		uint16 prefix[MaxStackSize];
		uint8 suffix[MaxStackSize];

		uint8 pixel_stack[MaxStackSize + 1];
		uint8* top_stack = pixel_stack;

		for (int code = 0; code < clear; ++code)
		{
			prefix[code] = 0;
			suffix[code] = uint8(code);
		}

		// decode gif pixel stream
		int bits = 0;
		int count = 0;
		uint32 datum = 0;
		uint8 first = 0;
		uint8* q = q_buffer;

		uint8 packet[256];
		uint8* c = NULL;

		while (q < q_buffer_end)
		{
			if (top_stack == pixel_stack)
			{
				if (bits < code_size)
				{
					// load bytes until there is enough bits for a code
					if (!count)
					{
						// read a new data block
						uint8 block_size = *p++;
						count = block_size;

						if (count > 0)
						{
							std::memcpy(packet, p, count);
							p += count;
						}
						else
						{
							break;
						}

						c = packet;
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
					first = uint8(code);
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
				if (available >= MaxStackSize)
				{
					break;
				}

				*top_stack++ = first;
				prefix[available] = uint16(old_code);
				suffix[available] = first;
				++available;

				if (!(available & code_mask) && (available < MaxStackSize))
				{
					++code_size;
					code_mask += available;
				}

				old_code = in_code;
			}

			// write sample
			*q++ = *(--top_stack);
		}

		// read the terminator
		uint8 terminator = *p++;

        data = p;

        if (terminator != 0)
		{
            //delete[] q_buffer;
			//MANGO_EXCEPTION(ID"Terminator missing from the gif stream.");
		}

		return q_buffer;
	}

	void deinterlace(uint8* dest, uint8* buffer, int width, int height)
	{
		static const int interlace_rate[] = { 8, 8, 4, 2 };
		static const int interlace_start[] = { 0, 4, 2, 1 };

		for (int pass = 0; pass < 4; ++pass)
		{
			const int rate = interlace_rate[pass];
			int j =  interlace_start[pass];

			while (j < height)
			{
				uint8* d = dest + j * width;
				std::memcpy(d, buffer, width);
				buffer += width;
				j += rate;
			}
		}
	}

    void blit_palette(Surface& surface, const uint8* bits, const Palette& palette, int width, int height)
    {
        for (int y = 0; y < height; ++y)
        {
            uint32* dest = surface.address<uint32>(0, y);
            for (int x = 0; x < width; ++x)
            {
                dest[x] = palette[bits[x]];
            }
            bits += width;
        }
    }

    void read_image(uint8*& data, uint8* end, const gif_logical_screen_descriptor& desc, Surface& surface, Palette* ptr_palette)
    {
		gif_image_descriptor image_desc;
        image_desc.read(data, end);

		Palette palette;

		// choose palette
		if (image_desc.local_color_table())
		{
			// local palette
			palette.size = image_desc.color_table_size();

			for (uint32 i = 0; i < palette.size; ++i)
			{
            	uint32 r = image_desc.palette[i * 3 + 0];
            	uint32 g = image_desc.palette[i * 3 + 1];
            	uint32 b = image_desc.palette[i * 3 + 2];
            	palette[i] = BGRA(r, g, b, 0xff);
			}
		}
		else
		{
			// global palette
			palette.size = desc.color_table_size();

			for (uint32 i = 0; i < palette.size; ++i)
			{
            	uint32 r = desc.palette[i * 3 + 0];
            	uint32 g = desc.palette[i * 3 + 1];
            	uint32 b = desc.palette[i * 3 + 2];
            	palette[i] = BGRA(r, g, b, 0xff);
			}
		}

		// translucent color
		palette[desc.background].a = 0;

        int width = image_desc.width;
        int height = image_desc.height;

		// decode gif bit stream
		uint8* bits = readBits(data, width, height);

        // deinterlace
		if (image_desc.interlaced())
		{
            uint8* temp = new uint8[width * height];
			deinterlace(temp, bits, width, height);
            delete[] bits;
            bits = temp;
		}

		bool dimensions = surface.width == width && surface.height == height;

		if (ptr_palette && dimensions && surface.format.bits == 8)
		{
			*ptr_palette = palette;
			uint8* dest = surface.address<uint8>(0,0);
			std::memcpy(dest, bits, width * height);
		}
		else
		{
    	    bool suitable = dimensions && surface.format == FORMAT_B8G8R8A8;
        	bool direct = suitable && !(image_desc.left || image_desc.top);

        	if (direct)
        	{
            	blit_palette(surface, bits, palette, width, height);
        	}
        	else
        	{
            	Bitmap temp(width, height, FORMAT_B8G8R8A8);
            	blit_palette(temp, bits, palette, width, height);

            	int x = image_desc.left;
            	int y = image_desc.top;
            	surface.blit(x, y, temp);
        	}
		}

		delete[] bits;
    }

	void read_extension(uint8*& data)
	{
        uint8* p = data;

		++p;

		for (;;)
		{
			uint8 size = *p++;
			p += size;
			if (!size) break;
		}

        data = p;
	}

    void read_magic(uint8*& data, uint8* end)
    {
		if (data + 6 >= end)
		{
			MANGO_EXCEPTION(ID"Out of data.");
		}

		const char* magic = reinterpret_cast<const char*>(data);
        data += 6;

		if (std::strncmp(magic, "GIF87a", 6) && std::strncmp(magic, "GIF89a", 6))
		{
            MANGO_EXCEPTION(ID"Incorrect gif header, missing GIF87a or GIF89a identifier.");
		}
    }

    void read_chunks(uint8* data, uint8* end, const gif_logical_screen_descriptor& screen_desc, Surface& surface, Palette* ptr_palette)
    {
        while (data < end)
		{
			uint8 chunkID = *data++;

			switch (chunkID)
			{
				case GIF_EXTENSION:
					read_extension(data);
					break;

				case GIF_IMAGE:
				{
                    // TODO: Support animation / multiple frames. No random-access to individual frames
                    //       would be practical so the efficient API would be "decode_next_frame()" but
                    //       we don't really intend this interface to be for animations so it's all good. :)
                    // NOTE: Multi-frame GIFs can also store RGB images as 16x16 image_descs with unique palette.
                    //       This requires the decoding target to be unchanged between frames. The "animation"
                    //       will progressively fill the screen_desc. This is a curiosity we don't feel pressed to
                    //       support at this time.
                    read_image(data, end, screen_desc, surface, ptr_palette);
                    return;
				}

				case GIF_TERMINATE:
                    return;
			}
		}
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        Memory m_memory;

        Interface(Memory memory)
        	: m_memory(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            uint8* data = m_memory.address;
			uint8* end = data + m_memory.size;

			read_magic(data, end);

            gif_logical_screen_descriptor screen_desc;
            screen_desc.read(data, end);

            ImageHeader header;

            header.width   = screen_desc.width;
            header.height  = screen_desc.height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = true;
            header.format  = FORMAT_B8G8R8A8;
            header.compression = TextureCompression::NONE;

            return header;
        }

        void decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            uint8* data = m_memory.address;
            uint8* end = data + m_memory.size;

            read_magic(data, end);
            gif_logical_screen_descriptor screen_desc;
            screen_desc.read(data, end);
            read_chunks(data, end, screen_desc, dest, ptr_palette);
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango
{

    void registerImageDecoderGIF()
    {
        registerImageDecoder(createInterface, "gif");
    }

} // namespace mango
