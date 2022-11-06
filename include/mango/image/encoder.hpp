/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
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
        Palette palette;          // gif, png

        ConstMemory icc;          // jpg, png, jp2

        float quality = 0.90f;    // jpg, jp2, heif: [0.0, 1.0]
        int compression = 5;      // png: [0, 10]
        bool parallel = true;     // png
        bool dithering = true;    // gif
        bool lossless = false;    // webp, jp2, heif

        bool simd = true;         // jpg
        bool multithread = true;  // jpg, jp2
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

    void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension);
    bool isImageEncoder(const std::string& extension);

} // namespace mango::image
