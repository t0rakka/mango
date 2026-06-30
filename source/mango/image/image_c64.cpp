/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Commodore 64 decoders copyright (C) 2011 Toni Lönnberg. All rights reserved.
*/

#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

// References:
// - https://codebase.c64.org/doku.php?id=base:c64_grafix_files_specs_list_v0.03
// - http://unusedino.de/ec64/technical/aay/c64/

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // Commodore 64 utilities
    // ------------------------------------------------------------

    static constexpr size_t g_c64_palette_size = 16;

    const Color g_c64_palette_colors [] =
    { 
        0xFF000000,
        0xFFFFFFFF,
        0xFF2B3768,
        0xFFB2A470,
        0xFF863D6F,
        0xFF438D58,
        0xFF792835,
        0xFF6FC7B8,
        0xFF254F6F,
        0xFF003943,
        0xFF59679A,
        0xFF444444,
        0xFF6C6C6C,
        0xFF84D29A,
        0xFFB55E6C,
        0xFF959595,
    };

    struct PaletteC64 : Palette
    {
        PaletteC64()
        {
            size = g_c64_palette_size;

            for (size_t i = 0; i < g_c64_palette_size; ++i)
            {
                color[i] = g_c64_palette_colors[i];
            }
        }
    } g_c64_palette;

    void rle_ecb(u8* buffer, const u8* input, int scansize, const u8* input_end, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;

        for ( ; buffer < buffer_end && input < input_end; )
        {
            u8 v = *input++;

            if (v == escape_char)
            {
                // escape sequence is two more bytes: count and value
                if (input + 2 > input_end)
                {
                    break;
                }

                int n = *input++;
                if (n == 0)
                {
                    n = 256;
                }

                u8 value = *input++;

                // clamp the run to the remaining output
                if (n > buffer_end - buffer)
                {
                    n = int(buffer_end - buffer);
                }

                std::memset(buffer, value, n);
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

    void multicolor_to_surface(const Surface& s, const u8 *data, int width, int height, 
                               u32 bitmap_offset, u32 video_ram_offset, u32 color_ram_offset, 
                               u32 background_offset, u32 opcode_colors_offset, 
                               int background_mode, bool fli)
    {
        Buffer temp(width * height, 0);

        convert_multicolor_bitmap(width, height, temp, 
                                  data + bitmap_offset, data + video_ram_offset, 
                                  data + color_ram_offset, data + background_offset, data + opcode_colors_offset,
                                  background_mode, fli);

        Surface indices(width, height, IndexedFormat(8), width, temp);
        indices.palette = &g_c64_palette;
        resolve(s, indices);
    }

    void multicolor_interlace_to_surface(const Surface& s, const u8 *data, int width, int height,
                                         u32 bitmap_offset_1, u32 bitmap_offset_2, 
                                         u32 video_ram_offset_1, u32 video_ram_offset_2, 
                                         u32 color_ram_offset, u8 *background_colors,
                                         u8 *opcode_colors,
                                         int background_mode,
                                         bool fli,
                                         int mode)
    {
        PaletteC64 palette;

        Buffer bitmap1(width * height);
        Buffer bitmap2(width * height);

        convert_multicolor_bitmap(width, height, bitmap1, data + bitmap_offset_1, data + video_ram_offset_1, data + color_ram_offset, background_colors, opcode_colors, background_mode, fli);
        convert_multicolor_bitmap(width, height, bitmap2, data + bitmap_offset_2, data + video_ram_offset_2, data + color_ram_offset, background_colors, opcode_colors, background_mode, fli);

        for (int y = 0; y < height; ++y)
        {
            Color* image = s.address<Color>(0, y);

            for (int x = 0; x < width; ++x)
            {
                const int offset = x + width * y;

                image[x].a = 0xff;

                if (mode == 0)
                {
                    image[x].r = (palette[bitmap1[offset]].r >> 1) + (palette[bitmap2[offset]].r >> 1);
                    image[x].g = (palette[bitmap1[offset]].g >> 1) + (palette[bitmap2[offset]].g >> 1);
                    image[x].b = (palette[bitmap1[offset]].b >> 1) + (palette[bitmap2[offset]].b >> 1);
                }
                else if (mode == 1)
                {
                    if ((offset % 320) == 0)
                    {
                        image[x].r = (palette[bitmap1[offset]].r >> 1);
                        image[x].g = (palette[bitmap1[offset]].g >> 1);
                        image[x].b = (palette[bitmap1[offset]].b >> 1);
                    }
                    else
                    {
                        image[x].r = (palette[bitmap1[offset + 0]].r >> 1) + (palette[bitmap2[offset - 1]].r >> 1);
                        image[x].g = (palette[bitmap1[offset + 0]].g >> 1) + (palette[bitmap2[offset - 1]].g >> 1);
                        image[x].b = (palette[bitmap1[offset + 0]].b >> 1) + (palette[bitmap2[offset - 1]].b >> 1);
                    }
                }
                else if (mode == 2)
                {
                    if ((offset & 0x1) == 0)
                    {
                        image[x].r = palette[bitmap1[offset]].r;
                        image[x].g = palette[bitmap1[offset]].g;
                        image[x].b = palette[bitmap1[offset]].b;
                    }
                    else
                    {
                        image[x].r = palette[bitmap2[offset]].r;
                        image[x].g = palette[bitmap2[offset]].g;
                        image[x].b = palette[bitmap2[offset]].b;
                    }
                }
            }
        }
    }

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

    void hires_to_surface(const Surface& s, const u8* data, int width, int height, 
                          u32 bitmap_offset, u32 video_ram_offset, 
                          bool fli = false,
                          bool show_fli_bug = false,
                          u8 fli_bug_color = 0)
    {
        Buffer temp(width * height, 0);

        convert_hires_bitmap(width, height, temp, data + bitmap_offset, data + video_ram_offset, fli, show_fli_bug, fli_bug_color);

        Surface indices(width, height, IndexedFormat(8), width, temp);
        indices.palette = &g_c64_palette;
        resolve(s, indices);
    }

    void hires_interlace_to_surface(const Surface& s, const u8* data, int width, int height,
                                    u32 bitmap_offset_1, u32 bitmap_offset_2, 
                                    u32 video_ram_offset_1, u32 video_ram_offset_2, 
                                    bool fli = false,
                                    bool show_fli_bug = false,
                                    u8 fli_bug_color = 0)
    {
        PaletteC64 palette;

        Buffer bitmap1(width * height);
        Buffer bitmap2(width * height);

        convert_hires_bitmap(width, height, bitmap1, data + bitmap_offset_1, data + video_ram_offset_1, fli, show_fli_bug, fli_bug_color);
        convert_hires_bitmap(width, height, bitmap2, data + bitmap_offset_2, data + video_ram_offset_2, fli, show_fli_bug, fli_bug_color);

        for (int y = 0; y < height; ++y)
        {
            Color* image = s.address<Color>(0, y);

            for (int x = 0; x < width; ++x)
            {
                int offset = x + y * width;

                image[x].r = (palette[bitmap1[offset]].r >> 1) + (palette[bitmap2[offset]].r >> 1);
                image[x].g = (palette[bitmap1[offset]].g >> 1) + (palette[bitmap2[offset]].g >> 1);
                image[x].b = (palette[bitmap1[offset]].b >> 1) + (palette[bitmap2[offset]].b >> 1);
                image[x].a = 0xff;
            }
        }
    }

    bool check_format(u16 format_address, size_t format_size, u16 load_address, size_t size)
    {
        //printf("format_address: %d, format_size: %d, load_address: %d, size: %d \n", int(format_address), int(format_size), int(load_address), int(size));
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

        const u8* parse(const u8* data, size_t size, u16 format_address, size_t format_size)
        {
            // need at least the 2-byte load address
            if (size < 2)
            {
                return nullptr;
            }

            LittleEndianConstPointer p = data;
            u16 load_address = p.read16();

            if (check_format(format_address, format_size, load_address, size))
            {
                width = 320;
                height = 200;
                return p;
            }

            return nullptr;
        }

        void multicolor_load(const Surface& s, const u8* data,
                             u32 bitmap_offset, u32 video_ram_offset, 
                             u32 color_ram_offset, u32 background_offset, u32 opcode_colors_offset,
                             int background_mode, bool fli)
        {
            multicolor_to_surface(s, data, width, height, 
                                  bitmap_offset, video_ram_offset, color_ram_offset, background_offset, opcode_colors_offset,
                                  background_mode, fli);
        }

        void hires_load(const Surface& s, const u8* data, 
                        u32 bitmap_offset, u32 video_ram_offset, bool fli)
        {
            hires_to_surface(s, data, width, height, bitmap_offset, video_ram_offset, fli);
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
        }

        ~Interface()
        {
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_memory.address)
            {
                status.setError("C64 ImageDecoder - no data.");
                return status;
            }

            // A failed/unrecognised header leaves the dimensions at zero (and the derived
            // decoder's data pointer null); bail out before decodeImage() dereferences it.
            if (header.width <= 0 || header.height <= 0)
            {
                status.setError("[ImageDecoder.C64] Incorrect or unsupported file.");
                return status;
            }

            DecodeTargetBitmap target(dest, header.width, header.height, header.format);

            const char* error = decodeImage(target);
            if (error)
            {
                status.setError(error);
            }
            else
            {
                target.resolve();
            }

            status.direct = target.isDirect();

            return status;
        }

        virtual const char* decodeImage(const Surface& dest) = 0;
    };

    struct GenericInterface : Interface
    {
        header_generic m_generic_header;
        const u8* m_data;

        GenericInterface(ConstMemory memory, u16 format_address, size_t format_size)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = m_generic_header.parse(memory.address, memory.size, format_address, format_size);
            if (m_data)
            {
                header.width  = m_generic_header.width;
                header.height = m_generic_header.height;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
        }
    };

    // ------------------------------------------------------------
    // ImageDecoder: MPIC (Advanced Art Studio)
    // ------------------------------------------------------------

    struct InterfaceMPIC : GenericInterface
    {
        InterfaceMPIC(ConstMemory memory)
            : GenericInterface(memory, 0x2000, 10018)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x0, 0x1f40, 0x2338, 0x2329, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceMPIC(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceMPIC(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: AFL (AFLI-editor v2.0)
    // ------------------------------------------------------------

    struct InterfaceAFL : GenericInterface
    {
        InterfaceAFL(ConstMemory memory)
            : GenericInterface(memory, 0x4000, 16385)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            hires_to_surface(s, m_data, header.width, header.height, 0x2000, 0x0, true, false, 0);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceAFL(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceAFL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: AMI (Amica Painter)
    // ------------------------------------------------------------

    struct InterfaceAMI : Interface
    {
        header_generic m_generic_header;
        const u8* m_data;

        InterfaceAMI(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            // read header
            const u8* end = memory.end();

            if (memory.size < 4)
            {
                return;
            }

            if (end[-1] == 0x0 && end[-2] == 0xc2)
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_generic_header.compressed = true;
                m_generic_header.escape_char = 0xc2;

                m_data = memory.address + 2;
            }
        }
            
        const char* decodeImage(const Surface& s) override
        {
            const u8* end = m_memory.end();

            Buffer temp;
            const u8* buffer = m_data;

            if (m_generic_header.compressed)
            {
                temp.reset(10513);
                rle_ecb(temp, m_data, 10513, end, m_generic_header.escape_char);
                buffer = temp;
            }

            multicolor_to_surface(s, buffer, header.width, header.height, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, false, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceAMI(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceAMI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: ART (Art Studio)
    // ------------------------------------------------------------

    struct InterfaceART : GenericInterface
    {
        InterfaceART(ConstMemory memory)
            : GenericInterface(memory, 0x2000, 9009)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.hires_load(s, m_data, 0x0, 0x1f40, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceART(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceART(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: A64
    // ------------------------------------------------------------

    struct InterfaceA64 : GenericInterface
    {
        InterfaceA64(ConstMemory memory)
            : GenericInterface(memory, 0x4000, 10242)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x0, 0x2000, 0x2400, 0x27ff, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceA64(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceA64(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: BLP (Blazing Paddles)
    // ------------------------------------------------------------

    struct InterfaceBLP : GenericInterface
    {
        InterfaceBLP(ConstMemory memory)
            : GenericInterface(memory, 0xa000, 10242)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x0, 0x2000, 0x2400, 0x1f80, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceBLP(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceBLP(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: CDU (CDU-Paint)
    // ------------------------------------------------------------

    struct InterfaceCDU : GenericInterface
    {
        InterfaceCDU(ConstMemory memory)
            : GenericInterface(memory, 0x7eef, 10277)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x111, 0x2051, 0x2439, 0x2821, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceCDU(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceCDU(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: DOL (Dolphin Ed)
    // ------------------------------------------------------------

    struct InterfaceDOL : GenericInterface
    {
        InterfaceDOL(ConstMemory memory)
            : GenericInterface(memory, 0x5800, 10242)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x800, 0x400, 0x0, 0x7e8, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceDOL(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceDOL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: DD (Doodle)
    // ------------------------------------------------------------

    struct InterfaceDD : GenericInterface
    {
        InterfaceDD(ConstMemory memory)
            : GenericInterface(memory, 0x1c00, 9218)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.hires_load(s, m_data, 0x400, 0x0, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceDD(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceDD(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: DRL (Drazlace)
    // ------------------------------------------------------------

    struct InterfaceDRL : Interface
    {
        const u8* m_data;
        bool m_compressed;
        u8 m_escape_char;

        InterfaceDRL(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
            , m_compressed(false)
            , m_escape_char(0)
        {
            if (memory.size < 2)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x5800, 18242, load_address, memory.size))
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else
            {
                // load address (2) + keyword (13) + escape byte (1)
                if (load_address == 0x5800 && memory.size >= 16)
                {
                    u8 keyword[] = "DRAZLACE! 1.0";
                    if (std::memcmp(keyword, p, sizeof(keyword) - 1) == 0)
                    {
                        p += sizeof(keyword) - 1;

                        header.width = 320;
                        header.height = 200;
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                        m_compressed = true;
                        m_escape_char = p.read8();
                        m_data = p;
                    }
                }
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            const u8* end = m_memory.end();

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(18240);
                rle_ecb(temp, m_data, 18240, end, m_escape_char);
                buffer = temp;
            }

            Buffer background(200, *(buffer + 0x2740));
            multicolor_interlace_to_surface(s, buffer, header.width, header.height, 0x800, 0x2800, 0x400, 0x400, 0x0, background, 0x0, 2, false, 2);

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceDRL(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceDRL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: DRZ (Drazpaint)
    // ------------------------------------------------------------

    struct InterfaceDRZ : Interface
    {
        const u8* m_data = nullptr;
        bool m_compressed = false;
        u8 m_escape_char = 0;

        InterfaceDRZ(ConstMemory memory)
            : Interface(memory)
        {
            if (memory.size < 2)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x5800, 10051, load_address, memory.size))
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else
            {
                // load address (2) + keyword (13) + escape byte (1)
                if (load_address == 0x5800 && memory.size >= 16)
                {
                    u8 keyword[] = "DRAZPAINT 2.0";
                    if (std::memcmp(keyword, p, sizeof(keyword) - 1) == 0)
                    {
                        p += sizeof(keyword) - 1;

                        header.width = 320;
                        header.height = 200;
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                        m_compressed = true;
                        m_escape_char = p.read8();
                        m_data = p;
                    }
                }
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            const u8* end = m_memory.end();

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(10049);
                rle_ecb(temp, m_data, 10049, end, m_escape_char);
                buffer = temp;
            }

            multicolor_to_surface(s, buffer, header.width, header.height, 0x800, 0x400, 0x0, 0x2740, 0x0, false, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceDRZ(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceDRZ(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: ECI (ECI Graphic Editor v1.0)
    // ------------------------------------------------------------

    struct InterfaceECI : Interface
    {
        const u8* m_data = nullptr;
        bool m_compressed = false;
        u8 m_escape_char = 0;

        InterfaceECI(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (memory.size < 3)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 32770, load_address, memory.size))
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else
            {
                if (load_address == 0x4000)
                {
                    header.width = 320;
                    header.height = 200;
                    header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                    m_compressed = true;
                    m_escape_char = p.read8();
                    m_data = p;
                }
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            const u8* end = m_memory.end();

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(32768);
                rle_ecb(temp, m_data, 32768, end, m_escape_char);
                buffer = temp;
            }

            hires_interlace_to_surface(s, buffer, header.width, header.height, 0x0, 0x4000, 0x2000, 0x6000, true, false, 0);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceECI(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceECI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: FPT (Face Painter)
    // ------------------------------------------------------------

    struct InterfaceFPT : GenericInterface
    {
        InterfaceFPT(ConstMemory memory)
            : GenericInterface(memory, 0x4000, 10004)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x0, 0x1f40, 0x2328, 0x2712, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceFPT(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceFPT(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: FD2 (FLI Designer 1.1 & 2.0 (FBI Crew))
    // ------------------------------------------------------------

    struct InterfaceFD2 : GenericInterface
    {
        InterfaceFD2(ConstMemory memory)
            : GenericInterface(memory, 0x3c00, 17409)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x2400, 0x400, 0x0, 0x0, 0x0, 0, true);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceFD2(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceFD2(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: FPR (FLI-Profi)
    // ------------------------------------------------------------

    struct InterfaceFPR : GenericInterface
    {
        InterfaceFPR(ConstMemory memory)
            : GenericInterface(memory, 0x3780, 18370)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            u8 sprite_color1 = m_data[0x448];
            u8 sprite_color2 = m_data[0x449];
            const u8 *sprite_colors = m_data + 0x280;

            Buffer temp(header.width * header.height, 0);
            u8* image = temp;

            convert_multicolor_bitmap(header.width, header.height, image, m_data + 0x2880, m_data + 0x880, m_data + 0x480, NULL, m_data + 0x380, 0, true);

            // Overlay sprite data
            // - Y-expanded
            // - Switching VIC bank every two scanlines, pattern: 1221
            for (int y = 0; y < 200; ++y)
            {
                for (int x = 0; x < 24; ++x)
                {
                    int offset = x + y * header.width;
                    u8 index = 0;

                    int sprite_nb = y / 42;
                    int sprite_line = (y % 42) >> 1;
                    int vic_bank = ((y + 1) >> 1) & 0x1;
                    int sprite_offset = (sprite_line * 3) + (sprite_nb * 64) + (vic_bank * 0x140);
                    int sprite_byte_offset = (x % 24) >> 3;

                    u8 sprite_byte = m_data[sprite_offset + sprite_byte_offset];
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

            Surface indices(header.width, header.height, IndexedFormat(8), header.width, temp);
            indices.palette = &g_c64_palette;
            resolve(s, indices);

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceFPR(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceFPR(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: FUN (Funpaint 2)
    // ------------------------------------------------------------

    void depack_fun(u8* buffer, const u8* input, int scansize, const u8* input_end, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;

        for (; buffer < buffer_end && input < input_end;)
        {
            u8 v = *input++;

            if (v == escape_char)
            {
                if (input + 2 > input_end)
                {
                    break;
                }

                int n = *input++;
                if (n == 0)
                {
                    break;
                }

                u8 value = *input++;

                if (n > buffer_end - buffer)
                {
                    n = int(buffer_end - buffer);
                }

                std::memset(buffer, value, n);
                buffer += n;
            }
            else
            {
                *buffer++ = v;
            }
        }
    }

    struct InterfaceFUN : Interface
    {
        const u8* m_data;
        bool m_compressed;
        u8 m_escape_char;

        InterfaceFUN(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
            , m_compressed(false)
            , m_escape_char(0)
        {
            if (memory.size < 2)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (load_address == 0x3ff0 && memory.size > 16)
            {
                u8 keyword[] = "FUNPAINT (MT) ";
                if (std::memcmp(keyword, p, sizeof(keyword) - 1) == 0)
                {
                    p += 14;
                    m_compressed = p.read8() != 0;
                    m_escape_char = p.read8();

                    if (!m_compressed)
                    {
                        if (memory.size == 33694)
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

                    header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    m_data = p;
                }
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            const u8* end = m_memory.end();

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(33678);
                depack_fun(temp, m_data, 33678, end, m_escape_char);
                buffer = temp;
            }

            Buffer background(200);
            std::memcpy(background.data() +   0, buffer + 0x3f48, 100);
            std::memcpy(background.data() + 100, buffer + 0x8328, 100);

            multicolor_interlace_to_surface(s, buffer, header.width, header.height, 0x2000, 0x63e8, 0x0, 0x43e8, 0x4000, background.data(), 0x0, 2, true, 2);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceFUN(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceFUN(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: GUN (Gunpaint)
    // ------------------------------------------------------------

    struct InterfaceGUN : Interface
    {
        const u8* m_data;

        InterfaceGUN(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (memory.size < 2)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 33603, load_address, memory.size))
            {
                u8 keyword[] = "GUNPAINT (JZ)   ";
                if (std::memcmp(keyword, memory.address + 0x3ea, 16) == 0)
                {
                    header.width = 320;
                    header.height = 200;
                    header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                    m_data = p;
                }
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            Buffer background(200);
            std::memcpy(background.data() +   0, m_data + 0x3f4f, 177);
            std::memcpy(background.data() + 177, m_data + 0x47e8, 20);
            background[197] = background[198] = background[199] = background[196];  // replicate the last color four times

            multicolor_interlace_to_surface(s, m_data, header.width, header.height, 0x2000, 0x6400, 0x0, 0x4400, 0x4000, background.data(), 0x0, 2, true, 2);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceGUN(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceGUN(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: HCB (HCB-Editor v0.05)
    // ------------------------------------------------------------

    struct InterfaceHCB : GenericInterface
    {
        InterfaceHCB(ConstMemory memory)
            : GenericInterface(memory, 0x5000, 12148)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            const u8* bitmap_c64 = m_data + 0x1000;
            const u8* video_ram = m_data + 0x800;
            const u8* color_ram = m_data;
            const u8* background = m_data + 0x2f40;

            Buffer temp(header.width * header.height, 0);
            u8* image = temp;

            for (int y = 0; y < header.height; ++y)
            {
                for (int x = 0; x < header.width; ++x)
                {
                    int x_offset = x & 0x7;
                    int y_offset = (y >> 2) & 0x1;
                    int bitmap_offset = (x & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                    int screen_offset = bitmap_offset >> 3;
                    int offset = x + y * header.width;

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

            Surface indices(header.width, header.height, IndexedFormat(8), header.width, temp);
            indices.palette = &g_c64_palette;
            resolve(s, indices);

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceHCB(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceHCB(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: HFC (Hires FLI Designer)
    // ------------------------------------------------------------

    struct InterfaceHFC : GenericInterface
    {
        InterfaceHFC(ConstMemory memory)
            : GenericInterface(memory, 0x4000, 16386)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            hires_to_surface(s, m_data, header.width, header.height, 0x0, 0x2000, true, false, 0);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceHFC(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceHFC(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: HIM (Hires Manager)
    // ------------------------------------------------------------

    const char* depack_him(u8* buffer, const u8* input, int scansize, int insize, u8 escape_char)
    {
        MANGO_UNREFERENCED(scansize);

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
                    return "Hires Manager: unpacked size does not match file format.";
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
                    return "Hires Manager: unpacked size does not match file format.";
                }

                for (int i = 0; i < n; ++i)
                {
                    *out-- = *in--;
                }
            }
        }

        return nullptr;
    }

    struct InterfaceHIM : Interface
    {
        const u8* m_data;
        bool m_compressed = false;

        InterfaceHIM(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (memory.size < 3)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 16385, load_address, memory.size))
            {
                if (*p == 0xff)
                {
                    header.width = 320;
                    header.height = 192;
                    header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                    m_compressed = false;
                    m_data = p;
                }
            }
            else if (load_address == 0x4000)
            {
                header.width = 320;
                header.height = 192;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = true;
                m_data = p;
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            Buffer temp;
            const u8* buffer = m_data;

            const char* error = nullptr;

            if (m_compressed)
            {
                temp.reset(16383);
                error = depack_him(temp, m_data, 16383, int(m_memory.size - 2), 0);
                buffer = temp;
            }

            hires_to_surface(s, buffer, header.width, header.height, 0x140, 0x2028, true, false, 0);
            return error;
        }
    };

    ImageDecodeInterface* createInterfaceHIM(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceHIM(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: KOA (Koala Painter)
    // ------------------------------------------------------------

    struct InterfaceKOA : Interface
    {
        const u8* m_data;

        InterfaceKOA(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (memory.size < 2)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x6000, 10003, load_address, memory.size))
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                m_data = p;
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            multicolor_to_surface(s, m_data, header.width, header.height, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, false, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceKOA(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceKOA(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: PMG (Paint Magic)
    // ------------------------------------------------------------

    struct InterfacePMG : GenericInterface
    {
        InterfacePMG(ConstMemory memory)
            : GenericInterface(memory, 0x3f8e, 9332)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            Buffer temp(header.width * header.height, 0);
            Buffer color_ram(1000, *(m_data + 0x1fb5));

            convert_multicolor_bitmap(header.width, header.height, temp.data(), 
                                      m_data + 0x72, m_data + 0x2072, 
                                      color_ram, m_data + 0x1fb2, NULL,
                                      1, false);

            Surface indices(header.width, header.height, IndexedFormat(8), header.width, temp);
            indices.palette = &g_c64_palette;
            resolve(s, indices);

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfacePMG(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfacePMG(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: PP (Pixel Perfect)
    // ------------------------------------------------------------

    void depack_ppp(u8* buffer, const u8* input, int scansize, const u8* input_end, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;

        for (; buffer < buffer_end && input < input_end;)
        {
            u8 v = *input++;

            if (v == escape_char)
            {
                if (input + 2 > input_end)
                {
                    break;
                }

                int n = (*input++) + 1;
                u8 value = *input++;

                if (n > buffer_end - buffer)
                {
                    n = int(buffer_end - buffer);
                }

                std::memset(buffer, value, n);
                buffer += n;
            }
            else
            {
                *buffer++ = v;
            }
        }
    }

    const u8* read_header_pp(header_generic& header, const u8* data, size_t size)
    {
        // load address (2) + signature bytes (3) + escape byte (1)
        if (size < 6)
        {
            return nullptr;
        }

        LittleEndianConstPointer p = data;
        u16 load_address = p.read16();

        if (check_format(0x3c00, 33602, load_address, size))
        {
            header.width = 320;
            header.height = 200;
            header.compressed = false;
            return p;
        }
        else
        {
            if (load_address == 0x3bfc)
            {
                if (p[0] == 0x10 && 
                    p[1] == 0x10 && 
                    p[2] == 0x10)
                {
                    p += 3;

                    header.width = 320;
                    header.height = 200;
                    header.compressed = true;
                    header.escape_char = p.read8();
                    return p;
                }
            }
        }

        return nullptr;
    }

    struct InterfacePP : Interface
    {
        header_generic m_generic_header;
        const u8* m_data;

        InterfacePP(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            m_data = read_header_pp(m_generic_header, memory.address, memory.size);
            if (m_data)
            {
                header.width  = m_generic_header.width;
                header.height = m_generic_header.height;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            const u8* end = m_memory.end();

            Buffer temp;
            const u8* buffer = m_data;

            if (m_generic_header.compressed)
            {
                temp.reset(33600);
                depack_ppp(temp, m_data, 33600, end, m_generic_header.escape_char);
                buffer = temp;
            }

            Buffer background(200, *(buffer + 0x437f));
            multicolor_interlace_to_surface(s, buffer, header.width, header.height, 0x2400, 0x6400, 0x400, 0x4400, 0x0, background, nullptr, 1, true, 2);

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfacePP(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfacePP(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: RPM (Run Paint)
    // ------------------------------------------------------------

    struct InterfaceRPM : GenericInterface
    {
        InterfaceRPM(ConstMemory memory)
            : GenericInterface(memory, 0x6000, 10006)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceRPM(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceRPM(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: SAR (Saracen Paint)
    // ------------------------------------------------------------

    struct InterfaceSAR : GenericInterface
    {
        InterfaceSAR(ConstMemory memory)
            : GenericInterface(memory, 0x7800, 10018)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x400, 0x0, 0x2400, 0x3f0, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceSAR(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceSAR(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: SHF (SHF-Editor v1.0)
    // ------------------------------------------------------------

    struct InterfaceSHF : Interface
    {
        const u8* m_data;
        bool m_compressed = false;
        u8 m_escape_char = 0;

        InterfaceSHF(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (memory.size < 3)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 15874, load_address, memory.size))
            {
                header.width = 96;
                header.height = 167;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else if (load_address == 0xa000)
            {
                header.width = 96;
                header.height = 167;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = true;
                m_escape_char = p.read8();
                m_data = p;
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            const u8* buffer = m_data;

            const u8* bitmap_c64 = buffer + 0x2000;
            const u8* video_ram = buffer;
            u8 sprite_color1 = *(buffer + 0x3e8);
            u8 sprite_color2 = *(buffer + 0x3e9);

            Buffer tempImage(header.width * header.height, 0);
            u8* image = tempImage;

            for (int y = 0; y < header.height; ++y)
            {
                for (int x = 0; x < header.width; ++x)
                {
                    int offset = x + y * header.width;
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
                        return "SHF: invalid sprite pointer.";
                    }
                    */

                    int sprite_byte_offset1 = (sprite_pointer1 * 64) + (sprite_line * 3) + (x % 24) / 8;
                    u8 sprite_byte1 = buffer[sprite_byte_offset1];
                    int sprite_bit_pattern1 = (sprite_byte1 >> (7 - (x & 0x7))) & 0x1;

                    u8 sprite_pointer2 = buffer[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb + 4];
                    /*
                    if (sprite_pointer2 > 15872)
                    {
                        return "SHF: invalid sprite pointer.";
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

            Surface indices(header.width, header.height, IndexedFormat(8), header.width, tempImage);
            indices.palette = &g_c64_palette;
            resolve(s, indices);

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceSHF(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceSHF(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: SHFXL (SHF-XL v1.0)
    // ------------------------------------------------------------

    struct InterfaceSHFXL : GenericInterface
    {
        InterfaceSHFXL(ConstMemory memory)
            : GenericInterface(memory, 0x4000, 15362)
        {
            if (m_data)
            {
                header.width = 144;
                header.height = 168;
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            if (!m_data)
            {
                return "SHF-XL: invalid data.";
            }

            const u8* buffer = m_data;

            const u8* bitmap_c64 = buffer + 0x2000;
            const u8* video_ram = buffer;
            u8 sprite_color = *(buffer + 0x3e9);

            Buffer tempImage(header.width * header.height, 0);
            u8* image = tempImage;

            for (int y = 0; y < header.height; ++y)
            {
                for (int x = 0; x < header.width; ++x)
                {
                    int offset = x + y * header.width;
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
                    u8 sprite_pointer = buffer[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb];
                    int sprite_byte_offset = (sprite_pointer * 64) + (sprite_line * 3) + (x % 24) / 8;

                    if (sprite_byte_offset > 15360)
                    {
                        return "SHF-XL: invalid sprite pointer.";
                    }

                    u8 sprite_byte = buffer[sprite_byte_offset];
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

            Surface indices(header.width, header.height, IndexedFormat(8), header.width, tempImage);
            indices.palette = &g_c64_palette;
            resolve(s, indices);

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceSHFXL(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceSHFXL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: MCI (True Paint)
    // ------------------------------------------------------------

    const char* depack_mci(u8* buffer, const u8* input, int scansize, int insize)
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
                    return "True Paint: unpacked size does not match file format.";
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
                    return "True Paint: unpacked size does not match file format.";
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
                    return "True Paint: unpacked size does not match file format.";
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
                    return "True Paint: unpacked size does not match file format.";
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
                    return "True Paint: unpacked size does not match file format.";
                }

                *out-- = *(input + 0x88);
                *out-- = *(input + 0x88);
            }
            else if (v == *(input + 0x85))
            {
                // 2nd 2-character run
                if (out - 2 < out_end)
                {
                    return "True Paint: unpacked size does not match file format.";
                }

                *out-- = *(input + 0x89);
                *out-- = *(input + 0x89);
            }
            else if (v == *(input + 0x86))
            {
                // 3rd 2-character run
                if (out - 2 < out_end)
                {
                    return "True Paint: unpacked size does not match file format.";
                }

                *out-- = *(input + 0x8a);
                *out-- = *(input + 0x8a);
            }
            else
            {
                *out-- = v;
            }
        }

        return nullptr;
    }

    struct InterfaceMCI : Interface
    {
        const u8* m_data;
        bool m_compressed = false;

        InterfaceMCI(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            // load address (2) + signature offset 5 + "2059" (4)
            if (memory.size < 11)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x9c00, 19434, load_address, memory.size))
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else if (load_address == 0x0801)
            {
                u8 keyword[] = "2059";
                if (std::memcmp(keyword, p + 5, sizeof(keyword) - 1) == 0)
                {
                    header.width = 320;
                    header.height = 200;
                    header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                    m_compressed = true;
                    m_data = p;
                }
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            Buffer temp;
            const u8* buffer = m_data;

            const char* error = nullptr;

            if (m_compressed)
            {
                temp.reset(19432);
                error = depack_mci(temp, m_data, 19432, int(m_memory.size - 2));
                buffer = temp;
            }

            Buffer background(200, *(buffer + 0x3e8));
            multicolor_interlace_to_surface(s, buffer, header.width, header.height, 0x400, 0x2400, 0x0, 0x4400, 0x4800, background, 0x0, 2, false, 2);

            return error;
        }
    };

    ImageDecodeInterface* createInterfaceMCI(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceMCI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: UFLI (UFLI-Editor v1.0 & v2.0)
    // ------------------------------------------------------------

    struct InterfaceUFLI : Interface
    {
        const u8* m_data;
        bool m_compressed = false;
        u8 m_escape_char = 0;

        InterfaceUFLI(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (memory.size < 3)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 16194, load_address, memory.size))
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else if (load_address == 0x8000)
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = true;
                m_escape_char = p.read8();
                m_data = p;
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(16192);
                const u8* end = m_memory.end();
                rle_ecb(temp, m_data, 16192, end, m_escape_char);
                buffer = temp;
            }

            const u8* bitmap_c64 = buffer + 0x2000;
            const u8* video_ram = buffer + 0x1000;
            const u8* sprite_colors = buffer + 0xff0;
            const u8 background_color = *(buffer + 0xff1);
            bool ufli2 = *(buffer + 0xfef) ? true : false;

            PaletteC64 palette;

            Buffer tempImage(header.width * header.height, 0);
            u8* image = tempImage;

            for (int y = 0; y < header.height; ++y)
            {
                for (int x = 0; x < header.width; ++x)
                {
                    int offset = x + y * header.width;
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

            Surface indices(header.width, header.height, IndexedFormat(8), header.width, tempImage);
            indices.palette = &g_c64_palette;
            resolve(s, indices);

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceUFLI(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceUFLI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: UIFLI (UIFLI Editor v1.0)
    // ------------------------------------------------------------

    void depack_uifli(u8* buffer, const u8* input, int scansize, int insize, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;
        const u8* input_end = input + insize;

        for ( ; buffer < buffer_end && input < input_end; )
        {
            // the run detection peeks three bytes ahead; fall back to a literal copy
            // (and let the next iteration's input < input_end check stop us) when those
            // look-ahead bytes are not present
            if (input + 4 >= input_end)
            {
                *buffer++ = *input++;
                continue;
            }

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

                if (n > buffer_end - buffer)
                {
                    n = int(buffer_end - buffer);
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

    struct InterfaceUIFLI : Interface
    {
        const u8* m_data;
        bool m_compressed = false;
        u8 m_escape_char = 0;

        InterfaceUIFLI(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            if (memory.size < 3)
            {
                return;
            }

            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (load_address == 0x4000)
            {
                header.width = 320;
                header.height = 200;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = true;
                m_escape_char = p.read8();
                m_data = p;
            }
        }

        const char* decodeImage(const Surface& s) override
        {
            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(32897);
                depack_uifli(temp, m_data, 32897, int(m_memory.size - 3), m_escape_char);
                buffer = temp;
            }

            const u8* bitmap_c64[2] = { buffer + 0x2000, buffer + 0x6000 };
            const u8* video_ram[2] = { buffer, buffer + 0x4000 };
            u8 sprite_color[2] = { *(buffer + 0xff0), *(buffer + 0x4ff0) };
            const u8* sprites[2] = { buffer + 0x1000, buffer + 0x5000 };

            for (int y = 0; y < header.height; ++y)
            {
                Color* image = s.address<Color>(0, y);

                for (int x = 0; x < header.width; ++x)
                {
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

                    Color color0 = g_c64_palette_colors[index[0]]; 
                    Color color1 = g_c64_palette_colors[index[1]]; 

                    image[x].r = (color0.r >> 1) + (color1.r >> 1);
                    image[x].g = (color0.g >> 1) + (color1.g >> 1);
                    image[x].b = (color0.b >> 1) + (color1.b >> 1);
                    image[x].a = 0xff;
                }
            }

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceUIFLI(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceUIFLI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: VID (Vidcom 64)
    // ------------------------------------------------------------

    struct InterfaceVID : GenericInterface
    {
        InterfaceVID(ConstMemory memory)
            : GenericInterface(memory, 0x5800, 10050)
        {
        }

        const char* decodeImage(const Surface& s) override
        {
            m_generic_header.multicolor_load(s, m_data, 0x800, 0x400, 0x0, 0x7e9, 0x0, 1, false);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceVID(ConstMemory memory)
    {
        ImageDecodeInterface* x = new InterfaceVID(memory);
        return x;
    }

    // ------------------------------------------------------------
    // Commodore VIC-20 palette (for Fluff64 VIC-20 export)
    // ------------------------------------------------------------

    const Color g_vic20_palette_colors [16] =
    {
        0xFF000000, 0xFFFFFFFF, 0xFF6D2327, 0xFFA0FEF8,
        0xFF8E3C97, 0xFF7EDA75, 0xFF252390, 0xFFFFFF86,
        0xFFA4643B, 0xFFFFC8A1, 0xFFF2A7AB, 0xFFDBFFFF,
        0xFFFFB4FF, 0xFFD7FFCE, 0xFF9D9AFF, 0xFFFFFFC9,
    };

    struct PaletteVIC20 : Palette
    {
        PaletteVIC20()
        {
            size = 16;
            for (size_t i = 0; i < 16; ++i)
                color[i] = g_vic20_palette_colors[i];
        }
    } g_vic20_palette;

    // C64 character ROM (uppercase/graphics set), 128 glyphs x 8 rows.
    // Used to render PETSCII text-screen images (Fluff64 PET mode).
    const u8 g_c64_font [1024] =
    {
        60, 102, 110, 110, 96, 98, 60, 0, 24, 60, 102, 126, 102, 102, 102, 0,
        124, 102, 102, 124, 102, 102, 124, 0, 60, 102, 96, 96, 96, 102, 60, 0,
        120, 108, 102, 102, 102, 108, 120, 0, 126, 96, 96, 120, 96, 96, 126, 0,
        126, 96, 96, 120, 96, 96, 96, 0, 60, 102, 96, 110, 102, 102, 60, 0,
        102, 102, 102, 126, 102, 102, 102, 0, 60, 24, 24, 24, 24, 24, 60, 0,
        30, 12, 12, 12, 12, 108, 56, 0, 102, 108, 120, 112, 120, 108, 102, 0,
        96, 96, 96, 96, 96, 96, 126, 0, 99, 119, 127, 107, 99, 99, 99, 0,
        102, 118, 126, 126, 110, 102, 102, 0, 60, 102, 102, 102, 102, 102, 60, 0,
        124, 102, 102, 124, 96, 96, 96, 0, 60, 102, 102, 102, 102, 60, 14, 0,
        124, 102, 102, 124, 120, 108, 102, 0, 60, 102, 96, 60, 6, 102, 60, 0,
        126, 24, 24, 24, 24, 24, 24, 0, 102, 102, 102, 102, 102, 102, 60, 0,
        102, 102, 102, 102, 102, 60, 24, 0, 99, 99, 99, 107, 127, 119, 99, 0,
        102, 102, 60, 24, 60, 102, 102, 0, 102, 102, 102, 60, 24, 24, 24, 0,
        126, 6, 12, 24, 48, 96, 126, 0, 60, 48, 48, 48, 48, 48, 60, 0,
        12, 18, 48, 124, 48, 98, 252, 0, 60, 12, 12, 12, 12, 12, 60, 0,
        0, 24, 60, 126, 24, 24, 24, 24, 0, 16, 48, 127, 127, 48, 16, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 24, 24, 24, 24, 0, 0, 24, 0,
        102, 102, 102, 0, 0, 0, 0, 0, 102, 102, 255, 102, 255, 102, 102, 0,
        24, 62, 96, 60, 6, 124, 24, 0, 98, 102, 12, 24, 48, 102, 70, 0,
        60, 102, 60, 56, 103, 102, 63, 0, 6, 12, 24, 0, 0, 0, 0, 0,
        12, 24, 48, 48, 48, 24, 12, 0, 48, 24, 12, 12, 12, 24, 48, 0,
        0, 102, 60, 255, 60, 102, 0, 0, 0, 24, 24, 126, 24, 24, 0, 0,
        0, 0, 0, 0, 0, 24, 24, 48, 0, 0, 0, 126, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 24, 24, 0, 0, 3, 6, 12, 24, 48, 96, 0,
        60, 102, 110, 118, 102, 102, 60, 0, 24, 24, 56, 24, 24, 24, 126, 0,
        60, 102, 6, 12, 48, 96, 126, 0, 60, 102, 6, 28, 6, 102, 60, 0,
        6, 14, 30, 102, 127, 6, 6, 0, 126, 96, 124, 6, 6, 102, 60, 0,
        60, 102, 96, 124, 102, 102, 60, 0, 126, 102, 12, 24, 24, 24, 24, 0,
        60, 102, 102, 60, 102, 102, 60, 0, 60, 102, 102, 62, 6, 102, 60, 0,
        0, 0, 24, 0, 0, 24, 0, 0, 0, 0, 24, 0, 0, 24, 24, 48,
        14, 24, 48, 96, 48, 24, 14, 0, 0, 0, 126, 0, 126, 0, 0, 0,
        112, 24, 12, 6, 12, 24, 112, 0, 60, 102, 6, 12, 24, 0, 24, 0,
        0, 0, 0, 255, 255, 0, 0, 0, 8, 28, 62, 127, 127, 28, 62, 0,
        24, 24, 24, 24, 24, 24, 24, 24, 0, 0, 0, 255, 255, 0, 0, 0,
        0, 0, 255, 255, 0, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 255, 255, 0, 0, 48, 48, 48, 48, 48, 48, 48, 48,
        12, 12, 12, 12, 12, 12, 12, 12, 0, 0, 0, 224, 240, 56, 24, 24,
        24, 24, 28, 15, 7, 0, 0, 0, 24, 24, 56, 240, 224, 0, 0, 0,
        192, 192, 192, 192, 192, 192, 255, 255, 192, 224, 112, 56, 28, 14, 7, 3,
        3, 7, 14, 28, 56, 112, 224, 192, 255, 255, 192, 192, 192, 192, 192, 192,
        255, 255, 3, 3, 3, 3, 3, 3, 0, 60, 126, 126, 126, 126, 60, 0,
        0, 0, 0, 0, 0, 255, 255, 0, 54, 127, 127, 127, 62, 28, 8, 0,
        96, 96, 96, 96, 96, 96, 96, 96, 0, 0, 0, 7, 15, 28, 24, 24,
        195, 231, 126, 60, 60, 126, 231, 195, 0, 60, 126, 102, 102, 126, 60, 0,
        24, 24, 102, 102, 24, 24, 60, 0, 6, 6, 6, 6, 6, 6, 6, 6,
        8, 28, 62, 127, 62, 28, 8, 0, 24, 24, 24, 255, 255, 24, 24, 24,
        192, 192, 48, 48, 192, 192, 48, 48, 24, 24, 24, 24, 24, 24, 24, 24,
        0, 0, 3, 62, 118, 54, 54, 0, 255, 127, 63, 31, 15, 7, 3, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 240, 240, 240, 240, 240, 240, 240, 240,
        0, 0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 255, 192, 192, 192, 192, 192, 192, 192, 192,
        204, 204, 51, 51, 204, 204, 51, 51, 3, 3, 3, 3, 3, 3, 3, 3,
        0, 0, 0, 0, 204, 204, 51, 51, 255, 254, 252, 248, 240, 224, 192, 128,
        3, 3, 3, 3, 3, 3, 3, 3, 24, 24, 24, 31, 31, 24, 24, 24,
        0, 0, 0, 0, 15, 15, 15, 15, 24, 24, 24, 31, 31, 0, 0, 0,
        0, 0, 0, 248, 248, 24, 24, 24, 0, 0, 0, 0, 0, 0, 255, 255,
        0, 0, 0, 31, 31, 24, 24, 24, 24, 24, 24, 255, 255, 0, 0, 0,
        0, 0, 0, 255, 255, 24, 24, 24, 24, 24, 24, 248, 248, 24, 24, 24,
        192, 192, 192, 192, 192, 192, 192, 192, 224, 224, 224, 224, 224, 224, 224, 224,
        7, 7, 7, 7, 7, 7, 7, 7, 255, 255, 0, 0, 0, 0, 0, 0,
        255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255,
        3, 3, 3, 3, 3, 3, 255, 255, 0, 0, 0, 0, 240, 240, 240, 240,
        15, 15, 15, 15, 0, 0, 0, 0, 24, 24, 24, 248, 248, 0, 0, 0,
        240, 240, 240, 240, 0, 0, 0, 0, 240, 240, 240, 240, 15, 15, 15, 15,
    };

    // CGA composite/RGBI palette (used by the Fluff64 PC export).
    const u32 g_cga_palette [16] =
    {
        0x000000, 0x0000aa, 0x00aa00, 0x00aaaa,
        0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
        0x555555, 0x5555ff, 0x55ff55, 0x55ffff,
        0xff5555, 0xff55ff, 0xffff55, 0xffffff,
    };

    // ------------------------------------------------------------
    // ImageDecoder: FLF (Fluff64)
    //
    // Fluff64 is a multi-platform exporter; byte[11] selects the target
    // machine. The Commodore targets are rendered via the C64/VIC-20
    // character generators; the remaining targets are plain chunky
    // (1 byte per pixel) bitmaps with a per-platform palette:
    //
    //     1, 4, 5     C64 multicolor font (12-byte cells: 8 bitmap + 4 colors)
    //     6           C64 hires font
    //     7           C64 PETSCII text screen (rendered via character ROM)
    //     9           VIC-20 multicolor font
    //     0x0b        PC / CGA          320x200  (4 colors)
    //     0x0c        Amiga             320x200  (R8G8B8 palette)
    //     0x0d        Amiga             320x256  (R8G8B8 palette)
    //     0x16        Atari ST          320x200  (R8G8B8 palette)
    //     0x18        Amstrad CPC       320x200  (16 firmware colors, 2x1)
    //     0x1a        BBC Micro         320x256  (mode 1: 1x1 / mode 2: 2x1)
    //     0x1b        PC                320x200  (R8G8B8 palette)
    //     0x1c        ZX Spectrum       256x192  (ULAplus 64-color palette)
    //
    // ------------------------------------------------------------

    struct InterfaceFLF : Interface
    {
        enum Mode { NONE, FONT_C64, FONT_VIC20, PET, CHUNKY } m_mode = NONE;
        int m_offset = 0;
        int m_columns = 0;
        int m_rows = 0;
        int m_colors = 0;
        int m_xmask = 0;
        int m_cmask = 0;
        int m_pet_screen = 0;
        int m_pet_colors = 0;
        int m_pet_background = 0;

        // CHUNKY mode (non-Commodore platforms)
        int m_chunk_offset = 0;
        int m_orig_width = 0;
        int m_orig_height = 0;
        int m_scale_x = 1;
        Palette m_chunk_palette;

        InterfaceFLF(ConstMemory memory)
            : Interface(memory)
        {
            const u8* c = memory.address;
            size_t length = memory.size;

            if (length < 20 || std::memcmp(c, "FLUFF64", 7) != 0)
                return;

            switch (c[11])
            {
                case 1:
                    setupFont(15, 40, 25, 16, 6, 3, false);
                    break;
                case 4:
                case 5:
                    setupFont(18, 40, 25, 16, 6, 3, false);
                    break;
                case 6:
                    setupFont(18, 40, 25, 16, 7, 1, false);
                    break;
                case 7:
                {
                    int columns = c[0x0f];
                    int rows = c[0x10];
                    int count = columns * rows;
                    if (columns <= 0 || rows <= 0 || length < size_t(0x2d + (count << 1)))
                        return;
                    m_mode = PET;
                    m_columns = columns;
                    m_rows = rows;
                    m_pet_screen = 0x1d + count;
                    m_pet_colors = 0x1d;
                    m_pet_background = c[0x0d];
                    setHeader(columns << 3, rows << 3);
                    break;
                }
                case 9:
                    if (c[12] != 6)
                        return;
                    setupFont(20, c[0x12], c[0x13], 8, 6, 3, true);
                    break;

                case 0x0b: // PC / CGA
                {
                    if (length != 64269)
                        return;
                    int sel;
                    switch (c[12])
                    {
                        case 2: sel = 1; break;
                        case 3: sel = 9; break;
                        case 4: sel = 0; break;
                        case 5: sel = 8; break;
                        default: return;
                    }
                    initChunkPalette();
                    setPaletteColor(1, g_cga_palette[sel + 2]);
                    setPaletteColor(2, g_cga_palette[sel + 4]);
                    setPaletteColor(3, g_cga_palette[sel + 6]);
                    setChunky(320, 200, 1, 13);
                    break;
                }

                case 0x0c: // Amiga 320x200
                    if (!setupFlfBytes(200))
                        return;
                    break;

                case 0x0d: // Amiga 320x256
                    if (!setupFlfBytes(256))
                        return;
                    break;

                case 0x16: // Atari ST 320x200
                    if (!setupFlfBytes(200))
                        return;
                    break;

                case 0x18: // Amstrad CPC
                    if (c[12] != 0x0b || length != 32269)
                        return;
                    initChunkPalette();
                    if (!setAmstradFirmwarePalette(0x7dcd, 16))
                        return;
                    setChunky(320, 200, 2, 13);
                    break;

                case 0x1a: // BBC Micro
                    if (c[12] != 0x0c)
                        return;
                    initChunkPalette();
                    switch (c[13])
                    {
                        case 4:
                            if (length != 82190)
                                return;
                            setPaletteColor(1, 0xffffff);
                            setChunky(320, 256, 1, 13);
                            break;
                        case 5:
                        {
                            if (length != 41230)
                                return;
                            static const u32 bbc2bit [4] =
                            {
                                0x000000, 0xff0000, 0xffff00, 0xffffff
                            };
                            for (int i = 0; i < 4; ++i)
                                setPaletteColor(i, bbc2bit[i]);
                            setChunky(320, 256, 2, 13);
                            break;
                        }
                        default:
                            return;
                    }
                    break;

                case 0x1b: // PC
                    if (!setupFlfBytes(200))
                        return;
                    break;

                case 0x1c: // ZX Spectrum
                    if (c[12] != 0x0e || length < 49165)
                        return;
                    setZxPalette();
                    setChunky(256, 192, 1, 13);
                    break;

                default:
                    // unsupported platform
                    break;
            }
        }

        void initChunkPalette()
        {
            m_chunk_palette.size = 256;
            for (int i = 0; i < 256; ++i)
                m_chunk_palette.color[i] = Color(0, 0, 0, 0xff);
        }

        void setPaletteColor(int index, u32 rgb)
        {
            m_chunk_palette.color[index] = Color((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, 0xff);
        }

        void setChunky(int width, int height, int scaleX, int offset)
        {
            m_mode = CHUNKY;
            m_orig_width = width / scaleX;
            m_orig_height = height;
            m_scale_x = scaleX;
            m_chunk_offset = offset;
            setHeader(width, height);
        }

        // Amiga / Atari ST / PC: chunky bitmap with an R8G8B8 palette stored
        // after the pixels
        bool setupFlfBytes(int height)
        {
            const u8* c = m_memory.address;
            int length = int(m_memory.size);

            int colorsOffset = 14 + 320 * height;
            if (length < colorsOffset + 6)
                return false;

            int colors = c[colorsOffset - 1];
            if (colors == 0)
                colors = 256;

            int trailing = length - colorsOffset - colors * 3;
            if (trailing != 0 && trailing != 256)
                return false;

            initChunkPalette();
            for (int i = 0; i < colors; ++i)
            {
                const u8* p = c + colorsOffset + i * 3;
                m_chunk_palette.color[i] = Color(p[0], p[1], p[2], 0xff);
            }

            setChunky(320, height, 1, 13);
            return true;
        }

        bool setAmstradFirmwarePalette(int offset, int count)
        {
            const u8* c = m_memory.address;
            if (size_t(offset + count) > m_memory.size)
                return false;

            const u8 triLevel [3] = { 0, 0x80, 0xff };
            for (int i = 0; i < count; ++i)
            {
                int v = c[offset + i];
                if (v > 26)
                    return false;
                m_chunk_palette.color[i] = Color(triLevel[(v / 3) % 3], triLevel[v / 9], triLevel[v % 3], 0xff);
            }
            return true;
        }

        // ZX Spectrum: 8 normal + 8 bright colors laid out as a 64-entryULAplus palette
        void setZxPalette()
        {
            m_chunk_palette.size = 256;
            for (int i = 0; i < 64; ++i)
            {
                u32 rgb = ((i >> 1) & 1) * 0xff0000 | ((i >> 2) & 1) * 0x00ff00 | (i & 1) * 0x0000ff;
                if ((i & 0x10) == 0)
                    rgb &= 0xcdcdcd; // not bright
                m_chunk_palette.color[i] = Color((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, 0xff);
            }
            for (int i = 64; i < 256; ++i)
                m_chunk_palette.color[i] = Color(0, 0, 0, 0xff);
        }

        void setHeader(int width, int height)
        {
            header.width = width;
            header.height = height;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        void setupFont(int offset, int columns, int rows, int colors, int xmask, int cmask, bool vic20)
        {
            if (columns <= 0 || rows <= 0)
                return;
            if (m_memory.size != size_t(offset + columns * rows * 12))
                return;
            m_mode = vic20 ? FONT_VIC20 : FONT_C64;
            m_offset = offset;
            m_columns = columns;
            m_rows = rows;
            m_colors = colors;
            m_xmask = xmask;
            m_cmask = cmask;
            setHeader(columns << 3, rows << 3);
        }

        const char* decodeImage(const Surface& s) override
        {
            const u8* content = m_memory.address;
            const int width = header.width;
            const int height = header.height;

            Buffer temp(width * height, 0);
            u8* image = temp;

            if (m_mode == CHUNKY)
            {
                const int ow = m_orig_width;
                const int oh = m_orig_height;
                const int sx = m_scale_x;

                if (size_t(m_chunk_offset + ow * oh) > m_memory.size)
                    return "[ImageDecoder.C64] Truncated Fluff64 image.";

                for (int y = 0; y < oh; ++y)
                for (int x = 0; x < ow; ++x)
                {
                    u8 index = content[m_chunk_offset + y * ow + x];
                    for (int k = 0; k < sx; ++k)
                        image[y * width + x * sx + k] = index;
                }

                Surface indices(width, height, IndexedFormat(8), width, temp);
                indices.palette = &m_chunk_palette;
                resolve(s, indices);
                return nullptr;
            }

            if (m_mode == PET)
            {
                for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    int offset = (y >> 3) * m_columns + (x >> 3);
                    int ch = content[m_pet_screen + offset];
                    int pixel = (g_c64_font[((ch & 0x7f) << 3) + (y & 7)] >> (~x & 7) ^ (ch >> 7)) & 1;
                    int color = pixel == 0 ? m_pet_background : content[m_pet_colors + offset];
                    image[y * width + x] = u8(color & 0xf);
                }
            }
            else
            {
                for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    int offset = m_offset + ((y >> 3) * m_columns + (x >> 3)) * 12;
                    int c = content[offset + (y & 7)] >> (x & m_xmask) & m_cmask;
                    c = content[offset + 8 + c];
                    if (c >= m_colors)
                        return "[ImageDecoder.C64] Invalid Fluff64 color index.";
                    image[y * width + x] = u8(c);
                }
            }

            Surface indices(width, height, IndexedFormat(8), width, temp);
            indices.palette = (m_mode == FONT_VIC20) ? (Palette*)&g_vic20_palette : (Palette*)&g_c64_palette;
            resolve(s, indices);
            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceFLF(ConstMemory memory)
    {
        return new InterfaceFLF(memory);
    }

    // ------------------------------------------------------------
    // ImageDecoder: VIC (length-keyed Commodore container)
    //
    // The .VIC extension is an alias that selects an underlying C64 picture
    // format purely by file length:
    //
    //     9002/9003/9009   hires            (IPH/ART)
    //     10018            multicolor       (OCP Advanced Art Studio)
    //     10241            multicolor       (Dolphin Ed)
    //     10242            multicolor       (Blazing Paddles)
    //     17218/17409      multicolor FLI   (FLI Designer / FD2)
    //     17410            multicolor FLI   (FLI w/ background)
    //     17474/17665/.6   multicolor FLI   (FLI bars / BML)
    //     18242            interlaced       (DrazLace)
    //     33602/33603      interlaced       (Gunpaint)
    //     33694            interlaced       (FunPaint, optional RLE)
    //
    // All offsets are relative to the data area (after the 2-byte load
    // address) and reuse the shared C64 render helpers.
    //
    // ------------------------------------------------------------

    struct InterfaceVIC : Interface
    {
        enum Mode { NONE, HIRES, MULTI, FLI, BARS, DRL, GUN, FUN } m_mode = NONE;

        // multicolor / hires render parameters (relative to m_data)
        u32 m_bitmap = 0;
        u32 m_vm = 0;
        u32 m_color = 0;
        u32 m_bg = 0;
        int m_bgmode = 0;
        bool m_fli = false;

        // FunPaint compression
        bool m_fun_compressed = false;
        u8 m_fun_escape = 0;

        const u8* m_data = nullptr;

        InterfaceVIC(ConstMemory memory)
            : Interface(memory)
        {
            if (memory.size < 3)
                return;

            m_data = memory.address + 2; // skip 2-byte load address
            size_t size = memory.size;

            switch (size)
            {
                case 9002:
                case 9003:
                case 9009:
                    m_mode = HIRES;
                    m_bitmap = 0x0; m_vm = 0x1f40; m_fli = false;
                    break;
                case 10018: // OCP
                    setMulti(0x0, 0x1f40, 0x2338, 0x2329, 1, false);
                    break;
                case 10241: // DOL
                    setMulti(0x800, 0x400, 0x0, 0x7e8, 1, false);
                    break;
                case 10242: // BPL
                    setMulti(0x0, 0x2000, 0x2400, 0x1f80, 1, false);
                    break;
                case 17218: // FD2
                case 17409: // FLI
                    setMulti(0x2400, 0x400, 0x0, 0x0, 0, true);
                    break;
                case 17410: // FLM
                    setMulti(0x2400, 0x400, 0x0, 0x437f, 1, true);
                    break;
                case 17474:
                case 17665:
                case 17666: // BML
                    setMulti(0x2500, 0x500, 0x100, 0x0, 2, true);
                    m_mode = BARS;
                    break;
                case 18242: // DrazLace (raw)
                    m_mode = DRL;
                    break;
                case 33602:
                case 33603: // Gunpaint
                    m_mode = GUN;
                    break;
                case 33694: // FunPaint
                    if (!setupFun())
                        return;
                    break;
                default:
                    return;
            }

            header.width = 320;
            header.height = 200;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }

        void setMulti(u32 bitmap, u32 vm, u32 color, u32 bg, int bgmode, bool fli)
        {
            m_mode = MULTI;
            m_bitmap = bitmap;
            m_vm = vm;
            m_color = color;
            m_bg = bg;
            m_bgmode = bgmode;
            m_fli = fli;
        }

        bool setupFun()
        {
            const u8* p = m_memory.address;
            u16 load_address = p[0] | (p[1] << 8);
            if (load_address != 0x3ff0 || m_memory.size <= 18)
                return false;
            if (std::memcmp(p + 2, "FUNPAINT (MT) ", 14) != 0)
                return false;
            m_fun_compressed = p[16] != 0;
            m_fun_escape = p[17];
            m_data = p + 18;
            m_mode = FUN;
            return true;
        }

        const char* decodeImage(const Surface& s) override
        {
            const int width = header.width;
            const int height = header.height;
            const u8* end = m_memory.end();

            switch (m_mode)
            {
                case HIRES:
                    hires_to_surface(s, m_data, width, height, m_bitmap, m_vm, m_fli, false, 0);
                    break;

                case MULTI:
                case BARS:
                    multicolor_to_surface(s, m_data, width, height, m_bitmap, m_vm, m_color, m_bg, 0x0, m_bgmode, m_fli);
                    break;

                case DRL:
                {
                    Buffer background(200, *(m_data + 0x2740));
                    multicolor_interlace_to_surface(s, m_data, width, height, 0x800, 0x2800, 0x400, 0x400, 0x0, background, 0x0, 2, false, 2);
                    break;
                }

                case GUN:
                {
                    Buffer background(200);
                    std::memcpy(background.data() +   0, m_data + 0x3f4f, 177);
                    std::memcpy(background.data() + 177, m_data + 0x47e8, 20);
                    background[197] = background[198] = background[199] = background[196];
                    multicolor_interlace_to_surface(s, m_data, width, height, 0x2000, 0x6400, 0x0, 0x4400, 0x4000, background.data(), 0x0, 2, true, 2);
                    break;
                }

                case FUN:
                {
                    Buffer temp;
                    const u8* buffer = m_data;
                    if (m_fun_compressed)
                    {
                        temp.reset(33678);
                        depack_fun(temp, m_data, 33678, end, m_fun_escape);
                        buffer = temp;
                    }
                    Buffer background(200);
                    std::memcpy(background.data() +   0, buffer + 0x3f48, 100);
                    std::memcpy(background.data() + 100, buffer + 0x8328, 100);
                    multicolor_interlace_to_surface(s, buffer, width, height, 0x2000, 0x63e8, 0x0, 0x43e8, 0x4000, background.data(), 0x0, 2, true, 2);
                    break;
                }

                default:
                    return "[ImageDecoder.C64] Unsupported .VIC image.";
            }

            return nullptr;
        }
    };

    ImageDecodeInterface* createInterfaceVIC(ConstMemory memory)
    {
        return new InterfaceVIC(memory);
    }

} // namespace

namespace mango::image
{

    void registerImageCodecC64()
    {
        // Advanced Art Studio
        registerImageDecoder(createInterfaceMPIC, ".mpic");

        // AFLI-editor v2.0
        registerImageDecoder(createInterfaceAFL, ".afl");
        registerImageDecoder(createInterfaceAFL, ".afli");

        // Amica Paint
        registerImageDecoder(createInterfaceAMI, ".ami");

        // Art Studio
        registerImageDecoder(createInterfaceART, ".art");
        registerImageDecoder(createInterfaceART, ".ocp");

        // Artist 64
        registerImageDecoder(createInterfaceA64, ".a64");

        // Blazing Paddles
        registerImageDecoder(createInterfaceBLP, ".blp");
        registerImageDecoder(createInterfaceBLP, ".bpi");
        registerImageDecoder(createInterfaceBLP, ".pi");

        // CDU-Paint
        registerImageDecoder(createInterfaceCDU, ".cdu");

        // Dolphin Ed
        registerImageDecoder(createInterfaceDOL, ".dol");

        // Doodle
        registerImageDecoder(createInterfaceDD, ".dd");
        registerImageDecoder(createInterfaceDD, ".ddl");
        //registerImageDecoder(createInterfaceDD, "jj");

        // Drazlace
        registerImageDecoder(createInterfaceDRL, ".drl");
        registerImageDecoder(createInterfaceDRL, ".dlp");

        // Drazpaint
        registerImageDecoder(createInterfaceDRZ, ".drz");
        registerImageDecoder(createInterfaceDRZ, ".dp64");
        registerImageDecoder(createInterfaceDRZ, ".drp");
        registerImageDecoder(createInterfaceDRZ, ".dp");

        // ECI Graphic Editor v1.0
        registerImageDecoder(createInterfaceECI, ".eci");

        // Face Painter
        registerImageDecoder(createInterfaceFPT, ".fpt");
        registerImageDecoder(createInterfaceFPT, ".fcp");

        // FLI Designer 1.1 & 2.0 (FBI Crew)
        registerImageDecoder(createInterfaceFD2, ".fd2");

        // FLI-Profi
        registerImageDecoder(createInterfaceFPR, ".fpr");

        // Funpaint 2
        registerImageDecoder(createInterfaceFUN, ".fun");
        registerImageDecoder(createInterfaceFUN, ".fp2");

        // Gunpaint
        registerImageDecoder(createInterfaceGUN, ".gun");
        registerImageDecoder(createInterfaceGUN, ".ifl");

        // HCB-Editor v0.05
        registerImageDecoder(createInterfaceHCB, ".hcb");

        // Hires FLI Designer
        registerImageDecoder(createInterfaceHFC, ".hfc");

        // Hires Manager
        registerImageDecoder(createInterfaceHIM, ".him");

        // Koala Painter II
        registerImageDecoder(createInterfaceKOA, ".koa");
        registerImageDecoder(createInterfaceKOA, ".kla");

        // Paint Magic
        registerImageDecoder(createInterfacePMG, ".pmg");

        // Pixel Perfect
        registerImageDecoder(createInterfacePP, ".pp");
        registerImageDecoder(createInterfacePP, ".ppp");

        // Run paint
        registerImageDecoder(createInterfaceRPM, ".rpm");

        // Saracen Paint
        registerImageDecoder(createInterfaceSAR, ".sar");

        // SHF-Editor v1.0
        registerImageDecoder(createInterfaceSHF, ".unp");
        registerImageDecoder(createInterfaceSHF, ".shfli");

        // SHF-XL v1.0
        registerImageDecoder(createInterfaceSHFXL, ".shx");
        registerImageDecoder(createInterfaceSHFXL, ".shfxl");

        // True Paint
        registerImageDecoder(createInterfaceMCI, ".mci");
        registerImageDecoder(createInterfaceMCI, ".mcp");

        // UFLI-Editor v1.0 & v2.0
        registerImageDecoder(createInterfaceUFLI, ".ufup");
        registerImageDecoder(createInterfaceUFLI, ".ufli");

        // UIFLI Editor v1.0
        registerImageDecoder(createInterfaceUIFLI, ".uifli");

        // Vidcom 64
        registerImageDecoder(createInterfaceVID, ".vid");

        // Fluff64 (Commodore variants)
        registerImageDecoder(createInterfaceFLF, ".flf");

        // VIC (length-keyed Commodore container)
        registerImageDecoder(createInterfaceVIC, ".vic");
    }

} // namespace mango::image
