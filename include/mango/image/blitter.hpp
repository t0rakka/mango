/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/memory.hpp>
#include <mango/simd/simd.hpp>
#include <mango/image/surface.hpp>

namespace mango::image
{

    struct BlitRect
    {
        int width;
        int height;

        u8* src_address;
        size_t src_stride;

        u8* dest_address;
        size_t dest_stride;
    };

    class Blitter : protected NonCopyable
    {
    public:
        Format srcFormat;
        Format destFormat;

        using ScanFunc = void (*)(u8* dest, const u8* src, int count);
        using RectFunc = void (*)(const Blitter& blitter, const BlitRect& rect);

        ScanFunc scan_convert;
        RectFunc rect_convert;

        Blitter(const Format& dest, const Format& source);
        ~Blitter();

        void convert(const BlitRect& rect) const;
    };

} // namespace mango::image
