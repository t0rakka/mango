/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include <string>
#include <mango/core/configure.hpp>
#include <mango/core/memory.hpp>
#include <mango/filesystem/file.hpp>
#include <mango/image/format.hpp>
#include <mango/image/color.hpp>
#include <mango/image/decoder.hpp>
#include <mango/image/encoder.hpp>

namespace mango::image
{

    class Surface
    {
    public:
        Format   format;
        u8*      image;
        Palette* palette;
        size_t   stride;
        int      width;
        int      height;

        Surface();
        Surface(const Surface& surface, bool yflip = false);
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

        operator const Palette& () const;
        operator Palette& ();

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
        Bitmap(const Surface& source);
        Bitmap(const Surface& source, const Format& format);
        Bitmap(const ImageHeader& header);
        Bitmap(const ImageHeader& header, const Format& format);

        Bitmap(ConstMemory memory, const std::string& extension, const ImageDecodeOptions& options = ImageDecodeOptions());
        Bitmap(ConstMemory memory, const std::string& extension, const Format& format, const ImageDecodeOptions& options = ImageDecodeOptions());

        Bitmap(const filesystem::File& file, const ImageDecodeOptions& options = ImageDecodeOptions());
        Bitmap(const filesystem::File& file, const Format& format, const ImageDecodeOptions& options = ImageDecodeOptions());

        Bitmap(const std::string& filename, const ImageDecodeOptions& options = ImageDecodeOptions());
        Bitmap(const std::string& filename, const Format& format, const ImageDecodeOptions& options = ImageDecodeOptions());

        ~Bitmap();
    };

    /*
        TemporaryBitmap is convenient when source surface does not match what is needed; it creates a temporary
        copy which is in correct format and dimensions. The temporary copy is ONLY created when surface format
        and dimensions don't match.
    */
    class TemporaryBitmap : private NonCopyable, public Surface
    {
    protected:
        std::unique_ptr<Bitmap> m_bitmap;

    public:
        TemporaryBitmap(const Surface& surface, const Format& format, bool yflip = false);
        TemporaryBitmap(const Surface& surface, int width, int height, const Format& format, bool yflip = false);
    };

    /*
        The DecodeTargetBitmap is used as temporary decoding target; it creates a temporary bitmap when
        target format or dimensions don't match the decoded iamge. It does resolve the temporary bitmap
        into the target surface in the destructor if direct decoding wasn't used (matcing dimensions and format).
    */
    class DecodeTargetBitmap : private NonCopyable, public Surface
    {
    protected:
        std::unique_ptr<Bitmap> m_bitmap;
        Surface m_target;

    public:
        DecodeTargetBitmap(const Surface& target, int width, int height, const Format& format, bool yflip = false);
        DecodeTargetBitmap(const Surface& target, int width, int height, const Format& format, const Palette& palette, bool yflip = false);
        ~DecodeTargetBitmap();

        bool isDirect() const;
        void resolve(int x, int y, int width, int height);
        void resolve();
    };

    class LuminanceBitmap : public Bitmap
    {
    public:
        LuminanceBitmap(const Surface& source, bool alpha = false, bool force_linear = true);
    };

    // NOTE: The surface format must be 32 bit RGBA
    void resolve(const Surface& surface, const Surface& indexed);
    void transform(const Surface& surface, ConstMemory icc);
    void srgbToLinear(const Surface& surface);
    void linearToSRGB(const Surface& surface);

} // namespace mango::image
