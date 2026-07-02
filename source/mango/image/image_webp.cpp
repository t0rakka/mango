/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <cstring>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#if defined(MANGO_ENABLE_WEBP)

#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/encode.h>

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
    // RIFF container scan (for the optional ICCP color profile chunk)
    // ------------------------------------------------------------

    ConstMemory webpFindChunk(ConstMemory memory, u32 fourcc)
    {
        // WebP files are a RIFF container: "RIFF" <u32 size> "WEBP" then a sequence of
        // chunks, each a 4-byte FourCC + u32 little-endian payload size + payload padded
        // to an even length. Extended (VP8X) files may carry an "ICCP" profile chunk. The
        // simple libwebp decode API does not expose it, so we walk the container ourselves.
        // All reads are bounds-checked against the buffer end.
        if (memory.size < 12 ||
            std::memcmp(memory.address, "RIFF", 4) != 0 ||
            std::memcmp(memory.address + 8, "WEBP", 4) != 0)
        {
            return ConstMemory(nullptr, 0);
        }

        const u8* p = memory.address + 12;
        const u8* end = memory.end();

        while (p + 8 <= end)
        {
            u32 id = bigEndian::uload32(p);            // FourCC, read in byte order
            u32 size = littleEndian::uload32(p + 4);   // RIFF chunk sizes are little-endian
            p += 8;

            if (size > size_t(end - p))
            {
                break;
            }

            if (id == fourcc)
            {
                return ConstMemory(p, size);
            }

            p += size + (size & 1); // chunks are padded to an even size
        }

        return ConstMemory(nullptr, 0);
    }

    bool webpIsAnimated(ConstMemory memory)
    {
        ConstMemory vp8x = webpFindChunk(memory, u32_mask_rev('V', 'P', '8', 'X'));
        if (vp8x.size < 1)
        {
            return false;
        }

        constexpr u8 ANIMATION_FLAG = 0x02;
        return (vp8x[0] & ANIMATION_FLAG) != 0;
    }

    WEBP_CSP_MODE webpColorMode(const WebPFormat& wpformat)
    {
        if (wpformat.decode_func == WebPDecodeBGRAInto)
        {
            return MODE_BGRA;
        }

        // WebPAnimDecoder only supports 32-bit RGBA/BGRA output modes.
        return MODE_RGBA;
    }

    bool webpDecodeAnimated(const Surface& dest, ConstMemory memory, const WebPFormat& wpformat)
    {
        WebPData webp_data;
        webp_data.bytes = memory.address;
        webp_data.size = memory.size;

        WebPAnimDecoderOptions options;
        if (!WebPAnimDecoderOptionsInit(&options))
        {
            return false;
        }

        options.color_mode = webpColorMode(wpformat);

        WebPAnimDecoder* decoder = WebPAnimDecoderNew(&webp_data, &options);
        if (!decoder)
        {
            return false;
        }

        uint8_t* canvas = nullptr;
        int timestamp = 0;
        bool ok = WebPAnimDecoderGetNext(decoder, &canvas, &timestamp) && canvas;

        if (ok)
        {
            const int width = dest.width;
            const int height = dest.height;
            const size_t row_bytes = size_t(width) * 4;
            const u8* src = canvas;
            u8* image = dest.image;

            for (int y = 0; y < height; ++y)
            {
                std::memcpy(image, src, row_bytes);
                src += row_bytes;
                image += dest.stride;
            }
        }

        WebPAnimDecoderDelete(decoder);
        return ok;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        bool m_isAnimated = false;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            m_isAnimated = webpIsAnimated(m_memory);

            int width;
            int height;
            if (!WebPGetInfo(m_memory.address, m_memory.size, &width, &height))
            {
                header.setError("[ImageDecoder.WEBP] Incorrect header.");
            }
            else
            {
                header.width   = width;
                header.height  = height;
                header.depth   = 0;
                header.levels  = 0;
                header.faces   = 0;
                header.format  = webpDefaultFormat(true).format;
                header.compression = TextureCompression::NONE;

                // WebP pixel data is sRGB (the default). Forward an embedded ICC profile
                // when present; it then defines the color space.
                icc = webpFindChunk(m_memory, u32_mask_rev('I', 'C', 'C', 'P'));
                if (icc.size)
                {
                    header.color.primaries = ColorPrimaries::Unspecified;
                    header.color.transfer = TransferFunction::Unspecified;
                }
            }
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            WebPFormat wpformat = webpFindFormat(dest.format);

            DecodeTargetBitmap target(dest, header.width, header.height, wpformat.format);

            bool ok = false;

            if (m_isAnimated)
            {
                ok = webpDecodeAnimated(target, m_memory, wpformat);
            }
            else
            {
                uint8_t* output = wpformat.decode(target, m_memory);
                ok = output != nullptr;
            }

            if (!ok)
            {
                status.setError("[ImageDecoder.WEBP] Decoding failed.");
            }
            else
            {
                target.resolve();
            }

            status.direct = target.isDirect();

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
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
            TemporaryBitmap temp(surface, wpformat.format);
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

    void registerImageCodecWEBP()
    {
        registerImageDecoder(createInterface, ".webp");
        registerImageEncoder(imageEncode, ".webp");
    }

} // namespace mango::image

#else

namespace mango::image
{

    void registerImageCodecWEBP()
    {
        // WEBP codec is disabled
    }

} // namespace mango::image

#endif // defined(MANGO_ENABLE_WEBP)
