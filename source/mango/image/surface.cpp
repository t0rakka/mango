/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <algorithm>
#include <mango/core/exception.hpp>
#include <mango/core/thread.hpp>
#include <mango/core/string.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/half.hpp>
#include <mango/math/vector.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;

    // ----------------------------------------------------------------------------
    // clear
    // ----------------------------------------------------------------------------

    void clear_u8_scan(u8* dest, int count, u32 color)
    {
		std::memset(dest, u8(color), count);
    }

    void clear_u16_scan(u8* dest, int count, u32 color)
    {
        std::fill_n(reinterpret_cast<u16*>(dest), count, u16(color));
    }

    void clear_u24_scan(u8* dest, int count, u32 color)
    {
        std::fill_n(reinterpret_cast<u24*>(dest), count, u24(color));
    }

    void clear_u32_scan(u8* dest, int count, u32 color)
    {
        std::fill_n(reinterpret_cast<u32*>(dest), count, color);
    }

    template <typename FloatType>
    void clear_float_scan(u8* dest, int count, const FloatType* color, const int size)
    {
        FloatType* d = reinterpret_cast<FloatType*>(dest);
        for (int i = 0; i < count; ++i)
        {
            switch (size)
            {
                case 4: d[3] = color[3];
                case 3: d[2] = color[2];
                case 2: d[1] = color[1];
                case 1: d[0] = color[0];
            }
            d += size;
        }
    }

    template <typename FloatType>
    int config_clear_color(FloatType* color, const Format& format, float red, float green, float blue, float alpha)
    {
        int size = 0;

        // make the parameters indexable
        const float temp[] = { red, green, blue, alpha };

        for (int i = 0; i < 4; ++i)
        {
            if (format.size[i])
            {
                int index = format.offset[i] / (sizeof(FloatType) * 8);
                color[index] = FloatType(temp[i]);
                size = std::max(size, index + 1);
            }
        }

        return size;
    }

    // ----------------------------------------------------------------------------
    // load_surface()
    // ----------------------------------------------------------------------------

    void load_surface(Surface& surface, ConstMemory memory, const std::string& extension, const Format* format)
    {
        ImageDecoder decoder(memory, extension);
        if (decoder.isDecoder())
        {
            ImageHeader header = decoder.header();

            size_t stride = header.width * surface.format.bytes();
            size_t bytes = header.height * stride;

            // configure surface
            surface.width  = header.width;
            surface.height = header.height;
            surface.format = format ? *format : header.format;
            surface.stride = int(stride);
            surface.image  = new u8[bytes];

            // decode
            ImageDecodeStatus status = decoder.decode(surface);
            MANGO_UNREFERENCED(status);
        }
    }

    void load_surface(Surface& surface, const std::string& filename, const Format* format)
    {
        filesystem::File file(filename);
        load_surface(surface, file, filesystem::getExtension(filename), format);
    }

    void load_palette_surface(Surface& surface, ConstMemory memory, const std::string& extension, Palette& palette)
    {
        palette.size = 0;

        ImageDecoder decoder(memory, extension);
        if (decoder.isDecoder())
        {
            ImageHeader header = decoder.header();
            if (header.palette)
            {
                size_t stride = surface.width;
                size_t bytes = header.height * stride;

                // configure surface
                surface.width  = header.width;
                surface.height = header.height;
                surface.format = IndexedFormat(8);
                surface.stride = surface.width;
                surface.image  = new u8[bytes];

                // decode
                ImageDecodeOptions options;
                options.palette = &palette;
                decoder.decode(surface, options, 0, 0, 0);
            }
            else
            {
                // fallback: client requests a palette but image doesn't have one
                load_surface(surface, memory, extension, nullptr);
            }
        }
    }

    void load_palette_surface(Surface& surface, const std::string& filename, Palette& palette)
    {
        filesystem::File file(filename);
        load_palette_surface(surface, file, filesystem::getExtension(filename), palette);
    }

} // namespace

namespace mango
{

    // ----------------------------------------------------------------------------
    // Surface
    // ----------------------------------------------------------------------------

    Surface::Surface()
        : format()
        , image(nullptr)
        , stride(0)
        , width(0)
        , height(0)
    {
    }

    Surface::Surface(const Surface& surface)
        : format(surface.format)
        , image(surface.image)
        , stride(surface.stride)
        , width(surface.width)
        , height(surface.height)
    {
    }

    Surface::Surface(int width, int height, const Format& format, int stride, const void* image)
        : format(format)
        , image(const_cast<u8*>(reinterpret_cast<const u8*>(image)))
        , stride(stride)
        , width(width)
        , height(height)
    {
    }

    Surface::Surface(const Surface& surface, int x, int y, int w, int h)
        : format(surface.format)
        , stride(surface.stride)
    {
        if (x < 0)
        {
            w = std::max(0, w + x);
            x = 0;
        }

        if (y < 0)
        {
            h = std::max(0, h + y);
            y = 0;
        }

        image  = surface.address(x, y);
        width  = std::max(0, std::min(surface.width, x + w) - x);
        height = std::max(0, std::min(surface.height, y + h) - y);
    }

    Surface::~Surface()
    {
    }

    Surface& Surface::operator = (const Surface& surface)
    {
        format = surface.format;
        image = surface.image;
        stride = surface.stride;
        width = surface.width;
        height = surface.height;
        return *this;
    }

    void Surface::save(const std::string& filename, const ImageEncodeOptions& options) const
    {
        ImageEncoder encoder(filename);
        if (encoder.isEncoder())
        {
            filesystem::FileStream file(filename, Stream::WRITE);
            encoder.encode(file, *this, options);
        }
    }

    void Surface::clear(float red, float green, float blue, float alpha) const
    {
        switch (format.type)
        {
            case Format::UNORM:
            {
                void (*func)(u8 *, int, u32) = nullptr;

                switch (format.bits)
                {
                    case 8: func = clear_u8_scan; break;
                    case 16: func = clear_u16_scan; break;
                    case 24: func = clear_u24_scan; break;
                    case 32: func = clear_u32_scan; break;
                }

                if (func)
                {
                    u32 color = format.pack(red, green, blue, alpha);

                    for (int y = 0; y < height; ++y)
                    {
                        func(image + y * stride, width, color);
                    }
                }

                break;
            }

            case Format::FLOAT16:
            {
                float16 color[4];
                int size = config_clear_color<float16>(color, format, red, green, blue, alpha);

                for (int y = 0; y < height; ++y)
                {
                    clear_float_scan<float16>(image + y * stride, width, color, size);
                }

                break;
            }

            case Format::FLOAT32:
            {
                float color[4];
                int size = config_clear_color<float>(color, format, red, green, blue, alpha);

                for (int y = 0; y < height; ++y)
                {
                    clear_float_scan<float>(image + y * stride, width, color, size);
                }

                break;
            }

            default:
                // TODO: not supported?
                break;
        }
    }

    void Surface::clear(ColorRGBA color) const
    {
        const float s = 1.0f / 255.0f;
        clear(color.r * s, color.g * s, color.b * s, color.a * s);
    }

    void Surface::blit(int x, int y, const Surface& source) const
    {
        if (!source.width || !source.height || !source.format.bits || !format.bits)
            return;

        Surface dest(*this, x, y, source.width, source.height);

        if (!dest.width || !dest.height)
            return;

        BlitRect rect;

        rect.src.address = source.image;
        rect.src.stride = source.stride;
        rect.dest.address = dest.image;
        rect.dest.stride = dest.stride;
        rect.width = dest.width;
        rect.height = dest.height;

        if (x < 0)
        {
            rect.src.address -= x * source.format.bytes();
        }

        if (y < 0)
        {
            rect.src.address -= y * source.stride;
        }

        Blitter blitter(dest.format, source.format);

        const int slice = 96;

        if (ThreadPool::getHardwareConcurrency() > 2 && rect.height >= slice * 2)
        {
            ConcurrentQueue queue("blit", Priority::HIGH);

            for (int y = 0; y < rect.height; y += slice)
            {
                queue.enqueue([=, &blitter]
                {
                    int y0 = y;
                    int y1 = std::min(y + slice, rect.height);

                    BlitRect temp = rect;

                    temp.dest.address += y0 * rect.dest.stride;
                    temp.src.address += y0 * rect.src.stride;
                    temp.height = y1 - y0;

                    blitter.convert(temp);
                });
            }
        }
        else
        {
            blitter.convert(rect);
        }
    }

    void Surface::xflip() const
    {
        if (!image || !stride)
            return;

        const int bytes_per_pixel = format.bytes();
        const int half_width = width / 2;

        u8* left = image;
        u8* right = image + (width - 1) * bytes_per_pixel;

        for (int y = 0; y < height; ++y)
        {
            u8* a = left;
            u8* b = right;

            for (int x = 0; x < half_width; ++x)
            {
                // swap pixels using the slowest possible method
                for (int i = 0; i < bytes_per_pixel; ++i)
                {
                    std::swap(a[i], b[i]);
                }

                // next pixel
                a += bytes_per_pixel;
                b -= bytes_per_pixel;
            }

            // next scanline
            left += stride;
            right += stride;
        }
    }

    void Surface::yflip() const
    {
        if (!image || !stride)
            return;

        const int bytes_per_pixel = format.bytes();
        const int bytes_per_scan = width * bytes_per_pixel;
        const int half_height = height / 2;

        u8* top = image;
        u8* bottom = image + (height - 1) * stride;

        for (int y = 0; y < half_height; ++y)
        {
            // swap pixels using the slowest possible method
            for (int i = 0; i < bytes_per_scan; ++i)
            {
                std::swap(top[i], bottom[i]);
            }

            // next scanline
            top += stride;
            bottom -= stride;
        }
    }

    // ----------------------------------------------------------------------------
    // Bitmap
    // ----------------------------------------------------------------------------

    Bitmap::Bitmap(int w, int h, const Format& f, int s)
        : Surface(w, h, f, s, nullptr)
    {
        if (!stride)
        {
            stride = width * format.bytes();
        }

        image = new u8[stride * height];
    }

    Bitmap::Bitmap(const Surface& source, const Format& format)
        : Surface(source.width, source.height, format, 0, nullptr)
    {
        stride = width * format.bytes();
        image = new u8[stride * height];
        blit(0, 0, source);
    }

    Bitmap::Bitmap(ConstMemory memory, const std::string& extension)
    {
        load_surface(*this, memory, extension, nullptr);
    }

    Bitmap::Bitmap(ConstMemory memory, const std::string& extension, const Format& format)
    {
        load_surface(*this, memory, extension, &format);
    }

    Bitmap::Bitmap(const std::string& filename)
    {
        load_surface(*this, filename, nullptr);
    }

    Bitmap::Bitmap(const std::string& filename, const Format& format)
    {
        load_surface(*this, filename, &format);
    }

    Bitmap::Bitmap(ConstMemory memory, const std::string& extension, Palette& palette)
    {
        load_palette_surface(*this, memory, extension, palette);
    }

    Bitmap::Bitmap(const std::string& filename, Palette& palette)
    {
        load_palette_surface(*this, filename, palette);
    }

    Bitmap::Bitmap(Bitmap&& bitmap)
        : Surface(bitmap)
    {
        // move image ownership
        bitmap.image = nullptr;
    }

    Bitmap::~Bitmap()
    {
        delete[] image;
    }

    Bitmap& Bitmap::operator = (Bitmap&& bitmap)
    {
        // copy surface
        format = bitmap.format;
        image = bitmap.image;
        stride = bitmap.stride;
        width = bitmap.width;
        height = bitmap.height;

        // move image ownership
        bitmap.image = nullptr;

        return *this;
    }

} // namespace mango
