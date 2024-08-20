/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
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
        SIGNATURE_IFF = 0,
        SIGNATURE_PBM = 1,
        SIGNATURE_RGB8 = 2,
        SIGNATURE_RGBN = 3,
        SIGNATURE_ERROR = 4
    };

    Signature read_signature(const u8*& data)
    {
        BigEndianConstPointer p = data;

        u32 v0 = p.read32(); p += 4;
        u32 v1 = p.read32();
        data = p;

        if (v0 != u32_mask_rev('F','O','R','M'))
        {
            return SIGNATURE_ERROR;
        }

        Signature signature = SIGNATURE_ERROR;

        switch (v1)
        {
            case u32_mask_rev('I','L','B','M'):
                signature = SIGNATURE_IFF;
                break;

            case u32_mask_rev('P','B','M',' '):
                signature = SIGNATURE_PBM;
                break;

            case u32_mask_rev('R','G','B','N'):
                signature = SIGNATURE_RGBN;
                break;

            case u32_mask_rev('R','G','B','8'):
                signature = SIGNATURE_RGB8;
                break;
        }

        return signature;
    }

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

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            parseHeader();
        }

        ~Interface()
        {
        }

        void parseHeader()
        {
            const u8* data = m_memory.address;
            const u8* end = m_memory.address + m_memory.size - 12;

            Signature signature = read_signature(data);
            if (signature == SIGNATURE_ERROR)
            {
                m_header.setError("[ImageDecoder.IFF] Incorrect signature.");
                return;
            }

            bool ham = false;
            u8 nplanes = 0;

            // chunk reader
            while (data < end)
            {
                // chunk header
                BigEndianConstPointer p = data;

                u32 id = p.read32();
                u32 size = p.read32();

                // next chunk
                data = p + size + (size & 1);

                switch (id)
                {
                    case u32_mask_rev('B','M','H','D'):
                    {
                        ChunkBMHD bmhd;
                        bmhd.parse(p);

                        if (signature == SIGNATURE_RGBN || signature == SIGNATURE_RGB8)
                        {
                            if (bmhd.compression != 4)
                            {
                                m_header.setError("[ImageDecoder.IFF] Incorrect compression.");
                            }
                        }

                        m_header.width  = bmhd.xsize;
                        m_header.height = bmhd.ysize;
                        nplanes = bmhd.nplanes;
                        break;
                    }

                    case u32_mask_rev('C','A','M','G'):
                    {
                        u32 v = p.read32();
                        ham = (v & 0x0800) != 0;
                        break;
                    }

                    default:
                        break;
                }
            }

            m_header.palette = nplanes <= 8 && !ham;
            m_header.format = select_format(nplanes, ham);
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

            const u8* data = m_memory.address;
            const u8* end = m_memory.address + m_memory.size - 12;

            Signature signature = read_signature(data);
            bool is_pbm = signature == SIGNATURE_PBM;

            Palette palette;
            Palette* ptr_palette = options.palette; // palette request destination

            std::unique_ptr<u8[]> allocation;
            const u8* buffer = nullptr;

            bool ham = false;
            bool ehb = false;

            ChunkBMHD bmhd;

            // chunk reader
            while (data < end)
            {
                // chunk header
                BigEndianConstPointer p = data;

                u32 id = p.read32();
                u32 size = p.read32();

                // next chunk
                data = p + size + (size & 1);

                switch (id)
                {
                    case u32_mask_rev('A','N','N','O'):
                    {
                        break;
                    }

                    case u32_mask_rev('B','M','H','D'):
                    {
                        bmhd.parse(p);
                        break;
                    }

                    case u32_mask_rev('C','M','A','P'):
                    {
                        palette.size = size / 3;
                        for (u32 i = 0; i < palette.size; ++i)
                        {
                            palette[i] = Color(p[0], p[1], p[2], 0xff);
                            p += 3;
                        }
                        break;
                    }

                    case u32_mask_rev('C','A','M','G'):
                    {
                        u32 v = p.read32();
                        ham = (v & 0x0800) != 0;
                        ehb = (v & 0x0080) != 0;
                        break;
                    }

                    case u32_mask_rev('B','O','D','Y'):
                    {
                        if (bmhd.compression == 1)
                        {
                            size_t scan_size = ((bmhd.xsize + 15) & ~15) / 8 * (bmhd.nplanes + (bmhd.masking == 1));
                            size_t bytes = scan_size * bmhd.ysize;

                            allocation.reset(new u8[bytes]);
                            u8* allocated = allocation.get();

                            unpack(Memory(allocated, bytes), ConstMemory(p, size));
                            buffer = allocated;
                        }
                        else if (signature == SIGNATURE_RGBN && bmhd.compression == 4)
                        {
                            size_t pixels = bmhd.xsize * bmhd.ysize;
                            size_t bytes = pixels * 2;

                            allocation.reset(new u8[bytes]);
                            u8* allocated = allocation.get();

                            u16* ptr = reinterpret_cast<u16*>(allocated);

                            while (pixels > 0)
                            {
                                u16 v = p.read16();
                                u16 color = (v >> 4) | 0xf000;
                                u16 count = v & 0x07;
                                if (!count)
                                {
                                    count = p.read8();
                                }
                                if (!count)
                                {
                                    count = p.read16();
                                }

                                for (u32 i = 0; i < count; ++i)
                                {
                                    ptr[i] = color;
                                }

                                ptr += count;
                                pixels -= count;
                            }

                            buffer = allocated;
                        }
                        else if (signature == SIGNATURE_RGB8 && bmhd.compression == 4)
                        {
                            size_t pixels = bmhd.xsize * bmhd.ysize;
                            size_t bytes = pixels * 4;

                            allocation.reset(new u8[bytes]);
                            u8* allocated = allocation.get();

                            u32* ptr = reinterpret_cast<u32*>(allocated);

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

                            buffer = allocated;
                        }
                        else
                        {
                            buffer = p;
                        }

                        break;
                    }

                    default:
                        break;
                }
            }

            if (!buffer)
            {
                status.setError("[ImageDecoder.IFF] BODY chunk missing.");
                return status;
            }

            // alignment
            bmhd.xsize = bmhd.xsize + (bmhd.xsize & 1);

            // fix ehb palette
            if (ehb && (palette.size == 32 || palette.size == 64))
            {
                for (int i = 0; i < 32; ++i)
                {
                    palette[i + 32].r = palette[i].r >> 1;
                    palette[i + 32].g = palette[i].g >> 1;
                    palette[i + 32].b = palette[i].b >> 1;
                    palette[i + 32].a = 0xff;
                }
            }

            if (signature == SIGNATURE_RGBN)
            {
                Format format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4);
                Bitmap temp(bmhd.xsize, bmhd.ysize, format);

                std::memcpy(temp.image, buffer, bmhd.xsize * bmhd.ysize * 2);
                dest.blit(0, 0, temp);
            }
            else if (signature == SIGNATURE_RGB8)
            {
                Format format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                Bitmap temp(bmhd.xsize, bmhd.ysize, format);

                std::memcpy(temp.image, buffer, bmhd.xsize * bmhd.ysize * 4);
                dest.blit(0, 0, temp);
            }
            else if (palette.size > 0 && !ham && ptr_palette)
            {
                // client requests for palette and the image has one
                *ptr_palette = palette;

                if (is_pbm)
                {
                    // linear
                    std::memcpy(dest.image, buffer, bmhd.xsize * bmhd.ysize);
                }
                else
                {
                    // interlaced
                    Bitmap raw(bmhd.xsize, bmhd.ysize, LuminanceFormat(8, Format::UNORM, 8, 0));
                    p2c_raw(raw.image, buffer, bmhd.xsize, bmhd.ysize, bmhd.nplanes, bmhd.masking);
                    std::memcpy(dest.image, raw.image, bmhd.xsize * bmhd.ysize);
                }
            }
            else
            {
                // choose pixelformat
                Format format = select_format(bmhd.nplanes, ham);
                Bitmap temp(bmhd.xsize, bmhd.ysize, format);

                // planar-to-chunky conversion
                if (ham)
                {
                    p2c_ham(temp.image, buffer, bmhd.xsize, bmhd.ysize, bmhd.nplanes, palette);
                }
                else
                {
                    if (is_pbm)
                    {
                        // linear
                        if (bmhd.nplanes <= 8)
                        {
                            expand_palette(temp.image, buffer, bmhd.xsize, bmhd.ysize, palette);
                        }
                        else
                        {
                            std::memcpy(temp.image, buffer, temp.stride * bmhd.ysize);
                        }
                    }
                    else
                    {
                        // interlaced
                        if (bmhd.nplanes <= 8)
                        {
                            Bitmap raw(bmhd.xsize, bmhd.ysize, LuminanceFormat(8, Format::UNORM, 8, 0));
                            p2c_raw(raw.image, buffer, bmhd.xsize, bmhd.ysize, bmhd.nplanes, bmhd.masking);
                            expand_palette(temp.image, raw.image, bmhd.xsize, bmhd.ysize, palette);
                        }
                        else
                        {
                            p2c_raw(temp.image, buffer, bmhd.xsize, bmhd.ysize, bmhd.nplanes, bmhd.masking);
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
