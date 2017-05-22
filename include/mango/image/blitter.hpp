/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../core/object.hpp"
#include "../simd/simd.hpp"
#include "surface.hpp"

namespace mango
{

    struct BlitRect
    {
        uint8* destImage;
        uint8* srcImage;
        int destStride;
        int srcStride;
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
            uint32 srcMask;
            uint32 destMask;
            float scale;
            float bias;
            float constant; // TODO: configure
            int offset; // TODO: configure

            uint32 computePack(uint32 s) const
            {
                return uint32((s & srcMask) * scale + bias) & destMask;
            }
        } component[4];

        int components;
        int sampleSize;
        uint32 initMask;
        uint32 copyMask;

#ifdef MANGO_ENABLE_SSE2
        __m128 sseScale;
        __m128i sseSrcMask;
        __m128i sseDestMask;
        __m128i sseShiftMask;
#endif

        typedef void (*FastFunc)(uint8 *, const uint8 *, int);
        typedef void (*ConvertFunc)(const Blitter& blitter, const BlitRect& rect);

        FastFunc custom;
        ConvertFunc convertFunc;

        Blitter(const Format& dest, const Format& source);
        ~Blitter();

        void convert(const BlitRect& rect) const
        {
            if (convertFunc)
                convertFunc(*this, rect);
        }
    };

} // namespace mango
