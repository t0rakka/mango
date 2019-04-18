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

#define BLOCK_SIZE  64

namespace
{
    using namespace mango;
    using namespace jpeg;

    static SampleFormat g_format_table [] =
    {
        { JPEG_U8_Y,    FORMAT_L8 },
        { JPEG_U8_BGR,  FORMAT_B8G8R8 },
        { JPEG_U8_RGB,  FORMAT_R8G8B8 },
        { JPEG_U8_BGRA, FORMAT_B8G8R8A8 },
        { JPEG_U8_RGBA, FORMAT_R8G8B8A8 },
    };

    const u16 luminance_dc_code_table [] =
    {
        0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006,
        0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE
    };

    const u16 luminance_dc_size_table [] =
    {
        0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
        0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009
    };

    const u16 chrominance_dc_code_table [] =
    {
        0x0000, 0x0001, 0x0002, 0x0006, 0x000E, 0x001E,
        0x003E, 0x007E, 0x00FE, 0x01FE, 0x03FE, 0x07FE
    };

    const u16 chrominance_dc_size_table [] =
    {
        0x0002, 0x0002, 0x0002, 0x0003, 0x0004, 0x0005,
        0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B
    };

    const u16 luminance_ac_code_table [] =
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

    const u16 luminance_ac_size_table [] =
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

    const u16 chrominance_ac_code_table [] =
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

    const u16 chrominance_ac_size_table [] =
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

    const u8 bit_size [] =
    {
        0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
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

    struct jpeg_chan
    {
        int     component;
        u16*    qtable;
    };

    struct jpeg_encode
    {
        int     mcu_width;
        int     mcu_height;
        int     horizontal_mcus;
        int     vertical_mcus;
        int     cols_in_right_mcus;
        int     rows_in_bottom_mcus;

        int     length_minus_mcu_width;
        int     length_minus_width;
        int     mcu_width_size;

        u8      Lqt [BLOCK_SIZE];
        u8      Cqt [BLOCK_SIZE];
        AlignedVector<u16> ILqt;
        AlignedVector<u16> ICqt;

        // MCU configuration
        jpeg_chan   channel[3];
        int         channel_count;

        void (*read_format) (jpeg_encode* jp, s16 *block, u8* input, int rows, int cols, int incr);

        jpeg_encode(Sample sample, u32 width, u32 height, u32 stride, u32 quality);
        ~jpeg_encode();

        void init_quantization_tables(u32 quality);
        void write_markers(BigEndianStream& p, Sample sample, u32 width, u32 height);
    };

    struct HuffmanEncoder
    {
        int ldc1;
        int ldc2;
        int ldc3;

#if defined(MANGO_CPU_64BIT)
        u64 lcode;
#else
        u32 lcode;
#endif
        int bitindex;

        HuffmanEncoder()
        {
            ldc1 = 0;
            ldc2 = 0;
            ldc3 = 0;
            lcode = 0;
            bitindex = 0;
        }

        ~HuffmanEncoder()
        {
        }

        u8* write32_stuff(u8* output, u32 code) const
        {
            // JPEG bitstream uses 0xff as a marker, followed by ID byte
            // If the ID byte is zero ("stuff") that means the preceding 0xff is a literal value
            if ((*output++ = u8(code >> 24)) == 0xff) *output++ = 0; // write stuff byte
            if ((*output++ = u8(code >> 16)) == 0xff) *output++ = 0;
            if ((*output++ = u8(code >>  8)) == 0xff) *output++ = 0;
            if ((*output++ = u8(code >>  0)) == 0xff) *output++ = 0;
            return output;
        }

        u8* putbits(u8* output, u32 data, int numbits)
        {
            constexpr int regbits = sizeof(lcode) * 8;

            int bits_in_next_word = bitindex + numbits - regbits;
            if (bits_in_next_word < 0)
            {
                lcode = (lcode << numbits) | data;
                bitindex += numbits;
            }
            else
            {
                lcode = (lcode << (regbits - bitindex)) | (data >> bits_in_next_word);

#if defined(MANGO_CPU_64BIT)
                if (u64_has_zero_byte(~lcode))
                {
                    output = write32_stuff(output, u32(lcode >> 32));
                    output = write32_stuff(output, u32(lcode));
                }
                else
                {
                    ustore64be(output, lcode);
                    output += 8;
                }
#else
                if (u32_has_zero_byte(~lcode))
                {
                    output = write32_stuff(output, lcode);
                }
                else
                {
                    ustore32be(output, lcode);
                    output += 4;
                }
#endif
                lcode = data;
                bitindex = bits_in_next_word;
            }

            return output;
        }

        u8* flush(u8* p)
        {
            if (bitindex > 0)
            {
                constexpr int regbits = sizeof(lcode) * 8;
                lcode <<= (regbits - bitindex);

                int count = (bitindex + 7) >> 3;
                u8* ptr = reinterpret_cast<u8 *>(&lcode) + (sizeof(lcode) - 1);

                for (int i = 0; i < count; ++i)
                {
                    u8 v = *ptr--;
                    *p++ = v;
                    if (v == 0xff)
                    {
                        // write stuff byte
                        *p++ = 0;
                    }
                }
            }

            return p;
        }

        u8* encode(u8* p, int component, s16* input)
        {
            const u16* DcCodeTable;
            const u16* DcSizeTable;
            const u16* AcCodeTable;
            const u16* AcSizeTable;

            int Coeff = input[0];
            int LastDc;

            if (component == 1)
            {
                DcCodeTable = luminance_dc_code_table;
                DcSizeTable = luminance_dc_size_table;
                AcCodeTable = luminance_ac_code_table;
                AcSizeTable = luminance_ac_size_table;

                LastDc = ldc1;
                ldc1 = Coeff;
            }
            else
            {
                DcCodeTable = chrominance_dc_code_table;
                DcSizeTable = chrominance_dc_size_table;
                AcCodeTable = chrominance_ac_code_table;
                AcSizeTable = chrominance_ac_size_table;

                if (component == 2)
                {
                    LastDc = ldc2;
                    ldc2 = Coeff;
                }
                else
                {
                    LastDc = ldc3;
                    ldc3 = Coeff;
                }
            }

            Coeff -= LastDc;
            int AbsCoeff = (Coeff < 0) ? -Coeff-- : Coeff;

            int DataSize = 0;

            while (AbsCoeff != 0)
            {
                AbsCoeff >>= 1;
                DataSize++;
            }

            u16 HuffCode = DcCodeTable [DataSize];
            u16 HuffSize = DcSizeTable [DataSize];

            Coeff &= (1 << DataSize) - 1;

            u32 data = (HuffCode << DataSize) | Coeff;
            int numbits = HuffSize + DataSize;
            p = putbits(p, data, numbits);

            int RunLength = 0;

            for (int i = 1; i < 64; ++i)
            {
                s16 Coeff = input[zigzag_table_inverse[i]];
                if (Coeff)
                {
                    while (RunLength > 15)
                    {
                        RunLength -= 16;

                        data = AcCodeTable [161];
                        numbits = AcSizeTable [161];
                        p = putbits(p, data, numbits);
                    }

                    AbsCoeff = (Coeff < 0) ? -Coeff-- : Coeff;
                    if (AbsCoeff >> 8 == 0)
                        DataSize = bit_size [AbsCoeff];
                    else
                        DataSize = bit_size [AbsCoeff >> 8] + 8;

                    int index = RunLength * 10 + DataSize;
                    HuffCode = AcCodeTable [index];
                    HuffSize = AcSizeTable [index];

                    Coeff &= (1 << DataSize) - 1;

                    data = (HuffCode << DataSize) | Coeff;
                    numbits = HuffSize + DataSize;
                    p = putbits(p, data, numbits);

                    RunLength = 0;
                }
                else
                {
                    ++RunLength;
                }
            }

            if (RunLength != 0)
            {
                data = AcCodeTable [0];
                numbits = AcSizeTable [0];
                p = putbits(p, data, numbits);
            }

            return p;
        }
    };

#if 1

    void fdct(s16* dest, const s16* data, const u16* quant_table)
    {
        const s16 c1 = 1420;  // cos 1PI/16 * root(2)
        const s16 c2 = 1338;  // cos 2PI/16 * root(2)
        const s16 c3 = 1204;  // cos 3PI/16 * root(2)
        const s16 c5 = 805;   // cos 5PI/16 * root(2)
        const s16 c6 = 554;   // cos 6PI/16 * root(2)
        const s16 c7 = 283;   // cos 7PI/16 * root(2)

        s16 temp[64];

        for (int i = 0; i < 8; ++i)
        {
            int x8 = data [0] + data [7];
            int x0 = data [0] - data [7];
            int x7 = data [1] + data [6];
            int x1 = data [1] - data [6];
            int x6 = data [2] + data [5];
            int x2 = data [2] - data [5];
            int x5 = data [3] + data [4];
            int x3 = data [3] - data [4];
            int x4 = x8 + x5;
            x8 = x8 - x5;
            x5 = x7 + x6;
            x7 = x7 - x6;
            temp[i * 8 + 0] = s16(x4 + x5);
            temp[i * 8 + 4] = s16(x4 - x5);
            temp[i * 8 + 2] = s16((x8 * c2 + x7 * c6) >> 10);
            temp[i * 8 + 6] = s16((x8 * c6 - x7 * c2) >> 10);
            temp[i * 8 + 7] = s16((x0 * c7 - x1 * c5 + x2 * c3 - x3 * c1) >> 10);
            temp[i * 8 + 5] = s16((x0 * c5 - x1 * c1 + x2 * c7 + x3 * c3) >> 10);
            temp[i * 8 + 3] = s16((x0 * c3 - x1 * c7 - x2 * c1 - x3 * c5) >> 10);
            temp[i * 8 + 1] = s16((x0 * c1 + x1 * c3 + x2 * c5 + x3 * c7) >> 10);
            data += 8;
        }

        for (int i = 0; i < 8; ++i)
        {
            int x8 = temp [i +  0] + temp [i + 56];
            int x0 = temp [i +  0] - temp [i + 56];
            int x7 = temp [i +  8] + temp [i + 48];
            int x1 = temp [i +  8] - temp [i + 48];
            int x6 = temp [i + 16] + temp [i + 40];
            int x2 = temp [i + 16] - temp [i + 40];
            int x5 = temp [i + 24] + temp [i + 32];
            int x3 = temp [i + 24] - temp [i + 32];
            int x4 = x8 + x5;
            x8 = x8 - x5;
            x5 = x7 + x6;
            x7 = x7 - x6;
            auto v0 = s16((x4 + x5) >> 3);
            auto v4 = s16((x4 - x5) >> 3);
            auto v2 = s16((x8 * c2 + x7 * c6) >> 13);
            auto v6 = s16((x8 * c6 - x7 * c2) >> 13);
            auto v7 = s16((x0 * c7 - x1 * c5 + x2 * c3 - x3 * c1) >> 13);
            auto v5 = s16((x0 * c5 - x1 * c1 + x2 * c7 + x3 * c3) >> 13);
            auto v3 = s16((x0 * c3 - x1 * c7 - x2 * c1 - x3 * c5) >> 13);
            auto v1 = s16((x0 * c1 + x1 * c3 + x2 * c5 + x3 * c7) >> 13);
            dest[i + 8 * 0] = s16((v0 * quant_table[i * 8 + 0] + 0x4000) >> 15);
            dest[i + 8 * 1] = s16((v1 * quant_table[i * 8 + 1] + 0x4000) >> 15);
            dest[i + 8 * 2] = s16((v2 * quant_table[i * 8 + 2] + 0x4000) >> 15);
            dest[i + 8 * 3] = s16((v3 * quant_table[i * 8 + 3] + 0x4000) >> 15);
            dest[i + 8 * 4] = s16((v4 * quant_table[i * 8 + 4] + 0x4000) >> 15);
            dest[i + 8 * 5] = s16((v5 * quant_table[i * 8 + 5] + 0x4000) >> 15);
            dest[i + 8 * 6] = s16((v6 * quant_table[i * 8 + 6] + 0x4000) >> 15);
            dest[i + 8 * 7] = s16((v7 * quant_table[i * 8 + 7] + 0x4000) >> 15);
        }
    }

#else

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

    void fdct(s16* dest, const s16* data, const u16* quant_table)
    {
        constexpr s16 c1 = 1420;  // cos 1PI/16 * root(2)
        constexpr s16 c2 = 1338;  // cos 2PI/16 * root(2)
        constexpr s16 c3 = 1204;  // cos 3PI/16 * root(2)
        constexpr s16 c5 = 805;   // cos 5PI/16 * root(2)
        constexpr s16 c6 = 554;   // cos 6PI/16 * root(2)
        constexpr s16 c7 = 283;   // cos 7PI/16 * root(2)

        const __m128i* s = reinterpret_cast<const __m128i *>(data);
        __m128i v0 = _mm_loadu_si128(s + 0);
        __m128i v1 = _mm_loadu_si128(s + 1);
        __m128i v2 = _mm_loadu_si128(s + 2);
        __m128i v3 = _mm_loadu_si128(s + 3);
        __m128i v4 = _mm_loadu_si128(s + 4);
        __m128i v5 = _mm_loadu_si128(s + 5);
        __m128i v6 = _mm_loadu_si128(s + 6);
        __m128i v7 = _mm_loadu_si128(s + 7);

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

        __m128i a_lo;
        __m128i a_hi;
        __m128i b_lo;
        __m128i b_hi;

        v0 = x4 + x5;
        v4 = x4 - x5;

        // v2 = (x8 * c2 + x7 * c6) >> 10;
        a_lo = _mm_madd_epi16(x87_lo, c26p);
        a_hi = _mm_madd_epi16(x87_hi, c26p);
        a_lo = _mm_srai_epi32(a_lo, 10);
        a_hi = _mm_srai_epi32(a_hi, 10);
        v2 = _mm_packs_epi32(a_lo, a_hi);

        // v6 = (x8 * c6 - x7 * c2) >> 10;
        a_lo = _mm_madd_epi16(x87_lo, c62n);
        a_hi = _mm_madd_epi16(x87_hi, c62n);
        a_lo = _mm_srai_epi32(a_lo, 10);
        a_hi = _mm_srai_epi32(a_hi, 10);
        v6 = _mm_packs_epi32(a_lo, a_hi);

        // v7 = (x0 * c7 - x1 * c5   +   x2 * c3 - x3 * c1) >> 10;
        a_lo = _mm_madd_epi16(x01_lo, c75n);
        a_hi = _mm_madd_epi16(x01_hi, c75n);
        b_lo = _mm_madd_epi16(x23_lo, c31n);
        b_hi = _mm_madd_epi16(x23_hi, c31n);
        a_lo = _mm_add_epi32(a_lo, b_lo);
        a_hi = _mm_add_epi32(a_hi, b_hi);
        a_lo = _mm_srai_epi32(a_lo, 10);
        a_hi = _mm_srai_epi32(a_hi, 10);
        v7 = _mm_packs_epi32(a_lo, a_hi);

        // v5 = (x0 * c5 - x1 * c1   +   x2 * c7 + x3 * c3) >> 10;
        a_lo = _mm_madd_epi16(x01_lo, c51n);
        a_hi = _mm_madd_epi16(x01_hi, c51n);
        b_lo = _mm_madd_epi16(x23_lo, c73p);
        b_hi = _mm_madd_epi16(x23_hi, c73p);
        a_lo = _mm_add_epi32(a_lo, b_lo);
        a_hi = _mm_add_epi32(a_hi, b_hi);
        a_lo = _mm_srai_epi32(a_lo, 10);
        a_hi = _mm_srai_epi32(a_hi, 10);
        v5 = _mm_packs_epi32(a_lo, a_hi);

        // v3 = (x0 * c3 - x1 * c7   -   x2 * c1 - x3 * c5) >> 10;
        a_lo = _mm_madd_epi16(x01_lo, c37n);
        a_hi = _mm_madd_epi16(x01_hi, c37n);
        b_lo = _mm_madd_epi16(x23_lo, c15p);
        b_hi = _mm_madd_epi16(x23_hi, c15p);
        a_lo = _mm_sub_epi32(a_lo, b_lo);
        a_hi = _mm_sub_epi32(a_hi, b_hi);
        a_lo = _mm_srai_epi32(a_lo, 10);
        a_hi = _mm_srai_epi32(a_hi, 10);
        v3 = _mm_packs_epi32(a_lo, a_hi);

        // v1 = (x0 * c1 + x1 * c3   +   x2 * c5 + x3 * c7) >> 10;
        a_lo = _mm_madd_epi16(x01_lo, c13p);
        a_hi = _mm_madd_epi16(x01_hi, c13p);
        b_lo = _mm_madd_epi16(x23_lo, c57p);
        b_hi = _mm_madd_epi16(x23_hi, c57p);
        a_lo = _mm_add_epi32(a_lo, b_lo);
        a_hi = _mm_add_epi32(a_hi, b_hi);
        a_lo = _mm_srai_epi32(a_lo, 10);
        a_hi = _mm_srai_epi32(a_hi, 10);
        v1 = _mm_packs_epi32(a_lo, a_hi);

        JPEG_TRANSPOSE16();

        const __m128i* q = reinterpret_cast<const __m128i*>(quant_table);

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

        // v2 = (x8 * c2 + x7 * c6) >> 13;
        a_lo = _mm_madd_epi16(x87_lo, c26p);
        a_hi = _mm_madd_epi16(x87_hi, c26p);
        a_lo = _mm_srai_epi32(a_lo, 13);
        a_hi = _mm_srai_epi32(a_hi, 13);
        v2 = _mm_packs_epi32(a_lo, a_hi);

        // v6 = (x8 * c6 - x7 * c2) >> 13;
        a_lo = _mm_madd_epi16(x87_lo, c62n);
        a_hi = _mm_madd_epi16(x87_hi, c62n);
        a_lo = _mm_srai_epi32(a_lo, 13);
        a_hi = _mm_srai_epi32(a_hi, 13);
        v6 = _mm_packs_epi32(a_lo, a_hi);

        // v7 = (x0 * c7 - x1 * c5   +   x2 * c3 - x3 * c1) >> 13;
        a_lo = _mm_madd_epi16(x01_lo, c75n);
        a_hi = _mm_madd_epi16(x01_hi, c75n);
        b_lo = _mm_madd_epi16(x23_lo, c31n);
        b_hi = _mm_madd_epi16(x23_hi, c31n);
        a_lo = _mm_add_epi32(a_lo, b_lo);
        a_hi = _mm_add_epi32(a_hi, b_hi);
        a_lo = _mm_srai_epi32(a_lo, 13);
        a_hi = _mm_srai_epi32(a_hi, 13);
        v7 = _mm_packs_epi32(a_lo, a_hi);

        // v5 = (x0 * c5 - x1 * c1   +   x2 * c7 + x3 * c3) >> 13;
        a_lo = _mm_madd_epi16(x01_lo, c51n);
        a_hi = _mm_madd_epi16(x01_hi, c51n);
        b_lo = _mm_madd_epi16(x23_lo, c73p);
        b_hi = _mm_madd_epi16(x23_hi, c73p);
        a_lo = _mm_add_epi32(a_lo, b_lo);
        a_hi = _mm_add_epi32(a_hi, b_hi);
        a_lo = _mm_srai_epi32(a_lo, 13);
        a_hi = _mm_srai_epi32(a_hi, 13);
        v5 = _mm_packs_epi32(a_lo, a_hi);

        // v3 = (x0 * c3 - x1 * c7   -   x2 * c1 - x3 * c5) >> 13;
        a_lo = _mm_madd_epi16(x01_lo, c37n);
        a_hi = _mm_madd_epi16(x01_hi, c37n);
        b_lo = _mm_madd_epi16(x23_lo, c15p);
        b_hi = _mm_madd_epi16(x23_hi, c15p);
        a_lo = _mm_sub_epi32(a_lo, b_lo);
        a_hi = _mm_sub_epi32(a_hi, b_hi);
        a_lo = _mm_srai_epi32(a_lo, 13);
        a_hi = _mm_srai_epi32(a_hi, 13);
        v3 = _mm_packs_epi32(a_lo, a_hi);

        // v1 = (x0 * c1 + x1 * c3   +   x2 * c5 + x3 * c7) >> 13;
        a_lo = _mm_madd_epi16(x01_lo, c13p);
        a_hi = _mm_madd_epi16(x01_hi, c13p);
        b_lo = _mm_madd_epi16(x23_lo, c57p);
        b_hi = _mm_madd_epi16(x23_hi, c57p);
        a_lo = _mm_add_epi32(a_lo, b_lo);
        a_hi = _mm_add_epi32(a_hi, b_hi);
        a_lo = _mm_srai_epi32(a_lo, 13);
        a_hi = _mm_srai_epi32(a_hi, 13);
        v1 = _mm_packs_epi32(a_lo, a_hi);

        // NOTE: we can double the qtable values at setup
        v0 = _mm_add_epi16(v0, v0);
        v1 = _mm_add_epi16(v1, v1);
        v2 = _mm_add_epi16(v2, v2);
        v3 = _mm_add_epi16(v3, v3);
        v4 = _mm_add_epi16(v4, v4);
        v5 = _mm_add_epi16(v5, v5);
        v6 = _mm_add_epi16(v6, v6);
        v7 = _mm_add_epi16(v7, v7);
        v0 = _mm_mulhi_epi16(v0, q[0]);
        v1 = _mm_mulhi_epi16(v1, q[1]);
        v2 = _mm_mulhi_epi16(v2, q[2]);
        v3 = _mm_mulhi_epi16(v3, q[3]);
        v4 = _mm_mulhi_epi16(v4, q[4]);
        v5 = _mm_mulhi_epi16(v5, q[5]);
        v6 = _mm_mulhi_epi16(v6, q[6]);
        v7 = _mm_mulhi_epi16(v7, q[7]);

        JPEG_TRANSPOSE16();

        __m128i* d = reinterpret_cast<__m128i *>(dest);
        _mm_storeu_si128(d + 0, v0);
        _mm_storeu_si128(d + 1, v1);
        _mm_storeu_si128(d + 2, v2);
        _mm_storeu_si128(d + 3, v3);
        _mm_storeu_si128(d + 4, v4);
        _mm_storeu_si128(d + 5, v5);
        _mm_storeu_si128(d + 6, v6);
        _mm_storeu_si128(d + 7, v7);
    }

#endif

    // ----------------------------------------------------------------------------
    // read_xxx_format
    // ----------------------------------------------------------------------------

    void read_y_format(jpeg_encode* jp, s16* block, u8* input, int rows, int cols, int incr)
    {
        for (int i = 0; i < rows; ++i)
        {
            for (int j = cols; j > 0; --j)
            {
                *block++ = (*input++) - 128;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                *block = *(block - 1);
                ++block;
            }

            input += incr;
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

    void read_bgr_format(jpeg_encode* jp, s16* block, u8* input, int rows, int cols, int incr)
    {
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                int r = input[2];
                int g = input[1];
                int b = input[0];
                int y = (76 * r + 151 * g + 29 * b) >> 8;
                int cr = ((r - y) * 182) >> 8;
                int cb = ((b - y) * 144) >> 8;
                block[0 * 64] = s16(y - 128);
                block[1 * 64] = s16(cb);
                block[2 * 64] = s16(cr);
                ++block;
                input += 3;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 1];
                block[1 * 64] = block[1 * 64 - 1];
                block[2 * 64] = block[2 * 64 - 1];
                ++block;
            }

            input += incr;
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

    void read_rgb_format(jpeg_encode* jp, s16* block, u8* input, int rows, int cols, int incr)
    {
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                int r = input[0];
                int g = input[1];
                int b = input[2];
                int y = (76 * r + 151 * g + 29 * b) >> 8;
                int cr = ((r - y) * 182) >> 8;
                int cb = ((b - y) * 144) >> 8;
                block[0 * 64] = s16(y - 128);
                block[1 * 64] = s16(cb);
                block[2 * 64] = s16(cr);
                ++block;
                input += 3;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 1];
                block[1 * 64] = block[1 * 64 - 1];
                block[2 * 64] = block[2 * 64 - 1];
                ++block;
            }

            input += incr;
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

    void read_bgra_format(jpeg_encode* jp, s16* block, u8* input, int rows, int cols, int incr)
    {
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                int r = input[2];
                int g = input[1];
                int b = input[0];
                int y = (76 * r + 151 * g + 29 * b) >> 8;
                int cr = ((r - y) * 182) >> 8;
                int cb = ((b - y) * 144) >> 8;
                block[0 * 64] = s16(y - 128);
                block[1 * 64] = s16(cb);
                block[2 * 64] = s16(cr);
                ++block;
                input += 4;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 1];
                block[1 * 64] = block[1 * 64 - 1];
                block[2 * 64] = block[2 * 64 - 1];
                ++block;
            }

            input += incr;
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

    void read_rgba_format(jpeg_encode* jp, s16* block, u8* input, int rows, int cols, int incr)
    {
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                int r = input[0];
                int g = input[1];
                int b = input[2];
                int y = (76 * r + 151 * g + 29 * b) >> 8;
                int cr = ((r - y) * 182) >> 8;
                int cb = ((b - y) * 144) >> 8;
                block[0 * 64] = s16(y - 128);
                block[1 * 64] = s16(cb);
                block[2 * 64] = s16(cr);
                ++block;
                input += 4;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                block[0 * 64] = block[0 * 64 - 1];
                block[1 * 64] = block[1 * 64 - 1];
                block[2 * 64] = block[2 * 64 - 1];
                ++block;
            }

            input += incr;
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

    // ----------------------------------------------------------------------------
    // jpeg_encode methods
    // ----------------------------------------------------------------------------

    jpeg_encode::jpeg_encode(Sample sample, u32 width, u32 height, u32 stride, u32 quality)
        : ILqt(64)
        , ICqt(64)
    {
        int bytes_per_pixel = 0;

        channel_count = 0;

        channel[0].component = 1;
        channel[0].qtable = ILqt.data();

        channel[1].component = 2;
        channel[1].qtable = ICqt.data();

        channel[2].component = 3;
        channel[2].qtable = ICqt.data();

        switch (sample)
        {
            case JPEG_U8_Y:
                read_format = read_y_format;
                bytes_per_pixel = 1;
                channel_count = 1;
                break;

            case JPEG_U8_BGR:
                read_format = read_bgr_format;
                bytes_per_pixel = 3;
                channel_count = 3;
                break;

            case JPEG_U8_RGB:
                read_format = read_rgb_format;
                bytes_per_pixel = 3;
                channel_count = 3;
                break;

            case JPEG_U8_BGRA:
                read_format = read_bgra_format;
                bytes_per_pixel = 4;
                channel_count = 3;
                break;

            case JPEG_U8_RGBA:
                read_format = read_rgba_format;
                bytes_per_pixel = 4;
                channel_count = 3;
                break;
        }

        mcu_width = 8;
        mcu_height = 8;

        horizontal_mcus = (width + mcu_width - 1) >> 3;
        vertical_mcus   = (height + mcu_height - 1) >> 3;

        rows_in_bottom_mcus = height - (vertical_mcus - 1) * mcu_height;
        cols_in_right_mcus  = width  - (horizontal_mcus - 1) * mcu_width;

        length_minus_mcu_width = stride - mcu_width * bytes_per_pixel;
        length_minus_width     = stride - cols_in_right_mcus * bytes_per_pixel;

        mcu_width_size = mcu_width * bytes_per_pixel;

        init_quantization_tables(quality);
    }

    jpeg_encode::~jpeg_encode()
    {
    }

    void jpeg_encode::init_quantization_tables(u32 quality)
    {
        quality = std::min(quality, 1024u) * 16;

        for (int i = 0; i < 64; ++i)
        {
            int i_transpose = ((i & 7) << 3) | (i >> 3);
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
            ILqt [i_transpose] = u16(0x8000 / value);

            // chrominance quantization table * quality factor
            value = chrominance_quant_table [i] * quality;
            value = (value + 0x200) >> 10;

            if (value < 2)
                value = 2;
            else if (value > 255)
                value = 255;

            Cqt [index] = (u8) value;
            ICqt [i_transpose] = u16(0x8000 / value);
        }
    }

    void jpeg_encode::write_markers(BigEndianStream& p, Sample sample, u32 width, u32 height)
    {
        // Start of image marker
        p.write16(0xffd8);

        // Quantization table marker
        p.write16(0xffdb);
        p.write16(0x43); // quantization table length
        p.write8(0x00); // Pq, Tq

        // Lqt table
        p.write(Lqt, 64);

        // Quantization table marker
        p.write16(0xffdb);
        p.write16(0x43); // quantization table length
        p.write8(0x01); // Pq, Tq

        // Cqt table
        p.write(Cqt, 64);

        // Start of frame marker
        p.write16(0xffc0);

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
        p.write16(0xffdd);
        p.write16(4);
        p.write16(horizontal_mcus);

        // Start of scan marker
        p.write16(0xffda);
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

    void encodeJPEG(const Surface& surface, Stream& stream, int quality, Sample sample)
    {
        jpeg_encode jp(sample, surface.width, surface.height, surface.stride, quality);

        u8* input = surface.image;

        // bitstream for each MCU scan
        Buffer* buffers = new Buffer[jp.vertical_mcus];

        ConcurrentQueue queue;

        // encode MCUs
        for (int y = 0; y < jp.vertical_mcus; ++y)
        {
            int rows;

            const int bottom_mcu = jp.vertical_mcus - 1;
            if (y < bottom_mcu)
            {
                rows = jp.mcu_height;
            }
            else
            {
                // clipping
                rows = jp.rows_in_bottom_mcus;
            }

            queue.enqueue([&jp, y, buffers, input, rows]
            {
                u8* image = input;

                HuffmanEncoder huffman;

                constexpr int buffer_size = 2048;
                constexpr int flush_threshold = buffer_size - 512;

                u8 huff_temp[buffer_size]; // encoding buffer
                u8* ptr = huff_temp;

                const int right_mcu = jp.horizontal_mcus - 1;

                for (int x = 0; x < jp.horizontal_mcus; ++x)
                {
                    int cols;
                    int incr;

                    if (x < right_mcu)
                    {
                        cols = jp.mcu_width;
                        incr = jp.length_minus_mcu_width;
                    }
                    else
                    {
                        // clipping
                        cols = jp.cols_in_right_mcus;
                        incr = jp.length_minus_width;
                    }

                    s16 block[BLOCK_SIZE * 3];

                    // read MCU data
                    jp.read_format(&jp, block, image, rows, cols, incr);

                    // encode the data in MCU
                    for (int i = 0; i < jp.channel_count; ++i)
                    {
                        s16 temp[BLOCK_SIZE];
                        fdct(temp, block + i * BLOCK_SIZE, jp.channel[i].qtable);
                        ptr = huffman.encode(ptr, jp.channel[i].component, temp);
                    }

                    // flush encoding buffer
                    if (ptr - huff_temp > flush_threshold)
                    {
                        buffers[y].write(huff_temp, ptr - huff_temp);
                        ptr = huff_temp;
                    }

                    image += jp.mcu_width_size;
                }

                // flush encoding buffer
                ptr = huffman.flush(ptr);
                buffers[y].write(huff_temp, ptr - huff_temp);
            });

            input += surface.stride * jp.mcu_height;
        }

        queue.wait();

        BigEndianStream s(stream);

        // writing marker data
        jp.write_markers(s, sample, surface.width, surface.height);

        for (int y = 0; y < jp.vertical_mcus; ++y)
        {
            Buffer& buffer = buffers[y];

            // write huffman bitstream
            s.write(buffer, size_t(buffer.size()));

            // write restart marker
            int index = y & 7;
            s.write16(0xffd0 + index);
        }

        delete[] buffers;

        // EOI marker
        s.write16(0xffd9);
    }

} // namespace

namespace mango {
namespace jpeg {

    SampleFormat getSampleFormat(const Format& format)
    {
        // set default format
        SampleFormat result { JPEG_U8_RGBA, FORMAT_R8G8B8A8 };

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

    void encodeImage(Stream& stream, const Surface& surface, float quality)
    {
        // configure quality
        quality = clamp(1.0f - quality, 0.0f, 1.0f);
        const u32 iq = u32(quality * 1024);

        SampleFormat sf = getSampleFormat(surface.format);

        // encode
        if (surface.format == sf.format)
        {
            encodeJPEG(surface, stream, iq, sf.sample);
        }
        else
        {
            // convert source surface to format supported in the encoder
            Bitmap temp(surface.width, surface.height, sf.format);
            temp.blit(0, 0, surface);
            encodeJPEG(temp, stream, iq, sf.sample);
        }
    }

} // namespace jpeg
} // namespace mango
