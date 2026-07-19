/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <cctype>
#include <cmath>
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

    // MANGO TODO: mipmap / ripmap selection
    // MANGO TODO: deep image support
    // MANGO TODO: multi-part support
    // MANGO TODO: more flexible color resolver (more formats)

    /*
    // exr half/float to uint8 conversion:
    u8 gamma(float x)
    {
        x = max(0.0f, x);
        x = pow (5.5555f x, 0.4545f) * 84.66f;
        return u8(clamp(x, 0.0f, 255.0f));
    }
    */

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
{                                 \
    c = (c << 8) | *(u8 *)(in++); \
    lc += 8;                      \
}

#define getCode(po, rlc, c, lc, in, out, ob, oe) \
{                               \
    if (po == rlc)              \
    {                           \
        if (lc < 8)             \
            getChar(c, lc, in); \
                                \
        lc -= 8;                \
                                \
        u8 cs = (c >> lc);      \
                                \
        /* clamp the run to the output buffer; corrupt data must not */ \
        /* read before the start or write past the end */ \
        u16 s = (out > ob) ? out[-1] : 0; \
                                \
        while (cs-- > 0 && out < oe) \
            *out++ = s;         \
    }                           \
    else if (out < oe)          \
    {                           \
        *out++ = u16(po);       \
    }                           \
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
                /*
                if ( lc < 0 )
                {
                    //invalidCode(); // code length too long
                }
                */
                getCode(pl.lit, rlc, c, lc, in, out, outb, oe)
            }
            else
            {
                if (!pl.p)
                {
                    // wrong code: bail out instead of spinning forever on
                    // corrupt input (nothing would consume any bits here)
                    break;
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
                            getCode(pl.p[j], rlc, c, lc, in, out, outb, oe)
                            break;
                        }
                    }
                }

                if (j == pl.lit)
                {
                    // not found: bail out to avoid an infinite loop on
                    // corrupt input (no bits would be consumed otherwise)
                    break;
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
            /*
            if ( lc < 0 )
            {
                //invalidCode(); // code length too long
            }
            */
            getCode(pl.lit, rlc, c, lc, in, out, outb, oe)
        }
        else
        {
            // wrong (long) code: bail out, otherwise lc never decreases
            break;
        }
    }

    if (out - outb != ptrdiff_t(no))
    {
        //notEnoughData ();
    }
}

//
// FAST DECODER
//
// Canonical Huffman decoder based on 'On the Implementation of Minimum
// Redundancy Prefix Codes' by Moffat and Turpin, ported from OpenEXR's
// FastHufDecoder (ImfFastHuf.cpp). It is bitstream-compatible with the
// reference hufDecode() above, but several times faster: it keeps a 64-bit
// bit buffer (bulk refill, no per-symbol byte juggling) and resolves all
// short codes (<= TABLE_LOOKUP_BITS) through a compact, L1-resident table
// instead of the 256 KB hash table the reference decoder walks.
//
// Used only for streams of at least 128 bits; shorter streams fall back to
// the reference decoder.
//

class FastHufDecoder
{
public:
    static const int MAX_CODE_LEN = 58;
    static const int TABLE_LOOKUP_BITS = 12;

    FastHufDecoder() = default;

    ~FastHufDecoder()
    {
        delete[] _idToSymbol;
    }

    FastHufDecoder(const FastHufDecoder&) = delete;
    FastHufDecoder& operator = (const FastHufDecoder&) = delete;

    // Parse the packed code-length table (advancing 'table') and build the
    // acceleration tables. Returns false on malformed table data.
    bool build(const u8*& table, int numBytes, int minSymbol, int maxSymbol, int rleSymbol);

    // Decode 'numDstElems' symbols from the byte-aligned bitstream 'src'
    // (which holds 'numSrcBits' bits). Returns false on corrupt input.
    bool decode(const u8* src, int numSrcBits, u16* dst, int numDstElems);

private:
    static u64 read64(const u8* p)
    {
        return bigEndian::uload64(p);
    }

    static u64 readBits(int numBits, u64& buffer, int& bufferNumBits, const u8*& currByte)
    {
        while (bufferNumBits < numBits)
        {
            buffer = (buffer << 8) | *currByte++;
            bufferNumBits += 8;
        }
        bufferNumBits -= numBits;
        return (buffer >> bufferNumBits) & ((u64(1) << numBits) - 1);
    }

    void refill(u64& buffer, int numBits, u64& bufferBack, int& bufferBackNumBits,
                const u8*& currByte, int& currBitsLeft)
    {
        buffer |= bufferBack >> (64 - numBits);

        if (bufferBackNumBits < numBits)
        {
            numBits -= bufferBackNumBits;

            if (currBitsLeft >= 64)
            {
                bufferBack = read64(currByte);
                bufferBackNumBits = 64;
                currByte += sizeof(u64);
                currBitsLeft -= 8 * int(sizeof(u64));
            }
            else
            {
                bufferBack = 0;
                bufferBackNumBits = 64;

                int shift = 56;
                while (currBitsLeft > 0)
                {
                    bufferBack |= u64(*currByte) << shift;
                    ++currByte;
                    shift -= 8;
                    currBitsLeft -= 8;
                }

                if (currBitsLeft < 0)
                    currBitsLeft = 0;
            }

            buffer |= bufferBack >> (64 - numBits);
        }

        if (bufferBackNumBits <= numBits)
            bufferBack = 0;
        else
            bufferBack = bufferBack << numBits;
        bufferBackNumBits -= numBits;
    }

    bool buildTables(const u64* base, const u64* offset);

    int _rleSymbol = 0;
    int _numSymbols = 0;
    int _minCodeLength = 255;
    int _maxCodeLength = 0;
    int* _idToSymbol = nullptr;

    u64 _ljBase[MAX_CODE_LEN + 1];
    u64 _ljOffset[MAX_CODE_LEN + 1];

    int _tableSymbol[1 << TABLE_LOOKUP_BITS];
    u8  _tableCodeLen[1 << TABLE_LOOKUP_BITS];
    u64 _tableMin = 0;
};

bool FastHufDecoder::build(const u8*& table, int numBytes, int minSymbol, int maxSymbol, int rleSymbol)
{
    _rleSymbol = rleSymbol;

    std::vector<u64> symbols;

    u64 base[MAX_CODE_LEN + 1];
    u64 offset[MAX_CODE_LEN + 1];
    size_t codeCount[MAX_CODE_LEN + 1];

    for (int i = 0; i <= MAX_CODE_LEN; ++i)
    {
        codeCount[i] = 0;
        base[i] = 0xffffffffffffffffULL;
        offset[i] = 0;
    }

    const u8* currByte = table;
    u64 currBits = 0;
    int currBitCount = 0;

    for (u64 symbol = u64(minSymbol); symbol <= u64(maxSymbol); ++symbol)
    {
        if (currByte - table >= numBytes)
            return false;

        u64 codeLen = readBits(6, currBits, currBitCount, currByte);

        if (codeLen == u64(LONG_ZEROCODE_RUN))
        {
            if (currByte - table >= numBytes)
                return false;

            int runLen = int(readBits(8, currBits, currBitCount, currByte)) + SHORTEST_LONG_RUN;
            if (symbol + runLen > u64(maxSymbol + 1))
                return false;

            symbol += runLen - 1;
        }
        else if (codeLen >= u64(SHORT_ZEROCODE_RUN))
        {
            int runLen = int(codeLen) - SHORT_ZEROCODE_RUN + 2;
            if (symbol + runLen > u64(maxSymbol + 1))
                return false;

            symbol += runLen - 1;
        }
        else if (codeLen != 0)
        {
            symbols.push_back((symbol << 6) | (codeLen & 63));

            if (int(codeLen) < _minCodeLength)
                _minCodeLength = int(codeLen);
            if (int(codeLen) > _maxCodeLength)
                _maxCodeLength = int(codeLen);

            codeCount[codeLen]++;
        }
    }

    for (int i = 0; i < MAX_CODE_LEN; ++i)
        _numSymbols += int(codeCount[i]);

    table = currByte;

    if (_numSymbols == 0 || _maxCodeLength == 0 || _maxCodeLength > MAX_CODE_LEN)
        return false;

    // Closed-form 'base' (minimum numeric code at each length).
    {
        std::vector<double> countTmp(_maxCodeLength + 1, 0.0);

        for (int l = _minCodeLength; l <= _maxCodeLength; ++l)
            countTmp[l] = double(codeCount[l]) * double(2ll << (_maxCodeLength - l));

        for (int l = _minCodeLength; l <= _maxCodeLength; ++l)
        {
            double tmp = 0;
            for (int k = l + 1; k <= _maxCodeLength; ++k)
                tmp += countTmp[k];
            tmp /= double(2ll << (_maxCodeLength - l));
            base[l] = u64(std::ceil(tmp));
        }
    }

    // 'offset' (first id at each length, in sorted-by-length id order).
    offset[_maxCodeLength] = 0;
    for (int i = _maxCodeLength - 1; i >= _minCodeLength; --i)
        offset[i] = offset[i + 1] + codeCount[i + 1];

    _idToSymbol = new int[_numSymbols];

    u64 mapping[MAX_CODE_LEN + 1];
    for (int i = 0; i <= MAX_CODE_LEN; ++i)
        mapping[i] = u64(-1);
    for (int i = _minCodeLength; i <= _maxCodeLength; ++i)
        mapping[i] = offset[i];

    for (u64 sym : symbols)
    {
        int codeLen = int(sym & 63);
        int symbol = int(sym >> 6);

        if (mapping[codeLen] >= u64(_numSymbols))
            return false;

        _idToSymbol[mapping[codeLen]] = symbol;
        mapping[codeLen]++;
    }

    return buildTables(base, offset);
}

bool FastHufDecoder::buildTables(const u64* base, const u64* offset)
{
    for (int i = 0; i <= MAX_CODE_LEN; ++i)
    {
        if (base[i] != 0xffffffffffffffffULL)
            _ljBase[i] = base[i] << (64 - i);
        else
            _ljBase[i] = 0xffffffffffffffffULL;
    }

    _ljOffset[0] = offset[0] - _ljBase[0];
    for (int i = 1; i <= MAX_CODE_LEN; ++i)
        _ljOffset[i] = offset[i] - (_ljBase[i] >> (64 - i));

    for (u64 i = 0; i < (1 << TABLE_LOOKUP_BITS); ++i)
    {
        u64 value = i << (64 - TABLE_LOOKUP_BITS);

        _tableSymbol[i] = 0xffff;
        _tableCodeLen[i] = 0;

        for (int codeLen = _minCodeLength; codeLen <= _maxCodeLength; ++codeLen)
        {
            if (_ljBase[codeLen] <= value)
            {
                _tableCodeLen[i] = u8(codeLen);

                u64 id = _ljOffset[codeLen] + (value >> (64 - codeLen));
                if (id < u64(_numSymbols))
                    _tableSymbol[i] = _idToSymbol[id];
                else
                    return false;
                break;
            }
        }
    }

    int minIdx = TABLE_LOOKUP_BITS;
    while (minIdx > 0 && _ljBase[minIdx] == 0xffffffffffffffffULL)
        --minIdx;

    if (minIdx < 0)
        _tableMin = 0xffffffffffffffffULL;
    else
        _tableMin = _ljBase[minIdx];

    return true;
}

bool FastHufDecoder::decode(const u8* src, int numSrcBits, u16* dst, int numDstElems)
{
    if (numSrcBits < 128)
        return false;

    const u8* currByte = src + 2 * sizeof(u64);
    int currBitsLeft = numSrcBits - 8 * 2 * int(sizeof(u64));

    u64 buffer = read64(src);
    int bufferNumBits = 64;

    u64 bufferBack = read64(src + sizeof(u64));
    int bufferBackNumBits = 64;

    int dstIdx = 0;

    while (dstIdx < numDstElems)
    {
        int codeLen;
        int symbol;

        if (_tableMin <= buffer)
        {
            int tableIdx = int(buffer >> (64 - TABLE_LOOKUP_BITS));
            codeLen = _tableCodeLen[tableIdx];
            symbol = _tableSymbol[tableIdx];

            if (codeLen == 0)
                return false; // invalid code
        }
        else
        {
            if (bufferNumBits < 64)
            {
                refill(buffer, 64 - bufferNumBits, bufferBack, bufferBackNumBits,
                       currByte, currBitsLeft);
                bufferNumBits = 64;
            }

            codeLen = TABLE_LOOKUP_BITS + 1;
            while (codeLen <= _maxCodeLength && _ljBase[codeLen] > buffer)
                ++codeLen;

            if (codeLen > _maxCodeLength)
                return false;

            u64 id = _ljOffset[codeLen] + (buffer >> (64 - codeLen));
            if (id < u64(_numSymbols))
                symbol = _idToSymbol[id];
            else
                return false;
        }

        buffer = buffer << codeLen;
        bufferNumBits -= codeLen;

        if (symbol == _rleSymbol)
        {
            if (bufferNumBits < 8)
            {
                refill(buffer, 64 - bufferNumBits, bufferBack, bufferBackNumBits,
                       currByte, currBitsLeft);
                bufferNumBits = 64;
            }

            int rleCount = int(buffer >> 56);

            if (dstIdx < 1 || rleCount <= 0 || dstIdx + rleCount > numDstElems)
                return false;

            u16 prev = dst[dstIdx - 1];
            for (int i = 0; i < rleCount; ++i)
                dst[dstIdx + i] = prev;

            dstIdx += rleCount;

            buffer = buffer << 8;
            bufferNumBits -= 8;
        }
        else
        {
            dst[dstIdx] = u16(symbol);
            ++dstIdx;
        }

        if (bufferNumBits < TABLE_LOOKUP_BITS)
        {
            refill(buffer, 64 - bufferNumBits, bufferBack, bufferBackNumBits,
                   currByte, currBitsLeft);
            bufferNumBits = 64;
        }
    }

    return currBitsLeft == 0;
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

    int im = littleEndian::uload32(compressed + 0);
    int iM = littleEndian::uload32(compressed + 4);
    int nBits = littleEndian::uload32(compressed + 12);

    if (im < 0 || im >= HUF_ENCSIZE || iM < 0 || iM >= HUF_ENCSIZE)
    {
        return false;
    }

    constexpr int headerSize = 20;

    compressed += headerSize;
    nCompressed -= headerSize;

    if (nBits > 8 * nCompressed)
    {
        return false;
    }

    // Fast path: the table-accelerated decoder needs the encoding table plus a
    // bitstream of at least 128 bits. The table is parsed directly from the
    // packed bytes; on any malformed input we fall back to the reference path.
    if (nBits > 128 && !output.empty())
    {
        const u8* tablePtr = compressed;
        FastHufDecoder fhd;

        if (fhd.build(tablePtr, nCompressed, im, iM, iM))
        {
            const int tableBytes = int(tablePtr - compressed);
            if (8 * (nCompressed - tableBytes) >= nBits &&
                fhd.decode(tablePtr, nBits, output.data(), int(output.size())))
            {
                return true;
            }
        }
    }

    std::vector<u64> freq(HUF_ENCSIZE);
    std::vector<HufDec> hdec(HUF_DECSIZE);

    hufClearDecTable(hdec.data());
    hufUnpackEncTable(&compressed, nCompressed, im, iM, freq.data());

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

    return u16(n); // maximum k where lut[k] is non-zero
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

enum class DataType
{
    UINT = 0,
    HALF = 1,
    FLOAT = 2,
    NONE
};

enum class ColorType
{
    LUMINANCE,
    CHROMA,
    RGB,
    NONE
};

enum class Component
{
    R,
    G,
    B,
    A,
    Y,
    RY,
    BY,
    NONE
};

struct Channel
{
    DataType datatype = DataType::NONE;
    int xsamples = 1;
    int ysamples = 1;
    int linear = 0;

    int bytes = 0;
    int offset = 0;
    Component component = Component::NONE;

    // full channel name (e.g. "R", "diffuse.G"); needed by the DWA decoder
    // to classify channels (DCT / RLE / lossless) and group RGB CSC sets.
    std::string name;
};

struct Layer
{
    std::string name;
    std::vector<Channel> channels;
    ColorType colortype = ColorType::NONE;
    int bytes = 0;
};

struct ChannelList
{
    // decompressor needs all channels
    std::vector<Channel> channels;
    int bytes = 0;

    // decoder is only interested in a specific layer
    std::vector<Layer> layers;

    Layer& getLayer(const std::string& name)
    {
        for (Layer& layer : layers)
        {
            if (layer.name == name)
                return layer;
        }

        Layer temp;
        temp.name = name;

        layers.push_back(temp);
        return layers.back();
    }
};

struct TileDesc
{
    u32 xsize = 0;
    u32 ysize = 0;
    u8 mode = 0; // 0: single image, 1: mipmap, 2: ripmap
                 // 5th bit (0x10) indicates rounding direction (0: down, 1: up)

    bool isMipmap() const
    {
        return (mode & 0x01) != 0;
    }

    bool isRipmap() const
    {
        return (mode & 0x02) != 0;
    }
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
    int offset = 0;

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

        channel.name = name;
        channel.xsamples = xsamples;
        channel.ysamples = ysamples;
        channel.linear = linear;
        //printLine(Print::Debug, "  {} xs: {}  ys: {}  linear: {}", name, xsamples, ysamples, linear);

        switch (type)
        {
            case 0:
                channel.datatype = DataType::UINT;
                channel.bytes = 4;
                break;
            case 1:
                channel.datatype = DataType::HALF;
                channel.bytes = 2;
                break;
            case 2:
                channel.datatype = DataType::FLOAT;
                channel.bytes = 4;
                break;
            default:
                channel.datatype = DataType::NONE;
                channel.bytes = 0;
                break;
        }

        channel.offset = offset;

        std::string layer_name;
        std::string channel_name = name;

        auto n = channel_name.find_last_of('.');
        if (n != std::string_view::npos)
        {
            layer_name = channel_name.substr(0, n);
            channel_name = channel_name.substr(n + 1);
        }

        if (channel_name == "R")
        {
            channel.component = Component::R;
        }
        else if (channel_name == "G")
        {
            channel.component = Component::G;
        }
        else if (channel_name == "B")
        {
            channel.component = Component::B;
        }
        else if (channel_name == "A")
        {
            channel.component = Component::A;
        }
        else if (channel_name == "Y")
        {
            channel.component = Component::Y;
        }
        else if (channel_name == "RY")
        {
            channel.component = Component::RY;
        }
        else if (channel_name == "BY")
        {
            channel.component = Component::BY;
        }
        else
        {
            // not supported
            channel.component = Component::NONE;
        }

        if (channel.component != Component::NONE)
        {
            Layer& layer = data.getLayer(layer_name);
            layer.channels.push_back(channel);
        }

        data.channels.push_back(channel);

        offset += div_ceil(channel.bytes, xsamples);
        data.bytes += channel.bytes;

        printLine(Print::Debug, "    \"{}\", type: {}, linear: {}, offset: {}, size: {}, samples: ({} x {})",
            name.data(), type, linear, channel.offset, channel.bytes, xsamples, ysamples);
    }

    for (Layer& layer : data.layers)
    {
        bool rgb = false;
        bool luminance = false;
        bool chroma = false;

        for (Channel& channel : layer.channels)
        {
            switch (channel.component)
            {
                case Component::R:
                case Component::G:
                case Component::B:
                    rgb = true;
                    break;
                case Component::A:
                    break;
                case Component::Y:
                    luminance = true;
                    break;
                case Component::RY:
                case Component::BY:
                    chroma = true;
                    break;
                case Component::NONE:
                    break;
            }
        }

        if (luminance)
        {
            if (chroma)
            {
                layer.colortype = ColorType::CHROMA;
            }
            else
            {
                layer.colortype = ColorType::LUMINANCE;
            }
        }
        else if (rgb)
        {
            layer.colortype = ColorType::RGB;
        }

        layer.bytes = data.bytes;
    }

    printLine(Print::Debug, "    DataPerPixel: {} bytes", data.bytes);
}

template <>
void readAttribute<TileDesc>(TileDesc& data, LittleEndianConstPointer p)
{
    data.xsize = p.read32();
    data.ysize = p.read32();
    data.mode = p.read8();
    printLine(Print::Debug, "    {} x {} mode: {:#x} (mipmap: {}, ripmap: {}, round: {})", data.xsize, data.ysize, data.mode,
        (data.mode & 0x02) != 0, (data.mode & 0x02) != 0, (data.mode & 0x10) != 0);
}

/*
template <>
void readAttribute<String>(String& data, LittleEndianConstPointer p)
{
    MANGO_UNREFERENCED(data);
    MANGO_UNREFERENCED(p);
}

template <>
void readAttribute<Text>(Text& data, LittleEndianConstPointer p)
{
    MANGO_UNREFERENCED(data);
    MANGO_UNREFERENCED(p);
}
*/

template <>
void readAttribute<u8>(u8& data, LittleEndianConstPointer p)
{
    data = p.read8();
    printLine(Print::Debug, "    {}", data);
}

/*
template <>
void readAttribute<u32>(u32& data, LittleEndianConstPointer p)
{
    data = p.read32();
    printLine(Print::Debug, "    {}", data);
}
*/

template <>
void readAttribute<float>(float& data, LittleEndianConstPointer p)
{
    data = p.read32f();
    printLine(Print::Debug, "    {}", data);
}

template <>
void readAttribute<float32x2>(float32x2& data, LittleEndianConstPointer p)
{
    data.x = p.read32f();
    data.y = p.read32f();
    printLine(Print::Debug, "    {}, {}", float(data.x), float(data.y));
}

template <>
void readAttribute<Box2i>(Box2i& data, LittleEndianConstPointer p)
{
    data.xmin = p.read32();
    data.ymin = p.read32();
    data.xmax = p.read32();
    data.ymax = p.read32();
    printLine(Print::Debug, "    {}, {}, {}, {}", data.xmin, data.ymin, data.xmax, data.ymax);
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
    printLine(Print::Debug, "    red:   {}, {}", float(data.red.x), float(data.red.y));
    printLine(Print::Debug, "    green: {}, {}", float(data.green.x), float(data.green.y));
    printLine(Print::Debug, "    blue:  {}, {}", float(data.blue.x), float(data.blue.y));
    printLine(Print::Debug, "    white: {}, {}", float(data.white.x), float(data.white.y));
}

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
    u8          envmap = 0; // 0: None, 1: LatLong, 2: Cube
                            // cube face order: +x, -x, +y, -y, +z. -z
    Chromaticities chromaticities;

    // Tile Header Attribute
    TileDesc    tiledesc;

    // Multi-View Header Attribute
    Text        view;

    // Multi-Part Data Header Attributes
    /*
    String      name;
    String      type;
    u32         version;
    u32         chunkCount;
    */

    // Deep Data Header Attributes
    u32         maxSamplesPerPixel;

    AttributeTable()
    {
        chromaticities = Chromaticities_BT709();
    }

    void parse(const std::string& name, const std::string& type, LittleEndianConstPointer p, u32 size)
    {
        MANGO_UNREFERENCED(type);
        MANGO_UNREFERENCED(size);

        if (name == "channels")
        {
            readAttribute(chlist, p);
        }
        else if (name == "tiles")
        {
            readAttribute(tiledesc, p);
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
            printLine(Print::Debug, "    Unknown attribute.");
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

    Buffer m_image_buffer;
    Surface m_surface;

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
    const u8* decompress_dwa(Memory dest, ConstMemory source, int width, int height, int ystart);

    void decodeBlock(Surface surface, ConstMemory memory, int x0, int y0, int x1, int y1);
    void decodeImage(const ImageDecodeOptions& options);

    ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face);

    void initLogTable()
    {
        for (u32 i = 0; i < 0x10000; ++i)
        {
            float16 hf;
            hf.u = u16(i);

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
        printLine(Print::Debug, "  decompress: {}.{} ms (cpu time)", m_time_decompress / 1000, m_time_decompress % 1000);
        printLine(Print::Debug, "  blit: {}.{} ms (cpu time)", m_time_blit / 1000, m_time_blit % 1000);
        printLine(Print::Debug, "  decode: {}.{} ms", m_time_decode / 1000, m_time_decode % 1000);
    }
};

ContextEXR::ContextEXR(ConstMemory memory)
    : m_memory(memory)
{
    printLine(Print::Debug, "Memory: {} KB", memory.size / 1024);

    const u8* end = memory.end();
    LittleEndianConstPointer p = memory.address;

    u32 magic = p.read32();
    if (magic != 0x01312f76)
    {
        m_header.setError("Incorrect format identifier: {:#010x}", magic);
        return;
    }

    u32 flags = p.read32();
    u32 version = flags & 0xff;

    is_single_tile = (flags & 0x0200) != 0;
    is_long_name   = (flags & 0x0400) != 0;
    is_deep_format = (flags & 0x0800) != 0;
    is_multi_part  = (flags & 0x1000) != 0;

    printLine(Print::Debug, "[Header]");
    printLine(Print::Debug, "  version: {}", version);
    printLine(Print::Debug, "  single_tile: {}\n"
                           "  long_name: {}\n"
                           "  deep_format: {}\n"
                           "  multi-part: {}\n",
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

    printLine(Print::Debug, "[Attributes]");

    for ( ; *p; )
    {
        std::string name = p.cast<const char>();
        p += name.length() + 1;

        std::string type = p.cast<const char>();
        p += type.length() + 1;

        u32 size = p.read32();
        printLine(Print::Debug, "  \"{}\", type: {}, size: {}", name, type, size);

        m_attributes.parse(name, type, p, size);
        p += size;

        if (p > end)
        {
            m_header.setError("Incorrect file: out of data.");
            return;
        }
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
            m_header.setError("Incorrect compression: {:#010x}", m_attributes.compression);
            return;
    }

    m_pointer = p;

    int width = m_attributes.dataWindow.xmax - m_attributes.dataWindow.xmin + 1;
    int height = m_attributes.dataWindow.ymax - m_attributes.dataWindow.ymin + 1;
    printLine(Print::Debug, "Image: {} x {}", width, height);

    // Reject implausible geometry from corrupt headers before it leads to a
    // huge allocation. The bounds are very generous (256M pixels) so that any
    // realistic image is accepted.
    if (width < 1 || height < 1 || width > (1 << 18) || height > (1 << 18) ||
        u64(width) * u64(height) > (u64(1) << 28))
    {
        m_header.setError("[ImageDecoder.EXR] Incorrect dimensions ({} x {}).", width, height);
        return;
    }

    bool isCubemap = (m_attributes.envmap == 2) && ((height % 6) == 0);
    if (isCubemap)
    {
        height = height / 6;
    }

    m_header.width   = width;
    m_header.height  = height;
    m_header.depth   = 0;
    m_header.levels  = 0;
    m_header.faces   = isCubemap ? 6 : 0;
    m_header.format  = Format(64, Format::FLOAT16, Format::RGBA, 16, 16, 16, 16, Format::LINEAR);
    m_header.linear  = true;
    m_header.compression = TextureCompression::NONE;

    // OpenEXR stores linear (radiometric) RGB. The chromaticities attribute defines the
    // primaries; it defaults to Rec.709 per the EXR specification when not present.
    {
        const Chromaticities& chroma = m_attributes.chromaticities;

        ColorInfo& color = m_header.color;
        color.transfer = TransferFunction::Linear;
        color.has_chromaticities = true;
        color.white = { chroma.white.x, chroma.white.y };
        color.red   = { chroma.red.x,   chroma.red.y   };
        color.green = { chroma.green.x, chroma.green.y };
        color.blue  = { chroma.blue.x,  chroma.blue.y  };
        color.primaries = identifyPrimaries(color.white, color.red, color.green, color.blue);
    }
}

ContextEXR::~ContextEXR()
{
}

const u8* ContextEXR::decompress_none(Memory dest, ConstMemory source)
{
    MANGO_UNREFERENCED(dest);
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
    //printLine(Print::Debug, "  out: {} bytes", status.size);

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

    u16 minNonZero = littleEndian::uload16(ptr + 0);
    u16 maxNonZero = littleEndian::uload16(ptr + 2);
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

    int length = littleEndian::uload32(ptr);
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

    for (const Channel& channel : channels)
    {
        int nx = div_ceil(width, channel.xsamples);
        int ny = div_ceil(height, channel.ysamples);
        int wcount = channel.bytes / 2;

        ChannelData cd;

        cd.nx = nx;
        cd.ny = ny;
        cd.wcount = wcount;
        cd.ysamples = channel.ysamples;
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
        for (const Channel& channel : channels)
        {
            size_t nBytes = width * channel.bytes;

            if (height == 0 || (channel.ysamples > 1 && (y % channel.ysamples) != 0))
                continue;

            if (out + nBytes > out_end)
            {
                printLine(Print::Error, "OUT OF MEMORY");
                return nullptr;
            }

            switch (channel.datatype)
            {
                case DataType::UINT:
                {
                    const u8* ptr = lastIn;
                    lastIn += nBytes;

                    u32 pixel = 0;

                    if (lastIn > lastIn_end)
                    {
                        printLine(Print::Error, "CORRUPT CHUNK");
                        return nullptr;
                    }

                    for (int x = 0; x < width; ++x)
                    {
                        u32 diff = ((u32(ptr[x + width * 0]) << 24) |
                                    (u32(ptr[x + width * 1]) << 16) |
                                    (u32(ptr[x + width * 2]) <<  8) |
                                    (u32(ptr[x + width * 3]) <<  0));
                        pixel += diff;
                        ustore32(out + x * 4, pixel);
                    }
                    break;
                }

                case DataType::HALF:
                {
                    const u8* ptr = lastIn;
                    lastIn += nBytes;

                    u32 pixel = 0;

                    if (lastIn > lastIn_end)
                    {
                        printLine(Print::Error, "CORRUPT CHUNK");
                        return nullptr;
                    }

                    for (int x = 0; x < width; ++x)
                    {
                        u32 diff = ((u32(ptr[x + width * 0]) << 8) |
                                    (u32(ptr[x + width * 1]) << 0));
                        pixel += diff;
                        ustore16(out + x * 2, u16(pixel));
                    }
                    break;
                }

                case DataType::FLOAT:
                {
                    const u8* ptr = lastIn;
                    lastIn += width * 3; // 24 bits per sample

                    u32 pixel = 0;

                    if (lastIn > lastIn_end)
                    {
                        printLine(Print::Error, "CORRUPT CHUNK");
                        return nullptr;
                    }

                    for (int x = 0; x < width; ++x)
                    {
                        u32 diff = ((u32(ptr[x + width * 0]) << 24) |
                                    (u32(ptr[x + width * 1]) << 16) |
                                    (u32(ptr[x + width * 2]) <<  8));
                        pixel += diff;
                        ustore32(out + x * 4, pixel);
                    }
                    break;
                }

                case DataType::NONE:
                    break;
            }

            out += nBytes;
        }
    }

    return dest;
}

const u8* ContextEXR::decompress_b44(Memory dest, ConstMemory source, int width, int height, int ystart)
{
    const std::vector<Channel>& channels = m_attributes.chlist.channels;

    const int block_width = width;
    const int block_height = std::max(height, m_scanLinesPerBlock);

    // The unpack loops below fill the per-channel scratch for the full block_height
    // (rounded up to 4x4 B44 cells), which can exceed dest.size for the last, partial
    // scanline block (height < m_scanLinesPerBlock). Size the scratch to exactly what
    // those loops write; the second pass copies only `height` rows back into dest.
    // (Sizing temp to dest.size here overran the heap by up to one 4x4 cell row.)
    size_t scratch_size = 0;
    for (const Channel& channel : channels)
    {
        int nx = div_ceil(block_width, channel.xsamples);
        int ny = div_ceil(block_height, channel.ysamples);
        scratch_size += size_t(ny) * nx * channel.bytes;
    }

    Buffer temp(scratch_size);

    u8* scratch = temp;
    const u8* in = source.address;

    const u8* source_end = source.end();
    const u8* dest_end = dest.end();

    for (const Channel& channel : channels)
    {
        int nx = div_ceil(block_width, channel.xsamples);
        int ny = div_ceil(block_height, channel.ysamples);
        int nBytes = ny * nx * channel.bytes;

        if (nBytes == 0)
            continue;

        if (channel.datatype != DataType::HALF)
        {
            if (in + nBytes > source_end)
            {
                // out of memory
                return nullptr;
            }

            std::memcpy(scratch, in, nBytes);
            in += nBytes;
            scratch += nBytes;
            continue;
        }

        for (int y = 0; y < ny; y += 4)
        {
            u16* row0 = reinterpret_cast<u16*>(scratch) + y * nx;
            u16* row1 = row0 + nx;
            u16* row2 = row1 + nx;
            u16* row3 = row2 + nx;

            //printLine(Print::Debug, "({},{}) [{} x {}]", 0, y, nx, ny);
            for (int x = 0; x < nx; x += 4)
            {
                if (in + 3 > source_end)
                {
                    // out of memory
                    return nullptr;
                }

                u16 s[16];

                // check if 3-byte encoded flat field
                if (in[2] >= (13 << 2))
                {
                    unpack3(in, s);
                    in += 3;
                }
                else
                {
                    if (in + 14 > source_end)
                    {
                        // out of memory
                        return nullptr;
                    }

                    unpack14(in, s);
                    in += 14;
                }

                if (channel.linear)
                    convertToLinear(s);

                size_t n = (x + 3 < nx) ? 4 * sizeof(u16) : size_t(nx - x) * sizeof(u16);
                if (y + 3 < ny)
                {
                    std::memcpy(row0, s + 0, n);
                    std::memcpy(row1, s + 4, n);
                    std::memcpy(row2, s + 8, n);
                    std::memcpy(row3, s + 12, n);
                }
                else
                {
                    std::memcpy(row0, s + 0, n);
                    if (y + 1 < ny) std::memcpy(row1, s + 4, n);
                    if (y + 2 < ny) std::memcpy(row2, s + 8, n);
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
    u8* out = dest.address;

    for (int y = 0; y < height; ++y)
    {
        int cury = y + ystart;

        scratch = temp;

        for (const Channel& channel : channels)
        {
            int nx = div_ceil(block_width, channel.xsamples);
            int ny = div_ceil(block_height, channel.ysamples);
            int bpl = nx * channel.bytes;
            int nBytes = ny * bpl;

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
                tmp += u64(y / channel.ysamples) * bpl;
            }
            else
            {
                tmp += u64(y) * bpl;
            }

            if (out + bpl > dest_end)
            {
                // out of memory
                return nullptr;
            }

            std::memcpy(out, tmp, bpl);

            out += bpl;
            scratch += nBytes;
        }
    }

    return dest;
}

// ----------------------------------------------------------------------------
// DWAA / DWAB decompression
// ----------------------------------------------------------------------------
//
// DWA is OpenEXR's lossy DCT codec. A compressed block is a small header of
// counters followed by up to four sub-streams:
//
//   - UNKNOWN : channels we don't know how to lossy-compress (zlib only)
//   - AC      : quantized DCT AC coefficients (RLE + static Huffman or zlib)
//   - DC      : DCT DC coefficients (zlib, with the standard EXR zip transform)
//   - RLE     : losslessly stored channels (byte-split + RLE + zlib)
//
// Channels are classified by name/type into LOSSY_DCT, RLE or UNKNOWN. R,G,B
// triples from the same layer are decorrelated with a Y'CbCr (709) transform
// before the DCT, so they are decoded together. The half-float coefficients
// live in a perceptually-uniform non-linear space; a 64KiB lookup table maps
// them back to the linear half values stored in the output.
//
// This is a faithful (scalar) port of OpenEXR's DwaCompressor decode path and
// assumes a little-endian host (as does the rest of this EXR decoder).

namespace
{

    enum : int
    {
        DWA_VERSION = 0,
        DWA_UNKNOWN_UNCOMPRESSED_SIZE,
        DWA_UNKNOWN_COMPRESSED_SIZE,
        DWA_AC_COMPRESSED_SIZE,
        DWA_DC_COMPRESSED_SIZE,
        DWA_RLE_COMPRESSED_SIZE,
        DWA_RLE_UNCOMPRESSED_SIZE,
        DWA_RLE_RAW_SIZE,
        DWA_AC_UNCOMPRESSED_COUNT,
        DWA_DC_UNCOMPRESSED_COUNT,
        DWA_AC_COMPRESSION,
        DWA_NUM_SIZES_SINGLE
    };

    enum DwaScheme
    {
        DWA_UNKNOWN_SCHEME = 0,
        DWA_LOSSY_DCT      = 1,
        DWA_RLE            = 2,
        DWA_NUM_SCHEMES    = 3
    };

    enum DwaAcCompression
    {
        DWA_STATIC_HUFFMAN = 0,
        DWA_DEFLATE        = 1
    };

    inline int dwaPixelTypeSize(DataType type)
    {
        switch (type)
        {
            case DataType::UINT:  return 4;
            case DataType::HALF:  return 2;
            case DataType::FLOAT: return 4;
            default:              return 0;
        }
    }

    // The non-linear -> linear LUT. Built once; identical to the table generated
    // by OpenEXR's dwaLookups (Xdr is little-endian, so no byte swap is needed).
    const u16* dwaToLinearTable()
    {
        static const std::vector<u16> table = []
        {
            std::vector<u16> t(65536);
            t[0] = 0;
            const float logBase = float(std::pow(2.7182818, 2.2));
            for (int i = 1; i < 65536; ++i)
            {
                if ((i & 0x7c00) == 0x7c00)
                {
                    // NaN / Inf map to 0
                    t[i] = 0;
                    continue;
                }

                float h = float(Half(u16(i)));
                float sign = h < 0.0f ? -1.0f : 1.0f;
                float a = std::fabs(h);

                float r = (a <= 1.0f)
                    ? sign * std::pow(a, 2.2f)
                    : sign * std::pow(logBase, a - 1.0f);

                t[i] = Half(r).u;
            }
            return t;
        }();
        return table.data();
    }

    // Inverse 709 Y'CbCr -> R'G'B', single element and full 8x8 block.
    inline void dwaCsc709Inverse(float& comp0, float& comp1, float& comp2)
    {
        float y  = comp0;
        float cb = comp1;
        float cr = comp2;

        comp0 = y + 1.5747f * cr;
        comp1 = y - 0.1873f * cb - 0.4682f * cr;
        comp2 = y + 1.8556f * cb;
    }

    inline void dwaCsc709Inverse64(float* c0, float* c1, float* c2)
    {
        for (int i = 0; i < 64; ++i)
            dwaCsc709Inverse(c0[i], c1[i], c2[i]);
    }

    // Inverse 8x8 DCT, constant-block (DC only) fast path.
    inline void dwaDctInverse8x8DcOnly(float* data)
    {
        float val = data[0] * 3.535536e-01f * 3.535536e-01f;
        for (int i = 0; i < 64; ++i)
            data[i] = val;
    }

    // Full inverse 8x8 DCT, in-place. 'zeroedRows' is the number of trailing
    // all-zero rows, which lets us skip work in the row pass.
    void dwaDctInverse8x8(float* data, int zeroedRows)
    {
        static const float a = 0.5f * std::cos(3.14159f / 4.0f);
        static const float b = 0.5f * std::cos(3.14159f / 16.0f);
        static const float c = 0.5f * std::cos(3.14159f / 8.0f);
        static const float d = 0.5f * std::cos(3.0f * 3.14159f / 16.0f);
        static const float e = 0.5f * std::cos(5.0f * 3.14159f / 16.0f);
        static const float f = 0.5f * std::cos(3.0f * 3.14159f / 8.0f);
        static const float g = 0.5f * std::cos(7.0f * 3.14159f / 16.0f);

        float alpha[4], beta[4], theta[4], gamma[4];

        // First pass - rows
        for (int row = 0; row < 8 - zeroedRows; ++row)
        {
            float* rowPtr = data + row * 8;

            alpha[0] = c * rowPtr[2];
            alpha[1] = f * rowPtr[2];
            alpha[2] = c * rowPtr[6];
            alpha[3] = f * rowPtr[6];

            beta[0] = b * rowPtr[1] + d * rowPtr[3] + e * rowPtr[5] + g * rowPtr[7];
            beta[1] = d * rowPtr[1] - g * rowPtr[3] - b * rowPtr[5] - e * rowPtr[7];
            beta[2] = e * rowPtr[1] - b * rowPtr[3] + g * rowPtr[5] + d * rowPtr[7];
            beta[3] = g * rowPtr[1] - e * rowPtr[3] + d * rowPtr[5] - b * rowPtr[7];

            theta[0] = a * (rowPtr[0] + rowPtr[4]);
            theta[3] = a * (rowPtr[0] - rowPtr[4]);
            theta[1] = alpha[0] + alpha[3];
            theta[2] = alpha[1] - alpha[2];

            gamma[0] = theta[0] + theta[1];
            gamma[1] = theta[3] + theta[2];
            gamma[2] = theta[3] - theta[2];
            gamma[3] = theta[0] - theta[1];

            rowPtr[0] = gamma[0] + beta[0];
            rowPtr[1] = gamma[1] + beta[1];
            rowPtr[2] = gamma[2] + beta[2];
            rowPtr[3] = gamma[3] + beta[3];
            rowPtr[4] = gamma[3] - beta[3];
            rowPtr[5] = gamma[2] - beta[2];
            rowPtr[6] = gamma[1] - beta[1];
            rowPtr[7] = gamma[0] - beta[0];
        }

        // Second pass - columns
        for (int column = 0; column < 8; ++column)
        {
            alpha[0] = c * data[16 + column];
            alpha[1] = f * data[16 + column];
            alpha[2] = c * data[48 + column];
            alpha[3] = f * data[48 + column];

            beta[0] = b * data[8 + column] + d * data[24 + column] +
                      e * data[40 + column] + g * data[56 + column];
            beta[1] = d * data[8 + column] - g * data[24 + column] -
                      b * data[40 + column] - e * data[56 + column];
            beta[2] = e * data[8 + column] - b * data[24 + column] +
                      g * data[40 + column] + d * data[56 + column];
            beta[3] = g * data[8 + column] - e * data[24 + column] +
                      d * data[40 + column] - b * data[56 + column];

            theta[0] = a * (data[column] + data[32 + column]);
            theta[3] = a * (data[column] - data[32 + column]);
            theta[1] = alpha[0] + alpha[3];
            theta[2] = alpha[1] - alpha[2];

            gamma[0] = theta[0] + theta[1];
            gamma[1] = theta[3] + theta[2];
            gamma[2] = theta[3] - theta[2];
            gamma[3] = theta[0] - theta[1];

            data[column]      = gamma[0] + beta[0];
            data[8 + column]  = gamma[1] + beta[1];
            data[16 + column] = gamma[2] + beta[2];
            data[24 + column] = gamma[3] + beta[3];
            data[32 + column] = gamma[3] - beta[3];
            data[40 + column] = gamma[2] - beta[2];
            data[48 + column] = gamma[1] - beta[1];
            data[56 + column] = gamma[0] - beta[0];
        }
    }

    // Un-zig-zag a block of 64 half-float coefficients into row-major floats.
    void dwaFromHalfZigZag(const u16* src, float* dst)
    {
        static const int zigzag[64] =
        {
             0,  1,  5,  6, 14, 15, 27, 28,
             2,  4,  7, 13, 16, 26, 29, 42,
             3,  8, 12, 17, 25, 30, 41, 43,
             9, 11, 18, 24, 31, 40, 44, 53,
            10, 19, 23, 32, 39, 45, 52, 54,
            20, 22, 33, 38, 46, 51, 55, 60,
            21, 34, 37, 47, 50, 56, 59, 61,
            35, 36, 48, 49, 57, 58, 62, 63
        };

        for (int i = 0; i < 64; ++i)
            dst[i] = float(Half(src[zigzag[i]]));
    }

    // Un-RLE the packed AC coefficients of one 8x8 block into 'halfZig' (which
    // must be zeroed). High byte 0xff signals a run of zeros; 0xff00 is "end of
    // block". Returns the zig-zag index of the last non-zero coefficient, or -1
    // if the AC stream is exhausted (corrupt data).
    int dwaUnRleAc(const u16*& cur, const u16* end, u16* halfZig)
    {
        int lastNonZero = 0;
        int dctComp = 1;

        while (dctComp < 64)
        {
            if (cur >= end)
                return -1;

            u16 value = *cur;

            if (value == 0xff00)
            {
                // end of block
                dctComp = 64;
            }
            else if ((value >> 8) == 0xff)
            {
                // run of zeros (block already zeroed)
                dctComp += value & 0xff;
            }
            else
            {
                lastNonZero = dctComp;
                if (dctComp < 64)
                    halfZig[dctComp] = value;
                dctComp++;
            }

            ++cur;
        }

        return lastNonZero;
    }

    // Expand EXR-style RLE (signed run-length) from 'in' into 'out'. Returns the
    // number of bytes written, or -1 on overflow / truncation.
    int dwaRleUncompress(int inLength, int maxLength, const s8* in, u8* out)
    {
        u8* start = out;

        while (inLength > 0)
        {
            if (*in < 0)
            {
                int count = -int(*in++);
                inLength -= count + 1;
                if ((maxLength -= count) < 0 || inLength < 0)
                    return -1;
                std::memcpy(out, in, count);
                out += count;
                in += count;
            }
            else
            {
                int count = int(*in++);
                inLength -= 2;
                if ((maxLength -= count + 1) < 0 || inLength < 0)
                    return -1;
                std::memset(out, *reinterpret_cast<const u8*>(in), count + 1);
                out += count + 1;
                ++in;
            }
        }

        return int(out - start);
    }

    // Decode a group of 1 or 3 LOSSY_DCT channels. The AC and DC cursors are
    // advanced as coefficients are consumed. Returns false on corrupt input.
    bool dwaDctDecodeGroup(const std::vector<u8*>* const rows[3], int numComp,
                           const u16*& acCur, const u16* acEnd, const u16*& dcCur,
                           const u16* const toLinear[3], const DataType type[3],
                           int width, int height)
    {
        const int numBlocksX = (width  + 7) / 8;
        const int numBlocksY = (height + 7) / 8;
        const int numFullBlocksX = width / 8;
        const int leftoverX = width  - (numBlocksX - 1) * 8;
        const int leftoverY = height - (numBlocksY - 1) * 8;

        std::vector<u16> rowBlock(size_t(numComp) * numBlocksX * 64);
        u16* rowBlockComp[3] = { nullptr, nullptr, nullptr };
        for (int comp = 0; comp < numComp; ++comp)
            rowBlockComp[comp] = rowBlock.data() + size_t(comp) * numBlocksX * 64;

        // DC values are grouped by component plane.
        const u16* dc[3] = { nullptr, nullptr, nullptr };
        for (int comp = 0; comp < numComp; ++comp)
            dc[comp] = dcCur + size_t(comp) * numBlocksX * numBlocksY;

        u16 halfZig[3][64];
        float dctData[3][64];

        for (int blocky = 0; blocky < numBlocksY; ++blocky)
        {
            const int maxY = (blocky == numBlocksY - 1) ? leftoverY : 8;

            for (int blockx = 0; blockx < numBlocksX; ++blockx)
            {
                const int maxX = (blockx == numBlocksX - 1) ? leftoverX : 8;

                bool blockIsConstant = true;

                for (int comp = 0; comp < numComp; ++comp)
                {
                    std::memset(halfZig[comp], 0, sizeof(halfZig[comp]));
                    halfZig[comp][0] = *dc[comp]++;

                    int lastNonZero = dwaUnRleAc(acCur, acEnd, halfZig[comp]);
                    if (lastNonZero < 0)
                        return false;

                    if (lastNonZero == 0)
                    {
                        // DC only
                        dctData[comp][0] = float(Half(halfZig[comp][0]));
                        dwaDctInverse8x8DcOnly(dctData[comp]);
                    }
                    else
                    {
                        blockIsConstant = false;
                        dwaFromHalfZigZag(halfZig[comp], dctData[comp]);

                        int zeroedRows = 0;
                        if (lastNonZero < 2)       zeroedRows = 7;
                        else if (lastNonZero < 3)  zeroedRows = 6;
                        else if (lastNonZero < 9)  zeroedRows = 5;
                        else if (lastNonZero < 10) zeroedRows = 4;
                        else if (lastNonZero < 20) zeroedRows = 3;
                        else if (lastNonZero < 21) zeroedRows = 2;
                        else if (lastNonZero < 35) zeroedRows = 1;
                        else                       zeroedRows = 0;

                        dwaDctInverse8x8(dctData[comp], zeroedRows);
                    }
                }

                if (numComp == 3)
                {
                    if (!blockIsConstant)
                        dwaCsc709Inverse64(dctData[0], dctData[1], dctData[2]);
                    else
                        dwaCsc709Inverse(dctData[0][0], dctData[1][0], dctData[2][0]);
                }

                for (int comp = 0; comp < numComp; ++comp)
                {
                    u16* dst = &rowBlockComp[comp][blockx * 64];
                    if (!blockIsConstant)
                    {
                        for (int i = 0; i < 64; ++i)
                            dst[i] = Half(dctData[comp][i]).u;
                    }
                    else
                    {
                        u16 v = Half(dctData[comp][0]).u;
                        for (int i = 0; i < 64; ++i)
                            dst[i] = v;
                    }
                }
            }

            // Un-block this row of blocks into the output, mapping back to linear.
            for (int comp = 0; comp < numComp; ++comp)
            {
                const u16* lut = toLinear[comp];

                for (int y = 8 * blocky; y < 8 * blocky + maxY; ++y)
                {
                    u16* dst = reinterpret_cast<u16*>((*rows[comp])[y]);
                    const int localRow = (y & 7) * 8;

                    for (int bx = 0; bx < numFullBlocksX; ++bx)
                    {
                        const u16* src = &rowBlockComp[comp][bx * 64 + localRow];
                        if (lut)
                        {
                            for (int k = 0; k < 8; ++k)
                                dst[k] = lut[src[k]];
                        }
                        else
                        {
                            for (int k = 0; k < 8; ++k)
                                dst[k] = src[k];
                        }
                        dst += 8;
                    }

                    if (numFullBlocksX != numBlocksX)
                    {
                        const u16* src = &rowBlockComp[comp][numFullBlocksX * 64 + localRow];
                        for (int x = 0; x < leftoverX; ++x)
                            dst[x] = lut ? lut[src[x]] : src[x];
                    }
                }
            }
        }

        dcCur += size_t(numComp) * numBlocksX * numBlocksY;

        // FLOAT channels: the data was decoded as half values packed at the
        // start of each (float-sized) row; expand them in place to float.
        for (int comp = 0; comp < numComp; ++comp)
        {
            if (type[comp] != DataType::FLOAT)
                continue;

            std::vector<u16> halfRow(width);
            for (int y = 0; y < height; ++y)
            {
                u8* row = (*rows[comp])[y];
                std::memcpy(halfRow.data(), row, size_t(width) * sizeof(u16));
                float* dst = reinterpret_cast<float*>(row);
                for (int x = 0; x < width; ++x)
                    dst[x] = float(Half(halfRow[x]));
            }
        }

        return true;
    }

    // Channel classification rule.
    struct DwaRule
    {
        std::string suffix;
        int scheme = DWA_UNKNOWN_SCHEME;
        DataType type = DataType::NONE;
        int cscIdx = -1;
        bool caseInsensitive = false;
    };

    inline std::string dwaToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [] (unsigned char c) { return char(std::tolower(c)); });
        return s;
    }

    bool dwaRuleMatch(const DwaRule& rule, const std::string& suffix, DataType type)
    {
        if (rule.type != type)
            return false;
        if (rule.caseInsensitive)
            return dwaToLower(suffix) == rule.suffix;
        return suffix == rule.suffix;
    }

    void dwaInitLegacyRules(std::vector<DwaRule>& rules)
    {
        auto add = [&] (const char* s, int sc, DataType t, int csc, bool ci)
        {
            DwaRule r;
            r.suffix = ci ? dwaToLower(s) : s;
            r.scheme = sc;
            r.type = t;
            r.cscIdx = csc;
            r.caseInsensitive = ci;
            rules.push_back(r);
        };

        add("r",     DWA_LOSSY_DCT, DataType::HALF,  0, true);
        add("r",     DWA_LOSSY_DCT, DataType::FLOAT, 0, true);
        add("red",   DWA_LOSSY_DCT, DataType::HALF,  0, true);
        add("red",   DWA_LOSSY_DCT, DataType::FLOAT, 0, true);
        add("g",     DWA_LOSSY_DCT, DataType::HALF,  1, true);
        add("g",     DWA_LOSSY_DCT, DataType::FLOAT, 1, true);
        add("grn",   DWA_LOSSY_DCT, DataType::HALF,  1, true);
        add("grn",   DWA_LOSSY_DCT, DataType::FLOAT, 1, true);
        add("green", DWA_LOSSY_DCT, DataType::HALF,  1, true);
        add("green", DWA_LOSSY_DCT, DataType::FLOAT, 1, true);
        add("b",     DWA_LOSSY_DCT, DataType::HALF,  2, true);
        add("b",     DWA_LOSSY_DCT, DataType::FLOAT, 2, true);
        add("blu",   DWA_LOSSY_DCT, DataType::HALF,  2, true);
        add("blu",   DWA_LOSSY_DCT, DataType::FLOAT, 2, true);
        add("blue",  DWA_LOSSY_DCT, DataType::HALF,  2, true);
        add("blue",  DWA_LOSSY_DCT, DataType::FLOAT, 2, true);

        add("y",     DWA_LOSSY_DCT, DataType::HALF,  -1, true);
        add("y",     DWA_LOSSY_DCT, DataType::FLOAT, -1, true);
        add("by",    DWA_LOSSY_DCT, DataType::HALF,  -1, true);
        add("by",    DWA_LOSSY_DCT, DataType::FLOAT, -1, true);
        add("ry",    DWA_LOSSY_DCT, DataType::HALF,  -1, true);
        add("ry",    DWA_LOSSY_DCT, DataType::FLOAT, -1, true);
        add("a",     DWA_RLE,       DataType::UINT,  -1, true);
        add("a",     DWA_RLE,       DataType::HALF,  -1, true);
        add("a",     DWA_RLE,       DataType::FLOAT, -1, true);
    }

} // namespace

const u8* ContextEXR::decompress_dwa(Memory dest, ConstMemory source, int width, int height, int ystart)
{
    MANGO_UNREFERENCED(ystart);

    const std::vector<Channel>& channels = m_attributes.chlist.channels;
    const size_t numChan = channels.size();
    if (numChan == 0)
        return nullptr;

    const u8* inPtr = source.address;
    const u64 inSize = source.size;

    const u64 headerBytes = u64(DWA_NUM_SIZES_SINGLE) * sizeof(u64);
    if (inSize < headerBytes)
        return nullptr;

    // Header counters (little-endian, matching EXR Xdr).
    u64 counter[DWA_NUM_SIZES_SINGLE];
    for (int i = 0; i < DWA_NUM_SIZES_SINGLE; ++i)
        counter[i] = littleEndian::uload64(inPtr + i * sizeof(u64));

    const u64 version                 = counter[DWA_VERSION];
    const u64 unknownUncompressedSize = counter[DWA_UNKNOWN_UNCOMPRESSED_SIZE];
    const u64 unknownCompressedSize   = counter[DWA_UNKNOWN_COMPRESSED_SIZE];
    const u64 acCompressedSize        = counter[DWA_AC_COMPRESSED_SIZE];
    const u64 dcCompressedSize        = counter[DWA_DC_COMPRESSED_SIZE];
    const u64 rleCompressedSize       = counter[DWA_RLE_COMPRESSED_SIZE];
    const u64 rleUncompressedSize     = counter[DWA_RLE_UNCOMPRESSED_SIZE];
    const u64 rleRawSize              = counter[DWA_RLE_RAW_SIZE];
    const u64 totalAcUncompressedCount = counter[DWA_AC_UNCOMPRESSED_COUNT];
    const u64 totalDcUncompressedCount = counter[DWA_DC_UNCOMPRESSED_COUNT];
    const u64 acCompression           = counter[DWA_AC_COMPRESSION];

    const u64 compressedSize = unknownCompressedSize + acCompressedSize +
                               dcCompressedSize + rleCompressedSize;

    if (version > 2)
        return nullptr;

    const u8* dataPtr = inPtr + headerBytes;
    u64 dataAvailable = inSize - headerBytes;

    // Channel classification rules.
    std::vector<DwaRule> rules;

    if (version < 2)
    {
        dwaInitLegacyRules(rules);
    }
    else
    {
        // The rules are embedded at the start of the data block.
        if (dataAvailable < sizeof(u16))
            return nullptr;

        u16 ruleSize = littleEndian::uload16(dataPtr);
        if (ruleSize < sizeof(u16) || ruleSize > dataAvailable)
            return nullptr;

        const u8* p = dataPtr + sizeof(u16);
        const u8* ruleEnd = dataPtr + ruleSize;

        while (p < ruleEnd)
        {
            DwaRule rule;

            // null-terminated suffix
            const u8* z = p;
            while (z < ruleEnd && *z)
                ++z;
            if (z >= ruleEnd)
                return nullptr;
            rule.suffix.assign(reinterpret_cast<const char*>(p), z - p);
            p = z + 1;

            if (p + 2 > ruleEnd)
                return nullptr;

            u8 value = *p++;
            rule.cscIdx = int(value >> 4) - 1;
            rule.scheme = (value >> 2) & 3;
            rule.caseInsensitive = (value & 1) != 0;

            u8 typeValue = *p++;
            if (rule.cscIdx < -1 || rule.cscIdx >= 3 ||
                rule.scheme < 0 || rule.scheme >= DWA_NUM_SCHEMES ||
                typeValue > 2)
            {
                return nullptr;
            }
            rule.type = DataType(typeValue);

            if (rule.caseInsensitive)
                rule.suffix = dwaToLower(rule.suffix);

            rules.push_back(rule);
        }

        dataPtr += ruleSize;
        dataAvailable -= ruleSize;
    }

    if (compressedSize > dataAvailable)
        return nullptr;

    // Per-channel decode state.
    struct DwaChannel
    {
        int scheme = DWA_UNKNOWN_SCHEME;
        int width = 0;
        int height = 0;
        int cscIdx = -1;
        size_t planarBase = 0; // base offset within RLE or UNKNOWN planar buffer
    };

    std::vector<DwaChannel> cd(numChan);

    // Classify channels and gather CSC (RGB) groups by layer prefix.
    struct CscGroup
    {
        std::string prefix;
        int idx[3] = { -1, -1, -1 };
    };
    std::vector<CscGroup> groups;

    for (size_t i = 0; i < numChan; ++i)
    {
        const std::string& name = channels[i].name;
        DataType type = channels[i].datatype;

        std::string prefix;
        std::string suffix = name;
        size_t dot = name.find_last_of('.');
        if (dot != std::string::npos)
        {
            prefix = name.substr(0, dot);
            suffix = name.substr(dot + 1);
        }

        cd[i].scheme = DWA_UNKNOWN_SCHEME;
        cd[i].cscIdx = -1;

        for (const DwaRule& rule : rules)
        {
            if (dwaRuleMatch(rule, suffix, type))
            {
                cd[i].scheme = rule.scheme;
                if (rule.cscIdx >= 0)
                {
                    cd[i].cscIdx = rule.cscIdx;

                    CscGroup* group = nullptr;
                    for (CscGroup& g : groups)
                        if (g.prefix == prefix) { group = &g; break; }
                    if (!group)
                    {
                        groups.push_back(CscGroup());
                        groups.back().prefix = prefix;
                        group = &groups.back();
                    }
                    group->idx[rule.cscIdx] = int(i);
                }
            }
        }
    }

    // Keep group ordering deterministic (sorted by prefix), matching the
    // encoder which builds CSC sets from a std::map.
    std::sort(groups.begin(), groups.end(),
              [] (const CscGroup& a, const CscGroup& b) { return a.prefix < b.prefix; });

    std::vector<CscGroup> cscSets;
    for (const CscGroup& g : groups)
    {
        int r = g.idx[0];
        int gg = g.idx[1];
        int b = g.idx[2];
        if (r < 0 || gg < 0 || b < 0)
            continue;

        if (channels[r].xsamples != channels[gg].xsamples ||
            channels[r].xsamples != channels[b].xsamples ||
            channels[r].ysamples != channels[gg].ysamples ||
            channels[r].ysamples != channels[b].ysamples)
        {
            continue;
        }

        cscSets.push_back(g);
    }

    // Per-channel sizes and planar buffer layout.
    size_t unknownSize = 0;
    size_t rleSize = 0;

    // Upper bounds (from geometry) on the number of DCT coefficients; used to
    // reject corrupt headers before allocating buffers from attacker-controlled
    // counts. Each 8x8 block contributes at most 64 AC symbols and 1 DC value.
    u64 acBound = 0;
    u64 dcBound = 0;

    for (size_t i = 0; i < numChan; ++i)
    {
        cd[i].width  = div_ceil(width, channels[i].xsamples);
        cd[i].height = div_ceil(height, channels[i].ysamples);

        if (cd[i].scheme == DWA_LOSSY_DCT)
        {
            const u64 blocks = u64((cd[i].width + 7) / 8) * ((cd[i].height + 7) / 8);
            acBound += blocks * 64;
            dcBound += blocks;
            continue;
        }

        const size_t bytes = size_t(cd[i].width) * cd[i].height * dwaPixelTypeSize(channels[i].datatype);

        if (cd[i].scheme == DWA_RLE)
        {
            cd[i].planarBase = rleSize;
            rleSize += bytes;
        }
        else // UNKNOWN
        {
            cd[i].planarBase = unknownSize;
            unknownSize += bytes;
        }
    }

    // Sanity-check all uncompressed sizes against the geometry before we trust
    // them for allocation. This keeps corrupt files from requesting gigabytes.
    if (totalAcUncompressedCount > acBound ||
        totalDcUncompressedCount > dcBound ||
        (unknownCompressedSize > 0 && unknownUncompressedSize != unknownSize) ||
        (rleRawSize > 0 && rleRawSize != rleSize) ||
        rleUncompressedSize > u64(rleRawSize) * 2 + 64)
    {
        return nullptr;
    }

    const u8* compressedUnknown = dataPtr;
    const u8* compressedAc      = compressedUnknown + unknownCompressedSize;
    const u8* compressedDc      = compressedAc + acCompressedSize;
    const u8* compressedRle     = compressedDc + dcCompressedSize;

    // Decompress sub-streams.

    std::vector<u8> unknownBuffer;
    if (unknownCompressedSize > 0)
    {
        if (unknownUncompressedSize != unknownSize)
            return nullptr;
        unknownBuffer.resize(unknownSize);
        CompressionStatus s = deflate_zlib::decompress(
            Memory(unknownBuffer.data(), unknownBuffer.size()),
            ConstMemory(compressedUnknown, unknownCompressedSize));
        if (!s || s.size != unknownSize)
            return nullptr;
    }

    std::vector<u16> acBuffer;
    if (acCompressedSize > 0)
    {
        acBuffer.resize(totalAcUncompressedCount);

        if (acCompression == DWA_STATIC_HUFFMAN)
        {
            if (!hufUncompress(compressedAc, int(acCompressedSize), acBuffer))
                return nullptr;
        }
        else if (acCompression == DWA_DEFLATE)
        {
            CompressionStatus s = deflate_zlib::decompress(
                Memory(reinterpret_cast<u8*>(acBuffer.data()), acBuffer.size() * sizeof(u16)),
                ConstMemory(compressedAc, acCompressedSize));
            if (!s || s.size != acBuffer.size() * sizeof(u16))
                return nullptr;
        }
        else
        {
            return nullptr;
        }
    }

    std::vector<u16> dcBuffer;
    if (dcCompressedSize > 0)
    {
        const size_t dcBytes = totalDcUncompressedCount * sizeof(u16);
        Buffer temp(dcBytes);
        CompressionStatus s = deflate_zlib::decompress(
            Memory(temp.data(), temp.size()),
            ConstMemory(compressedDc, dcCompressedSize));
        if (!s || s.size != dcBytes)
            return nullptr;

        // DC values use the standard EXR zip transform (predictor + reorder).
        predictor(temp, dcBytes);
        dcBuffer.resize(totalDcUncompressedCount);
        deinterleave(reinterpret_cast<u8*>(dcBuffer.data()), temp, dcBytes);
    }
    else if (totalDcUncompressedCount != 0)
    {
        return nullptr;
    }

    std::vector<u8> rleBuffer;
    if (rleRawSize > 0)
    {
        if (rleRawSize != rleSize)
            return nullptr;

        Buffer temp(rleUncompressedSize);
        CompressionStatus s = deflate_zlib::decompress(
            Memory(temp.data(), temp.size()),
            ConstMemory(compressedRle, rleCompressedSize));
        if (!s || s.size != rleUncompressedSize)
            return nullptr;

        rleBuffer.resize(rleSize);
        int n = dwaRleUncompress(int(rleUncompressedSize), int(rleRawSize),
                                 reinterpret_cast<const s8*>(temp.data()), rleBuffer.data());
        if (n != int(rleRawSize))
            return nullptr;
    }

    // Compute the row pointers into 'dest' (per scanline, per channel).
    std::vector<std::vector<u8*>> rowPtrs(numChan);
    {
        u8* cursor = dest.address;
        u8* destEnd = dest.address + dest.size;

        for (int y = 0; y < height; ++y)
        {
            for (size_t i = 0; i < numChan; ++i)
            {
                if (channels[i].ysamples > 1 && (y % channels[i].ysamples) != 0)
                    continue;

                rowPtrs[i].push_back(cursor);
                cursor += size_t(cd[i].width) * channels[i].bytes;
                if (cursor > destEnd)
                    return nullptr;
            }
        }
    }

    const u16* toLinearTable = dwaToLinearTable();
    const u16* acCur = acBuffer.empty() ? nullptr : acBuffer.data();
    const u16* acEnd = acBuffer.empty() ? nullptr : acBuffer.data() + acBuffer.size();
    const u16* dcCur = dcBuffer.empty() ? nullptr : dcBuffer.data();

    std::vector<bool> decoded(numChan, false);

    // CSC (RGB) groups first.
    for (const CscGroup& set : cscSets)
    {
        int idx[3] = { set.idx[0], set.idx[1], set.idx[2] };

        for (int k = 0; k < 3; ++k)
            if (cd[idx[k]].scheme != DWA_LOSSY_DCT)
                return nullptr;

        const std::vector<u8*>* rows[3] =
        {
            &rowPtrs[idx[0]], &rowPtrs[idx[1]], &rowPtrs[idx[2]]
        };
        const u16* toLinear[3] = { toLinearTable, toLinearTable, toLinearTable };
        DataType type[3] =
        {
            channels[idx[0]].datatype, channels[idx[1]].datatype, channels[idx[2]].datatype
        };

        if (!dwaDctDecodeGroup(rows, 3, acCur, acEnd, dcCur, toLinear, type,
                               cd[idx[0]].width, cd[idx[0]].height))
        {
            return nullptr;
        }

        decoded[idx[0]] = decoded[idx[1]] = decoded[idx[2]] = true;
    }

    // Remaining channels individually.
    for (size_t i = 0; i < numChan; ++i)
    {
        if (decoded[i])
            continue;

        const int pixelSize = dwaPixelTypeSize(channels[i].datatype);

        switch (cd[i].scheme)
        {
            case DWA_LOSSY_DCT:
            {
                const std::vector<u8*>* rows[3] = { &rowPtrs[i], nullptr, nullptr };
                const u16* toLinear[3] =
                {
                    channels[i].linear ? nullptr : toLinearTable, nullptr, nullptr
                };
                DataType type[3] = { channels[i].datatype, DataType::NONE, DataType::NONE };

                if (!dwaDctDecodeGroup(rows, 1, acCur, acEnd, dcCur, toLinear, type,
                                       cd[i].width, cd[i].height))
                {
                    return nullptr;
                }
                break;
            }

            case DWA_RLE:
            {
                // The data is un-RLE'd but split by byte plane; re-interleave it.
                const u8* plane[4];
                for (int byte = 0; byte < pixelSize; ++byte)
                {
                    plane[byte] = rleBuffer.data() + cd[i].planarBase +
                                  size_t(byte) * cd[i].width * cd[i].height;
                }

                for (int row = 0; row < int(rowPtrs[i].size()); ++row)
                {
                    u8* dst = rowPtrs[i][row];
                    for (int x = 0; x < cd[i].width; ++x)
                        for (int byte = 0; byte < pixelSize; ++byte)
                            *dst++ = *plane[byte]++;
                }
                break;
            }

            case DWA_UNKNOWN_SCHEME:
            default:
            {
                const u8* src = unknownBuffer.data() + cd[i].planarBase;
                const size_t scanline = size_t(cd[i].width) * pixelSize;

                for (int row = 0; row < int(rowPtrs[i].size()); ++row)
                {
                    std::memcpy(rowPtrs[i][row], src, scanline);
                    src += scanline;
                }
                break;
            }
        }

        decoded[i] = true;
    }

    return dest.address;
}

const u8* ContextEXR::decompress_dwaa(Memory dest, ConstMemory source, int width, int height, int ystart)
{
    return decompress_dwa(dest, source, width, height, ystart);
}

const u8* ContextEXR::decompress_dwab(Memory dest, ConstMemory source, int width, int height, int ystart)
{
    return decompress_dwa(dest, source, width, height, ystart);
}

static inline
void writeChromaRGBA(float16* dest, float32x3 yw, float RY, float BY, float Y, float16 alpha)
{
    float r = RY * Y + Y;
    float b = BY * Y + Y;
    float g = (Y - r * yw.x - b * yw.z) / yw.y;
    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
    dest[3] = alpha;
}

static
void decodeLuminance(Surface surface, const u8* src, const Layer& layer, int x0, int y0, int x1, int y1)
{
    int blockWidth = x1 - x0;

    size_t bytesPerScan = blockWidth * layer.bytes;

    DataType datatype = DataType::NONE;

    Channel luminance;
    Channel alpha;

    for (const Channel& channel : layer.channels)
    {
        if (datatype != DataType::NONE && datatype != channel.datatype)
        {
            // all channels must have same datatype
            return;
        }

        datatype = channel.datatype;

        if (channel.component == Component::Y)
        {
            luminance = channel;
        }

        if (channel.component == Component::A)
        {
            alpha = channel;
        }
    }

    if (datatype == DataType::HALF)
    {
        float16 one(1.0f);

        const u8* scan_a = reinterpret_cast<const u8*>(&one);

        size_t step_y = luminance.bytes;
        size_t step_a = alpha.bytes;

        for (int y = y0; y < y1; ++y)
        {
            float16* image = surface.address<float16>(x0, y);

            const u8* scan_y = src + blockWidth * luminance.offset;
            if (step_a)
            {
                scan_a = src + blockWidth * alpha.offset;
            }

            for (int x = x0; x < x1; ++x)
            {
                float16 s = uload16f(scan_y);
                float16 a = uload16f(scan_a);

                image[0] = s;
                image[1] = s;
                image[2] = s;
                image[3] = a;

                scan_y += step_y;
                scan_a += step_a;
                image += 4;
            }

            src += bytesPerScan;
        }
    }
    else
    {
        // MANGO TODO
    }
}

static
void decodeChroma(Surface surface, const u8* src, const Layer& layer, const Chromaticities& chromaticities, int x0, int y0, int x1, int y1)
{
    int blockWidth = x1 - x0;
    //int blockHeight = y1 - y0;

    size_t bytesPerScan = blockWidth * layer.bytes;

    float32x3 yw = computeYW(chromaticities, 1.0f);

    DataType datatype = DataType::NONE;

    Channel luminance;
    Channel red;
    Channel blue;

    for (const Channel& channel : layer.channels)
    {
        if (datatype != DataType::NONE && datatype != channel.datatype)
        {
            // all channels must have same datatype
            return;
        }

        datatype = channel.datatype;

        if (channel.component == Component::Y)
        {
            luminance = channel;
        }

        if (channel.component == Component::RY)
        {
            red = channel;
        }

        if (channel.component == Component::BY)
        {
            blue = channel;
        }
    }

    if (datatype == DataType::HALF)
    {
        // MANGO TODO: clipping
        for (int y = y0; y < y1; y += 2)
        {
            float16* image0 = surface.address<float16>(0, y + 0);
            float16* image1 = surface.address<float16>(0, y + 1);

            float16 alpha(1.0f);

            const u8* scan_y0 = src + blockWidth * (luminance.offset + 0);
            const u8* scan_y1 = src + blockWidth * (luminance.offset + 2);
            const u8* scan_r = src + blockWidth * red.offset;
            const u8* scan_b = src + blockWidth * blue.offset;

            for (int x = x0; x < x1; x += 2)
            {
                float r = uload16f(scan_r + x * 1);
                float b = uload16f(scan_b + x * 1);
                float s0 = uload16f(scan_y0 + x * 2 + 0);
                float s1 = uload16f(scan_y0 + x * 2 + 2);
                float s2 = uload16f(scan_y1 + x * 2 + 0);
                float s3 = uload16f(scan_y1 + x * 2 + 2);

                writeChromaRGBA(image0 + 0, yw, r, b, s0, alpha);
                writeChromaRGBA(image0 + 4, yw, r, b, s1, alpha);
                writeChromaRGBA(image1 + 0, yw, r, b, s2, alpha);
                writeChromaRGBA(image1 + 4, yw, r, b, s3, alpha);

                image0 += 8;
                image1 += 8;
            }

            src += bytesPerScan;
        }
    }
    else
    {
        // MANGO TODO
    }
}

static
void decodeRGB(Surface surface, const u8* src, const Layer& layer, int x0, int y0, int x1, int y1)
{
    int blockWidth = x1 - x0;

    size_t bytesPerScan = blockWidth * layer.bytes;

    size_t ch_step [] = { 0, 0, 0, 0 };
    int ch_offset [] = { 0, 0, 0, 0 };

    DataType datatype = DataType::NONE;

    for (const Channel& channel : layer.channels)
    {
        if (datatype != DataType::NONE && datatype != channel.datatype)
        {
            // all channels must have same datatype
            return;
        }

        datatype = channel.datatype;

        if (channel.component == Component::R)
        {
            ch_offset[0] = channel.offset;
            ch_step[0] = channel.bytes;
        }

        if (channel.component == Component::G)
        {
            ch_offset[1] = channel.offset;
            ch_step[1] = channel.bytes;
        }

        if (channel.component == Component::B)
        {
            ch_offset[2] = channel.offset;
            ch_step[2] = channel.bytes;
        }

        if (channel.component == Component::A)
        {
            ch_offset[3] = channel.offset;
            ch_step[3] = channel.bytes;
        }
    }

    if (datatype == DataType::HALF)
    {
        float16 zeros [] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float16 ones [] = { 1.0f, 1.0f, 1.0f, 1.0f };

        const u8* ptr[4] =
        {
            reinterpret_cast<const u8*>(zeros),
            reinterpret_cast<const u8*>(zeros),
            reinterpret_cast<const u8*>(zeros),
            reinterpret_cast<const u8*>(ones)
        };

        for (int y = y0; y < y1; ++y)
        {
            float16* image = surface.address<float16>(x0, y);

            for (int i = 0; i < 4; ++i)
            {
                if (ch_step[i])
                {
                    ptr[i] = src + blockWidth * ch_offset[i];
                }
            }

            int count = blockWidth;

            while (count >= 4)
            {
                float32x4 r = convert<float32x4>(float16x4::uload(ptr[0]));
                float32x4 g = convert<float32x4>(float16x4::uload(ptr[1]));
                float32x4 b = convert<float32x4>(float16x4::uload(ptr[2]));
                float32x4 a = convert<float32x4>(float16x4::uload(ptr[3]));

                float32x4 color[4];
                transpose(color, r, g, b, a);

                float16x4::ustore(image +  0, convert<float16x4>(color[0]));
                float16x4::ustore(image +  4, convert<float16x4>(color[1]));
                float16x4::ustore(image +  8, convert<float16x4>(color[2]));
                float16x4::ustore(image + 12, convert<float16x4>(color[3]));

                ptr[0] += ch_step[0] * 4;
                ptr[1] += ch_step[1] * 4;
                ptr[2] += ch_step[2] * 4;
                ptr[3] += ch_step[3] * 4;

                image += 16;
                count -= 4;
            }

            while (count-- > 0)
            {
                float32 r = uload16f(ptr[0]);
                float32 g = uload16f(ptr[1]);
                float32 b = uload16f(ptr[2]);
                float32 a = uload16f(ptr[3]);

                image[0] = r;
                image[1] = g;
                image[2] = b;
                image[3] = a;

                ptr[0] += ch_step[0];
                ptr[1] += ch_step[1];
                ptr[2] += ch_step[2];
                ptr[3] += ch_step[3];

                image += 4;
            }

            src += bytesPerScan;
        }
    }
    else if (datatype == DataType::FLOAT)
    {
        float32 zeros [] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float32 ones [] = { 1.0f, 1.0f, 1.0f, 1.0f };

        const u8* ptr[4] =
        {
            reinterpret_cast<const u8*>(zeros),
            reinterpret_cast<const u8*>(zeros),
            reinterpret_cast<const u8*>(zeros),
            reinterpret_cast<const u8*>(ones)
        };

        for (int y = y0; y < y1; ++y)
        {
            float16* image = surface.address<float16>(x0, y);

            for (int i = 0; i < 4; ++i)
            {
                if (ch_step[i])
                {
                    ptr[i] = src + blockWidth * ch_offset[i];
                }
            }

            int count = blockWidth;

            while (count >= 4)
            {
                float32x4 r = float32x4::uload(ptr[0]);
                float32x4 g = float32x4::uload(ptr[1]);
                float32x4 b = float32x4::uload(ptr[2]);
                float32x4 a = float32x4::uload(ptr[3]);

                float32x4 color[4];
                transpose(color, r, g, b, a);

                float16x4::ustore(image +  0, convert<float16x4>(color[0]));
                float16x4::ustore(image +  4, convert<float16x4>(color[1]));
                float16x4::ustore(image +  8, convert<float16x4>(color[2]));
                float16x4::ustore(image + 12, convert<float16x4>(color[3]));

                ptr[0] += ch_step[0] * 4;
                ptr[1] += ch_step[1] * 4;
                ptr[2] += ch_step[2] * 4;
                ptr[3] += ch_step[3] * 4;

                image += 16;
                count -= 4;
            }

            while (count-- > 0)
            {
                float32 r = uload32f(ptr[0]);
                float32 g = uload32f(ptr[1]);
                float32 b = uload32f(ptr[2]);
                float32 a = uload32f(ptr[3]);

                image[0] = r;
                image[1] = g;
                image[2] = b;
                image[3] = a;

                ptr[0] += ch_step[0];
                ptr[1] += ch_step[1];
                ptr[2] += ch_step[2];
                ptr[3] += ch_step[3];

                image += 4;
            }

            src += bytesPerScan;
        }
    }
}

void ContextEXR::decodeBlock(Surface surface, ConstMemory memory, int x0, int y0, int x1, int y1)
{
    int blockWidth = x1 - x0;
    int blockHeight = y1 - y0;
    //printLine(Print::Debug, "decodeBlock: ({}, {}) {} x {}", x0, y0, blockWidth, blockHeight);

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

    // select first layer
    const Layer& layer = m_attributes.chlist.layers[0];

    switch (layer.colortype)
    {
        case ColorType::LUMINANCE:
            decodeLuminance(surface, src, layer, x0, y0, x1, y1);
            break;

        case ColorType::CHROMA:
            decodeChroma(surface, src, layer, m_attributes.chromaticities, x0, y0, x1, y1);
            break;

        case ColorType::RGB:
            decodeRGB(surface, src, layer, x0, y0, x1, y1);
            break;

        case ColorType::NONE:
            break;
    }

    u64 time2 = mango::Time::us();

    m_time_decompress += (time1 - time0);
    m_time_blit += (time2 - time1);
}

void ContextEXR::decodeImage(const ImageDecodeOptions& options)
{
    if (m_surface.image)
    {
        // already decoded
        return;
    }

    int width = m_header.width;
    int height = m_header.height * std::max(1, m_header.faces);
    size_t stride = width * m_header.format.bytes();

    m_image_buffer.resize(height * stride);
    m_surface = Surface(width, height, m_header.format, stride, m_image_buffer.data());

    if (m_attributes.lineOrder)
    {
        m_surface.image = m_surface.image + m_surface.stride * (m_surface.height - 1);
        m_surface.stride = -m_surface.stride;
    }

    u64 time0 = mango::Time::us();

    LittleEndianConstPointer p = m_pointer;

    ConcurrentQueue q;

    if (is_single_tile)
    {
        int xtiles = div_ceil(width, m_attributes.tiledesc.xsize);
        int ytiles = div_ceil(height, m_attributes.tiledesc.ysize);
        int ntiles = xtiles * ytiles;

        printLine(Print::Debug, "Tiles: {} x {} ({})", xtiles, ytiles, ntiles);

        const u8* base = m_memory.address;
        const size_t total = m_memory.size;

        for (int i = 0; i < ntiles; ++i)
        {
            // bounds-check the offset table entry itself
            if (m_pointer + (size_t(i) + 1) * sizeof(u64) > base + total)
                break;

            u64 offset = p.read64();

            // tile chunk header is 20 bytes (tilex, tiley, xlevel, ylevel, size)
            if (offset > total || total - offset < 20)
                continue;

            LittleEndianConstPointer ptr = base + offset;

            int tilex = ptr.read32();
            int tiley = ptr.read32();
            int xlevel = ptr.read32();
            int ylevel = ptr.read32();
            u32 size = ptr.read32();

            // make sure the tile payload is within the file
            if (size > total - offset - 20)
                continue;

            //printLine(Print::Debug, "  pos:({},{}) level:({},{}) size: {} bytes", tilex, tiley, xlevel, ylevel, size);
            MANGO_UNREFERENCED(xlevel);
            MANGO_UNREFERENCED(ylevel);

            int tileWidth = m_attributes.tiledesc.xsize;
            int tileHeight = m_attributes.tiledesc.ysize;

            int x0 = tilex * tileWidth;
            int y0 = tiley * tileHeight;
            int x1 = std::min(width, x0 + tileWidth);
            int y1 = std::min(height, y0 + tileHeight);

            if (x0 < 0 || x1 > width || y0 < 0 || y1 > height)
            {
                // incorrect tile
                return;
            }

            auto task = [=, this]
            {
                ConstMemory memory(ptr, size);
                decodeBlock(m_surface, memory, x0, y0, x1, y1);
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

        printLine(Print::Debug, "Blocks: {}", nblocks);

        const u8* base = m_memory.address;
        const size_t total = m_memory.size;

        for (int i = 0; i < nblocks; ++i)
        {
            // bounds-check the offset table entry itself
            if (m_pointer + (size_t(i) + 1) * sizeof(u64) > base + total)
                break;

            u64 offset = p.read64();

            // chunk header is 8 bytes (ystart + size); make sure it fits
            if (offset > total || total - offset < 8)
                continue;

            LittleEndianConstPointer ptr = base + offset;

            int ystart = ptr.read32();
            u32 size = ptr.read32();

            // make sure the chunk payload is within the file
            if (size > total - offset - 8)
                continue;

            int x0 = 0;
            int y0 = ystart - m_attributes.dataWindow.ymin;
            int x1 = width;
            int y1 = std::min(height, y0 + m_scanLinesPerBlock);

            if (y0 < 0 || y1 > height)
            {
                // incorrect block
                return;
            }

            //printLine(Print::Debug, "  y:{}, size: {} bytes", y0, size);

            auto task = [=, this]
            {
                ConstMemory memory(ptr, size);
                decodeBlock(m_surface, memory, x0, y0, x1, y1);
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

    report();
}

ImageDecodeStatus ContextEXR::decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face)
{
    MANGO_UNREFERENCED(level);
    MANGO_UNREFERENCED(depth);

    ImageDecodeStatus status;

    if (!m_pointer)
    {
        status.setError("No data.");
        return status;
    }

    decodeImage(options);

    int width = m_header.width;
    int height = m_header.height;

    face = std::clamp(face, 0, std::max(1, m_header.faces) - 1);

    // flip z-axis for cubemap faces
    if (face == 4) face = 5;
    else if (face == 5) face = 4;

    dest.blit(0, 0, Surface(m_surface, 0, face * height, width, (face + 1) * height));

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

    struct ImageDecoderEXR : ImageDecodeInterface
    {
        ContextEXR m_context;

        ImageDecoderEXR(ConstMemory memory)
            : m_context(memory)
        {
            header = m_context.m_header;
        }

        ~ImageDecoderEXR()
        {
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(face);
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            return m_context.decode(dest, options, level, depth, face);
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new ImageDecoderEXR(memory);
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
