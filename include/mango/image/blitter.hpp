/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/memory.hpp>
#include <mango/simd/simd.hpp>
#include <mango/image/surface.hpp>

namespace mango::image
{

    struct BlitScan
    {
        u8* address;
        size_t stride;
    };

    struct BlitRect
    {
        int width;
        int height;
        BlitScan source;
        BlitScan dest;
    };

    class Blitter : protected NonCopyable
    {
    public:
        Format srcFormat;
        Format destFormat;

        using ScanFunc = void (*)(u8* dest, const u8* source, int count);
        using RectFunc = void (*)(const Blitter& blitter, const BlitRect& rect);

        ScanFunc scan_convert;
        RectFunc rect_convert;

        Blitter(const Format& dest, const Format& source);
        ~Blitter();

        void convert(const BlitRect& rect) const;
    };

} // namespace mango::image
