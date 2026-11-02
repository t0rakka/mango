/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
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
    enum ImageEncodeCapability : u32
    {
        ENCODE_CAPABILITY_INDEXED      = 0x00000001, // accepts indexed surface + palette on encode
        ENCODE_CAPABILITY_INDEXED_AUTO = 0x00000002, // may quantize truecolor to indexed during encode
        ENCODE_CAPABILITY_FLOAT        = 0x00000004, // accepts floating-point component surfaces
        ENCODE_CAPABILITY_UNORM16      = 0x00000008, // accepts 16-bit/component UNORM surfaces
        ENCODE_CAPABILITY_LUMINANCE    = 0x00000010, // can encode grayscale / I(A) on wire
        ENCODE_CAPABILITY_ALPHA        = 0x00000020, // alpha channel can be stored on wire

        ENCODE_CAPABILITY_LINEAR       = 0x00000100, // linear transfer on wire (not display sRGB)

        ENCODE_CAPABILITY_LOSSLESS     = 0x00001000, // lossless encode (always or via options.lossless)
        ENCODE_CAPABILITY_LOSSY        = 0x00002000, // lossy encode (quality-driven)
    };

    struct ImageEncodeCapabilities
    {
        bool encoder = false;
        u32 flags = 0;

        bool indexed() const
        {
            return (flags & ENCODE_CAPABILITY_INDEXED) != 0;
        }

        bool indexedAutoQuantize() const
        {
            return (flags & ENCODE_CAPABILITY_INDEXED_AUTO) != 0;
        }

        bool floatFormat() const
        {
            return (flags & ENCODE_CAPABILITY_FLOAT) != 0;
        }

        bool unorm16() const
        {
            return (flags & ENCODE_CAPABILITY_UNORM16) != 0;
        }

        bool luminance() const
        {
            return (flags & ENCODE_CAPABILITY_LUMINANCE) != 0;
        }

        bool alpha() const
        {
            return (flags & ENCODE_CAPABILITY_ALPHA) != 0;
        }

        bool linear() const
        {
            return (flags & ENCODE_CAPABILITY_LINEAR) != 0;
        }

        bool lossless() const
        {
            return (flags & ENCODE_CAPABILITY_LOSSLESS) != 0;
        }

        bool lossy() const
        {
            return (flags & ENCODE_CAPABILITY_LOSSY) != 0;
        }
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

    void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension, u32 capabilities = 0);
    bool isImageEncoder(const std::string& extension);

    ImageEncodeCapabilities getImageEncodeCapabilities(const std::string& extension);
    bool isImageEncoderIndexed(const std::string& extension);
    std::vector<std::string> listImageEncoderExtensions();

} // namespace mango::image
