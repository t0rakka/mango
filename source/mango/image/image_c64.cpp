/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
/*
    Commodore 64 decoders copyright (C) 2011 Toni Lönnberg. All rights reserved.
*/

#include <cmath>
#include <mango/core/pointer.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // Commodore 64 utilities
    // ------------------------------------------------------------

    constexpr int g_c64_palette_size = 16;

    const Color g_c64_palette[] =
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
            for (int i = 0; i < g_c64_palette_size; ++i)
            {
                color[i] = g_c64_palette[i];
            }
        }
    };

    void resolve_palette(const Surface& s, u8* image, int width, int height, const Palette& palette)
    {
        for (int y = 0; y < height; ++y)
        {
            Color* scan = s.address<Color>(0, y);

            for (int x = 0; x < width; ++x)
            {
                u8 index = *image++;
                scan[x] = palette[index];
            }
        }
    }

    void rle_ecb(u8* buffer, const u8* input, int scansize, const u8* input_end, u8 escape_char)
    {
        u8* buffer_end = buffer + scansize;

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

    void multicolor_to_surface(const Surface& s, const u8 *data, int width, int height, 
                               u32 bitmap_offset, u32 video_ram_offset, u32 color_ram_offset, 
                               u32 background_offset, u32 opcode_colors_offset, 
                               int background_mode, bool fli)
    {
        PaletteC64 palette;
        Buffer temp(width * height, 0);

        convert_multicolor_bitmap(width, height, temp, 
                                  data + bitmap_offset, data + video_ram_offset, 
                                  data + color_ram_offset, data + background_offset, data + opcode_colors_offset,
                                  background_mode, fli);

        resolve_palette(s, temp, width, height, palette);
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
        PaletteC64 palette;
        Buffer temp(width * height, 0);

        convert_hires_bitmap(width, height, temp, data + bitmap_offset, data + video_ram_offset, fli, show_fli_bug, fli_bug_color);

        resolve_palette(s, temp, width, height, palette);
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

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;

        Interface(ConstMemory memory)
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

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(options);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            status.direct = dest.format == m_header.format &&
                            dest.width >= m_header.width &&
                            dest.height >= m_header.height;

            if (status.direct)
            {
                decodeImage(dest);
            }
            else
            {
                Bitmap temp(m_header.width, m_header.height, m_header.format);
                decodeImage(temp);
                dest.blit(0, 0, temp);
            }

            status.success = true;
            return status;
        }

        virtual void decodeImage(const Surface& dest) = 0;
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
                m_header.width  = m_generic_header.width;
                m_header.height = m_generic_header.height;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x0, 0x1f40, 0x2338, 0x2329, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceMPIC(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceMPIC(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            hires_to_surface(s, m_data, m_header.width, m_header.height, 0x2000, 0x0, true, false, 0);
        }
    };

    ImageDecoderInterface* createInterfaceAFL(ConstMemory memory)
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
        const u8* m_data;

        InterfaceAMI(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            // read header
            const u8* end = memory.address + memory.size;

            if (end[-1] == 0x0 && end[-2] == 0xc2)
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_generic_header.compressed = true;
                m_generic_header.escape_char = 0xc2;

                m_data = memory.address + 2;
            }
        }
            
        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;

            Buffer temp;
            const u8* buffer = m_data;

            if (m_generic_header.compressed)
            {
                temp.reset(10513);
                rle_ecb(temp, m_data, 10513, end, m_generic_header.escape_char);
                buffer = temp;
            }

            multicolor_to_surface(s, buffer, m_header.width, m_header.height, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, false, false);
        }
    };

    ImageDecoderInterface* createInterfaceAMI(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceAMI(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.hires_load(s, m_data, 0x0, 0x1f40, false);
        }
    };

    ImageDecoderInterface* createInterfaceART(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceART(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x0, 0x2000, 0x2400, 0x27ff, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceA64(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceA64(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

        m_generic_header.multicolor_load(s, m_data, 0x0, 0x2000, 0x2400, 0x1f80, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceBLP(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceBLP(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x111, 0x2051, 0x2439, 0x2821, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceCDU(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceCDU(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x800, 0x400, 0x0, 0x7e8, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceDOL(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceDOL(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.hires_load(s, m_data, 0x400, 0x0, false);
        }
    };

    ImageDecoderInterface* createInterfaceDD(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceDD(memory);
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
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x5800, 18242, load_address, memory.size))
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else
            {
                if (load_address == 0x5800)
                {
                    u8 keyword[] = "DRAZLACE! 1.0";
                    if (std::memcmp(keyword, p, sizeof(keyword) - 1) == 0)
                    {
                        p += sizeof(keyword) - 1;

                        m_header.width = 320;
                        m_header.height = 200;
                        m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                        m_compressed = true;
                        m_escape_char = p.read8();
                        m_data = p;
                    }
                }
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(18240);
                rle_ecb(temp, m_data, 18240, end, m_escape_char);
                buffer = temp;
            }

            Buffer background(200, *(buffer + 0x2740));
            multicolor_interlace_to_surface(s, buffer, m_header.width, m_header.height, 0x800, 0x2800, 0x400, 0x400, 0x0, background, 0x0, 2, false, 2);
        }
    };

    ImageDecoderInterface* createInterfaceDRL(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceDRL(memory);
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
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x5800, 10051, load_address, memory.size))
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else
            {
                if (load_address == 0x5800)
                {
                    u8 keyword[] = "DRAZPAINT 2.0";
                    if (std::memcmp(keyword, p, sizeof(keyword) - 1) == 0)
                    {
                        p += sizeof(keyword) - 1;

                        m_header.width = 320;
                        m_header.height = 200;
                        m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                        m_compressed = true;
                        m_escape_char = p.read8();
                        m_data = p;
                    }
                }
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(10049);
                rle_ecb(temp, m_data, 10049, end, m_escape_char);
                buffer = temp;
            }

            multicolor_to_surface(s, buffer, m_header.width, m_header.height, 0x800, 0x400, 0x0, 0x2740, 0x0, false, false);
        }
    };

    ImageDecoderInterface* createInterfaceDRZ(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceDRZ(memory);
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
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 32770, load_address, memory.size))
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else
            {
                if (load_address == 0x4000)
                {
                    m_header.width = 320;
                    m_header.height = 200;
                    m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                    m_compressed = true;
                    m_escape_char = p.read8();
                    m_data = p;
                }
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(32768);
                rle_ecb(temp, m_data, 32768, end, m_escape_char);
                buffer = temp;
            }

            hires_interlace_to_surface(s, buffer, m_header.width, m_header.height, 0x0, 0x4000, 0x2000, 0x6000, true, false, 0);
        }
    };

    ImageDecoderInterface* createInterfaceECI(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceECI(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x0, 0x1f40, 0x2328, 0x2712, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceFPT(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceFPT(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x2400, 0x400, 0x0, 0x0, 0x0, 0, true);
        }
    };

    ImageDecoderInterface* createInterfaceFD2(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceFD2(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            u8 sprite_color1 = m_data[0x448];
            u8 sprite_color2 = m_data[0x449];
            const u8 *sprite_colors = m_data + 0x280;

            PaletteC64 palette;

            Buffer temp(m_header.width * m_header.height, 0);
            u8* image = temp;

            convert_multicolor_bitmap(m_header.width, m_header.height, image, m_data + 0x2880, m_data + 0x880, m_data + 0x480, NULL, m_data + 0x380, 0, true);

            // Overlay sprite data
            // - Y-expanded
            // - Switching VIC bank every two scanlines, pattern: 1221
            for (int y = 0; y < 200; ++y)
            {
                for (int x = 0; x < 24; ++x)
                {
                    int offset = x + y * m_header.width;
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

            resolve_palette(s, temp, m_header.width, m_header.height, palette);
        }
    };

    ImageDecoderInterface* createInterfaceFPR(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceFPR(memory);
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
                            m_header.width = 320;
                            m_header.height = 200;
                        }
                    }
                    else
                    {
                        m_header.width = 320;
                        m_header.height = 200;
                    }

                    m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    m_data = p;
                }
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;

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

            multicolor_interlace_to_surface(s, buffer, m_header.width, m_header.height, 0x2000, 0x63e8, 0x0, 0x43e8, 0x4000, background.data(), 0x0, 2, true, 2);
        }
    };

    ImageDecoderInterface* createInterfaceFUN(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceFUN(memory);
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
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 33603, load_address, memory.size))
            {
                u8 keyword[] = "GUNPAINT (JZ)   ";
                if (std::memcmp(keyword, memory.address + 0x3ea, 16) == 0)
                {
                    m_header.width = 320;
                    m_header.height = 200;
                    m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                    m_data = p;
                }
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            Buffer background(200);
            std::memcpy(background.data() +   0, m_data + 0x3f4f, 177);
            std::memcpy(background.data() + 177, m_data + 0x47e8, 20);
            background[197] = background[198] = background[199] = background[196];  // replicate the last color four times

            multicolor_interlace_to_surface(s, m_data, m_header.width, m_header.height, 0x2000, 0x6400, 0x0, 0x4400, 0x4000, background.data(), 0x0, 2, true, 2);
        }
    };

    ImageDecoderInterface* createInterfaceGUN(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceGUN(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* bitmap_c64 = m_data + 0x1000;
            const u8* video_ram = m_data + 0x800;
            const u8* color_ram = m_data;
            const u8* background = m_data + 0x2f40;

            PaletteC64 palette;

            Buffer temp(m_header.width * m_header.height, 0);
            u8* image = temp;

            for (int y = 0; y < m_header.height; ++y)
            {
                for (int x = 0; x < m_header.width; ++x)
                {
                    int x_offset = x & 0x7;
                    int y_offset = (y >> 2) & 0x1;
                    int bitmap_offset = (x & 0xfffffff8) + (y & 0x7) + ((y >> 3) * (40 * 8));
                    int screen_offset = bitmap_offset >> 3;
                    int offset = x + y * m_header.width;

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

            resolve_palette(s, temp, m_header.width, m_header.height, palette);
        }
    };

    ImageDecoderInterface* createInterfaceHCB(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceHCB(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            hires_to_surface(s, m_data, m_header.width, m_header.height, 0x0, 0x2000, true, false, 0);
        }
    };

    ImageDecoderInterface* createInterfaceHFC(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceHFC(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: HIM (Hires Manager)
    // ------------------------------------------------------------

    void depack_him(u8* buffer, const u8* input, int scansize, int insize, u8 escape_char)
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
                    // TODO: "Hires Manager: unpacked size does not match file format."
                    return;
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
                    // TODO: "Hires Manager: unpacked size does not match file format."
                    return;
                }

                for (int i = 0; i < n; ++i)
                {
                    *out-- = *in--;
                }
            }
        }
    }

    struct InterfaceHIM : Interface
    {
        const u8* m_data;
        bool m_compressed = false;

        InterfaceHIM(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 16385, load_address, memory.size))
            {
                if (*p == 0xff)
                {
                    m_header.width = 320;
                    m_header.height = 192;
                    m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                    m_compressed = false;
                    m_data = p;
                }
            }
            else if (load_address == 0x4000)
            {
                m_header.width = 320;
                m_header.height = 192;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = true;
                m_data = p;
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(16383);
                depack_him(temp, m_data, 16383, int(m_memory.size - 2), 0);
                buffer = temp;
            }

            hires_to_surface(s, buffer, m_header.width, m_header.height, 0x140, 0x2028, true, false, 0);
        }
    };

    ImageDecoderInterface* createInterfaceHIM(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceHIM(memory);
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
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x6000, 10003, load_address, memory.size))
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                m_data = p;
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            multicolor_to_surface(s, m_data, m_header.width, m_header.height, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, false, false);
        }
    };

    ImageDecoderInterface* createInterfaceKOA(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceKOA(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            PaletteC64 palette;
            Buffer temp(m_header.width * m_header.height, 0);

            Buffer color_ram(1000, *(m_data + 0x1fb5));

            convert_multicolor_bitmap(m_header.width, m_header.height, temp.data(), 
                                      m_data + 0x72, m_data + 0x2072, 
                                      color_ram, m_data + 0x1fb2, NULL,
                                      1, false);

            resolve_palette(s, temp, m_header.width, m_header.height, palette);
        }
    };

    ImageDecoderInterface* createInterfacePMG(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfacePMG(memory);
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

    const u8* read_header_pp(header_generic& header, const u8* data, size_t size)
    {
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
                m_header.width  = m_generic_header.width;
                m_header.height = m_generic_header.height;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* end = m_memory.address + m_memory.size;

            Buffer temp;
            const u8* buffer = m_data;

            if (m_generic_header.compressed)
            {
                temp.reset(33600);
                depack_ppp(temp, m_data, 33600, end, m_generic_header.escape_char);
                buffer = temp;
            }

            Buffer background(200, *(buffer + 0x437f));
            multicolor_interlace_to_surface(s, buffer, m_header.width, m_header.height, 0x2400, 0x6400, 0x400, 0x4400, 0x0, background, nullptr, 1, true, 2);
        }
    };

    ImageDecoderInterface* createInterfacePP(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfacePP(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x0, 0x1f40, 0x2328, 0x2710, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceRPM(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceRPM(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x400, 0x0, 0x2400, 0x3f0, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceSAR(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceSAR(memory);
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
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 15874, load_address, memory.size))
            {
                m_header.width = 96;
                m_header.height = 167;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else if (load_address == 0xa000)
            {
                m_header.width = 96;
                m_header.height = 167;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = true;
                m_escape_char = p.read8();
                m_data = p;
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* buffer = m_data;

            const u8* bitmap_c64 = buffer + 0x2000;
            const u8* video_ram = buffer;
            u8 sprite_color1 = *(buffer + 0x3e8);
            u8 sprite_color2 = *(buffer + 0x3e9);

            PaletteC64 palette;

            Buffer tempImage(m_header.width * m_header.height, 0);
            u8* image = tempImage;

            for (int y = 0; y < m_header.height; ++y)
            {
                for (int x = 0; x < m_header.width; ++x)
                {
                    int offset = x + y * m_header.width;
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
                        // TODO: "SHF: invalid sprite pointer."
                        return;
                    }
                    */

                    int sprite_byte_offset1 = (sprite_pointer1 * 64) + (sprite_line * 3) + (x % 24) / 8;
                    u8 sprite_byte1 = buffer[sprite_byte_offset1];
                    int sprite_bit_pattern1 = (sprite_byte1 >> (7 - (x & 0x7))) & 0x1;

                    u8 sprite_pointer2 = buffer[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb + 4];
                    /*
                    if (sprite_pointer2 > 15872)
                    {
                        // TODO: "SHF: invalid sprite pointer."
                        return;
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

            resolve_palette(s, tempImage, m_header.width, m_header.height, palette);
        }
    };

    ImageDecoderInterface* createInterfaceSHF(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceSHF(memory);
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
                m_header.width = 144;
                m_header.height = 168;
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            const u8* bitmap_c64 = m_data + 0x2000;
            const u8* video_ram = m_data;
            u8 sprite_color = *(m_data + 0x3e9);

            PaletteC64 palette;

            Buffer tempImage(m_header.width * m_header.height, 0);
            u8* image = tempImage;

            for (int y = 0; y < m_header.height; ++y)
            {
                for (int x = 0; x < m_header.width; ++x)
                {
                    int offset = x + y * m_header.width;
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
                    u8 sprite_pointer = m_data[sprite_ram_bank * 0x400 + 0x3f8 + sprite_nb];
                    int sprite_byte_offset = (sprite_pointer * 64) + (sprite_line * 3) + (x % 24) / 8;

                    if (sprite_byte_offset > 15360)
                    {
                        // TODO: "SHF-XL: invalid sprite pointer."
                    }

                    u8 sprite_byte = m_data[sprite_byte_offset];
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

            resolve_palette(s, tempImage, m_header.width, m_header.height, palette);
        }
    };

    ImageDecoderInterface* createInterfaceSHFXL(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceSHFXL(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: MCI (True Paint)
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
                    // TODO: "True Paint: unpacked size does not match file format."
                    return;
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
                    // TODO: "True Paint: unpacked size does not match file format."
                    return;
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
                    // TODO: "True Paint: unpacked size does not match file format."
                    return;
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
                    // TODO: "True Paint: unpacked size does not match file format."
                    return;
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
                    // TODO: "True Paint: unpacked size does not match file format."
                    return;
                }

                *out-- = *(input + 0x88);
                *out-- = *(input + 0x88);
            }
            else if (v == *(input + 0x85))
            {
                // 2nd 2-character run
                if (out - 2 < out_end)
                {
                    // TODO: "True Paint: unpacked size does not match file format."
                    return;
                }

                *out-- = *(input + 0x89);
                *out-- = *(input + 0x89);
            }
            else if (v == *(input + 0x86))
            {
                // 3rd 2-character run
                if (out - 2 < out_end)
                {
                    // TODO: "True Paint: unpacked size does not match file format."
                    return;
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

    struct InterfaceMCI : Interface
    {
        const u8* m_data;
        bool m_compressed = false;

        InterfaceMCI(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x9c00, 19434, load_address, memory.size))
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else if (load_address == 0x0801)
            {
                u8 keyword[] = "2059";
                if (std::memcmp(keyword, p + 5, sizeof(keyword) - 1) == 0)
                {
                    m_header.width = 320;
                    m_header.height = 200;
                    m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                    m_compressed = true;
                    m_data = p;
                }
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(19432);
                depack_mci(temp, m_data, 19432, int(m_memory.size - 2));
                buffer = temp;
            }

            Buffer background(200, *(buffer + 0x3e8));
            multicolor_interlace_to_surface(s, buffer, m_header.width, m_header.height, 0x400, 0x2400, 0x0, 0x4400, 0x4800, background, 0x0, 2, false, 2);
        }
    };

    ImageDecoderInterface* createInterfaceMCI(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceMCI(memory);
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
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (check_format(0x4000, 16194, load_address, memory.size))
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = false;
                m_data = p;
            }
            else if (load_address == 0x8000)
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = true;
                m_escape_char = p.read8();
                m_data = p;
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            Buffer temp;
            const u8* buffer = m_data;

            if (m_compressed)
            {
                temp.reset(16192);
                const u8* end = m_memory.address + m_memory.size;
                rle_ecb(temp, m_data, 16192, end, m_escape_char);
                buffer = temp;
            }

            const u8* bitmap_c64 = buffer + 0x2000;
            const u8* video_ram = buffer + 0x1000;
            const u8* sprite_colors = buffer + 0xff0;
            const u8 background_color = *(buffer + 0xff1);
            bool ufli2 = *(buffer + 0xfef) ? true : false;

            PaletteC64 palette;

            Buffer tempImage(m_header.width * m_header.height, 0);
            u8* image = tempImage;

            for (int y = 0; y < m_header.height; ++y)
            {
                for (int x = 0; x < m_header.width; ++x)
                {
                    int offset = x + y * m_header.width;
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

            resolve_palette(s, tempImage, m_header.width, m_header.height, palette);
        }
    };

    ImageDecoderInterface* createInterfaceUFLI(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceUFLI(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageDecoder: UIFLI (UIFLI Editor v1.0)
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

    struct InterfaceUIFLI : Interface
    {
        const u8* m_data;
        bool m_compressed = false;
        u8 m_escape_char = 0;

        InterfaceUIFLI(ConstMemory memory)
            : Interface(memory)
            , m_data(nullptr)
        {
            LittleEndianConstPointer p = memory.address;

            u16 load_address = p.read16();
            if (load_address == 0x4000)
            {
                m_header.width = 320;
                m_header.height = 200;
                m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                m_compressed = true;
                m_escape_char = p.read8();
                m_data = p;
            }
        }

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

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

            for (int y = 0; y < m_header.height; ++y)
            {
                Color* image = s.address<Color>(0, y);

                for (int x = 0; x < m_header.width; ++x)
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

                    Color color0 = g_c64_palette[index[0]]; 
                    Color color1 = g_c64_palette[index[1]]; 

                    image[x].r = (color0.r >> 1) + (color1.r >> 1);
                    image[x].g = (color0.g >> 1) + (color1.g >> 1);
                    image[x].b = (color0.b >> 1) + (color1.b >> 1);
                    image[x].a = 0xff;
                }
            }
        }
    };

    ImageDecoderInterface* createInterfaceUIFLI(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceUIFLI(memory);
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

        void decodeImage(const Surface& s) override
        {
            if (!m_data)
                return;

            m_generic_header.multicolor_load(s, m_data, 0x800, 0x400, 0x0, 0x7e9, 0x0, 1, false);
        }
    };

    ImageDecoderInterface* createInterfaceVID(ConstMemory memory)
    {
        ImageDecoderInterface* x = new InterfaceVID(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageDecoderC64()
    {
        // Advanced Art Studio
        registerImageDecoder(createInterfaceMPIC, ".mpic"); // TODO: test

        // AFLI-editor v2.0
        registerImageDecoder(createInterfaceAFL, ".afl");
        registerImageDecoder(createInterfaceAFL, ".afli");

        // Amica Paint
        registerImageDecoder(createInterfaceAMI, ".ami");

        // Art Studio
        registerImageDecoder(createInterfaceART, ".art");
        registerImageDecoder(createInterfaceART, ".ocp"); // TODO: check the format_size

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
        //registerImageDecoder(createInterfaceDD, "jj"); // TODO: support compression

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
        registerImageDecoder(createInterfacePMG, "pmg");

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
        registerImageDecoder(createInterfaceSHFXL, ".shx"); // TODO: support compression
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
    }

} // namespace mango::image
