/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <mango/core/memory.hpp>
#include <mango/core/stream.hpp>
#include <mango/core/exception.hpp>
#include <mango/image/format.hpp>
#include <mango/image/compression.hpp>
#include <mango/image/exif.hpp>

namespace mango::image
{
    class Surface;

    struct ImageEncodeStatus : Status
    {
        bool direct = false;
    };

    struct ImageEncodeOptions
    {
        ConstMemory icc;          // jpg, png, jp2, jxl

        float quality = 0.90f;    // jpg, jp2, wp2, heif, jxl: [0.0, 1.0]
        int compression = 5;      // png, wp2, jxl: [0, 10]
        bool parallel = true;     // png
        bool dithering = true;    // gif
        bool lossless = false;    // webp, wp2, jp2, heif, jxl

        int astc_block_width = 4;
        int astc_block_height = 4;

        bool simd = true;         // jpg
        bool multithread = true;  // jpg, jp2
    };

    // Registered per output extension when codecs call registerImageEncoder().
    enum class ImageEncoderCapability : u32
    {
        INDEXED      = 0x00000001, // accepts indexed surface + palette on encode
        INDEXED_AUTO = 0x00000002, // may quantize truecolor to indexed during encode
        FLOAT        = 0x00000004, // accepts floating-point component surfaces
        UNORM16      = 0x00000008, // accepts 16-bit/component UNORM surfaces
        LUMINANCE    = 0x00000010, // can encode grayscale / I(A) on wire
        ALPHA        = 0x00000020, // alpha channel can be stored on wire

        LINEAR       = 0x00000100, // linear transfer on wire (not display sRGB)

        LOSSLESS     = 0x00001000, // lossless encode (always or via options.lossless)
        LOSSY        = 0x00002000, // lossy encode (quality-driven)
    };

    constexpr ImageEncoderCapability operator | (ImageEncoderCapability a, ImageEncoderCapability b)
    {
        return ImageEncoderCapability(u32(a) | u32(b));
    }

    struct ImageEncodeCapabilities
    {
        bool encoder = false;
        u32 flags = 0;

        bool indexed() const
        {
            return (flags & u32(ImageEncoderCapability::INDEXED)) != 0;
        }

        bool indexedAutoQuantize() const
        {
            return (flags & u32(ImageEncoderCapability::INDEXED_AUTO)) != 0;
        }

        bool floatFormat() const
        {
            return (flags & u32(ImageEncoderCapability::FLOAT)) != 0;
        }

        bool unorm16() const
        {
            return (flags & u32(ImageEncoderCapability::UNORM16)) != 0;
        }

        bool luminance() const
        {
            return (flags & u32(ImageEncoderCapability::LUMINANCE)) != 0;
        }

        bool alpha() const
        {
            return (flags & u32(ImageEncoderCapability::ALPHA)) != 0;
        }

        bool linear() const
        {
            return (flags & u32(ImageEncoderCapability::LINEAR)) != 0;
        }

        bool lossless() const
        {
            return (flags & u32(ImageEncoderCapability::LOSSLESS)) != 0;
        }

        bool lossy() const
        {
            return (flags & u32(ImageEncoderCapability::LOSSY)) != 0;
        }

        // require: all set bits must be present
        // exclude: any set bit must be absent
        bool matches(u32 require_flags, u32 exclude_flags = 0) const;
        bool matches(ImageEncoderCapability require, u32 exclude_flags = 0) const;
    };

    class ImageEncoder : protected NonCopyable
    {
    public:
        ImageEncoder(const std::string& extension);
        ~ImageEncoder();

        bool isEncoder() const;
        ImageEncodeStatus encode(Stream& output, const Surface& source, const ImageEncodeOptions& options);

        using EncodeFunc = ImageEncodeStatus (*)(Stream& output, const Surface& source, const ImageEncodeOptions& options);

    protected:
        EncodeFunc m_encode_func;
    };

    void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension,
        ImageEncoderCapability capabilities = ImageEncoderCapability(u32(0)));
    bool isImageEncoder(const std::string& extension);

    ImageEncodeCapabilities getImageEncodeCapabilities(const std::string& extension);
    bool isImageEncoderIndexed(const std::string& extension);
    std::vector<std::string> listImageEncoderExtensions();

    // Extensions whose registered flags include all require bits and none of exclude.
    std::vector<std::string> getImageEncoders(u32 require_capability_flags, u32 exclude_capability_flags = 0);
    std::vector<std::string> getImageEncoders(ImageEncoderCapability require, u32 exclude_capability_flags = 0);

} // namespace mango::image
