/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <cctype>
#include <memory>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    const char* skipComment(const char* p, const char* end)
    {
        for (;;)
        {
            if (p >= end)
                break;

            if (*p++ == '\n')
                break;
        }
        return p;
    }

    inline
    const char* nextValue(int& value, const char* p, const char* end)
    {
        if (!p)
        {
            return nullptr;
        }

        for (;;)
        {
            if (p >= end)
            {
                return nullptr;
            }

            char c = *p;
            if (std::isdigit((unsigned char)c) || c == '-')
                break;

            if (c == '#')
                p = skipComment(p, end);
            else
                ++p;
        }

        // Bounded integer parse: the input buffer is not guaranteed to be NUL terminated,
        // so std::strtol could read past the end. Parse digits explicitly against 'end'.
        bool negative = false;
        if (p < end && *p == '-')
        {
            negative = true;
            ++p;
        }

        long v = 0;
        while (p < end && std::isdigit((unsigned char)*p))
        {
            v = v * 10 + (*p - '0');
            if (v > 0x7fffffff)
                v = 0x7fffffff; // clamp, validated against dimensions later
            ++p;
        }

        value = int(negative ? -v : v);

        // skip the delimiter (matches the previous strtol-based behavior)
        return p + 1;
    }

    const char* nextLine(const char* p, const char* end)
    {
        if (!p)
        {
            return nullptr;
        }

        for (;;)
        {
            if (p >= end)
            {
                return nullptr;
            }

            char c = *p++;
            if (c == '\n')
                break;
        }
        return p;
    }

    // Map a netpbm sample in [0, maxvalue] to 16-bit UNORM (full range when maxvalue < 65535).
    inline
    u16 pnmSampleToU16(int value, int maxvalue)
    {
        if (value < 0)
            value = 0;
        else if (value > maxvalue)
            value = maxvalue;

        if (maxvalue <= 0)
            return 0;

        return u16((u32(value) * 65535 + u32(maxvalue / 2)) / u32(maxvalue));
    }

    // ------------------------------------------------------------
    // HeaderPNM
    // ------------------------------------------------------------

    struct HeaderPNM
    {
        int width;
        int height;
        int channels;
        int maxvalue;
        int endian;

        bool is_ascii;
        bool is_float;
        const char* data;
        u8 invert;

        ImageHeader header;

        void setDataError()
        {
            header.setError("Decoding error (out of data).");
        }

        HeaderPNM(ConstMemory memory)
            : width(0)
            , height(0)
            , channels(0)
            , maxvalue(0)
            , endian(0)
            , is_ascii(false)
            , is_float(false)
            , data(nullptr)
            , invert(0)
        {
            const char* p = reinterpret_cast<const char *>(memory.address);
            const char* end = reinterpret_cast<const char *>(memory.end());

            // need at least the 3-byte magic before touching the bytes below
            if (memory.size < 3)
            {
                header.setError("[ImageDecoder.PNM] Not enough data.");
                return;
            }

            printLine(Print::Debug, "[Header: {:c}{:c}]", p[0], p[1]);

            if (!std::strncmp(p, "Pf\n", 3))
            {
                p = nextLine(p, end);
                p = nextValue(width, p, end);
                p = nextValue(height, p, end);
                p = nextValue(endian, p, end);
                p = nextLine(p, end);

                if (!p)
                {
                    setDataError();
                    return;
                }

                channels = 1;
                maxvalue = 255;
                is_float = true;
            }
            else if (!std::strncmp(p, "PF\n", 3))
            {
                p = nextLine(p, end);
                p = nextValue(width, p, end);
                p = nextValue(height, p, end);
                p = nextValue(endian, p, end);
                p = nextLine(p, end);

                if (!p)
                {
                    setDataError();
                    return;
                }

                channels = 3;
                maxvalue = 255;
                is_float = true;
            }
            else if (!std::strncmp(p, "P7", 2) && (p[2] == '\n' || p[2] == '\r'))
            {
                // PAM: the header is a set of "TOKEN value" lines in any order, terminated
                // by ENDHDR. Comments (#...) and blank lines are ignored. The previous code
                // assumed a fixed WIDTH/HEIGHT/DEPTH/MAXVAL/TUPLTYPE order, which rejected
                // perfectly valid files that reorder or omit fields.

                // Parse a non-negative integer from [s, e); returns -1 when none is present.
                auto parseInt = [](const char* s, const char* e) -> int
                {
                    while (s < e && !std::isdigit((unsigned char)*s))
                        ++s;
                    if (s >= e)
                        return -1;
                    long v = 0;
                    while (s < e && std::isdigit((unsigned char)*s))
                    {
                        v = v * 10 + (*s - '0');
                        if (v > 0x7fffffff)
                            v = 0x7fffffff;
                        ++s;
                    }
                    return int(v);
                };

                p = nextLine(p, end); // skip the "P7" magic line

                bool endhdr = false;

                while (p && p < end)
                {
                    const char* line = p;
                    p = nextLine(p, end);
                    const char* lineEnd = p ? p : end;

                    // skip leading whitespace
                    while (line < lineEnd && (*line == ' ' || *line == '\t'))
                        ++line;

                    // ignore blank lines and comments
                    if (line >= lineEnd || *line == '\n' || *line == '\r' || *line == '#')
                        continue;

                    const size_t avail = size_t(lineEnd - line);

                    if (avail >= 6 && !std::strncmp(line, "ENDHDR", 6))
                    {
                        endhdr = true;
                        break;
                    }
                    else if (avail >= 5 && !std::strncmp(line, "WIDTH", 5))
                    {
                        width = parseInt(line + 5, lineEnd);
                    }
                    else if (avail >= 6 && !std::strncmp(line, "HEIGHT", 6))
                    {
                        height = parseInt(line + 6, lineEnd);
                    }
                    else if (avail >= 5 && !std::strncmp(line, "DEPTH", 5))
                    {
                        channels = parseInt(line + 5, lineEnd);
                    }
                    else if (avail >= 6 && !std::strncmp(line, "MAXVAL", 6))
                    {
                        maxvalue = parseInt(line + 6, lineEnd);
                    }
                    // TUPLTYPE is informational; the channel count comes from DEPTH
                }

                if (!endhdr)
                {
                    header.setError("[ImageDecoder.PNM] Incorrect ENDHDR.");
                    return;
                }

                if (!p)
                {
                    setDataError();
                    return;
                }
            }
            else
            {
                if (!std::strncmp(p, "P1", 2))
                {
                    is_ascii = true;
                    channels = 1;
                    maxvalue = 1;
                    invert = 0xff;
                }
                else if (!std::strncmp(p, "P2", 2))
                {
                    is_ascii = true;
                    channels = 1;
                }
                else if (!std::strncmp(p, "P3", 2))
                {
                    is_ascii = true;
                    channels = 3;
                }
                else if (!std::strncmp(p, "P4", 2))
                {
                    channels = 1;
                    maxvalue = 1;
                    invert = 0xff;
                }
                else if (!std::strncmp(p, "P5", 2))
                {
                    channels = 1;
                }
                else if (!std::strncmp(p, "P6", 2))
                {
                    channels = 3;
                }
                else
                {
                    header.setError("[ImageDecoder.PNM] Incorrect header magic ({}).", p);
                    return;
                }

                p += 3; // skip header magic
                p = nextValue(width, p, end);
                p = nextValue(height, p, end);

                if (!maxvalue)
                {
                    p = nextValue(maxvalue, p, end);
                }

                if (!p)
                {
                    setDataError();
                    return;
                }
            }

            printLine(Print::Debug, "  image: {} x {}, channels: {}", width, height, channels);
            printLine(Print::Debug, "  maxvalue: {}", maxvalue);

            if (!isValidImageDecodeDimensions(width, height))
            {
                header.setError("[ImageDecoder.PNM] Incorrect image dimensions ({} x {}).", width, height);
                return;
            }

            if (maxvalue < 1 || maxvalue > 65535)
            {
                header.setError("[ImageDecoder.PNM] Incorrect maxvalue.");
                return;
            }

            Format format;

            if (is_float)
            {
                switch (channels)
                {
                    case 1: format = Format(32, Format::FLOAT32, Color(32, 32, 32, 0), Color(0, 0, 0, 0)); break;
                    case 3: format = Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0); break;
                    default:
                        header.setError("[ImageDecoder.PNM] Incorrect number of channels.");
                        return;
                }
            }
            else
            {
                const bool wide = maxvalue > 255;

                switch (channels)
                {
                    case 1: format = wide ? LuminanceFormat(16, Format::UNORM, 16, 0) : LuminanceFormat(8, Format::UNORM, 8, 0); break;
                    case 2: format = wide ? LuminanceFormat(32, Format::UNORM, 16, 16) : LuminanceFormat(16, Format::UNORM, 8, 8); break;
                    case 3: format = wide ? Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0) : Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0); break;
                    case 4: format = wide ? Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16) : Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8); break;
                    default:
                        header.setError("[ImageDecoder.PNM] Incorrect number of channels.");
                        return;
                }
            }

            data = p;

            header.width   = width;
            header.height  = height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
            header.format  = format;
            header.compression = TextureCompression::NONE;

            if (is_float)
            {
                // PFM (Pf/PF) stores linear floating-point samples with no gamma,
                // like Radiance HDR. The integer PNM variants stay sRGB (the default).
                header.color.transfer = TransferFunction::Linear;
                header.linear = true;
            }
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        HeaderPNM m_pnm_header;

        Interface(ConstMemory memory)
            : m_memory(memory)
            , m_pnm_header(memory)
        {
            header = m_pnm_header.header;
        }

        ~Interface()
        {
        }

        void populateInspect(ImageInspect& report) const override
        {
            report.lossless = InspectTriState::Yes;
            report.progressive = InspectTriState::No;
            report.tiling.tiled = InspectTriState::No;
            report.chroma_subsampling = "4:4:4";
            report.bit_depth = m_pnm_header.is_float ? 32 : m_pnm_header.maxvalue > 255 ? 16 : 8;
            report.alpha = m_pnm_header.channels == 4 || m_pnm_header.channels == 2;

            if (m_pnm_header.is_float)
            {
                report.encoding = m_pnm_header.channels >= 3 ? "PFM RGB" : "PFM grayscale";
            }
            else if (m_pnm_header.channels == 1)
            {
                report.encoding = m_pnm_header.is_ascii ? "PGM ASCII" : "PGM";
            }
            else if (m_pnm_header.channels == 3)
            {
                report.encoding = m_pnm_header.is_ascii ? "PPM ASCII" : "PPM";
            }
            else
            {
                report.encoding = "PAM";
            }

            syncHdrInspectFromHeader(report);
        }

        bool decode_matching(const Surface& dest)
        {
            const char* p = m_pnm_header.data;
            const char* end = reinterpret_cast<const char *>(m_memory.end());

            if (!p || p > end)
            {
                return false;
            }

            const int xcount = m_pnm_header.width * m_pnm_header.channels;
            const int height = m_pnm_header.height;
            int maxvalue = m_pnm_header.maxvalue;
            u8 invert = m_pnm_header.invert;

            // available raw bytes from the start of the pixel data
            const size_t avail = size_t(end - p);

            if (m_pnm_header.is_ascii)
            {
                const bool wide = maxvalue > 255 && !invert;

                for (int y = 0; y < height; ++y)
                {
                    if (wide)
                    {
                        u16* image = dest.address<u16>(0, y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            int value;
                            p = nextValue(value, p, end);
                            if (!p)
                            {
                                return false;
                            }

                            image[x] = pnmSampleToU16(value, maxvalue);
                        }
                    }
                    else
                    {
                        u8* image = dest.address<u8>(0, y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            int value;
                            p = nextValue(value, p, end);
                            if (!p)
                            {
                                return false;
                            }

                            // clamp to [0, maxvalue] so the scaling cannot overflow
                            if (value < 0)
                                value = 0;
                            else if (value > maxvalue)
                                value = maxvalue;

                            image[x] = u8(value * 255 / maxvalue) ^ invert;
                        }
                    }
                }
            }
            else if (m_pnm_header.is_float)
            {
                // float samples: xcount * height 32-bit values
                if (avail < size_t(xcount) * size_t(height) * 4)
                {
                    return false;
                }

                if (m_pnm_header.endian < 0)
                {
                    LittleEndianConstPointer ptr = p;

                    for (int y = 0; y < m_pnm_header.height; ++y)
                    {
                        float* image = dest.address<float>(0, m_pnm_header.height - 1 - y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            image[x] = ptr.read32f();
                        }
                    }
                }
                else
                {
                    BigEndianConstPointer ptr = p;

                    for (int y = 0; y < m_pnm_header.height; ++y)
                    {
                        float* image = dest.address<float>(0, m_pnm_header.height - 1 - y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            image[x] = ptr.read32f();
                        }
                    }
                }
            }
            else
            {
                if (m_pnm_header.invert)
                {
                    // 1-bit packed data, each scanline byte-aligned
                    const size_t bytes_per_scan = (size_t(xcount) + 7) / 8;
                    if (avail < bytes_per_scan * size_t(height))
                    {
                        return false;
                    }

                    for (int y = 0; y < height; ++y)
                    {
                        u8* image = dest.address<u8>(0, y);

                        // 1 bit data is byte aligned; reset data register
                        u8 data = 0;
                        int size = 0;

                        for (int x = 0; x < xcount; ++x)
                        {
                            if (!size)
                            {
                                // out of data; fetch a byte
                                data = *p++;
                                size = 8;
                            }

                            // extract one bit in reverse order (MSB-first)
                            --size;
                            u8 sample = (data >> size) & 1;

                            // invert the sample
                            // 0 --> 255
                            // 1 --> 0
                            image[x] = sample + 0xff;
                        }
                    }
                }
                else if (m_pnm_header.maxvalue <= 255)
                {
                    // one byte per sample
                    if (avail < size_t(xcount) * size_t(height))
                    {
                        return false;
                    }

                    for (int y = 0; y < height; ++y)
                    {
                        u8* image = dest.address<u8>(0, y);
                        std::memcpy(image, p, xcount);
                        p += xcount;
                    }
                }
                else
                {
                    // two bytes per sample (big-endian on wire)
                    if (avail < size_t(xcount) * size_t(height) * 2)
                    {
                        return false;
                    }

                    BigEndianConstPointer e = p;

                    for (int y = 0; y < height; ++y)
                    {
                        u16* image = dest.address<u16>(0, y);

                        for (int x = 0; x < xcount; ++x)
                        {
                            const int value = e.read16();
                            image[x] = pnmSampleToU16(value, maxvalue);
                        }
                    }
                }
            }

            return true;
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

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);

            status.success = decode_matching(target);
            status.direct = target.isDirect();

            if (status.success)
            {
                target.resolve();
            }

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

    struct PnmRgbUnormSurface
    {
        bool valid = false;
        bool is16bit = false;
        bool hasAlpha = false;
    };

    PnmRgbUnormSurface classifyRgbUnormSurface(const Format& format)
    {
        PnmRgbUnormSurface info;

        if (format.isIndexed() || format.isFloat() || format.isLuminance())
            return info;

        if (format.type != Format::UNORM)
            return info;

        const int red = format.size[Format::RED];
        const int green = format.size[Format::GREEN];
        const int blue = format.size[Format::BLUE];
        const int alpha = format.size[Format::ALPHA];

        if (!red || green != red || blue != red)
            return info;

        if (red != 8 && red != 16)
            return info;

        if (alpha && alpha != red)
            return info;

        info.valid = true;
        info.is16bit = red == 16;
        info.hasAlpha = alpha > 0;
        return info;
    }

    static const Format& pnmRgba8Format()
    {
        static const Format format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        return format;
    }

    bool preparePnmIntegerSurface(const Surface& source, std::unique_ptr<Bitmap>& storage, const Surface*& encodeSurface, PnmRgbUnormSurface& info, ImageEncodeStatus& status)
    {
        encodeSurface = &source;
        storage.reset();

        if (source.format.isLuminance())
        {
            status.setError("[ImageEncoder.PNM] Luminance surfaces are not supported.");
            return false;
        }

        if (source.format.isFloat())
        {
            return false;
        }

        if (source.format.isIndexed())
        {
            if (!source.palette)
            {
                status.setError("[ImageEncoder.PNM] Indexed surface does not have a palette.");
                return false;
            }

            storage = std::make_unique<Bitmap>(source.width, source.height, pnmRgba8Format());
            image::resolve(*storage, source);

            encodeSurface = storage.get();
            info.valid = true;
            info.is16bit = false;
            info.hasAlpha = true;
            return true;
        }

        info = classifyRgbUnormSurface(source.format);
        if (!info.valid)
        {
            status.setError("[ImageEncoder.PNM] 8 or 16 bit RGB/RGBA UNORM, or indexed surface required.");
            return false;
        }

        return true;
    }

    ImageEncodeStatus encodePnmP6(Stream& stream, const Surface& surface, const PnmRgbUnormSurface& info)
    {
        ImageEncodeStatus status;

        const int width = surface.width;
        const int height = surface.height;
        const int maxval = info.is16bit ? 65535 : 255;

        const Format targetFormat = info.is16bit
            ? Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0)
            : Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0);

        TemporaryBitmap temp(surface, targetFormat);

        const std::string header = fmt::format("P6\n{} {}\n{}\n", width, height, maxval);
        stream.write(header.data(), header.size());

        if (info.is16bit)
        {
            BigEndianStream out(stream);

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const u16* pixel = temp.address<u16>(x, y);
                    out.write16(pixel[0]);
                    out.write16(pixel[1]);
                    out.write16(pixel[2]);
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                const u8* scan = temp.address<u8>(0, y);
                stream.write(scan, size_t(width) * 3);
            }
        }

        return status;
    }

    ImageEncodeStatus encodePnmPam(Stream& stream, const Surface& surface, const PnmRgbUnormSurface& info)
    {
        ImageEncodeStatus status;

        const int width = surface.width;
        const int height = surface.height;
        const int maxval = info.is16bit ? 65535 : 255;
        const int depth = info.hasAlpha ? 4 : 3;
        const char* tupltype = info.hasAlpha ? "RGB_ALPHA" : "RGB";

        const Format targetFormat = info.hasAlpha
            ? (info.is16bit
                ? Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16)
                : Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8))
            : (info.is16bit
                ? Format(48, Format::UNORM, Format::RGB, 16, 16, 16, 0)
                : Format(24, Format::UNORM, Format::RGB, 8, 8, 8, 0));

        TemporaryBitmap temp(surface, targetFormat);

        const std::string header = fmt::format(
            "P7\nWIDTH {}\nHEIGHT {}\nDEPTH {}\nMAXVAL {}\nTUPLTYPE {}\nENDHDR\n",
            width, height, depth, maxval, tupltype);
        stream.write(header.data(), header.size());

        if (info.is16bit)
        {
            BigEndianStream out(stream);

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const u16* pixel = temp.address<u16>(x, y);

                    for (int c = 0; c < depth; ++c)
                    {
                        out.write16(pixel[c]);
                    }
                }
            }
        }
        else
        {
            const int bytes_per_pixel = depth;

            for (int y = 0; y < height; ++y)
            {
                const u8* scan = temp.address<u8>(0, y);
                stream.write(scan, size_t(width) * bytes_per_pixel);
            }
        }

        return status;
    }

    ImageEncodeStatus encodePnmPfm(Stream& stream, const Surface& surface)
    {
        ImageEncodeStatus status;

        if (surface.format.isIndexed())
        {
            status.setError("[ImageEncoder.PFM] Floating-point surface required.");
            return status;
        }

        if (!surface.format.isFloat())
        {
            status.setError("[ImageEncoder.PFM] Floating-point surface required.");
            return status;
        }

        const int width = surface.width;
        const int height = surface.height;

        LittleEndianStream out(stream);

        if (surface.format.isLuminance())
        {
            TemporaryBitmap temp(surface, LuminanceFormat(32, Format::FLOAT32, 32, 0));

            const std::string header = fmt::format("Pf\n{} {}\n-1.0\n", width, height);
            out.write(header.data(), header.size());

            for (int y = height - 1; y >= 0; --y)
            {
                const float* scan = temp.address<float>(0, y);

                for (int x = 0; x < width; ++x)
                {
                    out.write32f(scan[x]);
                }
            }
        }
        else
        {
            TemporaryBitmap temp(surface, Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0));

            const std::string header = fmt::format("PF\n{} {}\n-1.0\n", width, height);
            out.write(header.data(), header.size());

            for (int y = height - 1; y >= 0; --y)
            {
                const float* scan = temp.address<float>(0, y);

                for (int x = 0; x < width; ++x)
                {
                    out.write32f(scan[0]);
                    out.write32f(scan[1]);
                    out.write32f(scan[2]);
                    scan += 3;
                }
            }
        }

        return status;
    }

    ImageEncodeStatus imageEncodePBM(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(stream);
        MANGO_UNREFERENCED(surface);
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;
        status.setError("[ImageEncoder.PBM] 1-bit PBM (P1/P4) encoding is not supported.");
        return status;
    }

    ImageEncodeStatus imageEncodePGM(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(stream);
        MANGO_UNREFERENCED(surface);
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;
        status.setError("[ImageEncoder.PGM] Grayscale PNM (P2/P5) encoding is not supported.");
        return status;
    }

    ImageEncodeStatus imageEncodePPM(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        if (surface.format.isFloat())
        {
            status.setError("[ImageEncoder.PPM] Floating-point surfaces are not supported.");
            return status;
        }

        std::unique_ptr<Bitmap> storage;
        const Surface* encodeSurface = nullptr;
        PnmRgbUnormSurface info;

        if (!preparePnmIntegerSurface(surface, storage, encodeSurface, info, status))
        {
            if (!status.info.empty())
                return status;
            status.setError("[ImageEncoder.PPM] 8 or 16 bit RGB/RGBA UNORM, or indexed surface required.");
            return status;
        }

        return encodePnmP6(stream, *encodeSurface, info);
    }

    ImageEncodeStatus imageEncodePAM(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        ImageEncodeStatus status;

        if (surface.format.isFloat())
        {
            status.setError("[ImageEncoder.PAM] Floating-point surfaces are not supported.");
            return status;
        }

        std::unique_ptr<Bitmap> storage;
        const Surface* encodeSurface = nullptr;
        PnmRgbUnormSurface info;

        if (!preparePnmIntegerSurface(surface, storage, encodeSurface, info, status))
        {
            if (!status.info.empty())
                return status;
            status.setError("[ImageEncoder.PAM] 8 or 16 bit RGB/RGBA UNORM, or indexed surface required.");
            return status;
        }

        return encodePnmPam(stream, *encodeSurface, info);
    }

    ImageEncodeStatus imageEncodePFM(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);
        return encodePnmPfm(stream, surface);
    }

    ImageEncodeStatus imageEncodePNM(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        MANGO_UNREFERENCED(options);

        if (surface.format.isFloat())
            return encodePnmPfm(stream, surface);

        ImageEncodeStatus status;

        std::unique_ptr<Bitmap> storage;
        const Surface* encodeSurface = nullptr;
        PnmRgbUnormSurface info;

        if (!preparePnmIntegerSurface(surface, storage, encodeSurface, info, status))
            return status;

        if (info.hasAlpha)
            return encodePnmPam(stream, *encodeSurface, info);

        return encodePnmP6(stream, *encodeSurface, info);
    }

    // ------------------------------------------------------------
    // Detect PNM format extension
    // ------------------------------------------------------------

    static
    bool isWhitespacePNM(u8 c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    static
    const char* getFormatExtensionPNM(ConstMemory memory)
    {
        if (memory.size < 3)
            return nullptr;

        const u8* p = memory.address;
        if (p[0] != 'P')
            return nullptr;

        if (!isWhitespacePNM(p[2]))
            return nullptr;

        switch (p[1])
        {
            case '1': case '4': return ".pbm";
            case '2': case '5': return ".pgm";
            case '3': case '6': return ".ppm";
            case '7': return ".pam";
            case 'f': case 'F': return ".pfm";
            default: return nullptr;
        }
    }

} // namespace

namespace mango::image
{

    void registerImageCodecPNM()
    {
        // decoders
        registerImageDecoder(createInterface, ".pbm"); // P1, P4: 1 bit per pixel (black and white)
        registerImageDecoder(createInterface, ".pgm"); // P2, P5: 1 channel of 8 or 16 bit unorm (luminance)
        registerImageDecoder(createInterface, ".ppm"); // P3, P6: 3 channels of 8 or 16 bit unorm (RGB)
        registerImageDecoder(createInterface, ".pam"); // P7: 1-4 channels of 8 or 16 bit unorm
        registerImageDecoder(createInterface, ".pfm"); // PF/Pf: 1 or 3 channels of float
        registerImageDecoder(createInterface, ".pnm"); // generic netpbm extension

        // encoders
        registerImageEncoder(imageEncodePBM, ".pbm");
        registerImageEncoder(imageEncodePGM, ".pgm");
        registerImageEncoder(imageEncodePPM, ".ppm",
            ENCODE_CAPABILITY_UNORM16 |
            ENCODE_CAPABILITY_LOSSLESS);
        registerImageEncoder(imageEncodePAM, ".pam",
            ENCODE_CAPABILITY_UNORM16 |
            ENCODE_CAPABILITY_ALPHA |
            ENCODE_CAPABILITY_LOSSLESS);
        registerImageEncoder(imageEncodePFM, ".pfm",
            ENCODE_CAPABILITY_FLOAT |
            ENCODE_CAPABILITY_LINEAR |
            ENCODE_CAPABILITY_LOSSLESS);
        registerImageEncoder(imageEncodePNM, ".pnm",
            ENCODE_CAPABILITY_UNORM16 |
            ENCODE_CAPABILITY_ALPHA |
            ENCODE_CAPABILITY_FLOAT |
            ENCODE_CAPABILITY_LINEAR |
            ENCODE_CAPABILITY_LOSSLESS);
    }

    const char* detectImageCodecPNM(ConstMemory memory)
    {
        return getFormatExtensionPNM(memory);
    }

} // namespace mango::image
