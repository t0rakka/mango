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
#include "../../external/libwebp/src/webp/encode.h"

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // webp formats
    // ------------------------------------------------------------

    struct WebPFormat
    {
        typedef uint8_t* (*DecodeFunc)(const uint8_t*, size_t, uint8_t*, size_t, int);
        typedef size_t (*EncodeFunc)(const uint8_t* image, int width, int height, int stride, float quality_factor, uint8_t** output);
        typedef size_t (*EncodeLosslessFunc)(const uint8_t* image, int width, int height, int stride, uint8_t** output);

        Format format;
        DecodeFunc decode_func;
        EncodeFunc encode_func;
        EncodeLosslessFunc encode_lossless_func;

        u8* decode(const Surface& dest, Memory memory) const
        {
            int bytes = dest.height * dest.stride;
            return decode_func(memory.address, memory.size, dest.image, bytes, dest.stride);

        }

        size_t encode(uint8_t** output, const Surface& source, float quality, bool lossless)
        {
            return lossless ? encode_lossless_func(source.image, source.width, source.height, source.stride, output)
                            : encode_func(source.image, source.width, source.height, source.stride, quality, output);
        }
    };

    static const WebPFormat g_formats[] =
    {
        { Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), WebPDecodeRGBAInto, WebPEncodeRGBA, WebPEncodeLosslessRGBA },
        { Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8), WebPDecodeBGRAInto, WebPEncodeBGRA, WebPEncodeLosslessBGRA },
        { Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0), WebPDecodeRGBInto, WebPEncodeRGB, WebPEncodeLosslessRGB },
        { Format(24, Format::UNORM, Format::BGR, 8, 8, 8, 0), WebPDecodeBGRInto, WebPEncodeBGR, WebPEncodeLosslessBGR },
    };

    WebPFormat webpDefaultFormat(bool alpha)
    {
        return alpha ? g_formats[0] : g_formats[2];
    }

    WebPFormat webpFindFormat(const Format& format)
    {
        WebPFormat best = webpDefaultFormat(format.isAlpha());
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
            m_header.format  = webpDefaultFormat(true).format;
            m_header.compression = TextureCompression::NONE;
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ImageDecodeStatus decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(palette);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            WebPFormat wpformat = webpFindFormat(dest.format);
            bool matching_formats = wpformat.format == dest.format;
            bool matching_dimensions = m_header.width == dest.width &&
                                       m_header.height == dest.height;

            status.direct = matching_formats && matching_dimensions;

            uint8_t* output = nullptr;

            if (status.direct)
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
            else
            {
                status.success = true;
            }

            return status;
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    void imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        WebPFormat wpformat = webpFindFormat(surface.format);
        bool matching_formats = wpformat.format == surface.format;

        float quality = options.quality * 100.0f;
        bool lossless = options.lossless;

        u8* output = nullptr;
        size_t bytes = 0;

        if (matching_formats)
        {
            // Direct encoding
            bytes = wpformat.encode(&output, surface, quality, lossless);
        }
        else
        {
            // Color conversion encoding
            Bitmap temp(surface.width, surface.height, wpformat.format);
            temp.blit(0, 0, surface);
            bytes = wpformat.encode(&output, temp, quality, lossless);
        }

        if (output)
        {
            if (bytes)
            {
                stream.write(output, bytes);
            }
            WebPFree(output);
        }

        if (!bytes)
        {
            MANGO_EXCEPTION("[ImageDecoder.WEBP] Encoding failed.");
        }
    }

} // namespace

namespace mango
{

    void registerImageDecoderWEBP()
    {
        registerImageDecoder(createInterface, ".webp");
        registerImageEncoder(imageEncode, ".webp");
    }

} // namespace mango
