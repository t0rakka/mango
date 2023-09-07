/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/compress.hpp>
#include <mango/math/math.hpp>
#include <mango/image/image.hpp>

namespace
{
    using namespace mango;
    using namespace mango::math;
    using namespace mango::image;

// --------------------------------------------------------------------------------------
//
// Copyright (c) Contributors to the OpenEXR Project. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, 
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, 
//    this list of conditions and the following disclaimer in the documentation and/or other 
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to 
//    endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// B44 uncompress
// --------------------------------------------------------------------------------------

static inline
void unpack14(const u8 b[14], u16 s[16])
{
    u16 shift = (b[2] >> 2);
    u16 bias  = u16(0x20u << shift);

    s[0] = ((u16) (b[0] << 8)) | ((u16) b[1]);
    s[4] = (u16) ((u32) s[0] + (u32) ((((u32) (b[2] << 4) | (u32) (b[3] >> 4)) & 0x3fu) << shift) - bias);
    s[8] = (u16) ((u32) s[4] + (u32) ((((u32) (b[3] << 2) | (u32) (b[4] >> 6)) & 0x3fu) << shift) - bias);
    s[12] = (u16) ((u32) s[8] + (u32) ((u32) (b[4] & 0x3fu) << shift) - bias);

    s[1] = (u16) ((u32) s[0] + (u32) ((u32) (b[5] >> 2) << shift) - bias);
    s[5] = (u16) ((u32) s[4] + (u32) ((((u32) (b[5] << 4) | (u32) (b[6] >> 4)) & 0x3fu) << shift) - bias);
    s[9] = (u16) ((u32) s[8] + (u32) ((((u32) (b[6] << 2) | (u32) (b[7] >> 6)) & 0x3fu) << shift) - bias);
    s[13] = (u16) ((u32) s[12] + (u32) ((u32) (b[7] & 0x3fu) << shift) - bias);

    s[2] = (u16) ((u32) s[1] + (u32) ((u32) (b[8] >> 2) << shift) - bias);
    s[6] = (u16) ((u32) s[5] + (u32) ((((u32) (b[8] << 4) | (u32) (b[9] >> 4)) & 0x3fu) << shift) - bias);
    s[10] = (u16) ((u32) s[9] + (u32) ((((u32) (b[9] << 2) | (u32) (b[10] >> 6)) & 0x3fu) << shift) - bias);
    s[14] = (u16) ((u32) s[13] + (u32) ((u32) (b[10] & 0x3fu) << shift) - bias);

    s[3]  = (u16) ((u32) s[2]  + (u32) ((u32) (b[11] >> 2) << shift) - bias);
    s[7]  = (u16) ((u32) s[6]  + (u32) ((((u32) (b[11] << 4) | (u32) (b[12] >> 4)) & 0x3fu) << shift) - bias);
    s[11] = (u16) ((u32) s[10] + (u32) ((((u32) (b[12] << 2) | (u32) (b[13] >> 6)) & 0x3fu) << shift) - bias);
    s[15] = (u16) ((u32) s[14] + (u32) ((u32) (b[13] & 0x3fu) << shift) - bias);

    for (int i = 0; i < 16; ++i)
    {
        if (s[i] & 0x8000)
            s[i] &= 0x7fff;
        else
            s[i] = ~s[i];
    }
}

static inline 
void unpack3(const u8 b[3], u16 s[16])
{
    s[0] = ((u16) (b[0] << 8)) | ((u16) b[1]);

    if (s[0] & 0x8000)
        s[0] &= 0x7fff;
    else
        s[0] = ~s[0];

    for (int i = 1; i < 16; ++i)
        s[i] = s[0];
}

// --------------------------------------------------------------------------------------
// PIZ uncompress
// --------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  16-bit Haar Wavelet encoding and decoding
//
//  The source code in this file is derived from the encoding
//  and decoding routines written by Christian Rouet for his
//  PIZ image file format.
//
//-----------------------------------------------------------------------------

//
// Wavelet basis functions without modulo arithmetic; they produce
// the best compression ratios when the wavelet-transformed data are
// Huffman-encoded, but the wavelet transform works only for 14-bit
// data (untransformed data values must be less than (1 << 14)).
//

inline
void wdec14(unsigned short l, unsigned short h, unsigned short &a, unsigned short &b)
{
    short ls = static_cast<short>(l);
    short hs = static_cast<short>(h);

    int hi = hs;
    int ai = ls + (hi & 1) + (hi >> 1);

    short as = static_cast<short>(ai);
    short bs = static_cast<short>(ai - hi);

    a = static_cast<unsigned short>(as);
    b = static_cast<unsigned short>(bs);
}

//
// Wavelet basis functions with modulo arithmetic; they work with full
// 16-bit data, but Huffman-encoding the wavelet-transformed data doesn't
// compress the data quite as well.
//

const int NBITS = 16;
const int A_OFFSET = 1 << (NBITS - 1);
const int MOD_MASK = (1 << NBITS) - 1;

inline
void wdec16(unsigned short l, unsigned short h, unsigned short &a, unsigned short &b)
{
    int m = l;
    int d = h;
    int bb = (m - (d >> 1)) & MOD_MASK;
    int aa = (d + bb - A_OFFSET) & MOD_MASK;
    b = static_cast<unsigned short>(bb);
    a = static_cast<unsigned short>(aa);
}

//
// 2D Wavelet decoding:
//

static
void wav2Decode(
    unsigned short *in,  // io: values are transformed in place
    int nx,              // i : x size
    int ox,              // i : x offset
    int ny,              // i : y size
    int oy,              // i : y offset
    unsigned short mx)   // i : maximum in[x][y] value
{
  bool w14 = (mx < (1 << 14));
  int n = (nx > ny) ? ny : nx;
  int p = 1;
  int p2;

  //
  // Search max level
  //

  while (p <= n)
    p <<= 1;

  p >>= 1;
  p2 = p;
  p >>= 1;

  //
  // Hierarchical loop on smaller dimension n
  //

  while (p >= 1) {
    unsigned short *py = in;
    unsigned short *ey = in + oy * (ny - p2);
    int oy1 = oy * p;
    int oy2 = oy * p2;
    int ox1 = ox * p;
    int ox2 = ox * p2;
    unsigned short i00, i01, i10, i11;

    //
    // Y loop
    //

    for (; py <= ey; py += oy2) {
      unsigned short *px = py;
      unsigned short *ex = py + ox * (nx - p2);

      //
      // X loop
      //

      for (; px <= ex; px += ox2) {
        unsigned short *p01 = px + ox1;
        unsigned short *p10 = px + oy1;
        unsigned short *p11 = p10 + ox1;

        //
        // 2D wavelet decoding
        //

        if (w14) {
          wdec14(*px, *p10, i00, i10);
          wdec14(*p01, *p11, i01, i11);
          wdec14(i00, i01, *px, *p01);
          wdec14(i10, i11, *p10, *p11);
        } else {
          wdec16(*px, *p10, i00, i10);
          wdec16(*p01, *p11, i01, i11);
          wdec16(i00, i01, *px, *p01);
          wdec16(i10, i11, *p10, *p11);
        }
      }

      //
      // Decode (1D) odd column (still in Y loop)
      //

      if (nx & p) {
        unsigned short *p10 = px + oy1;

        if (w14)
          wdec14(*px, *p10, i00, *p10);
        else
          wdec16(*px, *p10, i00, *p10);

        *px = i00;
      }
    }

    //
    // Decode (1D) odd line (must loop in X)
    //

    if (ny & p) {
      unsigned short *px = py;
      unsigned short *ex = py + ox * (nx - p2);

      for (; px <= ex; px += ox2) {
        unsigned short *p01 = px + ox1;

        if (w14)
          wdec14(*px, *p01, i00, *p01);
        else
          wdec16(*px, *p01, i00, *p01);

        *px = i00;
      }
    }

    //
    // Next level
    //

    p2 = p;
    p >>= 1;
  }
}

//-----------------------------------------------------------------------------
//
//  16-bit Huffman compression and decompression.
//
//  The source code in this file is derived from the 8-bit
//  Huffman compression and decompression routines written
//  by Christian Rouet for his PIZ image file format.
//
//-----------------------------------------------------------------------------

// Adds some modification for tinyexr.

const int HUF_ENCBITS = 16;  // literal (value) bit length
const int HUF_DECBITS = 14;  // decoding bit size (>= 8)

const int HUF_ENCSIZE = (1 << HUF_ENCBITS) + 1;  // encoding table size
const int HUF_DECSIZE = 1 << HUF_DECBITS;        // decoding table size
const int HUF_DECMASK = HUF_DECSIZE - 1;

struct HufDec
{  
    // short code    long code
    //-------------------------------
    int len : 8;   // code length    0
    int lit : 24;  // lit      p size
    int *p;        // 0      lits
};

inline u64 hufLength(u64 code) { return code & 63; }
inline u64 hufCode(u64 code) { return code >> 6; }

inline u64 getBits(int nBits, u64 &c, int &lc, const u8 *&in)
{
    while (lc < nBits)
    {
        c = (c << 8) | *(in++);
        lc += 8;
    }

    lc -= nBits;
    return (c >> lc) & ((1 << nBits) - 1);
}

//
// ENCODING TABLE BUILDING & (UN)PACKING
//

//
// Build a "canonical" Huffman code table:
//  - for each (uncompressed) symbol, hcode contains the length
//    of the corresponding code (in the compressed data)
//  - canonical codes are computed and stored in hcode
//  - the rules for constructing canonical codes are as follows:
//    * shorter codes (if filled with zeroes to the right)
//      have a numerically higher value than longer codes
//    * for codes with the same length, numerical values
//      increase with numerical symbol values
//  - because the canonical code table can be constructed from
//    symbol lengths alone, the code table can be transmitted
//    without sending the actual code values
//  - see http://www.compressconsult.com/huffman/
//

static
void hufCanonicalCodeTable(u64 hcode[HUF_ENCSIZE])
{
    u64 n[59];

    //
    // For each i from 0 through 58, count the
    // number of different codes of length i, and
    // store the count in n[i].
    //

    for (int i = 0; i <= 58; ++i) n[i] = 0;

    for (int i = 0; i < HUF_ENCSIZE; ++i) n[hcode[i]] += 1;

    //
    // For each i from 58 through 1, compute the
    // numerically lowest code with length i, and
    // store that code in n[i].
    //

    u64 c = 0;

    for (int i = 58; i > 0; --i)
    {
        u64 nc = ((c + n[i]) >> 1);
        n[i] = c;
        c = nc;
    }

    //
    // hcode[i] contains the length, l, of the
    // code for symbol i.  Assign the next available
    // code of length l to the symbol and store both
    // l and the code in hcode[i].
    //

    for (int i = 0; i < HUF_ENCSIZE; ++i)
    {
        int l = static_cast<int>(hcode[i]);

        if (l > 0) hcode[i] = l | (n[l]++ << 6);
    }
}

//
// Pack an encoding table:
//  - only code lengths, not actual codes, are stored
//  - runs of zeroes are compressed as follows:
//
//    unpacked    packed
//    --------------------------------
//    1 zero    0  (6 bits)
//    2 zeroes    59
//    3 zeroes    60
//    4 zeroes    61
//    5 zeroes    62
//    n zeroes (6 or more)  63 n-6  (6 + 8 bits)
//

const int SHORT_ZEROCODE_RUN = 59;
const int LONG_ZEROCODE_RUN = 63;
const int SHORTEST_LONG_RUN = 2 + LONG_ZEROCODE_RUN - SHORT_ZEROCODE_RUN;

//
// Unpack an encoding table packed by hufPackEncTable():
//

static bool hufUnpackEncTable(
    const u8 **pcode,  // io: ptr to packed table (updated)
    int ni,              // i : input size (in bytes)
    int im,              // i : min hcode index
    int iM,              // i : max hcode index
    u64 *hcode)    //  o: encoding table [HUF_ENCSIZE]
{
  memset(hcode, 0, sizeof(u64) * HUF_ENCSIZE);

  const u8 *p = *pcode;
  u64 c = 0;
  int lc = 0;

  for (; im <= iM; im++) {
    if (p - *pcode > ni) {
      return false;
    }

    u64 l = hcode[im] = getBits(6, c, lc, p);  // code length

    if (l == (u64)LONG_ZEROCODE_RUN) {
      if (p - *pcode > ni) {
        return false;
      }

      int zerun = getBits(8, c, lc, p) + SHORTEST_LONG_RUN;

      if (im + zerun > iM + 1) {
        return false;
      }

      while (zerun--) hcode[im++] = 0;

      im--;
    } else if (l >= (u64)SHORT_ZEROCODE_RUN) {
      int zerun = l - SHORT_ZEROCODE_RUN + 2;

      if (im + zerun > iM + 1) {
        return false;
      }

      while (zerun--) hcode[im++] = 0;

      im--;
    }
  }

  *pcode = const_cast<u8 *>(p);

  hufCanonicalCodeTable(hcode);

  return true;
}

//
// DECODING TABLE BUILDING
//

//
// Clear a newly allocated decoding table so that it contains only zeroes.
//

static
void hufClearDecTable(HufDec *hdecod)
{
    for (int i = 0; i < HUF_DECSIZE; i++)
    {
        hdecod[i].len = 0;
        hdecod[i].lit = 0;
        hdecod[i].p = nullptr;
    }
}

//
// Build a decoding hash table based on the encoding table hcode:
//  - short codes (<= HUF_DECBITS) are resolved with a single table access;
//  - long code entry allocations are not optimized, because long codes are
//    unfrequent;
//

static
bool hufBuildDecTable(const u64 *hcode,  // i : encoding table
                      int im,                  // i : min index in hcode
                      int iM,                  // i : max index in hcode
                      HufDec *hdecod)  //  o: (allocated by caller)
{
  //
  // Init hashtable & loop on all codes.
  // Assumes that hufClearDecTable(hdecod) has already been called.
  //

  for (; im <= iM; im++) {
    u64 c = hufCode(hcode[im]);
    int l = hufLength(hcode[im]);

    if (c >> l) {
      //
      // Error: c is supposed to be an l-bit code,
      // but c contains a value that is greater
      // than the largest l-bit number.
      //

      // invalidTableEntry();
      return false;
    }

    if (l > HUF_DECBITS) {
      //
      // Long code: add a secondary entry
      //

      HufDec *pl = hdecod + (c >> (l - HUF_DECBITS));

      if (pl->len) {
        //
        // Error: a short code has already
        // been stored in table entry *pl.
        //

        // invalidTableEntry();
        return false;
      }

      pl->lit++;

      if (pl->p) {
        int *p = pl->p;
        pl->p = new int[pl->lit]; // xxx

        for (int i = 0; i < pl->lit - 1; ++i) pl->p[i] = p[i];

        delete[] p;
      } else {
        pl->p = new int[1]; // xxx
      }

      pl->p[pl->lit - 1] = im;
    } else if (l) {
      //
      // Short code: init all primary entries
      //

      HufDec *pl = hdecod + (c << (HUF_DECBITS - l));

      for (u64 i = 1ULL << (HUF_DECBITS - l); i > 0; i--, pl++) {
        if (pl->len || pl->p) {
          //
          // Error: a short code or a long code has
          // already been stored in table entry *pl.
          //

          // invalidTableEntry();
          return false;
        }

        pl->len = l;
        pl->lit = im;
      }
    }
  }

  return true;
}

//
// Free the long code entries of a decoding table built by hufBuildDecTable()
//

static
void hufFreeDecTable(HufDec *hdecod)
{
    for (int i = 0; i < HUF_DECSIZE; i++)
    {
        if (hdecod[i].p)
        {
            delete[] hdecod[i].p;
            hdecod[i].p = nullptr;
        }
    }
}

//
// DECODING
//

//
// In order to force the compiler to inline them,
// getChar() and getCode() are implemented as macros
// instead of "inline" functions.
//

#define getChar(c, lc, in) \
{ \
    c = (c << 8) | *(u8 *)(in++); \
    lc += 8; \
}

#define getCode(po, rlc, c, lc, in, out, ob, oe) \
{ \
    if (po == rlc) \
    { \
        if (lc < 8) \
            getChar(c, lc, in); \
        \
        lc -= 8; \
        \
        u8 cs = (c >> lc); \
        \
        if (out + cs > oe) \
            ; \
        else if (out - 1 < ob) \
            ; \
        \
        u16 s = out[-1]; \
        \
        while (cs-- > 0) \
            *out++ = s; \
    } \
    else if (out < oe) \
    { \
        *out++ = po; \
    } \
    else \
    { \
    } \
}

//
// Decode (uncompress) ni bits based on encoding & decoding tables:
//

void hufDecode(const u64*  hcode, // i : encoding table
     const HufDec* hdecod, // i : decoding table
     const u8* in, // i : compressed input buffer
     int ni, // i : input size (in bits)
     int rlc, // i : run-length code
     size_t no, // i : expected output size (in bytes)
     u16* out)	//  o: uncompressed output buffer
{
    uint64_t c = 0;
    int lc = 0;
    u16* outb = out;
    u16* oe = out + no;
    const u8* ie = in + (ni + 7) / 8; // input byte size

    //
    // Loop on input bytes
    //

    while (in < ie)
    {
        getChar (c, lc, in);

        //
        // Access decoding table
        //

        while (lc >= HUF_DECBITS)
        {
            const HufDec pl = hdecod[(c >> (lc-HUF_DECBITS)) & HUF_DECMASK];

            if (pl.len)
            {
                //
                // Get short code
                //

                lc -= pl.len;

                if ( lc < 0 )
                {
                    //invalidCode(); // code length too long
                }
                getCode (pl.lit, rlc, c, lc, in, out, outb, oe);
            }
            else
            {
                if (!pl.p)
                {
                    //invalidCode(); // wrong code
                }

                //
                // Search long code
                //

                int j;

                for (j = 0; j < pl.lit; j++)
                {
                    int	l = hufLength (hcode[pl.p[j]]);

                    while (lc < l && in < ie)	// get more bits
                        getChar (c, lc, in);

                    if (lc >= l)
                    {
                        if (hufCode (hcode[pl.p[j]]) == ((c >> (lc - l)) & ((u64(1) << l) - 1)))
                        {
                            //
                            // Found : get long code
                            //

                            lc -= l;
                            getCode (pl.p[j], rlc, c, lc, in, out, outb, oe);
                            break;
                        }
                    }
                }

                if (j == pl.lit)
                {
                    //invalidCode(); // Not found
                }
            }
        }
    }

    //
    // Get remaining (short) codes
    //

    int i = (8 - ni) & 7;
    c >>= i;
    lc -= i;

    while (lc > 0)
    {
        const HufDec pl = hdecod[(c << (HUF_DECBITS - lc)) & HUF_DECMASK];

        if (pl.len)
        {
            lc -= pl.len;
            if ( lc < 0 )
            {
                //invalidCode(); // code length too long
            }
            getCode (pl.lit, rlc, c, lc, in, out, outb, oe);
        }
        else
        {
            //invalidCode(); // wrong (long) code
        }
    }

    if (out - outb != no)
    {
        //notEnoughData ();
    }
}

//
// EXTERNAL INTERFACE
//

static
bool hufUncompress(const u8* compressed, int nCompressed, std::vector<u16>& output)
{
    if (nCompressed == 0)
    {
        return false;
    }

    // read header (20 bytes)

    int im = mango::uload32le(compressed + 0);
    int iM = mango::uload32le(compressed + 4);
    int nBits = mango::uload32le(compressed + 12);

    if (im < 0 || im >= HUF_ENCSIZE || iM < 0 || iM >= HUF_ENCSIZE)
    {
        return false;
    }

    constexpr int headerSize = 20;

    compressed += headerSize;
    nCompressed -= headerSize;

    std::vector<u64> freq(HUF_ENCSIZE);
    std::vector<HufDec> hdec(HUF_DECSIZE);

    hufClearDecTable(hdec.data());
    hufUnpackEncTable(&compressed, nCompressed, im, iM, freq.data());

    if (nBits > 8 * nCompressed)
    {
        return false;
    }

    hufBuildDecTable(freq.data(), im, iM, hdec.data());
    hufDecode(freq.data(), hdec.data(), compressed, nBits, iM, output.size(), output.data());

    hufFreeDecTable(hdec.data());

    return true;
}

//
// Functions to compress the range of values in the pixel data
//

const int USHORT_RANGE = (1 << 16);
const int BITMAP_SIZE = (USHORT_RANGE >> 3);

static
u16 reverseLutFromBitmap(const u8 bitmap[BITMAP_SIZE], u16 lut[USHORT_RANGE])
{
    int k = 0;

    lut[k++] = 0;
    if (bitmap[0] & (1 << 1)) lut[k++] = 1;
    if (bitmap[0] & (1 << 2)) lut[k++] = 2;
    if (bitmap[0] & (1 << 3)) lut[k++] = 3;
    if (bitmap[0] & (1 << 4)) lut[k++] = 4;
    if (bitmap[0] & (1 << 5)) lut[k++] = 5;
    if (bitmap[0] & (1 << 6)) lut[k++] = 6;
    if (bitmap[0] & (1 << 7)) lut[k++] = 7;

    for (int i = 8; i < USHORT_RANGE; i += 8)
    {
        u32 value = bitmap[i >> 3];
        if (value & (1 << 0)) lut[k++] = i + 0;
        if (value & (1 << 1)) lut[k++] = i + 1;
        if (value & (1 << 2)) lut[k++] = i + 2;
        if (value & (1 << 3)) lut[k++] = i + 3;
        if (value & (1 << 4)) lut[k++] = i + 4;
        if (value & (1 << 5)) lut[k++] = i + 5;
        if (value & (1 << 6)) lut[k++] = i + 6;
        if (value & (1 << 7)) lut[k++] = i + 7;
    }

    int n = k - 1;

    for ( ; k < USHORT_RANGE; ++k)
    {
        lut[k] = 0;
    }

    return n; // maximum k where lut[k] is non-zero
}

static
void applyLut(const u16 lut[USHORT_RANGE], u16 data[], size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = lut[data[i]];
    }
}

static
void predictor(u8* data, size_t count)
{
    u8 delta = data[0];

    for (size_t i = 1; i < count; ++i)
    {
        delta += (data[i] - 128);
        data[i] = delta;
    }
}

static
void deinterleave(u8* dest, const u8* source, size_t size)
{
    const size_t count = size / 2;
    const u8* temp0 = source;
    const u8* temp1 = source + ((size + 1) / 2);

    for (size_t i = 0; i < count; ++i)
    {
        dest[i * 2 + 0] = temp0[i];
        dest[i * 2 + 1] = temp1[i];
    }
}

static inline
bool isfinite(float16 f)
{
    constexpr u16 exponent_mask = 0x7c00;
    constexpr u16 mantissa_mask = 0x03ff;

    u16 e = f.u & exponent_mask;
    if (e == exponent_mask)
    {
        return false;
    }
    else if (e == 0)
    {
        return (f.u & mantissa_mask) != 0;
    }

    return true;
}

// --------------------------------------------------------------------------------------
// Context
// --------------------------------------------------------------------------------------

enum Compression : u8
{
    NO_COMPRESSION    = 0,
    RLE_COMPRESSION   = 1,
    ZIPS_COMPRESSION  = 2,
    ZIP_COMPRESSION   = 3,
    PIZ_COMPRESSION   = 4,
    PXR24_COMPRESSION = 5,
    B44_COMPRESSION   = 6,
    B44A_COMPRESSION  = 7,
    DWAA_COMPRESSION  = 8,
    DWAB_COMPRESSION  = 9,
};

enum LineOrder : u8
{
    INCREASING_Y = 0,
    DECREASING_Y = 1,
    RANDOM_Y = 2
};

enum PixelType : u32
{
    UINT = 0,
    HALF = 1,
    FLOAT = 2
};

enum class ColorType
{
    LUMINANCE,
    CHROMA,
    RGB,
    NONE
};

struct Channel
{
    PixelType type;
    int xsamples;
    int ysamples;
    int linear;

    int bytes;
    int component;
    int offset;
};

struct ChannelList
{
    std::vector<Channel> channels;
    ColorType colortype = ColorType::NONE;
    int bytes = 0;
};

struct TileDesc
{
    u32 xsize;
    u32 ysize;
    u8 mode; // 0: single image, 1: mipmap, 2: ripmap
             // 4th bit (0x10) indicates rounding direction (0: down, 1: up)
};

struct Box2i
{
    s32 xmin, ymin, xmax, ymax;
};

struct Text
{
    int temp;
};

struct String
{
    int temp;
};

struct Chromaticities
{
    float32x2 red;
    float32x2 green;
    float32x2 blue;
    float32x2 white;
};

static
Chromaticities Chromaticities_BT709()
{
    Chromaticities chromaticities;

    // Rec. ITU-R BT.709-3
    chromaticities.red   = float32x2(0.6400f, 0.3300f);
    chromaticities.green = float32x2(0.3000f, 0.6000f);
    chromaticities.blue  = float32x2(0.1500f, 0.0600f);
    chromaticities.white = float32x2(0.3127f, 0.3290f);

    return chromaticities;
}

static
Matrix4x4 RGBtoXYZ(const Chromaticities& chromaticities, float Y)
{
    // For an explanation of how the color conversion matrix is derived,
    // see Roy Hall, "Illumination and Color in Computer Generated Imagery",
    // Springer-Verlag, 1989, chapter 3, "Perceptual Response"; and 
    // Charles A. Poynton, "A Technical Introduction to Digital Video",
    // John Wiley & Sons, 1996, chapter 7, "Color science for video".

    // X and Z values of RGB value (1, 1, 1), or "white"

    float32x2 red   = chromaticities.red;
    float32x2 green = chromaticities.green;
    float32x2 blue  = chromaticities.blue;
    float32x2 white = chromaticities.white;

    float X = white.x * Y / white.y;
    float Z = (1.0f - white.x - white.y) * Y / white.y;

    // Scale factors for matrix rows, compute numerators and common denominator

    float d = red.x * (blue.y  - green.y) +
             blue.x * (green.y - red.y) +
            green.x * (red.y   - blue.y);

    float SrN = (X * (blue.y - green.y) -
        green.x * (Y * (blue.y - 1) +
        blue.y  * (X + Z)) +
        blue.x  * (Y * (green.y - 1) +
        green.y * (X + Z)));


    float SgN = (X * (red.y - blue.y) +
        red.x   * (Y * (blue.y - 1) +
        blue.y  * (X + Z)) -
        blue.x  * (Y * (red.y - 1) +
        red.y   * (X + Z)));

    float SbN = (X * (green.y - red.y) -
        red.x   * (Y * (green.y - 1) +
        green.y * (X + Z)) +
        green.x * (Y * (red.y - 1) +
        red.y   * (X + Z)));

    float Sr = SrN / d;
    float Sg = SgN / d;
    float Sb = SbN / d;

    // Assemble the matrix

    Matrix4x4 M;

    M[0][0] = Sr * red.x;
    M[0][1] = Sr * red.y;
    M[0][2] = Sr * (1.0f - red.x - red.y);

    M[1][0] = Sg * green.x;
    M[1][1] = Sg * green.y;
    M[1][2] = Sg * (1.0f - green.x - green.y);

    M[2][0] = Sb * blue.x;
    M[2][1] = Sb * blue.y;
    M[2][2] = Sb * (1.0f - blue.x - blue.y);

    return M;
}

static
float32x3 computeYW(const Chromaticities& chromaticities, float Y = 1.0f)
{
    Matrix4x4 m = RGBtoXYZ(chromaticities, Y);
    return float32x3(m[0][1], m[1][1], m[2][1]) / (m[0][1] + m[1][1] + m[2][1]);
}

template <typename T>
struct Attribute
{
    T data;
};

template <typename T>
void readAttribute(T& data, LittleEndianConstPointer p)
{
    MANGO_UNREFERENCED(data);
    MANGO_UNREFERENCED(p);
}

template <>
void readAttribute<ChannelList>(ChannelList& data, LittleEndianConstPointer p)
{
    for ( ; *p; )
    {
        std::string name = p.cast<const char>();
        p += name.length() + 1;

        u32 type = p.read32();
        u8 linear = p.read8();
        p += 3; // reserved
        u32 xsamples = p.read32();
        u32 ysamples = p.read32();

        Channel channel;

        channel.xsamples = xsamples;
        channel.ysamples = ysamples;
        channel.linear = linear;
        //debugPrint("  %s xs: %d  ys: %d  linear: %d\n", name.c_str(), xsamples, ysamples, linear);

        switch (type)
        {
            case 0:
                channel.type = UINT;
                channel.bytes = 4;
                break;
            case 1:
                channel.type = HALF;
                channel.bytes = 2;
                break;
            case 2:
                channel.type = FLOAT;
                channel.bytes = 4;
                break;
            default:
                // TODO: error
                channel.bytes = 0;
                break;
        }

        if (name == "R")
        {
            channel.component = 0;
            channel.offset = data.bytes;
            data.channels.push_back(channel);
            data.colortype = ColorType::RGB;
        }
        else if (name == "G")
        {
            channel.component = 1;
            channel.offset = data.bytes;
            data.channels.push_back(channel);
            data.colortype = ColorType::RGB;
        }
        else if (name == "B")
        {
            channel.component = 2;
            channel.offset = data.bytes;
            data.channels.push_back(channel);
            data.colortype = ColorType::RGB;
        }
        else if (name == "A")
        {
            channel.component = 3;
            channel.offset = data.bytes;
            data.channels.push_back(channel);
        }
        else if (name == "BY")
        {
            channel.component = 0;
            channel.offset = data.bytes;
            data.channels.push_back(channel);
            data.colortype = ColorType::CHROMA;
        }
        else if (name == "RY")
        {
            channel.component = 1;
            channel.offset = data.bytes;
            data.channels.push_back(channel);
            data.colortype = ColorType::CHROMA;
        }
        else if (name == "Y")
        {
            channel.component = 2;
            channel.offset = data.bytes;
            data.channels.push_back(channel);

            if (data.colortype != ColorType::CHROMA)
            {
                data.colortype = ColorType::LUMINANCE;
            }
        }
        else
        {
            // not supported
            channel.component = -1;
            channel.offset = data.bytes;
            data.channels.push_back(channel);
        }

        data.bytes += channel.bytes;

        debugPrint("    \"%s\", type: %d, linear: %d, offset: %d, samples: (%d x %d)\n",
            name.c_str(), type, linear, channel.offset, xsamples, ysamples);
    }

    const char* color = "NONE";
    switch (data.colortype)
    {
        case ColorType::LUMINANCE:
            color = "LUMINANCE";
            break;
        case ColorType::CHROMA:
            color = "CHROMA";
            break;
        case ColorType::RGB:
            color = "RGB";
            break;
        default:
            break;
    }

    debugPrint("    Color: %s\n", color);
}

template <>
void readAttribute<TileDesc>(TileDesc& data, LittleEndianConstPointer p)
{
    data.xsize = p.read32();
    data.ysize = p.read32();
    data.mode = p.read8();
    debugPrint("    %d x %d mode: %x\n", data.xsize, data.ysize, data.mode);
}

/*
template <>
void readAttribute<String>(String& data, LittleEndianConstPointer p)
{
    // TODO
    MANGO_UNREFERENCED(data);
    MANGO_UNREFERENCED(p);
}

template <>
void readAttribute<Text>(Text& data, LittleEndianConstPointer p)
{
    // TODO
    MANGO_UNREFERENCED(data);
    MANGO_UNREFERENCED(p);
}
*/

template <>
void readAttribute<u8>(u8& data, LittleEndianConstPointer p)
{
    data = p.read8();
    debugPrint("    %d\n", data);
}

/*
template <>
void readAttribute<u32>(u32& data, LittleEndianConstPointer p)
{
    data = p.read32();
    debugPrint("    %d\n", data);
}
*/

template <>
void readAttribute<float>(float& data, LittleEndianConstPointer p)
{
    data = p.read32f();
    debugPrint("    %f\n", data);
}

template <>
void readAttribute<float32x2>(float32x2& data, LittleEndianConstPointer p)
{
    data.x = p.read32f();
    data.y = p.read32f();
    debugPrint("    %f, %f\n", data.x, data.y);
}

template <>
void readAttribute<Box2i>(Box2i& data, LittleEndianConstPointer p)
{
    data.xmin = p.read32();
    data.ymin = p.read32();
    data.xmax = p.read32();
    data.ymax = p.read32();
    debugPrint("    %d, %d, %d, %d\n", data.xmin, data.ymin, data.xmax, data.ymax);
}

template <>
void readAttribute<Chromaticities>(Chromaticities& data, LittleEndianConstPointer p)
{
    data.red.x   = p.read32f();
    data.red.y   = p.read32f();
    data.green.x = p.read32f();
    data.green.y = p.read32f();
    data.blue.x  = p.read32f();
    data.blue.y  = p.read32f();
    data.white.x = p.read32f();
    data.white.y = p.read32f();
    debugPrint("    red:   %f, %f\n", float(data.red.x), float(data.red.y));
    debugPrint("    green: %f, %f\n", float(data.green.x), float(data.green.y));
    debugPrint("    blue:  %f, %f\n", float(data.blue.x), float(data.blue.y));
    debugPrint("    white: %f, %f\n", float(data.white.x), float(data.white.y));
}

struct AttributeTable
{
    // Header Attributes
    ChannelList chlist;
    u8          compression;
    Box2i       dataWindow;
    Box2i       displayWindow;
    u8          lineOrder; // 0: increasing, 1: decreasing
    float       pixelAspectRatio;
    float32x2   screenWindowCenter;
    float       screenWindowWidth;
    u8          envmap; // 0: None, 1: LatLong, 2 - Cube
    Chromaticities chromaticities;

    // Tile Header Attribute
    TileDesc    tiles;

    // Multi-View Header Attribute
    Text        view;

    // Multi-Part Data Header Attributes
    String      name;
    String      type;
    u32         version;
    u32         chunkCount;

    // Deep Data Header Attributes
    u32         maxSamplesPerPixel;

    AttributeTable()
    {
        envmap = 0;
        chromaticities = Chromaticities_BT709();
    }

    void parse(const std::string& name, const std::string& type, LittleEndianConstPointer p, u32 size)
    {
        if (name == "channels")
        {
            readAttribute(chlist, p);
        }
        else if (name == "tiles")
        {
            readAttribute(tiles, p);
        }
        else if (name == "compression")
        {
            readAttribute(compression, p);
        }
        else if (name == "dataWindow")
        {
            readAttribute(dataWindow, p);
        }
        else if (name == "displayWindow")
        {
            readAttribute(displayWindow, p);
        }
        else if (name == "lineOrder")
        {
            readAttribute(lineOrder, p);
        }
        else if (name == "pixelAspectRatio")
        {
            readAttribute(pixelAspectRatio, p);
        }
        else if (name == "screenWindowCenter")
        {
            readAttribute(screenWindowCenter, p);
        }
        else if (name == "screenWindowWidth")
        {
            readAttribute(screenWindowWidth, p);
        }
        else if (name == "envmap")
        {
            readAttribute(envmap, p);
            ++envmap;
        }
        else if (name == "chromaticities")
        {
            readAttribute(chromaticities, p);
        }
        else
        {
            debugPrint("    Unknown attribute.\n");
        }
    }
};

struct ContextEXR
{
    ConstMemory m_memory;
    ImageHeader m_header;

    const u8* m_pointer = nullptr;

    bool is_single_tile;
    bool is_long_name;
    bool is_deep_format;
    bool is_multi_part;

    int m_scanLinesPerBlock = 0;

    AttributeTable m_attributes;

    u16 m_log_table[0x10000];

    u64 m_time_decompress = 0;
    u64 m_time_blit = 0;
    u64 m_time_decode = 0;

    ContextEXR(ConstMemory memory);
    ~ContextEXR();

    const u8* decompress_none(Memory dest, ConstMemory source);
    const u8* decompress_rle(Memory dest, ConstMemory source);
    const u8* decompress_zip(Memory dest, ConstMemory source);
    const u8* decompress_piz(Memory dest, ConstMemory source, int width, int height);
    const u8* decompress_pxr24(Memory dest, ConstMemory source, int width, int height, int ystart);
    const u8* decompress_b44(Memory dest, ConstMemory source, int width, int height, int ystart);
    const u8* decompress_dwaa(Memory dest, ConstMemory source, int width, int height, int ystart);
    const u8* decompress_dwab(Memory dest, ConstMemory source, int width, int height, int ystart);

    void decodeBlock(Surface surface, ConstMemory memory, int x0, int y0, int x1, int y1);
    ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face);

    void initLogTable()
    {
        for (u32 i = 0; i < 0x10000; ++i)
        {
            float16 hf;
            hf.u = i;

            u16 value = 0;

            if (isfinite(hf))
            {
                float x = hf;
                x = 8.0f * std::log(x);
                if (std::isfinite(x))
                {
                    hf = x;
                    value = hf.u;
                }
            }

            m_log_table[i] = value;
        }
    }

    void convertToLinear(u16 s[16]) const
    {
        for (int i = 0; i < 16; ++i)
        {
            s[i] = m_log_table[s[i]];
        }
    }

    void report()
    {
        debugPrint("  decoding: %d ms\n", int(m_time_decompress / 1000));
        debugPrint("  blitting: %d ms\n", int(m_time_blit / 1000));
        debugPrint("  decode: %d ms\n", int(m_time_decode / 1000));
    }
};

ContextEXR::ContextEXR(ConstMemory memory)
    : m_memory(memory)
{
    debugPrint("Memory: %d KB\n", int(memory.size / 1024));

    LittleEndianConstPointer p = memory.address;

    u32 magic = p.read32();
    if (magic != 0x01312f76)
    {
        m_header.setError("Incorrect format identifier: 0x%.8x", magic);
        return;
    }

    u32 flags = p.read32();
    u32 version = flags & 0xff;

    is_single_tile = (flags & 0x0200) != 0;
    is_long_name   = (flags & 0x0400) != 0;
    is_deep_format = (flags & 0x0800) != 0;
    is_multi_part  = (flags & 0x1000) != 0;

    debugPrint("[Header]\n");
    debugPrint("  version: %d\n", version);
    debugPrint("  single_tile: %d\n"
               "  long_name: %d\n"
               "  deep_format: %d\n"
               "  multi-part: %d\n",
        is_single_tile, is_long_name, is_deep_format, is_multi_part);

    if (is_multi_part)
    {
        m_header.setError("Multi-part files are not supported.");
        return;
    }

    if (is_deep_format)
    {
        m_header.setError("Deep format files are not supported.");
        return;
    }

    debugPrint("\n");
    debugPrint("[Attributes]\n");

    for ( ; *p; )
    {
        std::string name = p.cast<const char>();
        p += name.length() + 1;

        std::string type = p.cast<const char>();
        p += type.length() + 1;

        u32 size = p.read32();
        debugPrint("  \"%s\", type: %s, size: %d\n", name.c_str(), type.c_str(), size);

        m_attributes.parse(name, type, p, size);
        p += size;
    }

    ++p; // skip terminator

    switch (m_attributes.compression)
    {
        case NO_COMPRESSION:
            m_scanLinesPerBlock = 1;
            break;

        case RLE_COMPRESSION:
            m_scanLinesPerBlock = 1;
            break;

        case ZIPS_COMPRESSION:
            m_scanLinesPerBlock = 1;
            break;

        case ZIP_COMPRESSION:
            m_scanLinesPerBlock = 16;
            break;

        case PIZ_COMPRESSION:
            m_scanLinesPerBlock = 32;
            break;

        case PXR24_COMPRESSION:
            m_scanLinesPerBlock = 16;
            break;

        case B44_COMPRESSION:
            m_scanLinesPerBlock = 32;
            initLogTable();
            break;

        case B44A_COMPRESSION:
            m_scanLinesPerBlock = 32;
            initLogTable();
            break;

        case DWAA_COMPRESSION:
            m_scanLinesPerBlock = 32;
            break;

        case DWAB_COMPRESSION:
            m_scanLinesPerBlock = 256;
            break;

        default:
            m_header.setError("Incorrect compression: 0x%.8x", m_attributes.compression);
            return;
    }

    m_pointer = p;

    int width = m_attributes.dataWindow.xmax - m_attributes.dataWindow.xmin + 1;
    int height = m_attributes.dataWindow.ymax - m_attributes.dataWindow.ymin + 1;
    debugPrint("Image: %d x %d\n", width, height);

    m_header.width   = width;
    m_header.height  = height;
    m_header.depth   = 0;
    m_header.levels  = 0;
    m_header.faces   = 0;
    m_header.palette = false;
    m_header.format  = Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16);
    m_header.compression = TextureCompression::NONE;
}

ContextEXR::~ContextEXR()
{
}

const u8* ContextEXR::decompress_none(Memory dest, ConstMemory source)
{
    return source.address;
}

const u8* ContextEXR::decompress_rle(Memory dest, ConstMemory source)
{
    if (dest.size == source.size)
    {
        // no compression
        return source.address;
    }

    Buffer temp(dest.size);

    size_t source_bytes = source.size;
    const u8* in = source.address;
    u8* out = temp.data();
    u8* end = out + dest.size;

    while (source_bytes > 0)
    {
        int count = s8(*in++);

        if (count < 0)
        {
            count = -count;

            if (out + count > end)
                return dest;

            std::memcpy(out, in, count);
            out += count;
            in += count;
            source_bytes -= count + 1;
        }
        else 
        {
            ++count;

            if (out + count > end)
                return dest;

            u8 value = *in++;
            std::memset(out, value, count);
            out += count;
            source_bytes -= 2;
        }
    }

    predictor(temp, dest.size);
    deinterleave(dest, temp, dest.size);

    return dest;
}

const u8* ContextEXR::decompress_zip(Memory dest, ConstMemory source)
{
    Buffer temp(dest.size);

    CompressionStatus status = deflate_zlib::decompress(temp, source);
    //debugPrint("  out: %d bytes\n", int(status.size));

    predictor(temp, dest.size);
    deinterleave(dest, temp, dest.size);

    return dest;
}

const u8* ContextEXR::decompress_piz(Memory dest, ConstMemory source, int width, int height)
{
    const std::vector<Channel>& channels = m_attributes.chlist.channels;

    if (dest.size == source.size)
    {
        // no compression
        return source.address;
    }

    if (!source.size)
    {
        // special case: no input data
        return source.address;
    }

    std::vector<u8> bitmap(BITMAP_SIZE, 0);

    const u8* ptr = source.address; 

    u16 minNonZero = uload16le(ptr + 0);
    u16 maxNonZero = uload16le(ptr + 2);
    ptr += 4;

    if (maxNonZero >= BITMAP_SIZE)
    {
        return nullptr;
    }

    if (minNonZero <= maxNonZero)
    {
        size_t count = maxNonZero - minNonZero + 1;
        std::memcpy(bitmap.data() + minNonZero, ptr, count);
        ptr += count;
    }

    std::vector<u16> lut(USHORT_RANGE);
    u16 maxValue = reverseLutFromBitmap(bitmap.data(), lut.data());

    // Huffman decoding

    int length = uload32le(ptr);
    ptr += 4;

    size_t outputSize = 0;

    struct ChannelData
    {
        int nx;
        int ny;
        int wcount;
        int ysamples;
        size_t offset;
    };
    std::vector<ChannelData> data;

    for (size_t i = 0; i < channels.size(); ++i)
    {
        const Channel& chan = channels[i];

        int nx = div_ceil(width, chan.xsamples);
        int ny = div_ceil(height, chan.ysamples);
        int wcount = chan.bytes / 2;

        ChannelData cd;

        cd.nx = nx;
        cd.ny = ny;
        cd.wcount = wcount;
        cd.ysamples = chan.ysamples;
        cd.offset = outputSize;

        data.push_back(cd);

        outputSize += nx * ny * wcount;
    }

    std::vector<u16> tmpBuffer(outputSize);
    hufUncompress(ptr, length, tmpBuffer);

    // Wavelet decoding

    u16* buf = tmpBuffer.data();

    for (const auto& channel : data)
    {
        int nx = channel.nx;
        int ny = channel.ny;
        int wcount = channel.wcount;

        for (int j = 0; j < wcount; ++j)
        {
            wav2Decode(buf + j, nx, wcount, ny, nx * wcount, maxValue);
        }

        buf += nx * ny * wcount;
    }

    // Expand the pixel data to their original range

    applyLut(lut.data(), tmpBuffer.data(), tmpBuffer.size());

    // Rearrange the pixel data into the format expected by the caller

    u8* dst = dest.address;
    buf = tmpBuffer.data();

    for (int y = 0; y < height; ++y)
    {
        for (auto& channel : data)
        {
            if ((y % channel.ysamples) != 0)
                continue;

            int n = channel.nx * channel.wcount;
            std::memcpy(dst, buf + channel.offset, n * sizeof(u16));
            dst += n * sizeof(u16);
            channel.offset += n;
        }
    }

    return dest;
}

const u8* ContextEXR::decompress_pxr24(Memory dest, ConstMemory source, int width, int height, int y0)
{
    const std::vector<Channel>& channels = m_attributes.chlist.channels;

    Buffer temp(dest.size);
    CompressionStatus status = deflate_zlib::decompress(temp, source);

    u8* out = dest.address;
    u8* out_end = dest.end();
    const u8* lastIn = temp.data();
    const u8* lastIn_end = lastIn + dest.size;

    int y1 = y0 + height;

    for (int y = y0; y < y1; ++y)
    {
        for (size_t c = 0; c < channels.size(); ++c)
        {
            const Channel& channel = channels[c];

            int w = width; // channel.width
            size_t nBytes = w * channel.bytes;

            if (height == 0 || (channel.ysamples > 1 && (y % channel.ysamples) != 0))
                continue;

            if (out + nBytes > out_end)
            {
                debugPrint("OUT OF MEMORY\n");
                return nullptr;
            }

            switch (channel.type)
            {
                case UINT:
                {
                    const u8* ptr = lastIn;
                    lastIn += nBytes;

                    u32 pixel = 0;

                    if (lastIn > lastIn_end)
                    {
                        debugPrint("CORRUPT CHUNK\n");
                        return nullptr;
                    }

                    for (int x = 0; x < w; ++x)
                    {
                        u32 diff = ((u32(ptr[x + w * 0]) << 24) |
                                    (u32(ptr[x + w * 1]) << 16) |
                                    (u32(ptr[x + w * 2]) <<  8) |
                                    (u32(ptr[x + w * 3]) <<  0));
                        pixel += diff;
                        ustore32(out + x * 4, pixel);
                    }
                    break;
                }

                case HALF:
                {
                    const u8* ptr = lastIn;
                    lastIn += nBytes;

                    u32 pixel = 0;

                    if (lastIn > lastIn_end)
                    {
                        debugPrint("CORRUPT CHUNK\n");
                        return nullptr;
                    }

                    for (int x = 0; x < w; ++x)
                    {
                        u32 diff = ((u32(ptr[x + w * 0]) << 8) |
                                    (u32(ptr[x + w * 1]) << 0));
                        pixel += diff;
                        ustore16(out + x * 2, u16(pixel));
                    }
                    break;
                }

                case FLOAT:
                {
                    const u8* ptr = lastIn;
                    lastIn += w * 3; // 24 bits per sample

                    u32 pixel = 0;

                    if (lastIn > lastIn_end)
                    {
                        debugPrint("CORRUPT CHUNK\n");
                        return nullptr;
                    }

                    for (int x = 0; x < w; ++x)
                    {
                        u32 diff = ((u32(ptr[x + w * 0]) << 24) |
                                    (u32(ptr[x + w * 1]) << 16) |
                                    (u32(ptr[x + w * 2]) <<  8));
                        pixel += diff;
                        ustore32(out + x * 4, pixel);
                    }
                    break;
                }
            }

            out += nBytes;
        }
    }

    return dest;
}

const u8* ContextEXR::decompress_b44(Memory dest, ConstMemory source, int width, int height, int ystart)
{
    const std::vector<Channel>& channels = m_attributes.chlist.channels;

    Buffer temp(dest.size);

    u8* scratch = temp;
    const u8* in = source.address;
    size_t bIn = 0;
    size_t n;

    u16 s[16];

    for (size_t c = 0; c < channels.size(); ++c)
    {
        const Channel& channel = channels[c];

        int nx = width / channel.xsamples;
        int ny = height / channel.ysamples;
        size_t nBytes = ny * nx * channel.bytes;

        if (nBytes == 0)
            continue;

        if (channel.type != HALF)
        {
            //if (bIn + nBytes > comp_buf_size)
            //    return EXR_ERR_OUT_OF_MEMORY;

            std::memcpy(scratch, in, nBytes);
            in += nBytes;
            bIn += nBytes;
            scratch += nBytes;
            continue;
        }

        for (int y = 0; y < ny; y += 4)
        {
            u16* row0;
            u16* row1;
            u16* row2;
            u16* row3;

            row0 = (u16*) scratch;
            row0 += y * nx;
            row1 = row0 + nx;
            row2 = row1 + nx;
            row3 = row2 + nx;

            //debugPrint("(%d,%d) [%d x %d]\n", 0, y, nx, ny);
            for (int x = 0; x < nx; x += 4)
            {
                if (bIn + 3 > source.size)
                {
                //    return EXR_ERR_OUT_OF_MEMORY;
                }

                // check if 3-byte encoded flat field
                if (in[2] >= (13 << 2))
                {
                    unpack3(in, s);
                    in += 3;
                    bIn += 3;
                }
                else
                {
                    //if (bIn + 14 > comp_buf_size)
                    //    return EXR_ERR_OUT_OF_MEMORY;

                    unpack14(in, s);
                    in += 14;
                    bIn += 14;
                }

                if (channel.linear)
                    convertToLinear(s);

                //priv_from_native16(s, 16);

                n = (x + 3 < nx) ? 4 * sizeof(u16) : (size_t) (nx - x) * sizeof(u16);
                if (y + 3 < ny)
                {
                    std::memcpy(row0, &s[0], n);
                    std::memcpy(row1, &s[4], n);
                    std::memcpy(row2, &s[8], n);
                    std::memcpy(row3, &s[12], n);
                }
                else
                {
                    std::memcpy(row0, &s[0], n);
                    if (y + 1 < ny) std::memcpy(row1, &s[4], n);
                    if (y + 2 < ny) std::memcpy(row2, &s[8], n);
                }

                row0 += 4;
                row1 += 4;
                row2 += 4;
                row3 += 4;
            }
        }

        scratch += nBytes;
    }

    // now put it back so each scanline has channel data
    bIn = 0;
    u8* out = dest.address;

    for (int y = 0; y < height; ++y)
    {
        int cury = y + ystart;

        scratch = temp;

        for (size_t c = 0; c < channels.size(); ++c)
        {
            const Channel& channel = channels[c];

            int nx = width / channel.xsamples;
            int ny = height / channel.ysamples;
            size_t bpl    = ((u64) (nx)) * (u64) channel.bytes;
            size_t nBytes = ((u64) (ny)) * bpl;

            if (nBytes == 0)
                continue;

            u8* tmp = scratch;
            if (channel.ysamples > 1)
            {
                if ((cury % channel.ysamples) != 0)
                {
                    scratch += nBytes;
                    continue;
                }
                tmp += ((u64) (y / channel.ysamples)) * bpl;
            }
            else
            {
                tmp += u64(y) * bpl;
            }

            //if (bIn + bpl > uncomp_buf_size)
            //    return EXR_ERR_OUT_OF_MEMORY;

            std::memcpy(out, tmp, bpl);

            bIn += bpl;
            out += bpl;
            scratch += nBytes;
        }
    }

    return dest;
}

const u8* ContextEXR::decompress_dwaa(Memory dest, ConstMemory source, int width, int height, int ystart)
{
    MANGO_UNREFERENCED(dest);
    MANGO_UNREFERENCED(source);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
    MANGO_UNREFERENCED(ystart);

    // TODO
    return nullptr;
}

const u8* ContextEXR::decompress_dwab(Memory dest, ConstMemory source, int width, int height, int ystart)
{
    MANGO_UNREFERENCED(dest);
    MANGO_UNREFERENCED(source);
    MANGO_UNREFERENCED(width);
    MANGO_UNREFERENCED(height);
    MANGO_UNREFERENCED(ystart);

    // TODO
    return nullptr;
}

static inline
void writeChromaRGBA(float16* dest, float32x3 yw, float RY, float BY, float Y, float16 alpha)
{
    float r = RY * Y + Y;
    float b = BY * Y + Y;
    float g = (Y - r * yw.x - b * yw.z) / yw.y;
    dest[0] = linear_to_srgb(r); // TODO: replace with gamma()
    dest[1] = linear_to_srgb(g);
    dest[2] = linear_to_srgb(b);
    dest[3] = alpha;
}

static
void decodeLuminance(Surface surface, const u8* src, const AttributeTable& attributes, int x0, int y0, int x1, int y1)
{
    int blockWidth = x1 - x0;
    //int blockHeight = y1 - y0;

    size_t bytesPerPixel = attributes.chlist.bytes;
    size_t bytesPerScan = blockWidth * bytesPerPixel;

    for (int y = y0; y < y1; ++y)
    {
        float16* image = surface.address<float16>(x0, y);

        const float16* scan = reinterpret_cast<const float16*>(src + blockWidth * 0);

        // TODO: FLOAT, alpha

        float16 one(1.0f);

        for (int x = x0; x < x1; ++x)
        {
            float16 s = *scan++;
            image[0] = s;
            image[1] = s;
            image[2] = s;
            image[3] = one;
            image += 4;
        }

        src += bytesPerScan;
    }
}

static
void decodeChroma(Surface surface, const u8* src, const AttributeTable& attributes, int x0, int y0, int x1, int y1)
{
    int blockWidth = x1 - x0;
    //int blockHeight = y1 - y0;

    size_t bytesPerPixel = attributes.chlist.bytes;
    size_t bytesPerScan = blockWidth * bytesPerPixel;

    float32x3 yw = computeYW(attributes.chromaticities, 1.0f);

    // TODO: configure channel inputs correctly
    // TODO: support UINT and FLOAT
    // TODO: alpha

    for (int y = y0; y < y1; y += 2)
    {
        // TODO: clip modulo scanline at the end
        float16* image0 = surface.address<float16>(0, y + 0);
        float16* image1 = surface.address<float16>(0, y + 1);

        float16 alpha(1.0f);

#if 1
        const float16* scan_b = reinterpret_cast<const float16*>(src + blockWidth * 0);
        const float16* scan_r = reinterpret_cast<const float16*>(src + blockWidth * 1);
        const float16* scan_0 = reinterpret_cast<const float16*>(src + blockWidth * 2);
        const float16* scan_1 = reinterpret_cast<const float16*>(src + blockWidth * 4);
#endif
        //const float16* scan_0 = reinterpret_cast<const float16*>(src + blockWidth * 0);
        //const float16* scan_1 = reinterpret_cast<const float16*>(src + blockWidth * 2);

        // TODO: clip modulo pixel at the end
        for (int x = x0; x < x1; x += 2)
        {
            float RY = scan_r[x / 2];
            float BY = scan_b[x / 2];
            float Y0 = scan_0[x + 0];
            float Y1 = scan_0[x + 1];
            float Y2 = scan_1[x + 0];
            float Y3 = scan_1[x + 1];

            writeChromaRGBA(image0 + 0, yw, RY, BY, Y0, alpha);
            writeChromaRGBA(image0 + 4, yw, RY, BY, Y1, alpha);
            writeChromaRGBA(image1 + 0, yw, RY, BY, Y2, alpha);
            writeChromaRGBA(image1 + 4, yw, RY, BY, Y3, alpha);

            image0 += 8;
            image1 += 8;
        }

        //src += blockWidth * 4;
        //src += blockWidth * 6;
        src += bytesPerScan;
    }
}

static
void decodeRGB(Surface surface, const u8* src, const AttributeTable& attributes, int x0, int y0, int x1, int y1)
{
    int blockWidth = x1 - x0;
    //int blockHeight = y1 - y0;

    size_t bytesPerPixel = attributes.chlist.bytes;
    size_t bytesPerScan = blockWidth * bytesPerPixel;

    int ch_offset[4] = { 0, 0, 0, 0 };
    int ch_size[4] = { 0, 0, 0, 0 };

    for (auto channel : attributes.chlist.channels)
    {
        if (channel.component >= 0)
        {
            ch_offset[channel.component] = channel.offset;
            ch_size[channel.component] = channel.bytes;
        }
    }

    //debugPrint("  blitting: (%d, %d) %d x %d)\n", x0, y0, x1 - x0, y1 - y0);

    for (int y = y0; y < y1; ++y)
    {
        float16* image = surface.address<float16>(x0, y);

        // TODO: component type (UINT, HALF, FLOAT)
        // TODO: UINT is not "color" but support it for extraction of arbitrary channels

        float16 values [] = { 0.0f, 0.0f, 0.0f, 1.0f };

        const float16* ptr[4];
        size_t step[4];

        for (int i = 0; i < 4; ++i)
        {
            if (ch_size[i] > 0)
            {
                ptr[i] = reinterpret_cast<const float16*>(src + blockWidth * ch_offset[i]);
                step[i] = 1;
            }
            else
            {
                ptr[i] = values + i;
                step[i] = 0;
            }
        }

        for (int x = x0; x < x1; ++x)
        {
            image[0] = linear_to_srgb(*ptr[0]); // TODO: replace with gamma()
            image[1] = linear_to_srgb(*ptr[1]);
            image[2] = linear_to_srgb(*ptr[2]);
            image[3] = *ptr[3];

            ptr[0] += step[0];
            ptr[1] += step[1];
            ptr[2] += step[2];
            ptr[3] += step[3];

            image += 4;
        }

        src += bytesPerScan;
    }
}

void ContextEXR::decodeBlock(Surface surface, ConstMemory memory, int x0, int y0, int x1, int y1)
{
    int blockWidth = x1 - x0;
    int blockHeight = y1 - y0;

    size_t bytesPerPixel = m_attributes.chlist.bytes;
    size_t bytesPerScan = blockWidth * bytesPerPixel;

    Buffer buffer(blockHeight * bytesPerScan);

    const u8* src = nullptr;

    u64 time0 = mango::Time::us();

    switch (m_attributes.compression)
    {
        case NO_COMPRESSION:
            src = decompress_none(buffer, memory);
            break;

        case RLE_COMPRESSION:
            src = decompress_rle(buffer, memory);
            break;

        case ZIPS_COMPRESSION:
        case ZIP_COMPRESSION:
            src = decompress_zip(buffer, memory);
            break;

        case PIZ_COMPRESSION:
            src = decompress_piz(buffer, memory, blockWidth, blockHeight);
            break;

        case PXR24_COMPRESSION:
            src = decompress_pxr24(buffer, memory, blockWidth, blockHeight, y0);
            break;

        case B44_COMPRESSION:
            src = decompress_b44(buffer, memory, blockWidth, blockHeight, y0);
            break;

        case B44A_COMPRESSION:
            src = decompress_b44(buffer, memory, blockWidth, blockHeight, y0);
            break;

        case DWAA_COMPRESSION:
            src = decompress_dwaa(buffer, memory, blockWidth, blockHeight, y0);
            break;

        case DWAB_COMPRESSION:
            src = decompress_dwab(buffer, memory, blockWidth, blockHeight, y0);
            break;

        default:
            break;
    }

    if (!src)
    {
        // decompression failed
        return;
    }

    u64 time1 = mango::Time::us();

    switch (m_attributes.chlist.colortype)
    {
        case ColorType::LUMINANCE:
            decodeLuminance(surface, src, m_attributes, x0, y0, x1, y1);
            break;

        case ColorType::CHROMA:
            decodeChroma(surface, src, m_attributes, x0, y0, x1, y1);
            break;

        case ColorType::RGB:
            decodeRGB(surface, src, m_attributes, x0, y0, x1, y1);
            break;

        case ColorType::NONE:
            break;
    }

    u64 time2 = mango::Time::us();

    m_time_decompress += (time1 - time0);
    m_time_blit += (time2 - time1);
}

ImageDecodeStatus ContextEXR::decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face)
{
    ImageDecodeStatus status;

    if (!m_pointer)
    {
        status.setError("Decoding stopped.");
        return status;
    }

    u64 time0 = mango::Time::us();

    int width = m_header.width;
    int height = m_header.height;

    Bitmap bitmap(m_header.width, m_header.height, Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16));

    Surface surface(bitmap);
    if (m_attributes.lineOrder)
    {
        surface.image = surface.image + surface.stride * (surface.height - 1);
        surface.stride = -surface.stride;
    }

    LittleEndianConstPointer p = m_pointer;

    ConcurrentQueue q;

    if (is_single_tile)
    {
        int xtiles = div_ceil(width, m_attributes.tiles.xsize);
        int ytiles = div_ceil(height, m_attributes.tiles.ysize);
        int ntiles = xtiles * ytiles;

        debugPrint("Tiles: %d x %d (%d)\n", xtiles, ytiles, ntiles);

        for (int i = 0; i < ntiles; ++i)
        {
            u64 offset = p.read64();
            LittleEndianConstPointer ptr = m_memory.address + offset;

            int tilex = ptr.read32();
            int tiley = ptr.read32();
            int xlevel = ptr.read32();
            int ylevel = ptr.read32();
            u32 size = ptr.read32();

            //debugPrint("  pos:(%d,%d) level:(%d,%d) size: %d bytes\n", tilex, tiley, xlevel, ylevel, size);
            MANGO_UNREFERENCED(xlevel);
            MANGO_UNREFERENCED(ylevel);

            int tileWidth = m_attributes.tiles.xsize;
            int tileHeight = m_attributes.tiles.ysize;

            int x0 = tilex * tileWidth;
            int y0 = tiley * tileHeight;
            int x1 = std::min(width, x0 + tileWidth);
            int y1 = std::min(height, y0 + tileHeight);

            auto task = [=]
            {
                ConstMemory memory(ptr, size);
                decodeBlock(surface, memory, x0, y0, x1, y1);
            };

            if (options.multithread)
            {
                q.enqueue(task);
            }
            else
            {
                task();
            }
        }
    }
    else
    {
        int nblocks = div_ceil(height, m_scanLinesPerBlock);

        debugPrint("Blocks: %d\n", nblocks);

        for (int i = 0; i < nblocks; ++i)
        {
            u64 offset = p.read64();
            LittleEndianConstPointer ptr = m_memory.address + offset;

            int ystart = ptr.read32();
            u32 size = ptr.read32();

            //debugPrint("  y:%d, size: %d bytes\n", ystart, size);

            int x0 = 0;
            int y0 = ystart - m_attributes.dataWindow.ymin;
            int x1 = width;
            int y1 = std::min(height, y0 + m_scanLinesPerBlock);

            auto task = [=]
            {
                ConstMemory memory(ptr, size);
                decodeBlock(surface, memory, x0, y0, x1, y1);
            };

            if (options.multithread)
            {
                q.enqueue(task);
            }
            else
            {
                task();
            }
        }
    }

    q.wait();

    u64 time1 = mango::Time::us();
    m_time_decode += (time1 - time0);

    dest.blit(0, 0, bitmap);

    report();

    return status;
}

} // namespace

namespace
{
    using namespace mango;
    using namespace mango::image;

    // ------------------------------------------------------------
    // ImageDecoderEXR
    // ------------------------------------------------------------

    struct ImageDecoderEXR : ImageDecoderInterface
    {
        ContextEXR m_context;

        ImageDecoderEXR(ConstMemory memory)
            : m_context(memory)
        {
        }

        ~ImageDecoderEXR()
        {
        }

        ImageHeader header() override
        {
            return m_context.m_header;
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            return m_context.decode(dest, options, level, depth, face);
        }
    };

    ImageDecoderInterface* createInterface(ConstMemory memory)
    {
        ImageDecoderInterface* x = new ImageDecoderEXR(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecEXR()
    {
        registerImageDecoder(createInterface, ".exr");
    }

} // namespace mango::image
