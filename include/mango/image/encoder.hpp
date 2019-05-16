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
#include "header.hpp"
#include "exif.hpp"

namespace mango
{
    class Surface;

    struct ImageEncoderOptions
    {
        Palette palette;
        float quality = 0.90f;
        bool dithering = true;
    };

    class ImageEncoder : protected NonCopyable
    {
    public:
        typedef void (*EncodeFunc)(Stream& output, const Surface& source, const ImageEncoderOptions& options);

        ImageEncoder(const std::string& extension);
        ~ImageEncoder();

        bool isEncoder() const;

        void encode(Stream& output, const Surface& source, const ImageEncoderOptions& options);

    protected:
        EncodeFunc m_encode_func;
    };

    void registerImageEncoder(ImageEncoder::EncodeFunc func, const std::string& extension);
    bool isImageEncoder(const std::string& extension);

} // namespace mango
