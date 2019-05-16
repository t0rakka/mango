/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include <string>
#include "../core/configure.hpp"
#include "../core/object.hpp"
#include "../core/memory.hpp"
#include "../filesystem/file.hpp"
#include "format.hpp"
#include "encoder.hpp"

namespace mango
{

    class Surface
    {
    protected:
        Surface();

    public:
        Format  format;
        u8*     image;
        int     stride;
        int     width;
        int     height;

        Surface(int width, int height, const Format& format, int stride, void* image);
        Surface(const Surface& source, int x, int y, int width, int height);
        ~Surface();

        u8* address(int x = 0, int y = 0) const
        {
            u8* scan = image + y * stride;
            return scan + x * format.bytes();
        }

        template <typename SampleType>
        SampleType* address(int x = 0, int y = 0) const
        {
            SampleType* scan = reinterpret_cast<SampleType*>(image + y * stride);
            return scan + x;
        }

        void save(const std::string& filename, const ImageEncoderOptions& options = ImageEncoderOptions());
        void clear(float red, float green, float blue, float alpha);
        void blit(int x, int y, const Surface& source);
        void xflip();
        void yflip();
    };

    class Bitmap : private NonCopyable, public Surface
    {
    public:
        Bitmap(int width, int height, const Format& format, int stride = 0, u8* image = nullptr);
        Bitmap(Memory memory, const std::string& extension);
        Bitmap(Memory memory, const std::string& extension, const Format& format);
        Bitmap(const std::string& filename);
        Bitmap(const std::string& filename, const Format& format);
        Bitmap(Memory memory, const std::string& extension, Palette& palette);
        Bitmap(const std::string& filename, Palette& palette);
        Bitmap(Bitmap&& bitmap);
        ~Bitmap();

        Bitmap& operator = (Bitmap&& bitmap);
    };

} // namespace mango
