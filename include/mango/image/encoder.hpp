/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include "format.hpp"
#include "compression.hpp"
#include "header.hpp"
#include "exif.hpp"

namespace mango
{

    class ImageEncoder
    {
    public:
        typedef void (*CreateFunc)(Stream& output, const Surface& source, float quality);

        ImageEncoder(const std::string& extension);
        ~ImageEncoder();

        bool isEncoder() const;

        void encode(Stream& output, const Surface& source, float quality);

    protected:
        CreateFunc m_encode;
    };

    void registerImageEncoder(ImageEncoder::CreateFunc func, const std::string& extension);
    bool isImageEncoder(const std::string& extension);

} // namespace mango
