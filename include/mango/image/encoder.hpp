/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include "../core/object.hpp"
#include "../core/stream.hpp"
#include "format.hpp"
#include "compression.hpp"
#include "exif.hpp"

namespace mango
{
    class Surface;

    struct ImageEncodeOptions
    {
        Palette palette;
        float quality = 0.90f;
        bool dithering = true;
        bool lossless = false;
    };

#if 0
    struct ImageEncodeStatus
    {
        std::string name;
        bool direct;
    };
#endif

    class ImageEncoder : protected NonCopyable
    {
    public:
        ImageEncoder(const std::string& extension);
        ~ImageEncoder();

        bool isEncoder() const;
        void encode(Stream& output, const Surface& source, const ImageEncodeOptions& options);

        typedef void (*EncodeFunc)(Stream& output, const Surface& source, const ImageEncodeOptions& options);

    protected:
        EncodeFunc m_encode_func;
    };

    void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension);
    bool isImageEncoder(const std::string& extension);

} // namespace mango
