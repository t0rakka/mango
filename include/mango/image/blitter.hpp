/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/configure.hpp>
#include <mango/image/surface.hpp>

namespace mango::image
{

    class Blitter : protected NonCopyable
    {
    public:
        struct Scan
        {
            u8* address;
            size_t stride;
        };

        struct Rect
        {
            int width;
            int height;
            Scan source;
            Scan dest;
        };

        using ScanFunc = void (*)(u8* dest, const u8* source, int count);
        using RectFunc = void (*)(const Blitter& blitter, const Rect& rect);

        Format destFormat;
        Format sourceFormat;

        ScanFunc scan_convert;
        RectFunc rect_convert;

        Blitter(const Format& dest, const Format& source);
        ~Blitter();

        void convert(const Rect& rect) const;
    };

} // namespace mango::image
