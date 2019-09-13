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

#ifndef __etc1_h__
#define __etc1_h__

#define ETC1_ENCODED_BLOCK_SIZE 8
#define ETC1_DECODED_BLOCK_SIZE 64

typedef unsigned char etc1_byte;
typedef int etc1_bool;
typedef unsigned int etc1_uint32;

// Encode a block of pixels.
//
// pIn is a pointer to a ETC_DECODED_BLOCK_SIZE array of bytes that represent a
// 4 x 4 square of 4-byte pixels in form R, G, B, A. Byte (4 * (x + 4 * y) is the R
// value of pixel (x, y).
//
// validPixelMask is a 16-bit mask where bit (1 << (x + y * 4)) indicates whether
// the corresponding (x,y) pixel is valid. Invalid pixel color values are ignored when compressing.
//
// pOut is an ETC1 compressed version of the data.

//void encode_block_etc1(etc1_byte* pOut, const etc1_byte* pIn, etc1_uint32 validPixelMask);

// Decode a block of pixels.
//
// pIn is an ETC1 compressed version of the data.
//
// pOut is a pointer to a ETC_DECODED_BLOCK_SIZE array of bytes that represent a
// 4 x 4 square of 3-byte pixels in form R, G, B. Byte (3 * (x + 4 * y) is the R
// value of pixel (x, y).

//void decode_block_etc1(etc1_byte* pOut, const etc1_byte* pIn, int stride);

#endif
