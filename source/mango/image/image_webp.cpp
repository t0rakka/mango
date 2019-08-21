/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#include "../../external/libwebp/src/webp/decode.h"

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // webp formats
    // ------------------------------------------------------------

    struct WebPFormat
    {
        typedef uint8_t* (*DecodeFunc)(const uint8_t*, size_t, uint8_t*, size_t, int);

        Format format;
        DecodeFunc decode_func;

        u8* decode(const Surface& dest, Memory memory) const
        {
            int bytes = dest.height * dest.stride;
            return decode_func(memory.address, memory.size, dest.image, bytes, dest.stride);

        }
    };

    static const WebPFormat g_formats[] =
    {
        { Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), WebPDecodeRGBAInto },
        { Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), WebPDecodeBGRAInto },
        { Format(32, Format::UNORM, Format::ARGB, 8, 8, 8, 8), WebPDecodeARGBInto },
        { Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), WebPDecodeRGBInto },
        { Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), WebPDecodeBGRInto },
    };

    WebPFormat webpDefaultFormat()
    {
        return g_formats[0];
    }

    WebPFormat webpFindFormat(const Format& format)
    {
        WebPFormat best = webpDefaultFormat();
        for (const auto& current : g_formats)
        {
            if (current.format == format)
            {
                best = current;
                break;
            }
        }
        return best;
    }

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
            int width;
            int height;
            if (!WebPGetInfo(m_memory.address, m_memory.size, &width, &height))
            {
                MANGO_EXCEPTION("[ImageDecoder.WEBP] Incorrect header.");
            }

            m_header.width   = width;
            m_header.height  = height;
            m_header.depth   = 0;
            m_header.levels  = 0;
            m_header.faces   = 0;
			m_header.palette = false;
            m_header.format  = webpDefaultFormat().format;
            m_header.compression = TextureCompression::NONE;
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

            WebPFormat wpformat = webpFindFormat(dest.format);
            bool matching_formats = wpformat.format == dest.format;
            bool matching_dimensions = m_header.width == dest.width &&
                                       m_header.height == dest.height;

            uint8_t* output = nullptr;

            if (matching_formats && matching_dimensions)
            {
                // Direct decoding
                output = wpformat.decode(dest, m_memory);
            }
            else
            {
                // Color conversion decoding
                Bitmap temp(m_header.width, m_header.height, wpformat.format);
                output = wpformat.decode(temp, m_memory);
                if (output)
                {
                    dest.blit(0, 0, temp);
                }
            }

            if (!output)
            {
                MANGO_EXCEPTION("[ImageDecoder.WEBP] Decoding failed.");
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

    void registerImageDecoderWEBP()
    {
        registerImageDecoder(createInterface, ".webp");
    }

} // namespace mango
