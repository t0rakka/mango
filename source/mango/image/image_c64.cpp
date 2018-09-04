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

#define ID "ImageStream.C64: "

#if 0

namespace
{
    using namespace fusion::core;

    ucolor c64_palette[16] =
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

    typedef inputfilter<little_endian>  infilter;

    // ------------------------------------------------------------
    // RLE depacker (ESCAPE, COUNT, BYTE)
    // ------------------------------------------------------------
    void rle_ecb(uint8* buffer, const uint8* input, int scansize, int insize, uint8 escape_char)
    {
        uint8* buffer_end = buffer + scansize;
        const uint8* input_end = input + insize;

        for (; buffer < buffer_end && input < input_end;)
        {
            uint8 v = *input++;

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

    // ------------------------------------------------------------
    // Bitmap conversion functions
    // ------------------------------------------------------------
    void convert_multicolor_bitmap(int width, int height, uint8* image, 
                                   const uint8* bitmap_c64, const uint8* video_ram, const uint8* color_ram, 
                                   const uint8* background, 
                                   const uint8* opcode_colors,
                                   int background_mode, bool fli)
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

                uint8 byte = bitmap_c64[bitmap_offset];
                int bit_pattern = (byte >> (6 - (x_offset & 0x6))) & 0x3;

                uint8 index = 0;
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

/*
    void convert_multicolor_sprite(int width, int height, uint8* image, int sprite_x, int sprite_y, const uint8* sprite_data,
                                   uint8 sprite_color1, uint8 sprite_color2, const uint8 *sprite_color, bool fli)
    {
        int i, j;

        for (i = 0; i < 63; ++i)
        {
            uint8 data = sprite_data[i];

            int x = (i % 3) * 8;
            int y = (i / 3);

            for (j = 0; j < 4; ++j)
            {
                int bit_pattern = (data >> (6 - (j * 2))) & 0x3;
                int x_offset = sprite_x + (x + (j * 2));
                int y_offset = (sprite_y + y);

                if (x >= 0 && x < width && y >= 0 && y < height)
                {
                    int offset = x_offset + (y_offset * width);

                    uint8 index = 0;
                    switch (bit_pattern)
                    {
                    case 1:
                        index = sprite_color1;
                        break;

                    case 2:
                        if (fli)
                        {
                            index = sprite_color[y];
                        }
                        else
                        {
                            index = *sprite_color;
                        }
                        break;
                        break;

                    case 3:
                        index = sprite_color2;
                    }

                    if (index != 0)
                    {
                        image[offset + 0] = index;
                        image[offset + 1] = index;
                    }
                }
            }
        }
    }
*/

    surface* multicolor_to_surface(const uint8 *data, int width, int height, 
                                   uint32 bitmap_offset, uint32 video_ram_offset, uint32 color_ram_offset, 
                                   uint32 background_offset, uint32 opcode_colors_offset, 
                                   int background_mode, bool fli)
    {
        ucolor palette[256];
        std::memset(palette, 0, 256 * sizeof(ucolor));
        std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

        surface* so = bitmap::create(width, height, pixelformat(palette));
        uint8* image = so->lock<uint8>();
        std::memset(image, 0, width * height);

        convert_multicolor_bitmap(width, height, image, 
                                  data + bitmap_offset, data + video_ram_offset, 
                                  data + color_ram_offset, data + background_offset, data + opcode_colors_offset,
                                  background_mode, fli);

        so->unlock();
        return so;
    }

    surface* multicolor_interlace_to_surface(const uint8 *data, int width, int height, uint32 
                                             bitmap_offset_1, uint32 bitmap_offset_2, 
                                             uint32 video_ram_offset_1, uint32 video_ram_offset_2, 
                                             uint32 color_ram_offset, uint8 *background_colors,
                                             uint8 *opcode_colors,
                                             int background_mode,
                                             bool fli,
                                             int mode)
    {
        int x, y;

        ucolor palette[256];
        std::memset(palette, 0, 256 * sizeof(ucolor));
        std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

        uint8 *bitmap1 = new uint8[width * height];
        uint8 *bitmap2 = new uint8[width * height];

        convert_multicolor_bitmap(width, height, bitmap1, data + bitmap_offset_1, data + video_ram_offset_1, data + color_ram_offset, background_colors, opcode_colors, background_mode, fli);
        convert_multicolor_bitmap(width, height, bitmap2, data + bitmap_offset_2, data + video_ram_offset_2, data + color_ram_offset, background_colors, opcode_colors, background_mode, fli);

        surface* so = bitmap::create(width, height, pixelformat::argb8888);
        ucolor* image = so->lock<ucolor>();
        std::memset(image, 0, width * height * sizeof(ucolor));

        for (y = 0; y < height; ++y)
        {
            for (x = 0; x < width; ++x)
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

        so->unlock();
        
        delete[] bitmap2;
        delete[] bitmap1;
        
        return so;
    }

    void convert_hires_bitmap(int width, int height, 
                              uint8* image, const uint8* bitmap_c64, const uint8* video_ram, 
                              bool fli,
                              bool show_fli_bug, 
                              uint8 fli_bug_color)
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

                uint8 byte = bitmap_c64[bitmap_offset];
                int bit_pattern = (byte >> (7 - x_offset)) & 0x1;

                uint8 index = 0;
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
/*
    void convert_hires_sprite(int width, int height, uint8* image, int sprite_x, int sprite_y, const uint8* sprite_data,
                              bool x_stretch, bool y_stretch, uint8 sprite_color)
    {
        int i, j;

        for (i = 0; i < 63; ++i)
        {
            uint8 data = sprite_data[i];

            for (j = 0; j < 8; ++j)
            {
                int bit_pattern = (data >> (7 - j)) & 0x1;

                int x = (i % 3) * 8 + j;
                int y = (i / 3);

                x *= (x_stretch ? 2 : 1);
                y *= (y_stretch ? 2 : 1);

                int x_offset = sprite_x + x;
                int y_offset = sprite_y + y;

                if (x_offset >= 0 && x_offset < width && y_offset >= 0 && y_offset < height)
                {
                    int offset = x_offset + (y_offset * width);

                    uint8 index = 0;
                    switch (bit_pattern)
                    {
                    case 1:
                        index = sprite_color;
                        break;
                    }

                    if (index != 0)
                    {
                        if (!x_stretch && !y_stretch)
                        {
                            image[offset] = index;
                        }
                        else if (x_stretch && !y_stretch)
                        {
                            image[offset + 0] = index;
                            image[offset + 1] = index;
                        }
                        else if (!x_stretch && y_stretch)
                        {
                            image[offset + 0] = index;
                            image[offset + width] = index;
                        }
                        else
                        {
                            image[offset + 0] = index;
                            image[offset + 1] = index;
                            image[offset + width + 0] = index;
                            image[offset + width + 1] = index;
                        }
                    }
                }
            }
        }
    }
*/
    surface* hires_to_surface(const uint8* data, int width, int height, 
                              uint32 bitmap_offset, uint32 video_ram_offset, 
                              bool fli = false,
                              bool show_fli_bug = false,
                              uint8 fli_bug_color = 0)
    {
        ucolor palette[256];
        std::memset(palette, 0, 256 * sizeof(ucolor));
        std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

        surface* so = bitmap::create(width, height, pixelformat(palette));
        uint8* image = so->lock<uint8>();

        std::memset(image, 0, width * height);
        convert_hires_bitmap(width, height, image, data + bitmap_offset, data + video_ram_offset, fli, show_fli_bug, fli_bug_color);

        so->unlock();
        return so;
    }

    surface* hires_interlace_to_surface(const uint8* data, int width, int height, uint32 
                                        bitmap_offset_1, uint32 bitmap_offset_2, 
                                        uint32 video_ram_offset_1, uint32 video_ram_offset_2, 
                                        bool fli = false,
                                        bool show_fli_bug = false,
                                        uint8 fli_bug_color = 0)
    {
        int x, y;

        ucolor palette[256];
        std::memset(palette, 0, 256 * sizeof(ucolor));
        std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

        uint8* bitmap1 = new uint8[width * height];
        uint8* bitmap2 = new uint8[width * height];

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

    bool check_format(uint16 format_address, int format_size, uint16 load_address, int size)
    {
        if (load_address == format_address && size == format_size)
        {
            return true;
        }

        return false;
    }

    // ------------------------------------------------------------
    // imagefilter generic
    // ------------------------------------------------------------
    struct header_generic
    {
        int width;
        int height;
        bool compressed;
        uint8 escape_char;

        header_generic()
        : width(0), height(0), compressed(false), escape_char(0)
        {
        }

        ~header_generic()
        {
        }
    };

    const uint8* read_header_generic(header_generic& header, const uint8* data, int size, uint16 format_address, int format_size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

        if (check_format(format_address, format_size, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            return xf;
        }

        return NULL;
    }

    imageheader generic_header(stream* s, uint16 format_address, uint32 format_size)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, format_address, format_size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* generic_multicolor_load(stream* s, uint16 format_address, int format_size, 
                                     uint32 bitmap_offset, uint32 video_ram_offset, 
                                     uint32 color_ram_offset, uint32 background_offset, uint32 opcode_colors_offset,
                                     int background_mode, bool fli)
    {
        int size = int(s->size());
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, format_address, format_size);

        if (data)
        {
            return multicolor_to_surface(data, header.width, header.height, 
                                         bitmap_offset, video_ram_offset, color_ram_offset, background_offset, opcode_colors_offset,
                                         background_mode, fli);
        }

        return NULL;
    }

    surface* generic_hires_load(stream* s, uint16 format_address, int format_size, uint32 bitmap_offset, uint32 video_ram_offset, bool fli)
    {
        int size = int(s->size());
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, format_address, format_size);

        if (data)
        {
            return hires_to_surface(data, header.width, header.height, bitmap_offset, video_ram_offset, fli);
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Advanced Art Studio
    // ------------------------------------------------------------
    imageheader mpic_header(stream* s)
    {
        return generic_header(s, 0x2000, 10018);
    }

    surface* mpic_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;
        return generic_multicolor_load(s, 0x2000, 10018, 0x0, 0x1f40, 0x2338, 0x2329, 0x0, 1, false);
    }

    // ------------------------------------------------------------
    // imagefilter AFLI-editor v2.0
    // ------------------------------------------------------------
    imageheader afl_header(stream* s)
    {
        return generic_header(s, 0x4000, 16385);
    }

    surface* afl_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x4000, 16385);

        if (data)
        {
            return hires_to_surface(data, header.width, header.height, 0x2000, 0x0, true, false, 0);
        }

        return NULL;
    }

    // ------------------------------------------------------------
    // imagefilter Amica Painter
    // ------------------------------------------------------------
    const uint8* read_header_ami(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        xf.read<uint16>();  // skip load address

        if (data[size - 1] == 0x0 && 
            data[size - 2] == 0xc2)
        {
            header.width = 320;
            header.height = 200;
            header.compressed = true;
            header.escape_char = 0xc2;
            return xf;
        }

        return NULL;
    }

    imageheader ami_header(stream* s)
    {
        imageheader image_header;
        image_header.width = 0;
        image_header.height = 0;

        ucolor *palette = NULL;
        image_header.format = pixelformat(palette);

        int size = int(s->size());
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_ami(header, data, size);

        if (data)
        {
            image_header.width = header.width;
            image_header.height = header.height;
        }

        return image_header;
    }

    surface* ami_load(stream* s, bool thumbnail)
    {
        (void) thumbnail;

        int size = int(s->size());
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_ami(header, data, size);

        if (data)
        {
            const uint8* buffer = data;
            uint8* temp = NULL;

            if (header.compressed)
            {
                temp = new uint8[10513];
                rle_ecb(temp, data, 10513, size - 3, header.escape_char);
                buffer = temp;
            }

            surface* so = multicolor_to_surface(buffer, header.width, header.height, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, false, false);

            if (temp)
            {
                delete [] temp;
            }

            return so;
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
    const uint8* read_header_drl(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

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
                uint8 keyword[] = "DRAZLACE! 1.0";
                if (std::memcmp(keyword, xf, sizeof(keyword) - 1) == 0)
                {
                    xf += sizeof(keyword) - 1;

                    header.width = 320;
                    header.height = 200;
                    header.compressed = true;
                    header.escape_char = xf.read<uint8>();

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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_drl(header, data, size);

        if (data)
        {
            const uint8* buffer = data;
            uint8* temp = NULL;

            if (header.compressed)
            {
                temp = new uint8[18240];
                rle_ecb(temp, data, 18240, size - 3, header.escape_char);
                buffer = temp;
            }

            uint8* background = new uint8[200];
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
    const uint8* read_header_drz(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

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
                uint8 keyword[] = "DRAZPAINT 2.0";
                if (std::memcmp(keyword, xf, sizeof(keyword) - 1) == 0)
                {
                    xf += sizeof(keyword) - 1;

                    header.width = 320;
                    header.height = 200;
                    header.compressed = true;
                    header.escape_char = xf.read<uint8>();

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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_drz(header, data, size);

        if (data)
        {
            const uint8* buffer = data;
            uint8* temp = NULL;

            if (header.compressed)
            {
                temp = new uint8[10049];
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
    const uint8* read_header_eci(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

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
                header.escape_char = xf.read<uint8>();
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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_eci(header, data, size);

        if (data)
        {
            const uint8* buffer = data;
            uint8* temp = NULL;

            if (header.compressed)
            {
                temp = new uint8[32768];
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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x3780, 18370);

        if (data)
        {
            uint8 sprite_color1 = data[0x448];
            uint8 sprite_color2 = data[0x449];
            const uint8 *sprite_colors = data + 0x280;

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            uint8* image = so->lock<uint8>();
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
                    uint8 index = 0;

                    int sprite_nb = y / 42;
                    int sprite_line = (y % 42) >> 1;
                    int vic_bank = ((y + 1) >> 1) & 0x1;
                    int sprite_offset = (sprite_line * 3) + (sprite_nb * 64) + (vic_bank * 0x140);
                    int sprite_byte_offset = (x % 24) >> 3;

                    uint8 sprite_byte = data[sprite_offset + sprite_byte_offset];
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
    void depack_fun(uint8* buffer, const uint8* input, int scansize, int insize, uint8 escape_char)
    {
        uint8* buffer_end = buffer + scansize;
        const uint8* input_end = input + insize;

        for (; buffer < buffer_end && input < input_end;)
        {
            uint8 v = *input++;

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

    const uint8* read_header_fun(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

        if (load_address == 0x3ff0 && size > 16)
        {
            uint8 keyword[] = "FUNPAINT (MT) ";
            if (std::memcmp(keyword, xf, sizeof(keyword) - 1) == 0)
            {
                xf += 14;
                header.compressed = xf.read<uint8>() ? true : false;
                header.escape_char = xf.read<uint8>();

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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);
        uint8* temp = NULL;

        header_generic header;
        data = read_header_fun(header, data, size);

        if (data)
        {
            const uint8* buffer = data;

            if (header.compressed)
            {
                temp = new uint8[33678];
                depack_fun(temp, data, 33678, size - 18, header.escape_char);
                buffer = temp;
            }

            uint8* background = new uint8[200];
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
    const uint8* read_header_gun(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

        if (check_format(0x4000, 33603, load_address, size))
        {
            uint8 keyword[] = "GUNPAINT (JZ)   ";
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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_gun(header, data, size);

        if (data)
        {
            uint8 *background = new uint8[200];
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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x5000, 12148);

        if (data)
        {
            const uint8* bitmap_c64 = data + 0x1000;
            const uint8* video_ram = data + 0x800;
            const uint8* color_ram = data;
            const uint8* background = data + 0x2f40;

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            uint8* image = so->lock<uint8>();
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

                    uint8 byte = bitmap_c64[bitmap_offset];
                    int bit_pattern = (byte >> (6 - (x_offset & 0x6))) & 0x3;

                    uint8 index = 0;
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
        const uint8* data = s->read(size);

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
    void depack_him(uint8* buffer, const uint8* input, int scansize, int insize, uint8 escape_char)
    {
        (void) scansize;

        const uint8* in = input + insize - 1;
        const uint8* in_end = input + 0x10 - 1;

        uint8* out = buffer + 0x3ff2 - 1;
        const uint8* out_end = buffer - 1;
        
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

    const uint8* read_header_him(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);
        uint8* temp = NULL;

        header_generic header;
        data = read_header_him(header, data, size);

        if (data)
        {
            const uint8* buffer = data;

            if (header.compressed)
            {
                temp = new uint8[16383];
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
    const uint8* read_header_koa(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x3f8e, 9332);

        if (data)
        {
            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            uint8* image = so->lock<uint8>();
            std::memset(image, 0, header.width * header.height);

            uint8* color_ram = new uint8[1000];
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
    void depack_ppp(uint8* buffer, const uint8* input, int scansize, int insize, uint8 escape_char)
    {
        uint8* buffer_end = buffer + scansize;
        const uint8* input_end = input + insize;

        for (; buffer < buffer_end && input < input_end;)
        {
            uint8 v = *input++;

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

    const uint8* read_header_pp(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

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
                    header.escape_char = xf.read<uint8>();
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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_pp(header, data, size);

        if (data)
        {
            const uint8* buffer = data;
            uint8* temp = NULL;

            if (header.compressed)
            {
                temp = new uint8[33600];
                depack_ppp(temp, data, 33600, size - 6, header.escape_char);
                buffer = temp;
            }

            uint8* background = new uint8[200];
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
    const uint8* read_header_shf(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

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
            header.escape_char = xf.read<uint8>();
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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);
        uint8* temp = NULL;

        header_generic header;
        data = read_header_shf(header, data, size);

        if (data)
        {
            const uint8* buffer = data;
/*
            if (header.compressed)
            {
                temp = new uint8[15874];
                rle_ecb(temp, data, 15874, size - 3, header.escape_char);
                buffer = temp;

                FILE *f = fopen("unpacked.shf", "wb");
                fwrite(buffer, 1, 15874, f);
                fclose(f);
            }
*/
            const uint8 *bitmap_c64 = buffer + 0x2000;
            const uint8 *video_ram = buffer;
            uint8 sprite_color1 = *(buffer + 0x3e8);
            uint8 sprite_color2 = *(buffer + 0x3e9);

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            uint8* image = so->lock<uint8>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int offset = x + (y * header.width);
                    uint8 index = 0;

                    // Hires data
                    int x_offset = (x + 112) & 0x7;
                    int y_offset = (y + 1) & 0x7;
                    int bitmap_offset = ((x + 112) & 0xfffffff8) + ((y + 1) & 0x7) + (((y + 1) >> 3) * (40 * 8));
                    int screen_offset = bitmap_offset >> 3;

                    uint8 byte = bitmap_c64[bitmap_offset];
                    int bit_pattern = (byte >> (7 - x_offset)) & 0x1;

                    // 2 x overlay sprite data
                    // - Multiplexed every 21 scanlines
                    int sprite_nb = (x / 24);
                    int sprite_line = (y % 21);
                    int sprite_ram_bank = y & 0x7;

                    uint8 sprite_pointer1 = buffer[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb];
                    /*
                    if (sprite_pointer1 > 15872)
                    {
                        FUSIONCORE_EXCEPTION("SHF: invalid sprite pointer.");
                    }
                    */

                    int sprite_byte_offset1 = (sprite_pointer1 * 64) + (sprite_line * 3) + (x % 24) / 8;
                    uint8 sprite_byte1 = buffer[sprite_byte_offset1];
                    int sprite_bit_pattern1 = (sprite_byte1 >> (7 - (x & 0x7))) & 0x1;

                    uint8 sprite_pointer2 = buffer[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb + 4];
                    /*
                    if (sprite_pointer2 > 15872)
                    {
                        FUSIONCORE_EXCEPTION("SHF: invalid sprite pointer.");
                    }
                    */

                    int sprite_byte_offset2 = (sprite_pointer2 * 64) + (sprite_line * 3) + (x % 24) / 8;
                    uint8 sprite_byte2 = buffer[sprite_byte_offset2];
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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);

        header_generic header;
        data = read_header_generic(header, data, size, 0x4000, 15362);

        if (data)
        {
            header.width = 144;
            header.height = 168;

            const uint8 *bitmap_c64 = data + 0x2000;
            const uint8 *video_ram = data;
            uint8 sprite_color = *(data + 0x3e9);

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            uint8* image = so->lock<uint8>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int offset = x + (y * header.width);
                    uint8 index = 0;

                    // Hires data
                    int x_offset = (x + 88) & 0x7;
                    int y_offset = y & 0x7;
                    int bitmap_offset = ((x + 88) & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                    int screen_offset = bitmap_offset >> 3;

                    uint8 byte = bitmap_c64[bitmap_offset];
                    int bit_pattern = (byte >> (7 - x_offset)) & 0x1;

                    // Overlay sprite data
                    // - Multiplexed every 21 scanlines
                    int sprite_nb = (x / 24) + 1;
                    int sprite_line = (y % 21);
                    int sprite_ram_bank = (y + 7) & 0x7;
                    uint8 sprite_pointer = data[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb];
                    int sprite_byte_offset = (sprite_pointer * 64) + (sprite_line * 3) + (x % 24) / 8;

                    if (sprite_byte_offset > 15360)
                    {
                        FUSIONCORE_EXCEPTION("SHF-XL: invalid sprite pointer.");
                    }

                    uint8 sprite_byte = data[sprite_byte_offset];
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
    void depack_mci(uint8* buffer, const uint8* input, int scansize, int insize)
    {
        const uint8* in = input + insize - 1;
        const uint8* in_end = input + 272;

        uint8* out = buffer + scansize - 1;
        const uint8* out_end = buffer - 1;
        
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

    const uint8* read_header_mci(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

        if (check_format(0x9c00, 19434, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            header.compressed = false;
            return xf;
        }
        else if (load_address == 0x0801)
        {
            uint8 keyword[] = "2059";
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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);
        uint8* temp = NULL;

        header_generic header;
        data = read_header_mci(header, data, size);

        if (data)
        {
            const uint8* buffer = data;

            if (header.compressed)
            {
                temp = new uint8[19432];
                depack_mci(temp, data, 19432, size - 2);
                buffer = temp;
            }

            uint8* background = new uint8[200];
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
    const uint8* read_header_ufli(header_generic& header, const uint8* data, int size)
    {
        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

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
            header.escape_char = xf.read<uint8>();
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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);
        uint8* temp = NULL;

        header_generic header;
        data = read_header_ufli(header, data, size);

        if (data)
        {
            const uint8* buffer = data;

            if (header.compressed)
            {
                temp = new uint8[16192];
                rle_ecb(temp, data, 16192, size - 3, header.escape_char);
                buffer = temp;
            }

            const uint8* bitmap_c64 = buffer + 0x2000;
            const uint8* video_ram = buffer + 0x1000;
            const uint8* sprite_colors = buffer + 0xff0;
            const uint8 background_color = *(buffer + 0xff1);
            bool ufli2 = *(buffer + 0xfef) ? true : false;

            ucolor palette[256];
            std::memset(palette, 0, 256 * sizeof(ucolor));
            std::memcpy(palette, c64_palette, 16 * sizeof(ucolor));

            surface* so = bitmap::create(header.width, header.height, pixelformat(palette));
            uint8* image = so->lock<uint8>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int offset = x + (y * header.width);
                    uint8 index = 0;

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

                        uint8 byte = bitmap_c64[bitmap_offset];
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

                        uint8 sprite_byte = buffer[sprite_offset + sprite_byte_offset];
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
    void depack_uifli(uint8* buffer, const uint8* input, int scansize, int insize, uint8 escape_char)
    {
        uint8* buffer_end = buffer + scansize;
        const uint8* input_end = input + insize;

        for (; buffer < buffer_end && input < input_end;)
        {
            uint8 look_ahead1 = *(input + 2);
            uint8 look_ahead2 = *(input + 3);
            uint8 look_ahead3 = *(input + 4);
            uint8 v = *input++;

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

    const uint8* read_header_uifli(header_generic& header, const uint8* data, int size)
    {
        (void) size;

        infilter xf(data);
        uint16 load_address = xf.read<uint16>();

        if (load_address == 0x4000)
        {
            header.width = 320;
            header.height = 200;
            header.compressed = true;
            header.escape_char = xf.read<uint8>();
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
        const uint8* data = s->read(size);

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
        const uint8* data = s->read(size);
        uint8* temp = NULL;

        header_generic header;
        data = read_header_uifli(header, data, size);

        if (data)
        {
            const uint8* buffer = data;

            if (header.compressed)
            {
                temp = new uint8[32897];
                depack_uifli(temp, data, 32897, size - 3, header.escape_char);
                buffer = temp;
            }

            const uint8* bitmap_c64[2] = { buffer + 0x2000, buffer + 0x6000 };
            const uint8* video_ram[2] = { buffer, buffer + 0x4000 };
            uint8 sprite_color[2] = { *(buffer + 0xff0), *(buffer + 0x4ff0) };
            const uint8* sprites[2] = { buffer + 0x1000, buffer + 0x5000 };

            surface* so = bitmap::create(header.width, header.height, pixelformat::argb8888);
            ucolor* image = so->lock<ucolor>();
            std::memset(image, 0, header.width * header.height);

            int x, y;
            for (y = 0; y < header.height; ++y)
            {
                for (x = 0; x < header.width; ++x)
                {
                    int offset = x + (y * header.width);
                    uint8 index[2] = { 0, 0 };

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

                        uint8 byte[2];
                        int bit_pattern[2];
                        uint8 sprite_byte[2];
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

namespace
{
    using namespace mango;

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

    struct InterfaceMPIC : Interface
    {
        InterfaceMPIC(Memory memory)
            : Interface(memory)
        {
            m_header.width   = 0; // TODO
            m_header.height  = 0; // TODO
            m_header.format  = Format(); // TODO
        }

        void decodeImage(Surface& dest) override
        {
            // TODO
        }
    };

    ImageDecoderInterface* createInterfaceMPIC(Memory memory)
    {
        ImageDecoderInterface* x = new InterfaceMPIC(memory);
        return x;
    }

} // namespace

namespace mango
{

    void registerImageDecoderC64()
    {
        // Advanced Art Studio
        registerImageDecoder(createInterfaceMPIC, "mpic");

#if 0 // TODO
        // AFLI-editor v2.0
        registerImageDecoder(createInterfaceAFL, "afl");
        registerImageDecoder(createInterfaceAFL, "afli");

        // Amica Paint
        registerImageDecoder(createInterfaceAMI, "ami");

        // Art Studio
        registerImageDecoder(createInterfaceART, "art");
        registerImageDecoder(createInterfaceART, "ocp");

        // Artist 64
        registerImageDecoder(createInterface, "a64");

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
