/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // iff
    // ------------------------------------------------------------

    // https://wiki.amigaos.net/wiki/ILBM_IFF_Interleaved_Bitmap

    static
    void unpack(Memory dest, ConstMemory source)
    {
        const u8* s = source.address;
        const u8* s_end = source.end();

        u8* d = dest.address;
        u8* d_end = dest.end();

        for ( ; d < d_end && s < s_end; )
        {
            u8 n = *s++;
            if (n > 128)
            {
                int count = 257 - n;
                s8 value = *s++;
                std::memset(d, value, count);
                d += count;
            }
            else if (n < 128)
            {
                int count = n + 1;
                std::memcpy(d, s, count);
                s += count;
                d += count;
            }
        }
    }

    static
    void unpack_rgb8(Memory dest, ConstMemory source, size_t pixels)
    {
        u32* ptr = reinterpret_cast<u32*>(dest.address);
        BigEndianConstPointer p = source.address;

        while (pixels > 0)
        {
            u32 v = p.read32();
            u32 color = (v >> 8) | 0xff000000;
            u32 count = v & 0x7f;

            for (u32 i = 0; i < count; ++i)
            {
                ptr[i] = color;
            }

            ptr += count;
            pixels -= count;
        }
    }

    static
    void unpack_rgbn(Memory dest, ConstMemory source, size_t pixels)
    {
        u16* ptr = reinterpret_cast<u16*>(dest.address);
        BigEndianConstPointer p = source.address;

        while (pixels > 0)
        {
            u16 v = p.read16();
            u16 color = (v >> 4) | 0xf000;
            u16 count = v & 0x07;
            if (!count)
                count = p.read8();
            if (!count)
                count = p.read16();

            for (u32 i = 0; i < count; ++i)
            {
                ptr[i] = color;
            }

            ptr += count;
            pixels -= count;
        }
    }

    static
    void p2c_byte(u8* image, const u8* src, int count, int plane, int)
    {
        while (count >= 8)
        {
            u64 data = *src++;
            u64 value = ((data & 0x08) << 29) | ((data & 0x04) << 38) | ((data & 0x02) << 47) | ((data & 0x01) << 56) |
                        ((data & 0x80) >>  7) | ((data & 0x40) <<  2) | ((data & 0x20) << 11) | ((data & 0x10) << 20);
            u64* dest = reinterpret_cast<u64*>(image);
            *dest |= (value << plane);
            image += 8;
            count -= 8;
        }

        u8 value = u8(1) << plane;

        u8 data = *src;
        u8 mask = 0x80;

        while (count-- > 0)
        {
            *image++ |= ((data & mask) ? value : 0);
            mask >>= 1;
        }
    }

    static
    void p2c_wide(u8* image, const u8* src, int count, int plane, int bpp)
    {
        u8* dest = image + (plane / 8);
        const int pshift = plane & 7;

        for (int x = 0; x < count; ++x)
        {
            dest[0] |= ((src[x / 8] >> ((x ^ 7) & 7)) & 1) << pshift;
            dest += bpp;
        }
    }

    void p2c_raw(u8* image, const u8* temp, int xsize, int ysize, int nplanes, int masking)
    {
        const int pixel_bytes = (nplanes + 7) >> 3;
        const int plane_bytes = ((xsize + 15) & ~15) / 8;

        auto p2c = nplanes <= 8 ? p2c_byte : p2c_wide;

        for (int y = 0; y < ysize; ++y)
        {
            std::memset(image, 0, xsize * pixel_bytes);

            for (int p = 0; p < nplanes; ++p)
            {
                p2c(image, temp, xsize, p, pixel_bytes);
                temp += plane_bytes;
            }

            if (masking == 1)
                temp += plane_bytes;

            image += xsize * pixel_bytes;
        }
    }

    void p2c_ham(u8* dest, const u8* workptr, int width, int height, int nplanes, const Palette& palette)
    {
        bool hamcode2b = (nplanes == 6 || nplanes == 8);
        int ham_shift = 8 - (nplanes - (hamcode2b ? 2 : 1));
        //int ham_mask = (1 << ham_shift) - 1;

        int lineskip = ((width + 15) >> 4) << 1;

        for (int y = 0; y < height; ++y)
        {
            u32 r = palette[0].r;
            u32 g = palette[0].g;
            u32 b = palette[0].b;

            u32 bitmask = 0x80;
            const u8* workptr2 = workptr;

            for (int x = 0; x < width; ++x)
            {
                const u8* workptr3 = workptr2;

                // read value
                u32 v = 0;
                u32 colorbit = 1;

                for (int plane = 2; plane < nplanes; ++plane)
                {
                    if (*workptr3 & bitmask)
                    {
                        v |= colorbit;
                    }
                    workptr3 += lineskip;
                    colorbit += colorbit;
                }

                // read hamcode
                u32 hamcode = 0;

                if (*workptr3 & bitmask)
                {
                    hamcode = 1;
                }
                workptr3 += lineskip;

                if (hamcode2b)
                {
                    if (*workptr3 & bitmask)
                    {
                        hamcode |= 2;
                    }
                    workptr3 += lineskip;
                }

                // hold-and-modify
                switch (hamcode)
                {
                    case 0:
                        r = palette[v].r;
                        g = palette[v].g;
                        b = palette[v].b;
                        break;

                    case 1:
                        b = v << ham_shift;
                        break;

                    case 2:
                        r = v << ham_shift;
                        break;

                    case 3:
                        g = v << ham_shift;
                        break;
                }

                dest[0] = u8(r);
                dest[1] = u8(g);
                dest[2] = u8(b);
                dest[3] = 0xff;
                dest += 4;

                bitmask >>= 1;

                if (!bitmask)
                {
                    bitmask = 0x80;
                    ++workptr2;
                }
            }

            workptr += lineskip * nplanes;
        }
    }

    void expand_palette(u8* dest, const u8* src, int xsize, int ysize, const Palette& palette)
    {
        Color* image = reinterpret_cast<Color*>(dest);
        int count = xsize * ysize;

        for (int i = 0; i < count; ++i)
        {
            image[i] = palette[src[i]];
        }
    }

    enum Signature
    {
        SIGNATURE_ERROR = 0,
        SIGNATURE_IFF = 1,
        SIGNATURE_PBM = 2,
        SIGNATURE_RGB8 = 3,
        SIGNATURE_RGBN = 4,
    };

    struct ChunkBMHD
    {
        u16 xsize = 0;
        u16 ysize = 0;
        s16 xorigin = 0;
        s16 yorigin = 0;
        u8 nplanes = 0;
        u8 masking = 0; // 0: None, 1: HasMask, 2: TransparentColor, 3: Lasso
        u8 compression = 0; // 0: None, 1: ByteRun1
        u8 unused = 0;
        u16 transparent = 0;
        u8 xaspect = 0;
        u8 yaspect = 0;
        s16 xscreen = 0;
        s16 yscreen = 0;

        void parse(BigEndianConstPointer p)
        {
            xsize = p.read16();
            ysize = p.read16();
            xorigin = p.read16();
            yorigin = p.read16();
            nplanes = p.read8();
            masking = p.read8();
            compression = p.read8();
            unused = p.read8();
            transparent = p.read16();
            xaspect = p.read8();
            yaspect = p.read8();
            xscreen = p.read16();
            yscreen = p.read16();
        }
    };

    Format select_format(int nplanes, bool ham)
    {
        // choose pixelformat
        Format format;

        if (ham)
        {
            // always decode Hold And Modify modes into 32 bpp
            format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        }
        else
        {
            int bpp = (nplanes + 7) >> 3;
            switch (bpp)
            {
                case 1:
                    // expand palette
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;
                case 2:
                    format = Format(16, Format::UNORM, Format::BGR, 5, 6, 5);
                    break;
                case 3:
                    format = Format(24, Format::UNORM, Format::RGB, 8, 8, 8);
                    break;
                case 4:
                    format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;
            }
        }

        return format;
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ConstMemory m_memory;
        ImageHeader m_header;

        Signature m_signature;
        ChunkBMHD m_bmhd;
        bool m_ham = false;
        bool m_ehb = false;
        Palette m_palette;

        ConstMemory m_body;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            parse();
        }

        ~Interface()
        {
        }

        const u8* readSignature(const u8* data)
        {
            BigEndianConstPointer p = data;

            u32 s0 = p.read32();
            p += 4;
            u32 s1 = p.read32();

            if (s0 != u32_mask_rev('F','O','R','M'))
            {
                const char* c = reinterpret_cast<const char*>(p - 4);
                m_header.setError("[ImageDecoder.IFF] Incorrect signature ({}{}{}{}).", c[0], c[1], c[2], c[3]);
                return nullptr;
            }

            switch (s1)
            {
                case u32_mask_rev('I','L','B','M'):
                    m_signature = SIGNATURE_IFF;
                    break;

                case u32_mask_rev('P','B','M',' '):
                    m_signature = SIGNATURE_PBM;
                    break;

                case u32_mask_rev('R','G','B','N'):
                    m_signature = SIGNATURE_RGBN;
                    break;

                case u32_mask_rev('R','G','B','8'):
                    m_signature = SIGNATURE_RGB8;
                    break;

                default:
                {
                    const char* c = reinterpret_cast<const char*>(p - 4);
                    m_header.setError("[ImageDecoder.IFF] Incorrect signature ({}{}{}{}).", c[0], c[1], c[2], c[3]);
                    return nullptr;
                }
            }

            return p;
        }

        void parse()
        {
            const u8* data = m_memory.address;
            const u8* end = m_memory.address + m_memory.size - 12;

            data = readSignature(data);
            if (!data)
            {
                // incorrect signature
                return;
            }

            // chunk reader
            while (data < end)
            {
                BigEndianConstPointer p = data;

                // chunk header
                u32 id = p.read32();
                u32 size = p.read32();

                const char* c = reinterpret_cast<const char*>(p - 8);
                printLine(Print::Info, "[{}{}{}{}] {} bytes", c[0], c[1], c[2], c[3], size);

                // next chunk
                data = p + size + (size & 1);

                switch (id)
                {
                    case u32_mask_rev('B','M','H','D'):
                    {
                        m_bmhd.parse(p);

                        m_header.width  = m_bmhd.xsize;
                        m_header.height = m_bmhd.ysize;
                        break;
                    }

                    case u32_mask_rev('C','A','M','G'):
                    {
                        u32 v = p.read32();
                        m_ham = (v & 0x0800) != 0;
                        m_ehb = (v & 0x0080) != 0;
                        break;
                    }

                    case u32_mask_rev('A','N','N','O'):
                    {
                        break;
                    }

                    case u32_mask_rev('C','M','A','P'):
                    {
                        m_palette.size = size / 3;
                        for (u32 i = 0; i < m_palette.size; ++i)
                        {
                            m_palette[i] = Color(p[0], p[1], p[2], 0xff);
                            p += 3;
                        }
                        break;
                    }

                    case u32_mask_rev('B','O','D','Y'):
                    {
                        m_body = ConstMemory(p, size);
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }
            }

            u8 nplanes = m_bmhd.nplanes;

            m_header.palette = nplanes <= 8 && !m_ham;
            m_header.format = select_format(nplanes, m_ham);
        }

        ImageHeader header() override
        {
            return m_header;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!m_header.success)
            {
                status.setError(m_header.info);
                return status;
            }

            if (!m_body.address)
            {
                status.setError("[ImageDecoder.IFF] Missing BODY chunk.");
                return status;
            }

            if (m_signature == SIGNATURE_RGBN || m_signature == SIGNATURE_RGB8)
            {
                if (m_bmhd.compression != 4)
                {
                    m_header.setError("[ImageDecoder.IFF] Incorrect compression.");
                }
            }

            const u8* data = m_body.address;
            //const u8* end = m_body.end();

            //u8 nplanes = m_bmhd.nplanes;

            bool is_pbm = m_signature == SIGNATURE_PBM;

            Palette* ptr_palette = options.palette; // palette request destination

            std::unique_ptr<u8[]> allocation;
            const u8* buffer = nullptr;

            const u8* p = data;

            if (m_bmhd.compression == 1)
            {
                size_t scan_size = ((m_bmhd.xsize + 15) & ~15) / 8 * (m_bmhd.nplanes + (m_bmhd.masking == 1));
                size_t bytes = scan_size * m_bmhd.ysize;

                allocation.reset(new u8[bytes]);
                u8* allocated = allocation.get();

                unpack(Memory(allocated, bytes), m_body);
                buffer = allocated;
            }
            else if (m_signature == SIGNATURE_RGBN && m_bmhd.compression == 4)
            {
                size_t pixels = m_bmhd.xsize * m_bmhd.ysize;
                size_t bytes = pixels * 2;

                allocation.reset(new u8[bytes]);
                u8* allocated = allocation.get();

                unpack_rgbn(Memory(allocated, bytes), m_body, pixels);
                buffer = allocated;
            }
            else if (m_signature == SIGNATURE_RGB8 && m_bmhd.compression == 4)
            {
                size_t pixels = m_bmhd.xsize * m_bmhd.ysize;
                size_t bytes = pixels * 4;

                allocation.reset(new u8[bytes]);
                u8* allocated = allocation.get();

                unpack_rgb8(Memory(allocated, bytes), m_body, pixels);
                buffer = allocated;
            }
            else
            {
                buffer = p;
            }

            if (!buffer)
            {
                status.setError("[ImageDecoder.IFF] BODY chunk missing.");
                return status;
            }

            // alignment
            m_bmhd.xsize = m_bmhd.xsize + (m_bmhd.xsize & 1);

            // fix ehb palette
            if (m_ehb && (m_palette.size == 32 || m_palette.size == 64))
            {
                for (int i = 0; i < 32; ++i)
                {
                    m_palette[i + 32].r = m_palette[i].r >> 1;
                    m_palette[i + 32].g = m_palette[i].g >> 1;
                    m_palette[i + 32].b = m_palette[i].b >> 1;
                    m_palette[i + 32].a = 0xff;
                }
            }

            if (m_signature == SIGNATURE_RGBN)
            {
                Format format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4);
                Bitmap temp(m_bmhd.xsize, m_bmhd.ysize, format);

                std::memcpy(temp.image, buffer, m_bmhd.xsize * m_bmhd.ysize * 2);
                dest.blit(0, 0, temp);
            }
            else if (m_signature == SIGNATURE_RGB8)
            {
                Format format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                Bitmap temp(m_bmhd.xsize, m_bmhd.ysize, format);

                std::memcpy(temp.image, buffer, m_bmhd.xsize * m_bmhd.ysize * 4);
                dest.blit(0, 0, temp);
            }
            else if (m_palette.size > 0 && !m_ham && ptr_palette)
            {
                // client requests for palette and the image has one
                *ptr_palette = m_palette;

                if (is_pbm)
                {
                    // linear
                    std::memcpy(dest.image, buffer, m_bmhd.xsize * m_bmhd.ysize);
                }
                else
                {
                    // interlaced
                    Bitmap raw(m_bmhd.xsize, m_bmhd.ysize, LuminanceFormat(8, Format::UNORM, 8, 0));
                    p2c_raw(raw.image, buffer, m_bmhd.xsize, m_bmhd.ysize, m_bmhd.nplanes, m_bmhd.masking);
                    std::memcpy(dest.image, raw.image, m_bmhd.xsize * m_bmhd.ysize);
                }
            }
            else
            {
                // choose pixelformat
                Format format = select_format(m_bmhd.nplanes, m_ham);
                Bitmap temp(m_bmhd.xsize, m_bmhd.ysize, format);

                // planar-to-chunky conversion
                if (m_ham)
                {
                    p2c_ham(temp.image, buffer, m_bmhd.xsize, m_bmhd.ysize, m_bmhd.nplanes, m_palette);
                }
                else
                {
                    if (is_pbm)
                    {
                        // linear
                        if (m_bmhd.nplanes <= 8)
                        {
                            expand_palette(temp.image, buffer, m_bmhd.xsize, m_bmhd.ysize, m_palette);
                        }
                        else
                        {
                            std::memcpy(temp.image, buffer, temp.stride * m_bmhd.ysize);
                        }
                    }
                    else
                    {
                        // interlaced
                        if (m_bmhd.nplanes <= 8)
                        {
                            Bitmap raw(m_bmhd.xsize, m_bmhd.ysize, LuminanceFormat(8, Format::UNORM, 8, 0));
                            p2c_raw(raw.image, buffer, m_bmhd.xsize, m_bmhd.ysize, m_bmhd.nplanes, m_bmhd.masking);
                            expand_palette(temp.image, raw.image, m_bmhd.xsize, m_bmhd.ysize, m_palette);
                        }
                        else
                        {
                            p2c_raw(temp.image, buffer, m_bmhd.xsize, m_bmhd.ysize, m_bmhd.nplanes, m_bmhd.masking);
                        }
                    }
                }

                dest.blit(0, 0, temp);
            }

            return status;
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecIFF()
    {
        registerImageDecoder(createInterface, ".iff");
        registerImageDecoder(createInterface, ".lbm");
        registerImageDecoder(createInterface, ".ham");
        registerImageDecoder(createInterface, ".ham8");
        registerImageDecoder(createInterface, ".ilbm");
        registerImageDecoder(createInterface, ".ehb");
        registerImageDecoder(createInterface, ".rgbn");
    }

} // namespace mango::image
