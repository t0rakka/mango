/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  jpeg.c - JPEG compression for SRV-1 robot
 *    Copyright (C) 2005-2009  Surveyor Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details (www.gnu.org/licenses)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//
// This file contains optimizations and other changes (C) Frank Van Hooft 2009
//

//
// The code has been modified for integration with MANGO image encode/decode and streaming API.
//

#include <mango/core/pointer.hpp>
#include "jpeg.hpp"
#include <cstring>

namespace
{
    using namespace mango;
    using namespace jpeg;

    constexpr int BLOCK_SIZE = 64;

    static SampleFormat g_format_table [] =
    {
        { JPEG_U8_Y,    LuminanceFormat(8, Format::UNORM, 8, 0) },
        { JPEG_U8_BGR,  Format(24, Format::UNORM, Format::BGR, 8, 8, 8) },
        { JPEG_U8_RGB,  Format(24, Format::UNORM, Format::RGB, 8, 8, 8) },
        { JPEG_U8_BGRA, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8) },
        { JPEG_U8_RGBA, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8) },
    };

    const u16 g_luminance_dc_code_table [] =
    {
        0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006,
        0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE
    };

    const u16 g_luminance_dc_size_table [] =
    {
        0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
        0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009
    };

    const u16 g_chrominance_dc_code_table [] =
    {
        0x0000, 0x0001, 0x0002, 0x0006, 0x000E, 0x001E,
        0x003E, 0x007E, 0x00FE, 0x01FE, 0x03FE, 0x07FE
    };

    const u16 g_chrominance_dc_size_table [] =
    {
        0x0002, 0x0002, 0x0002, 0x0003, 0x0004, 0x0005,
        0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B
    };

    const u16 g_luminance_ac_code_table [] =
    {
        0x000A,
        0x0000, 0x0001, 0x0004, 0x000B, 0x001A, 0x0078, 0x00F8, 0x03F6, 0xFF82, 0xFF83,
        0x000C, 0x001B, 0x0079, 0x01F6, 0x07F6, 0xFF84, 0xFF85, 0xFF86, 0xFF87, 0xFF88,
        0x001C, 0x00F9, 0x03F7, 0x0FF4, 0xFF89, 0xFF8A, 0xFF8b, 0xFF8C, 0xFF8D, 0xFF8E,
        0x003A, 0x01F7, 0x0FF5, 0xFF8F, 0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95,
        0x003B, 0x03F8, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D,
        0x007A, 0x07F7, 0xFF9E, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5,
        0x007B, 0x0FF6, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD,
        0x00FA, 0x0FF7, 0xFFAE, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5,
        0x01F8, 0x7FC0, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD,
        0x01F9, 0xFFBE, 0xFFBF, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6,
        0x01FA, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
        0x03F9, 0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8,
        0x03FA, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1,
        0x07F8, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA,
        0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4,
        0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE,
        0x07F9
    };

    const u16 g_luminance_ac_size_table [] =
    {
        0x0004,
        0x0002, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0008, 0x000A, 0x0010, 0x0010,
        0x0004, 0x0005, 0x0007, 0x0009, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0005, 0x0008, 0x000A, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0006, 0x0009, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0006, 0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0007, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0008, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0009, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000B
    };

    const u16 g_chrominance_ac_code_table [] =
    {
        0x0000,
        0x0001, 0x0004, 0x000A, 0x0018, 0x0019, 0x0038, 0x0078, 0x01F4, 0x03F6, 0x0FF4,
        0x000B, 0x0039, 0x00F6, 0x01F5, 0x07F6, 0x0FF5, 0xFF88, 0xFF89, 0xFF8A, 0xFF8B,
        0x001A, 0x00F7, 0x03F7, 0x0FF6, 0x7FC2, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F, 0xFF90,
        0x001B, 0x00F8, 0x03F8, 0x0FF7, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95, 0xFF96,
        0x003A, 0x01F6, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E,
        0x003B, 0x03F9, 0xFF9F, 0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5, 0xFFA6,
        0x0079, 0x07F7, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE,
        0x007A, 0x07F8, 0xFFAF, 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6,
        0x00F9, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD, 0xFFBE, 0xFFBF,
        0x01F7, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8,
        0x01F8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF, 0xFFD0, 0xFFD1,
        0x01F9, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFDA,
        0x01FA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF, 0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3,
        0x07F9, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEb, 0xFFEC,
        0x3FE0, 0xFFED, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5,
        0x7FC3, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE,
        0x03FA
    };

    const u16 g_chrominance_ac_size_table [] =
    {
        0x0002,
        0x0002, 0x0003, 0x0004, 0x0005, 0x0005, 0x0006, 0x0007, 0x0009, 0x000A, 0x000C,
        0x0004, 0x0006, 0x0008, 0x0009, 0x000B, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0005, 0x0008, 0x000A, 0x000C, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0005, 0x0008, 0x000A, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0006, 0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0006, 0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0007, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0008, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000E, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000A
    };

    const u8 marker_data [] =
    {
        0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
        0xFF, 0xC4, 0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
		0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25,
		0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67,
		0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
		0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
		0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
        0xFF, 0xC4, 0x00, 0x1F, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
        0xFF, 0xC4, 0x00, 0xB5, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
		0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18,
		0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66,
		0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4,
		0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
		0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
    };

    const u8 zigzag_table [] =
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

    const u8 zigzag_table_inverse [] =
    {
         0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63,
    };

    const u8 luminance_quant_table [] =
    {
        16, 11, 10, 16,  24,  40,  51,  61,
        12, 12, 14, 19,  26,  58,  60,  55,
        14, 13, 16, 24,  40,  57,  69,  56,
        14, 17, 22, 29,  51,  87,  80,  62,
        18, 22, 37, 56,  68, 109, 103,  77,
        24, 35, 55, 64,  81, 104, 113,  92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103,  99
    };

    const u8 chrominance_quant_table [] =
    {
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99
    };

#if defined(MANGO_ENABLE_LZCNT)

    static inline
    u32 getSymbolSize(int value)
    {
        return u32_log2(value) + 1;
    }

#else

    const u8 g_log2_4bit_table [] =
    {
        0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    };

    static inline
    u32 getSymbolSize(u32 value)
    {
        /* branchless binary search
        u32 base = 0;
        base += 8 * ((value >> base) > 0xff);
        base += 4 * ((value >> base) > 0x0f);
        base += 2 * ((value >> base) > 0x03);
        base += 1 * ((value >> base) > 0x01);
        return base;
        */
        u32 base = 8 * (value > 255);
        base += 4 * ((value >> base) > 15);
        return g_log2_4bit_table[value >> base] + base;
    }

#endif

    static inline
    u8* writeStuffedBytes(u8* output, DataType code, int count)
    {
        code = byteswap(code);
        for (int i = 0; i < count; ++i)
        {
            u8 value = u8(code);
            code >>= 8;
            *output++ = value;

#if 1
            // always write the stuff byte
            // .. but advance ptr only when it actually was one
            // this variant of the code can be branchless (measured: ~5% faster)
            *output = 0;
            output += (value == 0xff);
#else
            if (value == 0xff)
            {
                // write stuff byte
                *output++ = 0;
            }
#endif
        }
        return output;
    }

    struct jpeg_chan
    {
        int     component;
        s16*    qtable;
    };

    struct jpeg_encode
    {
        int     mcu_width;
        int     mcu_height;
        int     horizontal_mcus;
        int     vertical_mcus;
        int     cols_in_right_mcus;
        int     rows_in_bottom_mcus;

        int     mcu_width_size;

        u8      Lqt [BLOCK_SIZE];
        u8      Cqt [BLOCK_SIZE];
        AlignedStorage<s16> ILqt;
        AlignedStorage<s16> ICqt;

        // MCU configuration
        jpeg_chan   channel[3];
        int         channel_count;

        std::string info;

        void (*read_8x8) (s16* block, const u8* input, size_t stride, int rows, int cols);
        void (*read)     (s16* block, const u8* input, size_t stride, int rows, int cols);

        jpeg_encode(SampleType sample, u32 width, u32 height, size_t stride, u32 quality);
        ~jpeg_encode();

        void init_quantization_tables(u32 quality);
        void write_markers(BigEndianStream& p, SampleType sample, u32 width, u32 height);
        
        ConstMemory icc;
    };

    struct EncodeBuffer : Buffer
    {
        std::atomic<bool> ready { false };
    };

    struct HuffmanEncoder
    {
        int last_dc_value[3];

        DataType code;
        int space;

        HuffmanEncoder()
        {
            last_dc_value[0] = 0;
            last_dc_value[1] = 0;
            last_dc_value[2] = 0;
            code = 0;
            space = JPEG_REGISTER_BITS;
        }

        ~HuffmanEncoder()
        {
        }

        u8* putBits(u8* output, DataType data, int numbits)
        {
            if (space >= numbits)
            {
                space -= numbits;
                code |= (data << space);
            }
            else
            {
                int overflow = numbits - space;
                code |= (data >> overflow);
                output = writeStuffedBytes(output, code, JPEG_REGISTER_BYTES);
                space = JPEG_REGISTER_BITS - overflow;
                code = data << space;
            }
            return output;
        }

        u8* flush(u8* output)
        {
            int count = ((JPEG_REGISTER_BITS - space) + 7) >> 3;
            output = writeStuffedBytes(output, code, count);
            return output;
        }

        u8* encode(u8* p, int component, const s16* input)
        {
            struct
            {
                const u16* code;
                const u16* size;
            } dc, ac;

            if (component != 0)
            {
                dc.code = g_chrominance_dc_code_table;
                dc.size = g_chrominance_dc_size_table;
                ac.code = g_chrominance_ac_code_table;
                ac.size = g_chrominance_ac_size_table;
            }
            else
            {
                dc.code = g_luminance_dc_code_table;
                dc.size = g_luminance_dc_size_table;
                ac.code = g_luminance_ac_code_table;
                ac.size = g_luminance_ac_size_table;
            }

            int coeff = input[0] - last_dc_value[component];
            last_dc_value[component] = input[0];

            u32 absCoeff = (coeff < 0) ? -coeff-- : coeff;
            u32 dataSize = getSymbolSize(absCoeff);
            u32 dataMask = (1 << dataSize) - 1;

            p = putBits(p, dc.code[dataSize], dc.size[dataSize]);
            p = putBits(p, coeff & dataMask, dataSize);

            int runLength = 0;

            for (int i = 1; i < 64; ++i)
            {
                int coeff = input[zigzag_table_inverse[i]];
                if (coeff)
                {
                    while (runLength > 15)
                    {
                        runLength -= 16;
                        p = putBits(p, ac.code[161], ac.size[161]);
                    }

                    u32 absCoeff = (coeff < 0) ? -coeff-- : coeff;
                    u32 dataSize = getSymbolSize(absCoeff);
                    u32 dataMask = (1 << dataSize) - 1;

                    int index = runLength * 10 + dataSize;
                    p = putBits(p, ac.code[index], ac.size[index]);
                    p = putBits(p, coeff & dataMask, dataSize);

                    runLength = 0;
                }
                else
                {
                    ++runLength;
                }
            }

            if (runLength != 0)
            {
                p = putBits(p, ac.code[0], ac.size[0]);
            }

            return p;
        }
    };

#if defined(JPEG_ENABLE_SSE2)

#if defined(JPEG_ENABLE_AVX2)
    constexpr const char* fdct_name = "AVX2 DCT";
#else
    constexpr const char* fdct_name = "SSE2 DCT";
#endif

    // ----------------------------------------------------------------------------
    // fdct sse2
    // ----------------------------------------------------------------------------

    static inline void interleave16(__m128i& a, __m128i& b)
    {
        __m128i c = a;
        a = _mm_unpacklo_epi16(a, b);
        b = _mm_unpackhi_epi16(c, b);
    }

    #define JPEG_CONST16(x, y) \
        _mm_setr_epi16(x, y, x, y, x, y, x, y)

    #define JPEG_TRANSPOSE16() \
        interleave16(v0, v4); \
        interleave16(v2, v6); \
        interleave16(v1, v5); \
        interleave16(v3, v7); \
        interleave16(v0, v2); \
        interleave16(v1, v3); \
        interleave16(v4, v6); \
        interleave16(v5, v7); \
        interleave16(v0, v1); \
        interleave16(v2, v3); \
        interleave16(v4, v5); \
        interleave16(v6, v7)

    #define JPEG_TRANSFORM(n) { \
        __m128i a_lo; \
        __m128i a_hi; \
        __m128i b_lo; \
        __m128i b_hi; \
        \
        a_lo = _mm_madd_epi16(x87_lo, c26p); \
        a_hi = _mm_madd_epi16(x87_hi, c26p); \
        a_lo = _mm_srai_epi32(a_lo, n); \
        a_hi = _mm_srai_epi32(a_hi, n); \
        v2 = _mm_packs_epi32(a_lo, a_hi); \
        \
        a_lo = _mm_madd_epi16(x87_lo, c62n); \
        a_hi = _mm_madd_epi16(x87_hi, c62n); \
        a_lo = _mm_srai_epi32(a_lo, n); \
        a_hi = _mm_srai_epi32(a_hi, n); \
        v6 = _mm_packs_epi32(a_lo, a_hi); \
        \
        a_lo = _mm_madd_epi16(x01_lo, c75n); \
        a_hi = _mm_madd_epi16(x01_hi, c75n); \
        b_lo = _mm_madd_epi16(x23_lo, c31n); \
        b_hi = _mm_madd_epi16(x23_hi, c31n); \
        a_lo = _mm_add_epi32(a_lo, b_lo); \
        a_hi = _mm_add_epi32(a_hi, b_hi); \
        a_lo = _mm_srai_epi32(a_lo, n); \
        a_hi = _mm_srai_epi32(a_hi, n); \
        v7 = _mm_packs_epi32(a_lo, a_hi); \
        \
        a_lo = _mm_madd_epi16(x01_lo, c51n); \
        a_hi = _mm_madd_epi16(x01_hi, c51n); \
        b_lo = _mm_madd_epi16(x23_lo, c73p); \
        b_hi = _mm_madd_epi16(x23_hi, c73p); \
        a_lo = _mm_add_epi32(a_lo, b_lo); \
        a_hi = _mm_add_epi32(a_hi, b_hi); \
        a_lo = _mm_srai_epi32(a_lo, n); \
        a_hi = _mm_srai_epi32(a_hi, n); \
        v5 = _mm_packs_epi32(a_lo, a_hi); \
        \
        a_lo = _mm_madd_epi16(x01_lo, c37n); \
        a_hi = _mm_madd_epi16(x01_hi, c37n); \
        b_lo = _mm_madd_epi16(x23_lo, c15p); \
        b_hi = _mm_madd_epi16(x23_hi, c15p); \
        a_lo = _mm_sub_epi32(a_lo, b_lo); \
        a_hi = _mm_sub_epi32(a_hi, b_hi); \
        a_lo = _mm_srai_epi32(a_lo, n); \
        a_hi = _mm_srai_epi32(a_hi, n); \
        v3 = _mm_packs_epi32(a_lo, a_hi); \
        \
        a_lo = _mm_madd_epi16(x01_lo, c13p); \
        a_hi = _mm_madd_epi16(x01_hi, c13p); \
        b_lo = _mm_madd_epi16(x23_lo, c57p); \
        b_hi = _mm_madd_epi16(x23_hi, c57p); \
        a_lo = _mm_add_epi32(a_lo, b_lo); \
        a_hi = _mm_add_epi32(a_hi, b_hi); \
        a_lo = _mm_srai_epi32(a_lo, n); \
        a_hi = _mm_srai_epi32(a_hi, n); \
        v1 = _mm_packs_epi32(a_lo, a_hi); }

#if defined(JPEG_ENABLE_AVX2)

    static inline
    __m256i quantize(__m256i v, __m256i q, __m256i one, __m256i bias)
    {
        __m256i lo = _mm256_madd_epi16(_mm256_unpacklo_epi16(v, one), _mm256_unpacklo_epi16(q, bias));
        __m256i hi = _mm256_madd_epi16(_mm256_unpackhi_epi16(v, one), _mm256_unpackhi_epi16(q, bias));
        lo = _mm256_srai_epi32(lo, 15);
        hi = _mm256_srai_epi32(hi, 15);
        v = _mm256_packs_epi32(lo, hi);
        return v;
    }

#else

    static inline
    __m128i quantize(__m128i v, __m128i q, __m128i one, __m128i bias)
    {
        __m128i lo = _mm_madd_epi16(_mm_unpacklo_epi16(v, one), _mm_unpacklo_epi16(q, bias));
        __m128i hi = _mm_madd_epi16(_mm_unpackhi_epi16(v, one), _mm_unpackhi_epi16(q, bias));
        lo = _mm_srai_epi32(lo, 15);
        hi = _mm_srai_epi32(hi, 15);
        v = _mm_packs_epi32(lo, hi);
        return v;
    }

#endif

    static
    void fdct(s16* dest, const s16* data, const s16* quant_table)
    {
        constexpr s16 c1 = 1420; // cos 1PI/16 * root(2)
        constexpr s16 c2 = 1338; // cos 2PI/16 * root(2)
        constexpr s16 c3 = 1204; // cos 3PI/16 * root(2)
        constexpr s16 c5 = 805;  // cos 5PI/16 * root(2)
        constexpr s16 c6 = 554;  // cos 6PI/16 * root(2)
        constexpr s16 c7 = 283;  // cos 7PI/16 * root(2)

#if 0
        const __m128i c00 = _mm_setr_epi16(c2, c6, c6, -c2, c7, -c5, c3, -c1);
        const __m128i c01 = _mm_setr_epi16(c5, -c1, c7, c3, c3, -c7, c1, c5);
        __m128i c26p = _mm_shuffle_epi32(c00, 0x00);
        __m128i c62n = _mm_shuffle_epi32(c00, 0x55);
        __m128i c75n = _mm_shuffle_epi32(c00, 0xaa);
        __m128i c31n = _mm_shuffle_epi32(c00, 0xff);
        __m128i c51n = _mm_shuffle_epi32(c01, 0x00);
        __m128i c73p = _mm_shuffle_epi32(c01, 0x55);
        __m128i c37n = _mm_shuffle_epi32(c01, 0xaa);
        __m128i c15p = _mm_shuffle_epi32(c01, 0xff);
        __m128i c13p = JPEG_CONST16(c1, c3);
        __m128i c57p = JPEG_CONST16(c5, c7);
#else
        __m128i c26p = JPEG_CONST16(c2, c6);
        __m128i c62n = JPEG_CONST16(c6,-c2);
        __m128i c75n = JPEG_CONST16(c7,-c5);
        __m128i c31n = JPEG_CONST16(c3,-c1);
        __m128i c51n = JPEG_CONST16(c5,-c1);
        __m128i c73p = JPEG_CONST16(c7, c3);
        __m128i c37n = JPEG_CONST16(c3,-c7);
        __m128i c15p = JPEG_CONST16(c1, c5);
        __m128i c13p = JPEG_CONST16(c1, c3);
        __m128i c57p = JPEG_CONST16(c5, c7);
#endif

        // load

        const __m128i* s = reinterpret_cast<const __m128i *>(data);
        __m128i v0 = _mm_loadu_si128(s + 0);
        __m128i v1 = _mm_loadu_si128(s + 1);
        __m128i v2 = _mm_loadu_si128(s + 2);
        __m128i v3 = _mm_loadu_si128(s + 3);
        __m128i v4 = _mm_loadu_si128(s + 4);
        __m128i v5 = _mm_loadu_si128(s + 5);
        __m128i v6 = _mm_loadu_si128(s + 6);
        __m128i v7 = _mm_loadu_si128(s + 7);

        // pass 1

        JPEG_TRANSPOSE16();

        __m128i x8 = _mm_add_epi16(v0, v7);
        __m128i x7 = _mm_add_epi16(v1, v6);
        __m128i x6 = _mm_add_epi16(v2, v5);
        __m128i x5 = _mm_add_epi16(v3, v4);
        __m128i x0 = _mm_sub_epi16(v0, v7);
        __m128i x1 = _mm_sub_epi16(v1, v6);
        __m128i x2 = _mm_sub_epi16(v2, v5);
        __m128i x3 = _mm_sub_epi16(v3, v4);
        __m128i x4 = _mm_add_epi16(x8, x5);

        x8 = _mm_sub_epi16(x8, x5);
        x5 = _mm_add_epi16(x7, x6);
        x7 = _mm_sub_epi16(x7, x6);

        __m128i x87_lo = _mm_unpacklo_epi16(x8, x7);
        __m128i x87_hi = _mm_unpackhi_epi16(x8, x7);
        __m128i x01_lo = _mm_unpacklo_epi16(x0, x1);
        __m128i x01_hi = _mm_unpackhi_epi16(x0, x1);
        __m128i x23_lo = _mm_unpacklo_epi16(x2, x3);
        __m128i x23_hi = _mm_unpackhi_epi16(x2, x3);

        v0 = _mm_add_epi16(x4, x5);
        v4 = _mm_sub_epi16(x4, x5);

        JPEG_TRANSFORM(10);

        // pass 2

        JPEG_TRANSPOSE16();

        x8 = _mm_add_epi16(v0, v7);
        x0 = _mm_sub_epi16(v0, v7);
        x7 = _mm_add_epi16(v1, v6);
        x1 = _mm_sub_epi16(v1, v6);
        x6 = _mm_add_epi16(v2, v5);
        x2 = _mm_sub_epi16(v2, v5);
        x5 = _mm_add_epi16(v3, v4);
        x3 = _mm_sub_epi16(v3, v4);
        x4 = _mm_add_epi16(x8, x5);

        x8 = _mm_sub_epi16(x8, x5);
        x5 = _mm_add_epi16(x7, x6);
        x7 = _mm_sub_epi16(x7, x6);

        x87_lo = _mm_unpacklo_epi16(x8, x7);
        x87_hi = _mm_unpackhi_epi16(x8, x7);
        x01_lo = _mm_unpacklo_epi16(x0, x1);
        x01_hi = _mm_unpackhi_epi16(x0, x1);
        x23_lo = _mm_unpacklo_epi16(x2, x3);
        x23_hi = _mm_unpackhi_epi16(x2, x3);

        v0 = _mm_srai_epi16(_mm_add_epi16(x4, x5), 3);
        v4 = _mm_srai_epi16(_mm_sub_epi16(x4, x5), 3);

        JPEG_TRANSFORM(13);

#if defined(JPEG_ENABLE_AVX2)

        __m256i v01 = _mm256_setr_m128i(v0, v1);
        __m256i v23 = _mm256_setr_m128i(v2, v3);
        __m256i v45 = _mm256_setr_m128i(v4, v5);
        __m256i v67 = _mm256_setr_m128i(v6, v7);

        // quantize

        const __m256i one = _mm256_set1_epi16(1);
        const __m256i bias = _mm256_set1_epi16(0x4000);
        const __m256i* q = reinterpret_cast<const __m256i*>(quant_table);

        v01 = quantize(v01, q[0], one, bias);
        v23 = quantize(v23, q[1], one, bias);
        v45 = quantize(v45, q[2], one, bias);
        v67 = quantize(v67, q[3], one, bias);

        // store

        __m256i* d = reinterpret_cast<__m256i *>(dest);
        _mm256_storeu_si256(d + 0, v01);
        _mm256_storeu_si256(d + 1, v23);
        _mm256_storeu_si256(d + 2, v45);
        _mm256_storeu_si256(d + 3, v67);

#else

        // quantize

        const __m128i one = _mm_set1_epi16(1);
        const __m128i bias = _mm_set1_epi16(0x4000);
        const __m128i* q = reinterpret_cast<const __m128i*>(quant_table);

        v0 = quantize(v0, q[0], one, bias);
        v1 = quantize(v1, q[1], one, bias);
        v2 = quantize(v2, q[2], one, bias);
        v3 = quantize(v3, q[3], one, bias);
        v4 = quantize(v4, q[4], one, bias);
        v5 = quantize(v5, q[5], one, bias);
        v6 = quantize(v6, q[6], one, bias);
        v7 = quantize(v7, q[7], one, bias);

        // store

        __m128i* d = reinterpret_cast<__m128i *>(dest);
        _mm_storeu_si128(d + 0, v0);
        _mm_storeu_si128(d + 1, v1);
        _mm_storeu_si128(d + 2, v2);
        _mm_storeu_si128(d + 3, v3);
        _mm_storeu_si128(d + 4, v4);
        _mm_storeu_si128(d + 5, v5);
        _mm_storeu_si128(d + 6, v6);
        _mm_storeu_si128(d + 7, v7);

#endif
    }

#elif defined(JPEG_ENABLE_NEON)

    constexpr const char* fdct_name = "NEON DCT";

    // ----------------------------------------------------------------------------
    // fdct neon
    // ----------------------------------------------------------------------------

    static inline
    void dct_trn16(int16x8_t& x, int16x8_t& y)
    {
        int16x8x2_t t = vtrnq_s16(x, y);
        x = t.val[0];
        y = t.val[1];
    }

    static inline
    void dct_trn32(int16x8_t& x, int16x8_t& y)
    {
        int32x4x2_t t = vtrnq_s32(vreinterpretq_s32_s16(x), vreinterpretq_s32_s16(y));
        x = vreinterpretq_s16_s32(t.val[0]);
        y = vreinterpretq_s16_s32(t.val[1]);
    }

    static inline
    void dct_trn64(int16x8_t& x, int16x8_t& y)
    {
        int16x8_t x0 = x;
        int16x8_t y0 = y;
        x = vcombine_s16(vget_low_s16(x0), vget_low_s16(y0));
        y = vcombine_s16(vget_high_s16(x0), vget_high_s16(y0));
    }

    static inline
    int16x8_t packs_s32(int32x4_t a, int32x4_t b)
    {
        return vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
    }

    static inline
    int16x8_t quantize(int16x8_t v, int16x8_t q, int32x4_t bias)
    {
        int32x4_t lo = vmlal_s16(bias, vget_low_s16(v), vget_low_s16(q));
        int32x4_t hi = vmlal_s16(bias, vget_high_s16(v), vget_high_s16(q));
        lo = vshrq_n_s32(lo, 15);
        hi = vshrq_n_s32(hi, 15);
        v = packs_s32(lo, hi);
        return v;
    }

    #define JPEG_TRANSPOSE16() \
        dct_trn16(v0, v1); \
        dct_trn16(v2, v3); \
        dct_trn16(v4, v5); \
        dct_trn16(v6, v7); \
        dct_trn32(v0, v2); \
        dct_trn32(v1, v3); \
        dct_trn32(v4, v6); \
        dct_trn32(v5, v7); \
        dct_trn64(v0, v4); \
        dct_trn64(v1, v5); \
        dct_trn64(v2, v6); \
        dct_trn64(v3, v7)

    #define JPEG_MUL2(v, x0, c0, x1, c1, f1, n) { \
        int32x4_t lo; \
        int32x4_t hi; \
        lo = vmull_s16(vget_low_s16(x0), c0); \
        lo = vml##f1##l_s16(lo, vget_low_s16(x1), c1); \
        lo = vshrq_n_s32(lo, n); \
        hi = vmull_s16(vget_high_s16(x0), c0); \
        hi = vml##f1##l_s16(hi, vget_high_s16(x1), c1); \
        hi = vshrq_n_s32(hi, n); \
        v = packs_s32(lo, hi); }

    #define JPEG_MUL4(v, x0, c0, x1, c1, x2, c2, x3, c3, f1, f2, f3, n) { \
        int32x4_t lo; \
        int32x4_t hi; \
        lo = vmull_s16(vget_low_s16(x0), c0); \
        lo = vml##f1##l_s16(lo, vget_low_s16(x1), c1); \
        lo = vml##f2##l_s16(lo, vget_low_s16(x2), c2); \
        lo = vml##f3##l_s16(lo, vget_low_s16(x3), c3); \
        lo = vshrq_n_s32(lo, n); \
        hi = vmull_s16(vget_high_s16(x0), c0); \
        hi = vml##f1##l_s16(hi, vget_high_s16(x1), c1); \
        hi = vml##f2##l_s16(hi, vget_high_s16(x2), c2); \
        hi = vml##f3##l_s16(hi, vget_high_s16(x3), c3); \
        hi = vshrq_n_s32(hi, n); \
        v = packs_s32(lo, hi); }

    #define JPEG_TRANSFORM(n) \
        JPEG_MUL2(v2, x8, c2, x7, c6, a, n); \
        JPEG_MUL2(v6, x8, c6, x7, c2, s, n); \
        JPEG_MUL4(v7, x0, c7, x1, c5, x2, c3, x3, c1, s, a, s, n); \
        JPEG_MUL4(v5, x0, c5, x1, c1, x2, c7, x3, c3, s, a, a, n); \
        JPEG_MUL4(v3, x0, c3, x1, c7, x2, c1, x3, c5, s, s, s, n); \
        JPEG_MUL4(v1, x0, c1, x1, c3, x2, c5, x3, c7, a, a, a, n);

    static
    void fdct(s16* dest, const s16* data, const s16* quant_table)
    {
        const int16x4_t c1 = vdup_n_s16(1420); // cos 1PI/16 * root(2)
        const int16x4_t c2 = vdup_n_s16(1338); // cos 2PI/16 * root(2)
        const int16x4_t c3 = vdup_n_s16(1204); // cos 3PI/16 * root(2)
        const int16x4_t c5 = vdup_n_s16(805);  // cos 5PI/16 * root(2)
        const int16x4_t c6 = vdup_n_s16(554);  // cos 6PI/16 * root(2)
        const int16x4_t c7 = vdup_n_s16(283);  // cos 7PI/16 * root(2)

        // load

        int16x8_t v0 = vld1q_s16(data + 0 * 8);
        int16x8_t v1 = vld1q_s16(data + 1 * 8);
        int16x8_t v2 = vld1q_s16(data + 2 * 8);
        int16x8_t v3 = vld1q_s16(data + 3 * 8);
        int16x8_t v4 = vld1q_s16(data + 4 * 8);
        int16x8_t v5 = vld1q_s16(data + 5 * 8);
        int16x8_t v6 = vld1q_s16(data + 6 * 8);
        int16x8_t v7 = vld1q_s16(data + 7 * 8);

        // pass 1

        JPEG_TRANSPOSE16();

        int16x8_t x8 = vaddq_s16(v0, v7);
        int16x8_t x0 = vsubq_s16(v0, v7);
        int16x8_t x7 = vaddq_s16(v1, v6);
        int16x8_t x1 = vsubq_s16(v1, v6);
        int16x8_t x6 = vaddq_s16(v2, v5);
        int16x8_t x2 = vsubq_s16(v2, v5);
        int16x8_t x5 = vaddq_s16(v3, v4);
        int16x8_t x3 = vsubq_s16(v3, v4);
        int16x8_t x4 = vaddq_s16(x8, x5);
        x8 = vsubq_s16(x8, x5);
        x5 = vaddq_s16(x7, x6);
        x7 = vsubq_s16(x7, x6);

        v0 = vaddq_s16(x4, x5);
        v4 = vsubq_s16(x4, x5);

        JPEG_TRANSFORM(10);

        // pass 2

        JPEG_TRANSPOSE16();

        x8 = vaddq_s16(v0, v7);
        x0 = vsubq_s16(v0, v7);
        x7 = vaddq_s16(v1, v6);
        x1 = vsubq_s16(v1, v6);
        x6 = vaddq_s16(v2, v5);
        x2 = vsubq_s16(v2, v5);
        x5 = vaddq_s16(v3, v4);
        x3 = vsubq_s16(v3, v4);
        x4 = vaddq_s16(x8, x5);
        x8 = vsubq_s16(x8, x5);
        x5 = vaddq_s16(x7, x6);
        x7 = vsubq_s16(x7, x6);

        v0 = vshrq_n_s16(vaddq_s16(x4, x5), 3);
        v4 = vshrq_n_s16(vsubq_s16(x4, x5), 3);

        JPEG_TRANSFORM(13);

        // quantize

        const int32x4_t bias = vdupq_n_s32(0x4000);
        const int16x8_t* q = reinterpret_cast<const int16x8_t *>(quant_table);

        v0 = quantize(v0, q[0], bias);
        v1 = quantize(v1, q[1], bias);
        v2 = quantize(v2, q[2], bias);
        v3 = quantize(v3, q[3], bias);
        v4 = quantize(v4, q[4], bias);
        v5 = quantize(v5, q[5], bias);
        v6 = quantize(v6, q[6], bias);
        v7 = quantize(v7, q[7], bias);

        // store

        vst1q_s16(dest + 0 * 8, v0);
        vst1q_s16(dest + 1 * 8, v1);
        vst1q_s16(dest + 2 * 8, v2);
        vst1q_s16(dest + 3 * 8, v3);
        vst1q_s16(dest + 4 * 8, v4);
        vst1q_s16(dest + 5 * 8, v5);
        vst1q_s16(dest + 6 * 8, v6);
        vst1q_s16(dest + 7 * 8, v7);
    }

#else

    constexpr const char* fdct_name = "Scalar DCT";

    // ----------------------------------------------------------------------------
    // fdct scalar
    // ----------------------------------------------------------------------------

    static
    void fdct(s16* dest, const s16* data, const s16* quant_table)
    {
        constexpr s16 c1 = 1420; // cos 1PI/16 * root(2)
        constexpr s16 c2 = 1338; // cos 2PI/16 * root(2)
        constexpr s16 c3 = 1204; // cos 3PI/16 * root(2)
        constexpr s16 c5 = 805;  // cos 5PI/16 * root(2)
        constexpr s16 c6 = 554;  // cos 6PI/16 * root(2)
        constexpr s16 c7 = 283;  // cos 7PI/16 * root(2)

        s16 temp[64];

        for (int i = 0; i < 8; ++i)
        {
            s16 x8 = data [0] + data [7];
            s16 x0 = data [0] - data [7];
            s16 x7 = data [1] + data [6];
            s16 x1 = data [1] - data [6];
            s16 x6 = data [2] + data [5];
            s16 x2 = data [2] - data [5];
            s16 x5 = data [3] + data [4];
            s16 x3 = data [3] - data [4];
            s16 x4 = x8 + x5;
            x8 = x8 - x5;
            x5 = x7 + x6;
            x7 = x7 - x6;
            temp[i * 8 + 0] = x4 + x5;
            temp[i * 8 + 4] = x4 - x5;
            temp[i * 8 + 2] = (x8 * c2 + x7 * c6) >> 10;
            temp[i * 8 + 6] = (x8 * c6 - x7 * c2) >> 10;
            temp[i * 8 + 7] = (x0 * c7 - x1 * c5 + x2 * c3 - x3 * c1) >> 10;
            temp[i * 8 + 5] = (x0 * c5 - x1 * c1 + x2 * c7 + x3 * c3) >> 10;
            temp[i * 8 + 3] = (x0 * c3 - x1 * c7 - x2 * c1 - x3 * c5) >> 10;
            temp[i * 8 + 1] = (x0 * c1 + x1 * c3 + x2 * c5 + x3 * c7) >> 10;
            data += 8;
        }

        for (int i = 0; i < 8; ++i)
        {
            s16 x8 = temp [i +  0] + temp [i + 56];
            s16 x0 = temp [i +  0] - temp [i + 56];
            s16 x7 = temp [i +  8] + temp [i + 48];
            s16 x1 = temp [i +  8] - temp [i + 48];
            s16 x6 = temp [i + 16] + temp [i + 40];
            s16 x2 = temp [i + 16] - temp [i + 40];
            s16 x5 = temp [i + 24] + temp [i + 32];
            s16 x3 = temp [i + 24] - temp [i + 32];
            s16 x4 = x8 + x5;
            x8 = x8 - x5;
            x5 = x7 + x6;
            x7 = x7 - x6;
            s16 v0 = (x4 + x5) >> 3;
            s16 v4 = (x4 - x5) >> 3;
            s16 v2 = (x8 * c2 + x7 * c6) >> 13;
            s16 v6 = (x8 * c6 - x7 * c2) >> 13;
            s16 v7 = (x0 * c7 - x1 * c5 + x2 * c3 - x3 * c1) >> 13;
            s16 v5 = (x0 * c5 - x1 * c1 + x2 * c7 + x3 * c3) >> 13;
            s16 v3 = (x0 * c3 - x1 * c7 - x2 * c1 - x3 * c5) >> 13;
            s16 v1 = (x0 * c1 + x1 * c3 + x2 * c5 + x3 * c7) >> 13;
            dest[i + 8 * 0] = (v0 * quant_table[i + 8 * 0] + 0x4000) >> 15;
            dest[i + 8 * 1] = (v1 * quant_table[i + 8 * 1] + 0x4000) >> 15;
            dest[i + 8 * 2] = (v2 * quant_table[i + 8 * 2] + 0x4000) >> 15;
            dest[i + 8 * 3] = (v3 * quant_table[i + 8 * 3] + 0x4000) >> 15;
            dest[i + 8 * 4] = (v4 * quant_table[i + 8 * 4] + 0x4000) >> 15;
            dest[i + 8 * 5] = (v5 * quant_table[i + 8 * 5] + 0x4000) >> 15;
            dest[i + 8 * 6] = (v6 * quant_table[i + 8 * 6] + 0x4000) >> 15;
            dest[i + 8 * 7] = (v7 * quant_table[i + 8 * 7] + 0x4000) >> 15;
        }
    }

#endif

    // ----------------------------------------------------------------------------
    // read_xxx_format
    // ----------------------------------------------------------------------------

    static
    void read_y_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int i = 0; i < rows; ++i)
        {
            const u8* scan = input;
            for (int j = cols; j > 0; --j)
            {
                *block++ = (*scan++) - 128;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                *block = *(block - 1);
                ++block;
            }

            input += stride;
        }

        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                *block = *(block - 8);
                ++block;
            }
        }
    }

    static
    void read_bgr_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int i = 0; i < rows; ++i)
        {
            const u8* scan = input;
            for (int j = 0; j < cols; ++j)
            {
                int r = scan[2];
                int g = scan[1];
                int b = scan[0];
                int y = (76 * r + 151 * g + 29 * b) >> 8;
                int cr = ((r - y) * 182) >> 8;
                int cb = ((b - y) * 144) >> 8;
                block[0 * 64] = s16(y - 128);
                block[1 * 64] = s16(cb);
                block[2 * 64] = s16(cr);
                ++block;
                scan += 3;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 1];
                block[1 * 64] = block[1 * 64 - 1];
                block[2 * 64] = block[2 * 64 - 1];
                ++block;
            }

            input += stride;

        }
        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 8];
                block[1 * 64] = block[1 * 64 - 8];
                block[2 * 64] = block[2 * 64 - 8];
                ++block;
            }
        }
    }

    static
    void read_rgb_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int i = 0; i < rows; ++i)
        {
            const u8* scan = input;
            for (int j = 0; j < cols; ++j)
            {
                int r = scan[0];
                int g = scan[1];
                int b = scan[2];
                int y = (76 * r + 151 * g + 29 * b) >> 8;
                int cr = ((r - y) * 182) >> 8;
                int cb = ((b - y) * 144) >> 8;
                block[0 * 64] = s16(y - 128);
                block[1 * 64] = s16(cb);
                block[2 * 64] = s16(cr);
                ++block;
                scan += 3;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 1];
                block[1 * 64] = block[1 * 64 - 1];
                block[2 * 64] = block[2 * 64 - 1];
                ++block;
            }

            input += stride;
        }

        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 8];
                block[1 * 64] = block[1 * 64 - 8];
                block[2 * 64] = block[2 * 64 - 8];
                ++block;
            }
        }
    }

    static
    void read_bgra_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int i = 0; i < rows; ++i)
        {
            const u8* scan = input;
            for (int j = 0; j < cols; ++j)
            {
                s16 r = scan[2];
                s16 g = scan[1];
                s16 b = scan[0];
                s16 y = (76 * r + 151 * g + 29 * b) >> 8;
                s16 cr = ((r - y) * 182) >> 8;
                s16 cb = ((b - y) * 144) >> 8;
                block[0 * 64] = s16(y - 128);
                block[1 * 64] = s16(cb);
                block[2 * 64] = s16(cr);
                ++block;
                scan += 4;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 1];
                block[1 * 64] = block[1 * 64 - 1];
                block[2 * 64] = block[2 * 64 - 1];
                ++block;
            }

            input += stride;
        }

        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 8];
                block[1 * 64] = block[1 * 64 - 8];
                block[2 * 64] = block[2 * 64 - 8];
                ++block;
            }
        }
    }

    static
    void read_rgba_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int i = 0; i < rows; ++i)
        {
            const u8* scan = input;
            for (int j = 0; j < cols; ++j)
            {
                int r = scan[0];
                int g = scan[1];
                int b = scan[2];
                int y = (76 * r + 151 * g + 29 * b) >> 8;
                int cr = ((r - y) * 182) >> 8;
                int cb = ((b - y) * 144) >> 8;
                block[0 * 64] = s16(y - 128);
                block[1 * 64] = s16(cb);
                block[2 * 64] = s16(cr);
                ++block;
                scan += 4;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 1];
                block[1 * 64] = block[1 * 64 - 1];
                block[2 * 64] = block[2 * 64 - 1];
                ++block;
            }

            input += stride;
        }

        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 8];
                block[1 * 64] = block[1 * 64 - 8];
                block[2 * 64] = block[2 * 64 - 8];
                ++block;
            }
        }
    }

#if defined(JPEG_ENABLE_SSE2)

    static
    void read_y_format_sse2(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        // load
        __m128i v0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(input)); input += stride;
        __m128i v1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(input)); input += stride;
        __m128i v2 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(input)); input += stride;
        __m128i v3 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(input)); input += stride;
        __m128i v4 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(input)); input += stride;
        __m128i v5 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(input)); input += stride;
        __m128i v6 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(input)); input += stride;
        __m128i v7 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(input));

        // adjust bias
        __m128i bias = _mm_set1_epi8(-128);
        v0 = _mm_sub_epi8(v0, bias);
        v1 = _mm_sub_epi8(v1, bias);
        v2 = _mm_sub_epi8(v2, bias);
        v3 = _mm_sub_epi8(v3, bias);
        v4 = _mm_sub_epi8(v4, bias);
        v5 = _mm_sub_epi8(v5, bias);
        v6 = _mm_sub_epi8(v6, bias);
        v7 = _mm_sub_epi8(v7, bias);

        // sign-extend
#if defined(JPEG_ENABLE_SSE4)
        v0 = _mm_cvtepi8_epi16(v0);
        v1 = _mm_cvtepi8_epi16(v1);
        v2 = _mm_cvtepi8_epi16(v2);
        v3 = _mm_cvtepi8_epi16(v3);
        v4 = _mm_cvtepi8_epi16(v4);
        v5 = _mm_cvtepi8_epi16(v5);
        v6 = _mm_cvtepi8_epi16(v6);
        v7 = _mm_cvtepi8_epi16(v7);
#else
        __m128i zero = _mm_setzero_si128();
        v0 = _mm_unpacklo_epi8(v0, _mm_cmpgt_epi8(zero, v0));
        v1 = _mm_unpacklo_epi8(v1, _mm_cmpgt_epi8(zero, v1));
        v2 = _mm_unpacklo_epi8(v2, _mm_cmpgt_epi8(zero, v2));
        v3 = _mm_unpacklo_epi8(v3, _mm_cmpgt_epi8(zero, v3));
        v4 = _mm_unpacklo_epi8(v4, _mm_cmpgt_epi8(zero, v4));
        v5 = _mm_unpacklo_epi8(v5, _mm_cmpgt_epi8(zero, v5));
        v6 = _mm_unpacklo_epi8(v6, _mm_cmpgt_epi8(zero, v6));
        v7 = _mm_unpacklo_epi8(v7, _mm_cmpgt_epi8(zero, v7));
#endif

        // store
        __m128i* dest = reinterpret_cast<__m128i*>(block);
        _mm_storeu_si128(dest + 0, v0);
        _mm_storeu_si128(dest + 1, v1);
        _mm_storeu_si128(dest + 2, v2);
        _mm_storeu_si128(dest + 3, v3);
        _mm_storeu_si128(dest + 4, v4);
        _mm_storeu_si128(dest + 5, v5);
        _mm_storeu_si128(dest + 6, v6);
        _mm_storeu_si128(dest + 7, v7);
    }

    static
    void read_bgra_format_sse2(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        __m128i* dest = reinterpret_cast<__m128i*>(block);

        const __m128i mask = _mm_set1_epi32(0xff);
        const __m128i c76 = _mm_set1_epi16(76);
        const __m128i c151 = _mm_set1_epi16(151);
        const __m128i c29 = _mm_set1_epi16(29);
        const __m128i c128 = _mm_set1_epi16(128);
        const __m128i c182 = _mm_set1_epi16(182);
        const __m128i c144 = _mm_set1_epi16(144);

        for (int y = 0; y < 8; ++y)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(input);
            input += stride;

            // load, unpack
            __m128i b0 = _mm_loadu_si128(ptr + 0);
            __m128i b1 = _mm_loadu_si128(ptr + 1);
            __m128i g0 = _mm_srli_epi32(b0, 8);
            __m128i r0 = _mm_srli_epi32(b0, 16);
            __m128i g1 = _mm_srli_epi32(b1, 8);
            __m128i r1 = _mm_srli_epi32(b1, 16);
            b0 = _mm_and_si128(b0, mask);
            b1 = _mm_and_si128(b1, mask);
            g0 = _mm_and_si128(g0, mask);
            g1 = _mm_and_si128(g1, mask);
            r0 = _mm_and_si128(r0, mask);
            r1 = _mm_and_si128(r1, mask);
            __m128i b = _mm_packs_epi32(b0, b1);
            __m128i g = _mm_packs_epi32(g0, g1);
            __m128i r = _mm_packs_epi32(r0, r1);

            // compute luminance
            __m128i s0 = _mm_mullo_epi16(r, c76);
            __m128i s1 = _mm_mullo_epi16(g, c151);
            __m128i s2 = _mm_mullo_epi16(b, c29);
            __m128i s = _mm_add_epi16(s0, _mm_add_epi16(s1, s2));
            s = _mm_srli_epi16(s, 8);

            // compute chroma
            __m128i cr = _mm_sub_epi16(r, s);
            __m128i cb = _mm_sub_epi16(b, s);
            cr = _mm_mullo_epi16(cr, c182);
            cb = _mm_mullo_epi16(cb, c144);
            cr = _mm_srai_epi16(cr, 8);
            cb = _mm_srai_epi16(cb, 8);

            // adjust bias
            s = _mm_sub_epi16(s, c128);

            // store
            _mm_storeu_si128(dest + y + 0, s);
            _mm_storeu_si128(dest + y + 8, cb);
            _mm_storeu_si128(dest + y + 16, cr);
        }
    }

    static
    void read_rgba_format_sse2(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        __m128i* dest = reinterpret_cast<__m128i*>(block);

        const __m128i mask = _mm_set1_epi32(0xff);
        const __m128i c76 = _mm_set1_epi16(76);
        const __m128i c151 = _mm_set1_epi16(151);
        const __m128i c29 = _mm_set1_epi16(29);
        const __m128i c128 = _mm_set1_epi16(128);
        const __m128i c182 = _mm_set1_epi16(182);
        const __m128i c144 = _mm_set1_epi16(144);

        for (int y = 0; y < 8; ++y)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(input);
            input += stride;

            // load, unpack
            __m128i r0 = _mm_loadu_si128(ptr + 0);
            __m128i r1 = _mm_loadu_si128(ptr + 1);
            __m128i g0 = _mm_srli_epi32(r0, 8);
            __m128i b0 = _mm_srli_epi32(r0, 16);
            __m128i g1 = _mm_srli_epi32(r1, 8);
            __m128i b1 = _mm_srli_epi32(r1, 16);
            b0 = _mm_and_si128(b0, mask);
            b1 = _mm_and_si128(b1, mask);
            g0 = _mm_and_si128(g0, mask);
            g1 = _mm_and_si128(g1, mask);
            r0 = _mm_and_si128(r0, mask);
            r1 = _mm_and_si128(r1, mask);
            __m128i b = _mm_packs_epi32(b0, b1);
            __m128i g = _mm_packs_epi32(g0, g1);
            __m128i r = _mm_packs_epi32(r0, r1);

            // compute luminance
            __m128i s0 = _mm_mullo_epi16(r, c76);
            __m128i s1 = _mm_mullo_epi16(g, c151);
            __m128i s2 = _mm_mullo_epi16(b, c29);
            __m128i s = _mm_add_epi16(s0, _mm_add_epi16(s1, s2));
            s = _mm_srli_epi16(s, 8);

            // compute chroma
            __m128i cr = _mm_sub_epi16(r, s);
            __m128i cb = _mm_sub_epi16(b, s);
            cr = _mm_mullo_epi16(cr, c182);
            cb = _mm_mullo_epi16(cb, c144);
            cr = _mm_srai_epi16(cr, 8);
            cb = _mm_srai_epi16(cb, 8);

            // adjust bias
            s = _mm_sub_epi16(s, c128);

            // store
            _mm_storeu_si128(dest + y + 0, s);
            _mm_storeu_si128(dest + y + 8, cb);
            _mm_storeu_si128(dest + y + 16, cr);
        }
    }

#endif // JPEG_ENABLE_SSE2

#if defined(JPEG_ENABLE_SSE4)

    static
    void read_bgr_format_ssse3(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        __m128i* dest = reinterpret_cast<__m128i*>(block);

        const __m128i c76 = _mm_set1_epi16(76);
        const __m128i c151 = _mm_set1_epi16(151);
        const __m128i c29 = _mm_set1_epi16(29);
        const __m128i c128 = _mm_set1_epi16(128);
        const __m128i c182 = _mm_set1_epi16(182);
        const __m128i c144 = _mm_set1_epi16(144);

        for (int y = 0; y < 8; ++y)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(input);
            input += stride;

            // load
            __m128i v0 = _mm_loadu_si128(ptr + 0);
            __m128i v1 = _mm_loadl_epi64(ptr + 1);

            // unpack
            constexpr u8 n = 0x80;
            __m128i b0 = _mm_shuffle_epi8(v0, _mm_setr_epi8(0, n, 3, n, 6, n, 9, n, 12, n, 15, n, n, n, n, n));
            __m128i b1 = _mm_shuffle_epi8(v1, _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, n, n, 2, n, 5, n));
            __m128i g0 = _mm_shuffle_epi8(v0, _mm_setr_epi8(1, n, 4, n, 7, n, 10, n, 13, n, n, n, n, n, n, n));
            __m128i g1 = _mm_shuffle_epi8(v1, _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, 0, n, 3, n, 6, n));
            __m128i r0 = _mm_shuffle_epi8(v0, _mm_setr_epi8(2, n, 5, n, 8, n, 11, n, 14, n, n, n, n, n, n, n));
            __m128i r1 = _mm_shuffle_epi8(v1, _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, 1, n, 4, n, 7, n));
           __m128i b = _mm_or_si128(b0, b1);
           __m128i g = _mm_or_si128(g0, g1);
           __m128i r = _mm_or_si128(r0, r1);

            // compute luminance
            __m128i s0 = _mm_mullo_epi16(r, c76);
            __m128i s1 = _mm_mullo_epi16(g, c151);
            __m128i s2 = _mm_mullo_epi16(b, c29);
            __m128i s = _mm_add_epi16(s0, _mm_add_epi16(s1, s2));
            s = _mm_srli_epi16(s, 8);

            // compute chroma
            __m128i cr = _mm_sub_epi16(r, s);
            __m128i cb = _mm_sub_epi16(b, s);
            cr = _mm_mullo_epi16(cr, c182);
            cb = _mm_mullo_epi16(cb, c144);
            cr = _mm_srai_epi16(cr, 8);
            cb = _mm_srai_epi16(cb, 8);

            // adjust bias
            s = _mm_sub_epi16(s, c128);

            // store
            _mm_storeu_si128(dest + y + 0, s);
            _mm_storeu_si128(dest + y + 8, cb);
            _mm_storeu_si128(dest + y + 16, cr);
        }
    }

    static
    void read_rgb_format_ssse3(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        __m128i* dest = reinterpret_cast<__m128i*>(block);

        const __m128i c76 = _mm_set1_epi16(76);
        const __m128i c151 = _mm_set1_epi16(151);
        const __m128i c29 = _mm_set1_epi16(29);
        const __m128i c128 = _mm_set1_epi16(128);
        const __m128i c182 = _mm_set1_epi16(182);
        const __m128i c144 = _mm_set1_epi16(144);

        for (int y = 0; y < 8; ++y)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(input);
            input += stride;

            // load
            __m128i v0 = _mm_loadu_si128(ptr + 0);
            __m128i v1 = _mm_loadl_epi64(ptr + 1);

            // unpack
            constexpr u8 n = 0x80;
            __m128i r0 = _mm_shuffle_epi8(v0, _mm_setr_epi8(0, n, 3, n, 6, n, 9, n, 12, n, 15, n, n, n, n, n));
            __m128i r1 = _mm_shuffle_epi8(v1, _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, n, n, 2, n, 5, n));
            __m128i g0 = _mm_shuffle_epi8(v0, _mm_setr_epi8(1, n, 4, n, 7, n, 10, n, 13, n, n, n, n, n, n, n));
            __m128i g1 = _mm_shuffle_epi8(v1, _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, 0, n, 3, n, 6, n));
            __m128i b0 = _mm_shuffle_epi8(v0, _mm_setr_epi8(2, n, 5, n, 8, n, 11, n, 14, n, n, n, n, n, n, n));
            __m128i b1 = _mm_shuffle_epi8(v1, _mm_setr_epi8(n, n, n, n, n, n, n, n, n, n, 1, n, 4, n, 7, n));
           __m128i b = _mm_or_si128(b0, b1);
           __m128i g = _mm_or_si128(g0, g1);
           __m128i r = _mm_or_si128(r0, r1);

            // compute luminance
            __m128i s0 = _mm_mullo_epi16(r, c76);
            __m128i s1 = _mm_mullo_epi16(g, c151);
            __m128i s2 = _mm_mullo_epi16(b, c29);
            __m128i s = _mm_add_epi16(s0, _mm_add_epi16(s1, s2));
            s = _mm_srli_epi16(s, 8);

            // compute chroma
            __m128i cr = _mm_sub_epi16(r, s);
            __m128i cb = _mm_sub_epi16(b, s);
            cr = _mm_mullo_epi16(cr, c182);
            cb = _mm_mullo_epi16(cb, c144);
            cr = _mm_srai_epi16(cr, 8);
            cb = _mm_srai_epi16(cb, 8);

            // adjust bias
            s = _mm_sub_epi16(s, c128);

            // store
            _mm_storeu_si128(dest + y + 0, s);
            _mm_storeu_si128(dest + y + 8, cb);
            _mm_storeu_si128(dest + y + 16, cr);
        }
    }

#endif // JPEG_ENABLE_SSE4

    // ----------------------------------------------------------------------------
    // jpeg_encode
    // ----------------------------------------------------------------------------

    jpeg_encode::jpeg_encode(SampleType sample, u32 width, u32 height, size_t stride, u32 quality)
        : ILqt(64)
        , ICqt(64)
    {
        MANGO_UNREFERENCED(stride);

        int bytes_per_pixel = 0;

        channel_count = 0;

        channel[0].component = 1;
        channel[0].qtable = ILqt;

        channel[1].component = 2;
        channel[1].qtable = ICqt;

        channel[2].component = 3;
        channel[2].qtable = ICqt;

        read_8x8 = nullptr;

        u64 flags = getCPUFlags();
        MANGO_UNREFERENCED(flags);

        const char* sampler_name = nullptr;

        switch (sample)
        {
            case JPEG_U8_Y:
#if defined(JPEG_ENABLE_SSE2)
                if (flags & INTEL_SSE2)
                {
                    read_8x8 = read_y_format_sse2;
                    sampler_name = "SSE2 Y 8x8";
                }
#endif
                read = read_y_format;
                bytes_per_pixel = 1;
                channel_count = 1;
                break;

            case JPEG_U8_BGR:
#if defined(JPEG_ENABLE_SSE4)
				if (flags & INTEL_SSSE3)
                {
                    read_8x8 = read_bgr_format_ssse3;
                    sampler_name = "SSSE3 BGR 8x8";
                }
#endif
                read = read_bgr_format;
                bytes_per_pixel = 3;
                channel_count = 3;
                break;

            case JPEG_U8_RGB:
#if defined(JPEG_ENABLE_SSE4)
                if (flags & INTEL_SSSE3)
                {
                    read_8x8 = read_rgb_format_ssse3;
                    sampler_name = "SSSE3 RGB 8x8";
                }
#endif
                read = read_rgb_format;
                bytes_per_pixel = 3;
                channel_count = 3;
                break;

            case JPEG_U8_BGRA:
#if defined(JPEG_ENABLE_SSE2)
                if (flags & INTEL_SSE2)
                {
                    read_8x8 = read_bgra_format_sse2;
                    sampler_name = "SSE2 BGRA 8x8";
                }
#endif
                read = read_bgra_format;
                bytes_per_pixel = 4;
                channel_count = 3;
                break;

            case JPEG_U8_RGBA:
#if defined(JPEG_ENABLE_SSE2)
                if (flags & INTEL_SSE2)
                {
                    read_8x8 = read_rgba_format_sse2;
                    sampler_name = "SSE2 RGBA 8x8";
                }
#endif
                read = read_rgba_format;
                bytes_per_pixel = 4;
                channel_count = 3;
                break;
        }

        if (!read_8x8)
        {
            read_8x8 = read;
        }

        // build encoder info string
        info = "JPEG Encoder: ";
        info += fdct_name;
        if (sampler_name)
        {
            info += " ";
            info += sampler_name;
        }

        mcu_width = 8;
        mcu_height = 8;

        horizontal_mcus = (width + mcu_width - 1) >> 3;
        vertical_mcus   = (height + mcu_height - 1) >> 3;

        rows_in_bottom_mcus = height - (vertical_mcus - 1) * mcu_height;
        cols_in_right_mcus  = width  - (horizontal_mcus - 1) * mcu_width;

        mcu_width_size = mcu_width * bytes_per_pixel;

        init_quantization_tables(quality);
    }

    jpeg_encode::~jpeg_encode()
    {
    }

    void jpeg_encode::init_quantization_tables(u32 quality)
    {
        for (int i = 0; i < 64; ++i)
        {
            u16 index = zigzag_table [i];
            u32 value;

            // luminance quantization table * quality factor
            value = luminance_quant_table [i] * quality;
            value = (value + 0x200) >> 10;

            if (value < 2)
                value = 2;
            else if (value > 255)
                value = 255;

            Lqt [index] = (u8) value;
            ILqt [i] = u16(0x8000 / value);

            // chrominance quantization table * quality factor
            value = chrominance_quant_table [i] * quality;
            value = (value + 0x200) >> 10;

            if (value < 2)
                value = 2;
            else if (value > 255)
                value = 255;

            Cqt [index] = (u8) value;
            ICqt [i] = u16(0x8000 / value);
        }
    }

    void jpeg_encode::write_markers(BigEndianStream& p, SampleType sample, u32 width, u32 height)
    {
        // Start of image marker
        p.write16(MARKER_SOI);
        
        // ICC profile.  splitting to multiple markers if necessary
        if(icc.size)
        {
            const size_t magicICCLength = 12;
            const u8 magicICC[magicICCLength] = { 0x49, 0x43, 0x43, 0x5f, 0x50, 0x52, 0x4f, 0x46, 0x49, 0x4c, 0x45, 0 }; // 'ICC_PROFILE', 0
            
            const size_t iccMaxSegmentSize = 65000;  // marker size is 16bit minus icc stuff, if larger we need to split
            const size_t iccSegments = (icc.size+iccMaxSegmentSize-1)/iccMaxSegmentSize;
            
            for(size_t i=0;i<iccSegments;i++)
            {
                const size_t size = i<(iccSegments-1) ? iccMaxSegmentSize :  icc.size%iccMaxSegmentSize;
                p.write16(MARKER_APP2);
                p.write16(size + magicICCLength + 4);
                p.write(magicICC, magicICCLength);
                p.write8(i+1); // segment index, 1-based
                p.write8(iccSegments);
                p.write(icc.slice(i*iccMaxSegmentSize), size);
            }
        }

        // Quantization table marker
        p.write16(MARKER_DQT);
        p.write16(0x43); // quantization table length
        p.write8(0x00); // Pq, Tq

        // Lqt table
        p.write(Lqt, 64);

        // Quantization table marker
        p.write16(MARKER_DQT);
        p.write16(0x43); // quantization table length
        p.write8(0x01); // Pq, Tq

        // Cqt table
        p.write(Cqt, 64);

        // Start of frame marker
        p.write16(MARKER_SOF0);

        u8 number_of_components = 0;

        switch (sample)
        {
            case JPEG_U8_Y:
                number_of_components = 1;
                break;

            case JPEG_U8_RGB:
            case JPEG_U8_BGR:
            case JPEG_U8_BGRA:
            case JPEG_U8_RGBA:
                number_of_components = 3;
                break;
        }

        u16 header_length = 8 + 3 * number_of_components;

        p.write16(header_length); // frame header length
        p.write8(8); // precision
        p.write16(u16(height)); // image height
        p.write16(u16(width)); // image width
        p.write8(number_of_components); // Nf

        const u8 nfdata[] =
        {
            0x01, 0x11, 0x00, // component 1
            0x00, 0x00, 0x00, // padding
            0x01, 0x11, 0x00, // component 1
            0x02, 0x11, 0x01, // component 2
            0x03, 0x11, 0x01, // component 3
        };

        p.write(nfdata + (number_of_components - 1) * 3, number_of_components * 3);

        // huffman table(DHT)
        p.write(marker_data, sizeof(marker_data));

        // Define Restart Interval marker
        p.write16(MARKER_DRI);
        p.write16(4);
        p.write16(horizontal_mcus);

        // Start of scan marker
        p.write16(MARKER_SOS);
        p.write16(6 + number_of_components * 2); // header length
        p.write8(number_of_components); // Ns

        const u8 nsdata[] =
        {
            0x01, 0x00,
            0x02, 0x11,
            0x03, 0x11,
        };

        p.write(nsdata, number_of_components * 2);
        p.write8(0x00);
        p.write8(0x3f);
        p.write8(0x00);
    }

    // ----------------------------------------------------------------------------
    // encodeJPEG()
    // ----------------------------------------------------------------------------

    void encodeJPEG(ImageEncodeStatus& status, const Surface& surface, Stream& stream, int quality, SampleType sample, const ImageEncodeOptions& options)
    {
        jpeg_encode jp(sample, surface.width, surface.height, surface.stride, quality);
        jp.icc = options.icc;

        const u8* input = surface.image;
        size_t stride = surface.stride;

        // bitstream for each MCU scan
        std::vector<EncodeBuffer> buffers(jp.vertical_mcus);

        ConcurrentQueue queue;

        // encode MCUs
        for (int y = 0; y < jp.vertical_mcus; ++y)
        {
            int rows = jp.mcu_height;
            auto read_func = jp.read_8x8; // default: optimized 8x8 reader

            if (y >= jp.vertical_mcus - 1)
            {
                // vertical clipping
                rows = jp.rows_in_bottom_mcus;
                read_func = jp.read; // clipping reader
            }

            queue.enqueue([&jp, y, &buffers, input, stride, rows, read_func]
            {
                const u8* image = input;

                HuffmanEncoder huffman;
                EncodeBuffer& buffer = buffers[y];

                constexpr int buffer_size = 2048;
                constexpr int flush_threshold = buffer_size - 512;

                u8 huff_temp[buffer_size]; // encoding buffer
                u8* ptr = huff_temp;

                int cols = jp.mcu_width;
                auto read = read_func;

                const int right_mcu = jp.horizontal_mcus - 1;

                for (int x = 0; x < jp.horizontal_mcus; ++x)
                {
                    if (x >= right_mcu)
                    {
                        // horizontal clipping
                        cols = jp.cols_in_right_mcus;
                        read = jp.read; // clipping reader
                    }

                    // read MCU data
                    s16 block[BLOCK_SIZE * 3];
                    read(block, image, stride, rows, cols);

                    // encode the data in MCU
                    for (int i = 0; i < jp.channel_count; ++i)
                    {
                        s16 temp[BLOCK_SIZE];
                        fdct(temp, block + i * BLOCK_SIZE, jp.channel[i].qtable);
                        ptr = huffman.encode(ptr, jp.channel[i].component - 1, temp);
                    }

                    // flush encoding buffer
                    if (ptr - huff_temp > flush_threshold)
                    {
                        buffer.append(huff_temp, ptr - huff_temp);
                        ptr = huff_temp;
                    }

                    image += jp.mcu_width_size;
                }

                // flush encoding buffer
                ptr = huffman.flush(ptr);
                buffer.append(huff_temp, ptr - huff_temp);

                // mark buffer ready for writing
                buffer.ready = true;
            });

            input += surface.stride * jp.mcu_height;
        }

        BigEndianStream s(stream);

        // writing marker data
        jp.write_markers(s, sample, surface.width, surface.height);

        for (int y = 0; y < jp.vertical_mcus; ++y)
        {
            EncodeBuffer& buffer = buffers[y];

            for ( ; !buffer.ready; )
            {
                // buffer is not processed yet; help the thread pool while waiting
                queue.steal();
            }

            // write huffman bitstream
            s.write(buffer, buffer.size());

            // write restart marker
            int index = y & 7;
            s.write16(MARKER_RST0 + index);
        }

        // EOI marker
        s.write16(MARKER_EOI);

        status.info = jp.info;
    }

} // namespace

namespace mango {
namespace jpeg {

    SampleFormat getSampleFormat(const Format& format)
    {
        // set default format
        SampleFormat result { JPEG_U8_RGBA, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8) };

        // find better match
        for (auto sf : g_format_table)
        {
            if (format == sf.format)
            {
                result = sf;
                break;
            }
        }

        return result;
    }

    ImageEncodeStatus encodeImage(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        ImageEncodeStatus status;

        // configure quality
        float quality = clamp(1.0f - options.quality, 0.0f, 1.0f);
        u32 iq = u32(std::pow(1.0f + quality, 11.0f) * 8.0f);

        SampleFormat sf = getSampleFormat(surface.format);

        // encode
        if (surface.format == sf.format)
        {
            encodeJPEG(status, surface, stream, iq, sf.sample, options);
            status.direct = true;
        }
        else
        {
            // convert source surface to format supported in the encoder
            Bitmap temp(surface, sf.format);
            encodeJPEG(status, temp, stream, iq, sf.sample, options);
        }

        return status;
    }

} // namespace jpeg
} // namespace mango
