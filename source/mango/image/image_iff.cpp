/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <vector>
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/image/image.hpp>

// References:
// - https://wiki.amigaos.net/wiki/EA_IFF_85_Standard_for_Interchange_Format_Files
// - https://1fish2.github.io/IFF/IFF%20docs%20with%20Commodore%20revisions/ILBM.pdf
// - IFF ANIM (delta-compressed animation): http://www.textfiles.com/programming/FORMATS/anim7.txt
//   ANIM op 5 ("Byte Vertical Delta Compression") is the most common method.

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

                // need one source byte; clamp the run to the destination
                if (s >= s_end)
                    break;
                if (count > d_end - d)
                    count = int(d_end - d);

                s8 value = *s++;
                std::memset(d, value, count);
                d += count;
            }
            else if (n < 128)
            {
                int count = n + 1;

                // clamp the literal run to both source and destination
                if (count > s_end - s)
                    count = int(s_end - s);
                if (count > d_end - d)
                    count = int(d_end - d);

                std::memcpy(d, s, count);
                s += count;
                d += count;
            }
        }
    }

    static
    void unpack_rgb8(Memory dest, ConstMemory source, u32 pixels)
    {
        u32* ptr = reinterpret_cast<u32*>(dest.address);
        const u8* s = source.address;
        const u8* s_end = source.end();

        while (pixels > 0)
        {
            if (s + 4 > s_end)
                break;

            u32 v = bigEndian::uload32(s);
            s += 4;

            u32 color = (v >> 8) | 0xff000000;
            u32 count = v & 0x7f;

            // a run must never write past the destination pixel count
            if (count > pixels)
                count = pixels;

            for (u32 i = 0; i < count; ++i)
            {
                ptr[i] = color;
            }

            ptr += count;
            pixels -= count;
        }
    }

    static
    void unpack_rgbn(Memory dest, ConstMemory source, u32 pixels)
    {
        u16* ptr = reinterpret_cast<u16*>(dest.address);
        const u8* s = source.address;
        const u8* s_end = source.end();

        while (pixels > 0)
        {
            if (s + 2 > s_end)
                break;

            u16 v = bigEndian::uload16(s);
            s += 2;

            u16 color = (v >> 4) | 0xf000;
            u32 count = v & 0x07;
            if (!count)
            {
                if (s >= s_end)
                    break;
                count = *s++;
            }
            if (!count)
            {
                if (s + 2 > s_end)
                    break;
                count = bigEndian::uload16(s);
                s += 2;
            }

            // a run must never write past the destination pixel count
            if (count > pixels)
                count = pixels;

            for (u32 i = 0; i < count; ++i)
            {
                ptr[i] = color;
            }

            ptr += count;
            pixels -= count;
        }
    }

    static
    void decode_rgbn(const Surface& dest, ConstMemory source, u32 xsize, u32 ysize)
    {
        u32 count = xsize * ysize;

        Format format(16, Format::UNORM, Format::BGRA, 4, 4, 4, 4);
        Bitmap temp(xsize, ysize, format);

        unpack_rgbn(Memory(temp.image, count * 2), source, count);

        dest.blit(0, 0, temp);
    }

    static
    void decode_rgb8(const Surface& dest, ConstMemory source, u32 xsize, u32 ysize)
    {
        u32 count = xsize * ysize;

        Format format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
        Bitmap temp(xsize, ysize, format);

        unpack_rgb8(Memory(temp.image, count * 4), source, count);

        dest.blit(0, 0, temp);
    }

    static
    void p2c_byte(u8* image, const u8* src, int count, int plane, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

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

    static
    void p2c_raw(const Surface& surface, const u8* buffer, int nplanes, int masking)
    {
        const int pixel_bytes = (nplanes + 7) >> 3;
        const int plane_bytes = ((surface.width + 15) & ~15) / 8;
        const int scan_bytes = surface.width * pixel_bytes;

        // Select p2c function
        auto p2c = nplanes <= 8 ? p2c_byte : p2c_wide;

        u8* image = surface.image;

        for (int y = 0; y < surface.height; ++y)
        {
            std::memset(image, 0, scan_bytes);

            for (int p = 0; p < nplanes; ++p)
            {
                p2c(image, buffer, surface.width, p, pixel_bytes);
                buffer += plane_bytes;
            }

            if (masking == 1)
                buffer += plane_bytes;

            image += surface.stride;
        }
    }

    static
    void p2c_ham(const Surface& surface, const u8* workptr, int nplanes, const Palette& palette)
    {
        bool hamcode2b = (nplanes == 6 || nplanes == 8);
        int ham_shift = 8 - (nplanes - (hamcode2b ? 2 : 1));
        //int ham_mask = (1 << ham_shift) - 1;

        int lineskip = ((surface.width + 15) >> 4) << 1;

        for (int y = 0; y < surface.height; ++y)
        {
            u32 r = palette[0].r;
            u32 g = palette[0].g;
            u32 b = palette[0].b;
            u32 a = palette[0].a;

            u32 bitmask = 0x80;
            const u8* workptr2 = workptr;

            u8* dest = surface.image + y * surface.stride;

            for (int x = 0; x < surface.width; ++x)
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
                        a = palette[v].a;
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
                dest[3] = u8(a);
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

    static
    void copy(const Surface& surface, const u8* buffer)
    {
        for (int y = 0; y < surface.height; ++y)
        {
            std::memcpy(surface.image + y * surface.stride, buffer, surface.width);
            buffer += surface.width;
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

            printLine(Print::Info, "  size: {} x {}", xsize, ysize);
            printLine(Print::Info, "  nplanes: {}", nplanes);
            printLine(Print::Info, "  compression: {}", compression);
        }
    };

    // IFF ANIM animation header (per frame). Only the fields we need are decoded.
    struct ChunkANHD
    {
        u8 operation = 0;   // compression method (0: BODY, 5: byte vertical delta, ...)
        u8 mask = 0;
        u8 interleave = 0;  // 0 means 2 (double buffering): delta is applied to frame n-2
        u32 abstime = 0;
        u32 reltime = 0;    // frame duration in jiffies (1/60 s)
        u32 bits = 0;

        void parse(BigEndianConstPointer p, size_t size)
        {
            if (size < 24)
                return;

            operation = p.read8();
            mask = p.read8();
            p += 2;             // w
            p += 2;             // h
            p += 2;             // x
            p += 2;             // y
            abstime = p.read32();
            reltime = p.read32();
            interleave = p.read8();
            p += 1;             // pad
            bits = p.read32();
        }
    };

    // ANIM op 5: "Byte Vertical Delta Compression".
    // The DLTA chunk begins with 16 big-endian longword plane offsets (only the first
    // nplanes are used; 0 means the plane is unchanged). Each plane is stored as a set of
    // columns; every column is an opcode stream walked top-to-bottom:
    //   code == 0      : <count><value>  run of 'count' identical bytes
    //   code &  0x80   : copy (code & 0x7f) literal bytes from the stream
    //   else           : skip 'code' rows (left unchanged)
    // Deltas are applied in-place onto an interleaved (ILBM-layout) bitplane buffer.
    static
    void apply_delta5(u8* planar, const u8* dlta, const u8* dlta_end, int rowbytes, int nplanes, int height)
    {
        const int stride = rowbytes * nplanes; // interleaved: one full scanline of all planes

        for (int plane = 0; plane < nplanes; ++plane)
        {
            if (dlta + (plane + 1) * 4 > dlta_end)
                break;

            u32 offset = bigEndian::uload32(dlta + plane * 4);
            if (offset == 0)
                continue; // plane unchanged this frame

            const u8* ptr = dlta + offset;
            if (ptr < dlta || ptr > dlta_end)
                continue;

            for (int col = 0; col < rowbytes; ++col)
            {
                if (ptr >= dlta_end)
                    break;

                const int base = plane * rowbytes + col;
                int row = 0;

                u8 opcount = *ptr++;

                for (int op = 0; op < opcount; ++op)
                {
                    if (ptr >= dlta_end)
                        break;

                    u8 code = *ptr++;

                    if (code == 0)
                    {
                        // run of identical bytes
                        if (ptr + 2 > dlta_end)
                            break;

                        u8 count = *ptr++;
                        u8 value = *ptr++;

                        while (count-- && row < height)
                        {
                            planar[base + row * stride] = value;
                            ++row;
                        }
                    }
                    else if (code & 0x80)
                    {
                        // literal bytes
                        int count = code & 0x7f;

                        while (count-- && row < height)
                        {
                            if (ptr >= dlta_end)
                                break;
                            planar[base + row * stride] = *ptr++;
                            ++row;
                        }
                    }
                    else
                    {
                        // skip rows (unchanged)
                        row += code;
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;

        Signature m_signature;
        ChunkBMHD m_bmhd;
        bool m_ham = false;
        bool m_ehb = false;
        Palette m_palette;

        ConstMemory m_body;

        // IFF ANIM (animation) state
        bool m_anim = false;
        struct Frame { const u8* begin; const u8* end; };
        std::vector<Frame> m_frames;
        int m_anim_index = 0;
        std::unique_ptr<u8[]> m_planar[2];
        size_t m_planar_size = 0;

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
                header.setError("[ImageDecoder.IFF] Incorrect signature ({}{}{}{}).", c[0], c[1], c[2], c[3]);
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

                case u32_mask_rev('A','N','I','M'):
                    // FORM ANIM is a container of nested FORM ILBM frames
                    m_signature = SIGNATURE_IFF;
                    m_anim = true;
                    break;

                default:
                {
                    const char* c = reinterpret_cast<const char*>(p - 4);
                    header.setError("[ImageDecoder.IFF] Incorrect signature ({}{}{}{}).", c[0], c[1], c[2], c[3]);
                    return nullptr;
                }
            }

            return p;
        }

        void parse()
        {
            const u8* data = m_memory.address;
            const u8* memory_end = m_memory.end();

            // need the 12-byte FORM/type signature at minimum
            if (m_memory.size < 12)
            {
                header.setError("[ImageDecoder.IFF] Not enough data.");
                return;
            }

            data = readSignature(data);
            if (!data)
            {
                // incorrect signature
                return;
            }

            if (m_anim)
            {
                parseAnim(data, memory_end);
                return;
            }

            // chunk reader: each chunk has an 8-byte header (id + size)
            while (data + 8 <= memory_end)
            {
                BigEndianConstPointer p = data;

                // chunk header
                u32 id = p.read32();
                u32 size = p.read32();

                const char* c = reinterpret_cast<const char*>(p - 8);
                printLine(Print::Info, "[{}{}{}{}] {} bytes", c[0], c[1], c[2], c[3], size);

                // Clamp the declared payload to what is actually present so a corrupt or
                // oversized chunk size cannot drive any handler past the end of the buffer.
                const size_t avail = size_t(memory_end - p);
                const size_t payload = std::min<size_t>(size, avail);

                // next chunk (chunks are padded to an even size)
                data = p + payload + (payload & 1);

                switch (id)
                {
                    case u32_mask_rev('B','M','H','D'):
                    {
                        if (payload >= 20)
                        {
                            m_bmhd.parse(p);

                            header.width  = m_bmhd.xsize;
                            header.height = m_bmhd.ysize;
                        }
                        break;
                    }

                    case u32_mask_rev('C','A','M','G'):
                    {
                        if (payload >= 4)
                        {
                            u32 v = p.read32();
                            m_ham = (v & 0x0800) != 0;
                            m_ehb = (v & 0x0080) != 0;
                            printLine(Print::Info, "  ham: {}", m_ham);
                            printLine(Print::Info, "  ehb: {}", m_ehb);
                        }
                        break;
                    }

                    case u32_mask_rev('A','N','N','O'):
                    {
                        break;
                    }

                    case u32_mask_rev('C','M','A','P'):
                    {
                        // the palette holds at most 256 entries (Palette::color[256])
                        u32 colors = std::min<u32>(u32(payload / 3), 256);
                        m_palette.size = colors;
                        for (u32 i = 0; i < colors; ++i)
                        {
                            m_palette[i] = Color(p[0], p[1], p[2], 0xff);
                            p += 3;
                        }

                        printLine(Print::Info, "  palette: {} colors", m_palette.size);
                        break;
                    }

                    case u32_mask_rev('B','O','D','Y'):
                    {
                        m_body = ConstMemory(p, payload);
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }
            }

            if (m_signature == SIGNATURE_RGBN || m_signature == SIGNATURE_RGB8)
            {
                if (m_bmhd.compression != 4)
                {
                    header.setError("[ImageDecoder.IFF] Incorrect compression.");
                }
            }

            resolveFormat();
        }

        void resolveFormat()
        {
            if (m_ham)
            {
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
            else
            {
                int bpp = (m_bmhd.nplanes + 7) >> 3;
                switch (bpp)
                {
                    case 1:
                        header.format = IndexedFormat(8);
                        break;
                    case 2:
                        header.format = Format(16, Format::UNORM, Format::BGR, 5, 6, 5);
                        break;
                    case 3:
                        header.format = Format(24, Format::UNORM, Format::RGB, 8, 8, 8);
                        break;
                    case 4:
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                        break;
                    default:
                        header.setError("[ImageDecoder.IFF] Incorrect number of planes.");
                        break;
                }
            }
        }

        // Walk the chunks inside one ILBM frame (interior of its FORM). Updates the shared
        // BMHD / palette / HAM-EHB state when those chunks are present and reports the per-
        // frame BODY, DLTA and ANHD. Animations carry BMHD/CMAP/CAMG only in the first
        // frame; later frames typically hold just ANHD + DLTA.
        void parseFrameChunks(const Frame& frame, ConstMemory& out_body, ConstMemory& out_dlta, ChunkANHD* out_anhd)
        {
            const u8* data = frame.begin;
            const u8* end = frame.end;

            while (data + 8 <= end)
            {
                BigEndianConstPointer p = data;

                u32 id = p.read32();
                u32 size = p.read32();

                const size_t avail = size_t(end - p);
                const size_t payload = std::min<size_t>(size, avail);

                data = p + payload + (payload & 1);

                switch (id)
                {
                    case u32_mask_rev('B','M','H','D'):
                    {
                        if (payload >= 20)
                        {
                            m_bmhd.parse(p);
                            header.width  = m_bmhd.xsize;
                            header.height = m_bmhd.ysize;
                        }
                        break;
                    }

                    case u32_mask_rev('C','A','M','G'):
                    {
                        if (payload >= 4)
                        {
                            u32 v = p.read32();
                            m_ham = (v & 0x0800) != 0;
                            m_ehb = (v & 0x0080) != 0;
                        }
                        break;
                    }

                    case u32_mask_rev('C','M','A','P'):
                    {
                        u32 colors = std::min<u32>(u32(payload / 3), 256);
                        m_palette.size = colors;
                        for (u32 i = 0; i < colors; ++i)
                        {
                            m_palette[i] = Color(p[0], p[1], p[2], 0xff);
                            p += 3;
                        }
                        break;
                    }

                    case u32_mask_rev('A','N','H','D'):
                    {
                        if (out_anhd)
                        {
                            out_anhd->parse(p, payload);
                        }
                        break;
                    }

                    case u32_mask_rev('B','O','D','Y'):
                    {
                        out_body = ConstMemory(p, payload);
                        break;
                    }

                    case u32_mask_rev('D','L','T','A'):
                    {
                        out_dlta = ConstMemory(p, payload);
                        break;
                    }

                    default:
                        break;
                }
            }
        }

        void parseAnim(const u8* data, const u8* end)
        {
            // collect the nested FORM ILBM frames
            while (data + 8 <= end)
            {
                BigEndianConstPointer p = data;

                u32 id = p.read32();
                u32 size = p.read32();

                const size_t avail = size_t(end - p);
                const size_t payload = std::min<size_t>(size, avail);
                const u8* next = p + payload + (payload & 1);

                if (id == u32_mask_rev('F','O','R','M') && payload >= 4)
                {
                    // skip the 4-byte form type; the interior holds this frame's chunks
                    Frame frame { p + 4, p + payload };
                    m_frames.push_back(frame);
                }

                data = next;
            }

            if (m_frames.empty())
            {
                header.setError("[ImageDecoder.IFF] ANIM has no frames.");
                return;
            }

            // the first frame establishes geometry, palette and colour mode
            ConstMemory body, dlta;
            ChunkANHD anhd;
            parseFrameChunks(m_frames[0], body, dlta, &anhd);

            header.frames = int(m_frames.size());

            resolveFormat();
        }

        // Render an interleaved (ILBM-layout) bitplane buffer to the decode target using the
        // current palette / colour mode. Shared by the ANIM frame path.
        void renderPlanar(const Surface& dest, const u8* buffer)
        {
            u32 xsize = m_bmhd.xsize;
            u32 ysize = m_bmhd.ysize;
            xsize = xsize + (xsize & 1);

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

            DecodeTargetBitmap target(dest, xsize, ysize, header.format, m_palette);

            if (m_ham)
            {
                p2c_ham(target, buffer, m_bmhd.nplanes, m_palette);
            }
            else
            {
                p2c_raw(target, buffer, m_bmhd.nplanes, m_bmhd.masking);
            }

            target.resolve();
        }

        ImageDecodeStatus decodeAnim(const Surface& dest)
        {
            ImageDecodeStatus status;

            const int index = m_anim_index;

            ConstMemory body, dlta;
            ChunkANHD anhd;
            parseFrameChunks(m_frames[index], body, dlta, &anhd);

            const int rowbytes = ((int(m_bmhd.xsize) + 15) >> 4) << 1;
            const int nplanes = m_bmhd.nplanes;
            const int height = m_bmhd.ysize;
            const size_t planar_size = size_t(rowbytes) * nplanes * height;

            if (!planar_size)
            {
                status.setError("[ImageDecoder.IFF] Invalid ANIM dimensions.");
                return status;
            }

            // allocate the double buffers once (interleave==0 uses frame n-2 as the base)
            if (!m_planar[0])
            {
                m_planar[0].reset(new u8[planar_size]());
                m_planar[1].reset(new u8[planar_size]());
                m_planar_size = planar_size;
            }

            if (index == 0)
            {
                // keyframe: a normal (optionally ByteRun1) BODY, used to (re)seed both buffers
                u8* buffer = m_planar[0].get();
                std::memset(buffer, 0, planar_size);

                if (body.address)
                {
                    if (m_bmhd.compression == 1)
                    {
                        unpack(Memory(buffer, planar_size), body);
                    }
                    else
                    {
                        std::memcpy(buffer, body.address, std::min(planar_size, body.size));
                    }
                }

                std::memcpy(m_planar[1].get(), buffer, planar_size);

                renderPlanar(dest, buffer);
            }
            else
            {
                // delta frame: ping-pong so the delta is applied onto frame n-2
                u8* buffer = m_planar[index & 1].get();

                if (dlta.address)
                {
                    apply_delta5(buffer, dlta.address, dlta.end(), rowbytes, nplanes, height);
                }

                renderPlanar(dest, buffer);
            }

            status.current_frame_index = index;
            m_anim_index = (index + 1) % header.frames;
            status.next_frame_index = m_anim_index;

            // ANHD.reltime is the frame duration in jiffies (1/60 s)
            status.frame_delay_numerator = anhd.reltime ? int(anhd.reltime) : 2;
            status.frame_delay_denominator = 60;

            return status;
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);

            ImageDecodeStatus status;

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            if (m_anim)
            {
                return decodeAnim(dest);
            }

            if (!m_body.address)
            {
                status.setError("[ImageDecoder.IFF] Missing BODY chunk.");
                return status;
            }

            u32 xsize = m_bmhd.xsize;
            u32 ysize = m_bmhd.ysize;

            switch (m_signature)
            {
                case SIGNATURE_IFF:
                    break;
                case SIGNATURE_PBM:
                    break;
                case SIGNATURE_RGB8:
                    decode_rgb8(dest, m_body, xsize, ysize);
                    return status;
                case SIGNATURE_RGBN:
                    decode_rgbn(dest, m_body, xsize, ysize);
                    return status;
                default:
                    break;
            }

            const u8* p = m_body.address;

            bool is_pbm = m_signature == SIGNATURE_PBM;

            std::unique_ptr<u8[]> allocation;
            const u8* buffer = nullptr;
            size_t buffer_size = 0;

            if (m_bmhd.compression == 1)
            {
                size_t scan_size = ((xsize + 15) & ~15) / 8 * (m_bmhd.nplanes + (m_bmhd.masking == 1));
                size_t bytes = scan_size * ysize;

                allocation.reset(new u8[bytes]);
                u8* allocated = allocation.get();

                unpack(Memory(allocated, bytes), m_body);
                buffer = allocated;
                buffer_size = bytes;
            }
            else
            {
                buffer = p;
                buffer_size = m_body.size;
            }

            if (!buffer)
            {
                status.setError("[ImageDecoder.IFF] No decoding buffer.");
                return status;
            }

            // alignment
            xsize = xsize + (xsize & 1);

            // Verify the decode buffer is large enough for the pixel path that follows so
            // the planar-to-chunky / copy readers below cannot run past its end.
            {
                const size_t plane_bytes = (((size_t(xsize) + 15) & ~size_t(15)) / 8);
                size_t needed;

                if (m_ham)
                {
                    needed = plane_bytes * m_bmhd.nplanes * ysize;
                }
                else if (is_pbm)
                {
                    needed = size_t(xsize) * ysize;
                }
                else
                {
                    needed = plane_bytes * (m_bmhd.nplanes + (m_bmhd.masking == 1 ? 1 : 0)) * ysize;
                }

                if (buffer_size < needed)
                {
                    status.setError("[ImageDecoder.IFF] Truncated image data.");
                    return status;
                }
            }

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

            // masking == 2: the BMHD transparent colour index renders fully
            // transparent. For palette-based output this propagates through the
            // attached palette; the HAM writer reads palette alpha in its base
            // (hold) case so the same entry stays transparent there too.
            if (m_bmhd.masking == 2 && m_bmhd.transparent < m_palette.size)
            {
                m_palette[m_bmhd.transparent].a = 0;
            }

            DecodeTargetBitmap target(dest, xsize, ysize, header.format, m_palette);

            if (m_ham)
            {
                p2c_ham(target, buffer, m_bmhd.nplanes, m_palette);
            }
            else if (is_pbm)
            {
                copy(target, buffer);
            }
            else
            {
                // planar-to-chunky conversion
                p2c_raw(target, buffer, m_bmhd.nplanes, m_bmhd.masking);
            }

            target.resolve();
            status.direct = target.isDirect();

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecIFF()
    {
        registerImageDecoder(createInterface, ".iff");
        registerImageDecoder(createInterface, ".anim");
        registerImageDecoder(createInterface, ".lbm");
        registerImageDecoder(createInterface, ".ham");
        registerImageDecoder(createInterface, ".ham8");
        registerImageDecoder(createInterface, ".ilbm");
        registerImageDecoder(createInterface, ".ehb");
        registerImageDecoder(createInterface, ".rgbn");
    }

} // namespace mango::image
