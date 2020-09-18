/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/core/object.hpp>
#include <mango/simd/simd.hpp>
#include <mango/image/surface.hpp>

namespace mango
{

    struct BlitScan
    {
        u8* address;
        int stride;
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

#ifdef MANGO_ENABLE_SSE2
        __m128 sseScale;
        __m128i sseSrcMask;
        __m128i sseDestMask;
        __m128i sseShiftMask;
#endif

        using FastFunc = void (*)(u8 *, const u8 *, int);
        using ConvertFunc = void (*)(const Blitter& blitter, const BlitRect& rect);

        FastFunc custom;
        ConvertFunc convertFunc;

        Blitter(const Format& dest, const Format& source);
        ~Blitter();

        void convert(const BlitRect& rect) const;
    };

} // namespace mango
