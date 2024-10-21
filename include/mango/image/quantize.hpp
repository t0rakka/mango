/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/image/format.hpp>
#include <mango/image/surface.hpp>

namespace mango::image
{

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
        Palette getPalette() const;

        // quantize ANY image with the quantization network (the original color image is recommended)
        void quantize(const Surface& dest, const Surface& source, bool dithering = true);

    protected:
        void buildIndex();
        int getIndex(int r, int g, int b) const;
    };

    class QuantizedBitmap : public Bitmap
    {
    private:
        Palette m_palette;

    public:
        QuantizedBitmap(const Surface& source, float quality = 0.90f, bool dithering = true);
        QuantizedBitmap(const Surface& source, const Palette& palette, bool dithering = true);
        ~QuantizedBitmap();

        Palette getPalette() const;
    };

    class LuminanceBitmap : public Bitmap
    {
    public:
        LuminanceBitmap(const Surface& source, bool alpha = false, bool linear = true);
    };

} // namespace mango::image
