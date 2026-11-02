/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <memory>
#include <mango/image/surface.hpp>

namespace mango::image
{

    struct IndexedEncodeTarget
    {
        int color_count = 0;     // requested palette colors (after clamp 1..256)
        int table_size = 256;    // power-of-two palette table (padded)
        int index_bit_depth = 8; // 1, 2, 4, or 8 index bits on wire
    };

    // Round requested color count for a given output extension.
    IndexedEncodeTarget resolveIndexedEncodeTarget(int requested_colors, const std::string& extension);

    // Build indexed bitmap for encoders that accept ENCODE_CAPABILITY_INDEXED.
    std::unique_ptr<Bitmap> quantizeToIndexedBitmap(const Surface& source, int requested_colors,
        bool dither, float quality, const std::string& extension,
        IndexedEncodeTarget* out_target = nullptr);

    class ColorQuantizer
    {
    protected:
        enum { NETSIZE = 256 };

        Palette m_palette;
        int m_network[NETSIZE][4];
        int m_netindex[NETSIZE];

    public:
        ColorQuantizer(const Surface& source, float quality = 0.90f);
        ColorQuantizer(const Palette& palette);
        ~ColorQuantizer();

        // get generated palette
        const Palette& getPalette() const;

        // quantize ANY image with the quantization network (the original color image is recommended)
        void quantize(const Surface& dest, const Surface& source, bool dithering = true);

    protected:
        void buildIndex();
        int getIndex(int r, int g, int b) const;
    };

    class QuantizedBitmap : public Bitmap
    {
    public:
        QuantizedBitmap(const Surface& source, float quality = 0.90f, bool dithering = true);
        QuantizedBitmap(const Surface& source, const Palette& palette, bool dithering = true);
        ~QuantizedBitmap();
    };

} // namespace mango::image
