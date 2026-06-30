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
// - HAM-E (Black Belt Systems): a 4-plane hires ILBM reinterpreted by an external decoder,
//   programmed via "magic cookie" trigger scanlines.

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

    // Atari ST "VDAT" vertical RLE (ILBM compression mode 2). The BODY holds one
    // VDAT chunk per bitplane. Each chunk carries a byte command stream followed by
    // a 16-bit value (word) stream; words are laid out vertically, one 16-pixel
    // column at a time, top to bottom. Command bytes:
    //   b == 0        : <wordCount = value> literal run of that many words
    //   b == 1        : <wordCount = value><word> run of value words
    //   2 <= b <= 127 : run of b copies of the next word
    //   128 <= b      : literal run of (256 - b) words
    // Output is written into mango's plane-sequential interleaved scanline layout.
    static
    bool decode_vdat(u8* buffer, size_t scan_size, ConstMemory body, int width, int height, int nplanes)
    {
        const u8* end = body.end();
        const int rowbytes = ((width + 15) >> 4) << 1;
        const int words_per_row = rowbytes >> 1;

        const u8* chunk = body.address;

        for (int plane = 0; plane < nplanes; ++plane)
        {
            if (chunk + 14 > end || std::memcmp(chunk, "VDAT", 4) != 0)
                return false;

            const u32 chunk_size = bigEndian::uload32(chunk + 4);
            const u8* chunk_end = chunk + 8 + chunk_size;
            if (chunk_end > end || chunk_end < chunk)
                chunk_end = end;

            // command stream byte count (includes the 2-byte count field itself)
            const int cmd_count = (chunk[8] << 8) | chunk[9];
            const u8* cmd = chunk + 10;
            const u8* cmd_end = chunk + 8 + cmd_count;
            if (cmd_end > chunk_end || cmd_end < cmd)
                cmd_end = chunk_end;
            const u8* val = cmd_end;
            const u8* val_end = chunk_end;

            int repeatCount = 0;
            int repeatValue = -1; // -1 means a run of literals

            auto readValue = [&]() -> int
            {
                if (val + 2 > val_end)
                    return -1;
                int v = (val[0] << 8) | val[1];
                val += 2;
                return v;
            };

            auto readCommand = [&]() -> bool
            {
                if (cmd >= cmd_end)
                    return false;
                int b = *cmd++;
                if (b < 128)
                {
                    if (b == 0 || b == 1)
                    {
                        repeatCount = readValue();
                        if (repeatCount < 0)
                            return false;
                    }
                    else
                    {
                        repeatCount = b;
                    }
                    repeatValue = (b == 0) ? -1 : readValue();
                }
                else
                {
                    repeatCount = 256 - b;
                    repeatValue = -1;
                }
                return true;
            };

            auto readWord = [&]() -> int
            {
                while (repeatCount == 0)
                {
                    if (!readCommand())
                        return -1;
                }
                --repeatCount;
                if (repeatValue >= 0)
                    return repeatValue;
                return readValue();
            };

            // each VDAT column is a vertical 16-pixel strip filled top to bottom
            for (int w = 0; w < words_per_row; ++w)
            {
                for (int y = 0; y < height; ++y)
                {
                    int word = readWord();
                    if (word < 0)
                        return false; // partial decode: caller renders what we have

                    size_t offset = size_t(y) * scan_size + size_t(plane) * rowbytes + size_t(w) * 2;
                    buffer[offset] = u8(word >> 8);
                    buffer[offset + 1] = u8(word & 0xff);
                }
            }

            chunk = chunk_end;
        }

        return true;
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

    // HAM (Hold-And-Modify) planar-to-chunky writer. A scanline's "base" colours come from
    // palettes[clamp(y >> line_shift)]: standard HAM passes a single palette (num_palettes
    // == 1), while SHAM (Sliced HAM) supplies one palette per scanline (line_shift 0) or per
    // two scanlines when interlaced (line_shift 1).
    static
    void p2c_ham(const Surface& surface, const u8* workptr, int nplanes,
                 const Palette* palettes, int num_palettes, int line_shift)
    {
        bool hamcode2b = (nplanes == 6 || nplanes == 8);
        int ham_shift = 8 - (nplanes - (hamcode2b ? 2 : 1));
        //int ham_mask = (1 << ham_shift) - 1;

        int lineskip = ((surface.width + 15) >> 4) << 1;

        for (int y = 0; y < surface.height; ++y)
        {
            int pidx = y >> line_shift;
            if (pidx >= num_palettes)
                pidx = num_palettes - 1;
            const Palette& palette = palettes[pidx];

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

    // HAM-E (Black Belt Systems) is a "black box" that sits between the Amiga and the
    // display and reinterprets a plain 4-bitplane hires ILBM. Two hires pixels combine into
    // one 8-bit HAM-E byte, so the visible image is half as wide. The hardware reads the
    // IRGB colour lines (not the bitplane indices), so the real HAM-E nibble is recovered
    // from the CMAP colour each index maps to.
    //
    // A scanline whose first eight HAM-E bytes match the "magic cookie" is a trigger line:
    // it programs 64 colour registers (mode 20 = register / 256-colour, mode 24 = HAM) and
    // is itself displayed as black. Consecutive trigger lines fill successive register banks.
    // Interlaced sources duplicate the trigger lines per field, so even/odd scanlines keep
    // independent register sets.
    static inline
    int hameNibble(const u8* line, int rowbytes, int nplanes, int px, const Palette& palette)
    {
        int idx = 0;
        const int byte = px >> 3;
        const int bit = 7 - (px & 7);
        for (int p = 0; p < nplanes; ++p)
        {
            idx |= ((line[p * rowbytes + byte] >> bit) & 1) << p;
        }
        if (idx >= int(palette.size))
            idx = int(palette.size) - 1;
        const Color c = palette[idx];
        return ((c.r >> 7) & 1) << 3 | ((c.g >> 7) & 1) << 2 | ((c.b >> 7) & 1) << 1 | ((c.b >> 4) & 1);
    }

    static inline
    int hameByte(const u8* line, int rowbytes, int nplanes, int x, const Palette& palette)
    {
        return (hameNibble(line, rowbytes, nplanes, 2 * x + 0, palette) << 4)
             | (hameNibble(line, rowbytes, nplanes, 2 * x + 1, palette) << 0);
    }

    static
    bool isHameCookie(const u8* line, int rowbytes, int nplanes, const Palette& palette, int& mode)
    {
        static const u8 MAGIC[7] = { 162, 245, 132, 220, 109, 176, 127 };
        for (int i = 0; i < 7; ++i)
        {
            if (hameByte(line, rowbytes, nplanes, i, palette) != MAGIC[i])
                return false;
        }
        mode = hameByte(line, rowbytes, nplanes, 7, palette);
        return mode == 20 || mode == 24; // 20: register (256 colours), 24: HAM
    }

    // PCHG (Palette Change): per-scanline palette modifications layered on top of the base
    // CMAP. Supports the uncompressed and Huffman-compressed variants and both the 12-bit
    // (OCS / SmallLineChanges) and 32-bit (BigLineChanges) change records.
    struct PchgState
    {
        const u8* content = nullptr;
        size_t length = 0;
        size_t offset = 0;          // read cursor into the change data / compressed stream

        bool compressed = false;
        bool ocs = true;            // 12-bit OCS records vs 32-bit records
        u32 startLine = 0;
        int lineCount = 0;
        std::vector<u8> haveChange; // bitmask: which lines carry changes

        // Huffman state
        size_t treeOffset = 0;
        size_t treeLastOffset = 0;
        int bits = 0;               // MSB-first bit register (with sentinel)

        int readBit()
        {
            if ((bits & 127) == 0)
            {
                if (offset >= length)
                    return -1;
                bits = (content[offset++] << 1) | 1;
            }
            else
            {
                bits <<= 1;
            }
            return (bits >> 8) & 1;
        }

        int readHuffman()
        {
            size_t o = treeLastOffset;
            for ( ; ; )
            {
                int bit = readBit();
                if (bit == 0)
                {
                    o -= 2;
                    if (o < treeOffset)
                        return -1;
                    if ((content[o] & 129) == 1)
                        return content[o + 1];
                }
                else if (bit == 1)
                {
                    if (o + 1 >= length)
                        return -1;
                    int hi = content[o];
                    int lo = content[o + 1];
                    if (hi < 128)
                        return lo;
                    o += s32((hi - 256) << 8 | lo);
                    if (o < treeOffset || o > treeLastOffset)
                        return -1;
                }
                else
                {
                    return -1;
                }
            }
        }

        int readByteRaw()
        {
            if (offset >= length)
                return -1;
            return content[offset++];
        }

        int unpackByte()
        {
            return compressed ? readHuffman() : readByteRaw();
        }

        bool init(ConstMemory memory)
        {
            content = memory.address;
            length = memory.size;

            if (length < 20 || content[0] != 0)
                return false;

            switch (content[3] & 3)
            {
                case 1: ocs = true;  break; // SmallLineChanges (12-bit)
                case 2: ocs = false; break; // BigLineChanges (32-bit)
                default: return false;
            }

            startLine = u32(content[4] << 8 | content[5]); // unsigned: negative start disables
            lineCount = content[6] << 8 | content[7];

            const int haveLen = ((lineCount + 31) >> 5) << 2;
            haveChange.resize(haveLen);

            switch (content[1])
            {
                case 0: // uncompressed
                {
                    offset = 20;
                    if (offset + size_t(haveLen) > length)
                        return false;
                    std::memcpy(haveChange.data(), content + offset, haveLen);
                    offset += haveLen;
                    compressed = false;
                    break;
                }

                case 1: // Huffman
                {
                    treeOffset = 28;
                    if (treeOffset > length)
                        return false;

                    u32 treeLength = bigEndian::uload32(content + 20);
                    if (treeLength < 2 || treeLength > 1022)
                        return false;

                    offset = treeOffset + treeLength;
                    if (offset > length)
                        return false;

                    treeLastOffset = offset - 2;

                    for (int i = 0; i < haveLen; ++i)
                    {
                        int b = readHuffman();
                        if (b < 0)
                            return false;
                        haveChange[i] = u8(b);
                    }
                    compressed = true;
                    break;
                }

                default:
                    return false;
            }

            return true;
        }

        void setOcsColors(Palette& pal, int base, int count)
        {
            while (--count >= 0)
            {
                int rr = unpackByte();
                if (rr < 0)
                    return;
                int gb = unpackByte();
                if (gb < 0)
                    return;

                int reg = base + (rr >> 4);
                if (reg < int(pal.size))
                {
                    pal[reg] = Color((rr & 15) * 17, ((gb >> 4) & 15) * 17, (gb & 15) * 17, 0xff);
                }
            }
        }

        // Apply this scanline's cumulative palette changes. Must be called for every line in
        // order (0 .. height-1) so the change stream stays in sync.
        void applyLine(int y, Palette& pal)
        {
            int yy = y - int(startLine);
            if (yy < 0 || yy >= lineCount)
                return;

            if (((haveChange[yy >> 3] >> (~yy & 7)) & 1) == 0)
                return;

            int count = unpackByte();
            if (count < 0)
                return;
            int count2 = unpackByte();
            if (count2 < 0)
                return;

            if (ocs)
            {
                setOcsColors(pal, 0, count);
                setOcsColors(pal, 16, count2);
            }
            else
            {
                int n = count << 8 | count2;
                while (--n >= 0)
                {
                    if (unpackByte() != 0)
                        return;
                    int c = unpackByte();
                    if (c < 0)
                        return;
                    if (unpackByte() < 0) // alpha (ignored)
                        return;
                    int r = unpackByte();
                    if (r < 0)
                        return;
                    int b = unpackByte();
                    if (b < 0)
                        return;
                    int g = unpackByte();
                    if (g < 0)
                        return;
                    if (c < int(pal.size))
                        pal[c] = Color(r, g, b, 0xff);
                }
            }
        }
    };

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
        u8 compression = 0; // 0: None, 1: ByteRun1, 2: Atari ST VDAT vertical RLE
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
        u32 m_camg = 0;
        bool m_ham = false;
        bool m_ehb = false;
        Palette m_palette;

        // HAM-E (Black Belt Systems): trigger-line programmed 4-plane hires ILBM
        bool m_hame = false;
        bool m_hame_lace = false;

        // SHAM (Sliced HAM): per-scanline 16-colour palettes
        bool m_sham = false;
        std::vector<Palette> m_sham_palettes;

        // PCHG (Palette Change): per-scanline palette modifications
        bool m_pchg_present = false;
        ConstMemory m_pchg;

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
                            m_camg = v;
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

                    case u32_mask_rev('S','H','A','M'):
                    {
                        // Sliced HAM: a version word followed by 16-colour palettes, one per
                        // scanline (or per two scanlines when interlaced). Colours are Amiga
                        // 0x0RGB words (4 bits per channel).
                        if (payload >= 2 + 32)
                        {
                            p.read16(); // version

                            size_t count = (payload - 2) / 32;
                            m_sham_palettes.resize(count);

                            for (size_t i = 0; i < count; ++i)
                            {
                                Palette& pal = m_sham_palettes[i];
                                pal.size = 16;
                                for (int j = 0; j < 16; ++j)
                                {
                                    u16 w = p.read16();
                                    u32 r = (w >> 8) & 0xf;
                                    u32 g = (w >> 4) & 0xf;
                                    u32 b = (w >> 0) & 0xf;
                                    pal[j] = Color(r * 0x11, g * 0x11, b * 0x11, 0xff);
                                }
                            }

                            m_sham = true;
                            m_ham = true; // SHAM is always HAM

                            printLine(Print::Info, "  sham: {} palettes", count);
                        }
                        break;
                    }

                    case u32_mask_rev('P','C','H','G'):
                    {
                        m_pchg = ConstMemory(p, payload);
                        m_pchg_present = true;
                        printLine(Print::Info, "  pchg: {} bytes", payload);
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

            // HAM-E reinterprets a 4-plane hires ILBM; detection needs the unpacked BODY.
            detectHame();

            // PCHG modifies the palette per scanline, so the output must be true colour.
            if (m_pchg_present && !m_ham && !m_hame && header.success)
            {
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
        }

        // Probe the first scanline for the HAM-E magic cookie. When present the output is
        // true colour and half the stored width (two hires pixels per HAM-E byte).
        void detectHame()
        {
            if (m_signature != SIGNATURE_IFF)
                return;
            if (m_ham || m_ehb)
                return;
            if (m_bmhd.nplanes != 4 || m_bmhd.masking == 1)
                return;
            if (m_bmhd.xsize < 400 || m_palette.size < 16)
                return;
            if (!m_body.address)
                return;

            const int planar_w = m_bmhd.xsize;
            const int rowbytes = ((planar_w + 15) >> 4) << 1;
            const size_t scan = size_t(rowbytes) * m_bmhd.nplanes;

            // unpack only the first scanline for the cookie probe
            std::vector<u8> first(scan, 0);
            if (m_bmhd.compression == 1)
            {
                unpack(Memory(first.data(), scan), m_body);
            }
            else
            {
                std::memcpy(first.data(), m_body.address, std::min<size_t>(scan, m_body.size));
            }

            int mode = 0;
            if (isHameCookie(first.data(), rowbytes, m_bmhd.nplanes, m_palette, mode))
            {
                m_hame = true;
                m_hame_lace = (m_camg & 0x0004) != 0; // interlaced: per-field register sets
                header.width = planar_w / 2;
                header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
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

        // Dispatch HAM rendering: SHAM supplies per-scanline palettes, plain HAM a single one.
        void hamWrite(const Surface& target, const u8* buffer)
        {
            if (m_sham && !m_sham_palettes.empty())
            {
                int num = int(m_sham_palettes.size());
                int line_shift = (m_bmhd.ysize > num) ? 1 : 0; // interlaced: one palette / 2 lines
                p2c_ham(target, buffer, m_bmhd.nplanes, m_sham_palettes.data(), num, line_shift);
            }
            else
            {
                p2c_ham(target, buffer, m_bmhd.nplanes, &m_palette, 1, 0);
            }
        }

        // PCHG render: convert planar data to indices, then resolve each scanline through a
        // running palette that accumulates the per-line PCHG changes. Output is true colour.
        bool renderPchg(const Surface& dest, const u8* buffer, int xsize, int ysize)
        {
            // planar -> 8-bit indices
            Bitmap indexed(xsize, ysize, IndexedFormat(8));
            p2c_raw(indexed, buffer, m_bmhd.nplanes, m_bmhd.masking);

            // running palette seeded from the base CMAP
            Palette pal;
            pal.size = 256;
            for (int i = 0; i < 256; ++i)
            {
                pal[i] = (i < int(m_palette.size)) ? m_palette[i] : Color(0, 0, 0, 0xff);
            }

            PchgState pchg;
            bool ok = pchg.init(m_pchg);

            DecodeTargetBitmap target(dest, xsize, ysize, header.format);

            for (int y = 0; y < ysize; ++y)
            {
                if (ok)
                {
                    pchg.applyLine(y, pal);
                }

                const u8* idx = indexed.address<u8>(0, y);
                Color* d = target.address<Color>(0, y);
                for (int x = 0; x < xsize; ++x)
                {
                    d[x] = pal[idx[x]];
                }
            }

            bool direct = target.isDirect();
            target.resolve();
            return direct;
        }

        // HAM-E render: walk scanlines top-to-bottom, programming colour registers from
        // trigger lines and resolving every other line through the current register set in
        // either register (256-colour) or HAM mode. Output is true colour, half width.
        bool renderHame(const Surface& dest, const u8* buffer)
        {
            const int planar_w = m_bmhd.xsize;
            const int height = m_bmhd.ysize;
            const int nplanes = m_bmhd.nplanes;
            const int rowbytes = ((planar_w + 15) >> 4) << 1;
            const size_t scan = size_t(rowbytes) * nplanes;
            const int out_w = planar_w / 2;
            const bool lace = m_hame_lace;

            // per-field register banks (interlaced sources keep even/odd sets separate)
            u32 reg[2][256] = { { 0 }, { 0 } };
            int regfill[2] = { 0, 0 };
            bool hammode = false;

            DecodeTargetBitmap target(dest, out_w, height, header.format);

            for (int y = 0; y < height; ++y)
            {
                const u8* line = buffer + size_t(y) * scan;
                const int s = lace ? (y & 1) : 0;

                Color* dst = target.address<Color>(0, y);

                int mode = 0;
                if (isHameCookie(line, rowbytes, nplanes, m_palette, mode))
                {
                    const int off = regfill[s];
                    for (int c = 0; c < 64; ++c)
                    {
                        const int o = 8 + c * 3;
                        u32 r = hameByte(line, rowbytes, nplanes, o + 0, m_palette);
                        u32 g = hameByte(line, rowbytes, nplanes, o + 1, m_palette);
                        u32 b = hameByte(line, rowbytes, nplanes, o + 2, m_palette);
                        reg[s][(off + c) & 255] = (r << 16) | (g << 8) | b;
                    }
                    regfill[s] = (regfill[s] + 64) & 255;
                    hammode = (mode == 24);

                    // trigger lines are displayed as black
                    for (int x = 0; x < out_w; ++x)
                    {
                        dst[x] = Color(0, 0, 0, 0xff);
                    }
                    continue;
                }

                int bank = 0;
                u32 rgb = 0;
                const u32* palette = reg[s];

                for (int x = 0; x < out_w; ++x)
                {
                    const int c = hameByte(line, rowbytes, nplanes, x, m_palette);

                    if (hammode)
                    {
                        switch (c >> 6)
                        {
                            case 0:
                                if (c < 60)
                                    rgb = palette[(bank + c) & 255];
                                else
                                    bank = (c - 60) << 6; // 60..63 select register bank 0..3
                                break;
                            case 1: rgb = ((c & 63) <<  2) | (rgb & 0xffff00); break; // blue
                            case 2: rgb = ((c & 63) << 18) | (rgb & 0x00ffff); break; // red
                            default: rgb = ((c & 63) << 10) | (rgb & 0xff00ff); break; // green
                        }
                    }
                    else
                    {
                        rgb = palette[c];
                    }

                    dst[x] = Color((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, 0xff);
                }
            }

            target.resolve();
            return target.isDirect();
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
                hamWrite(target, buffer);
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
            else if (m_bmhd.compression == 2)
            {
                // Atari ST VDAT vertical RLE (one VDAT chunk per bitplane)
                size_t scan_size = ((xsize + 15) & ~15) / 8 * m_bmhd.nplanes;
                size_t bytes = scan_size * ysize;

                allocation.reset(new u8[bytes]());
                u8* allocated = allocation.get();

                decode_vdat(allocated, scan_size, m_body, xsize, ysize, m_bmhd.nplanes);
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

            // HAM-E reinterprets the planar buffer entirely (true colour, half width)
            if (m_hame)
            {
                status.direct = renderHame(dest, buffer);
                return status;
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

            // PCHG: per-scanline palette changes resolved to true colour
            if (m_pchg_present && !m_ham && !is_pbm)
            {
                status.direct = renderPchg(dest, buffer, xsize, ysize);
                return status;
            }

            DecodeTargetBitmap target(dest, xsize, ysize, header.format, m_palette);

            if (m_ham)
            {
                hamWrite(target, buffer);
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
