/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "ImageDecoder.HDR: "

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // tokenizer
    // ------------------------------------------------------------

	std::string readline(const uint8*& buffer, const uint8* end)
	{
		const uint8* p = buffer;

		int endsize = 1;

		// scan for endline
		for (; buffer < end;)
		{
			uint8 v = *p++;

			// Unix ("\n")
			if (v == '\n')
				break;

			// MacOS ("\r")
			if (v == '\r')
			{
				// Windows ("\r\n")
				if (*p == '\n')
				{
					++endsize;
					++p;
				}

				break;
			}
		}

		int size = int(p - buffer) - endsize;

		std::string msg(reinterpret_cast<const char*>(buffer), size);

		buffer = p;

		return msg;
	}

	inline bool whitespace(char v)
	{
		return v == ' ' || v == '\t' || v == '=';
	}

	void insert_token(std::vector<std::string>& tokens, const char* text, int size)
	{
		if (size > 0)
		{
			std::string msg(text, size);
			tokens.push_back(msg);
		}
	}

	std::vector<std::string> tokenize(const std::string& line)
	{
		std::vector<std::string> tokens;

		const char* p = line.c_str();
		const char* endline = p + line.length();

		for ( ; p < endline;)
		{
			// skip whitespaces
			for ( ;; ++p)
			{
				char v = *p;
				if (!v) return tokens;
				if (!whitespace(v)) break;
			}

			const char* begin = p;

			// seek next whitespace
			for ( ;; ++p)
			{
				char v = *p;

				if (!v)
				{
					int size = int(p - begin);
					insert_token(tokens, begin, size);
					return tokens;
				}

				if (whitespace(v))
					break;
			}

			int size = int(p - begin);
			insert_token(tokens, begin, size);
		}

		return tokens;
	}

    // ------------------------------------------------------------
    // decoder
    // ------------------------------------------------------------

	void write_rgbe(float* buffer, uint8 r, uint8 g, uint8 b, uint8 e)
	{
		float v = e ? std::ldexp(1.0f, e - 136) : 0;
		buffer[0] = r * v;
		buffer[1] = g * v;
		buffer[2] = b * v;
		buffer[3] = 1.0f;
	}

	struct radheader
	{
		enum radformat
		{
			rad_rle_rgbe,
			rad_unsupported
		} format;

		int width;
		int height;
		bool xflip;
		bool yflip;
		float exposure;

		void parse(const uint8*& buffer, const uint8* end)
		{
			format   = rad_unsupported;
			width    = 0;
			height   = 0;
			exposure = 1.0f;

            std::string id = readline(buffer, end);
			if (id != "#?RADIANCE")
			{
				MANGO_EXCEPTION(ID"Incorrect radiance header.");
			}

			for ( ;; )
			{
                std::string ln = readline(buffer, end);
				if (ln.empty())
					break;

				std::vector<std::string> tokens = tokenize(ln);

				if (tokens[0] == "FORMAT")
				{
					if (tokens.size() != 2)
					{
						MANGO_EXCEPTION(ID"Incorrect radiance header (format).");
					}

					if (tokens[1] == "32-bit_rle_rgbe")
						format = rad_rle_rgbe;
				}
				else if (tokens[1] == "EXPOSURE")
				{
					if (tokens.size() != 2)
					{
						MANGO_EXCEPTION(ID"Incorrect radiance header (exposure).");
					}

					exposure = float(std::atof(tokens[1].c_str()));
				}
			}

			if (format == rad_unsupported)
			{
				MANGO_EXCEPTION(ID"Incorrect or unsupported format.");
			}

            std::string dims = readline(buffer, end);
			std::vector<std::string> tokens = tokenize(dims);

			if (tokens.size() != 4)
			{
				MANGO_EXCEPTION(ID"Incorrect radiance header (dimensions).");
			}

			for (int i = 0; i < 2; ++i)
			{
				int index = i * 2;
				if (tokens[index] == "+Y")
				{
					yflip = false;
					height = std::atoi(tokens[index + 1].c_str());
				}
				else if (tokens[index] == "-Y")
				{
					yflip = true;
					height = std::atoi(tokens[index + 1].c_str());
				}
				else if (tokens[index] == "+X")
				{
					xflip = false;
					width = std::atoi(tokens[index + 1].c_str());
				}
				else if (tokens[index] == "-X")
				{
					xflip = true;
					width = std::atoi(tokens[index + 1].c_str());
				}
				else
				{
					MANGO_EXCEPTION(ID"Incorrect radiance header (dimensions).");
				}
			}

			if (!width || !height)
			{
				MANGO_EXCEPTION(ID"Incorrect radiance header (dimensions).");
			}
		}
	};

    void hdr_decode(Surface& surface, const uint8* data)
    {
        Buffer buffer(surface.width * 4);

		for (int y = 0; y < surface.height; ++y)
		{
			if (data[0] != 2 || data[1] != 2 || data[2] & 0x80)
			{
				MANGO_EXCEPTION(ID"Incorrect rle_rgbe stream (wrong header).");
			}

			if (((data[2] << 8) | data[3]) != surface.width)
			{
				MANGO_EXCEPTION(ID"Incorrect rle_rgbe stream (wrong scan).");
			}

			data += 4;
			uint8* p = buffer;

			for (int i = 0; i < 4; ++i)
			{
				uint8* end = buffer + (i + 1) * surface.width;

				while (p < end)
				{
					int count = data[0];
					int value = data[1];
                    data += 2;

					if (count > 128)
					{
						count -= 128;
						if (!count || count > (end - p))
						{
							MANGO_EXCEPTION(ID"Incorrect rle_rgbe stream (rle count).");
						}

						std::memset(p, value, count);
						p += count;
					}
					else
					{
						if (!count || count > (end - p))
						{
							MANGO_EXCEPTION(ID"Incorrect rle_rgbe stream (rle count).");
						}

						*p++ = static_cast<uint8>(value);

						if (--count > 0)
						{
							std::memcpy(p, data, count);
							data += count;
							p += count;
						}
					}
				}
			}

            float* image = surface.address<float>(0, y);

			for (int x = 0; x < surface.width; ++x)
			{
				uint8 r = buffer[x + surface.width * 0];
				uint8 g = buffer[x + surface.width * 1];
				uint8 b = buffer[x + surface.width * 2];
				uint8 e = buffer[x + surface.width * 3];
				write_rgbe(image, r, g, b, e);
				image += 4;
			}
		}
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        radheader m_header;
        const uint8* m_data;

        Interface(Memory memory)
        {
            const uint8* data = memory.address;
            const uint8* end = memory.address + memory.size;
            m_header.parse(data, end);
            m_data = data;
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            ImageHeader header;

            header.width   = m_header.width;
            header.height  = m_header.height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.format  = FORMAT_RGBA32F;
            header.compression = TextureCompression::NONE;

            return header;
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            const Format decoder_format = FORMAT_RGBA32F;

            if (dest.format == decoder_format)
            {
                hdr_decode(dest, m_data);
            }
            else
            {
                Bitmap temp(m_header.width, m_header.height, decoder_format);
                hdr_decode(temp, m_data);
                dest.blit(0, 0, temp);
            }
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

    void registerImageDecoderHDR()
    {
        registerImageDecoder(createInterface, ".hdr");
    }

} // namespace mango
