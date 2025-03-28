// Copyright 2009 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*
    The original source code has been modified for integration.
*/

#include <string.h>
#include <mango/core/configure.hpp>
#include <mango/image/compression.hpp>

/* From http://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt

 The number of bits that represent a 4x4 texel block is 64 bits if
 <internalformat> is given by ETC1_RGB8_OES.

 The data for a block is a number of bytes,

 {q0, q1, q2, q3, q4, q5, q6, q7}

 where byte q0 is located at the lowest memory address and q7 at
 the highest. The 64 bits specifying the block is then represented
 by the following 64 bit integer:

 int64bit = 256*(256*(256*(256*(256*(256*(256*q0+q1)+q2)+q3)+q4)+q5)+q6)+q7;

 ETC1_RGB8_OES:

 a) bit layout in bits 63 through 32 if diffbit = 0

 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
 -----------------------------------------------
 | base col1 | base col2 | base col1 | base col2 |
 | R1 (4bits)| R2 (4bits)| G1 (4bits)| G2 (4bits)|
 -----------------------------------------------

 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
 ---------------------------------------------------
 | base col1 | base col2 | table  | table  |diff|flip|
 | B1 (4bits)| B2 (4bits)| cw 1   | cw 2   |bit |bit |
 ---------------------------------------------------


 b) bit layout in bits 63 through 32 if diffbit = 1

 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
 -----------------------------------------------
 | base col1    | dcol 2 | base col1    | dcol 2 |
 | R1' (5 bits) | dR2    | G1' (5 bits) | dG2    |
 -----------------------------------------------

 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
 ---------------------------------------------------
 | base col 1   | dcol 2 | table  | table  |diff|flip|
 | B1' (5 bits) | dB2    | cw 1   | cw 2   |bit |bit |
 ---------------------------------------------------


 c) bit layout in bits 31 through 0 (in both cases)

 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 -----------------------------------------------
 |       most significant pixel index bits       |
 | p| o| n| m| l| k| j| i| h| g| f| e| d| c| b| a|
 -----------------------------------------------

 15 14 13 12 11 10  9  8  7  6  5  4  3   2   1  0
 --------------------------------------------------
 |         least significant pixel index bits       |
 | p| o| n| m| l| k| j| i| h| g| f| e| d| c | b | a |
 --------------------------------------------------


 Add table 3.17.2: Intensity modifier sets for ETC1 compressed textures:

 table codeword                modifier table
 ------------------        ----------------------
 0                     -8  -2  2   8
 1                    -17  -5  5  17
 2                    -29  -9  9  29
 3                    -42 -13 13  42
 4                    -60 -18 18  60
 5                    -80 -24 24  80
 6                   -106 -33 33 106
 7                   -183 -47 47 183


 Add table 3.17.3 Mapping from pixel index values to modifier values for
 ETC1 compressed textures:

 pixel index value
 ---------------
 msb     lsb           resulting modifier value
 -----   -----          -------------------------
 1       1            -b (large negative value)
 1       0            -a (small negative value)
 0       0             a (small positive value)
 0       1             b (large positive value)


 */

#define ETC1_ENCODED_BLOCK_SIZE 8
#define ETC1_DECODED_BLOCK_SIZE 64

typedef unsigned char etc1_byte;
typedef int etc1_bool;
typedef unsigned int etc1_uint32;

static const int kModifierTable[] = {
/* 0 */2, 8, -2, -8,
/* 1 */5, 17, -5, -17,
/* 2 */9, 29, -9, -29,
/* 3 */13, 42, -13, -42,
/* 4 */18, 60, -18, -60,
/* 5 */24, 80, -24, -80,
/* 6 */33, 106, -33, -106,
/* 7 */47, 183, -47, -183 };

static inline etc1_byte clamp(int x) {
    if ((unsigned)x > 255)
    {
        if (x < 0) x = 0;
        else x = 255;
    }
    return static_cast<etc1_byte>(x);
}

static
inline int convert4To8(int b) {
    int c = b & 0xf;
    return (c << 4) | c;
}

static
inline int convert5To8(int b) {
    int c = b & 0x1f;
    return (c << 3) | (c >> 2);
}

static
inline int divideBy255(int d) {
    //return d / 255;
    return (d + 128 + (d >> 8)) >> 8;
}

static
inline int convert8To4(int b) {
    int c = b & 0xff;
    return divideBy255(c * 15);
}

static
inline int convert8To5(int b) {
    int c = b & 0xff;
    return divideBy255(c * 31);
}

typedef struct {
    etc1_uint32 high;
    etc1_uint32 low;
    etc1_uint32 score; // Lower is more accurate
} etc_compressed;

static
inline void take_best(etc_compressed* a, const etc_compressed* b) {
    if (a->score > b->score) {
        *a = *b;
    }
}

static
void etc_average_colors_subblock(const etc1_byte* pIn, size_t stride,
        etc1_byte* pColors, bool flipped, int base) {
    int r = 0;
    int g = 0;
    int b = 0;

    if (flipped) {
        for (int y = 0; y < 2; y++) {
            const etc1_byte* p = pIn + (base + y) * stride;
            for (int x = 0; x < 4; x++) {
                r += p[0];
                g += p[1];
                b += p[2];
                p += 4;
            }
        }
    } else {
        for (int y = 0; y < 4; y++) {
            const etc1_byte* p = pIn + y * stride + base * 4;
            for (int x = 0; x < 2; x++) {
                r += p[0];
                g += p[1];
                b += p[2];
                p += 4;
            }
        }
    }
    pColors[0] = (etc1_byte)((r + 4) >> 3);
    pColors[1] = (etc1_byte)((g + 4) >> 3);
    pColors[2] = (etc1_byte)((b + 4) >> 3);
}

static
inline int square(int x) {
    return x * x;
}

static etc1_uint32 chooseModifier(const etc1_byte* pBaseColors,
        const etc1_byte* pIn, etc1_uint32 *pLow, int bitIndex,
        const int* pModifierTable) {
    etc1_uint32 bestScore = ~etc1_uint32(0);
    int bestIndex = 0;
    int pixelR = pIn[0];
    int pixelG = pIn[1];
    int pixelB = pIn[2];
    int r = pBaseColors[0];
    int g = pBaseColors[1];
    int b = pBaseColors[2];
    for (int i = 0; i < 4; i++) {
        int modifier = pModifierTable[i];
        int decodedG = clamp(g + modifier);
        etc1_uint32 score = (etc1_uint32) (6 * square(decodedG - pixelG));
        if (score >= bestScore) {
            continue;
        }
        int decodedR = clamp(r + modifier);
        score += (etc1_uint32) (3 * square(decodedR - pixelR));
        if (score >= bestScore) {
            continue;
        }
        int decodedB = clamp(b + modifier);
        score += (etc1_uint32) square(decodedB - pixelB);
        if (score < bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    etc1_uint32 lowMask = (((bestIndex >> 1) << 16) | (bestIndex & 1)) << bitIndex;
    *pLow |= lowMask;
    return bestScore;
}

static
void etc_encode_subblock_helper(const etc1_byte* pIn, size_t stride,
        etc_compressed* pCompressed, bool flipped, int base,
        const etc1_byte* pBaseColors, const int* pModifierTable) {
    int score = pCompressed->score;
    if (flipped) {
        for (int y = 0; y < 2; y++) {
            int yy = base + y;
            for (int x = 0; x < 4; x++) {
                score += chooseModifier(pBaseColors, pIn + x * 4 + yy * stride,
                        &pCompressed->low, yy + x * 4, pModifierTable);
            }
        }
    } else {
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 2; x++) {
                int xx = base + x;
                score += chooseModifier(pBaseColors, pIn + xx * 4 + y * stride,
                        &pCompressed->low, y + xx * 4, pModifierTable);
            }
        }
    }
    pCompressed->score = score;
}

static bool inRange4bitSigned(int color) {
    return color >= -4 && color <= 3;
}

static void etc_encodeBaseColors(etc1_byte* pBaseColors,
        const etc1_byte* pColors, etc_compressed* pCompressed) {
    int r1, g1, b1, r2, g2, b2; // 8 bit base colors for sub-blocks

    // Silence compiler warning about uninitialized variables
    r2 = 0;
    g2 = 0;
    b2 = 0;

    bool differential;
    {
        int r51 = convert8To5(pColors[0]);
        int g51 = convert8To5(pColors[1]);
        int b51 = convert8To5(pColors[2]);
        int r52 = convert8To5(pColors[4]);
        int g52 = convert8To5(pColors[5]);
        int b52 = convert8To5(pColors[6]);

        r1 = convert5To8(r51);
        g1 = convert5To8(g51);
        b1 = convert5To8(b51);

        int dr = r52 - r51;
        int dg = g52 - g51;
        int db = b52 - b51;

        differential = inRange4bitSigned(dr) && inRange4bitSigned(dg)
                && inRange4bitSigned(db);
        if (differential) {
            r2 = convert5To8(r51 + dr);
            g2 = convert5To8(g51 + dg);
            b2 = convert5To8(b51 + db);
            pCompressed->high |= (r51 << 27) | ((7 & dr) << 24) | (g51 << 19) | ((7 & dg) << 16) | (b51 << 11) | ((7 & db) << 8) | 2;
        }
    }

    if (!differential) {
        int r41 = convert8To4(pColors[0]);
        int g41 = convert8To4(pColors[1]);
        int b41 = convert8To4(pColors[2]);
        int r42 = convert8To4(pColors[4]);
        int g42 = convert8To4(pColors[5]);
        int b42 = convert8To4(pColors[6]);
        r1 = convert4To8(r41);
        g1 = convert4To8(g41);
        b1 = convert4To8(b41);
        r2 = convert4To8(r42);
        g2 = convert4To8(g42);
        b2 = convert4To8(b42);
        pCompressed->high |= (r41 << 28) | (r42 << 24) | (g41 << 20) | (g42 << 16) | (b41 << 12) | (b42 << 8);
    }
    pBaseColors[0] = static_cast<etc1_byte>(r1);
    pBaseColors[1] = static_cast<etc1_byte>(g1);
    pBaseColors[2] = static_cast<etc1_byte>(b1);
    pBaseColors[3] = static_cast<etc1_byte>(0);
    pBaseColors[4] = static_cast<etc1_byte>(r2);
    pBaseColors[5] = static_cast<etc1_byte>(g2);
    pBaseColors[6] = static_cast<etc1_byte>(b2);
    pBaseColors[7] = static_cast<etc1_byte>(0);
}

static
void etc_encode_block_helper(const etc1_byte* pIn, size_t stride,
        const etc1_byte* pColors, etc_compressed* pCompressed, bool flipped) {
    pCompressed->score = ~etc1_uint32(0);
    pCompressed->high = (flipped ? 1 : 0);
    pCompressed->low = 0;

    etc1_byte pBaseColors[8];

    etc_encodeBaseColors(pBaseColors, pColors, pCompressed);

    int originalHigh = pCompressed->high;

    const int* pModifierTable = kModifierTable;
    for (int i = 0; i < 8; i++, pModifierTable += 4) {
        etc_compressed temp;
        temp.score = 0;
        temp.high = originalHigh | (i << 5);
        temp.low = 0;
        etc_encode_subblock_helper(pIn, stride, &temp, flipped, 0,
                pBaseColors, pModifierTable);
        take_best(pCompressed, &temp);
    }
    pModifierTable = kModifierTable;
    etc_compressed firstHalf = *pCompressed;
    for (int i = 0; i < 8; i++, pModifierTable += 4) {
        etc_compressed temp;
        temp.score = firstHalf.score;
        temp.high = firstHalf.high | (i << 2);
        temp.low = firstHalf.low;
        etc_encode_subblock_helper(pIn, stride, &temp, flipped, 2,
                pBaseColors + 4, pModifierTable);
        if (i == 0) {
            *pCompressed = temp;
        } else {
            take_best(pCompressed, &temp);
        }
    }
}

namespace mango::image
{

    void encode_block_etc1(const TextureCompression& info, u8* output, const u8* input, size_t stride)
    {
        MANGO_UNREFERENCED(info);

        etc1_byte colors[8];
        etc1_byte flipped[8];
        etc_average_colors_subblock(input, stride, colors + 0, false, 0);
        etc_average_colors_subblock(input, stride, colors + 4, false, 2);
        etc_average_colors_subblock(input, stride, flipped + 0, true, 0);
        etc_average_colors_subblock(input, stride, flipped + 4, true, 2);

        etc_compressed a, b;
        etc_encode_block_helper(input, stride, colors, &a, false);
        etc_encode_block_helper(input, stride, flipped, &b, true);
        take_best(&a, &b);
        bigEndian::ustore32(output + 0, a.high);
        bigEndian::ustore32(output + 4, a.low);
    }

} // namespace mango::image
