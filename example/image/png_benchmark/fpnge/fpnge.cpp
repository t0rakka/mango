// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "fpnge.h"
#ifdef CAN_COMPILE_FPNGE

#include <assert.h>
#include <immintrin.h>
#include <queue>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr uint8_t kNbits[286] = {
    // First 16 symbols
    2, 3, 4, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    // Intermediate symbols, 16-239. They are clustered together so that they
    // end up having the same 4 upper bits.
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    // Last 16 literal symbols
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 6, 4, 3,
    // EOB
    12,
    // LZ77 symbols
    10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 12, 8};

alignas(32) constexpr uint8_t kFirst16Nbits[16] = {2, 3, 4, 6, 7, 8, 8, 8,
                                                   8, 8, 8, 8, 8, 8, 8, 8};
alignas(32) constexpr uint8_t kFirst16Bits[16] = {
    0x00, 0x02, 0x01, 0x05, 0x15, 0x35, 0xb5, 0x75,
    0xf5, 0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d,
};

alignas(32) constexpr uint8_t kLast16Nbits[16] = {8, 8, 8, 8, 8, 8, 8, 8,
                                                  8, 8, 8, 8, 7, 6, 4, 3};
alignas(32) constexpr uint8_t kLast16Bits[16] = {
    0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d,
    0xfd, 0x03, 0x83, 0x43, 0x55, 0x25, 0x09, 0x06};

alignas(32) constexpr uint8_t kMidNbits = 10;
alignas(32) constexpr uint8_t kMidLowBits[16] = {
    0,    0x23, 0x13, 0x33, 0x0b, 0x2b, 0x1b, 0x3b,
    0x07, 0x27, 0x17, 0x37, 0x0f, 0x2f, 0x1f, 0};

alignas(32) constexpr uint8_t kBitReverseNibbleLookup[16] = {
    0b0000, 0b1000, 0b0100, 0b1100, 0b0010, 0b1010, 0b0110, 0b1110,
    0b0001, 0b1001, 0b0101, 0b1101, 0b0011, 0b1011, 0b0111, 0b1111,
};

struct BitWriter {
  void Write(uint32_t count, uint64_t bits) {
    buffer |= bits << bits_in_buffer;
    bits_in_buffer += count;
    memcpy(data + bytes_written, &buffer, 8);
    size_t bytes_in_buffer = bits_in_buffer / 8;
    bits_in_buffer -= bytes_in_buffer * 8;
    buffer >>= bytes_in_buffer * 8;
    bytes_written += bytes_in_buffer;
  }

  void ZeroPadToByte() {
    if (bits_in_buffer != 0) {
      Write(8 - bits_in_buffer, 0);
    }
  }

  unsigned char *data;
  size_t bytes_written = 0;
  size_t bits_in_buffer = 0;
  uint64_t buffer = 0;
};

void WriteHuffmanCode(uint32_t num_channels, uint32_t &dist_nbits,
                      uint32_t &dist_bits, BitWriter *writer) {
  uint32_t dist = 0;
  if (num_channels == 1) {
    dist = 5;
    dist_nbits = 2;
    dist_bits = 2;
  } else if (num_channels == 2) {
    dist = 7;
    dist_nbits = 3;
    dist_bits = 6;
  } else if (num_channels == 3) {
    dist = 8;
    dist_nbits = 4;
    dist_bits = 14;
  } else if (num_channels == 4) {
    dist = 9;
    dist_nbits = 4;
    dist_bits = 14;
  } else {
    assert(false);
  }
  constexpr uint8_t kCodeLengthNbits[] = {
      5, 7, 7, 6, 6, 0, 6, 6, 2, 0, 1, 3, 6, 0, 0, 0, 0, 0, 0,
  };
  constexpr uint8_t kCodeLengthBits[] = {
      0x7, 0x3f, 0x7f, 0x17, 0x37, 0x0, 0xf, 0x2f, 0x1, 0x0,
      0x0, 0x3,  0x1f, 0x0,  0x0,  0x0, 0x0, 0x0,  0x0,
  };
  constexpr uint8_t kCodeLengthOrder[] = {
      16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
  };
  writer->Write(5, 29);   // all lit/len codes
  writer->Write(5, dist); // distance code up to dist, included
  writer->Write(4, 15);   // all code length codes
  for (size_t i = 0; i < 19; i++) {
    writer->Write(3, kCodeLengthNbits[kCodeLengthOrder[i]]);
  }

  for (size_t i = 0; i < 286; i++) {
    writer->Write(kCodeLengthNbits[kNbits[i]], kCodeLengthBits[kNbits[i]]);
  }
  for (size_t i = 0; i < dist; i++) {
    writer->Write(kCodeLengthNbits[0], kCodeLengthBits[0]);
  }
  writer->Write(kCodeLengthNbits[1], kCodeLengthBits[1]);
}

constexpr unsigned kCrcTable[] = {
    0x0,        0x77073096, 0xee0e612c, 0x990951ba, 0x76dc419,  0x706af48f,
    0xe963a535, 0x9e6495a3, 0xedb8832,  0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x9b64c2b,  0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x1db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x6b6b51f,  0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0xf00f934,  0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x86d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x3b6e20c,  0x74b1d29a,
    0xead54739, 0x9dd277af, 0x4db2615,  0x73dc1683, 0xe3630b12, 0x94643b84,
    0xd6d6a3e,  0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0xa00ae27,  0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x26d930a,  0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x5005713,  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0xcb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0xbdbdf21,  0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

unsigned long update_crc(unsigned long crc, unsigned char *buf, int len) {
  static const uint64_t k1k2[] = {0x1'5444'2BD4ULL, 0x1'C6E4'1596ULL};
  static const uint64_t k3k4[] = {0x1'7519'97D0ULL, 0x0'CCAA'009EULL};
  static const uint64_t k5k6[] = {0x1'63CD'6124ULL, 0x0'0000'0000ULL};
  static const uint64_t poly[] = {0x1'DB71'0641ULL, 0x1'F701'1641ULL};

  int n = 0;
  unsigned long c;

  if (len >= 128) {
    // Adapted from WUFFs code.
    auto x0 = _mm_loadu_si128((__m128i *)(buf + n + 0x00));
    auto x1 = _mm_loadu_si128((__m128i *)(buf + n + 0x10));
    auto x2 = _mm_loadu_si128((__m128i *)(buf + n + 0x20));
    auto x3 = _mm_loadu_si128((__m128i *)(buf + n + 0x30));

    x0 = _mm_xor_si128(x0, _mm_cvtsi32_si128(crc));
    n += 64;

    auto k = _mm_loadu_si128((__m128i *)k1k2);
    while (n + 64 <= len) {
      auto y0 = _mm_clmulepi64_si128(x0, k, 0x00);
      auto y1 = _mm_clmulepi64_si128(x1, k, 0x00);
      auto y2 = _mm_clmulepi64_si128(x2, k, 0x00);
      auto y3 = _mm_clmulepi64_si128(x3, k, 0x00);

      x0 = _mm_clmulepi64_si128(x0, k, 0x11);
      x1 = _mm_clmulepi64_si128(x1, k, 0x11);
      x2 = _mm_clmulepi64_si128(x2, k, 0x11);
      x3 = _mm_clmulepi64_si128(x3, k, 0x11);

      x0 = _mm_xor_si128(_mm_xor_si128(x0, y0),
                         _mm_loadu_si128((__m128i *)(buf + n + 0x00)));
      x1 = _mm_xor_si128(_mm_xor_si128(x1, y1),
                         _mm_loadu_si128((__m128i *)(buf + n + 0x10)));
      x2 = _mm_xor_si128(_mm_xor_si128(x2, y2),
                         _mm_loadu_si128((__m128i *)(buf + n + 0x20)));
      x3 = _mm_xor_si128(_mm_xor_si128(x3, y3),
                         _mm_loadu_si128((__m128i *)(buf + n + 0x30)));
      n += 64;
    }

    k = _mm_loadu_si128((__m128i *)k3k4);
    auto y0 = _mm_clmulepi64_si128(x0, k, 0x00);
    x0 = _mm_clmulepi64_si128(x0, k, 0x11);
    x0 = _mm_xor_si128(x0, x1);
    x0 = _mm_xor_si128(x0, y0);
    y0 = _mm_clmulepi64_si128(x0, k, 0x00);
    x0 = _mm_clmulepi64_si128(x0, k, 0x11);
    x0 = _mm_xor_si128(x0, x2);
    x0 = _mm_xor_si128(x0, y0);
    y0 = _mm_clmulepi64_si128(x0, k, 0x00);
    x0 = _mm_clmulepi64_si128(x0, k, 0x11);
    x0 = _mm_xor_si128(x0, x3);
    x0 = _mm_xor_si128(x0, y0);

    x1 = _mm_clmulepi64_si128(x0, k, 0x10);
    x2 = _mm_setr_epi32(~0U, 0, ~0U, 0);
    x0 = _mm_srli_si128(x0, 8);
    x0 = _mm_xor_si128(x0, x1);

    k = _mm_loadu_si128((__m128i *)k5k6);
    x1 = _mm_srli_si128(x0, 4);
    x0 = _mm_and_si128(x0, x2);
    x0 = _mm_clmulepi64_si128(x0, k, 0x00);
    x0 = _mm_xor_si128(x0, x1);

    k = _mm_loadu_si128((__m128i *)poly);
    x1 = _mm_and_si128(x0, x2);
    x1 = _mm_clmulepi64_si128(x1, k, 0x10);
    x1 = _mm_and_si128(x1, x2);
    x1 = _mm_clmulepi64_si128(x1, k, 0x00);
    x0 = _mm_xor_si128(x0, x1);

    c = _mm_extract_epi32(x0, 1);
  } else {
    c = crc;
  }

  for (; n < len; n++) {
    c = kCrcTable[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

unsigned long compute_crc(unsigned char *buf, int len) {
  return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

constexpr unsigned kAdler32Mod = 65521;

void UpdateAdler32(uint32_t &s1, uint32_t &s2, uint8_t byte) {
  s1 += byte;
  s2 += s1;
  s1 %= kAdler32Mod;
  s2 %= kAdler32Mod;
}

uint32_t hadd(__m256 v) {
  auto sum =
      _mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
  auto hi = _mm_unpackhi_epi64(sum, sum);

  sum = _mm_add_epi32(hi, sum);
  hi = _mm_shuffle_epi32(sum, 0xB1);

  sum = _mm_add_epi32(sum, hi);

  return _mm_cvtsi128_si32(sum);
}

template <size_t predictor, typename CB, typename CB_ADL, typename CB_RLE>
__attribute__((always_inline)) void
ProcessRow(size_t bytes_per_line_buf, const unsigned char *mask,
           unsigned char *current_row_buf, const unsigned char *top_buf,
           const unsigned char *left_buf, const unsigned char *topleft_buf,
           CB &&cb, CB_ADL &&cb_adl, CB_RLE &&cb_rle) {
  alignas(32) uint8_t last_predicted_data[32] = {};
  size_t run = 0;
  for (size_t i = 0; i + 32 <= bytes_per_line_buf; i += 32) {
    size_t bytes_per_32 = __builtin_popcount(
        _mm256_movemask_epi8(_mm256_load_si256((__m256i *)(mask + i))));
    alignas(32) uint8_t predicted_data[32] = {};
    if (predictor == 0) {
      for (size_t ii = 0; ii < 32; ii++) {
        uint8_t data = current_row_buf[i + ii];
        predicted_data[ii] = data;
      }
    } else if (predictor == 1) {
      for (size_t ii = 0; ii < 32; ii++) {
        uint8_t data = current_row_buf[i + ii] - left_buf[i + ii];
        predicted_data[ii] = data;
      }
    } else if (predictor == 2) {
      for (size_t ii = 0; ii < 32; ii++) {
        uint8_t data = current_row_buf[i + ii] - top_buf[i + ii];
        predicted_data[ii] = data;
      }
    } else if (predictor == 3) {
      for (size_t ii = 0; ii < 32; ii++) {
        uint8_t pred = (top_buf[i + ii] + left_buf[i + ii]) / 2;
        uint8_t data = current_row_buf[i + ii] - pred;
        predicted_data[ii] = data;
      }
    } else {
      for (size_t ii = 0; ii < 32; ii++) {
        uint8_t a = left_buf[i + ii];
        uint8_t b = top_buf[i + ii];
        uint8_t c = topleft_buf[i + ii];
        uint8_t bc = b - c;
        uint8_t ca = c - a;
        uint8_t pa = c < b ? bc : -bc;
        uint8_t pb = a < c ? ca : -ca;
        uint8_t pc = (a < c) == (c < b) ? (bc >= ca ? bc - ca : ca - bc) : 255;
        uint8_t pred = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
        uint8_t data = current_row_buf[i + ii] - pred;
        predicted_data[ii] = data;
      }
    }

    bool continue_rle = i != 0 && bytes_per_32 >= 3;
    for (size_t ii = 0; ii < 32; ii++) {
      continue_rle &=
          mask[i + ii] == 0 || predicted_data[ii] == last_predicted_data[ii];
    }

    if (continue_rle) {
      run += bytes_per_32;
    } else {
      cb_rle(run);
      run = 0;
      cb(predicted_data, mask + i);
    }
    cb_adl(bytes_per_32, predicted_data, mask + i, i);
    memcpy(last_predicted_data, predicted_data, 32);
  }
  cb_rle(run);
}

template <typename CB> void ForAllRLESymbols(size_t length, CB &&cb) {
  if (length == 0)
    return;
  assert(length >= 3);
  constexpr uint32_t kLZ77LengthNbits[259] = {
      0,  0,  0,  10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12,
      12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14,
      14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
      14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17,
      17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
      17, 17, 17, 17, 17, 17, 8};
  constexpr uint32_t kLZ77LengthBits[259] = {
      0,       0,       0,       0x3f,    0x23f,   0x13f,   0x33f,   0xbf,
      0x4bf,   0x2bf,   0x6bf,   0x1bf,   0x9bf,   0x5bf,   0xdbf,   0x3bf,
      0xbbf,   0x7bf,   0xfbf,   0x7f,    0x87f,   0x107f,  0x187f,  0x47f,
      0xc7f,   0x147f,  0x1c7f,  0x27f,   0xa7f,   0x127f,  0x1a7f,  0x67f,
      0xe7f,   0x167f,  0x1e7f,  0x17f,   0x97f,   0x117f,  0x197f,  0x217f,
      0x297f,  0x317f,  0x397f,  0x57f,   0xd7f,   0x157f,  0x1d7f,  0x257f,
      0x2d7f,  0x357f,  0x3d7f,  0x37f,   0xb7f,   0x137f,  0x1b7f,  0x237f,
      0x2b7f,  0x337f,  0x3b7f,  0x77f,   0xf7f,   0x177f,  0x1f7f,  0x277f,
      0x2f7f,  0x377f,  0x3f7f,  0xff,    0x8ff,   0x10ff,  0x18ff,  0x20ff,
      0x28ff,  0x30ff,  0x38ff,  0x40ff,  0x48ff,  0x50ff,  0x58ff,  0x60ff,
      0x68ff,  0x70ff,  0x78ff,  0x4ff,   0xcff,   0x14ff,  0x1cff,  0x24ff,
      0x2cff,  0x34ff,  0x3cff,  0x44ff,  0x4cff,  0x54ff,  0x5cff,  0x64ff,
      0x6cff,  0x74ff,  0x7cff,  0x2ff,   0xaff,   0x12ff,  0x1aff,  0x22ff,
      0x2aff,  0x32ff,  0x3aff,  0x42ff,  0x4aff,  0x52ff,  0x5aff,  0x62ff,
      0x6aff,  0x72ff,  0x7aff,  0x6ff,   0xeff,   0x16ff,  0x1eff,  0x26ff,
      0x2eff,  0x36ff,  0x3eff,  0x46ff,  0x4eff,  0x56ff,  0x5eff,  0x66ff,
      0x6eff,  0x76ff,  0x7eff,  0x1ff,   0x9ff,   0x11ff,  0x19ff,  0x21ff,
      0x29ff,  0x31ff,  0x39ff,  0x41ff,  0x49ff,  0x51ff,  0x59ff,  0x61ff,
      0x69ff,  0x71ff,  0x79ff,  0x81ff,  0x89ff,  0x91ff,  0x99ff,  0xa1ff,
      0xa9ff,  0xb1ff,  0xb9ff,  0xc1ff,  0xc9ff,  0xd1ff,  0xd9ff,  0xe1ff,
      0xe9ff,  0xf1ff,  0xf9ff,  0x5ff,   0xdff,   0x15ff,  0x1dff,  0x25ff,
      0x2dff,  0x35ff,  0x3dff,  0x45ff,  0x4dff,  0x55ff,  0x5dff,  0x65ff,
      0x6dff,  0x75ff,  0x7dff,  0x85ff,  0x8dff,  0x95ff,  0x9dff,  0xa5ff,
      0xadff,  0xb5ff,  0xbdff,  0xc5ff,  0xcdff,  0xd5ff,  0xddff,  0xe5ff,
      0xedff,  0xf5ff,  0xfdff,  0x3ff,   0xbff,   0x13ff,  0x1bff,  0x23ff,
      0x2bff,  0x33ff,  0x3bff,  0x43ff,  0x4bff,  0x53ff,  0x5bff,  0x63ff,
      0x6bff,  0x73ff,  0x7bff,  0x83ff,  0x8bff,  0x93ff,  0x9bff,  0xa3ff,
      0xabff,  0xb3ff,  0xbbff,  0xc3ff,  0xcbff,  0xd3ff,  0xdbff,  0xe3ff,
      0xebff,  0xf3ff,  0xfbff,  0xfff,   0x1fff,  0x2fff,  0x3fff,  0x4fff,
      0x5fff,  0x6fff,  0x7fff,  0x8fff,  0x9fff,  0xafff,  0xbfff,  0xcfff,
      0xdfff,  0xefff,  0xffff,  0x10fff, 0x11fff, 0x12fff, 0x13fff, 0x14fff,
      0x15fff, 0x16fff, 0x17fff, 0x18fff, 0x19fff, 0x1afff, 0x1bfff, 0x1cfff,
      0x1dfff, 0x1efff, 0xc3};

  if (length % 258 == 1 || length % 258 == 2) {
    length -= 3;
    cb(kLZ77LengthNbits[3], kLZ77LengthBits[3]);
  }
  while (length >= 258) {
    cb(kLZ77LengthNbits[258], kLZ77LengthBits[258]);
    length -= 258;
  }
  if (length) {
    cb(kLZ77LengthNbits[length], kLZ77LengthBits[length]);
  }
}

template <size_t pred>
void TryPredictor(size_t bytes_per_line_buf, const unsigned char *mask,
                  unsigned char *current_row_buf, const unsigned char *top_buf,
                  const unsigned char *left_buf,
                  const unsigned char *topleft_buf, size_t &best_cost,
                  uint8_t &predictor, size_t dist_nbits) {
  size_t cost_rle = 0;
  __m256i cost_direct = _mm256_setzero_si256();
  auto cost_chunk_cb = [&](const uint8_t *predicted_data, const uint8_t *mask)
      __attribute__((always_inline)) {

    auto bytes = _mm256_load_si256((__m256i *)predicted_data);

    auto data_for_lut = _mm256_and_si256(_mm256_set1_epi8(0xF), bytes);
    auto data_for_blend = _mm256_and_si256(_mm256_set1_epi8(0xF0), bytes);

    auto nbits_low16 = _mm256_shuffle_epi8(
        _mm256_broadcastsi128_si256(_mm_load_si128((__m128i *)kFirst16Nbits)),
        data_for_lut);
    auto nbits_hi16 = _mm256_shuffle_epi8(
        _mm256_broadcastsi128_si256(_mm_load_si128((__m128i *)kLast16Nbits)),
        data_for_lut);

    auto nbits = _mm256_set1_epi8(kMidNbits);

    nbits = _mm256_blendv_epi8(
        nbits, nbits_hi16,
        _mm256_cmpeq_epi8(data_for_blend, _mm256_set1_epi8(0xF0)));

    nbits = _mm256_blendv_epi8(
        nbits, nbits_low16,
        _mm256_cmpeq_epi8(data_for_blend, _mm256_setzero_si256()));

    nbits = _mm256_and_si256(nbits, _mm256_load_si256((__m256i *)mask));

    cost_direct = _mm256_add_epi32(
        cost_direct, _mm256_sad_epu8(nbits, _mm256_setzero_si256()));
  };
  ProcessRow<pred>(
      bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf, topleft_buf,
      cost_chunk_cb, [](size_t, const uint8_t *, const uint8_t *, size_t) {},
      [&](size_t run) {
        ForAllRLESymbols(run, [&](size_t nbits, size_t bits) {
          cost_rle += dist_nbits + nbits;
        });
      });
  size_t cost = cost_rle + hadd(cost_direct);
  if (cost < best_cost) {
    best_cost = cost;
    predictor = pred;
  }
}

// Either bits_hi is empty, or bits_lo contains exactly 6 (kMidNbits - 4) bits.
__attribute__((always_inline)) void
WriteBits(__m256i nbits, __m256i bits_lo, __m256i bits_hi, BitWriter *writer) {

  // Merge bits_lo and bits_hi in 16-bit "bits".
  auto nbits0 = _mm256_unpacklo_epi8(nbits, _mm256_setzero_si256());
  auto nbits1 = _mm256_unpackhi_epi8(nbits, _mm256_setzero_si256());
  auto bits_lo0 = _mm256_unpacklo_epi8(bits_lo, _mm256_setzero_si256());
  auto bits_lo1 = _mm256_unpackhi_epi8(bits_lo, _mm256_setzero_si256());
  auto bits_hi0 = _mm256_unpacklo_epi8(bits_hi, _mm256_setzero_si256());
  auto bits_hi1 = _mm256_unpackhi_epi8(bits_hi, _mm256_setzero_si256());

  auto bits0 = _mm256_or_si256(
      _mm256_mullo_epi16(_mm256_set1_epi16(1 << (kMidNbits - 4)), bits_hi0),
      bits_lo0);
  auto bits1 = _mm256_or_si256(
      _mm256_mullo_epi16(_mm256_set1_epi16(1 << (kMidNbits - 4)), bits_hi1),
      bits_lo1);

  // 16 -> 32
  auto nbits0_32_lo = _mm256_and_si256(nbits0, _mm256_set1_epi32(0xFF));
  auto nbits1_32_lo = _mm256_and_si256(nbits1, _mm256_set1_epi32(0xFF));
  auto nbits0_32_hi = _mm256_srai_epi32(nbits0, 16);
  auto nbits1_32_hi = _mm256_srai_epi32(nbits1, 16);

  auto bits0_32_lo = _mm256_and_si256(bits0, _mm256_set1_epi32(0xFFFF));
  auto bits1_32_lo = _mm256_and_si256(bits1, _mm256_set1_epi32(0xFFFF));
  auto bits0_32_hi =
      _mm256_sllv_epi32(_mm256_srli_epi32(bits0, 16), nbits0_32_lo);
  auto bits1_32_hi =
      _mm256_sllv_epi32(_mm256_srli_epi32(bits1, 16), nbits1_32_lo);

  auto nbits0_32 = _mm256_add_epi32(nbits0_32_lo, nbits0_32_hi);
  auto nbits1_32 = _mm256_add_epi32(nbits1_32_lo, nbits1_32_hi);
  auto bits0_32 = _mm256_or_si256(bits0_32_lo, bits0_32_hi);
  auto bits1_32 = _mm256_or_si256(bits1_32_lo, bits1_32_hi);

  // 32 -> 64
  auto nbits0_64_lo = _mm256_and_si256(nbits0_32, _mm256_set1_epi64x(0xFF));
  auto nbits1_64_lo = _mm256_and_si256(nbits1_32, _mm256_set1_epi64x(0xFF));
  auto nbits0_64_hi = _mm256_srli_epi64(nbits0_32, 32);
  auto nbits1_64_hi = _mm256_srli_epi64(nbits1_32, 32);

  auto bits0_64_lo = _mm256_and_si256(bits0_32, _mm256_set1_epi64x(0xFFFFFFFF));
  auto bits1_64_lo = _mm256_and_si256(bits1_32, _mm256_set1_epi64x(0xFFFFFFFF));
  auto bits0_64_hi =
      _mm256_sllv_epi64(_mm256_srli_epi64(bits0_32, 32), nbits0_64_lo);
  auto bits1_64_hi =
      _mm256_sllv_epi64(_mm256_srli_epi64(bits1_32, 32), nbits1_64_lo);

  auto nbits0_64 = _mm256_add_epi64(nbits0_64_lo, nbits0_64_hi);
  auto nbits1_64 = _mm256_add_epi64(nbits1_64_lo, nbits1_64_hi);
  auto bits0_64 = _mm256_or_si256(bits0_64_lo, bits0_64_hi);
  auto bits1_64 = _mm256_or_si256(bits1_64_lo, bits1_64_hi);

  // nbits_a <= 40 as we have at most 10 bits per symbol, so the call to the
  // writer is safe.
  alignas(32) uint64_t nbits_a[8];
  _mm256_store_si256((__m256i *)nbits_a, nbits0_64);
  _mm256_store_si256((__m256i *)nbits_a + 1, nbits1_64);
  alignas(32) uint64_t bits_a[8];
  _mm256_store_si256((__m256i *)bits_a, bits0_64);
  _mm256_store_si256((__m256i *)bits_a + 1, bits1_64);

  constexpr uint8_t kPerm[8] = {0, 1, 4, 5, 2, 3, 6, 7};

  // call to WriteBits manually inlined: compiler assumes aliasing may happen.
  uint64_t buffer = writer->buffer;
  uint64_t bits_in_buffer = writer->bits_in_buffer;
  uint64_t bytes_written = writer->bytes_written;
  for (size_t ii = 0; ii < 8; ii++) {
    uint64_t bits = bits_a[kPerm[ii]];
    uint64_t count = nbits_a[kPerm[ii]];
    buffer |= bits << bits_in_buffer;
    bits_in_buffer += count;
    memcpy(writer->data + bytes_written, &buffer, 8);
    size_t bytes_in_buffer = bits_in_buffer / 8;
    bits_in_buffer -= bytes_in_buffer * 8;
    buffer >>= bytes_in_buffer * 8;
    bytes_written += bytes_in_buffer;
  }
  writer->buffer = buffer;
  writer->bits_in_buffer = bits_in_buffer;
  writer->bytes_written = bytes_written;
}

void EncodeOneRow(size_t bytes_per_line_buf,
                  const uint8_t *aligned_adler_mul_buf_ptr,
                  const unsigned char *mask, unsigned char *current_row_buf,
                  const unsigned char *top_buf, const unsigned char *left_buf,
                  const unsigned char *topleft_buf, uint32_t &s1, uint32_t &s2,
                  size_t dist_nbits, size_t dist_bits, BitWriter *writer) {
#ifndef FPNGE_FIXED_PREDICTOR
  uint8_t predictor;
  size_t best_cost = ~0U;
  TryPredictor<1>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, best_cost, predictor, dist_nbits);
  TryPredictor<2>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, best_cost, predictor, dist_nbits);
  TryPredictor<3>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, best_cost, predictor, dist_nbits);
  TryPredictor<4>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, best_cost, predictor, dist_nbits);
#else
  uint8_t predictor = FPNGE_FIXED_PREDICTOR;
#endif

  writer->Write(kFirst16Nbits[predictor], kFirst16Bits[predictor]);
  UpdateAdler32(s1, s2, predictor);

  auto adler_accum_s1 = _mm256_castsi128_si256(_mm_cvtsi32_si128(s1));
  auto adler_accum_s2 = _mm256_castsi128_si256(_mm_cvtsi32_si128(s2));

  size_t last_adler_flush = 0;
  uint16_t len = 1;

  auto flush_adler = [&]() {
    uint32_t ls1 = hadd(adler_accum_s1);
    uint32_t ls2 = hadd(adler_accum_s2);
    ls1 %= kAdler32Mod;
    ls2 %= kAdler32Mod;
    s1 = ls1;
    s2 = ls2;
    adler_accum_s1 = _mm256_castsi128_si256(_mm_cvtsi32_si128(s1));
    adler_accum_s2 = _mm256_castsi128_si256(_mm_cvtsi32_si128(s2));
    last_adler_flush = len;
  };

  auto encode_chunk_cb = [&](const uint8_t *predicted_data,
                             const uint8_t *mask) {
    auto bytes = _mm256_load_si256((__m256i *)predicted_data);
    auto maskv = _mm256_load_si256((__m256i *)mask);

    auto data_for_lut = _mm256_and_si256(_mm256_set1_epi8(0xF), bytes);
    auto data_for_blend = _mm256_and_si256(_mm256_set1_epi8(0xF0), bytes);
    auto data_for_midlut =
        _mm256_and_si256(_mm256_set1_epi8(0xF), _mm256_srai_epi16(bytes, 4));

    auto nbits_low16 = _mm256_shuffle_epi8(
        _mm256_broadcastsi128_si256(_mm_load_si128((__m128i *)kFirst16Nbits)),
        data_for_lut);
    auto nbits_hi16 = _mm256_shuffle_epi8(
        _mm256_broadcastsi128_si256(_mm_load_si128((__m128i *)kLast16Nbits)),
        data_for_lut);

    auto bits_low16 = _mm256_shuffle_epi8(
        _mm256_broadcastsi128_si256(_mm_load_si128((__m128i *)kFirst16Bits)),
        data_for_lut);
    auto bits_hi16 = _mm256_shuffle_epi8(
        _mm256_broadcastsi128_si256(_mm_load_si128((__m128i *)kLast16Bits)),
        data_for_lut);
    auto bits_mid_lo = _mm256_shuffle_epi8(
        _mm256_broadcastsi128_si256(_mm_load_si128((__m128i *)kMidLowBits)),
        data_for_midlut);

    auto bits_mid_hi =
        _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(_mm_load_si128(
                                (__m128i *)kBitReverseNibbleLookup)),
                            data_for_lut);

    auto nbits = _mm256_set1_epi8(kMidNbits);

    nbits = _mm256_blendv_epi8(
        nbits, nbits_hi16,
        _mm256_cmpeq_epi8(data_for_blend, _mm256_set1_epi8(0xF0)));

    nbits = _mm256_blendv_epi8(
        nbits, nbits_low16,
        _mm256_cmpeq_epi8(data_for_blend, _mm256_setzero_si256()));

    nbits = _mm256_and_si256(nbits, maskv);

    auto bits_lo = _mm256_blendv_epi8(
        bits_mid_lo, bits_hi16,
        _mm256_cmpeq_epi8(data_for_blend, _mm256_set1_epi8(0xF0)));

    bits_lo = _mm256_blendv_epi8(
        bits_lo, bits_low16,
        _mm256_cmpeq_epi8(data_for_blend, _mm256_setzero_si256()));

    bits_lo = _mm256_and_si256(bits_lo, maskv);

    auto bits_hi = _mm256_and_si256(
        bits_mid_hi, _mm256_cmpeq_epi8(nbits, _mm256_set1_epi8(kMidNbits)));
    bits_hi = _mm256_and_si256(bits_hi, maskv);

    WriteBits(nbits, bits_lo, bits_hi, writer);
  };

  auto adler_chunk_cb = [&](size_t bytes_per_32, const uint8_t *predicted_data,
                            const uint8_t *mask, size_t i) {
    len += bytes_per_32;

    adler_accum_s2 = _mm256_add_epi32(
        _mm256_mullo_epi32(_mm256_set1_epi32(bytes_per_32), adler_accum_s1),
        adler_accum_s2);

    auto bytes = _mm256_and_si256(_mm256_load_si256((__m256i *)predicted_data),
                                  _mm256_load_si256((__m256i *)mask));

    adler_accum_s1 = _mm256_add_epi32(
        adler_accum_s1, _mm256_sad_epu8(bytes, _mm256_setzero_si256()));

    auto muls = _mm256_load_si256((__m256i *)(aligned_adler_mul_buf_ptr + i));
    auto bytesmuls = _mm256_maddubs_epi16(bytes, muls);
    adler_accum_s2 = _mm256_add_epi32(
        adler_accum_s2, _mm256_madd_epi16(bytesmuls, _mm256_set1_epi16(1)));

    if (len >= 5500 + last_adler_flush) {
      flush_adler();
    }
  };

  auto encode_rle_cb = [&](size_t run) {
    ForAllRLESymbols(run, [&](size_t nbits, size_t bits) {
      writer->Write(nbits, bits);
      writer->Write(dist_nbits, dist_bits);
    });
  };

  if (predictor == 0) {
    ProcessRow<0>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, encode_chunk_cb, adler_chunk_cb, encode_rle_cb);
  } else if (predictor == 1) {
    ProcessRow<1>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, encode_chunk_cb, adler_chunk_cb, encode_rle_cb);
  } else if (predictor == 2) {
    ProcessRow<2>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, encode_chunk_cb, adler_chunk_cb, encode_rle_cb);
  } else if (predictor == 3) {
    ProcessRow<3>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, encode_chunk_cb, adler_chunk_cb, encode_rle_cb);
  } else if (predictor == 4) {
    ProcessRow<4>(bytes_per_line_buf, mask, current_row_buf, top_buf, left_buf,
                  topleft_buf, encode_chunk_cb, adler_chunk_cb, encode_rle_cb);
  }

  flush_adler();
}

void AppendBE32(size_t value, BitWriter *writer) {
  writer->Write(8, value >> 24);
  writer->Write(8, (value >> 16) & 0xFF);
  writer->Write(8, (value >> 8) & 0xFF);
  writer->Write(8, value & 0xFF);
}

void WriteHeader(size_t width, size_t height, size_t bytes_per_channel,
                 size_t num_channels, BitWriter *writer) {
  constexpr uint8_t kPNGHeader[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  for (size_t i = 0; i < 8; i++) {
    writer->Write(8, kPNGHeader[i]);
  }
  // Length
  writer->Write(32, 0x0d000000);
  assert(writer->bits_in_buffer == 0);
  size_t crc_start = writer->bytes_written;
  // IHDR
  writer->Write(32, 0x52444849);
  AppendBE32(width, writer);
  AppendBE32(height, writer);
  // Bit depth
  writer->Write(8, bytes_per_channel * 8);
  // Colour type
  constexpr uint8_t numc_to_colour_type[] = {0, 0, 4, 2, 6};
  writer->Write(8, numc_to_colour_type[num_channels]);
  // Compression, filter and interlace methods.
  writer->Write(24, 0);
  assert(writer->bits_in_buffer == 0);
  size_t crc_end = writer->bytes_written;
  uint32_t crc = compute_crc(writer->data + crc_start, crc_end - crc_start);
  AppendBE32(crc, writer);
}

size_t FPNGEEncode(size_t bytes_per_channel, size_t num_channels,
                   const unsigned char *data, size_t width, size_t row_stride,
                   size_t height, unsigned char **output) {
  assert(bytes_per_channel == 1 || bytes_per_channel == 2);
  assert(num_channels != 0 && num_channels <= 4);
  size_t bytes_per_line = bytes_per_channel * num_channels * width;
  assert(row_stride >= bytes_per_line);

  // allows for padding, and for extra initial space for the "left" pixel for
  // predictors.
  size_t bytes_per_line_buf =
      (bytes_per_channel * 4 * width + 4 * bytes_per_channel + 31) / 32 * 32;

  // Extra space for alignment purposes.
  std::vector<unsigned char> buf(bytes_per_line_buf * 2 + 31 +
                                 4 * bytes_per_channel);
  unsigned char *aligned_buf_ptr = buf.data() + 4 * bytes_per_channel;
  aligned_buf_ptr += (intptr_t)aligned_buf_ptr % 32
                         ? (32 - (intptr_t)aligned_buf_ptr % 32)
                         : 0;

  std::vector<unsigned char> mask_buf(bytes_per_line_buf + 31);
  unsigned char *aligned_mask_buf_ptr = mask_buf.data();
  aligned_mask_buf_ptr += (intptr_t)aligned_mask_buf_ptr % 32
                              ? (32 - (intptr_t)aligned_mask_buf_ptr % 32)
                              : 0;

  std::vector<unsigned char> adler_mul_buf(bytes_per_line_buf + 31);
  unsigned char *aligned_adler_mul_buf_ptr = adler_mul_buf.data();
  aligned_adler_mul_buf_ptr +=
      (intptr_t)aligned_adler_mul_buf_ptr % 32
          ? (32 - (intptr_t)aligned_adler_mul_buf_ptr % 32)
          : 0;

  // Initialize the mask & adler multipliers data.
  if (bytes_per_channel == 1 && num_channels == 3) {
    for (size_t i = 0; i < width; i++) {
      aligned_mask_buf_ptr[4 * i + 0] = 0xFF;
      aligned_mask_buf_ptr[4 * i + 1] = 0xFF;
      aligned_mask_buf_ptr[4 * i + 2] = 0xFF;
      aligned_mask_buf_ptr[4 * i + 3] = 0x00;
    }
    for (size_t i = 0; i < width; i++) {
      aligned_adler_mul_buf_ptr[4 * i + 0] = 1;
      aligned_adler_mul_buf_ptr[4 * i + 1] = 1;
      aligned_adler_mul_buf_ptr[4 * i + 2] = 1;
      aligned_adler_mul_buf_ptr[4 * i + 3] = 0;
    }
  } else if (bytes_per_channel == 1 && num_channels == 4) {
    for (size_t i = 0; i < width; i++) {
      aligned_mask_buf_ptr[4 * i + 0] = 0xFF;
      aligned_mask_buf_ptr[4 * i + 1] = 0xFF;
      aligned_mask_buf_ptr[4 * i + 2] = 0xFF;
      aligned_mask_buf_ptr[4 * i + 3] = 0xFF;
    }
    for (size_t i = 0; i < width; i++) {
      aligned_adler_mul_buf_ptr[4 * i + 0] = 1;
      aligned_adler_mul_buf_ptr[4 * i + 1] = 1;
      aligned_adler_mul_buf_ptr[4 * i + 2] = 1;
      aligned_adler_mul_buf_ptr[4 * i + 3] = 1;
    }
  } else {
    assert(false);
  }
  for (size_t i = 0; i < 4 * bytes_per_channel * width; i += 32) {
    for (size_t ii = 31; ii-- > 0;) {
      aligned_adler_mul_buf_ptr[i + ii] +=
          aligned_adler_mul_buf_ptr[i + ii + 1];
    }
  }

  // likely an overestimate
  *output = (unsigned char *)malloc(1024 + 2 * bytes_per_line * height);

  BitWriter writer;
  writer.data = *output;

  WriteHeader(width, height, bytes_per_channel, num_channels, &writer);

  assert(writer.bits_in_buffer == 0);
  size_t chunk_length_pos = writer.bytes_written;
  writer.bytes_written += 4; // Skip space for length.
  size_t crc_pos = writer.bytes_written;
  writer.Write(32, 0x54414449); // IDAT
  // Deflate header
  writer.Write(8, 8);  // deflate with smallest window
  writer.Write(8, 29); // cfm+flg check value

  // Single block, dynamic huffman
  writer.Write(3, 0b101);
  uint32_t dist_nbits;
  uint32_t dist_bits;
  WriteHuffmanCode(num_channels, dist_nbits, dist_bits, &writer);

  uint32_t crc = ~0U;
  uint32_t s1 = 1;
  uint32_t s2 = 0;
  for (size_t y = 0; y < height; y++) {
    const unsigned char *current_row_in = data + row_stride * y;
    unsigned char *current_row_buf =
        aligned_buf_ptr + (y % 2 ? bytes_per_line_buf : 0);
    const unsigned char *top_buf =
        aligned_buf_ptr + ((y + 1) % 2 ? bytes_per_line_buf : 0);
    const unsigned char *left_buf = current_row_buf - bytes_per_channel * 4;
    const unsigned char *topleft_buf = top_buf - bytes_per_channel * 4;

    if (bytes_per_channel == 1 && num_channels == 3) {
      for (size_t i = 0; i < width; i++) {
        current_row_buf[4 * i + 0] = current_row_in[3 * i + 0];
        current_row_buf[4 * i + 1] = current_row_in[3 * i + 1];
        current_row_buf[4 * i + 2] = current_row_in[3 * i + 2];
        current_row_buf[4 * i + 3] = 0;
      }
    } else if (bytes_per_channel == 1 && num_channels == 4) {
      for (size_t i = 0; i < width; i++) {
        current_row_buf[4 * i + 0] = current_row_in[4 * i + 0];
        current_row_buf[4 * i + 1] = current_row_in[4 * i + 1];
        current_row_buf[4 * i + 2] = current_row_in[4 * i + 2];
        current_row_buf[4 * i + 3] = current_row_in[4 * i + 3];
      }
    } else {
      assert(false);
    }

    EncodeOneRow(bytes_per_line_buf, aligned_adler_mul_buf_ptr,
                 aligned_mask_buf_ptr, current_row_buf, top_buf, left_buf,
                 topleft_buf, s1, s2, dist_nbits, dist_bits, &writer);

    size_t bytes = (writer.bytes_written - crc_pos) / 64 * 64;
    crc = update_crc(crc, writer.data + crc_pos, bytes);
    crc_pos += bytes;
  }

  // EOB
  writer.Write(12, 0x7ff);

  writer.ZeroPadToByte();
  assert(writer.bits_in_buffer == 0);
  s1 %= kAdler32Mod;
  s2 %= kAdler32Mod;
  uint32_t adler32 = (s2 << 16) | s1;
  AppendBE32(adler32, &writer);

  size_t data_len = writer.bytes_written - chunk_length_pos - 8;
  writer.data[chunk_length_pos + 0] = data_len >> 24;
  writer.data[chunk_length_pos + 1] = (data_len >> 16) & 0xFF;
  writer.data[chunk_length_pos + 2] = (data_len >> 8) & 0xFF;
  writer.data[chunk_length_pos + 3] = data_len & 0xFF;

  crc = update_crc(crc, writer.data + crc_pos, writer.bytes_written - crc_pos);
  crc_pos = writer.bytes_written;
  crc ^= ~0U;
  AppendBE32(crc, &writer);

  // IEND
  writer.Write(32, 0);
  writer.Write(32, 0x444e4549);
  writer.Write(32, 0x826042ae);

  return writer.bytes_written;
}

#endif // CAN_COMPILE_FPNGE
