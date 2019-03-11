/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

#include "../../external/miniz/miniz.h"
#undef crc32 // fix miniz pollution

#define ID "[ImageDecoder.PNG] "
#define FILTER_BYTE 1
//#define DECODE_WITH_MINIZ
//#define PNG_ENABLE_PRINT

namespace
{
    using namespace mango;

    // ------------------------------------------------------------
    // print()
    // ------------------------------------------------------------

#ifdef PNG_ENABLE_PRINT
    #define print(...) printf(__VA_ARGS__)
#else
    #define print(...)
#endif

    // ------------------------------------------------------------
    // stb zlib
    // ------------------------------------------------------------

#ifndef DECODE_WITH_MINIZ

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman

#define STBIDEF
#define stbi_inline inline
#define STBI_ASSERT assert
#define stbi__err(x,y)  0
using stbi_uc = u8;
using stbi__uint16 = u16;
using stbi__uint32 = u32;

#ifndef STBI_MALLOC
#define STBI_MALLOC(sz)           malloc(sz)
#define STBI_REALLOC(p,newsz)     realloc(p,newsz)
#define STBI_FREE(p)              free(p)
#endif

#ifndef STBI_REALLOC_SIZED
#define STBI_REALLOC_SIZED(p,oldsz,newsz) STBI_REALLOC(p,newsz)
#endif

#define STBI_NOTUSED(v)  (void)(v)

static void *stbi__malloc(size_t size)
{
    return STBI_MALLOC(size);
}


// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBI__ZFAST_BITS  9 // accelerate all cases in default tables
#define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct
{
   stbi__uint16 fast[1 << STBI__ZFAST_BITS];
   stbi__uint16 firstcode[16];
   int maxcode[17];
   stbi__uint16 firstsymbol[16];
   stbi_uc  size[288];
   stbi__uint16 value[288];
} stbi__zhuffman;

stbi_inline static int stbi__bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

stbi_inline static int stbi__bit_reverse(int v, int bits)
{
   STBI_ASSERT(bits <= 16);
   // to bit reverse n bits, reverse 16 and shift
   // e.g. 11 bits, bit reverse and shift away 5
   return stbi__bitreverse16(v) >> (16-bits);
}

static int stbi__zbuild_huffman(stbi__zhuffman *z, const stbi_uc *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   // DEFLATE spec for generating codes
   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 0, sizeof(z->fast));
   for (i=0; i < num; ++i)
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      if (sizes[i] > (1 << i))
         return stbi__err("bad sizes", "Corrupt PNG");
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (stbi__uint16) code;
      z->firstsymbol[i] = (stbi__uint16) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return stbi__err("bad codelengths","Corrupt PNG");
      z->maxcode[i] = code << (16-i); // preshift for inner loop
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000; // sentinel
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         stbi__uint16 fastv = (stbi__uint16) ((s << 9) | i);
         z->size [c] = (stbi_uc     ) s;
         z->value[c] = (stbi__uint16) i;
         if (s <= STBI__ZFAST_BITS) {
            int j = stbi__bit_reverse(next_code[s],s);
            while (j < (1 << STBI__ZFAST_BITS)) {
               z->fast[j] = fastv;
               j += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct
{
   stbi_uc *zbuffer, *zbuffer_end;
   int num_bits;
   stbi__uint32 code_buffer;

   char *zout;
   char *zout_start;
   char *zout_end;
   int   z_expandable;

   stbi__zhuffman z_length, z_distance;
} stbi__zbuf;

stbi_inline static stbi_uc stbi__zget8(stbi__zbuf *z)
{
   if (z->zbuffer >= z->zbuffer_end) return 0;
   return *z->zbuffer++;
}

static void stbi__fill_bits(stbi__zbuf *z)
{
   do {
      STBI_ASSERT(z->code_buffer < (1U << z->num_bits));
      z->code_buffer |= (unsigned int) stbi__zget8(z) << z->num_bits;
      z->num_bits += 8;
   } while (z->num_bits <= 24);
}

stbi_inline static unsigned int stbi__zreceive(stbi__zbuf *z, int n)
{
   unsigned int k;
   if (z->num_bits < n) stbi__fill_bits(z);
   k = z->code_buffer & ((1 << n) - 1);
   z->code_buffer >>= n;
   z->num_bits -= n;
   return k;
}

static int stbi__zhuffman_decode_slowpath(stbi__zbuf *a, stbi__zhuffman *z)
{
   int b,s,k;
   // not resolved by fast table, so compute it the slow way
   // use jpeg approach, which requires MSbits at top
   k = stbi__bit_reverse(a->code_buffer, 16);
   for (s=STBI__ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s == 16) return -1; // invalid code!
   // code size is s, so:
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   STBI_ASSERT(z->size[b] == s);
   a->code_buffer >>= s;
   a->num_bits -= s;
   return z->value[b];
}

stbi_inline static int stbi__zhuffman_decode(stbi__zbuf *a, stbi__zhuffman *z)
{
   int b,s;
   if (a->num_bits < 16) stbi__fill_bits(a);
   b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
   if (b) {
      s = b >> 9;
      a->code_buffer >>= s;
      a->num_bits -= s;
      return b & 511;
   }
   return stbi__zhuffman_decode_slowpath(a, z);
}

static int stbi__zexpand(stbi__zbuf *z, char *zout, int n)  // need to make room for n bytes
{
   char *q;
   int cur, limit, old_limit;
   z->zout = zout;
   if (!z->z_expandable) return stbi__err("output buffer limit","Corrupt PNG");
   cur   = (int) (z->zout     - z->zout_start);
   limit = old_limit = (int) (z->zout_end - z->zout_start);
   while (cur + n > limit)
      limit *= 2;
   q = (char *) STBI_REALLOC_SIZED(z->zout_start, old_limit, limit);
   STBI_NOTUSED(old_limit);
   if (q == NULL) return stbi__err("outofmem", "Out of memory");
   z->zout_start = q;
   z->zout       = q + cur;
   z->zout_end   = q + limit;
   return 1;
}

static const int stbi__zlength_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static const int stbi__zlength_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static const int stbi__zdist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static const int stbi__zdist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int stbi__parse_huffman_block(stbi__zbuf *a)
{
   char *zout = a->zout;
   for(;;) {
      int z = stbi__zhuffman_decode(a, &a->z_length);
      if (z < 256) {
         if (z < 0) return stbi__err("bad huffman code","Corrupt PNG"); // error in huffman codes
         if (zout >= a->zout_end) {
            if (!stbi__zexpand(a, zout, 1)) return 0;
            zout = a->zout;
         }
         *zout++ = (char) z;
      } else {
         stbi_uc *p;
         int len,dist;
         if (z == 256) {
            a->zout = zout;
            return 1;
         }
         z -= 257;
         len = stbi__zlength_base[z];
         if (stbi__zlength_extra[z]) len += stbi__zreceive(a, stbi__zlength_extra[z]);
         z = stbi__zhuffman_decode(a, &a->z_distance);
         if (z < 0) return stbi__err("bad huffman code","Corrupt PNG");
         dist = stbi__zdist_base[z];
         if (stbi__zdist_extra[z]) dist += stbi__zreceive(a, stbi__zdist_extra[z]);
         if (zout - a->zout_start < dist) return stbi__err("bad dist","Corrupt PNG");
         if (zout + len > a->zout_end) {
            if (!stbi__zexpand(a, zout, len)) return 0;
            zout = a->zout;
         }
         p = (stbi_uc *) (zout - dist);
         if (dist == 1) { // run of one byte; common in images.
            stbi_uc v = *p;
            if (len) { do *zout++ = v; while (--len); }
         } else {
            if (len) { do *zout++ = *p++; while (--len); }
         }
      }
   }
}

static int stbi__compute_huffman_codes(stbi__zbuf *a)
{
   static const stbi_uc length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   stbi__zhuffman z_codelength;
   stbi_uc lencodes[286+32+137];//padding for maximum single op
   stbi_uc codelength_sizes[19];
   int i,n;

   int hlit  = stbi__zreceive(a,5) + 257;
   int hdist = stbi__zreceive(a,5) + 1;
   int hclen = stbi__zreceive(a,4) + 4;
   int ntot  = hlit + hdist;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = stbi__zreceive(a,3);
      codelength_sizes[length_dezigzag[i]] = (stbi_uc) s;
   }
   if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < ntot) {
      int c = stbi__zhuffman_decode(a, &z_codelength);
      if (c < 0 || c >= 19) return stbi__err("bad codelengths", "Corrupt PNG");
      if (c < 16)
         lencodes[n++] = (stbi_uc) c;
      else {
         stbi_uc fill = 0;
         if (c == 16) {
            c = stbi__zreceive(a,2)+3;
            if (n == 0) return stbi__err("bad codelengths", "Corrupt PNG");
            fill = lencodes[n-1];
         } else if (c == 17)
            c = stbi__zreceive(a,3)+3;
         else {
            STBI_ASSERT(c == 18);
            c = stbi__zreceive(a,7)+11;
         }
         if (ntot - n < c) return stbi__err("bad codelengths", "Corrupt PNG");
         memset(lencodes+n, fill, c);
         n += c;
      }
   }
   if (n != ntot) return stbi__err("bad codelengths","Corrupt PNG");
   if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
   if (!stbi__zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

static int stbi__parse_uncompressed_block(stbi__zbuf *a)
{
   stbi_uc header[4];
   int len,nlen,k;
   if (a->num_bits & 7)
      stbi__zreceive(a, a->num_bits & 7); // discard
   // drain the bit-packed data into header
   k = 0;
   while (a->num_bits > 0) {
      header[k++] = (stbi_uc) (a->code_buffer & 255); // suppress MSVC run-time check
      a->code_buffer >>= 8;
      a->num_bits -= 8;
   }
   STBI_ASSERT(a->num_bits == 0);
   // now fill header the normal way
   while (k < 4)
      header[k++] = stbi__zget8(a);
   len  = header[1] * 256 + header[0];
   nlen = header[3] * 256 + header[2];
   if (nlen != (len ^ 0xffff)) return stbi__err("zlib corrupt","Corrupt PNG");
   if (a->zbuffer + len > a->zbuffer_end) return stbi__err("read past buffer","Corrupt PNG");
   if (a->zout + len > a->zout_end)
      if (!stbi__zexpand(a, a->zout, len)) return 0;
   memcpy(a->zout, a->zbuffer, len);
   a->zbuffer += len;
   a->zout += len;
   return 1;
}

static int stbi__parse_zlib_header(stbi__zbuf *a)
{
   int cmf   = stbi__zget8(a);
   int cm    = cmf & 15;
   /* int cinfo = cmf >> 4; */
   int flg   = stbi__zget8(a);
   if ((cmf*256+flg) % 31 != 0) return stbi__err("bad zlib header","Corrupt PNG"); // zlib spec
   if (flg & 32) return stbi__err("no preset dict","Corrupt PNG"); // preset dictionary not allowed in png
   if (cm != 8) return stbi__err("bad compression","Corrupt PNG"); // DEFLATE required for png
   // window = 1 << (8 + cinfo)... but who cares, we fully buffer output
   return 1;
}

static const stbi_uc stbi__zdefault_length[288] =
{
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8
};
static const stbi_uc stbi__zdefault_distance[32] =
{
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};
/*
Init algorithm:
{
   int i;   // use <= to match clearly with spec
   for (i=0; i <= 143; ++i)     stbi__zdefault_length[i]   = 8;
   for (   ; i <= 255; ++i)     stbi__zdefault_length[i]   = 9;
   for (   ; i <= 279; ++i)     stbi__zdefault_length[i]   = 7;
   for (   ; i <= 287; ++i)     stbi__zdefault_length[i]   = 8;

   for (i=0; i <=  31; ++i)     stbi__zdefault_distance[i] = 5;
}
*/

static int stbi__parse_zlib(stbi__zbuf *a, int parse_header)
{
   int final, type;
   if (parse_header)
      if (!stbi__parse_zlib_header(a)) return 0;
   a->num_bits = 0;
   a->code_buffer = 0;
   do {
      final = stbi__zreceive(a,1);
      type = stbi__zreceive(a,2);
      if (type == 0) {
         if (!stbi__parse_uncompressed_block(a)) return 0;
      } else if (type == 3) {
         return 0;
      } else {
         if (type == 1) {
            // use fixed code lengths
            if (!stbi__zbuild_huffman(&a->z_length  , stbi__zdefault_length  , 288)) return 0;
            if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance,  32)) return 0;
         } else {
            if (!stbi__compute_huffman_codes(a)) return 0;
         }
         if (!stbi__parse_huffman_block(a)) return 0;
      }
   } while (!final);
   return 1;
}

static int stbi__do_zlib(stbi__zbuf *a, char *obuf, int olen, int exp, int parse_header)
{
   a->zout_start = obuf;
   a->zout       = obuf;
   a->zout_end   = obuf + olen;
   a->z_expandable = exp;

   return stbi__parse_zlib(a, parse_header);
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header)
{
   stbi__zbuf a;
   char *p = (char *) stbi__malloc(initial_size);
   if (p == NULL) return NULL;
   a.zbuffer = (stbi_uc *) buffer;
   a.zbuffer_end = (stbi_uc *) buffer + len;
   if (stbi__do_zlib(&a, p, initial_size, 1, parse_header)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      STBI_FREE(a.zout_start);
      return NULL;
   }
}

#endif // DECODE_WITH_MINIZ

    // ------------------------------------------------------------
    // PaethPredictor()
    // ------------------------------------------------------------

    inline u8 PaethPredictor(u8 a, u8 b, u8 c)
    {
        const int x = b - c;
        const int y = a - c;
        const int pa = abs(x);
        const int pb = abs(y);
        const int pc = abs(x + y);
        int pred;
        if (pa <= pb && pa <= pc)
            pred = a;
        else if (pb <= pc)
            pred = b;
        else
            pred = c;
        return pred;
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
            assert(pass >=0 && pass < 7);

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

    enum ColorType
    {
        COLOR_TYPE_I       = 0,
        COLOR_TYPE_RGB     = 2,
        COLOR_TYPE_PALETTE = 3,
        COLOR_TYPE_IA      = 4,
        COLOR_TYPE_RGBA    = 6,
    };

    struct Chromaticity
    {
        float2 white;
        float2 red;
        float2 green;
        float2 blue;
    };

    class ParserPNG
    {
    protected:
        Memory m_memory;

        u8* m_pointer = nullptr;
        u8* m_end = nullptr;
        const char* m_error = nullptr;

        Buffer m_compressed;

        // IHDR
        int m_width;
        int m_height;
        int m_bit_depth;
        int m_color_type;
        int m_compression;
        int m_filter;
        int m_interlace;

        int m_channels;
        int m_bytes_per_line;

        // PLTE
        Palette m_palette;

        // tRNS
        bool m_transparent_enable = false;
        u16 m_transparent_sample[3];
        ColorBGRA m_transparent_color;

        // cHRM
        Chromaticity m_chromaticity;

        // gAMA
        float m_gamma = 0.0f;

        // sBIT
        u8 m_scale_bits[4];

        // sRGB
        u8 m_srgb_render_intent = -1;

        void setError(const char* error);

        void read_IHDR(BigEndianPointer p, u32 size);
        void read_IDAT(BigEndianPointer p, u32 size);
        void read_PLTE(BigEndianPointer p, u32 size);
        void read_tRNS(BigEndianPointer p, u32 size);
        void read_cHRM(BigEndianPointer p, u32 size);
        void read_gAMA(BigEndianPointer p, u32 size);
        void read_sBIT(BigEndianPointer p, u32 size);
        void read_sRGB(BigEndianPointer p, u32 size);

        void parse();
        void filter(u8* buffer, int bytes, int height);
        void deinterlace1to4(u8* output, int stride, u8* buffer);
        void deinterlace8to16(u8* output, int stride, u8* buffer);

        void process_i1to4   (u8* dest, int stride, const u8* src);
        void process_i8      (u8* dest, int stride, const u8* src);
        void process_rgb8    (u8* dest, int stride, const u8* src);
        void process_pal1to4 (u8* dest, int stride, const u8* src, Palette* palette);
        void process_pal8    (u8* dest, int stride, const u8* src, Palette* palette);
        void process_ia8     (u8* dest, int stride, const u8* src);
        void process_rgba8   (u8* dest, int stride, const u8* src);
        void process_i16     (u8* dest, int stride, const u8* src);
        void process_rgb16   (u8* dest, int stride, const u8* src);
        void process_ia16    (u8* dest, int stride, const u8* src);
        void process_rgba16  (u8* dest, int stride, const u8* src);

        void process(u8* image, int stride, u8* src, Palette* palette);

    public:
        ParserPNG(Memory memory);
        ~ParserPNG();

        const char* getError() const;

        ImageHeader header() const;
        const char* decode(Surface& dest, Palette* palette);
    };

    // ------------------------------------------------------------
    // ParserPNG
    // ------------------------------------------------------------

    ParserPNG::ParserPNG(Memory memory)
        : m_memory(memory)
        , m_end(memory.address + memory.size)
    {
        BigEndianPointer p = memory.address;

        // read header magic
        const u64 magic = p.read64();
        if (magic != 0x89504e470d0a1a0a)
        {
            setError("Incorrect header magic.");
            return;
        }

        // read first chunk; it must be IHDR
        const u32 size = p.read32();
        const u32 id = p.read32();
        if (id != make_u32rev('I', 'H', 'D', 'R'))
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

    ParserPNG::~ParserPNG()
    {
    }

    void ParserPNG::setError(const char* error)
    {
        if (!m_error)
        {
            m_error = error;
        }
    }

    const char* ParserPNG::getError() const
    {
        return m_error;
    }

    void ParserPNG::read_IHDR(BigEndianPointer p, u32 size)
    {
        print("[\"IHDR\"] %d bytes\n", size);

        if (size != 13)
        {
            setError("Incorrect IHDR chunk size.");
            return;
        }

        m_width = p.read32();
        m_height = p.read32();
        m_bit_depth = p.read8();
        m_color_type = p.read8();
        m_compression = p.read8();
        m_filter = p.read8();
        m_interlace = p.read8();

        if (m_width > 0x8000 || m_height > 0x8000)
        {
            setError("Too large image.");
            return;
        }

        if (!u32_is_power_of_two(m_bit_depth))
        {
            setError("Incorrect bit depth.");
            return;
        }

        // look-ahead into the chunks to see if we have transparency information
        p += 4; // skip crc
        for (; p < m_end - 8;)
        {
            const u32 size = p.read32();
            const u32 id = p.read32();
            switch (id)
            {
                case make_u32rev('t', 'R', 'N', 'S'):
                    m_transparent_enable = true;
                    break;
            }
            p += (size + 4);
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
                setError("Incorrect color type.");
                return;
        }

        const int log2bits = u32_log2(m_bit_depth);
        if (log2bits < minBits || log2bits > maxBits)
        {
            setError("Unsupported bit depth for color type.");
            return;
        }

        // load default scaling values (override from sBIT chunk)
        for (int i = 0; i < m_channels; ++i)
        {
            m_scale_bits[i] = m_bit_depth;
        }

        m_bytes_per_line = m_channels * ((m_bit_depth * m_width + 7) / 8);

        print("  Image: (%d x %d), %d bits\n", m_width, m_height, m_bit_depth);
        print("  Color:       %d\n", m_color_type);
        print("  Compression: %d\n", m_compression);
        print("  Filter:      %d\n", m_filter);
        print("  Interlace:   %d\n", m_interlace);
    }

    void ParserPNG::read_IDAT(BigEndianPointer p, u32 size)
    {
        m_compressed.write(p, size);
    }

    void ParserPNG::read_PLTE(BigEndianPointer p, u32 size)
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
            m_palette[i] = ColorBGRA(p[0], p[1], p[2], 0xff);
            p += 3;
        }
    }

    void ParserPNG::read_tRNS(BigEndianPointer p, u32 size)
    {
        if (m_color_type == COLOR_TYPE_I)
        {
            if (size != 2)
            {
                setError("Incorrect tRNS chunk size.");
                return;
            }

            m_transparent_enable = true;
            m_transparent_sample[0] = p.read16();
        }
        else if (m_color_type == COLOR_TYPE_RGB)
        {
            if (size != 6)
            {
                setError("Incorrect tRNS chunk size.");
                return;
            }

            m_transparent_enable = true;
            m_transparent_sample[0] = p.read16();
            m_transparent_sample[1] = p.read16();
            m_transparent_sample[2] = p.read16();
            m_transparent_color = ColorBGRA(m_transparent_sample[0] & 0xff,
                                            m_transparent_sample[1] & 0xff,
                                            m_transparent_sample[2] & 0xff, 0xff);
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
            setError("Incorrect color type for alpha palette.");
        }
    }

    void ParserPNG::read_cHRM(BigEndianPointer p, u32 size)
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

    void ParserPNG::read_gAMA(BigEndianPointer p, u32 size)
    {
        if (size != 4)
        {
            setError("Incorrect gAMA chunk size.");
            return;
        }

        m_gamma = p.read32() / 100000.0f;
    }

    void ParserPNG::read_sBIT(BigEndianPointer p, u32 size)
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

    void ParserPNG::read_sRGB(BigEndianPointer p, u32 size)
    {
        if (size != 1)
        {
            setError("Incorrect sRGB chunk size.");
            return;
        }

        m_srgb_render_intent = p[0];
    }

    ImageHeader ParserPNG::header() const
    {
        ImageHeader header;

        if (!m_error)
        {
            header.width   = m_width;
            header.height  = m_height;
            header.depth   = 0;
            header.levels  = 0;
            header.faces   = 0;
			header.palette = false;
            header.compression = TextureCompression::NONE;

            // force alpha channel on when transparency is enabled
            int color_type = m_color_type;
            if (m_transparent_enable && color_type != COLOR_TYPE_PALETTE)
            {
                color_type |= 4;
            }

            // select decoding format
            switch (color_type)
            {
                case COLOR_TYPE_I:
                    header.format = m_bit_depth <= 8 ?
                        Format(8, 0xff, 0xff, 0xff, 0) :
                        Format(16, 0xffff, 0xffff, 0xffff, 0);
                    break;

                case COLOR_TYPE_IA:
                    header.format = m_bit_depth <= 8 ?
                        Format(16, 0xff, 0xff, 0xff, 0xff00) :
                        Format(32, 0xffff, 0xffff, 0xffff, 0xffff0000);
                    break;

                case COLOR_TYPE_PALETTE:
                    header.palette = true;
                    header.format = Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
                    break;

                case COLOR_TYPE_RGB:
                case COLOR_TYPE_RGBA:
                    header.format = m_bit_depth <= 8 ?
                        Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8) :
                        Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16); // RGBA!
                    break;
            }
        }

        return header;
    }

    void ParserPNG::parse()
    {
        BigEndianPointer p = m_pointer;

        for (; p < m_end - 8;)
        {
            const u32 size = p.read32();
            const u32 id = p.read32();

            print("[\"%c%c%c%c\"] %d bytes\n", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);

            // check that we won't read past end of file
            if (p + size + 4 > m_end)
            {
                setError("Corrupted file.\n");
                return;
            }

            switch (id)
            {
                case make_u32rev('I', 'H', 'D', 'R'):
                    setError("File can only have one IHDR chunk.");
                    break;

                case make_u32rev('P', 'L', 'T', 'E'):
                    read_PLTE(p, size);
                    break;

                case make_u32rev('t', 'R', 'N', 'S'):
                    read_tRNS(p, size);
                    break;

                case make_u32rev('g', 'A', 'M', 'A'):
                    read_gAMA(p, size);
                    break;

                case make_u32rev('s', 'B', 'I', 'T'):
                    read_sBIT(p, size);
                    break;

                case make_u32rev('s', 'R', 'G', 'B'):
                    read_sRGB(p, size);
                    break;

                case make_u32rev('c', 'H', 'R', 'M'):
                    read_cHRM(p, size);
                    break;

                case make_u32rev('I', 'D', 'A', 'T'):
                    read_IDAT(p, size);
                    break;

                case make_u32rev('p', 'H', 'Y', 's'):
                case make_u32rev('b', 'K', 'G', 'D'):
                case make_u32rev('z', 'T', 'X', 't'):
                case make_u32rev('t', 'E', 'X', 't'):
                case make_u32rev('t', 'I', 'M', 'E'):
                    // NOTE: ignoring these chunks
                    break;

                case make_u32rev('I', 'E', 'N', 'D'):
                    // terminate parsing (required for files with junk after the IEND marker)
                    p = m_end; 
                    break;

                default:
                    print("UNKNOWN CHUNK: [\"%c%c%c%c\"] %d bytes\n", (id >> 24), (id >> 16), (id >> 8), (id >> 0), size);
                    break;
            }

            if (m_error)
            {
                // error occured while reading chunks
                return;
            }

            p += size;
            p += 4; // skip CRC
        }
    }

    void ParserPNG::filter(u8* buffer, int bytes, int height)
    {
        // zero scanline
        std::vector<u8> zeros(bytes, 0);
        const u8* p = zeros.data();

        u8 prev[16];

        const int size = (m_bit_depth < 8) ? 1 : m_channels * m_bit_depth / 8;
        if (size > 8)
            return;

        u8* s = buffer;

        for (int y = 0; y < height; ++y)
        {
            int method = *s++;
            u8* buf = s;

            switch (method)
            {
                case 0:
                    break;

                case 1:
                    std::memset(prev, 0, 8);
                    for (int x = 0; x < bytes; x += size)
                    {
                        for (int i = 0; i < size; ++i)
                        {
                            const u8 value = buf[i] + prev[i];
                            prev[i] = value;
                            buf[i] = value;
                        }
                        buf += size;
                    }
                    break;

                case 2:
                    for (int x = 0; x < bytes; ++x)
                    {
                        const u8 value = buf[x] + p[x];
                        buf[x] = value;
                    }
                    break;

                case 3:
                    std::memset(prev, 0, 8);
                    for (int x = 0; x < bytes; x += size)
                    {
                        for (int i = 0; i < size; ++i)
                        {
                            const u8 value = buf[i] + ((prev[i] + p[i]) >> 1);
                            prev[i] = value;
                            buf[i] = value;
                        }
                        p += size;
                        buf += size;
                    }
                    break;

                case 4:
                    std::memset(prev, 0, 16);
                    for (int x = 0; x < bytes; x += size)
                    {
                        for (int i = 0; i < size; ++i)
                        {
                            u8 A = prev[i + 0];
                            u8 C = prev[i + 8];
                            u8 B = p[i];
                            u8 value = buf[i] + PaethPredictor(A, B, C);
                            buf[i] = value;
                            prev[i + 0] = value;
                            prev[i + 8] = B;
                        }
                        p += size;
                        buf += size;
                    }
                    break;
            }

            p = s;
            s += bytes;
        }
    }

    void ParserPNG::deinterlace1to4(u8* output, int stride, u8* buffer)
    {
        u8* p = buffer;

        const int samples = 8 / m_bit_depth;
        const int mask = samples - 1;
        const int shift = u32_log2(samples);
        const int valueShift = u32_log2(m_bit_depth);
        const int valueMask = (1 << m_bit_depth) - 1;

        for (int pass = 0; pass < 7; ++pass)
        {
            AdamInterleave adam(pass, m_width, m_height);
            print("  pass: %d (%d x %d)\n", pass, adam.w, adam.h);

            const int bw = FILTER_BYTE + ((adam.w + mask) >> shift);
            filter(p, bw - FILTER_BYTE, adam.h);

            if (adam.w && adam.h)
            {
                for (int y = 0; y < adam.h; ++y)
                {
                    const int yoffset = (y << adam.yspc) + adam.yorig;
                    u8* dest = output + yoffset * stride + FILTER_BYTE;
                    u8* src = p + y * bw + FILTER_BYTE;

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
                p += bw * adam.h;
            }
        }
    }

    void ParserPNG::deinterlace8to16(u8* output, int stride, u8* buffer)
    {
        u8* p = buffer;
        const int size = m_bytes_per_line / m_width;

        for (int pass = 0; pass < 7; ++pass)
        {
            AdamInterleave adam(pass, m_width, m_height);
            print("  pass: %d (%d x %d)\n", pass, adam.w, adam.h);

            const int bw = FILTER_BYTE + adam.w * size;
            filter(p, bw - FILTER_BYTE, adam.h);

            if (adam.w && adam.h)
            {
                const int ps = adam.w * size + FILTER_BYTE;

                for (int y = 0; y < adam.h; ++y)
                {
                    const int yoffset = (y << adam.yspc) + adam.yorig;
                    u8* dest = output + yoffset * stride + FILTER_BYTE;
                    u8* src = p + y * ps + FILTER_BYTE;

                    dest += adam.xorig * size;
                    const int xmax = (adam.w * size) << adam.xspc;
                    const int xstep = size << adam.xspc;

                    for (int x = 0; x < xmax; x += xstep)
                    {
                        std::memcpy(dest + x, src, size);
                        src += size;
                    }
                }

                // next pass
                p += bw * adam.h;
            }
        }
    }

    void ParserPNG::process_i1to4(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;
        const int bits = m_bit_depth;

        const int maxValue = (1 << bits) - 1;
        const int scale = (255 / maxValue) * 0x010101;
        const u32 mask = (1 << bits) - 1;

        for (int y = 0; y < height; ++y)
        {
            u8* d = dest;
            dest += stride;
            ++src; // skip filter byte

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
                *d++ = value * scale;
                if (m_transparent_enable)
                {
                    *d++ = value == m_transparent_sample[0] ? 0 : 0xff;
                }
                offset -= bits;
            }
        }
    }

    void ParserPNG::process_i8(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;

        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    const u16 alpha = m_transparent_sample[0] == src[x] ? 0 : 0xff00;
                    d[x] = alpha | src[x];
                }
                src += width;
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                ++src; // skip filter byte
                std::memcpy(dest, src, width);
                src += width;
                dest += stride;
            }
        }
    }

    void ParserPNG::process_rgb8(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;

        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    ColorBGRA color(src[0], src[1], src[2], 0xff);
                    if (color == m_transparent_color)
                    {
                        color.a = 0;
                    }
                    *d++ = color;
                    src += 3;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    *d++ = ColorBGRA(src[0], src[1], src[2], 0xff);
                    src += 3;
                }
            }
        }
    }

    void ParserPNG::process_pal1to4(u8* dest, int stride, const u8* src, Palette* ptr_palette)
    {
        const int width = m_width;
        const int height = m_height;
        const int bits = m_bit_depth;

        const u32 mask = (1 << bits) - 1;

        if (ptr_palette)
        {
            *ptr_palette = m_palette;

            for (int y = 0; y < height; ++y)
            {
                u8* d = reinterpret_cast<u8*>(dest);
                dest += stride;
                ++src; // skip filter byte

                u32 data = 0;
                int offset = -1;

                for (int x = 0; x < width; ++x)
                {
                    if (offset < 0)
                    {
                        offset = 8 - bits;
                        data = *src++;
                    }
                    *d++ = (data >> offset) & mask;
                    offset -= bits;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                u32 data = 0;
                int offset = -1;

                for (int x = 0; x < width; ++x)
                {
                    if (offset < 0)
                    {
                        offset = 8 - bits;
                        data = *src++;
                    }
                    *d++ = m_palette[(data >> offset) & mask];
                    offset -= bits;
                }
            }
        }
    }

    void ParserPNG::process_pal8(u8* dest, int stride, const u8* src, Palette* ptr_palette)
    {
        const int width = m_width;
        const int height = m_height;

        if (ptr_palette)
        {
            *ptr_palette = m_palette;

            for (int y = 0; y < height; ++y)
            {
                ++src; // skip filter byte
                std::memcpy(dest, src, width);
                src += width;
                dest += stride;
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u32* d = reinterpret_cast<u32*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    *d++ = m_palette[*src++];
                }
            }
        }
    }

    void ParserPNG::process_ia8(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;

        for (int y = 0; y < height; ++y)
        {
            u16* d = reinterpret_cast<u16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                *d++ = (src[1] << 8) | src[0];
                src += 2;
            }
        }
    }

    void ParserPNG::process_rgba8(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;

        for (int y = 0; y < height; ++y)
        {
            u32* d = reinterpret_cast<u32*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                *d++ = ColorBGRA(src[0], src[1], src[2], src[3]);
                src += 4;
            }
        }
    }

    void ParserPNG::process_i16(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;

        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    u16 gray = (src[0] << 8) | src[1];
                    u16 alpha = m_transparent_sample[0] == src[0] ? 0 : 0xffff;
                    d[0] = gray;
                    d[1] = alpha;
                    d += 2;
                    src += 2;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    *d++ = (src[0] << 8) | src[1];
                    src += 2;
                }
            }
        }
    }

    void ParserPNG::process_rgb16(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;

        if (m_transparent_enable)
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    u16 red   = (src[0] << 8) | src[1];
                    u16 green = (src[2] << 8) | src[3];
                    u16 blue  = (src[4] << 8) | src[5];
                    u16 alpha = 0xffff;
                    if (m_transparent_sample[0] == red &&
                        m_transparent_sample[1] == green &&
                        m_transparent_sample[1] == blue) alpha = 0;
                    d[0] = red;
                    d[1] = green;
                    d[2] = blue;
                    d[3] = alpha;
                    d += 4;
                    src += 6;
                }
            }
        }
        else
        {
            for (int y = 0; y < height; ++y)
            {
                u16* d = reinterpret_cast<u16*>(dest);
                dest += stride;
                ++src; // skip filter byte

                for (int x = 0; x < width; ++x)
                {
                    u16 red   = (src[0] << 8) | src[1];
                    u16 green = (src[2] << 8) | src[3];
                    u16 blue  = (src[4] << 8) | src[5];
                    u16 alpha = 0xffff;
                    d[0] = red;
                    d[1] = green;
                    d[2] = blue;
                    d[3] = alpha;
                    d += 4;
                    src += 6;
                }
            }
        }
    }

    void ParserPNG::process_ia16(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;

        for (int y = 0; y < height; ++y)
        {
            u16* d = reinterpret_cast<u16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                u16 gray  = (src[0] << 8) | src[1];
                u16 alpha = (src[2] << 8) | src[3];
                d[0] = gray;
                d[1] = alpha;
                d += 2;
                src += 4;
            }
        }
    }

    void ParserPNG::process_rgba16(u8* dest, int stride, const u8* src)
    {
        const int width = m_width;
        const int height = m_height;

        for (int y = 0; y < height; ++y)
        {
            u16* d = reinterpret_cast<u16*>(dest);
            dest += stride;
            ++src; // skip filter byte

            for (int x = 0; x < width; ++x)
            {
                u16 red   = (src[0] << 8) | src[1];
                u16 green = (src[2] << 8) | src[3];
                u16 blue  = (src[4] << 8) | src[5];
                u16 alpha = (src[6] << 8) | src[7];
                d[0] = blue;
                d[1] = green;
                d[2] = red;
                d[3] = alpha;
                d += 4;
                src += 8;
            }
        }
    }

    void ParserPNG::process(u8* image, int stride, u8* buffer, Palette* ptr_palette)
    {
        u8* temp = nullptr;

        if (m_interlace)
        {
            const int stride = FILTER_BYTE + m_bytes_per_line;
            temp = new u8[m_height * stride];
            if (!temp)
            {
                setError("Memory allocation failed.");
                return;
            }

            std::memset(temp, 0, m_height * stride);

            // deinterlace does filter for each pass
            if (m_bit_depth < 8)
                deinterlace1to4(temp, stride, buffer);
            else
                deinterlace8to16(temp, stride, buffer);

            // use de-interlaced temp buffer as processing source
            buffer = temp;
        }
        else
        {
            // filter the whole image in single pass
            filter(buffer, m_bytes_per_line, m_height);
        }

        if (m_error)
        {
            delete [] temp;
            return;
        }

        if (m_color_type == COLOR_TYPE_I)
        {
            if (m_bit_depth < 8)
                process_i1to4(image, stride, buffer);
            else if (m_bit_depth == 8)
                process_i8(image, stride, buffer);
            else
                process_i16(image, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_RGB)
        {
            if (m_bit_depth == 8)
                process_rgb8(image, stride, buffer);
            else
                process_rgb16(image, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_PALETTE)
        {
            if (m_bit_depth < 8)
                process_pal1to4(image, stride, buffer, ptr_palette);
            else
                process_pal8(image, stride, buffer, ptr_palette);
        }
        else if (m_color_type == COLOR_TYPE_IA)
        {
            if (m_bit_depth == 8)
                process_ia8(image, stride, buffer);
            else
                process_ia16(image, stride, buffer);
        }
        else if (m_color_type == COLOR_TYPE_RGBA)
        {
            if (m_bit_depth == 8)
                process_rgba8(image, stride, buffer);
            else
                process_rgba16(image, stride, buffer);
        }

        delete [] temp;
    }

    const char* ParserPNG::decode(Surface& dest, Palette* ptr_palette)
    {
        if (!m_error)
        {
            parse();

            int buffer_size = 0;
            
            // compute output buffer size
            if (m_interlace)
            {
                // NOTE: brute-force loop to resolve memory consumption
                for (int pass = 0; pass < 7; ++pass)
                {
                    AdamInterleave adam(pass, m_width, m_height);
                    if (adam.w && adam.h)
                    {
                        const int bytesPerLine = FILTER_BYTE + m_channels * ((adam.w * m_bit_depth + 7) / 8);
                        buffer_size += bytesPerLine * adam.h;
                    }
                }
            }
            else
            {
                buffer_size = (FILTER_BYTE + m_bytes_per_line) * m_height;
            }


#ifdef DECODE_WITH_MINIZ
            // allocate output buffer
            print("  buffer bytes: %d\n", buffer_size);
            u8* buffer = new u8[buffer_size];
            if (!buffer)
            {
                setError("Memory allocation failed.");
                return m_error;
            }

            // decompress stream
            mz_stream stream;
            int status;
            memset(&stream, 0, sizeof(stream));

            stream.next_in   = m_compressed;
            stream.avail_in  = (unsigned int)m_compressed.size();
            stream.next_out  = buffer;
            stream.avail_out = (unsigned int)buffer_size;

            status = mz_inflateInit(&stream);
            if (status != MZ_OK)
            {
                // TODO: error
            }

            status = mz_inflate(&stream, MZ_FINISH);
            if (status != MZ_STREAM_END)
            {
                // TODO: error
            }

            print("  # total_out: %d \n", int(stream.total_out));
            status = mz_inflateEnd(&stream);

            // process image
            process(dest.image, dest.stride, buffer, ptr_palette);
            delete[] buffer;
#else
            Memory mem = m_compressed;
            int raw_len = buffer_size;
            u8 *buffer = (u8*) stbi_zlib_decode_malloc_guesssize_headerflag(
                reinterpret_cast<const char *>(mem.address),
                int(mem.size),
                raw_len,
                &raw_len,
                1);
            if (buffer)
            {
                print("  # total_out: %d \n", raw_len);

                // process image
                process(dest.image, dest.stride, buffer, ptr_palette);
                STBI_FREE(buffer);
            }
#endif
        }

        return m_error;
    }

    // ------------------------------------------------------------
    // writePNG()
    // ------------------------------------------------------------

    void writeChunk(Stream& stream, Memory memory)
    {
        BigEndianStream s(stream);

        const u32 chunk_size = static_cast<u32>(memory.size - 4);
        const u32 chunk_crc = crc32(0, memory);

        s.write32(chunk_size);
        s.write(memory);
        s.write32(chunk_crc);
    }

    void write_IHDR(Stream& stream, const Surface& surface, u8 color_bits, ColorType color_type)
    {
        Buffer buffer;
        BigEndianStream s(buffer);

        s.write32(make_u32rev('I', 'H', 'D', 'R'));

        s.write32(surface.width);
        s.write32(surface.height);
        s.write8(color_bits);
        s.write8(color_type);
        s.write8(0); // compression
        s.write8(0); // filter
        s.write8(0); // interlace

        writeChunk(stream, buffer);
    }

    void write_IDAT(Stream& stream, const Surface& surface)
    {
        const int bytesPerLine = surface.width * surface.format.bytes();
        const int bytes = (FILTER_BYTE + bytesPerLine) * surface.height;

        z_stream z = { 0 };
        deflateInit(&z, -1);

        z.avail_out = (unsigned int)deflateBound(&z, bytes);
        Buffer buffer(4 + z.avail_out);

        BigEndianStream s(buffer);
        s.write32(make_u32rev('I', 'D', 'A', 'T'));

        z.next_out = buffer + 4;

        for (int y = 0; y < surface.height; ++y)
        {
            bool last_scan = (y == surface.height - 1);

            // compress filler byte
            u8 zero = 0;
            z.avail_in = 1;
            z.next_in = &zero;
            deflate(&z, Z_NO_FLUSH);

            // compress scanline
            z.avail_in = bytesPerLine;
            z.next_in = surface.address<u8>(0, y);
            deflate(&z, last_scan ? Z_FINISH : Z_NO_FLUSH);
        }

        // compressed size includes chunkID
        const size_t compressed_size = int(z.next_out - buffer);

        deflateEnd(&z);

        // write chunkdID + compressed data
        writeChunk(stream, Memory(buffer, compressed_size));
    }

    void writePNG(Stream& stream, const Surface& surface, u8 color_bits, ColorType color_type)
    {
        static const u8 magic[] =
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
        };

        BigEndianStream s(stream);

        // write magic
        s.write(magic, 8);

        write_IHDR(stream, surface, color_bits, color_type);
        write_IDAT(stream, surface);

        // write IEND
        s.write32(0);
        s.write32(0x49454e44);
        s.write32(0xae426082);
    }

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    struct Interface : ImageDecoderInterface
    {
        ParserPNG m_parser;
        ImageHeader m_header;

        Interface(Memory memory)
            : m_parser(memory)
        {
            m_header = m_parser.header();

            const char* error = m_parser.getError();
            if (error)
            {
                print("HEADER ERROR: %s\n", error);
            }
        }

        ~Interface()
        {
        }

        ImageHeader header() override
        {
            return m_header;
        }

        void decode(Surface& dest, Palette* ptr_palette, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED_PARAMETER(level);
            MANGO_UNREFERENCED_PARAMETER(depth);
            MANGO_UNREFERENCED_PARAMETER(face);

            const char* error = nullptr;

            if (dest.format == m_header.format &&
                dest.width >= m_header.width &&
                dest.height >= m_header.height &&
                !ptr_palette)
            {
                // direct decoding
                error = m_parser.decode(dest, nullptr);
            }
            else
            {
                if (ptr_palette && m_header.palette)
                {
                    // direct decoding with palette
                    error = m_parser.decode(dest, ptr_palette);
                }
                else
                {
                    // indirect
                    Bitmap temp(m_header.width, m_header.height, m_header.format);
                    error = m_parser.decode(temp, nullptr);
                    dest.blit(0, 0, temp);
                }
            }

            if (error)
            {
                print("DECODE ERROR: %s\n", error);
            }
        }
    };

    ImageDecoderInterface* createInterface(Memory memory)
    {
        ImageDecoderInterface* x = new Interface(memory);
        return x;
    }

    // ------------------------------------------------------------
    // ImageEncoder
    // ------------------------------------------------------------

    void imageEncode(Stream& stream, const Surface& surface, float quality)
    {
        MANGO_UNREFERENCED_PARAMETER(quality);

        // defaults
        u8 color_bits = 8;
        ColorType color_type = COLOR_TYPE_RGBA;
        Format format;

        // select png format
        if (surface.format.luminance())
        {
            if (surface.format.alpha())
            {
                color_type = COLOR_TYPE_IA;

                if (surface.format.size[0] > 8)
                {
                    color_bits = 16;
                    format = Format(32, 0x0000ffff, 0xffff0000);
                }
                else
                {
                    color_bits = 8;
                    format = Format(16, 0x00ff, 0xff00);
                }
            }
            else
            {
                color_type = COLOR_TYPE_I;

                if (surface.format.size[0] > 8)
                {
                    color_bits = 16;
                    format = Format(16, 0xffff, 0x0000);
                }
                else
                {
                    color_bits = 8;
                    format = Format(8, 0xff, 0x00);
                }
            }
        }
        else
        {
            // always encode alpha in non-luminance formats
            color_type = COLOR_TYPE_RGBA;

            if (surface.format.size[0] > 8)
            {
                color_bits = 16;
                format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
            }
            else
            {
                color_bits = 8;
                format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            }
        }

        if (surface.format == format)
        {
            writePNG(stream, surface, color_bits, color_type);
        }
        else
        {
            Bitmap temp(surface.width, surface.height, format);
            temp.blit(0, 0, surface);
            writePNG(stream, temp, color_bits, color_type);
        }
    }

} // namespace

namespace mango
{

    void registerImageDecoderPNG()
    {
        registerImageDecoder(createInterface, ".png");
        registerImageEncoder(imageEncode, ".png");
    }

} // namespace mango
