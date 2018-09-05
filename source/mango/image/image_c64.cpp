/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Commodore 64 decoders copyright (C) 2011 Toni Lönnberg. All rights reserved.
*/

#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

#define ID "ImageDecoder.C64: "

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // Commodore 64 utilities
    // ------------------------------------------------------------

    const BGRA c64_palette[16] =
    { 
        0xFF000000,
        0xFFFFFFFF,
        0xFF68372B,
        0xFF70A4B2,
        0xFF6F3D86,
        0xFF588D43,
        0xFF352879,
        0xFFB8C76F,
        0xFF6F4F25,
        0xFF433900,
        0xFF9A6759,
        0xFF444444,
        0xFF6C6C6C,
        0xFF9AD284,
        0xFF6C5EB5,
        0xFF959595,
    };

    void resolve_palette(Surface& s, u8* data, int width, int height, Palette& palette)
    {
        for (int y = 0; y < height; ++y)
        {
            BGRA* scan = s.address<BGRA>(0, y);

            for (int x = 0; x < width; ++x)
            {
                u8 index = *data++;
                scan[x] = palette[index];
            }
        }
    }

    void rle_ecb(u8* buffer, const u8* input, int scansize, int insize, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;
        const u8* input_end = input + insize;

        for ( ; buffer < buffer_end && input < input_end; )
        {
            u8 v = *input++;

            if (v == escape_char)
            {
                int n = *input++;
                if (n == 0)
                {
                    n = 256;
                }
                std::memset(buffer, *input++, n);
                buffer += n;
            }
            else
            {
                *buffer++ = v;
            }
        }
    }

    void convert_multicolor_bitmap(int width, int height, u8* image, 
                                   const u8* bitmap_c64, const u8* video_ram, const u8* color_ram, 
                                   const u8* background, 
                                   const u8* opcode_colors,
                                   int background_mode, bool fli)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int x_offset = x & 0x7;
                int y_offset = y & 0x7;
                int bitmap_offset = (x & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                int screen_offset = bitmap_offset >> 3;
                int offset = x + (y * width);

                u8 byte = bitmap_c64[bitmap_offset];
                int bit_pattern = (byte >> (6 - (x_offset & 0x6))) & 0x3;

                u8 index = 0;
                switch (bit_pattern)
                {
                case 0:
                    switch (background_mode)
                    {
                    case 0:
                        index = 0;
                        break;

                    case 1:
                        index = *background & 0xf;
                        break;

                    case 2:
                        index = background[y] & 0xf;
                        break;
                    }
                    break;

                case 1:
                    if (fli)
                    {
                        // Emulate the FLI bug
                        if (x < 24)
                        {
                            index = 0xf;
                        }
                        else
                        {
                            index = (video_ram[screen_offset + (y_offset * 0x400)] >> 4);
                        }
                    }
                    else
                    {
                        index = (video_ram[screen_offset] >> 4);
                    }
                    break;

                case 2:
                    if (fli)
                    {
                        // Emulate the FLI bug
                        if (x < 24)
                        {
                            index = 0xf;
                        }
                        else
                        {
                            index = (video_ram[screen_offset + (y_offset * 0x400)] & 0xf);
                        }
                    }
                    else
                    {
                        index = (video_ram[screen_offset] & 0xf);
                    }
                    break;

                case 3:
                    if (fli)
                    {
                        // Emulate the FLI bug
                        if (x < 24)
                        {
                            index = (opcode_colors[y] >> 4) & 0xf;
                        }
                        else
                        {
                            index = color_ram[screen_offset] & 0xf;
                        }
                    }
                    else
                    {
                        index = color_ram[screen_offset] & 0xf;
                    }
                    break;
                }

                image[offset] = index;
            }
        }
    }

    void multicolor_to_surface(Surface& s, const u8 *data, int width, int height, 
                            u32 bitmap_offset, u32 video_ram_offset, u32 color_ram_offset, 
                            u32 background_offset, u32 opcode_colors_offset, 
                            int background_mode, bool fli)
    {
        Palette palette;
        palette.size = 16;

        for (int i = 0; i < 16; ++i)
        {
            palette[i] = c64_palette[i];
        }

        std::vector<u8> temp(width * height, 0);

        convert_multicolor_bitmap(width, height, temp.data(), 
                                  data + bitmap_offset, data + video_ram_offset, 
                                  data + color_ram_offset, data + background_offset, data + opcode_colors_offset,
                                  background_mode, fli);

        resolve_palette(s, temp.data(), width, height, palette);
    }

#if 0

    surface* multicolor_interlace_to_surface(const u8 *data, int width, int height, u32 
                                             bitmap_offset_1, u32 bitmap_offset_2, 
                                             u32 video_ram_offset_1, u32 video_ram_offset_2, 
                                             u32 color_ram_offset, u8 *background_colors,
                                             u8 *opcode_colors,
                                             int background_mode,
                                             bool fli,
                                             int mode)
    {
        ucolor palette[256];
        std::memset(palette, 0, 256 * sizeof(ucolor));
        std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

        u8 *bitmap1 = new u8[width * height];
        u8 *bitmap2 = new u8[width * height];

        convert_multicolor_bitmap(width, height, bitmap1, data + bitmap_offset_1, data + video_ram_offset_1, data + color_ram_offset, background_colors, opcode_colors, background_mode, fli);
        convert_multicolor_bitmap(width, height, bitmap2, data + bitmap_offset_2, data + video_ram_offset_2, data + color_ram_offset, background_colors, opcode_colors, background_mode, fli);

        //surface* so = bitmap::create(width, height, pixelformat::argb8888);
        //ucolor* image = so->lock<ucolor>();
        //std::memset(image, 0, width * height * sizeof(ucolor));

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int offset = x + (y * width);

                image[offset].a = 0xff;

                if (mode == 0)
                {
                    image[offset].r = (palette[bitmap1[offset]].r >> 1) + (palette[bitmap2[offset]].r >> 1);
                    image[offset].g = (palette[bitmap1[offset]].g >> 1) + (palette[bitmap2[offset]].g >> 1);
                    image[offset].b = (palette[bitmap1[offset]].b >> 1) + (palette[bitmap2[offset]].b >> 1);
                }
                else if (mode == 1)
                {
                    if ((offset % 320) == 0)
                    {
                        image[offset].r = (palette[bitmap1[offset]].r >> 1);
                        image[offset].g = (palette[bitmap1[offset]].g >> 1);
                        image[offset].b = (palette[bitmap1[offset]].b >> 1);
                    }
                    else
                    {
                        image[offset].r = (palette[bitmap1[offset + 0]].r >> 1) + (palette[bitmap2[offset - 1]].r >> 1);
                        image[offset].g = (palette[bitmap1[offset + 0]].g >> 1) + (palette[bitmap2[offset - 1]].g >> 1);
                        image[offset].b = (palette[bitmap1[offset + 0]].b >> 1) + (palette[bitmap2[offset - 1]].b >> 1);
                    }
                }
                else if (mode == 2)
                {
                    if ((offset & 0x1) == 0)
                    {
                        image[offset].r = palette[bitmap1[offset]].r;
                        image[offset].g = palette[bitmap1[offset]].g;
                        image[offset].b = palette[bitmap1[offset]].b;
                    }
                    else
                    {
                        image[offset].r = palette[bitmap2[offset]].r;
                        image[offset].g = palette[bitmap2[offset]].g;
                        image[offset].b = palette[bitmap2[offset]].b;
                    }
                }
            }
        }

        delete[] bitmap2;
        delete[] bitmap1;
    }
#endif

    void convert_hires_bitmap(int width, int height, 
                              u8* image, const u8* bitmap_c64, const u8* video_ram, 
                              bool fli,
                              bool show_fli_bug, 
                              u8 fli_bug_color)
    {
        int x, y;
        for (y = 0; y < height; ++y)
        {
            for (x = 0; x < width; ++x)
            {
                int x_offset = x & 0x7;
                int y_offset = y & 0x7;
                int bitmap_offset = (x & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                int screen_offset = bitmap_offset >> 3;
                int offset = x + (y * width);

                u8 byte = bitmap_c64[bitmap_offset];
                int bit_pattern = (byte >> (7 - x_offset)) & 0x1;

                u8 index = 0;
                if (x < 24)
                {
                    if (show_fli_bug)
                    {
                        index = 0xf;
                    }
                    else
                    {
                        index = fli_bug_color;
                    }
                }
                else
                {
                    switch (bit_pattern)
                    {
                    case 0:
                        if (fli)
                        {
                            index = (video_ram[screen_offset + (y_offset * 0x400)] & 0xf);
                        }
                        else
                        {
                            index = (video_ram[screen_offset] & 0xf);
                        }
                        break;

                    case 1:
                        if (fli)
                        {
                            index = (video_ram[screen_offset + (y_offset * 0x400)] >> 4);
                        }
                        else
                        {
                            index = (video_ram[screen_offset] >> 4);
                        }
                        break;
                    }
                }

                image[offset] = index;
            }
        }
    }

    void hires_to_surface(Surface& s, const u8* data, int width, int height, 
                              u32 bitmap_offset, u32 video_ram_offset, 
                              bool fli = false,
                              bool show_fli_bug = false,
                              u8 fli_bug_color = 0)
    {
        Palette palette;
        palette.size = 16;

        for (int i = 0; i < 16; ++i)
        {
            palette[i] = c64_palette[i];
        }

        std::vector<u8> temp(width * height, 0);

        convert_hires_bitmap(width, height, temp.data(), data + bitmap_offset, data + video_ram_offset, fli, show_fli_bug, fli_bug_color);

        resolve_palette(s, temp.data(), width, height, palette);
    }

#if 0
    surface* hires_interlace_to_surface(const u8* data, int width, int height, u32 
                                        bitmap_offset_1, u32 bitmap_offset_2, 
                                        u32 video_ram_offset_1, u32 video_ram_offset_2, 
                                        bool fli = false,
                                        bool show_fli_bug = false,
                                        u8 fli_bug_color = 0)
    {
        int x, y;

        ucolor palette[256];
        std::memset(palette, 0, 256 * sizeof(ucolor));
        std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

        u8* bitmap1 = new u8[width * height];
        u8* bitmap2 = new u8[width * height];

        convert_hires_bitmap(width, height, bitmap1, data + bitmap_offset_1, data + video_ram_offset_1, fli, show_fli_bug, fli_bug_color);
        convert_hires_bitmap(width, height, bitmap2, data + bitmap_offset_2, data + video_ram_offset_2, fli, show_fli_bug, fli_bug_color);

        surface* so = bitmap::create(width, height, pixelformat::argb8888);
        ucolor* image = so->lock<ucolor>();
        std::memset(image, 0, width * height * sizeof(ucolor));

        for (y = 0; y < height; ++y)
        {
            for (x = 0; x < width; ++x)
            {
                int offset = x + (y * width);

                image[offset].a = 0xff;
                image[offset].r = (palette[bitmap1[offset]].r >> 1) + (palette[bitmap2[offset]].r >> 1);
                image[offset].g = (palette[bitmap1[offset]].g >> 1) + (palette[bitmap2[offset]].g >> 1);
                image[offset].b = (palette[bitmap1[offset]].b >> 1) + (palette[bitmap2[offset]].b >> 1);
            }
        }

        so->unlock();

				delete[] bitmap2;
        delete[] bitmap1;
        
        return so;
    }

#endif

    bool check_format(u16 format_address, size_t format_size, u16 load_address, size_t size)
    {
        return load_address == format_address && size == format_size;
    }

    // ------------------------------------------------------------
    // generic
    // ------------------------------------------------------------

    struct header_generic
    {
        int width = 0;
        int height = 0;
        bool compressed = false;
        u8 escape_char = 0;

        u8* parse(u8* data, size_t size, u16 format_address, size_t format_size)
        {
            LittleEndianPointer p = data;
            u16 load_address = p.read16();

            if (check_format(format_address, format_size, load_address, size))
            {
                width = 320;
                height = 200;
                return p;
            }

            return nullptr;
        }

        void multicolor_load(Surface& s, u8* data,
                            u32 bitmap_offset, u32 video_ram_offset, 
                            u32 color_ram_offset, u32 background_offset, u32 opcode_colors_offset,
                            int background_mode, bool fli)
        {
            multicolor_to_surface(s, data, width, height, 
                                  bitmap_offset, video_ram_offset, color_ram_offset, background_offset, opcode_colors_offset,
                                  background_mode, fli);
        }
    };


#if 0


    surface* generic_hires_load(stream* s, u16 format_address, int format_size, u32 bitmap_offset, u32 video_ram_offset, bool fli)
    {
        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, format_address, format_size);

        if (data)
        {
            return hires_to_surface(data, header.width, header.height, bitmap_offset, video_ram_offset, fli);
        }

        return NULL;
    }


    // ------------------------------------------------------------
    // imagefilter Art Studio
    // ------------------------------------------------------------
    imageheader art_header(stream* s)
    {
        return generic_header(s, 0x2000, 9009);
    }

    surface* art_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_hires_load(s, 0x2000, 9009, 0x0, 0x1f40, false);
    }

    // ------------------------------------------------------------
    // imagefilter Artist 64
    // ------------------------------------------------------------
    imageheader a64_header(stream* s)
    {
        return generic_header(s, 0x4000, 10242);
    }

    surface* a64_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x4000, 10242, 0x0, 0x2000, 0x2400, 0x27ff, 0x0, 1, false);
    }

    // ------------------------------------------------------------
    // imagefilter Blazing Paddles
    // ------------------------------------------------------------
    imageheader blp_header(stream* s)
    {
        return generic_header(s, 0xa000, 10242);
    }

    surface* blp_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0xa000, 10242, 0x0, 0x2000, 0x2400, 0x1f80, 0x0, 1, false);
    }

    // ------------------------------------------------------------
    // imagefilter CDU-Paint
    // ------------------------------------------------------------
    imageheader cdu_header(stream* s)
    {
        return generic_header(s, 0x7eef, 10277);
    }

    surface* cdu_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x7eef, 10277, 0x111, 0x2051, 0x2439, 0x2821, 0x0, 1, false);
    }

    // ------------------------------------------------------------
    // imagefilter Dolphin Ed
    // ------------------------------------------------------------
    imageheader dol_header(stream* s)
    {
        return generic_header(s, 0x5800, 10242);
    }

    surface* dol_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x5800, 10242, 0x800, 0x400, 0x0, 0x7e8, 0x0, 1, false);
    }

    // ------------------------------------------------------------
    // imagefilter Doodle
    // ------------------------------------------------------------
    imageheader dd_header(stream* s)
    {
        return generic_header(s, 0x1c00, 9218);
    }

    surface* dd_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_hires_load(s, 0x1c00, 9218, 0x400, 0x0, false);
    }

    // ------------------------------------------------------------
    // imagefilter Drazlace
    // ------------------------------------------------------------
    const u8* read_header_drl(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x5800, 18242, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            header.compressed = false;
            return xf;
        }
        else
        {
            if (load_address == 0x5800)
            {
                u8 keyword[] = "DRAZLACE! 1.0";
                if (std::memcmp(keyword, xf, sizeof(keyword) - 1) == 0)
                {
                    xf += sizeof(keyword) - 1;

                    header.width = 320;
                    header.height = 200;
                    header.compressed = true;
                    header.escape_char = xf.read<u8>();

                    return xf;
                }
            }
        }

        return NULL;
    }

    imageheader drl_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;
        image_header.format = pixelformat::argb8888;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_drl(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* drl_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_drl(header, data, size);

        if (data)
        {
            const u8* buffer = data;
            u8* temp = NULL;

            if (header.compressed)
            {
                temp = new u8[18240];
                rle_ecb(temp, data, 18240, size - 3, header.escape_char);
                buffer = temp;
            }

            u8* background = new u8[200];
            std::memset(background, *(buffer + 0x2740), 200);

            surface* so = multicolor_interlace_to_surface(buffer, header.width, header.height, 0x800, 0x2800, 0x400, 0x400, 0x0, background, 0x0, 2, false, 2);

            delete [] background;

            if (temp)
            {
                delete [] temp;
            }

            return so;

        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Drazpaint
    // ------------------------------------------------------------
    const u8* read_header_drz(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x5800, 10051, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            header.compressed = false;
            return xf;
        }
        else
        {
            if (load_address == 0x5800)
            {
                u8 keyword[] = "DRAZPAINT 2.0";
                if (std::memcmp(keyword, xf, sizeof(keyword) - 1) == 0)
                {
                    xf += sizeof(keyword) - 1;

                    header.width = 320;
                    header.height = 200;
                    header.compressed = true;
                    header.escape_char = xf.read<u8>();

                    return xf;
                }
            }
        }

        return NULL;
    }

    imageheader drz_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_drz(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* drz_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_drz(header, data, size);

        if (data)
        {
            const u8* buffer = data;
            u8* temp = NULL;

            if (header.compressed)
            {
                temp = new u8[10049];
                rle_ecb(temp, data, 10049, size - 3, header.escape_char);
                buffer = temp;
            }

            surface* so = multicolor_to_surface(buffer, header.width, header.height, 0x800, 0x400, 0x0, 0x2740, 0x0, false, false);

            if (temp)
            {
                delete [] temp;
            }

            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter ECI Graphic Editor v1.0
    // ------------------------------------------------------------
    const u8* read_header_eci(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x4000, 32770, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            header.compressed = false;
            return xf;
        }
        else
        {
            if (load_address == 0x4000)
            {
                header.width = 320;
                header.height = 200;
                header.compressed = true;
                header.escape_char = xf.read<u8>();
                return xf;
            }
        }

        return NULL;
    }

    imageheader eci_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_eci(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* eci_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_eci(header, data, size);

        if (data)
        {
            const u8* buffer = data;
            u8* temp = NULL;

            if (header.compressed)
            {
                temp = new u8[32768];
                rle_ecb(temp, data, 32768, size - 3, header.escape_char);
                buffer = temp;
            }

            surface* so = hires_interlace_to_surface(buffer, header.width, header.height, 0x0, 0x4000, 0x2000, 0x6000, true, false, 0);

            if (temp)
            {
                delete [] temp;
            }

            return so;

//            return hires_interlace_to_surface(data, header.width, header.height, 0x0, 0x4000, 0x2000, 0x6000, true, false, 0);
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Face Painter
    // ------------------------------------------------------------
    imageheader fpt_header(stream* s)
    {
        return generic_header(s, 0x4000, 10004);
    }

    surface* fpt_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x4000, 10004, 0x0, 0x1f40, 0x2328, 0x2712, 0x0, 1, false);
    }

    // ------------------------------------------------------------
    // imagefilter FLI Designer 1.1 & 2.0 (FBI Crew)
    // ------------------------------------------------------------
    imageheader fd2_header(stream* s)
    {
        return generic_header(s, 0x3c00, 17409);
    }

    surface* fd2_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x3c00, 17409, 0x2400, 0x400, 0x0, 0x0, 0x0, 0, true);
    }

    // ------------------------------------------------------------
    // imagefilter FLI-Profi
    // ------------------------------------------------------------
    imageheader fpr_header(stream* s)
    {
        return generic_header(s, 0x3780, 18370);
    }

    surface* fpr_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x3780, 18370);

        if (data)
        {
            u8 sprite_color1 = data[0x448];
            u8 sprite_color2 = data[0x449];
            const u8 *sprite_colors = data + 0x280;

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            u8* image = so->lock<u8>();
            std::memset(image, 0, header.width * header.height);

            convert_multicolor_bitmap(header.width, header.height, image, data + 0x2880, data + 0x880, data + 0x480, NULL, data + 0x380, 0, true);

            // Overlay sprite data
            // - Y-expanded
            // - Switching VIC bank every two scanlines, pattern: 1221
            int x, y;
            for (y = 0; y < 200; ++y)
            {
                for (x = 0; x < 24; ++x)
                {
                    int offset = x + (y * header.width);
                    u8 index = 0;

                    int sprite_nb = y / 42;
                    int sprite_line = (y % 42) >> 1;
                    int vic_bank = ((y + 1) >> 1) & 0x1;
                    int sprite_offset = (sprite_line * 3) + (sprite_nb * 64) + (vic_bank * 0x140);
                    int sprite_byte_offset = (x % 24) >> 3;

                    u8 sprite_byte = data[sprite_offset + sprite_byte_offset];
                    int sprite_bit_pattern = (sprite_byte >> (6 - (x & 0x6))) & 0x3;

                    switch (sprite_bit_pattern)
                    {
                    case 1:
                        index = sprite_colors[y];
                        break;

                    case 2:
                        index = sprite_color1;
                        break;

                    case 3:
                        index = sprite_color2;
                        break;
                    }

                    if (index)
                    {
                        image[offset] = index;
                    }
                }
            }

            so->unlock();
            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Funpaint 2
    // ------------------------------------------------------------
    void depack_fun(u8* buffer, const u8* input, int scansize, int insize, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;
        const u8* input_end = input + insize;

        for (; buffer < buffer_end && input < input_end;)
        {
            u8 v = *input++;

            if (v == escape_char)
            {
                int n = *input++;
                if (n == 0)
                {
                    break;
                }
                std::memset(buffer, *input++, n);
                buffer += n;
            }
            else
            {
                *buffer++ = v;
            }
        }
    }

    const u8* read_header_fun(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (load_address == 0x3ff0 && size > 16)
        {
            u8 keyword[] = "FUNPAINT (MT) ";
            if (std::memcmp(keyword, xf, sizeof(keyword) - 1) == 0)
            {
                xf += 14;
                header.compressed = xf.read<u8>() ? true : false;
                header.escape_char = xf.read<u8>();

                if (!header.compressed)
                {
                    if (size == 33694)
                    {
                        header.width = 320;
                        header.height = 200;
                    }
                }
                else
                {
                    header.width = 320;
                    header.height = 200;
                }

                return xf;
            }
        }

        return NULL;
    }

    imageheader fun_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor* palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_fun(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* fun_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);
        u8* temp = NULL;

        header_generic header;
        data = read_header_fun(header, data, size);

        if (data)
        {
            const u8* buffer = data;

            if (header.compressed)
            {
                temp = new u8[33678];
                depack_fun(temp, data, 33678, size - 18, header.escape_char);
                buffer = temp;
            }

            u8* background = new u8[200];
            std::memcpy(background, buffer + 0x3f48, 100);
            std::memcpy(background + 100, buffer + 0x8328, 100);

            surface* so = multicolor_interlace_to_surface(buffer, header.width, header.height, 0x2000, 0x63e8, 0x0, 0x43e8, 0x4000, background, 0x0, 2, true, 2);

            delete [] background;

            if (temp)
            {
                delete [] temp;
            }

            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Gunpaint
    // ------------------------------------------------------------
    const u8* read_header_gun(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x4000, 33603, load_address, size))
        {
            u8 keyword[] = "GUNPAINT (JZ)   ";
            if (std::memcmp(keyword, data + 0x3ea, 16) == 0)
            {
                header.width = 320;
                header.height = 200;
                return xf;
            }
        }

        return NULL;
    }

    imageheader gun_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_gun(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* gun_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_gun(header, data, size);

        if (data)
        {
            u8 *background = new u8[200];
            std::memcpy(background, data + 0x3f4f, 177);
            std::memcpy(background + 177, data + 0x47e8, 20);
            background[197] = background[198] = background[199] = background[196];  // replicate the last color four times

            return multicolor_interlace_to_surface(data, header.width, header.height, 0x2000, 0x6400, 0x0, 0x4400, 0x4000, background, 0x0, 2, true, 2);
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter HCB-Editor v0.05
    // ------------------------------------------------------------
    imageheader hcb_header(stream* s)
    {
        return generic_header(s, 0x5000, 12148);
    }

    surface* hcb_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x5000, 12148);

        if (data)
        {
            const u8* bitmap_c64 = data + 0x1000;
            const u8* video_ram = data + 0x800;
            const u8* color_ram = data;
            const u8* background = data + 0x2f40;

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            u8* image = so->lock<u8>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int x_offset = x & 0x7;
                    int y_offset = (y >> 2) & 0x1;
                    int bitmap_offset = (x & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                    int screen_offset = bitmap_offset >> 3;
                    int offset = x + (y * header.width);

                    u8 byte = bitmap_c64[bitmap_offset];
                    int bit_pattern = (byte >> (6 - (x_offset & 0x6))) & 0x3;

                    u8 index = 0;
                    switch (bit_pattern)
                    {
                    case 0:
                        index = background[y >> 2] & 0xf;
                        break;

                    case 1:
                        // Emulate the FLI bug
                        if (x < 24)
                        {
                            index = 0xf;
                        }
                        else
                        {
                            index = (video_ram[screen_offset + (y_offset * 0x400)] >> 4);
                        }
                        break;

                    case 2:
                        // Emulate the FLI bug
                        if (x < 24)
                        {
                            index = 0xf;
                        }
                        else
                        {
                            index = (video_ram[screen_offset + (y_offset * 0x400)] & 0xf);
                        }
                        break;

                    case 3:
                        // Emulate the FLI bug
                        if (x < 24)
                        {
                            index = rand() & 0xf;
                        }
                        else
                        {
                            index = color_ram[screen_offset + (y_offset * 0x400)] & 0xf;
                        }
                        break;
                    }

                    image[offset] = index;
                }
            }

            so->unlock();
            return so;
        }

        return NULL;
    }
    
    // ------------------------------------------------------------
    // imagefilter Hires FLI Designer
    // ------------------------------------------------------------
    imageheader hfc_header(stream* s)
    {
        return generic_header(s, 0x4000, 16386);
    }

    surface* hfc_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x4000, 16386);

        if (data)
        {
            return hires_to_surface(data, header.width, header.height, 0x0, 0x2000, true, false, 0);
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Hires Manager
    // ------------------------------------------------------------
    void depack_him(u8* buffer, const u8* input, int scansize, int insize, u8 escape_char)
    {
        (void) scansize;

        const u8* in = input + insize - 1;
        const u8* in_end = input + 0x10 - 1;

        u8* out = buffer + 0x3ff2 - 1;
        const u8* out_end = buffer - 1;
        
        while (in > in_end && out > out_end)
        {
            unsigned char v = *in--;
            
            if (v == escape_char)
            {
                int n = *in--;
                v = *in--;

                if (out - n < out_end)
                {
                    FUSIONCORE_EXCEPTION("Hires Manager: unpacked size does not match file format.");
                }

                for (int i = 0; i < n; ++i)
                {
                    *out-- = v;
                }
            }
            else
            {
                int n = v - 1;

                if (out - n < out_end || in - n < in_end)
                {
                    FUSIONCORE_EXCEPTION("Hires Manager: unpacked size does not match file format.");
                }

                for (int i = 0; i < n; ++i)
                {
                    *out-- = *in--;
                }
            }
        }
    }

    const u8* read_header_him(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x4000, 16385, load_address, size))
        {
            if (*xf == 0xff)
            {
                header.width = 320;
                header.height = 192;
                header.compressed = false;
                return xf;
            }
        }
        else if (load_address == 0x4000)
        {
            header.width = 320;
            header.height = 192;
            header.compressed = true;
            return xf;
        }

        return NULL;
    }

    imageheader him_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;
        image_header.format = pixelformat::argb8888;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_him(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* him_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);
        u8* temp = NULL;

        header_generic header;
        data = read_header_him(header, data, size);

        if (data)
        {
            const u8* buffer = data;

            if (header.compressed)
            {
                temp = new u8[16383];
                depack_him(temp, data, 16383, size - 2, 0);
                buffer = temp;
            }

            surface* so = hires_to_surface(buffer, header.width, header.height, 0x140, 0x2028, true, false, 0);

            if (temp)
            {
                delete [] temp;
            }

            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Koala Painter
    // ------------------------------------------------------------
    const u8* read_header_koa(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x6000, 10003, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            return xf;
        }

        return NULL;
    }

    imageheader koa_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_koa(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* koa_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_koa(header, data, size);

        if (data)
        {
            return multicolor_to_surface(data, header.width, header.height, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, false, false);
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Paint Magic
    // ------------------------------------------------------------
    imageheader pmg_header(stream* s)
    {
        return generic_header(s, 0x3f8e, 9332);
    }

    surface* pmg_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x3f8e, 9332);

        if (data)
        {
            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            u8* image = so->lock<u8>();
            std::memset(image, 0, header.width * header.height);

            u8* color_ram = new u8[1000];
            std::memset(color_ram, *(data + 0x1fb5), 1000);

            convert_multicolor_bitmap(header.width, header.height, image, 
                                      data + 0x72, data + 0x2072, 
                                      color_ram, data + 0x1fb2, NULL,
                                      1, false);

            delete [] color_ram;

            so->unlock();
            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Pixel Perfect
    // ------------------------------------------------------------
    void depack_ppp(u8* buffer, const u8* input, int scansize, int insize, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;
        const u8* input_end = input + insize;

        for (; buffer < buffer_end && input < input_end;)
        {
            u8 v = *input++;

            if (v == escape_char)
            {
                int n = (*input++) + 1;
                std::memset(buffer, *input++, n);
                buffer += n;
            }
            else
            {
                *buffer++ = v;
            }
        }
    }

    const u8* read_header_pp(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x3c00, 33602, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            header.compressed = false;
            return xf;
        }
        else
        {
            if (load_address == 0x3bfc)
            {
                if (xf[0] == 0x10 && 
                    xf[1] == 0x10 && 
                    xf[2] == 0x10)
                {
                    xf += 3;

                    header.width = 320;
                    header.height = 200;
                    header.compressed = true;
                    header.escape_char = xf.read<u8>();
                    return xf;
                }
            }
        }

        return NULL;
    }

    imageheader pp_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;
        image_header.format = pixelformat::argb8888;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_pp(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* pp_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_pp(header, data, size);

        if (data)
        {
            const u8* buffer = data;
            u8* temp = NULL;

            if (header.compressed)
            {
                temp = new u8[33600];
                depack_ppp(temp, data, 33600, size - 6, header.escape_char);
                buffer = temp;
            }

            u8* background = new u8[200];
            std::memset(background, *(buffer + 0x437f), 200);

            surface* so = multicolor_interlace_to_surface(buffer, header.width, header.height, 0x2400, 0x6400, 0x400, 0x4400, 0x0, background, NULL, 1, true, 2);

            delete [] background;

            if (temp)
            {
                delete [] temp;
            }

            return so;

        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Run Paint
    // ------------------------------------------------------------
    imageheader rpm_header(stream* s)
    {
        return generic_header(s, 0x6000, 10006);
    }

    surface* rpm_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x6000, 10006, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, 1, false);
    }

    // ------------------------------------------------------------
    // imagefilter Saracen Paint
    // ------------------------------------------------------------
    imageheader sar_header(stream* s)
    {
        return generic_header(s, 0x7800, 10018);
    }

    surface* sar_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x7800, 10018, 0x400, 0x0, 0x2400, 0x3f0, 0x0, 1, false);
    }

    // ------------------------------------------------------------
    // imagefilter SHF-Editor v1.0
    // ------------------------------------------------------------
    const u8* read_header_shf(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x4000, 15874, load_address, size))
        {
            header.width = 96;
            header.height = 167;
            header.compressed = false;
            return xf;
        }
        else if (load_address == 0xa000)
        {
            header.width = 96;
            header.height = 167;
            header.compressed = true;
            header.escape_char = xf.read<u8>();
            return xf;
        }

        return NULL;
    }

    imageheader shf_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_shf(header, data, size);

        if (data)
        {
            image_header.width = 96;
            image_header.height = 167;
        }

        return image_header;
    }

    surface* shf_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);
        u8* temp = NULL;

        header_generic header;
        data = read_header_shf(header, data, size);

        if (data)
        {
            const u8* buffer = data;
/*
            if (header.compressed)
            {
                temp = new u8[15874];
                rle_ecb(temp, data, 15874, size - 3, header.escape_char);
                buffer = temp;

                FILE *f = fopen("unpacked.shf", "wb");
                fwrite(buffer, 1, 15874, f);
                fclose(f);
            }
*/
            const u8 *bitmap_c64 = buffer + 0x2000;
            const u8 *video_ram = buffer;
            u8 sprite_color1 = *(buffer + 0x3e8);
            u8 sprite_color2 = *(buffer + 0x3e9);

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            u8* image = so->lock<u8>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int offset = x + (y * header.width);
                    u8 index = 0;

                    // Hires data
                    int x_offset = (x + 112) & 0x7;
                    int y_offset = (y + 1) & 0x7;
                    int bitmap_offset = ((x + 112) & 0xfffffff8) + ((y + 1) & 0x7) + (((y + 1) >> 3) * (40 * 8));
                    int screen_offset = bitmap_offset >> 3;

                    u8 byte = bitmap_c64[bitmap_offset];
                    int bit_pattern = (byte >> (7 - x_offset)) & 0x1;

                    // 2 x overlay sprite data
                    // - Multiplexed every 21 scanlines
                    int sprite_nb = (x / 24);
                    int sprite_line = (y % 21);
                    int sprite_ram_bank = y & 0x7;

                    u8 sprite_pointer1 = buffer[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb];
                    /*
                    if (sprite_pointer1 > 15872)
                    {
                        FUSIONCORE_EXCEPTION("SHF: invalid sprite pointer.");
                    }
                    */

                    int sprite_byte_offset1 = (sprite_pointer1 * 64) + (sprite_line * 3) + (x % 24) / 8;
                    u8 sprite_byte1 = buffer[sprite_byte_offset1];
                    int sprite_bit_pattern1 = (sprite_byte1 >> (7 - (x & 0x7))) & 0x1;

                    u8 sprite_pointer2 = buffer[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb + 4];
                    /*
                    if (sprite_pointer2 > 15872)
                    {
                        FUSIONCORE_EXCEPTION("SHF: invalid sprite pointer.");
                    }
                    */

                    int sprite_byte_offset2 = (sprite_pointer2 * 64) + (sprite_line * 3) + (x % 24) / 8;
                    u8 sprite_byte2 = buffer[sprite_byte_offset2];
                    int sprite_bit_pattern2 = (sprite_byte2 >> (7 - (x & 0x7))) & 0x1;

                    switch (bit_pattern)
                    {
                    case 0:
                        index = (video_ram[screen_offset + (y_offset * 0x400)] & 0xf);
                        break;

                    case 1:
                        index = (video_ram[screen_offset + (y_offset * 0x400)] >> 4);
                        break;
                    }

                    if (sprite_bit_pattern2)
                    {
                        index = sprite_color2;
                    }
                    else if (sprite_bit_pattern1)
                    {
                        index = sprite_color1;
                    }

                    image[offset] = index;
                }
            }

            if (temp)
            {
                delete [] temp;
            }

            so->unlock();
            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter SHF-XL v1.0
    // ------------------------------------------------------------
    imageheader shfxl_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x4000, 15362);

        if (data)
        {
            image_header.width = 144;
            image_header.height = 168;
        }

        return image_header;
    }

    surface* shfxl_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x4000, 15362);

        if (data)
        {
            header.width = 144;
            header.height = 168;

            const u8 *bitmap_c64 = data + 0x2000;
            const u8 *video_ram = data;
            u8 sprite_color = *(data + 0x3e9);

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            u8* image = so->lock<u8>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int offset = x + (y * header.width);
                    u8 index = 0;

                    // Hires data
                    int x_offset = (x + 88) & 0x7;
                    int y_offset = y & 0x7;
                    int bitmap_offset = ((x + 88) & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                    int screen_offset = bitmap_offset >> 3;

                    u8 byte = bitmap_c64[bitmap_offset];
                    int bit_pattern = (byte >> (7 - x_offset)) & 0x1;

                    // Overlay sprite data
                    // - Multiplexed every 21 scanlines
                    int sprite_nb = (x / 24) + 1;
                    int sprite_line = (y % 21);
                    int sprite_ram_bank = (y + 7) & 0x7;
                    u8 sprite_pointer = data[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb];
                    int sprite_byte_offset = (sprite_pointer * 64) + (sprite_line * 3) + (x % 24) / 8;

                    if (sprite_byte_offset > 15360)
                    {
                        FUSIONCORE_EXCEPTION("SHF-XL: invalid sprite pointer.");
                    }

                    u8 sprite_byte = data[sprite_byte_offset];
                    int sprite_bit_pattern = (sprite_byte >> (7 - (x & 0x7))) & 0x1;

                    switch (bit_pattern)
                    {
                    case 0:
                        index = (video_ram[screen_offset + (y_offset * 0x400)] & 0xf);
                        break;

                    case 1:
                        index = (video_ram[screen_offset + (y_offset * 0x400)] >> 4);
                        break;
                    }

                    if (sprite_bit_pattern)
                    {
                        index = sprite_color;
                    }

                    image[offset] = index;
                }
            }

            so->unlock();
            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter True Paint
    // ------------------------------------------------------------
    void depack_mci(u8* buffer, const u8* input, int scansize, int insize)
    {
        const u8* in = input + insize - 1;
        const u8* in_end = input + 272;

        u8* out = buffer + scansize - 1;
        const u8* out_end = buffer - 1;
        
        while (in > in_end && out > out_end)
        {
            unsigned char v = *in--;

            if (v == *(input + 0x7f))
            {
                // Single escaped literal
                *out-- = *in--;
            }
            else if (v == *(input + 0x80))
            {
                // 3-character run
                v = *in--;

                if (out - 3 < out_end)
                {
                    FUSIONCORE_EXCEPTION("True Paint: unpacked size does not match file format.");
                }

                *out-- = v;
                *out-- = v;
                *out-- = v;
            }
            else if (v == *(input + 0x81))
            {
                // N-zero run
                int n = *in--;
                n += 2;

                if (out - n < out_end)
                {
                    FUSIONCORE_EXCEPTION("True Paint: unpacked size does not match file format.");
                }

                for (int i = 0; i < n; ++i)
                {
                    *out-- = 0;
                }
            }
            else if (v == *(input + 0x82))
            {
                // 3-zero run
                if (out - 3 < out_end)
                {
                    FUSIONCORE_EXCEPTION("True Paint: unpacked size does not match file format.");
                }

                *out-- = 0;
                *out-- = 0;
                *out-- = 0;
            }
            else if (v == *(input + 0x83))
            {
                // N-character run
                int n = *in--;
                n += 2;

                v = *in--;

                if (out - n < out_end)
                {
                    FUSIONCORE_EXCEPTION("True Paint: unpacked size does not match file format.");
                }

                for (int i = 0; i < n; ++i)
                {
                    *out-- = v;
                }
            }
            else if (v == *(input + 0x84))
            {
                // 1st 2-character run
                if (out - 2 < out_end)
                {
                    FUSIONCORE_EXCEPTION("True Paint: unpacked size does not match file format.");
                }

                *out-- = *(input + 0x88);
                *out-- = *(input + 0x88);
            }
            else if (v == *(input + 0x85))
            {
                // 2nd 2-character run
                if (out - 2 < out_end)
                {
                    FUSIONCORE_EXCEPTION("True Paint: unpacked size does not match file format.");
                }

                *out-- = *(input + 0x89);
                *out-- = *(input + 0x89);
            }
            else if (v == *(input + 0x86))
            {
                // 3rd 2-character run
                if (out - 2 < out_end)
                {
                    FUSIONCORE_EXCEPTION("True Paint: unpacked size does not match file format.");
                }

                *out-- = *(input + 0x8a);
                *out-- = *(input + 0x8a);
            }
            else
            {
                *out-- = v;
            }
        }
    }

    const u8* read_header_mci(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x9c00, 19434, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            header.compressed = false;
            return xf;
        }
        else if (load_address == 0x0801)
        {
            u8 keyword[] = "2059";
            if (std::memcmp(keyword, xf + 5, sizeof(keyword) - 1) == 0)
            {
                header.width = 320;
                header.height = 200;
                header.compressed = true;
                return xf;
            }
        }

        return NULL;
    }

    imageheader mci_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;
        image_header.format = pixelformat::argb8888;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_mci(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* mci_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);
        u8* temp = NULL;

        header_generic header;
        data = read_header_mci(header, data, size);

        if (data)
        {
            const u8* buffer = data;

            if (header.compressed)
            {
                temp = new u8[19432];
                depack_mci(temp, data, 19432, size - 2);
                buffer = temp;
            }

            u8* background = new u8[200];
            std::memset(background, *(buffer + 0x3e8), 200);

            surface* so = multicolor_interlace_to_surface(buffer, header.width, header.height, 0x400, 0x2400, 0x0, 0x4400, 0x4800, background, 0x0, 2, false, 2);

            delete [] background;

            if (temp)
            {
                delete [] temp;
            }

            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter UFLI-Editor v1.0 & v2.0
    // ------------------------------------------------------------
    const u8* read_header_ufli(header_generic& header, const u8* data, int size)
    {
        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (check_format(0x4000, 16194, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            header.compressed = false;
            return xf;
        }
        else if (load_address == 0x8000)
        {
            header.width = 320;
            header.height = 200;
            header.compressed = true;
            header.escape_char = xf.read<u8>();
            return xf;
        }

        return NULL;
    }

    imageheader ufli_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_ufli(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* ufli_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);
        u8* temp = NULL;

        header_generic header;
        data = read_header_ufli(header, data, size);

        if (data)
        {
            const u8* buffer = data;

            if (header.compressed)
            {
                temp = new u8[16192];
                rle_ecb(temp, data, 16192, size - 3, header.escape_char);
                buffer = temp;
            }

            const u8* bitmap_c64 = buffer + 0x2000;
            const u8* video_ram = buffer + 0x1000;
            const u8* sprite_colors = buffer + 0xff0;
            const u8 background_color = *(buffer + 0xff1);
            bool ufli2 = *(buffer + 0xfef) ? true : false;

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            u8* image = so->lock<u8>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int offset = x + (y * header.width);
                    u8 index = 0;

                    if (x < 24 || x >= 312)
                    {
                        index = background_color & 0xf;
                    }
                    else
                    {
                        // Hires data
                        int x_offset = x & 0x7;
                        int y_offset = y & 0x7;
                        int bitmap_offset = (x & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                        int screen_offset = bitmap_offset >> 3;

                        u8 byte = bitmap_c64[bitmap_offset];
                        int bit_pattern = (byte >> (7 - x_offset)) & 0x1;

                        // Underlay sprite data
                        // - X- and Y-expanded
                        // - Multiplexed every 40 scanlines
                        // - First sprites positioned on Y=-1
                        // - Switching VIC bank every two scanlines
                        int sprite_x_offset = x - 24;
                        int sprite_column = (sprite_x_offset / 48);
                        int sprite_nb = sprite_column + (y / 40) * 6;
                        int sprite_line = ((y + 1) % 42) >> 1;
                        int vic_bank = (y >> 1) & 0x1;
                        int sprite_offset = (sprite_line * 3) + ((sprite_nb % 6) * 64) + (vic_bank * 0x180) + (sprite_nb / 6) * 0x300;
                        int sprite_byte_offset = (sprite_x_offset % 48) / 16;

                        u8 sprite_byte = buffer[sprite_offset + sprite_byte_offset];
                        int sprite_bit_pattern = (sprite_byte >> (7 - ((sprite_x_offset >> 1) & 0x7))) & 0x1;

                        switch (bit_pattern)
                        {
                        case 0:
                            if (sprite_bit_pattern)
                            {
                                if (ufli2)
                                {
                                    index = sprite_colors[sprite_column + 2] & 0xf;
                                }
                                else
                                {
                                    index = sprite_colors[0];
                                }
                            }
                            else
                            {
                                index = (video_ram[screen_offset + ((y_offset >> 1) * 0x400)] & 0xf);
                            }
                            break;

                        case 1:
                            index = (video_ram[screen_offset + ((y_offset >> 1) * 0x400)] >> 4);
                            break;
                        }
                    }

                    image[offset] = index;
                }
            }

            if (temp)
            {
                delete [] temp;
            }

            so->unlock();
            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter UIFLI Editor v1.0
    // ------------------------------------------------------------
    void depack_uifli(u8* buffer, const u8* input, int scansize, int insize, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;
        const u8* input_end = input + insize;

        for (; buffer < buffer_end && input < input_end;)
        {
            u8 look_ahead1 = *(input + 2);
            u8 look_ahead2 = *(input + 3);
            u8 look_ahead3 = *(input + 4);
            u8 v = *input++;

            if (look_ahead1 == escape_char && 
                look_ahead2 != escape_char && 
                look_ahead3 != escape_char)
            {
                int n = *input;
                if (n == 0)
                {
                    n = 256;
                }
                std::memset(buffer, v, n);
                buffer += n;

                input += 2;
            }
            else
            {
                *buffer++ = v;
            }
        }
    }

    const u8* read_header_uifli(header_generic& header, const u8* data, int size)
    {
        (void) size;

        infilter xf(data);
        u16 load_address = xf.read<u16>();

        if (load_address == 0x4000)
        {
            header.width = 320;
            header.height = 200;
            header.compressed = true;
            header.escape_char = xf.read<u8>();
            return xf;
        }

        return NULL;
    }

    imageheader uifli_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;
        image_header.format =  pixelformat::argb8888;

        int size = int(s->size());
        const u8* data = s->read(size);

        header_generic header;
        data = read_header_ufli(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* uifli_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const u8* data = s->read(size);
        u8* temp = NULL;

        header_generic header;
        data = read_header_uifli(header, data, size);

        if (data)
        {
            const u8* buffer = data;

            if (header.compressed)
            {
                temp = new u8[32897];
                depack_uifli(temp, data, 32897, size - 3, header.escape_char);
                buffer = temp;
            }

            const u8* bitmap_c64[2] = { buffer + 0x2000, buffer + 0x6000 };
            const u8* video_ram[2] = { buffer, buffer + 0x4000 };
            u8 sprite_color[2] = { *(buffer + 0xff0), *(buffer + 0x4ff0) };
            const u8* sprites[2] = { buffer + 0x1000, buffer + 0x5000 };

            surface* so = bitmap::create(header.width, header.height, pixelformat::argb8888);
            ucolor* image = so->lock<ucolor>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int offset = x + (y * header.width);
                    u8 index[2] = { 0, 0 };

                    if (x < 24)
                    {
                        index[0] = sprite_color[0];
                        index[1] = sprite_color[1];
                    }
                    else
                    {
                        // Hires data
                        int x_offset = x & 0x7;
                        int y_offset = y & 0x7;
                        int bitmap_offset = (x & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                        int screen_offset = bitmap_offset >> 3;

                        u8 byte[2];
                        int bit_pattern[2];
                        u8 sprite_byte[2];
                        int sprite_bit_pattern[2];

                        byte[0] = bitmap_c64[0][bitmap_offset];
                        byte[1] = bitmap_c64[1][bitmap_offset];

                        bit_pattern[0] = (byte[0] >> (7 - x_offset)) & 0x1;
                        bit_pattern[1] = (byte[1] >> (7 - x_offset)) & 0x1;

                        // Underlay sprite data
                        // - X- and Y-expanded
                        // - Multiplexed every 40 scanlines
                        // - First sprites positioned on Y=-1
                        // - Switching VIC bank every two scanlines
                        int sprite_x_offset = x - 24;
                        int sprite_nb = (sprite_x_offset / 48) + (y / 40) * 6;
                        int sprite_line = ((y + 1) % 42) >> 1;
                        int vic_bank = (y >> 1) & 0x1;
                        int sprite_offset = (sprite_line * 3) + ((sprite_nb % 6) * 64) + (vic_bank * 0x180) + (sprite_nb / 6) * 0x300;
                        int sprite_byte_offset = (sprite_x_offset % 48) / 16;

                        sprite_byte[0] = sprites[0][sprite_offset + sprite_byte_offset];
                        sprite_byte[1] = sprites[1][sprite_offset + sprite_byte_offset];

                        sprite_bit_pattern[0] = (sprite_byte[0] >> (7 - ((sprite_x_offset >> 1) & 0x7))) & 0x1;
                        sprite_bit_pattern[1] = (sprite_byte[1] >> (7 - ((sprite_x_offset >> 1) & 0x7))) & 0x1;

                        switch (bit_pattern[0])
                        {
                        case 0:
                            if (sprite_bit_pattern[0])
                            {
                                index[0] = sprite_color[0];
                            }
                            else
                            {
                                index[0] = (video_ram[0][screen_offset + ((y_offset >> 1) * 0x400)] & 0xf);
                            }
                            break;

                        case 1:
                            index[0] = (video_ram[0][screen_offset + ((y_offset >> 1) * 0x400)] >> 4);
                            break;
                        }

                        switch (bit_pattern[1])
                        {
                        case 0:
                            if (sprite_bit_pattern[1])
                            {
                                index[1] = sprite_color[1];
                            }
                            else
                            {
                                index[1] = (video_ram[1][screen_offset + ((y_offset >> 1) * 0x400)] & 0xf);
                            }
                            break;

                        case 1:
                            index[1] = (video_ram[1][screen_offset + ((y_offset >> 1) * 0x400)] >> 4);
                            break;
                        }
                    }

                    image[offset].a = 0xff;
                    image[offset].r = (c64_palette[index[0]].r >> 1) + (c64_palette[index[1]].r >> 1);
                    image[offset].g = (c64_palette[index[0]].g >> 1) + (c64_palette[index[1]].g >> 1);
                    image[offset].b = (c64_palette[index[0]].b >> 1) + (c64_palette[index[1]].b >> 1);
                }
            }

            if (temp)
            {
                delete [] temp;
            }

            so->unlock();
            return so;
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Vidcom 64
    // ------------------------------------------------------------
    imageheader vid_header(stream* s)
    {
        return generic_header(s, 0x5800, 10050);
    }

    surface* vid_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x5800, 10050, 0x800, 0x400, 0x0, 0x7e9, 0x0, 1, false);
    }
}

#endif

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        Memory m_memory;
        ImageHeader m_header;

        Interface(Memory memory)
            : m_memory(memory)
        {
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        void decode(Surface& dest, Palette* palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(palette);
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            if (dest.format == m_header.format && dest.width >= m_header.width && dest.height >= m_header.height)
            {
                decodeImage(dest);
            }
            else
            {
                Bitmap temp(m_header.width, m_header.height, m_header.format);
                decodeImage(temp);
                dest.blit(0, 0, temp);
            }
        }

        virtual void decodeImage(Surface& dest) = 0;
    };

    // ------------------------------------------------------------
    // ImageDecoder: MPIC (Advanced Art Studio)
    // ------------------------------------------------------------

    struct InterfaceMPIC : Interface
    {
        header_generic m_generic_header;
        u8* m_data;

        InterfaceMPIC(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_generic_header.parse(memory.address, memory.size, 0x2000, 10018);
            if (m_data)
            {
                m_header.width  = m_generic_header.width;
                m_header.height = m_generic_header.height;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x0, 0x1f40, 0x2338, 0x2329, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceMPIC(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceMPIC(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: AFL (AFLI-editor v2.0)
    // ------------------------------------------------------------

    struct InterfaceAFL : Interface
    {
        header_generic m_generic_header;
        u8* m_data;

        InterfaceAFL(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_generic_header.parse(memory.address, memory.size, 0x4000, 16385);
            if (m_data)
            {
                m_header.width  = m_generic_header.width;
                m_header.height = m_generic_header.height;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            hires_to_surface(s, m_data, m_header.width, m_header.height, 0x2000, 0x0, true, false, 0);
        }
    };

    ImageDecoderInterface* createInterfaceAFL(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceAFL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: AMI (Amica Painter)
    // ------------------------------------------------------------

    struct InterfaceAMI : Interface
    {
        header_generic m_generic_header;
        u8* m_data;

        InterfaceAMI(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            // read header
            u8* end = memory.address + memory.size;

            if (end[-1] == 0x0 && end[-2] == 0xc2)
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);

                m_generic_header.compressed = true;
                m_generic_header.escape_char = 0xc2;

                m_data = memory.address + 2;
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            u8* buffer = m_data;
            u8* temp = nullptr;

            if (m_generic_header.compressed)
            {
                temp = new u8[10513];
                rle_ecb(temp, m_data, 10513, int(m_memory.size - 3), m_generic_header.escape_char);
                buffer = temp;
            }

            multicolor_to_surface(s, buffer, m_header.width, m_header.height, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, false, false);
            delete [] temp;
        }
    };

    ImageDecoderInterface* createInterfaceAMI(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceAMI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: ART
    // ------------------------------------------------------------
#if 0

    struct InterfaceART : Interface
    {
        u8* m_data;

        InterfaceART(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceART(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceART(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: A64
    // ------------------------------------------------------------

    struct InterfaceA64 : Interface
    {
        u8* m_data;

        InterfaceA64(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceA64(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceA64(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: BLP
    // ------------------------------------------------------------

    struct InterfaceBLP : Interface
    {
        u8* m_data;

        InterfaceBLP(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceBLP(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceBLP(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: CDU
    // ------------------------------------------------------------

    struct InterfaceCDU : Interface
    {
        u8* m_data;

        InterfaceCDU(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceCDU(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceCDU(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: DOL
    // ------------------------------------------------------------

    struct InterfaceDOL : Interface
    {
        u8* m_data;

        InterfaceDOL(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceDOL(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceDOL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: DD
    // ------------------------------------------------------------

    struct InterfaceDD : Interface
    {
        u8* m_data;

        InterfaceDD(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceDD(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceDD(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: DRL
    // ------------------------------------------------------------

    struct InterfaceDRL : Interface
    {
        u8* m_data;

        InterfaceDRL(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceDRL(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceDRL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: DRZ
    // ------------------------------------------------------------

    struct InterfaceDRZ : Interface
    {
        u8* m_data;

        InterfaceDRZ(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceDRZ(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceDRZ(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: ECI
    // ------------------------------------------------------------

    struct InterfaceECI : Interface
    {
        u8* m_data;

        InterfaceECI(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceECI(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceECI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: FPT
    // ------------------------------------------------------------

    struct InterfaceFPT : Interface
    {
        u8* m_data;

        InterfaceFPT(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceFPT(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceFPT(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: FD2
    // ------------------------------------------------------------

    struct InterfaceFD2 : Interface
    {
        u8* m_data;

        InterfaceFD2(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceFD2(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceFD2(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: FPR
    // ------------------------------------------------------------

    struct InterfaceFPR : Interface
    {
        u8* m_data;

        InterfaceFPR(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceFPR(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceFPR(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: FUN
    // ------------------------------------------------------------

    struct InterfaceFUN : Interface
    {
        u8* m_data;

        InterfaceFUN(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceFUN(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceFUN(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: GUN
    // ------------------------------------------------------------

    struct InterfaceGUN : Interface
    {
        u8* m_data;

        InterfaceGUN(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceGUN(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceGUN(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: HCB
    // ------------------------------------------------------------

    struct InterfaceHCB : Interface
    {
        u8* m_data;

        InterfaceHCB(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceHCB(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceHCB(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: HFC
    // ------------------------------------------------------------

    struct InterfaceHFC : Interface
    {
        u8* m_data;

        InterfaceHFC(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceHFC(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceHFC(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: HIM
    // ------------------------------------------------------------

    struct InterfaceHIM : Interface
    {
        u8* m_data;

        InterfaceHIM(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceHIM(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceHIM(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: KOA
    // ------------------------------------------------------------

    struct InterfaceKOA : Interface
    {
        u8* m_data;

        InterfaceKOA(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceKOA(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceKOA(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: PMG
    // ------------------------------------------------------------

    struct InterfacePMG : Interface
    {
        u8* m_data;

        InterfacePMG(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfacePMG(Memory memory)
    {
        ImageDecoderInterface* x = new InterfacePMG(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: PP
    // ------------------------------------------------------------

    struct InterfacePP : Interface
    {
        u8* m_data;

        InterfacePP(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfacePP(Memory memory)
    {
        ImageDecoderInterface* x = new InterfacePP(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: RPM
    // ------------------------------------------------------------

    struct InterfaceRPM : Interface
    {
        u8* m_data;

        InterfaceRPM(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceRPM(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceRPM(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: SAR
    // ------------------------------------------------------------

    struct InterfaceSAR : Interface
    {
        u8* m_data;

        InterfaceSAR(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceSAR(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceSAR(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: SHF
    // ------------------------------------------------------------

    struct InterfaceSHF : Interface
    {
        u8* m_data;

        InterfaceSHF(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceSHF(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceSHF(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: SHFXL
    // ------------------------------------------------------------

    struct InterfaceSHFXL : Interface
    {
        u8* m_data;

        InterfaceSHFXL(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceSHFXL(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceSHFXL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: MCI
    // ------------------------------------------------------------

    struct InterfaceMCI : Interface
    {
        u8* m_data;

        InterfaceMCI(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceMCI(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceMCI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: UFLI
    // ------------------------------------------------------------

    struct InterfaceUFLI : Interface
    {
        u8* m_data;

        InterfaceUFLI(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceUFLI(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceUFLI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: UIFLI
    // ------------------------------------------------------------

    struct InterfaceUIFLI : Interface
    {
        u8* m_data;

        InterfaceUIFLI(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceUIFLI(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceUIFLI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: VID
    // ------------------------------------------------------------

    struct InterfaceVID : Interface
    {
        u8* m_data;

        InterfaceVID(Memory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (m_data)
            {
                // TODO
                m_header.width  = 0;
                m_header.height = 0;
                m_header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            }
        }

        void decodeImage(Surface& s) override
        {
            if (!m_data)
                return;

            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceVID(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceVID(memory);
        return x;
    }
#endif
} // namespace

namespace mango
{

    void registerImageDecoderC64()
    {
        // Advanced Art Studio
        registerImageDecoder(createInterfaceMPIC, "mpic"); // TODO: test

        // AFLI-editor v2.0
        registerImageDecoder(createInterfaceAFL, "afl");
        registerImageDecoder(createInterfaceAFL, "afli");

        // Amica Paint
        registerImageDecoder(createInterfaceAMI, "ami");

#if 0
        // Art Studio
        registerImageDecoder(createInterfaceART, "art");
        registerImageDecoder(createInterfaceART, "ocp");

        // Artist 64
        registerImageDecoder(createInterfaceA64, "a64");

        // Blazing Paddles
        registerImageDecoder(createInterfaceBLP, "blp");
        registerImageDecoder(createInterfaceBLP, "bpi");
        registerImageDecoder(createInterfaceBLP, "pi");

        // CDU-Paint
        registerImageDecoder(createInterfaceCDU, "cdu");

        // Dolphin Ed
        registerImageDecoder(createInterfaceDOL, "dol");

        // Doodle
        registerImageDecoder(createInterfaceDD, "dd");
        //registerImageDecoder(createInterfaceDD, "ddl");
        //registerImageDecoder(createInterfaceDD, "jj");

        // Drazlace
        registerImageDecoder(createInterfaceDRL, "drl");
        registerImageDecoder(createInterfaceDRL, "dlp");

        // Drazpaint
        registerImageDecoder(createInterfaceDRZ, "drz");
        registerImageDecoder(createInterfaceDRZ, "dp64");
        registerImageDecoder(createInterfaceDRZ, "drp");
        registerImageDecoder(createInterfaceDRZ, "dp");

        // ECI Graphic Editor v1.0
        registerImageDecoder(createInterfaceECI, "eci");

        // Face Painter
        registerImageDecoder(createInterfaceFPT, "fpt");
        registerImageDecoder(createInterfaceFPT, "fcp");

        // FLI Designer 1.1 & 2.0 (FBI Crew)
        registerImageDecoder(createInterfaceFD2, "fd2");

        // FLI-Profi
        registerImageDecoder(createInterfaceFPR, "fpr");

        // Funpaint 2
        registerImageDecoder(createInterfaceFUN, "fun");
        registerImageDecoder(createInterfaceFUN, "fp2");

        // Gunpaint
        registerImageDecoder(createInterfaceGUN, "gun");
        registerImageDecoder(createInterfaceGUN, "ifl");

        // HCB-Editor v0.05
        registerImageDecoder(createInterfaceHCB, "hcb");

        // Hires FLI Designer
        registerImageDecoder(createInterfaceHFC, "hfc");

        // Hires Manager
        registerImageDecoder(createInterfaceHIM, "him");

        // Koala Painter II
        registerImageDecoder(createInterfaceKOA, "koa");
        registerImageDecoder(createInterfaceKOA, "kla");

        // Paint Magic
        registerImageDecoder(createInterfacePMG, "pmg");

        // Pixel Perfect
        registerImageDecoder(createInterfacePP, "pp");
        registerImageDecoder(createInterfacePP, "ppp");

        // Run paint
        registerImageDecoder(createInterfaceRPM, "rpm");

        // Saracen Paint
        registerImageDecoder(createInterfaceSAR, "sar");

        // SHF-Editor v1.0
        registerImageDecoder(createInterfaceSHF, "unp");
        registerImageDecoder(createInterfaceSHF, "shfli");

        // SHF-XL v1.0
        registerImageDecoder(createInterfaceSHFXL, "shx");
        registerImageDecoder(createInterfaceSHFXL, "shfxl");

        // True Paint
        registerImageDecoder(createInterfaceMCI, "mci");
        registerImageDecoder(createInterfaceMCI, "mcp");

        // UFLI-Editor v1.0 & v2.0
        registerImageDecoder(createInterfaceUFLI, "ufup");
        registerImageDecoder(createInterfaceUFLI, "ufli");

        // UIFLI Editor v1.0
        registerImageDecoder(createInterfaceUIFLI, "uifli");

        // Vidcom 64
        registerImageDecoder(createInterfaceVID, "vid");
#endif
    }

} // namespace mango
