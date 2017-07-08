/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include <string>
#include "../core/configure.hpp"
#include "../core/object.hpp"
#include "../filesystem/file.hpp"
#include "format.hpp"

namespace mango
{

    class Surface
    {
    protected:
        Surface();

    public:
        int     width;
        int     height;
        int     stride;
        Format  format;
        uint8*  image;

        Surface(int width, int height, const Format& format, int stride, uint8* image);
        Surface(const Surface& source, int x, int y, int width, int height);
        ~Surface();

        template <typename SampleType>
        SampleType* address(int x = 0, int y = 0) const
        {
            uint8* sample = image + y * stride + x * format.bytes();
            return reinterpret_cast<SampleType*>(sample);
        }

        void save(const std::string& filename);
        void clear(float red, float green, float blue, float alpha);
        void blit(int x, int y, const Surface& source);
        void xflip();
        void yflip();
    };

    class Bitmap : private NonCopyable, public Surface
    {
    public:
        Bitmap(int width, int height, const Format& format, int stride = 0, uint8* image = nullptr);
        Bitmap(Memory memory, const std::string& extension);
        Bitmap(const std::string& filename, const Format& format);
        Bitmap(const std::string& filename);
        Bitmap(Memory memory, const std::string& extension, Palette& palette);
        Bitmap(const std::string& filename, Palette& palette);
        Bitmap(Bitmap&& bitmap);
        ~Bitmap();

        Bitmap& operator = (Bitmap&& bitmap);
    };

} // namespace mango
