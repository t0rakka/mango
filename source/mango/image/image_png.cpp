/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

#include "../../external/zlib/zlib.h"

#ifdef MANGO_ENABLE_ISAL
    #ifdef MANGO_COMPILER_MICROSOFT
        #include <isal/igzip_lib.h>
    #else
        #include <isa-l.h>
    #endif
#endif

// https://www.w3.org/TR/2003/REC-PNG-20031110/
// https://wiki.mozilla.org/APNG_Specification
// https://datatracker.ietf.org/doc/html/rfc1951

// MANGO TODO HDR support: "cICP", "iCCN"
// https://github.com/w3c/ColorWeb-CG/blob/master/hdr-in-png-requirements.md#cicp-chunk

namespace fpng
{
    /*
        This is free and unencumbered software released into the public domain.

        Anyone is free to copy, modify, publish, use, compile, sell, or
        distribute this software, either in source code form or as a compiled
        binary, for any purpose, commercial or non-commercial, and by any
        means.

        In jurisdictions that recognize copyright laws, the author or authors
        of this software dedicate any and all copyright interest in the
        software to the public domain. We make this dedication for the benefit
        of the public at large and to the detriment of our heirs and
        successors. We intend this dedication to be an overt act of
        relinquishment in perpetuity of all present and future rights to this
        software under copyright law.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
        EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
        MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
        IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
        OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
        ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
        OTHER DEALINGS IN THE SOFTWARE.

        For more information, please refer to <http://unlicense.org/>

        Richard Geldreich, Jr.
        12/30/2021
    */
    /*
        Integration with mango includes some modifications:
        - use the existing architecture neutral load/store
        - defer Adler32 checksum computation to the caller
    */
    using namespace mango;

    static const uint16_t g_defl_len_sym[256] = {
      257,258,259,260,261,262,263,264,265,265,266,266,267,267,268,268,269,269,269,269,270,270,270,270,271,271,271,271,272,272,272,272,
      273,273,273,273,273,273,273,273,274,274,274,274,274,274,274,274,275,275,275,275,275,275,275,275,276,276,276,276,276,276,276,276,
      277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
      279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,280,280,280,280,280,280,280,280,280,280,280,280,280,280,280,280,
      281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,
      282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,
      283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,
      284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,285 };

    static const uint8_t g_defl_len_extra[256] = {
      0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
      5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,0 };

    static const uint32_t g_bitmasks[17] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

    static const uint8_t g_dyn_huff_4[] = {
    120, 1, 229, 196, 99, 180, 37, 103, 218, 128, 225, 251, 121, 171, 106, 243, 216, 231, 180, 109, 196, 182, 51, 51, 73, 6, 201, 216, 182, 109, 219, 182,
    17, 140, 98, 219, 102, 219, 60, 125, 172, 205, 170, 122, 159, 111, 213, 143, 179, 214, 94, 189, 58, 153, 104, 166, 103, 190, 247, 199, 117 };
    const uint32_t DYN_HUFF_4_BITBUF = 1, DYN_HUFF_4_BITBUF_SIZE = 2;
    static const struct { uint8_t m_code_size; uint16_t m_code; } g_dyn_huff_4_codes[288] = {
    {2,0},{4,2},{5,6},{6,30},{6,62},{6,1},{7,41},{7,105},{7,25},{7,89},{7,57},{7,121},{8,117},{8,245},{8,13},{8,141},{8,77},{8,205},{8,45},{8,173},{8,109},{8,237},{8,29},{8,157},{8,93},{8,221},{8,61},{9,83},{9,339},{9,211},{9,467},{9,51},
    {9,307},{9,179},{9,435},{9,115},{9,371},{9,243},{9,499},{9,11},{9,267},{9,139},{9,395},{9,75},{9,331},{9,203},{9,459},{9,43},{9,299},{10,7},{10,519},{10,263},{10,775},{10,135},{10,647},{10,391},{10,903},{10,71},{10,583},{10,327},{10,839},{10,199},{10,711},{10,455},
    {10,967},{10,39},{10,551},{10,295},{10,807},{10,167},{10,679},{10,423},{10,935},{10,103},{10,615},{11,463},{11,1487},{11,975},{10,359},{10,871},{10,231},{11,1999},{11,47},{11,1071},{11,559},{10,743},{10,487},{11,1583},{11,303},{11,1327},{11,815},{11,1839},{11,175},{11,1199},{11,687},{11,1711},
    {11,431},{11,1455},{11,943},{11,1967},{11,111},{11,1135},{11,623},{11,1647},{11,367},{11,1391},{11,879},{11,1903},{11,239},{11,1263},{11,751},{11,1775},{11,495},{11,1519},{11,1007},{11,2031},{11,31},{11,1055},{11,543},{11,1567},{11,287},{11,1311},{11,799},{11,1823},{11,159},{11,1183},{11,671},{11,1695},
    {11,415},{11,1439},{11,927},{11,1951},{11,95},{11,1119},{11,607},{11,1631},{11,351},{11,1375},{11,863},{11,1887},{11,223},{11,1247},{11,735},{11,1759},{11,479},{11,1503},{11,991},{11,2015},{11,63},{11,1087},{11,575},{11,1599},{11,319},{11,1343},{11,831},{11,1855},{11,191},{11,1215},{11,703},{11,1727},
    {11,447},{11,1471},{11,959},{11,1983},{11,127},{11,1151},{11,639},{11,1663},{11,383},{10,999},{10,23},{10,535},{10,279},{11,1407},{11,895},{11,1919},{11,255},{11,1279},{10,791},{10,151},{10,663},{10,407},{10,919},{10,87},{10,599},{10,343},{10,855},{10,215},{10,727},{10,471},{10,983},{10,55},
    {10,567},{10,311},{10,823},{10,183},{10,695},{10,439},{10,951},{10,119},{10,631},{10,375},{10,887},{10,247},{10,759},{10,503},{10,1015},{10,15},{10,527},{10,271},{10,783},{10,143},{10,655},{10,399},{9,171},{9,427},{9,107},{9,363},{9,235},{9,491},{9,27},{9,283},{9,155},{9,411},
    {9,91},{9,347},{9,219},{9,475},{9,59},{9,315},{9,187},{9,443},{8,189},{9,123},{8,125},{8,253},{8,3},{8,131},{8,67},{8,195},{8,35},{8,163},{8,99},{8,227},{8,19},{7,5},{7,69},{7,37},{7,101},{7,21},{7,85},{6,33},{6,17},{6,49},{5,22},{4,10},
    {12,2047},{0,0},{6,9},{0,0},{0,0},{0,0},{8,147},{0,0},{0,0},{7,53},{0,0},{9,379},{0,0},{9,251},{10,911},{10,79},{11,767},{10,591},{10,335},{10,847},{10,207},{10,719},{11,1791},{11,511},{9,507},{11,1535},{11,1023},{12,4095},{5,14},{0,0},{0,0},{0,0}
    };

// compiler warning: always evaluates true (one of the asserts)
//#define PUT_BITS(bb, ll) do { uint32_t b = bb, l = ll; assert((l) >= 0 && (l) <= 16); assert((b) < (1ULL << (l))); bit_buf |= (((uint64_t)(b)) << bit_buf_size); bit_buf_size += (l); assert(bit_buf_size <= 64); } while(0)
#define PUT_BITS(bb, ll) do { uint32_t b = bb, l = ll; bit_buf |= (((uint64_t)(b)) << bit_buf_size); bit_buf_size += (l); } while(0)
#define PUT_BITS_CZ(bb, ll) do { uint32_t b = bb, l = ll; assert((l) >= 1 && (l) <= 16); assert((b) < (1ULL << (l))); bit_buf |= (((uint64_t)(b)) << bit_buf_size); bit_buf_size += (l); assert(bit_buf_size <= 64); } while(0)

#define PUT_BITS_FLUSH do { \
    if ((dst_ofs + 8) > dst_buf_size) \
        return 0; \
    littleEndian::ustore64(pDst + dst_ofs, bit_buf); \
    uint32_t bits_to_shift = bit_buf_size & ~7; \
    dst_ofs += (bits_to_shift >> 3); \
    assert(bits_to_shift < 64); \
    bit_buf = bit_buf >> bits_to_shift; \
    bit_buf_size -= bits_to_shift; \
} while(0)

#define PUT_BITS_FORCE_FLUSH do { \
    while (bit_buf_size > 0) \
    { \
        if ((dst_ofs + 1) > dst_buf_size) \
            return 0; \
        *(uint8_t*)(pDst + dst_ofs) = (uint8_t)bit_buf; \
        dst_ofs++; \
        bit_buf >>= 8; \
        bit_buf_size -= 8; \
    } \
} while(0)

    static
    u32 pixel_deflate_dyn_4_rle_one_pass(const u8* pImg, u32 w, u32 h, u8* pDst, u32 dst_buf_size)
    {
        const u32 bpl = 1 + w * 4;

        if (dst_buf_size < sizeof(g_dyn_huff_4))
            return false;
        memcpy(pDst, g_dyn_huff_4, sizeof(g_dyn_huff_4));
        u32 dst_ofs = sizeof(g_dyn_huff_4);

        u64 bit_buf = DYN_HUFF_4_BITBUF;
        int bit_buf_size = DYN_HUFF_4_BITBUF_SIZE;

        const u8* pSrc = pImg;
        u32 src_ofs = 0;

        for (u32 y = 0; y < h; y++)
        {
            const u32 end_src_ofs = src_ofs + bpl;

            const u32 filter_lit = pSrc[src_ofs++];
            PUT_BITS_CZ(g_dyn_huff_4_codes[filter_lit].m_code, g_dyn_huff_4_codes[filter_lit].m_code_size);

            PUT_BITS_FLUSH;

            u32 prev_lits;
            {
                u32 lits = littleEndian::uload32(pSrc + src_ofs);

                PUT_BITS_CZ(g_dyn_huff_4_codes[lits & 0xFF].m_code, g_dyn_huff_4_codes[lits & 0xFF].m_code_size);
                PUT_BITS_CZ(g_dyn_huff_4_codes[(lits >> 8) & 0xFF].m_code, g_dyn_huff_4_codes[(lits >> 8) & 0xFF].m_code_size);
                PUT_BITS_CZ(g_dyn_huff_4_codes[(lits >> 16) & 0xFF].m_code, g_dyn_huff_4_codes[(lits >> 16) & 0xFF].m_code_size);

                if (bit_buf_size >= 49)
                {
                    PUT_BITS_FLUSH;
                }

                PUT_BITS_CZ(g_dyn_huff_4_codes[(lits >> 24)].m_code, g_dyn_huff_4_codes[(lits >> 24)].m_code_size);

                src_ofs += 4;

                prev_lits = lits;
            }

            PUT_BITS_FLUSH;

            while (src_ofs < end_src_ofs)
            {
                u32 lits = littleEndian::uload32(pSrc + src_ofs);

                if (lits == prev_lits)
                {
                    u32 match_len = 4;
                    u32 max_match_len = std::min<int>(252, (int)(end_src_ofs - src_ofs));

                    while (match_len < max_match_len)
                    {
                        if (littleEndian::uload32(pSrc + src_ofs + match_len) != lits)
                            break;
                        match_len += 4;
                    }

                    u32 adj_match_len = match_len - 3;

                    const u32 match_code_bits = g_dyn_huff_4_codes[g_defl_len_sym[adj_match_len]].m_code_size;
                    const u32 len_extra_bits = g_defl_len_extra[adj_match_len];

                    if (match_len == 4)
                    {
                        // This check is optional - see if just encoding 4 literals would be cheaper than using a short match.
                        u32 lit_bits = g_dyn_huff_4_codes[lits & 0xFF].m_code_size + g_dyn_huff_4_codes[(lits >> 8) & 0xFF].m_code_size + 
                            g_dyn_huff_4_codes[(lits >> 16) & 0xFF].m_code_size + g_dyn_huff_4_codes[(lits >> 24)].m_code_size;

                        if ((match_code_bits + len_extra_bits + 1) > lit_bits)
                            goto do_literals;
                    }

                    PUT_BITS_CZ(g_dyn_huff_4_codes[g_defl_len_sym[adj_match_len]].m_code, match_code_bits);
                    PUT_BITS(adj_match_len & g_bitmasks[g_defl_len_extra[adj_match_len]], len_extra_bits + 1); // up to 6 bits, +1 for the match distance Huff code which is always 0

                    src_ofs += match_len;
                }
                else
                {
do_literals:
                    PUT_BITS_CZ(g_dyn_huff_4_codes[lits & 0xFF].m_code, g_dyn_huff_4_codes[lits & 0xFF].m_code_size);
                    PUT_BITS_CZ(g_dyn_huff_4_codes[(lits >> 8) & 0xFF].m_code, g_dyn_huff_4_codes[(lits >> 8) & 0xFF].m_code_size);
                    PUT_BITS_CZ(g_dyn_huff_4_codes[(lits >> 16) & 0xFF].m_code, g_dyn_huff_4_codes[(lits >> 16) & 0xFF].m_code_size);

                    if (bit_buf_size >= 49)
                    {
                        PUT_BITS_FLUSH;
                    }

                    PUT_BITS_CZ(g_dyn_huff_4_codes[(lits >> 24)].m_code, g_dyn_huff_4_codes[(lits >> 24)].m_code_size);

                    src_ofs += 4;

                    prev_lits = lits;
                }

                PUT_BITS_FLUSH;

            } // while (src_ofs < end_src_ofs)

        } // y

        assert(src_ofs == h * bpl);

        assert(bit_buf_size <= 7);

        PUT_BITS_CZ(g_dyn_huff_4_codes[256].m_code, g_dyn_huff_4_codes[256].m_code_size);

        PUT_BITS_FORCE_FLUSH;

        return dst_ofs;
    }

    static
    u32 write_raw_block(const u8* pSrc, u32 src_len, u8* pDst, u32 dst_buf_size)
    {
        if (dst_buf_size < 2)
            return 0;

        pDst[0] = 0x78;
        pDst[1] = 0x01;

        u32 dst_ofs = 2;

        u32 src_ofs = 0;
        while (src_ofs < src_len)
        {
            const u32 src_remaining = src_len - src_ofs;
            const u32 block_size = std::min<u32>(UINT16_MAX, src_remaining);
            const bool final_block = (block_size == src_remaining);

            if ((dst_ofs + 5 + block_size) > dst_buf_size)
                return 0;

            pDst[dst_ofs + 0] = final_block ? 1 : 0;

            pDst[dst_ofs + 1] = block_size & 0xFF;
            pDst[dst_ofs + 2] = (block_size >> 8) & 0xFF;

            pDst[dst_ofs + 3] = (~block_size) & 0xFF;
            pDst[dst_ofs + 4] = ((~block_size) >> 8) & 0xFF;

            memcpy(pDst + dst_ofs + 5, pSrc + src_ofs, block_size);

            src_ofs += block_size;
            dst_ofs += 5 + block_size;
        }

        return dst_ofs;
    }

} // namespace fpng

namespace
{
    using namespace mango;
    using namespace mango::image;
    using namespace mango::math;

    static constexpr int PNG_SIMD_PADDING = 16;
    static constexpr int PNG_FILTER_BYTE = 1;
    static constexpr u64 PNG_HEADER_MAGIC = 0x89504e470d0a1a0a;

    enum ColorType
    {
        COLOR_TYPE_I       = 0,
        COLOR_TYPE_RGB     = 2,
        COLOR_TYPE_PALETTE = 3,
        COLOR_TYPE_IA      = 4,
        COLOR_TYPE_RGBA    = 6,
    };

    inline
    const char* get_string(ColorType type)
    {
        const char* text = "INVALID";
        switch (type)
        {
            case COLOR_TYPE_I:
                text = "I";
                break;
            case COLOR_TYPE_RGB:
                text = "RGB";
                break;
            case COLOR_TYPE_PALETTE:
                text = "PALETTE";
                break;
            case COLOR_TYPE_IA:
                text = "IA";
                break;
            case COLOR_TYPE_RGBA:
                text = "RGBA";
                break;
            default:
                break;
        }
        return text;
    }

    // ------------------------------------------------------------
    // Filter
    // ------------------------------------------------------------

    enum FilterType : u8
    {
        FILTER_NONE    = 0,
        FILTER_SUB     = 1,
        FILTER_UP      = 2,
        FILTER_AVERAGE = 3,
        FILTER_PAETH   = 4,
    };

    using FilterFunc = void (*)(u8* scan, const u8* prev, int bytes, int bpp);

    void filter1_sub(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(prev);

        for (int x = bpp; x < bytes; ++x)
        {
            scan[x] += scan[x - bpp];
        }
    }

    void filter2_up(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        for (int x = 0; x < bytes; ++x)
        {
            scan[x] += prev[x];
        }
    }

    void filter3_average(u8* scan, const u8* prev, int bytes, int bpp)
    {
        for (int x = 0; x < bpp; ++x)
        {
            scan[x] += prev[x] / 2;
        }

        for (int x = bpp; x < bytes; ++x)
        {
            scan[x] += (prev[x] + scan[x - bpp]) / 2;
        }
    }

    void filter4_paeth(u8* scan, const u8* prev, int bytes, int bpp)
    {
        for (int x = 0; x < bpp; ++x)
        {
            scan[x] += prev[x];
        }

        for (int x = bpp; x < bytes; ++x)
        {
            int c = prev[x - bpp];
            int a = scan[x - bpp];
            int b = prev[x];

            int p = b - c;
            int q = a - c;
            int pa = std::abs(p);
            int pb = std::abs(q);
            int pc = std::abs(p + q);

            if (pb < pa)
            {
                pa = pb;
                a = b;
            }

            if (pc < pa)
            {
                a = c;
            }

            scan[x] += u8(a);
        }
    }

    void filter4_paeth_8bit(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        int c = prev[0];
        int a = (scan[0] + c) & 0xff;
        scan[0] = u8(a);

        for (int x = 1; x < bytes; ++x)
        {
            int b = prev[x];

            int p = b - c;
            int q = a - c;
            int pa = abs(p);
            int pb = abs(q);
            int pc = abs(p + q);

            if (pb < pa)
            {
                pa = pb;
                a = b;
            }

            if (pc < pa)
            {
                a = c;
            }

            c = b;
            a = (a + scan[x]) & 0xff;
            scan[x] = u8(a);
        }
    }

#if defined(MANGO_ENABLE_SSE2)

    // -----------------------------------------------------------------------------------
    // SSE2 Filters
    // -----------------------------------------------------------------------------------

    // helper functions

    inline __m128i load4(const void* p)
    {
        u32 temp = uload32(p);
        return _mm_cvtsi32_si128(temp);
    }

    inline void store4(void* p, __m128i v)
    {
        u32 temp = _mm_cvtsi128_si32(v);
        ustore32(p, temp);
    }

    inline __m128i load3(const void* p)
    {
        // NOTE: we have PNG_SIMD_PADDING for overflow guardband
        u32 temp = uload32(p);
        return _mm_cvtsi32_si128(temp);
    }

    inline void store3(void* p, __m128i v)
    {
        u32 temp = _mm_cvtsi128_si32(v);
        std::memcpy(p, &temp, 3);
    }

    inline __m128i average_sse2(__m128i a, __m128i b, __m128i d)
    {
        __m128i avg = _mm_avg_epu8(a, b);
        avg = _mm_sub_epi8(avg, _mm_and_si128(_mm_xor_si128(a, b), _mm_set1_epi8(1)));
        d = _mm_add_epi8(d, avg);
        return d;
    }

    inline int16x8 nearest_sse2(int16x8 a, int16x8 b, int16x8 c, int16x8 d)
    {
        int16x8 pa = b - c;
        int16x8 pb = a - c;
        int16x8 pc = pa + pb;
        pa = abs(pa);
        pb = abs(pb);
        pc = abs(pc);
        int16x8 smallest = min(pc, min(pa, pb));
        int16x8 nearest = select(smallest == pa, a,
                          select(smallest == pb, b, c));
        d = _mm_add_epi8(d, nearest);
        return d;
    }

    // filters

    void filter1_sub_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(prev);
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 3)
        {
            d = _mm_add_epi8(load3(scan + x), d);
            store3(scan + x, d);
        }
    }

    void filter1_sub_32bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(prev);
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        while (bytes >= 16)
        {
            __m128i b = _mm_loadu_si128(reinterpret_cast<__m128i*>(scan));
            d = _mm_add_epi8(d, b);
            d = _mm_add_epi8(d, _mm_bslli_si128(b, 4));
            d = _mm_add_epi8(d, _mm_bslli_si128(b, 8));
            d = _mm_add_epi8(d, _mm_bslli_si128(b, 12));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(scan), d);
            d = _mm_shuffle_epi32(d, 0xff);
            scan += 16;
            bytes -= 16;
        }

        while (bytes >= 4)
        {
            d = _mm_add_epi8(load4(scan), d);
            store4(scan, d);
            scan += 4;
            bytes -= 4;
        }
    }

    void filter2_up_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        while (bytes >= 16)
        {
            __m128i a = _mm_loadu_si128(reinterpret_cast<__m128i*>(scan));
            __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(prev));
            a = _mm_add_epi8(a, b);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(scan), a);
            scan += 16;
            prev += 16;
            bytes -= 16;
        }

        for (int x = 0; x < bytes; ++x)
        {
            scan[x] += prev[x];
        }
    }

    void filter3_average_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 3)
        {
            d = average_sse2(d, load3(prev + x), load3(scan + x));
            store3(scan + x, d);
        }
    }

    void filter3_average_32bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        for (int x = 0; x < bytes; x += 4)
        {
            d = average_sse2(d, load4(prev + x), load4(scan + x));
            store4(scan + x, d);
        }
    }

    void filter4_paeth_24bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        int8x16 zero = 0;
        int16x8 b = 0;
        int16x8 d = 0;

        for (int x = 0; x < bytes; x += 3)
        {
            int16x8 c = b;
            int16x8 a = d;
            b = _mm_unpacklo_epi8(load3(prev + x), zero);
            d = _mm_unpacklo_epi8(load3(scan + x), zero);
            d = nearest_sse2(a, b, c, d);
            store3(scan + x, _mm_packus_epi16(d, d));
        }
    }

    void filter4_paeth_32bit_sse2(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(bpp);

        int8x16 zero = 0;
        int16x8 b = 0;
        int16x8 d = 0;

        for (int x = 0; x < bytes; x += 4)
        {
            int16x8 c = b;
            int16x8 a = d;
            b = _mm_unpacklo_epi8(load4(prev + x), zero);
            d = _mm_unpacklo_epi8(load4(scan + x), zero);
            d = nearest_sse2(a, b, c, d);
            store4(scan + x, _mm_packus_epi16(d, d));
        }
    }

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_SSE4_1)

    // -----------------------------------------------------------------------------------
    // SSE4.1 Filters
    // -----------------------------------------------------------------------------------

    void filter1_sub_24bit_sse41(u8* scan, const u8* prev, int bytes, int bpp)
    {
        MANGO_UNREFERENCED(prev);
        MANGO_UNREFERENCED(bpp);

        __m128i d = _mm_setzero_si128();

        __m128i shuf0 = _mm_set_epi8(-1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0);
        __m128i shuf1 = _mm_set_epi8(4, 2, 1, 0, -1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5);
        __m128i shuf2 = _mm_set_epi8(9, 8, 6, 5, 4, 2, 1, 0, -1, -1, -1, -1, 14, 13, 12, 10);
        __m128i shuf3 = _mm_set_epi8(14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0, -1, -1, -1, -1);
        __m128i shuf4 = _mm_set_epi8(-1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0);
        __m128i mask0 = _mm_set_epi32(0, -1, -1, -1);
        __m128i mask1 = _mm_set_epi32(0,  0, -1, -1);
        __m128i mask2 = _mm_set_epi32(0,  0,  0, -1);

        while (bytes >= 48)
        {
            // load
            __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(scan + 0 * 16));
            __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(scan + 1 * 16));
            __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(scan + 2 * 16));

            // unpack
            __m128i color0 = _mm_shuffle_epi8(v0, shuf4);
            __m128i color1 = _mm_shuffle_epi8(_mm_alignr_epi8(v1, v0, 12), shuf4);
            __m128i color2 = _mm_shuffle_epi8(_mm_alignr_epi8(v2, v1,  8), shuf4);
            __m128i color3 = _mm_shuffle_epi8(_mm_alignr_epi8(v2, v2,  4), shuf4);

            d = _mm_add_epi8(d, color0);
            d = _mm_add_epi8(d, _mm_bslli_si128(color0, 4));
            d = _mm_add_epi8(d, _mm_bslli_si128(color0, 8));
            d = _mm_add_epi8(d, _mm_bslli_si128(color0, 12));
            color0 = _mm_shuffle_epi8(d, shuf0);
            d = _mm_shuffle_epi32(d, 0xff);

            d = _mm_add_epi8(d, color1);
            d = _mm_add_epi8(d, _mm_bslli_si128(color1, 4));
            d = _mm_add_epi8(d, _mm_bslli_si128(color1, 8));
            d = _mm_add_epi8(d, _mm_bslli_si128(color1, 12));
            color1 = _mm_shuffle_epi8(d, shuf1);
            d = _mm_shuffle_epi32(d, 0xff);

            d = _mm_add_epi8(d, color2);
            d = _mm_add_epi8(d, _mm_bslli_si128(color2, 4));
            d = _mm_add_epi8(d, _mm_bslli_si128(color2, 8));
            d = _mm_add_epi8(d, _mm_bslli_si128(color2, 12));
            color2 = _mm_shuffle_epi8(d, shuf2);
            d = _mm_shuffle_epi32(d, 0xff);

            d = _mm_add_epi8(d, color3);
            d = _mm_add_epi8(d, _mm_bslli_si128(color3, 4));
            d = _mm_add_epi8(d, _mm_bslli_si128(color3, 8));
            d = _mm_add_epi8(d, _mm_bslli_si128(color3, 12));
            color3 = _mm_shuffle_epi8(d, shuf3);
            d = _mm_shuffle_epi32(d, 0xff);

            // pack
            v0 = _mm_blendv_epi8(color1, color0, mask0);
            v1 = _mm_blendv_epi8(color2, color1, mask1);
            v2 = _mm_blendv_epi8(color3, color2, mask2);

            // store
            _mm_storeu_si128(reinterpret_cast<__m128i *>(scan + 0 * 16), v0);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(scan + 1 * 16), v1);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(scan + 2 * 16), v2);

            scan += 48;
            bytes -= 48;
        }

        for (int x = 0; x < bytes; x += 3)
        {
            d = _mm_add_epi8(load3(scan + x), d);
            store3(scan + x, d);
        }
    }

#endif // MANGO_ENABLE_SSE4_1

#if defined(MANGO_ENABLE_NEON)

    // -----------------------------------------------------------------------------------
    // NEON Filters
    // -----------------------------------------------------------------------------------

    // helper functions

    static inline
    uint8x16_t load12(const u8* ptr)
    {
        // NOTE: 4 byte overflow guarded by PNG_SIMD_PADDING
        return vld1q_u8(ptr);
    }

    static inline
    void store12(u8* dest, uint8x8_t v0, uint8x8_t v1)
    {
        vst1_u8(dest, v0);
        vst1_lane_u32(reinterpret_cast<u32 *>(dest + 8), vreinterpret_u32_u8(v1), 0);
    }

    static inline
    uint8x16_t splat(uint8x16_t v)
    {
#ifdef __aarch64__
        return vreinterpretq_u8_u32(vdupq_laneq_u32(vreinterpretq_u32_u8(v), 3));
#else
        return vreinterpretq_u8_u32(vdupq_n_u32(vgetq_lane_u32(vreinterpretq_u32_u8(v), 3)));
#endif
    }

    static inline
    void sub12(u8* scan, uint8x8_t& last)
    {
        uint8x16_t a = load12(scan);
        uint8x8_t a0 = vget_low_u8(a);
        uint8x8_t a1 = vget_high_u8(a);

        uint8x8_t s0 = a0;
        uint8x8_t s1 = vext_u8(a0, a1, 3);
        uint8x8_t s2 = vext_u8(a0, a1, 6);
        uint8x8_t s3 = vext_u8(a1, a1, 1);

        uint8x8_t v0 = vadd_u8(last, s0);
        uint8x8_t v1 = vadd_u8(v0, s1);
        uint8x8_t v2 = vadd_u8(v1, s2);
        uint8x8_t v3 = vadd_u8(v2, s3);

        last = v3;

        const uint8x8x4_t table =
        {
            v0, v1, v2, v3
        };

        const uint8x8_t idx0 = { 0, 1, 2, 8, 9, 10, 16, 17 };
        const uint8x8_t idx1 = { 18, 24, 25, 26, 255, 255, 255, 255 };

        a0 = vtbl4_u8(table, idx0);
        a1 = vtbl4_u8(table, idx1);

        store12(scan, a0, a1);
    }

    static inline
    void sub16(u8* scan, uint8x16_t& last)
    {
        const uint8x16_t zero = vdupq_n_u8(0);

        uint8x16_t a = vld1q_u8(scan);

        uint8x16_t d = vaddq_u8(a, last);
        d = vaddq_u8(d, vextq_u8(zero, a, 12));
        d = vaddq_u8(d, vextq_u8(zero, a, 8));
        d = vaddq_u8(d, vextq_u8(zero, a, 4));

        last = splat(d);

        vst1q_u8(scan, d);
    }

    static inline
    uint8x8_t average(uint8x8_t v, uint8x8_t a, uint8x8_t b)
    {
        return vadd_u8(vhadd_u8(v, b), a);
    }

    static inline
    void average12(u8* scan, const u8* prev, uint8x8_t& last)
    {
        // NOTE: 4 byte overflow guarded by PNG_SIMD_PADDING
        uint8x16_t a = vld1q_u8(scan);
        uint8x16_t b = vld1q_u8(prev);

        uint8x8_t a0 = vget_low_u8(a);
        uint8x8_t b0 = vget_low_u8(b);
        uint8x8_t b1 = vget_high_u8(b);
        uint8x8_t a1 = vget_high_u8(a);

        uint8x8_t s1 = vext_u8(a1, a1, 1);
        uint8x8_t t1 = vext_u8(b1, b1, 1);
        uint8x8_t a3 = vext_u8(a0, a1, 3);
        uint8x8_t b3 = vext_u8(b0, b1, 3);
        uint8x8_t a6 = vext_u8(a0, a1, 6);
        uint8x8_t b6 = vext_u8(b0, b1, 6);

        uint8x8x4_t value;
        value.val[0] = average(        last, a0, b0);
        value.val[1] = average(value.val[0], a3, b3);
        value.val[2] = average(value.val[1], a6, b6);
        value.val[3] = average(value.val[2], s1, t1);

        last = value.val[3];

        const uint8x8_t idx0 = { 0, 1, 2, 8, 9, 10, 16, 17 };
        const uint8x8_t idx1 = { 18, 24, 25, 26, 255, 255, 255, 255 };

        uint8x8_t v0 = vtbl4_u8(value, idx0);
        uint8x8_t v1 = vtbl4_u8(value, idx1);

        store12(scan, v0, v1);
    }

    static inline
    void average16(u8* scan, const u8* prev, uint8x8_t& last)
    {
        uint32x2x4_t tmp;

        tmp = vld4_lane_u32(reinterpret_cast<u32 *>(scan), tmp, 0);

        uint8x8x4_t a;
        a.val[0] = vreinterpret_u8_u32(tmp.val[0]);
        a.val[1] = vreinterpret_u8_u32(tmp.val[1]);
        a.val[2] = vreinterpret_u8_u32(tmp.val[2]);
        a.val[3] = vreinterpret_u8_u32(tmp.val[3]);

        tmp = vld4_lane_u32(reinterpret_cast<const u32 *>(prev), tmp, 0);

        uint8x8x4_t b;
        b.val[0] = vreinterpret_u8_u32(tmp.val[0]);
        b.val[1] = vreinterpret_u8_u32(tmp.val[1]);
        b.val[2] = vreinterpret_u8_u32(tmp.val[2]);
        b.val[3] = vreinterpret_u8_u32(tmp.val[3]);

        uint8x8x4_t value;
        value.val[0] = average(        last, a.val[0], b.val[0]);
        value.val[1] = average(value.val[0], a.val[1], b.val[1]);
        value.val[2] = average(value.val[1], a.val[2], b.val[2]);
        value.val[3] = average(value.val[2], a.val[3], b.val[3]);

        last = value.val[3];

        const uint8x8_t idx0 = { 0, 1, 2, 3, 8, 9, 10, 11 };
        const uint8x8_t idx1 = { 16, 17, 18, 19, 24, 25, 26, 27 };

        uint8x8_t v0 = vtbl4_u8(value, idx0);
        uint8x8_t v1 = vtbl4_u8(value, idx1);
        vst1q_u8(scan, vcombine_u8(v0, v1));
    }

    static inline
    uint8x8_t paeth(uint8x8_t a, uint8x8_t b, uint8x8_t c, uint8x8_t delta)
    {
        uint16x8_t pa = vabdl_u8(b, c);
        uint16x8_t pb = vabdl_u8(a, c);
        uint16x8_t pc = vabdq_u16(vaddl_u8(a, b), vaddl_u8(c, c));

        uint16x8_t ab = vcleq_u16(pa, pb);
        uint16x8_t ac = vcleq_u16(pa, pc);
        uint16x8_t bc = vcleq_u16(pb, pc);

        uint16x8_t abc = vandq_u16(ab, ac);

        uint8x8_t d = vmovn_u16(bc);
        uint8x8_t e = vmovn_u16(abc);

        d = vbsl_u8(d, b, c);
        e = vbsl_u8(e, a, d);

        e = vadd_u8(e, delta);

        return e;
    }

    static inline
    void paeth12(u8* scan, const u8* prev, uint8x8x4_t& value, uint8x8_t& last)
    {
        // NOTE: 4 byte overflow guarded by PNG_SIMD_PADDING
        uint8x8x2_t a = vld1_u8_x2(scan);
        uint8x8x2_t b = vld1_u8_x2(prev);

        uint8x8_t a0 = a.val[0];
        uint8x8_t b0 = b.val[0];
        uint8x8_t a1 = vext_u8(a.val[1], a.val[1], 1);
        uint8x8_t b1 = vext_u8(b.val[1], b.val[1], 1);
        uint8x8_t a3 = vext_u8(a.val[0], a.val[1], 3);
        uint8x8_t b3 = vext_u8(b.val[0], b.val[1], 3);
        uint8x8_t a6 = vext_u8(a.val[0], a.val[1], 6);
        uint8x8_t b6 = vext_u8(b.val[0], b.val[1], 6);

        value.val[0] = paeth(value.val[3], b0, last, a0);
        value.val[1] = paeth(value.val[0], b3, b0, a3);
        value.val[2] = paeth(value.val[1], b6, b3, a6);
        value.val[3] = paeth(value.val[2], b1, b6, a1);

        last = b1;

        const uint8x8_t idx0 = { 0, 1, 2, 8, 9, 10, 16, 17 };
        const uint8x8_t idx1 = { 18, 24, 25, 26, 255, 255, 255, 255 };

        uint8x8_t v0 = vtbl4_u8(value, idx0);
        uint8x8_t v1 = vtbl4_u8(value, idx1);

        store12(scan, v0, v1);
    }

    static inline
    void paeth16(u8* scan, const u8* prev, uint8x8x4_t& value, uint8x8_t& last)
    {
        uint32x2x4_t tmp;

        tmp = vld4_lane_u32(reinterpret_cast<u32 *>(scan), tmp, 0);

        uint8x8x4_t a;
        a.val[0] = vreinterpret_u8_u32(tmp.val[0]);
        a.val[1] = vreinterpret_u8_u32(tmp.val[1]);
        a.val[2] = vreinterpret_u8_u32(tmp.val[2]);
        a.val[3] = vreinterpret_u8_u32(tmp.val[3]);

        tmp = vld4_lane_u32(reinterpret_cast<const u32 *>(prev), tmp, 0);

        uint8x8x4_t b;
        b.val[0] = vreinterpret_u8_u32(tmp.val[0]);
        b.val[1] = vreinterpret_u8_u32(tmp.val[1]);
        b.val[2] = vreinterpret_u8_u32(tmp.val[2]);
        b.val[3] = vreinterpret_u8_u32(tmp.val[3]);

        value.val[0] = paeth(value.val[3], b.val[0],     last, a.val[0]);
        value.val[1] = paeth(value.val[0], b.val[1], b.val[0], a.val[1]);
        value.val[2] = paeth(value.val[1], b.val[2], b.val[1], a.val[2]);
        value.val[3] = paeth(value.val[2], b.val[3], b.val[2], a.val[3]);

        last = b.val[3];

        const uint8x8_t idx0 = { 0, 1, 2, 3, 8, 9, 10, 11 };
        const uint8x8_t idx1 = { 16, 17, 18, 19, 24, 25, 26, 27 };

        uint8x8_t v0 = vtbl4_u8(value, idx0);
        uint8x8_t v1 = vtbl4_u8(value, idx1);
        vst1q_u8(scan, vcombine_u8(v0, v1));
    }

    // filters

    void filter1_sub_24bit_neon(u8* scan, const u8* prev, int bytes, int bpp)
    {
        uint8x8_t last = vdup_n_u8(0);

        while (bytes >= 12)
        {
            sub12(scan, last);
            scan += 12;
            bytes -= 12;
        }

        if (bytes > 0)
        {
            u8 temp[16];
            std::memcpy(temp, scan, bytes);

            sub12(temp, last);
            std::memcpy(scan, temp, bytes);
        }
    }

    void filter1_sub_32bit_neon(u8* scan, const u8* prev, int bytes, int bpp)
    {
        uint8x16_t last = vdupq_n_u8(0);

        while (bytes >= 16)
        {
            sub16(scan, last);
            scan += 16;
            bytes -= 16;
        }

        if (bytes > 0)
        {
            u8 temp[16];
            std::memcpy(temp, scan, bytes);

            sub16(temp, last);
            std::memcpy(scan, temp, bytes);
        }
    }

    void filter2_up_neon(u8* scan, const u8* prev, int bytes, int bpp)
    {
        while (bytes >= 16)
        {
            uint8x16_t a = vld1q_u8(scan);
            uint8x16_t b = vld1q_u8(prev);
            vst1q_u8(scan, vaddq_u8(a, b));
            scan += 16;
            prev += 16;
            bytes -= 16;
        }

        for (int x = 0; x < bytes; ++x)
        {
            scan[x] += prev[x];
        }
    }

    void filter3_average_24bit_neon(u8* scan, const u8* prev, int bytes, int bpp)
    {
        uint8x8_t last = vdup_n_u8(0);

        while (bytes >= 12)
        {
            average12(scan, prev, last);
            scan += 12;
            prev += 12;
            bytes -= 12;
        }

        if (bytes > 0)
        {
            u8 temp[16];
            std::memcpy(temp, scan, bytes);

            average16(temp, prev, last);
            std::memcpy(scan, temp, bytes);
        }
    }

    void filter3_average_32bit_neon(u8* scan, const u8* prev, int bytes, int bpp)
    {
        uint8x8_t last = vdup_n_u8(0);

        while (bytes >= 16)
        {
            average16(scan, prev, last);
            scan += 16;
            prev += 16;
            bytes -= 16;
        }

        if (bytes > 0)
        {
            u8 temp[16];
            std::memcpy(temp, scan, bytes);

            average16(temp, prev, last);
            std::memcpy(scan, temp, bytes);
        }
    }

    void filter4_paeth_24bit_neon(u8* scan, const u8* prev, int bytes, int bpp)
    {
        uint8x8_t last = vdup_n_u8(0);

        uint8x8x4_t value;
        value.val[3] = vdup_n_u8(0);

        while (bytes >= 12)
        {
            paeth12(scan, prev, value, last);
            scan += 12;
            prev += 12;
            bytes -= 12;
        }

        if (bytes > 0)
        {
            u8 temp[16];
            std::memcpy(temp, scan, bytes);

            paeth12(temp, prev, value, last);
            std::memcpy(scan, temp, bytes);
        }
    }

    void filter4_paeth_32bit_neon(u8* scan, const u8* prev, int bytes, int bpp)
    {
        uint8x8_t last = vdup_n_u8(0);

        uint8x8x4_t value;
        value.val[3] = vdup_n_u8(0);

        while (bytes >= 16)
        {
            paeth16(scan, prev, value, last);
            scan += 16;
            prev += 16;
            bytes -= 16;
        }

        if (bytes > 0)
        {
            u8 temp[16];
            std::memcpy(temp, scan, bytes);

            paeth16(temp, prev, value, last);
            std::memcpy(scan, temp, bytes);
        }
    }

#endif // MANGO_ENABLE_NEON

    struct FilterDispatcher
    {
        FilterFunc sub = filter1_sub;
        FilterFunc up = filter2_up;
        FilterFunc average = filter3_average;
        FilterFunc paeth = filter4_paeth;
        int bpp = 0;

        FilterDispatcher(int bpp)
            : bpp(bpp)
        {
            u64 features = getCPUFlags();

            switch (bpp)
            {
                case 1:
                    paeth = filter4_paeth_8bit;
                    break;
                case 3:
#if defined(MANGO_ENABLE_SSE2)
                    if (features & INTEL_SSE2)
                    {
                        sub = filter1_sub_24bit_sse2;
                        average = filter3_average_24bit_sse2;
                        paeth = filter4_paeth_24bit_sse2;
                    }
#endif
#if defined(MANGO_ENABLE_SSE4_1)
                    if (features & INTEL_SSE4_1)
                    {
                        sub = filter1_sub_24bit_sse41;
                    }
#endif
#if defined(MANGO_ENABLE_NEON)
                    if (features & ARM_NEON)
                    {
                        sub = filter1_sub_24bit_neon;
                        average = filter3_average_24bit_neon;
                        paeth = filter4_paeth_24bit_neon;
                    }
#endif
                    break;
                case 4:
#if defined(MANGO_ENABLE_SSE2)
                    if (features & INTEL_SSE2)
                    {
                        sub = filter1_sub_32bit_sse2;
                        average = filter3_average_32bit_sse2;
                        paeth = filter4_paeth_32bit_sse2;
                    }
#endif
#if defined(MANGO_ENABLE_NEON)
                    if (features & ARM_NEON)
                    {
                        sub = filter1_sub_32bit_neon;
                        average = filter3_average_32bit_neon;
                        paeth = filter4_paeth_32bit_neon;
                    }
#endif
                    break;
            }

            // NOTE: up filter is selected here as it is same for all bit depths

#if defined(MANGO_ENABLE_SSE2)
            if (features & INTEL_SSE2)
            {
                up = filter2_up_sse2;
            }
#endif
#if defined(MANGO_ENABLE_NEON)
            if (features & ARM_NEON)
            {
                up = filter2_up_neon;
            }
#endif

            MANGO_UNREFERENCED(features);
        }

        void operator () (u8* scan, const u8* prev, int bytes) const
        {
            FilterType method = FilterType(scan[0]);
            //printf("%d", method);

            // skip filter byte
            ++scan;
            ++prev;
            --bytes;

            switch (method)
            {
                case FILTER_NONE:
                default:
                    break;

                case FILTER_SUB:
                    sub(scan, prev, bytes, bpp);
                    break;

                case FILTER_UP:
                    up(scan, prev, bytes, bpp);
                    break;

                case FILTER_AVERAGE:
                    average(scan, prev, bytes, bpp);
                    break;

                case FILTER_PAETH:
                    paeth(scan, prev, bytes, bpp);
                    break;
            }
        }
    };

    // ------------------------------------------------------------
    // color conversion
    // ------------------------------------------------------------

    struct ColorState
    {
        using Function = void (*)(const ColorState& state, int width, u8* dst, const u8* src);

        int bits;

        // tRNS
        bool transparent_enable = false;
        u16 transparent_sample[3];
        Color transparent_color;

        Color* palette = nullptr;
    };

    void process_pal1to4_indx(const ColorState& state, int width, u8* dst, const u8* src)
    {
        const int bits = state.bits;
        const u32 mask = (1 << bits) - 1;

        u32 data = 0;
        int offset = -1;

        for (int x = 0; x < width; ++x)
        {
            if (offset < 0)
            {
                offset = 8 - bits;
                data = *src++;
            }

            u32 index = (data >> offset) & mask;
            dst[x] = u8(index);
            offset -= bits;
        }
    }

    void process_pal1to4(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u32* dest = reinterpret_cast<u32*>(dst);

        Color* palette = state.palette;

        const int bits = state.bits;
        const u32 mask = (1 << bits) - 1;

        u32 data = 0;
        int offset = -1;

        for (int x = 0; x < width; ++x)
        {
            if (offset < 0)
            {
                offset = 8 - bits;
                data = *src++;
            }

            u32 index = (data >> offset) & mask;
            dest[x] = palette[index];
            offset -= bits;
        }
    }

    void process_pal8_indx(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        std::memcpy(dst, src, width);
    }

    void process_pal8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        u32* dest = reinterpret_cast<u32*>(dst);

        Color* palette = state.palette;

        for (int x = 0; x < width; ++x)
        {
            dest[x] = palette[src[x]];
        }
    }

    void process_i1to4(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u8* dest = dst;

        const bool transparent_enable = state.transparent_enable;
        const u16 transparent_sample = state.transparent_sample[0];

        const int bits = state.bits;
        const int maxValue = (1 << bits) - 1;
        const int scale = (255 / maxValue) * 0x010101;
        const u32 mask = (1 << bits) - 1;

        u32 data = 0;
        int offset = -1;

        for (int x = 0; x < width; ++x)
        {
            if (offset < 0)
            {
                offset = 8 - bits;
                data = *src++;
            }

            int value = (data >> offset) & mask;
            *dest++ = value * scale;

            if (transparent_enable)
            {
                *dest++ = (value == transparent_sample) ? 0 : 0xff;
            }

            offset -= bits;
        }
    }

    void process_i8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        std::memcpy(dst, src, width);
    }

    void process_i8_trns(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        const u16 transparent_sample = state.transparent_sample[0];

        for (int x = 0; x < width; ++x)
        {
            const u16 alpha = transparent_sample == src[x] ? 0 : 0xff00;
            dest[x] = alpha | src[x];
        }
    }

    void process_i16(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        for (int x = 0; x < width; ++x)
        {
            dest[x] = bigEndian::uload16(src);
            src += 2;
        }
    }

    void process_i16_trns(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        const u16 transparent_sample = state.transparent_sample[0];

        for (int x = 0; x < width; ++x)
        {
            u16 gray = bigEndian::uload16(src);
            u16 alpha = (transparent_sample == gray) ? 0 : 0xffff;
            dest[0] = gray;
            dest[1] = alpha;
            dest += 2;
            src += 2;
        }
    }

    void process_ia8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        std::memcpy(dst, src, width * 2);
    }

    void process_ia16(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        for (int x = 0; x < width; ++x)
        {
            dest[0] = bigEndian::uload16(src + 0);
            dest[1] = bigEndian::uload16(src + 2);
            dest += 2;
            src += 4;
        }
    }

    void process_rgb8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        // SIMD: SSE4.1, NEON
        MANGO_UNREFERENCED(state);

        u32* dest = reinterpret_cast<u32*>(dst);

        while (width >= 4)
        {
            u32 v0 = uload32(src + 0); // R0, G0, B0, R1
            u32 v1 = uload32(src + 4); // G1, B1, R2, G2
            u32 v2 = uload32(src + 8); // B2, R3, G3, B3
            dest[0] = 0xff000000 | v0;
            dest[1] = 0xff000000 | (v0 >> 24) | (v1 << 8);
            dest[2] = 0xff000000 | (v1 >> 16) | (v2 << 16);
            dest[3] = 0xff000000 | (v2 >> 8);
            src += 12;
            dest += 4;
            width -= 4;
        }

        for (int x = 0; x < width; ++x)
        {
            dest[x] = Color(src[0], src[1], src[2], 0xff);
            src += 3;
        }
    }

    void process_rgb8_trns(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u32* dest = reinterpret_cast<u32*>(dst);

        const Color transparent_color = state.transparent_color;

        for (int x = 0; x < width; ++x)
        {
            Color color(src[0], src[1], src[2], 0xff);
            if (color == transparent_color)
            {
                color.a = 0;
            }
            dest[x] = color;
            src += 3;
        }
    }

    void process_rgb16(const ColorState& state, int width, u8* dst, const u8* src)
    {
        // SIMD: NEON
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        for (int x = 0; x < width; ++x)
        {
            dest[0] = bigEndian::uload16(src + 0);
            dest[1] = bigEndian::uload16(src + 2);
            dest[2] = bigEndian::uload16(src + 4);
            dest[3] = 0xffff;
            dest += 4;
            src += 6;
        }
    }

    void process_rgb16_trns(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        const u16 transparent_sample0 = state.transparent_sample[0];
        const u16 transparent_sample1 = state.transparent_sample[1];
        const u16 transparent_sample2 = state.transparent_sample[2];

        for (int x = 0; x < width; ++x)
        {
            u16 red   = bigEndian::uload16(src + 0);
            u16 green = bigEndian::uload16(src + 2);
            u16 blue  = bigEndian::uload16(src + 4);
            u16 alpha = 0xffff;
            if (transparent_sample0 == red &&
                transparent_sample1 == green &&
                transparent_sample2 == blue) alpha = 0;
            dest[0] = red;
            dest[1] = green;
            dest[2] = blue;
            dest[3] = alpha;
            dest += 4;
            src += 6;
        }
    }

    void process_rgba8(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        std::memcpy(dst, src, width * 4);
    }

    void process_rgba16(const ColorState& state, int width, u8* dst, const u8* src)
    {
        // SIMD: SSE2, SSE4.1, NEON
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        for (int x = 0; x < width; ++x)
        {
            dest[0] = bigEndian::uload16(src + 0);
            dest[1] = bigEndian::uload16(src + 2);
            dest[2] = bigEndian::uload16(src + 4);
            dest[3] = bigEndian::uload16(src + 6);
            dest += 4;
            src += 8;
        }
    }

#if defined(MANGO_ENABLE_SSE2)

    void process_rgba16_sse2(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        while (width >= 4)
        {
            __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 0));
            __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 16));
            __m128i a0 = _mm_slli_epi16(v0, 8);
            __m128i b0 = _mm_srli_epi16(v0, 8);
            __m128i a1 = _mm_slli_epi16(v1, 8);
            __m128i b1 = _mm_srli_epi16(v1, 8);
            v0 = _mm_or_si128(a0, b0);
            v1 = _mm_or_si128(a1, b1);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 0), v0);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 8), v1);
            src += 32;
            dest += 16;
            width -= 4;
        }

        for (int x = 0; x < width; ++x)
        {
            dest[0] = bigEndian::uload16(src + 0);
            dest[1] = bigEndian::uload16(src + 2);
            dest[2] = bigEndian::uload16(src + 4);
            dest[3] = bigEndian::uload16(src + 6);
            dest += 4;
            src += 8;
        }
    }

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_SSE4_1)

    void process_rgba16_sse41(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        __m128i mask = _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);

        while (width >= 4)
        {
            __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 0));
            __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 16));
            v0 = _mm_shuffle_epi8(v0, mask);
            v1 = _mm_shuffle_epi8(v1, mask);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 0), v0);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 8), v1);
            src += 32;
            dest += 16;
            width -= 4;
        }

        for (int x = 0; x < width; ++x)
        {
            dest[0] = bigEndian::uload16(src + 0);
            dest[1] = bigEndian::uload16(src + 2);
            dest[2] = bigEndian::uload16(src + 4);
            dest[3] = bigEndian::uload16(src + 6);
            dest += 4;
            src += 8;
        }
    }

    void process_rgb8_sse41(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u32* dest = reinterpret_cast<u32*>(dst);

        __m128i mask = _mm_setr_epi8(0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1);
        __m128i alpha = _mm_set1_epi32(0xff000000);

        while (width >= 16)
        {
            __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 0 * 16));
            __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 1 * 16));
            __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + 2 * 16));
            __m128i color0 = _mm_shuffle_epi8(v0, mask);
            __m128i color1 = _mm_shuffle_epi8(_mm_alignr_epi8(v1, v0, 12), mask);
            __m128i color2 = _mm_shuffle_epi8(_mm_alignr_epi8(v2, v1,  8), mask);
            __m128i color3 = _mm_shuffle_epi8(_mm_alignr_epi8(v2, v2,  4), mask);
            color0 = _mm_or_si128(color0, alpha);
            color1 = _mm_or_si128(color1, alpha);
            color2 = _mm_or_si128(color2, alpha);
            color3 = _mm_or_si128(color3, alpha);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest +  0), color0);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest +  4), color1);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest +  8), color2);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + 12), color3);
            src += 48;
            dest += 16;
            width -= 16;
        }

        for (int x = 0; x < width; ++x)
        {
            dest[x] = Color(src[0], src[1], src[2], 0xff);
            src += 3;
        }
    }

#endif // MANGO_ENABLE_SSE4_1

#if defined(MANGO_ENABLE_NEON)

    void process_rgb8_neon(const ColorState& state, int width, u8* dest, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        while (width >= 8)
        {
            const uint8x8x3_t rgb = vld3_u8(src);
            uint8x8x4_t rgba;
            rgba.val[0] = rgb.val[0];
            rgba.val[1] = rgb.val[1];
            rgba.val[2] = rgb.val[2];
            rgba.val[3] = vdup_n_u8(0xff);
            vst4_u8(dest, rgba);
            src += 24;
            dest += 32;
            width -= 8;
        }

        while (width-- > 0)
        {
            dest[0] = src[0];
            dest[1] = src[1];
            dest[2] = src[2];
            dest[3] = 0xff;
            src += 3;
            dest += 4;
        }
    }

    void process_rgb16_neon(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        u16* dest = reinterpret_cast<u16*>(dst);

        while (width >= 4)
        {
            const uint16x4x3_t rgb = vld3_u16(reinterpret_cast<const u16*>(src));
            uint16x4x4_t rgba;
            rgba.val[0] = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(rgb.val[0])));
            rgba.val[1] = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(rgb.val[1])));
            rgba.val[2] = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(rgb.val[2])));
            rgba.val[3] = vdup_n_u16(0xffff);
            vst4_u16(dest, rgba);
            src += 24;
            dest += 16;
            width -= 4;
        }

        while (width-- > 0)
        {
            dest[0] = bigEndian::uload16(src + 0);
            dest[1] = bigEndian::uload16(src + 2);
            dest[2] = bigEndian::uload16(src + 4);
            dest[3] = 0xffff;
            src += 6;
            dest += 4;
        }
    }

    void process_rgba16_neon(const ColorState& state, int width, u8* dst, const u8* src)
    {
        MANGO_UNREFERENCED(state);

        const u16* source = reinterpret_cast<const u16*>(src);
        u16* dest = reinterpret_cast<u16*>(dst);

        while (width >= 2)
        {
            uint16x8_t a = vld1q_u16(source);
            a = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(a)));
            vst1q_u16(dest, a);
            source += 8;
            dest += 8;
            width -= 2;
        }

        if (width > 0)
        {
            uint16x4_t a = vld1_u16(source);
            a = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(a)));
            vst1_u16(dest, a);
        }
    }

#endif // MANGO_ENABLE_NEON

    ColorState::Function getColorFunction(const ColorState& state, int color_type, int bit_depth)
    {
        ColorState::Function function = nullptr;
        u64 features = getCPUFlags();

        if (color_type == COLOR_TYPE_PALETTE)
        {
            if (bit_depth < 8)
            {
                if (state.palette)
                {
                    function = process_pal1to4;
                }
                else
                {
                    function = process_pal1to4_indx;
                }
            }
            else
            {
                if (state.palette)
                {
                    function = process_pal8;
                }
                else
                {
                    function = process_pal8_indx;
                }
            }
        }
        else if (color_type == COLOR_TYPE_I)
        {
            if (bit_depth < 8)
            {
                function = process_i1to4;
            }
            else if (bit_depth == 8)
            {
                if (state.transparent_enable)
                {
                    function = process_i8_trns;
                }
                else
                {
                    function = process_i8;
                }
            }
            else
            {
                if (state.transparent_enable)
                {
                    function = process_i16_trns;
                }
                else
                {
                    function = process_i16;
                }
            }
        }
        else if (color_type == COLOR_TYPE_IA)
        {
            if (bit_depth == 8)
            {
                function = process_ia8;
            }
            else
            {
                function = process_ia16;
            }
        }
        else if (color_type == COLOR_TYPE_RGB)
        {
            if (bit_depth == 8)
            {
                if (state.transparent_enable)
                {
                    function = process_rgb8_trns;
                }
                else
                {
                    function = process_rgb8;
#if defined(MANGO_ENABLE_SSE4_1)
                    if (features & INTEL_SSE4_1)
                    {
                        function = process_rgb8_sse41;
                    }
#endif
#if defined(MANGO_ENABLE_NEON)
                    if (features & ARM_NEON)
                    {
                        function = process_rgb8_neon;
                    }
#endif
                }
            }
            else
            {
                if (state.transparent_enable)
                {
                    function = process_rgb16_trns;
                }
                else
                {
                    function = process_rgb16;
#if defined(MANGO_ENABLE_NEON)
                    if (features & ARM_NEON)
                    {
                        function = process_rgb16_neon;
                    }
#endif
                }
            }
        }
        else if (color_type == COLOR_TYPE_RGBA)
        {
            if (bit_depth == 8)
            {
                function = process_rgba8;
            }
            else
            {
                function = process_rgba16;
#if defined(MANGO_ENABLE_SSE2)
                if (features & INTEL_SSE2)
                {
                    function = process_rgba16_sse2;
                }
#endif
#if defined(MANGO_ENABLE_SSE4_1)
                if (features & INTEL_SSE4_1)
                {
                    function = process_rgba16_sse41;
                }
#endif
#if defined(MANGO_ENABLE_NEON)
                if (features & ARM_NEON)
                {
                    function = process_rgba16_neon;
                }
#endif
            }
        }

        MANGO_UNREFERENCED(features);
        return function;
    }

    // ------------------------------------------------------------
    // AdamInterleave
    // ------------------------------------------------------------

    struct AdamInterleave
    {
        int xorig;
        int yorig;
        int xspc;
        int yspc;
        int w;
        int h;

        AdamInterleave(u32 pass, u32 width, u32 height)
        {
            const u32 orig = 0x01020400 >> (pass * 4);
            const u32 spc = 0x01122333 >> (pass * 4);

            xorig = (orig >> 4) & 15;
            yorig = (orig >> 0) & 15;
            xspc = (spc >> 4) & 15;
            yspc = (spc >> 0) & 15;
            w = (width - xorig + (1 << xspc) - 1) >> xspc;
            h = (height - yorig + (1 << yspc) - 1) >> yspc;
        }
    };

    // ------------------------------------------------------------
    // ParserPNG
    // ------------------------------------------------------------

    struct Chromaticity
    {
        float32x2 white;
        float32x2 red;
        float32x2 green;
        float32x2 blue;
    };

    struct Frame
    {
        enum Dispose : u8
        {
            NONE       = 0,
            BACKGROUND = 1,
            PREVIOUS   = 2,
        };

        enum Blend : u8
        {
            SOURCE = 0,
            OVER   = 1,
        };

        u32 sequence_number;
        u32 width;
        u32 height;
        u32 xoffset;
        u32 yoffset;
        u16 delay_num;
        u16 delay_den;
        u8 dispose;
        u8 blend;

        void read(BigEndianConstPointer p)
        {
            sequence_number = p.read32();
            width = p.read32();
            height = p.read32();
            xoffset = p.read32();
            yoffset = p.read32();
            delay_num = p.read16();
            delay_den = p.read16();
            dispose = p.read8();
            blend = p.read8();

            printLine(Print::Info, "  Sequence: {}", sequence_number);
            printLine(Print::Info, "  Frame: {} x {} ({}, {})", width, height, xoffset, yoffset);
            printLine(Print::Info, "  Time: {} / {}", delay_num, delay_den);
            printLine(Print::Info, "  Dispose: {}", dispose);
            printLine(Print::Info, "  Blend: {}", blend);
        }
    };

    class ParserPNG
    {
    protected:
        ConstMemory m_memory;
        ImageHeader m_header;

        ImageDecodeInterface* m_interface = nullptr;

        const u8* m_pointer = nullptr;
        const u8* m_end = nullptr;
        const char* m_error = nullptr;

        ColorState m_color_state;
        Surface m_decode_target;

        u64 m_filter_time = 0;
        u64 m_color_time = 0;

        // IHDR
        int m_width;
        int m_height;
        int m_color_type;
        int m_compression;
        int m_filter;
        int m_interlace;

        int m_channels;

        // IDAT
        std::vector<ConstMemory> m_idat;

        // CgBI
        bool m_iphoneOptimized = false; // Apple's proprietary iphone optimized pngcrush

        // PLTE
        Palette m_palette;

        // cHRM
        Chromaticity m_chromaticity;

        // gAMA
        float m_gamma = 0.0f;

        // sBIT
        u8 m_scale_bits[4];

        // sRGB
        u8 m_srgb_render_intent = 0xff;

        // acTL
        u32 m_number_of_frames = 0;
        u32 m_repeat_count = 0;
        u32 m_current_frame_index = 0;
        u32 m_next_frame_index = 0;

        // fcTL
        Frame m_frame;
        const u8* m_first_frame = nullptr;

        // iCCP
        Buffer m_icc;

        // iDOT
        const u8* m_idot_address = nullptr;
        size_t m_idot_index = 0;
        u32 m_first_half_height = 0;
        u32 m_second_half_height = 0;

        // pLLD
        u32 m_parallel_height = 0;
        u8 m_parallel_flags = 0;
        std::vector<ConstMemory> m_parallel_segments;

        void read_IHDR(BigEndianConstPointer p, u32 size);
        void read_IDAT(BigEndianConstPointer p, u32 size);
        void read_PLTE(BigEndianConstPointer p, u32 size);
        void read_tRNS(BigEndianConstPointer p, u32 size);
        void read_cHRM(BigEndianConstPointer p, u32 size);
        void read_gAMA(BigEndianConstPointer p, u32 size);
        void read_sBIT(BigEndianConstPointer p, u32 size);
        void read_sRGB(BigEndianConstPointer p, u32 size);
        void read_acTL(BigEndianConstPointer p, u32 size);
        void read_fcTL(BigEndianConstPointer p, u32 size);
        void read_fdAT(BigEndianConstPointer p, u32 size);
        void read_iCCP(BigEndianConstPointer p, u32 size);
        void read_iDOT(BigEndianConstPointer p, u32 size);
        void read_pLLD(BigEndianConstPointer p, u32 size);

        void parse();

        void blend_ia8      (u8* dest, const u8* src, int width);
        void blend_ia16     (u8* dest, const u8* src, int width);
        void blend_rgba8    (u8* dest, const u8* src, int width);
        void blend_rgba16   (u8* dest, const u8* src, int width);
        void blend_indexed  (u8* dest, const u8* src, int width);

        void deinterlace(u8* output, int width, int height, size_t stride, u8* buffer);
        void filter(u8* buffer, int bytes, int height);
        void process_range(const Surface& target, u8* buffer, int y0, int y1);
        void process_image(const Surface& target, u8* buffer);

        void blend(Surface& d, Surface& s, Palette* palette);

        size_t getBytesPerLine(int width) const
        {
            return m_channels * ((m_color_state.bits * size_t(width) + 7) / 8);
        }

        void setError(const std::string& error)
        {
            m_header.info = "[ImageDecoder.PNG] ";
            m_header.info += error;
            m_header.success = false;
        }

    public:
        ParserPNG(ConstMemory memory)
            : m_memory(memory)
            , m_end(memory.address + memory.size)
        {
        }

        ~ParserPNG()
        {
        }

        const ImageHeader& getHeader()
        {
            if (m_header.success)
            {
                m_header.width   = m_width;
                m_header.height  = m_height;
                m_header.depth   = 0;
                m_header.levels  = 0;
                m_header.faces   = 0;
                m_header.palette = false;
                m_header.compression = TextureCompression::NONE;

                u16 flags = 0;

                // apple pngcrush premultiplies alpha
                if (m_iphoneOptimized)
                {
                    m_header.premultiplied = true;
                    flags = Format::PREMULT;
                }

                // force alpha channel on when transparency is enabled
                int color_type = m_color_type;
                if (m_color_state.transparent_enable && color_type != COLOR_TYPE_PALETTE)
                {
                    color_type |= 4;
                }

                // select decoding format
                int bits = m_color_state.bits <= 8 ? 8 : 16;
                switch (color_type)
                {
                    case COLOR_TYPE_I:
                        m_header.format = LuminanceFormat(bits, Format::UNORM, bits, 0, flags);
                        break;

                    case COLOR_TYPE_IA:
                        m_header.format = LuminanceFormat(bits * 2, Format::UNORM, bits, bits, flags);
                        break;

                    case COLOR_TYPE_PALETTE:
                        m_header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8, flags);
                        m_header.palette = true;
                        break;

                    case COLOR_TYPE_RGB:
                    case COLOR_TYPE_RGBA:
                        m_header.format = Format(bits * 4, Format::UNORM,
                            m_iphoneOptimized ? Format::BGRA : Format::RGBA,
                            bits, bits, bits, bits, flags);
                        break;
                }
            }

            return m_header;
        }

        void setInterface(ImageDecodeInterface* interface)
        {
            m_interface = interface;

            BigEndianConstPointer p = m_memory.address;

            // read header magic
            const u64 magic = p.read64();
            if (magic != PNG_HEADER_MAGIC)
            {
                setError("Incorrect header magic.");
                return;
            }

            // read first chunk; in standard it is IHDR, but apple has CgBI+IHDR
            u32 size = p.read32();
            u32 id = p.read32();

            if (id == u32_mask_rev('C', 'g', 'B', 'I'))
            {
                printLine(Print::Info, "CgBI: reading PNG as iphone optimized");

                m_iphoneOptimized = true;
                p += size; // skip chunk data
                p += sizeof(u32); // skip crc

                // next chunk
                size = p.read32();
                id = p.read32();
            }

            if (id != u32_mask_rev('I', 'H', 'D', 'R'))
            {
                setError("Incorrect file; the IHDR chunk must come first.");
                return;
            }

            read_IHDR(p, size);
            p += size; // skip chunk data
            p += sizeof(u32); // skip crc

            // keep track of parsing position
            m_pointer = p;
        }

        size_t getInterlacedPassSize(int pass, int width, int height) const
        {
            size_t bytes = 0;

            AdamInterleave adam(pass, width, height);
            if (adam.w && adam.h)
            {
                bytes = (PNG_FILTER_BYTE + getBytesPerLine(adam.w)) * adam.h;
            }

            return bytes;
        }

        ConstMemory icc() const
        {
            return m_icc;
        }

        void decode_plld(const Surface& target);
        void decode_idot(const Surface& target);
        bool decode_std(const Surface& target, ImageDecodeStatus& status);

        ImageDecodeStatus decode(const Surface& dest, bool multithread, Palette* palette);
    };

    void ParserPNG::read_IHDR(BigEndianConstPointer p, u32 size)
    {
        printLine(Print::Info, "[\"IHDR\"] {} bytes", size);

        if (size != 13)
        {
            setError("Incorrect IHDR chunk size.");
            return;
        }

        m_width = p.read32();
        m_height = p.read32();
        m_color_state.bits = p.read8();
        m_color_type = p.read8();
        m_compression = p.read8();
        m_filter = p.read8();
        m_interlace = p.read8();

        if (m_width <= 0 || m_height <= 0)
        {
            setError(fmt::format("Incorrect dimensions ({} x {}).", m_width, m_height));
            return;
        }

        if (!u32_is_power_of_two(m_color_state.bits))
        {
            setError(fmt::format("Incorrect bit depth ({}).", m_color_state.bits));
            return;
        }

        // look-ahead into the chunks to see if we have transparency information
        p += 4; // skip crc
        for ( ; p < m_end - 8; )
        {
            const u32 chunk_size = p.read32();
            const u32 id = p.read32();
            switch (id)
            {
                case u32_mask_rev('t', 'R', 'N', 'S'):
                    if (m_color_type < 4)
                    {
                        // Enable color keying only for valid color types
                        m_color_state.transparent_enable = true;
                    }
                    break;
            }
            p += (chunk_size + 4);
        }

        // bit-depth range defaults
        int minBits = 3; // log2(8) bits
        int maxBits = 4; // log2(16) bits

        switch (m_color_type)
        {
            case COLOR_TYPE_I:
                // supported: 1, 2, 4, 8, 16 bits
                minBits = 0;
                m_channels = 1;
                break;
            case COLOR_TYPE_RGB:
                // supported: 8, 16 bits
                m_channels = 3;
                break;
            case COLOR_TYPE_PALETTE:
                // supported: 1, 2, 4, 8 bits
                minBits = 0;
                maxBits = 3;
                m_channels = 1;
                break;
            case COLOR_TYPE_IA:
                // supported: 8, 16 bits
                m_channels = 2;
                break;
            case COLOR_TYPE_RGBA:
                // supported: 8, 16 bits
                m_channels = 4;
                break;
            default:
                setError(fmt::format("Incorrect color type ({}).", m_color_type));
                return;
        }

        const int log2bits = u32_log2(m_color_state.bits);
        if (log2bits < minBits || log2bits > maxBits)
        {
            setError(fmt::format("Unsupported bit depth for color type ({}).", m_color_state.bits));
            return;
        }

        // load default scaling values (override from sBIT chunk)
        for (int i = 0; i < m_channels; ++i)
        {
            m_scale_bits[i] = u8(m_color_state.bits);
        }

        printLine(Print::Info, "  Image: ({} x {}), {} bits", m_width, m_height, m_color_state.bits);
        printLine(Print::Info, "  Color:       {}", get_string(ColorType(m_color_type)));
        printLine(Print::Info, "  Compression: {}", m_compression);
        printLine(Print::Info, "  Filter:      {}", m_filter);
        printLine(Print::Info, "  Interlace:   {}", m_interlace);
    }

    void ParserPNG::read_IDAT(BigEndianConstPointer p, u32 size)
    {
        if (m_parallel_height)
        {
            m_parallel_segments.emplace_back(p, size);
        }
        else
        {
            if (p == m_idot_address)
            {
                m_idot_index = m_idat.size();
            }

            m_idat.emplace_back(p, size);
        }
    }

    void ParserPNG::read_PLTE(BigEndianConstPointer p, u32 size)
    {
        if (size % 3)
        {
            setError("Incorrect palette size.");
            return;
        }

        if (size > 768)
        {
            setError("Too large palette.");
            return;
        }

        m_palette.size = size / 3;

        for (u32 i = 0; i < m_palette.size; ++i)
        {
            m_palette[i] = Color(p[0], p[1], p[2], 0xff);
            p += 3;
        }
    }

    void ParserPNG::read_tRNS(BigEndianConstPointer p, u32 size)
    {
        if (m_color_type == COLOR_TYPE_I)
        {
            if (size != 2)
            {
                setError("Incorrect tRNS chunk size.");
                return;
            }

            m_color_state.transparent_enable = true;
            m_color_state.transparent_sample[0] = p.read16();
        }
        else if (m_color_type == COLOR_TYPE_RGB)
        {
            if (size != 6)
            {
                setError("Incorrect tRNS chunk size.");
                return;
            }

            m_color_state.transparent_enable = true;
            m_color_state.transparent_sample[0] = p.read16();
            m_color_state.transparent_sample[1] = p.read16();
            m_color_state.transparent_sample[2] = p.read16();
            m_color_state.transparent_color = Color(m_color_state.transparent_sample[0] & 0xff,
                                                    m_color_state.transparent_sample[1] & 0xff,
                                                    m_color_state.transparent_sample[2] & 0xff, 0xff);
        }
        else if (m_color_type == COLOR_TYPE_PALETTE)
        {
            if (m_palette.size < u32(size))
            {
                setError("Incorrect alpha palette size.");
                return;
            }

            for (u32 i = 0; i < size; ++i)
            {
                m_palette[i].a = p[i];
            }
        }
        else
        {
            // From the specification, ISO/IEC 15948:2003 ;
            // "A tRNS chunk shall not appear for colour types 4 and 6, since a full alpha channel is already present in those cases."
            //
            // Of course, some software apparently writes png files with color_type=RGBA which have a tRNS chunk
            // so the following validation check is disabled because the users expect the files to "work"

            //setError("Incorrect color type for alpha palette.");
        }
    }

    void ParserPNG::read_cHRM(BigEndianConstPointer p, u32 size)
    {
        if (size != 32)
        {
            setError("Incorrect cHRM chunk size.");
            return;
        }

        const float scale = 100000.0f;
        m_chromaticity.white.x = p.read32() / scale;
        m_chromaticity.white.y = p.read32() / scale;
        m_chromaticity.red.x   = p.read32() / scale;
        m_chromaticity.red.y   = p.read32() / scale;
        m_chromaticity.green.x = p.read32() / scale;
        m_chromaticity.green.y = p.read32() / scale;
        m_chromaticity.blue.x  = p.read32() / scale;
        m_chromaticity.blue.y  = p.read32() / scale;
    }

    void ParserPNG::read_gAMA(BigEndianConstPointer p, u32 size)
    {
        if (size != 4)
        {
            setError("Incorrect gAMA chunk size.");
            return;
        }

        m_gamma = p.read32() / 100000.0f;
    }

    void ParserPNG::read_sBIT(BigEndianConstPointer p, u32 size)
    {
        if (size > 4)
        {
            setError("Incorrect sBIT chunk size.");
            return;
        }

        for (u32 i = 0; i < size; ++i)
        {
            m_scale_bits[i] = p[i];
        }
    }

    void ParserPNG::read_sRGB(BigEndianConstPointer p, u32 size)
    {
        if (size != 1)
        {
            setError("Incorrect sRGB chunk size.");
            return;
        }

        m_srgb_render_intent = p[0];
    }

    void ParserPNG::read_acTL(BigEndianConstPointer p, u32 size)
    {
        if (size != 8)
        {
            setError("Incorrect acTL chunk size.");
            return;
        }

        m_number_of_frames = p.read32();
        m_repeat_count = p.read32();

        printLine(Print::Info, "  Frames: {}", m_number_of_frames);
        printLine(Print::Info, "  Repeat: {} {}", m_repeat_count, m_repeat_count ? "" : "(infinite)");
    }

    void ParserPNG::read_fcTL(BigEndianConstPointer p, u32 size)
    {
        if (size != 26)
        {
            setError("Incorrect fcTL chunk size.");
            return;
        }

        if (!m_first_frame)
        {
            m_first_frame = p - 8;
        }

        m_frame.read(p);
    }

    void ParserPNG::read_fdAT(BigEndianConstPointer p, u32 size)
    {
        u32 sequence_number = p.read32();
        size -= 4;

        printLine(Print::Info, "  Sequence: {}", sequence_number);
        MANGO_UNREFERENCED(sequence_number);

        // we can simply treat fdat like idat
        m_idat.emplace_back(p, size);
    }

    void ParserPNG::read_iCCP(BigEndianConstPointer p, u32 size)
    {
        /*
        Profile name:       1-79 bytes (character string)
        Null separator:     1 byte
        Compression method: 1 byte (0=deflate)
        Compressed profile: n bytes
        */
        const char* name = reinterpret_cast<const char*>(&p[0]);
        size_t name_len = stringLength(name, size);
        if (name_len == size)
        {
            printLine(Print::Info, "iCCP: profile name not terminated");
            return;
        }

        printLine(Print::Info, "  profile name '{}'", name);

        const u8* profile = p + name_len + 2;
        const size_t icc_bytes = size - name_len - 2;

        m_icc.reset();

        printLine(Print::Info, "  decompressing icc profile {} bytes", icc_bytes);

        constexpr size_t max_profile_size = 1024 * 1024 * 2;
        Buffer buffer(max_profile_size);

        CompressionStatus status = deflate_zlib::decompress(buffer, ConstMemory(profile, icc_bytes));
        if (status)
        {
            printLine(Print::Info, "  unpacked icc profile {} bytes", status.size);
            m_icc.append(buffer, status.size);
        }

        printLine(Print::Info, "  icc profile {} bytes", m_icc.size());
    }

    void ParserPNG::read_iDOT(BigEndianConstPointer p, u32 size)
    {
        if (size != 28)
        {
            setError("Incorrect iDOT chunk size.");
            return;
        }

        u32 height_divisor = p.read32();
        p += 4;
        u32 divided_height = p.read32();
        p += 4;

        if (height_divisor != 2)
        {
            setError("Unsupported height divisor.");
            return;
        }

        m_first_half_height = p.read32();
        m_second_half_height = p.read32();
        u32 idat_offset = p.read32();

        // Compute IDAT address (the offset is relative to iDOT chunk)
        BigEndianConstPointer x = p + idat_offset - 36;

        x += 4; // skip size field
        u32 id = x.read32();
        if (id == u32_mask_rev('I', 'D', 'A', 'T'))
        {
            m_idot_address = x;

            printLine(Print::Info, "  First:  {}", m_first_half_height);
            printLine(Print::Info, "  Second: {}", m_second_half_height);
            printLine(Print::Info, "  Offset: {}", idat_offset);
        }

        MANGO_UNREFERENCED(divided_height);
    }

    void ParserPNG::read_pLLD(BigEndianConstPointer p, u32 size)
    {
        if (size != 5)
        {
            setError("Incorrect pLLD chunk size.");
            return;
        }

        m_parallel_height = p.read32();
        m_parallel_flags = p.read8();

        printLine(Print::Info, "  Segment height: {}", m_parallel_height);
        printLine(Print::Info, "  Flags: {:#x}", m_parallel_flags);
    }

    void ParserPNG::parse()
    {
        BigEndianConstPointer p = m_pointer;

        for ( ; p < m_end - 8; )
        {
            const u32 size = p.read32();
            const u32 id = p.read32();

            const u8* ptr_next_chunk = p + size;
            ptr_next_chunk += 4; // skip crc

            printLine(Print::Info, "[\"{:c}{:c}{:c}{:c}\"] {} bytes", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);

            // check that we won't read past end of file
            if (p + size + 4 > m_end)
            {
                setError("Corrupted file.");
                return;
            }

            switch (id)
            {
                case u32_mask_rev('I', 'H', 'D', 'R'):
                    setError("File can only have one IHDR chunk.");
                    break;

                case u32_mask_rev('P', 'L', 'T', 'E'):
                    read_PLTE(p, size);
                    break;

                case u32_mask_rev('t', 'R', 'N', 'S'):
                    read_tRNS(p, size);
                    break;

                case u32_mask_rev('g', 'A', 'M', 'A'):
                    read_gAMA(p, size);
                    break;

                case u32_mask_rev('s', 'B', 'I', 'T'):
                    read_sBIT(p, size);
                    break;

                case u32_mask_rev('s', 'R', 'G', 'B'):
                    read_sRGB(p, size);
                    break;

                case u32_mask_rev('c', 'H', 'R', 'M'):
                    read_cHRM(p, size);
                    break;

                case u32_mask_rev('I', 'D', 'A', 'T'):
                    read_IDAT(p, size);
                    if (m_number_of_frames > 0)
                    {
                        m_pointer = ptr_next_chunk;
                        return;
                    }
                    break;

                case u32_mask_rev('a', 'c', 'T', 'L'):
                    read_acTL(p, size);
                    break;

                case u32_mask_rev('f', 'c', 'T', 'L'):
                    read_fcTL(p, size);
                    break;

                case u32_mask_rev('f', 'd', 'A', 'T'):
                    read_fdAT(p, size);
                    m_pointer = ptr_next_chunk;
                    return;

                case u32_mask_rev('i', 'C', 'C', 'P'):
                    read_iCCP(p, size);
                    break;

                case u32_mask_rev('i', 'D', 'O', 'T'):
                    read_iDOT(p, size);
                    break;

                case u32_mask_rev('p', 'L', 'L', 'D'):
                    read_pLLD(p, size);
                    break;

                case u32_mask_rev('p', 'H', 'Y', 's'):
                case u32_mask_rev('b', 'K', 'G', 'D'):
                case u32_mask_rev('z', 'T', 'X', 't'):
                case u32_mask_rev('t', 'E', 'X', 't'):
                case u32_mask_rev('t', 'I', 'M', 'E'):
                case u32_mask_rev('i', 'T', 'X', 't'):
                    // NOTE: ignoring these chunks
                    break;

                case u32_mask_rev('I', 'E', 'N', 'D'):
                    if (m_number_of_frames > 0)
                    {
                        // reset current pointer to first animation frame (for looping)
                        ptr_next_chunk = m_first_frame;
                    }
                    else
                    {
                        // terminate parsing
                        m_pointer = m_end;
                        return;
                    }
                    break;

                default:
                    printLine(Print::Info, "  # UNKNOWN: [\"{:c}{:c}{:c}{:c}\"] {} bytes", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);
                    break;
            }

            if (m_error)
            {
                // error occured while reading chunks
                return;
            }

            p = ptr_next_chunk;
        }
    }

    void ParserPNG::blend_ia8(u8* dest, const u8* src, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u8 alpha = src[1];
            u8 invalpha = 255 - alpha;
            dest[0] = src[0] + ((dest[0] - src[0]) * invalpha) / 255;
            dest[1] = alpha;
            src += 2;
            dest += 2;
        }
    }

    void ParserPNG::blend_ia16(u8* dest8, const u8* src8, int width)
    {
        u16* dest = reinterpret_cast<u16*>(dest8);
        const u16* src = reinterpret_cast<const u16*>(src8);
        for (int x = 0; x < width; ++x)
        {
            u16 alpha = src[1];
            u16 invalpha = 255 - alpha;
            dest[0] = src[0] + ((dest[0] - src[0]) * invalpha) / 255;
            dest[1] = alpha;
            src += 2;
            dest += 2;
        }
    }

    void ParserPNG::blend_rgba8(u8* dest, const u8* src, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u8 alpha = src[3];
            u8 invalpha = 255 - alpha;
            dest[0] = src[0] + ((dest[0] - src[0]) * invalpha) / 255;
            dest[1] = src[1] + ((dest[1] - src[1]) * invalpha) / 255;
            dest[2] = src[2] + ((dest[2] - src[2]) * invalpha) / 255;
            dest[3] = alpha;
            src += 4;
            dest += 4;
        }
    }

    void ParserPNG::blend_rgba16(u8* dest8, const u8* src8, int width)
    {
        u16* dest = reinterpret_cast<u16*>(dest8);
        const u16* src = reinterpret_cast<const u16*>(src8);
        for (int x = 0; x < width; ++x)
        {
            u16 alpha = src[3];
            u16 invalpha = 255 - alpha;
            dest[0] = src[0] + ((dest[0] - src[0]) * invalpha) / 65535;
            dest[1] = src[1] + ((dest[1] - src[1]) * invalpha) / 65535;
            dest[2] = src[2] + ((dest[2] - src[2]) * invalpha) / 65535;
            dest[3] = alpha;
            src += 4;
            dest += 4;
        }
    }

    void ParserPNG::blend_indexed(u8* dest, const u8* src, int width)
    {
        for (int x = 0; x < width; ++x)
        {
            u8 sample = src[x];
            Color color = m_palette[sample];
            if (color.a)
            {
                dest[x] = sample;
            }
        }
    }

    void ParserPNG::blend(Surface& d, Surface& s, Palette* palette)
    {
        int width = s.width;
        int height = s.height;
        if (m_frame.blend == Frame::SOURCE || !s.format.isAlpha())
        {
            d.blit(0, 0, s);
        }
        else if (m_frame.blend == Frame::OVER)
        {
            for (int y = 0; y < height; ++y)
            {
                const u8* src = s.address(0, y);
                u8* dest = d.address(0, y);

                if (palette)
                {
                    // palette extract mode: blend with the main image palette
                    blend_indexed(dest, src, width);
                }
                else
                {
                    switch (s.format.bits)
                    {
                        case 8:
                            blend_ia8(dest, src, width);
                            break;
                        case 16:
                            blend_ia16(dest, src, width);
                            break;
                        case 32:
                            blend_rgba8(dest, src, width);
                            break;
                        case 64:
                            blend_rgba16(dest, src, width);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    void ParserPNG::deinterlace(u8* output, int width, int height, size_t stride, u8* buffer)
    {
        if (m_color_state.bits < 8)
        {
            const int samples = 8 / m_color_state.bits;
            const int mask = samples - 1;
            const int shift = u32_log2(samples);
            const int valueShift = u32_log2(m_color_state.bits);
            const int valueMask = (1 << m_color_state.bits) - 1;

            for (int pass = 0; pass < 7; ++pass)
            {
                AdamInterleave adam(pass, width, height);
                printLine(Print::Info, "  pass: {} ({} x {})", pass, adam.w, adam.h);

                const int bw = PNG_FILTER_BYTE + ((adam.w + mask) >> shift);
                filter(buffer, bw, adam.h);

                if (adam.w && adam.h)
                {
                    for (int y = 0; y < adam.h; ++y)
                    {
                        const int yoffset = (y << adam.yspc) + adam.yorig;
                        u8* dest = output + yoffset * stride + PNG_FILTER_BYTE;
                        u8* src = buffer + y * bw + PNG_FILTER_BYTE;

                        for (int x = 0; x < adam.w; ++x)
                        {
                            const int xoffset = (x << adam.xspc) + adam.xorig;
                            u8 v = src[x >> shift];
                            int a = (mask - (x & mask)) << valueShift;
                            int b = (mask - (xoffset & mask)) << valueShift;
                            v = ((v >> a) & valueMask) << b;
                            dest[xoffset >> shift] |= v;
                        }
                    }

                    // next pass
                    buffer += bw * adam.h;
                }
            }
        }
        else
        {
            const int components = m_channels * (m_color_state.bits / 8);

            for (int pass = 0; pass < 7; ++pass)
            {
                AdamInterleave adam(pass, width, height);
                printLine(Print::Info, "  pass: {} ({} x {})", pass, adam.w, adam.h);

                const int bw = PNG_FILTER_BYTE + adam.w * components;
                filter(buffer, bw, adam.h);

                if (adam.w && adam.h)
                {
                    const int ps = adam.w * components + PNG_FILTER_BYTE;

                    for (int y = 0; y < adam.h; ++y)
                    {
                        const int yoffset = (y << adam.yspc) + adam.yorig;
                        u8* dest = output + yoffset * stride + PNG_FILTER_BYTE;
                        u8* src = buffer + y * ps + PNG_FILTER_BYTE;

                        dest += adam.xorig * components;
                        const int xmax = (adam.w * components) << adam.xspc;
                        const int xstep = components << adam.xspc;

                        for (int x = 0; x < xmax; x += xstep)
                        {
                            std::memcpy(dest + x, src, components);
                            src += components;
                        }
                    }

                    // next pass
                    buffer += bw * adam.h;
                }
            }
        }
    }

    void ParserPNG::filter(u8* buffer, int bytes, int height)
    {
        const int bpp = (m_color_state.bits < 8) ? 1 : m_channels * m_color_state.bits / 8;
        if (bpp > 8)
            return;

        FilterDispatcher filter(bpp);

        // zero scanline
        Buffer zeros(bytes, 0);
        const u8* prev = zeros.data();

        u64 time0 = Time::us();

        for (int y = 0; y < height; ++y)
        {
            filter(buffer, prev, bytes);
            prev = buffer;
            buffer += bytes;
        }

        u64 time1 = Time::us();
        m_filter_time += (time1 - time0);
    }

    void ParserPNG::process_range(const Surface& target, u8* buffer, int y0, int y1)
    {
        const int bpp = (m_color_state.bits < 8) ? 1 : m_channels * m_color_state.bits / 8;
        FilterDispatcher filter(bpp);

        const size_t bytes_per_line = getBytesPerLine(target.width) + PNG_FILTER_BYTE;
        ColorState::Function convert = getColorFunction(m_color_state, m_color_type, m_color_state.bits);

        u8* image = target.image + y0 * target.stride;

        for (int y = y0; y < y1; ++y)
        {
            u64 time0 = Time::us();

            // filtering
            filter(buffer, buffer - bytes_per_line, int(bytes_per_line));

            u64 time1 = Time::us();
            m_filter_time += (time1 - time0);

            // color conversion
            convert(m_color_state, target.width, image, buffer + PNG_FILTER_BYTE);

            u64 time2 = Time::us();
            m_color_time += (time2 - time1);

            buffer += bytes_per_line;
            image += target.stride;
        }

        ImageDecodeRect rect;

        rect.x = 0;
        rect.y = y0;
        rect.width = target.width;
        rect.height = y1 - y0;
        rect.progress = float(rect.height) / target.height;

        if (m_decode_target.image != target.image)
        {
            u8* dest = target.image + y0 * target.stride;
            Surface source(rect.width, rect.height, target.format, target.stride, dest);
            m_decode_target.blit(rect.x, rect.y, source);
        }

        if (m_interface->callback)
        {
            m_interface->callback(rect);
        }
    }

    void ParserPNG::process_image(const Surface& target, u8* buffer)
    {
        if (m_error)
        {
            return;
        }

        u8* image = target.image;

        const size_t bytes_per_line = getBytesPerLine(target.width) + PNG_FILTER_BYTE;

        ColorState::Function convert = getColorFunction(m_color_state, m_color_type, m_color_state.bits);

        if (m_interlace)
        {
            Buffer temp(target.height * bytes_per_line, 0);

            // deinterlace does filter for each pass
            deinterlace(temp, target.width, target.height, bytes_per_line, buffer);

            // use de-interlaced temp buffer as processing source
            buffer = temp;

            u64 time0 = Time::us();

            // color conversion
            for (int y = 0; y < target.height; ++y)
            {
                convert(m_color_state, target.width, image, buffer + PNG_FILTER_BYTE);
                image += target.stride;
                buffer += bytes_per_line;
            }

            u64 time1 = Time::us();
            m_color_time += (time1 - time0);

            ImageDecodeRect rect;

            rect.x = 0;
            rect.y = 0;
            rect.width = target.width;
            rect.height = target.height;
            rect.progress = 1.0f;

            if (m_decode_target.image != target.image)
            {
                Surface source(rect.width, rect.height, target.format, target.stride, target.image);
                m_decode_target.blit(rect.x, rect.y, source);
            }

            if (m_interface->callback)
            {
                m_interface->callback(rect);
            }
        }
        else
        {
            const int bytes_per_pixel = (m_color_state.bits < 8) ? 1 : m_channels * m_color_state.bits / 8;
            if (bytes_per_pixel > 8)
                return;

            const int y0 = 0;
            const int y1 = target.height;
            process_range(target, buffer + bytes_per_line * y0, y0, y1);
        }
    }

    void ParserPNG::decode_plld(const Surface& target)
    {    
        const size_t bytes_per_line = getBytesPerLine(target.width) + PNG_FILTER_BYTE;

        ConcurrentQueue q("png:decode");

        u32 y = 0;

        // skip zlib header
        size_t first = 2;

#ifdef MANGO_ENABLE_ISAL__disabled_the_deflate_is_slightly_faster
        auto decompress = getCompressor(Compressor::ISAL).decompress;
#else
        auto decompress = getCompressor(Compressor::DEFLATE).decompress;
#endif

        for (ConstMemory memory : m_parallel_segments)
        {
            if (m_interface->cancelled)
            {
                q.cancel();
                break;
            }

            memory = memory.slice(first, memory.size - first);
            first = 0;

            int h = std::min(m_parallel_height, m_height - y);

            q.enqueue([=]
            {
                if (m_interface->cancelled)
                {
                    return;
                }

                const size_t extra = bytes_per_line + PNG_SIMD_PADDING;
                Buffer temp(bytes_per_line * h + extra);

                // zero scanline for filters at the beginning
                std::memset(temp, 0, bytes_per_line);

                Memory buffer = temp;
                buffer.address += bytes_per_line;
                buffer.size -= bytes_per_line;

                CompressionStatus result = decompress(buffer, memory);
                if (!result)
                {
                    // NOTE: decompressors complain here that out of data
                    //printLine(Print::Error, "  {} y: {}", result.info, y);
                }

                if (m_interface->cancelled)
                {
                    return;
                }

                process_range(target, buffer, y, y + h);
            });

            y += m_parallel_height;
        }
    }

    void ParserPNG::decode_idot(const Surface& target)
    {
        const size_t bytes_per_line = getBytesPerLine(target.width) + PNG_FILTER_BYTE;

        size_t buffer_size = 0;

        if (m_interlace)
        {
            for (int pass = 0; pass < 7; ++pass)
            {
                buffer_size += getInterlacedPassSize(pass, target.width, target.height);
            }
        }
        else
        {
            buffer_size = (PNG_FILTER_BYTE + getBytesPerLine(target.width)) * target.height;
        }

        printLine(Print::Info, "  buffer bytes: {}", buffer_size);

        // allocate output buffer
        Buffer temp(bytes_per_line + buffer_size + PNG_SIMD_PADDING);

        // zero scanline for filters at the beginning
        std::memset(temp, 0, bytes_per_line);

        Memory buffer(temp + bytes_per_line, buffer_size);

        // ----------------------------------------------------------------------

        // TODO: decompress IDAT at a time and do updates progressively
        Buffer compressed_top;
        Buffer compressed_bottom;

        for (size_t i = 0; i < m_idot_index; ++i)
        {
            compressed_top.append(m_idat[i]);
        }

        for (size_t i = m_idot_index; i < m_idat.size(); ++i)
        {
            compressed_bottom.append(m_idat[i]);
        }

        // ----------------------------------------------------------------------

        size_t top_size = bytes_per_line * m_first_half_height;

        Memory top_buffer;
        top_buffer.address = buffer.address;
        top_buffer.size = top_size;

        Memory bottom_buffer;
        bottom_buffer.address = buffer.address + top_size;
        bottom_buffer.size = buffer.size - top_size;

        ConstMemory top_memory = compressed_top;
        ConstMemory bottom_memory = compressed_bottom;

        auto future = std::async(std::launch::async, [=]
        {
            // Apple uses raw deflate format for iDOT extended IDAT chunks
            CompressionStatus result = deflate::decompress(bottom_buffer, bottom_memory);
            return result.size;
        });

        if (!m_iphoneOptimized)
        {
            // skip zlib header
            top_memory = top_memory.slice(2, top_memory.size - 2);
        }

        auto decompress = deflate::decompress;

        CompressionStatus result = decompress(top_buffer, top_memory);

        size_t bytes_out_top = result.size;
        size_t bytes_out_bottom = future.get();

        printLine(Print::Info, "  output top bytes:     {}", bytes_out_top);
        printLine(Print::Info, "  output bottom bytes:  {}", bytes_out_bottom);
        MANGO_UNREFERENCED(bytes_out_top);
        MANGO_UNREFERENCED(bytes_out_bottom);

        // process image
        process_image(target, buffer);
    }

    bool ParserPNG::decode_std(const Surface& target, ImageDecodeStatus& status)
    {
        const size_t bytes_per_line = getBytesPerLine(target.width) + PNG_FILTER_BYTE;

        size_t buffer_size = 0;

        if (m_interlace)
        {
            for (int pass = 0; pass < 7; ++pass)
            {
                buffer_size += getInterlacedPassSize(pass, target.width, target.height);
            }
        }
        else
        {
            buffer_size = (PNG_FILTER_BYTE + getBytesPerLine(target.width)) * target.height;
        }

        printLine(Print::Info, "  buffer bytes: {}", buffer_size);

        // allocate output buffer
        Buffer temp(bytes_per_line + buffer_size + PNG_SIMD_PADDING);

        // zero scanline for filters at the beginning
        std::memset(temp, 0, bytes_per_line);

        Memory buffer(temp + bytes_per_line, buffer_size);

        // ----------------------------------------------------------------------

        if (m_interface->callback)
        {
#ifdef MANGO_ENABLE_ISAL

            inflate_state state;
            isal_inflate_init(&state);

            // skip zlib header
            size_t first = 2;

            Buffer compressed;

            const size_t lastIndex = m_idat.size() - 1;
            const size_t packetSize = 0x20000;

            int y0 = 0;

            for (size_t i = 0; i < m_idat.size(); ++i)
            {
                compressed.append(m_idat[i]);

                // decompress only when enough data or last IDAT
                if (compressed.size() < packetSize && i < lastIndex)
                {
                    continue;
                }

                state.next_in = const_cast<u8*>(compressed.data() + first);
                state.avail_in = u32(compressed.size() - first);
                first = 0;

                if (m_interface->cancelled)
                {
                    // TODO: cleanup
                    return false;
                }

                state.next_out = buffer.address + state.total_out;
                state.avail_out = u32(buffer.size - state.total_out);

                int s = isal_inflate(&state);

                const char* error = nullptr;
                switch (s)
                {
                    case ISAL_DECOMP_OK:
                        break;
                    case ISAL_END_INPUT:
                        error = "ISAL_END_INPUT.";
                        break;
                    case ISAL_NEED_DICT:
                        error = "ISAL_NEED_DICT.";
                        break;
                    case ISAL_OUT_OVERFLOW:
                        error = "ISAL_OUT_OVERFLOW.";
                        break;
                    case ISAL_INVALID_BLOCK:
                        error = "ISAL_INVALID_BLOCK.";
                        break;
                    case ISAL_INVALID_SYMBOL:
                        error = "ISAL_INVALID_SYMBOL.";
                        break;
                    case ISAL_INVALID_LOOKBACK:
                        error = "ISAL_INVALID_LOOKBACK.";
                        break;
                    case ISAL_INVALID_WRAPPER:
                        error = "ISAL_INVALID_WRAPPER.";
                        break;
                    case ISAL_UNSUPPORTED_METHOD:
                        error = "ISAL_UNSUPPORTED_METHOD.";
                        break;
                    case ISAL_INCORRECT_CHECKSUM:
                        error = "ISAL_INCORRECT_CHECKSUM.";
                        break;
                    default:
                        error = "UNDEFINED.";
                        break;
                }

                if (error)
                {
                    status.setError(error);
                    return false;
                }

                if (!m_interlace)
                {
                    int y1 = int(state.total_out / bytes_per_line);
                    process_range(target, buffer.address + bytes_per_line * y0, y0, y1);
                    y0 = y1;
                }

                compressed.resize(0);
            }


            printLine(Print::Info, "  output bytes: {}", state.total_out);

            if (m_interface->cancelled)
            {
                return false;
            }

            // process image
            if (m_interlace)
            {
                process_image(target, buffer);
            }

#else

            z_stream stream;

            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;
            stream.avail_in = 0;
            stream.next_in = Z_NULL;

            int ret = inflateInit(&stream);
            if (ret != Z_OK)
            {
                status.setError("inflateInit failed.");
                return false;
            }

            Buffer compressed;

            const size_t lastIndex = m_idat.size() - 1;
            const size_t packetSize = 0x20000;

            int y0 = 0;

            for (size_t i = 0; i < m_idat.size(); ++i)
            {
                compressed.append(m_idat[i]);

                // decompress only when enough data or last IDAT
                if (compressed.size() < packetSize && i < lastIndex)
                {
                    continue;
                }

                stream.avail_in = uInt(compressed.size());
                stream.next_in = const_cast<u8*>(compressed.data());

                if (m_interface->cancelled)
                {
                    ret = inflateEnd(&stream);
                    return false;
                }

                do
                {
                    stream.avail_out = uInt(buffer.size - stream.total_out);
                    stream.next_out = buffer.address + stream.total_out;

                    ret = inflate(&stream, Z_NO_FLUSH);
                    if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
                    {
                        inflateEnd(&stream);
                        status.setError("inflate failed.");
                        return false;
                    }
                }
                while (stream.avail_in > 0);

                if (!m_interlace)
                {
                    int y1 = int(stream.total_out / bytes_per_line);
                    process_range(target, buffer.address + bytes_per_line * y0, y0, y1);
                    y0 = y1;
                }

                compressed.resize(0);
            }

            ret = inflateEnd(&stream);
            if (ret != Z_OK && ret != Z_STREAM_END)
            {
                status.setError("inflateEnd failed.");
                return false;
            }

            printLine(Print::Info, "  output bytes: {}", stream.total_out);

            if (m_interface->cancelled)
            {
                return false;
            }

            // process image
            if (m_interlace)
            {
                process_image(target, buffer);
            }
#endif
        }
        else
        {
            Buffer compressed;

            for (auto data : m_idat)
            {
                compressed.append(data);
            }

            if (m_interface->cancelled)
            {
                return false;
            }

            ConstMemory memory = compressed;

            if (!m_iphoneOptimized)
            {
                // skip zlib header
                memory = memory.slice(2, memory.size - 2);
            }

            auto decompress = deflate::decompress;

            CompressionStatus result = decompress(buffer, memory);
            if (!result)
            {
                //printLine(Print::Info, "  {}", result.info);
                status.setError(result.info);
                return false;
            }

            printLine(Print::Info, "  output bytes: {}", result.size);

            if (m_interface->cancelled)
            {
                return false;
            }

            // process image
            process_image(target, buffer);
        }

        return true;
    }

    ImageDecodeStatus ParserPNG::decode(const Surface& dest, bool multithread, Palette* ptr_palette)
    {
        ImageDecodeStatus status;

        m_idat.clear();
        m_idot_index = 0;

        parse();

        if (m_idat.empty() && m_parallel_segments.empty())
        {
            status.setError("No compressed data.");
            return status;
        }

        if (!m_header.success)
        {
            status.setError(m_header.info);
            return status;
        }

        if (m_header.palette)
        {
            if (ptr_palette)
            {
                if (dest.format.isIndexed())
                {
                    status.setError("Decoding target must be indexed.");
                    return status;
                }

                // caller requests palette; give it and decode u8 indices
                *ptr_palette = m_palette;
                m_color_state.palette = nullptr;
            }
            else
            {
                // caller doesn't want palette; lookup RGBA colors from palette
                m_color_state.palette = m_palette.color;
            }
        }

        // default decoding target
        Surface target(dest);
        m_decode_target = dest;

        status.direct = target.width >= m_width &&
                        target.height >= m_height &&
                        target.format == m_header.format;

        std::unique_ptr<Bitmap> temp;

        if (m_number_of_frames > 0)
        {
            temp = std::make_unique<Bitmap>(m_frame.width, m_frame.height, m_header.format);
            target = *temp;
            m_decode_target = *temp;

            // compute frame indices (for external users)
            m_current_frame_index = m_next_frame_index++;
            if (m_next_frame_index >= m_number_of_frames)
            {
                m_next_frame_index = 0;
            }
        }
        else
        {
            if (!status.direct)
            {
                temp = std::make_unique<Bitmap>(m_width, m_height, m_header.format);
                target = *temp;
            }
        }

        Buffer buffer;

        if (m_interface->cancelled)
        {
            return status;
        }

        // The first scanline of segment does not require previous scanline from other segment
        bool plld = m_parallel_height && (m_parallel_flags & 1) != 0;

        if (plld)
        {
            decode_plld(target);
        }
        else if (m_idot_address && multithread)
        {
            decode_idot(target);
        }
        else
        {
            if (!decode_std(target, status))
            {
                return status;
            }
        }

        // These are not wall times; it is cumulative time from all concurrent tasks
        printLine(Print::Info, "  filter: {}.{} ms", m_filter_time / 1000, m_filter_time % 1000);
        printLine(Print::Info, "  color: {}.{} ms", m_color_time / 1000, m_color_time % 1000);

        if (m_interface->cancelled)
        {
            return status;
        }

        if (m_number_of_frames > 0)
        {
            Surface area(dest, m_frame.xoffset, m_frame.yoffset, m_frame.width, m_frame.height);
            TemporaryBitmap bitmap(area, m_header.format);
            blend(bitmap, *temp, ptr_palette);

            if (dest.format != bitmap.format)
            {
                dest.blit(m_frame.xoffset, m_frame.yoffset, bitmap);
            }
        }

        status.current_frame_index = m_current_frame_index;
        status.next_frame_index = m_next_frame_index;

        return status;
    }

    // ------------------------------------------------------------
    // write_png()
    // ------------------------------------------------------------

    static
    void write_filter_sub(u8* dest, const u8* scan, size_t bpp, size_t bytes)
    {
        std::memcpy(dest, scan, bpp);
        bytes -= bpp;
        dest += bpp;

        for (size_t i = 0; i < bytes; ++i)
        {
            dest[i] = u8(scan[i + bpp] - scan[i]);
        }
    }

#if 0
    // NOTE: This is what the UP/AVERAGE/PAETH filters would look like if we wanted to use them.
    //       We don't want to use them because these depend on previous scanline so don't work
    //       with parallel decoder unless each segment starts with NONE or SUB filter which do not
    //       require the previous scanline. Selecting which filter is the best one is too slow so
    //       we go with the SUB filter every time. :)

    static
    void write_filter_up(u8* dest, const u8* scan, const u8* prev, size_t bytes)
    {
        for (size_t i = 0; i < bytes; ++i)
        {
            dest[i] = u8(scan[i] - prev[i]);
        }
    }

    static
    void write_filter_average(u8* dest, const u8* scan, const u8* prev, size_t bpp, size_t bytes)
    {
        for (size_t i = 0; i < bpp; ++i)
        {
            dest[i] = u8(scan[i] - (prev[i] / 2));
        }

        bytes -= bpp;
        dest += bpp;
        prev += bpp;

        for (size_t i = 0; i < bytes; ++i)
        {
            dest[i] = u8(scan[i + bpp] - ((prev[i] + scan[i]) / 2));
        }
    }

    static
    void write_filter_paeth(u8* dest, const u8* scan, const u8* prev, size_t bpp, size_t bytes)
    {
        for (size_t i = 0; i < bpp; ++i)
        {
            dest[i] = u8(scan[i] - prev[i]);
        }

        bytes -= bpp;
        dest += bpp;

        for (size_t i = 0; i < bytes; ++i)
        {
            int b = prev[i + bpp];
            int c = prev[i];
            int a = scan[i];

            int p = b - c;
            int q = a - c;

            int pa = abs(p);
            int pb = abs(q);
            int pc = abs(p + q);

            p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;

            dest[i] = u8(scan[i + bpp] - p);
        }
    }
#endif

    static
    void write_chunk(Stream& stream, u32 chunk_id, ConstMemory memory)
    {
        BigEndianStream s(stream);

        u8 temp[4];
        bigEndian::ustore32(temp, chunk_id);

        u32 crc = crc32(0, Memory(temp, 4));
        crc = crc32(crc, memory);

        u32 chunk_size = u32(memory.size);

        s.write32(chunk_size);
        s.write32(chunk_id);
        s.write(memory);
        s.write32(crc);
    }

    static
    void write_IHDR(Stream& stream, const Surface& surface, u8 color_bits, ColorType color_type)
    {
        MemoryStream buffer;
        BigEndianStream s(buffer);

        s.write32(surface.width);
        s.write32(surface.height);
        s.write8(color_bits);
        s.write8(color_type);
        s.write8(0); // compression
        s.write8(0); // filter
        s.write8(0); // interlace

        write_chunk(stream, u32_mask_rev('I', 'H', 'D', 'R'), buffer);
    }

    static
    void write_PLTE(Stream& stream, const Palette& palette)
    {
        MemoryStream buffer;
        BigEndianStream s(buffer);

        for (int i = 0; i < 256; ++i)
        {
            s.write8(palette[i].r);
            s.write8(palette[i].g);
            s.write8(palette[i].b);
        }

        write_chunk(stream, u32_mask_rev('P', 'L', 'T', 'E'), buffer);
    }

    static
    void write_iCCP(Stream& stream, const ImageEncodeOptions& options)
    {
        if (options.icc.size == 0)
        {
            // empty profile chunk
            return;
        }

        MemoryStream buffer;
        BigEndianStream s(buffer);

        s.write8('-'); // profile name is 1-79 char according to spec. we dont have/need name
        s.write8(0); // profile name null separator
        s.write8(0); // compression method, 0=deflate

        size_t bound = deflate_zlib::bound(options.icc.size);
        Buffer compressed(bound);
        CompressionStatus result = deflate_zlib::compress(compressed, options.icc, options.compression);
        if (result)
        {
            buffer.write(compressed, result.size); // rest of chunk is compressed profile
            write_chunk(stream, u32_mask_rev('i', 'C', 'C', 'P'), buffer);
        }
    }

    static
    void write_pLLD(Stream& stream, int segment_height)
    {
        MemoryStream buffer;
        BigEndianStream s(buffer);

        s.write32(segment_height);
        s.write8(0x01); // parallel filtering is supported (because we always use SUB filter)

        write_chunk(stream, u32_mask_rev('p', 'L', 'L', 'D'), buffer);
    }

    static
    void filter_range(u8* buffer, const Surface& surface, int color_bits, int y0, int y1)
    {
        const int bpp = surface.format.bytes();
        const int bytes_per_scan = surface.width * bpp;

        u8* image = surface.address(0, y0);

        for (int y = y0; y < y1; ++y)
        {
            *buffer++ = FILTER_SUB;
            write_filter_sub(buffer, image, bpp, bytes_per_scan);

#ifdef MANGO_LITTLE_ENDIAN
            byteswap(Memory(buffer, bytes_per_scan), color_bits);
#endif

            buffer += bytes_per_scan;
            image += surface.stride;
        }
    }

    static
    void compress_serial(Stream& stream, ImageEncodeStatus& status, const Surface& surface, int color_bits, const ImageEncodeOptions& options)
    {
        const int bpp = surface.format.bytes();
        const int bytes_per_scan = surface.width * bpp + 1;

        Buffer buffer(bytes_per_scan * surface.height);

        // filtering
        filter_range(buffer, surface, color_bits, 0, surface.height);

        // compute fpng scaling factor
        int factor = 0; // default: not supported
        switch (surface.format.bits)
        {
            case 32:
                factor = 1;
                break;
            case 64:
                factor = 2;
                break;
        }

        if (factor)
        {
            // use fpng for compression; it is always best choice for serial writes which are small
            // because it has very low intertia (and is also very high performance). The performance
            // comes with a string attached: it can only compress 3 and 4 byte size symbols (pixels).

            // compress
            size_t guardband = 4096; // 4K "gimme a break" -guardband
            size_t bytes_in = bytes_per_scan * surface.height;
            Buffer compressed(bytes_in + guardband);
            size_t bytes_out = fpng::pixel_deflate_dyn_4_rle_one_pass(buffer.data(), surface.width * factor, surface.height, compressed.data(), u32(compressed.size()));

            if (!bytes_out)
            {
                // compression failed because the output buffer was too small; as it already was
                // guardband bytes larger than the input we want to change strategy and store the
                // data without compression.

                // The compression algorithm stores data in 65535 byte chunks which have 5 bytes
                // of overhead in form of a header. We compute number of block headers needed on top of
                // the stored data. THEN we still add the guardband.

                u32 block_overhead = u32(((bytes_in + 65534) / 65535) * 5);
                u32 header_and_adler = 6; // 2 byte header, 4 bytes for adler checksum

                compressed.resize(bytes_in + block_overhead + header_and_adler + guardband);
                bytes_out = fpng::write_raw_block(buffer.data(), u32(bytes_in), compressed.data(), u32(compressed.size()));
            }

            if (!bytes_out)
            {
                status.setError("fpng deflate failed.");
                return;
            }

            // compute checksum
            u32 adler = adler32(1, buffer);

            // append adler checksum
            bigEndian::ustore32(compressed.data() + bytes_out, adler);
            bytes_out += 4;

            // write chunkdID + compressed data
            write_chunk(stream, u32_mask_rev('I', 'D', 'A', 'T'), ConstMemory(compressed.data(), bytes_out));
        }
        else
        {
            // use libdeflate for compression

            // compress
            size_t bound = deflate_zlib::bound(buffer.size());
            Buffer compressed(bound);
            size_t bytes_out = deflate_zlib::compress(compressed, buffer, options.compression);

            // write chunkdID + compressed data
            write_chunk(stream, u32_mask_rev('I', 'D', 'A', 'T'), ConstMemory(compressed, bytes_out));
        }
    }

    static
    void compress_parallel(Stream& stream, ImageEncodeStatus& status, const Surface& surface, int segment_height, int color_bits, const ImageEncodeOptions& options)
    {
        const size_t bpp = surface.format.bytes();
        const size_t bytes_per_scan = size_t(surface.width) * bpp + PNG_FILTER_BYTE;

        Buffer buffer(bytes_per_scan * surface.height);

        const int N = div_ceil(surface.height, segment_height);
        const int level = math::clamp(options.compression, 0, 9);

        u32 cumulative_adler = 1;

        ConcurrentQueue q;
        TicketQueue tk;

        std::atomic<bool> encoding_failure { false };

        for (int i = 0; i < N; ++i)
        {
            int y = i * segment_height;
            int h = std::min(segment_height, surface.height - y);

            Memory source;
            source.address = buffer.data() + y * bytes_per_scan;
            source.size = h * bytes_per_scan;

            bool is_first = (i == 0);
            bool is_last = (i == N - 1);

            auto ticket = tk.acquire();

            q.enqueue([=, &encoding_failure, &surface, &stream, &cumulative_adler]
            {
                filter_range(source.address, surface, color_bits, y, y + h);

#if defined(MANGO_ENABLE_ISAL) && !defined(MANGO_CPU_ARM)

                constexpr size_t TEMP_SIZE = 128 * 1024;
                Buffer temp(TEMP_SIZE);

                isal_zstream zstream;
                isal_deflate_init(&zstream);

                zstream.next_in = source.address;
                zstream.avail_in = u32(source.size);
                zstream.next_out = temp;
                zstream.avail_out = u32(TEMP_SIZE);

                // defaults
                size_t level_buffer_size = 0;
                zstream.level = 0;

                switch (level)
                {
                    case 0:
                    case 1:
                    case 2:
                        zstream.level = 0;
                        level_buffer_size = ISAL_DEF_LVL0_DEFAULT;
                        break;

                    case 3:
                    case 4:
                    case 5:
                        zstream.level = 1;
                        level_buffer_size = ISAL_DEF_LVL1_DEFAULT;
                        break;

                    case 6:
                    case 7:
                    case 8:
                        zstream.level = 2;
                        level_buffer_size = ISAL_DEF_LVL2_DEFAULT;
                        break;

                    case 9:
                    case 10:
                        zstream.level = 3;
                        level_buffer_size = ISAL_DEF_LVL3_DEFAULT;
                        break;
                }

                Buffer level_buffer(level_buffer_size);

                zstream.level_buf_size = static_cast<uint32_t>(level_buffer.size());
                zstream.level_buf = level_buffer.data();

                zstream.end_of_stream = 0;
                zstream.flush = NO_FLUSH;

                zstream.gzip_flag = IGZIP_ZLIB;
                zstream.hist_bits = 15; // log2 compression window size

                Buffer compressed;

                while (zstream.avail_in != 0)
                {
                    int res = isal_deflate(&zstream);
                    if (res != COMP_OK)
                    {
                        encoding_failure = true;
                        return;
                    }

                    if (zstream.avail_out == 0)
                    {
                        compressed.append(temp, TEMP_SIZE);
                        zstream.next_out = temp;
                        zstream.avail_out = u32(TEMP_SIZE);
                    }
                }

                compressed.append(temp, TEMP_SIZE - zstream.avail_out);
                zstream.next_out = temp;
                zstream.avail_out = u32(TEMP_SIZE);

                if (is_last)
                {
                    zstream.end_of_stream = 1;
                    zstream.flush = FULL_FLUSH;
                }
                else
                {
                    zstream.flush = FULL_FLUSH;
                }

                int res = isal_deflate(&zstream);
                if (res != ZSTATE_END && res != COMP_OK)
                {
                    encoding_failure = true;
                    return;
                }

                compressed.append(temp, TEMP_SIZE - zstream.avail_out);

                // capture compressed memory
                Memory segment_memory = compressed.acquire();

                u32 segment_adler = adler32(1, source);
                u32 segment_length = u32(source.size);

#else

                constexpr size_t TEMP_SIZE = 128 * 1024;
                Buffer temp(TEMP_SIZE);

                z_stream strm;

                strm.zalloc = 0;
                strm.zfree = 0;
                strm.next_in = source.address;
                strm.avail_in = uInt(source.size);
                strm.next_out = temp;
                strm.avail_out = TEMP_SIZE;

                ::deflateInit(&strm, level);

                Buffer compressed;

                while (strm.avail_in != 0)
                {
                    int res = ::deflate(&strm, Z_NO_FLUSH);
                    if (res != Z_OK)
                    {
                        encoding_failure = true;
                        return;
                    }

                    if (strm.avail_out == 0)
                    {
                        compressed.append(temp, TEMP_SIZE);
                        strm.next_out = temp;
                        strm.avail_out = TEMP_SIZE;
                    }
                }

                compressed.append(temp, TEMP_SIZE - strm.avail_out);
                strm.next_out = temp;
                strm.avail_out = TEMP_SIZE;

                int flush = is_last ? Z_FINISH : Z_FULL_FLUSH;
                int res = ::deflate(&strm, flush);
                if (res != Z_STREAM_END && res != Z_OK)
                {
                    encoding_failure = true;
                    return;
                }

                ::deflateEnd(&strm);

                compressed.append(temp, TEMP_SIZE - strm.avail_out);

                // capture compressed memory
                Memory segment_memory = compressed.acquire();

                u32 segment_adler = strm.adler;
                u32 segment_length = u32(source.size);

#endif

                ticket.consume([=, &cumulative_adler, &stream]
                {
                    cumulative_adler = ::adler32_combine(cumulative_adler, segment_adler, segment_length);

                    Memory c = segment_memory;

                    if (!is_first)
                    {
                        // trim zlib header
                        c.address += 2;
                        c.size -= 2;
                    }

                    if (is_last)
                    {
                        // 4 last bytes is adler, overwrite it with cumulative adler
                        bigEndian::ustore32(c.address + c.size - 4, cumulative_adler);
                    }

                    // write chunkdID + compressed data
                    write_chunk(stream, u32_mask_rev('I', 'D', 'A', 'T'), c);

                    // free compressed memory
                    Buffer::release(segment_memory);
                });
            });
        }

        q.wait();
        tk.wait();

        if (encoding_failure)
        {
#if defined(MANGO_ENABLE_ISAL) && !defined(MANGO_CPU_ARM)
            status.setError("ISAL encoding failure.");
#else
            status.setError("ZLib encoding failure.");
#endif
        }
    }

    static
    int configure_segment(const Surface& surface, const ImageEncodeOptions& options)
    {
        int height = 0;

        if (options.parallel)
        {
            size_t scan_bytes = surface.width * surface.format.bytes();
            size_t image_bytes = surface.height * scan_bytes;

            constexpr size_t block_size = 1024 * 1024;
            size_t N = std::min<size_t>(128, image_bytes / block_size);

            if (N > 1)
            {
                height = int(surface.height / N);
                printLine(Print::Info, "[image]");
                printLine(Print::Info, "    {} x {}", surface.width, surface.height);
                printLine(Print::Info, "    size: {} KB", image_bytes / 1024);
                printLine(Print::Info, "[segment]");
                printLine(Print::Info, "    N:     {}", int(N));
                printLine(Print::Info, "    height: {}", height);
                printLine(Print::Info, "    size:   {} KB", block_size / 1024);
            }
        }

        return height;
    }

    static
    void write_png(Stream& stream, ImageEncodeStatus& status, const Surface& surface, u8 color_bits, ColorType color_type, const ImageEncodeOptions& options)
    {
        BigEndianStream s(stream);

        // write magic
        s.write64(PNG_HEADER_MAGIC);

        write_IHDR(stream, surface, color_bits, color_type);
        write_iCCP(stream, options);

        int segment_height = configure_segment(surface, options);

        if (options.palette.size > 0)
        {
            write_PLTE(stream, options.palette);
            segment_height = 0; // parallel encoder only supports 24 and 32 bit color
        }

        if (segment_height)
        {
            write_pLLD(stream, segment_height);
            compress_parallel(stream, status, surface, segment_height, color_bits, options);
        }
        else
        {
            compress_serial(stream, status, surface, color_bits, options);
        }

        // write IEND
        s.write32(0);
        s.write32(0x49454e44);
        s.write32(0xae426082);
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecodeInterface
    {
        ParserPNG m_parser;

        Interface(ConstMemory memory)
            : m_parser(memory)
        {
            m_parser.setInterface(this);

            async = true;
            header = m_parser.getHeader();
            icc = m_parser.icc();
        }

        ~Interface()
        {
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

            if (options.palette && header.palette)
            {
                status = m_parser.decode(dest, options.multithread, nullptr);
            }
            else
            {
                status = m_parser.decode(dest, options.multithread, options.palette);
            }

            return status;
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    ImageEncodeStatus imageEncode(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        if (options.palette.size > 0)
        {
            if (options.palette.size != 256)
            {
                status.setError("[ImageEncoder.PNG] Incorrect palette size - must be 0 or 256 (size: {}).", options.palette.size);
                return status;
            }

            if (!surface.format.isIndexed() || surface.format.bits != 8)
            {
                status.setError("[ImageEncoder.PNG] Incorrect format - must be Indexed 8 bit (bits: {}).", surface.format.bits);
                return status;
            }

            // encode indexed color image
            write_png(stream, status, surface, 8, COLOR_TYPE_PALETTE, options);

            return status;
        }

        // configure encoding parameters

        u8 color_bits = 8;
        ColorType color_type = COLOR_TYPE_RGBA;
        Format format;

        if (surface.format.isLuminance())
        {
            if (surface.format.isAlpha())
            {
                color_type = COLOR_TYPE_IA;

                if (surface.format.size[0] > 8)
                {
                    color_bits = 16;
                    format = LuminanceFormat(32, Format::UNORM, 16, 16);
                }
                else
                {
                    color_bits = 8;
                    format = LuminanceFormat(16, Format::UNORM, 8, 8);
                }
            }
            else
            {
                color_type = COLOR_TYPE_I;

                if (surface.format.size[0] > 8)
                {
                    color_bits = 16;
                    format = LuminanceFormat(16, Format::UNORM, 16, 0);
                }
                else
                {
                    color_bits = 8;
                    format = LuminanceFormat(8, Format::UNORM, 8, 0);
                }
            }
        }
        else
        {
            // always encode alpha in non-luminance formats
            color_type = COLOR_TYPE_RGBA;

            if (surface.format.size[0] > 8 && surface.format.type == Format::UNORM)
            {
                // the UNORM is required above because we don't have color conversion
                // from FLOAT / HALF to 16 bit UNORM
                color_bits = 16;
                format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
            }
            else
            {
                color_bits = 8;
                format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
        }

        // convert to correct format when required
        TemporaryBitmap temp(surface, format);

        // encode
        write_png(stream, status, temp, color_bits, color_type, options);

        return status;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecPNG()
    {
        registerImageDecoder(createInterface, ".png");
        registerImageEncoder(imageEncode, ".png");
    }

} // namespace mango::image
