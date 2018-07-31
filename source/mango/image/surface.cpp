/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <algorithm>
#include <mango/core/exception.hpp>
#include <mango/core/thread.hpp>
#include <mango/core/string.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/half.hpp>
#include <mango/simd/simd.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;

    // ----------------------------------------------------------------------------
	// fill_aligned
    // ----------------------------------------------------------------------------

    template <typename T>
    inline uint8* u8_fill(uint8* dest, int count, T value)
    {
        T* p = reinterpret_cast<T*>(dest);
        for (int i = 0; i < count; ++i)
        {
            p[i] = value;
        }
        return dest + count * sizeof(T);
    }

	template <typename T, typename E>
	void fill_aligned(uint8* dest, T value, E expanded_value, int count)
	{
		// compute padding required for alignment
		const ptrdiff_t address = dest - reinterpret_cast<uint8*>(0);
        const ptrdiff_t mask = sizeof(E) - 1;
		const int padding_in_bytes = int(-address & mask);

        // fill padding values to align the dest pointer
		const int padding_count = int(padding_in_bytes / sizeof(T));
        dest = u8_fill(dest, padding_count, value);
		count -= padding_count;

        // fill expanded value to aligned address
		const int expand_ratio = sizeof(E) / sizeof(T);
		const int expand_count = count / expand_ratio;
        dest = u8_fill(dest, expand_count, expanded_value);
		count -= expand_count * expand_ratio;

        // fill remaining values
        u8_fill(dest, count, value);
	}

    // ----------------------------------------------------------------------------
    // clear
    // ----------------------------------------------------------------------------

    void clear_uint8_scan(uint8* dest, int count, uint32 color)
    {
        const uint8 value = static_cast<uint8>(color);
		std::memset(dest, value, count);
    }

    void clear_uint16_scan(uint8* dest, int count, uint32 color)
    {
#if defined(MANGO_ENABLE_SIMD)
		if (count >= 32)
		{
			// 128 bit fill
			uint16 value16 = static_cast<uint16>(color);
			uint32 value32 = (color << 16) | color;
			simd::uint32x4 value128 = simd::uint32x4_set1(value32);
			fill_aligned(dest, value16, value128, count);
		}
#else
		if (count >= 16)
		{
			uint16 value16 = static_cast<uint16>(color);
			uint32 value32 = (color << 16) | color;
#ifdef MANGO_CPU_64BIT
			// 64 bit fill
			uint64 value64 = (uint64(value32) << 32) | value32;
			fill_aligned(dest, value16, value64, count);
#else
			// 32 bit fill
			fill_aligned(dest, value16, value32, count);
#endif
		}
#endif
		else
		{
			// 16 bit fill
			uint16 value16 = static_cast<uint16>(color);
            u8_fill(dest, count, value16);
		}
    }

    void clear_uint24_scan(uint8* dest, int count, uint32 color)
    {
        uint24 value24 = color;
        u8_fill(dest, count, value24);
    }

    void clear_uint32_scan(uint8* dest, int count, uint32 color)
    {
#if defined(MANGO_ENABLE_SIMD)
		if (count >= 16)
		{
			// 128 bit fill
			simd::uint32x4 value128 = simd::uint32x4_set1(color);
			fill_aligned(dest, color, value128, count);
		}
		else
#else

#ifdef MANGO_CPU_64BIT
		if (count >= 8)
		{
			// 64 bit fill
			uint64 value64 = (uint64(color) << 32) | color;
			fill_aligned(dest, color, value64, count);
		}
		else
#endif

#endif
		{
			// 32 bit fill
            u8_fill(dest, count, color);
		}
    }

    template <typename FloatType>
    void clear_float_scan(uint8* dest, int count, const FloatType* color, const int size)
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
            uint32 mask = format.mask(i);
            if (mask)
            {
                int index = clamp(u32_index_of_lsb(mask), 0, 3);
                color[index] = FloatType(temp[i]);
                size = std::max(size, index + 1);
            }
        }

        return size;
    }

    // ----------------------------------------------------------------------------
    // load_surface()
    // ----------------------------------------------------------------------------

    Surface load_surface(Memory memory, const std::string& extension, const Format* format)
    {
        Surface surface(0, 0, Format(), 0, nullptr);

        ImageDecoder decoder(memory, extension);
        if (decoder.isDecoder())
        {
            ImageHeader header = decoder.header();

            // configure surface
            surface.width  = header.width;
            surface.height = header.height;
            surface.format = format ? *format : header.format;
            surface.stride = surface.width * surface.format.bytes();
            surface.image  = new uint8[surface.height * surface.stride];

            // decode
            decoder.decode(surface, nullptr, 0, 0, 0);
        }

        return surface;
    }

    Surface load_surface(const std::string& filename, const Format* format)
    {
        const std::string extension = getExtension(filename);
        File file(filename);
        Surface surface = load_surface(file, extension, format);
        return surface;
    }

    Surface load_palette_surface(Memory memory, const std::string& extension, Palette& palette)
    {
        Surface surface(0, 0, Format(), 0, nullptr);
        palette.size = 0;

        ImageDecoder decoder(memory, extension);
        if (decoder.isDecoder())
        {
            ImageHeader header = decoder.header();
            if (header.palette)
            {
                // configure surface
                surface.width  = header.width;
                surface.height = header.height;
                surface.format = Format(8, 0xff, 0);
                surface.stride = surface.width;
                surface.image  = new uint8[surface.height * surface.stride];

                // decode
                decoder.decode(surface, &palette, 0, 0, 0);
            }
            else
            {
                // fallback: client requests a palette but image doesn't have one
                surface = load_surface(memory, extension, nullptr);
            }
        }

        return surface;
    }

    Surface load_palette_surface(const std::string& filename, Palette& palette)
    {
        const std::string extension = getExtension(filename);
        File file(filename);
        Surface surface = load_palette_surface(file, extension, palette);
        return surface;
    }

} // namespace

namespace mango
{

    // ----------------------------------------------------------------------------
    // Surface
    // ----------------------------------------------------------------------------

    Surface::Surface()
        : width(0)
        , height(0)
        , stride(0)
        , image(nullptr)
    {
    }

    Surface::Surface(int width, int height, const Format& format, int stride, uint8* image)
        : width(width)
        , height(height)
        , stride(stride)
        , format(format)
        , image(image)
    {
    }

    Surface::Surface(const Surface& source, int x, int y, int _width, int _height)
        : stride(source.stride)
        , format(source.format)
    {
        // clip rectangle to source surface
        const int x0 = std::max(0, x);
        const int y0 = std::max(0, y);
        const int x1 = std::min(source.width, x + _width);
        const int y1 = std::min(source.height, y + _height);

        // compute resulting surface
        width  = std::max(0, x1 - x0);
        height = std::max(0, y1 - y0);
        image  = source.address<uint8>(x0, y0);
    }

    Surface::~Surface()
    {
    }

    void Surface::save(const std::string& filename, float quality)
    {
        ImageEncoder encoder(filename);
        if (encoder.isEncoder())
        {
            FileStream file(filename, Stream::WRITE);
            encoder.encode(file, *this, quality);
        }
    }

    void Surface::clear(float red, float green, float blue, float alpha)
    {
        switch (format.type)
        {
            case Format::UNORM:
            {
                void (*func)(uint8 *, int, uint32) = nullptr;

                switch (format.bits)
                {
                    case 8: func = clear_uint8_scan; break;
                    case 16: func = clear_uint16_scan; break;
                    case 24: func = clear_uint24_scan; break;
                    case 32: func = clear_uint32_scan; break;
                }

                if (func)
                {
                    uint32 color = format.pack(red, green, blue, alpha);

#if 1
                    // super simple multi-threaded buffer clearing

                    std::thread top([=] {
                        for (int y = 0; y < height / 2; ++y)
                        {
                            func(image + y * stride, width, color);
                        }
                    });

                    std::thread bottom([=] {
                        for (int y = height / 2; y < height; ++y)
                        {
                            func(image + y * stride, width, color);
                        }
                    });

                    top.join();
                    bottom.join();
#else
                    for (int y = 0; y < height; ++y)
                    {
                        func(image + y * stride, width, color);
                    }
#endif
                }

                break;
            }

            case Format::FP16:
            {
                half color[4];
                int size = config_clear_color<half>(color, format, red, green, blue, alpha);

                for (int y = 0; y < height; ++y)
                {
                    clear_float_scan<half>(image + y * stride, width, color, size);
                }

                break;
            }

            case Format::FP32:
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

    void Surface::blit(int x, int y, const Surface& source)
    {
        if (!source.width || !source.height || !source.format.bits || !format.bits)
            return;

        Surface dest(*this, x, y, source.width, source.height);

        if (!dest.width || !dest.height)
            return;

        BlitRect rect;

        rect.srcImage = source.image;
        rect.srcStride = source.stride;
        rect.destImage = dest.image;
        rect.destStride = dest.stride;
        rect.width = dest.width;
        rect.height = dest.height;

        ConcurrentQueue queue("blit", Priority::HIGH);

        Blitter blitter(dest.format, source.format);

        const int threads = ThreadPool::getInstanceSize();
        const int tasksize = (rect.width * rect.height) / threads;

        const bool fast = dest.format == source.format;

        // don't use thread pool for:
        // - really small tasks
        // - when the pixel formats are identical ("fast mode")
        const int N = (tasksize < 8192) || fast ? 1 : threads;
        const int section = rect.height / N;

        int ypos = 0;

        // queue conversion tasks
        for (int i = 0; i < N; ++i)
        {
            const bool last = (i == (N - 1));
            const int ycount = last ? rect.height - ypos : section;

            if (N == 1)
            {
                // execute on main thread
                BlitRect temp = rect;

                temp.destImage += ypos * rect.destStride;
                temp.srcImage += ypos * rect.srcStride;
                temp.height = ycount;

                blitter.convert(temp);
            }
            else
            {
                queue.enqueue([=, &blitter]
                {
                    BlitRect temp = rect;

                    temp.destImage += ypos * rect.destStride;
                    temp.srcImage += ypos * rect.srcStride;
                    temp.height = ycount;

                    blitter.convert(temp);
                });
            }

            ypos += section;
        }

        queue.wait();
    }

    void Surface::xflip()
    {
        if (!image || !stride)
            return;

        const int bytes_per_pixel = format.bytes();
        const int half_width = width / 2;

        uint8* left = image;
        uint8* right = image + (width - 1) * bytes_per_pixel;

        for (int y = 0; y < height; ++y)
        {
            uint8* a = left;
            uint8* b = right;

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

    void Surface::yflip()
    {
        if (!image || !stride)
            return;

        const int bytes_per_pixel = format.bytes();
        const int bytes_per_scan = width * bytes_per_pixel;
        const int half_height = height / 2;

        uint8* top = image;
        uint8* bottom = image + (height - 1) * stride;

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

    Bitmap::Bitmap(int _width, int _height, const Format& _format, int _stride, uint8* _image)
        : Surface(_width, _height, _format, _stride, _image)
    {
        if (!stride)
            stride = width * format.bytes();

        if (!image)
            image = new uint8[stride * height];
    }

    Bitmap::Bitmap(Memory memory, const std::string& extension)
        : Surface(load_surface(memory, extension, nullptr))
    {
    }

    Bitmap::Bitmap(Memory memory, const std::string& extension, const Format& format)
        : Surface(load_surface(memory, extension, &format))
    {
    }

    Bitmap::Bitmap(const std::string& filename)
        : Surface(load_surface(filename, nullptr))
    {
    }

    Bitmap::Bitmap(const std::string& filename, const Format& format)
        : Surface(load_surface(filename, &format))
    {
    }

    Bitmap::Bitmap(Memory memory, const std::string& extension, Palette& palette)
        : Surface(load_palette_surface(memory, extension, palette))
    {
    }

    Bitmap::Bitmap(const std::string& filename, Palette& palette)
        : Surface(load_palette_surface(filename, palette))
    {
    }

    Bitmap::Bitmap(Bitmap&& bitmap)
        : Surface(bitmap)
    {
        bitmap.image = nullptr;
    }

    Bitmap::~Bitmap()
    {
        delete[] image;
    }

    Bitmap& Bitmap::operator = (Bitmap&& bitmap)
    {
        // copy surface
        width = bitmap.width;
        height = bitmap.height;
        format = bitmap.format;
        stride = bitmap.stride;
        image = bitmap.image;

        // move image ownership
        bitmap.image = nullptr;

        return *this;
    }

} // namespace mango
