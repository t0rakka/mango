/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#include "../../external/libwebp/src/webp/decode.h"
#include "../../external/libwebp/src/webp/encode.h"

// https://developers.google.com/speed/webp/docs/api

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // webp formats
    // ------------------------------------------------------------

    struct WebPFormat
    {
        using DecodeFunc = uint8_t* (*)(const uint8_t*, size_t, uint8_t*, size_t, int);
        using EncodeFunc = size_t (*)(const uint8_t* image, int width, int height, int stride, float quality_factor, uint8_t** output);
        using EncodeLosslessFunc = size_t (*)(const uint8_t* image, int width, int height, int stride, uint8_t** output);

        Format format;
        DecodeFunc decode_func;
        EncodeFunc encode_func;
        EncodeLosslessFunc encode_lossless_func;

        u8* decode(const Surface& dest, ConstMemory memory) const
        {
            size_t bytes = dest.height * dest.stride;
            return decode_func(memory.address, memory.size, dest.image, bytes, int(dest.stride));

        }

        size_t encode(uint8_t** output, const Surface& source, float quality, bool lossless)
        {
            return lossless ? encode_lossless_func(source.image, source.width, source.height, int(source.stride), output)
                            : encode_func(source.image, source.width, source.height, int(source.stride), quality, output);
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
        ConstMemory m_memory;
        ImageHeader m_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            int width;
            int height;
            if (!WebPGetInfo(m_memory.address, m_memory.size, &width, &height))
            {
                m_header.setError("[ImageDecoder.WEBP] Incorrect header.");
            }
            else
            {
                m_header.width   = width;
                m_header.height  = height;
                m_header.depth   = 0;
                m_header.levels  = 0;
                m_header.faces   = 0;
                m_header.palette = false;
                m_header.format  = webpDefaultFormat(true).format;
                m_header.compression = TextureCompression::NONE;
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
                status.setError("[ImageDecoder.WEBP] Decoding failed.");
            }

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

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
            Bitmap temp(surface, wpformat.format);
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
            status.setError("[ImageEncoder.WEBP] Encoding failed.");
        }

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderWEBP()
    {
        registerImageDecoder(createInterface, ".webp");
        registerImageEncoder(imageEncode, ".webp");
    }

} // namespace mango::image
