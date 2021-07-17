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

    struct BlitScan
    {
        u8* address;
        size_t stride;
    };

    struct BlitRect
    {
        BlitScan dest;
        BlitScan src;
        int width;
        int height;
    };

    class Blitter : protected NonCopyable
    {
    public:
        Format srcFormat;
        Format destFormat;

        struct Component
        {
            // integer components
            u32 srcMask;
            u32 destMask;
            float scale;
            float bias;

            // float components
            float constant;
            int offset;

            u32 computePack(u32 s) const
            {
                return u32((s & srcMask) * scale + bias) & destMask;
            }
        } component[4];

        int components;
        int sampleSize;
        u32 initMask;
        u32 copyMask;

        using CustomFunc = void (*)(u8 *, const u8 *, int);
        using ConvertFunc = void (*)(const Blitter& blitter, const BlitRect& rect);

        CustomFunc custom;
        ConvertFunc convertFunc;

        Blitter(const Format& dest, const Format& source);
        ~Blitter();

        void convert(const BlitRect& rect) const;
    };

} // namespace mango::image
