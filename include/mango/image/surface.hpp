/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include <string>
#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>
#include <mango/filesystem/file.hpp>
#include <mango/image/format.hpp>
#include <mango/image/decoder.hpp>
#include <mango/image/encoder.hpp>

namespace mango::image
{

    class Surface
    {
    public:
        Format  format;
        u8*     image;
        size_t  stride;
        int     width;
        int     height;

        Surface();
        Surface(const Surface& surface);
        Surface(int width, int height, const Format& format, size_t stride, const void* image);
        Surface(const Surface& source, int x, int y, int width, int height);
        ~Surface();

        Surface& operator = (const Surface& surface);

        u8* address(int x = 0, int y = 0) const
        {
            u8* scan = image + y * stride;
            return scan + size_t(x) * format.bytes();
        }

        template <typename SampleType>
        SampleType* address(int x = 0, int y = 0) const
        {
            u8* ptr = address(x, y);
            return reinterpret_cast<SampleType*>(ptr);
        }

        ImageEncodeStatus save(Stream& stream, const std::string& extension, const ImageEncodeOptions& options = ImageEncodeOptions()) const;
        ImageEncodeStatus save(const std::string& filename, const ImageEncodeOptions& options = ImageEncodeOptions()) const;

        void clear(float red, float green, float blue, float alpha) const;
        void clear(Color color) const;
        void blit(int x, int y, const Surface& source) const;
        void xflip() const;
        void yflip() const;
    };

    class Bitmap : private NonCopyable, public Surface
    {
    public:
        Bitmap(int width, int height, const Format& format, size_t stride = 0);
        Bitmap(const Surface& source, const Format& format);

        Bitmap(ConstMemory memory, const std::string& extension, const ImageDecodeOptions& options = ImageDecodeOptions());
        Bitmap(ConstMemory memory, const std::string& extension, const Format& format, const ImageDecodeOptions& options = ImageDecodeOptions());

        Bitmap(const filesystem::File& file, const ImageDecodeOptions& options = ImageDecodeOptions());
        Bitmap(const filesystem::File& file, const Format& format, const ImageDecodeOptions& options = ImageDecodeOptions());

        Bitmap(const std::string& filename, const ImageDecodeOptions& options = ImageDecodeOptions());
        Bitmap(const std::string& filename, const Format& format, const ImageDecodeOptions& options = ImageDecodeOptions());

        Bitmap(Bitmap&& bitmap);
        ~Bitmap();

        Bitmap& operator = (Bitmap&& bitmap);
    };

    class TemporaryBitmap : public Surface, private NonCopyable
    {
    protected:
        std::unique_ptr<Bitmap> m_bitmap;

    public:
        TemporaryBitmap(const Surface& surface, const Format& format)
            : Surface(surface)
        {
            if (surface.format != format)
            {
                m_bitmap = std::make_unique<Bitmap>(surface, format);
                static_cast<Surface&>(*this) = *m_bitmap;
            }
        }
    };

} // namespace mango::image
