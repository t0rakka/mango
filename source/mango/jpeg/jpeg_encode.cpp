/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
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

// NOTE: Some parts of the code still use the SRV-1 license where this code started from,
//       it has very little original code left but enough that we MUST respect the GPL license!!!
#ifdef MANGO_ENABLE_LICENSE_GPL

namespace
{
    using namespace mango;
    using namespace jpeg;

    constexpr int BLOCK_SIZE = 64;

    // Table K.1 – Luminance quantization table

    const u8 g_luminance_quant_table [] =
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

    // Table K.2 – Chrominance quantization table

    const u8 g_chrominance_quant_table [] =
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

    // Table K.3 – Table for luminance DC coefficient differences

    const u16 g_luminance_dc_code_table [] =
    {
        0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE
    };

    const u16 g_luminance_dc_size_table [] =
    {
        0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009
    };

    // Table K.4 – Table for chrominance DC coefficient differences

    const u16 g_chrominance_dc_code_table [] =
    {
        0x0000, 0x0001, 0x0002, 0x0006, 0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE, 0x03FE, 0x07FE
    };

    const u16 g_chrominance_dc_size_table [] =
    {
        0x0002, 0x0002, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B
    };

    // Table K.5 – Table for luminance AC coefficients

    alignas(64)
    const u16 g_luminance_ac_code_table [] =
    {
        0x000A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x0000, 0x000C, 0x001C, 0x003A, 0x003B, 0x007A, 0x007B, 0x00FA, 0x01F8, 0x01F9, 0x01FA, 0x03F9, 0x03FA, 0x07F8, 0xFFEB, 0xFFF5,
        0x0001, 0x001B, 0x00F9, 0x01F7, 0x03F8, 0x07F7, 0x0FF6, 0x0FF7, 0x7FC0, 0xFFBE, 0xFFC7, 0xFFD0, 0xFFD9, 0xFFE2, 0xFFEC, 0xFFF6,
        0x0004, 0x0079, 0x03F7, 0x0FF5, 0xFF96, 0xFF9E, 0xFFA6, 0xFFAE, 0xFFB6, 0xFFBF, 0xFFC8, 0xFFD1, 0xFFDA, 0xFFE3, 0xFFED, 0xFFF7,
        0x000B, 0x01F6, 0x0FF4, 0xFF8F, 0xFF97, 0xFF9F, 0xFFA7, 0xFFAF, 0xFFB7, 0xFFC0, 0xFFC9, 0xFFD2, 0xFFDB, 0xFFE4, 0xFFEE, 0xFFF8,
        0x001A, 0x07F6, 0xFF89, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC1, 0xFFCA, 0xFFD3, 0xFFDC, 0xFFE5, 0xFFEF, 0xFFF9,
        0x0078, 0xFF84, 0xFF8A, 0xFF91, 0xFF99, 0xFFA1, 0xFFA9, 0xFFB1, 0xFFB9, 0xFFC2, 0xFFCB, 0xFFD4, 0xFFDD, 0xFFE6, 0xFFF0, 0xFFFA,
        0x00F8, 0xFF85, 0xFF8b, 0xFF92, 0xFF9A, 0xFFA2, 0xFFAA, 0xFFB2, 0xFFBA, 0xFFC3, 0xFFCC, 0xFFD5, 0xFFDE, 0xFFE7, 0xFFF1, 0xFFFB,
        0x03F6, 0xFF86, 0xFF8C, 0xFF93, 0xFF9B, 0xFFA3, 0xFFAB, 0xFFB3, 0xFFBB, 0xFFC4, 0xFFCD, 0xFFD6, 0xFFDF, 0xFFE8, 0xFFF2, 0xFFFC,
        0xFF82, 0xFF87, 0xFF8D, 0xFF94, 0xFF9C, 0xFFA4, 0xFFAC, 0xFFB4, 0xFFBC, 0xFFC5, 0xFFCE, 0xFFD7, 0xFFE0, 0xFFE9, 0xFFF3, 0xFFFD,
        0xFF83, 0xFF88, 0xFF8E, 0xFF95, 0xFF9D, 0xFFA5, 0xFFAD, 0xFFB5, 0xFFBD, 0xFFC6, 0xFFCF, 0xFFD8, 0xFFE1, 0xFFEA, 0xFFF4, 0xFFFE,
        0x07F9
    };

    alignas(64)
    const u16 g_luminance_ac_size_table [] =
    {
        0x0004, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x0002, 0x0004, 0x0005, 0x0006, 0x0006, 0x0007, 0x0007, 0x0008, 0x0009, 0x0009, 0x0009, 0x000A, 0x000A, 0x000B, 0x0010, 0x0010,
        0x0002, 0x0005, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000C, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0003, 0x0007, 0x000A, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0004, 0x0009, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0005, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0007, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0008, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
        0x000B
    };

    // Table K.6 – Table for chrominance AC coefficients

    alignas(64)
    const u16 g_chrominance_ac_code_table [] =
    {
        0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x0001, 0x000B, 0x001A, 0x001B, 0x003A, 0x003B, 0x0079, 0x007A, 0x00F9, 0x01F7, 0x01F8, 0x01F9, 0x01FA, 0x07F9, 0x3FE0, 0x7FC3, 
        0x0004, 0x0039, 0x00F7, 0x00F8, 0x01F6, 0x03F9, 0x07F7, 0x07F8, 0xFFB7, 0xFFC0, 0xFFC9, 0xFFD2, 0xFFDB, 0xFFE4, 0xFFED, 0xFFF6, 
        0x000A, 0x00F6, 0x03F7, 0x03F8, 0xFF97, 0xFF9F, 0xFFA7, 0xFFAF, 0xFFB8, 0xFFC1, 0xFFCA, 0xFFD3, 0xFFDC, 0xFFE5, 0xFFEE, 0xFFF7, 
        0x0018, 0x01F5, 0x0FF6, 0x0FF7, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB9, 0xFFC2, 0xFFCB, 0xFFD4, 0xFFDD, 0xFFE6, 0xFFEF, 0xFFF8, 
        0x0019, 0x07F6, 0x7FC2, 0xFF91, 0xFF99, 0xFFA1, 0xFFA9, 0xFFB1, 0xFFBA, 0xFFC3, 0xFFCC, 0xFFD5, 0xFFDE, 0xFFE7, 0xFFF0, 0xFFF9, 
        0x0038, 0x0FF5, 0xFF8C, 0xFF92, 0xFF9A, 0xFFA2, 0xFFAA, 0xFFB2, 0xFFBB, 0xFFC4, 0xFFCD, 0xFFD6, 0xFFDF, 0xFFE8, 0xFFF1, 0xFFFA, 
        0x0078, 0xFF88, 0xFF8D, 0xFF93, 0xFF9B, 0xFFA3, 0xFFAB, 0xFFB3, 0xFFBC, 0xFFC5, 0xFFCE, 0xFFD7, 0xFFE0, 0xFFE9, 0xFFF2, 0xFFFB, 
        0x01F4, 0xFF89, 0xFF8E, 0xFF94, 0xFF9C, 0xFFA4, 0xFFAC, 0xFFB4, 0xFFBD, 0xFFC6, 0xFFCF, 0xFFD8, 0xFFE1, 0xFFEA, 0xFFF3, 0xFFFC, 
        0x03F6, 0xFF8A, 0xFF8F, 0xFF95, 0xFF9D, 0xFFA5, 0xFFAD, 0xFFB5, 0xFFBE, 0xFFC7, 0xFFD0, 0xFFD9, 0xFFE2, 0xFFEb, 0xFFF4, 0xFFFD, 
        0x0FF4, 0xFF8B, 0xFF90, 0xFF96, 0xFF9E, 0xFFA6, 0xFFAE, 0xFFB6, 0xFFBF, 0xFFC8, 0xFFD1, 0xFFDA, 0xFFE3, 0xFFEC, 0xFFF5, 0xFFFE,
        0x03FA
    };

    alignas(64)
    const u16 g_chrominance_ac_size_table [] =
    {
        0x0002, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x0002, 0x0004, 0x0005, 0x0005, 0x0006, 0x0006, 0x0007, 0x0007, 0x0008, 0x0009, 0x0009, 0x0009, 0x0009, 0x000B, 0x000E, 0x000F, 
        0x0003, 0x0006, 0x0008, 0x0008, 0x0009, 0x000A, 0x000B, 0x000B, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 
        0x0004, 0x0008, 0x000A, 0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 
        0x0005, 0x0009, 0x000C, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 
        0x0005, 0x000B, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 
        0x0006, 0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 
        0x0007, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 
        0x0009, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 
        0x000A, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 
        0x000C, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
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

    static inline
    u8* writeStuffedBytes(u8* output, DataType code, int count)
    {
        code = byteswap(code);
        for (int i = 0; i < count; ++i)
        {
            u8 value = u8(code);
            code >>= 8;
            *output++ = value;

            // always write the stuff byte
            *output = 0;

            // .. but advance ptr only when it actually was one
            output += (value == 0xff);
        }
        return output;
    }

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
    };

    struct jpegEncoder
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
        struct Channel
        {
            int component;
            const s16* qtable;
            const u16* dc_code;
            const u16* dc_size;
            const u16* ac_code;
            const u16* ac_size;
        };
        Channel channel[3];
        int     components;

        std::string info;

        void (*read_8x8)  (s16* block, const u8* input, size_t stride, int rows, int cols);
        void (*read)      (s16* block, const u8* input, size_t stride, int rows, int cols);
        void (*fdct)      (s16* dest, const s16* data, const s16* quant_table);
        u8*  (*encode_ac) (HuffmanEncoder& encoder, u8* p, const s16* input, const u16* ac_code, const u16* ac_size);

        jpegEncoder(SampleType sample, u32 width, u32 height, size_t stride, u32 quality);
        ~jpegEncoder();

        void writeMarkers(BigEndianStream& p, SampleType sample, u32 width, u32 height);

        ConstMemory icc;
    };

    static inline
    u8* encode_dc(HuffmanEncoder& encoder, u8* p, const s16* input, const u16* dc_code, const u16* dc_size, int last_dc)
    {
        int coeff = input[0] - last_dc;
        int absCoeff = std::abs(coeff);
        coeff -= (absCoeff != coeff);

        u32 size = absCoeff ? u32_log2(absCoeff) + 1 : 0;
        u32 mask = (1 << size) - 1;

        p = encoder.putBits(p, dc_code[size], dc_size[size]);
        p = encoder.putBits(p, coeff & mask, size);

        return p;
    }

    static
    u8* encode_ac_scalar(HuffmanEncoder& encoder, u8* p, const s16* input, const u16* ac_code, const u16* ac_size)
    {
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

        int runLength = 0;

        for (int i = 1; i < 64; ++i)
        {
            int coeff = input[zigzag_table_inverse[i]];
            if (coeff)
            {
                while (runLength > 15)
                {
                    runLength -= 16;
                    p = encoder.putBits(p, ac_code[176], ac_size[176]);
                }

                int absCoeff = std::abs(coeff);
                coeff -= (absCoeff != coeff);

                u32 size = u32_log2(absCoeff) + 1;
                u32 mask = (1 << size) - 1;

                int index = runLength + size * 16;
                p = encoder.putBits(p, ac_code[index], ac_size[index]);
                p = encoder.putBits(p, coeff & mask, size);

                runLength = 0;
            }
            else
            {
                ++runLength;
            }
        }

        if (runLength != 0)
        {
            p = encoder.putBits(p, ac_code[0], ac_size[0]);
        }

        return p;
    }

#if defined(JPEG_ENABLE_SSE4)

    // ----------------------------------------------------------------------------
    // encode_ac_ssse3
    // ----------------------------------------------------------------------------

    constexpr s8 lane(s8 v, s8 offset)
    {
        return v == -1 ? -1 : (v & 7) * 2 + offset;
    }

    static inline
    int16x8 shuffle(__m128i v, s8 c0, s8 c1, s8 c2, s8 c3, s8 c4, s8 c5, s8 c6, s8 c7)
    {
        const __m128i s = _mm_setr_epi8(
            lane(c0, 0), lane(c0, 1), lane(c1, 0), lane(c1, 1),
            lane(c2, 0), lane(c2, 1), lane(c3, 0), lane(c3, 1),
            lane(c4, 0), lane(c4, 1), lane(c5, 0), lane(c5, 1),
            lane(c6, 0), lane(c6, 1), lane(c7, 0), lane(c7, 1));
        return int16x8(_mm_shuffle_epi8(v, s));
    }

    u64 zigzag_ssse3(const s16* in, s16* out)
    {
        const __m128i* src = reinterpret_cast<const __m128i *>(in);

        const __m128i A = _mm_loadu_si128(src + 0); //  0 .. 7
        const __m128i B = _mm_loadu_si128(src + 1); //  8 .. 15
        const __m128i C = _mm_loadu_si128(src + 2); // 16 .. 23
        const __m128i D = _mm_loadu_si128(src + 3); // 24 .. 31
        const __m128i E = _mm_loadu_si128(src + 4); // 32 .. 39
        const __m128i F = _mm_loadu_si128(src + 5); // 40 .. 47
        const __m128i G = _mm_loadu_si128(src + 6); // 48 .. 55
        const __m128i H = _mm_loadu_si128(src + 7); // 56 .. 63

        constexpr s8 z = -1;

        // ------------------------------------------------------------------------
        //     0,  1,  8, 16,  9,  2,  3, 10,
        // ------------------------------------------------------------------------
        // A:  x   x   -   -   -   x   x   -
        // B:  -   -   x   -   x   -   -   x
        // C:  -   -   -   x   -   -   -   -
        // D:  -   -   -   -   -   -   -   -
        // E:  -   -   -   -   -   -   -   -
        // F:  -   -   -   -   -   -   -   -
        // G:  -   -   -   -   -   -   -   -
        // H:  -   -   -   -   -   -   -   -

        __m128i row0 = shuffle(A, 0, 1, z,  z, z, 2, 3,  z) |
                       shuffle(B, z, z, 8,  z, 9, z, z, 10) |
                       shuffle(C, z, z, z, 16, z, z, z,  z);

        // ------------------------------------------------------------------------
        //    17, 24, 32, 25, 18, 11,  4,  5,
        // ------------------------------------------------------------------------
        // A:  -   -   -   -   -   -   x   x
        // B:  -   -   -   -   -   x   -   -
        // C:  x   -   -   -   x   -   -   -
        // D:  -   x   -   x   -   -   -   -
        // E:  -   -   x   -   -   -   -   -
        // F:  -   -   -   -   -   -   -   -
        // G:  -   -   -   -   -   -   -   -
        // H:  -   -   -   -   -   -   -   -

        __m128i row1 = shuffle(A,  z,  z, z,  z,  z,  z, 4, 5) |
                       shuffle(B,  z,  z, z,  z,  z, 11, z, z) |
                       shuffle(C, 17,  z, z,  z, 18,  z, z, z) |
                       shuffle(D,  z, 24, z, 25,  z,  z, z, z) |
                       shuffle(E,  z, z, 32,  z,  z,  z, z, z);

        // ------------------------------------------------------------------------
        //    12, 19, 26, 33, 40, 48, 41, 34,
        // ------------------------------------------------------------------------
        // A:  -   -   -   -   -   -   -   -
        // B:  x   -   -   -   -   -   -   -
        // C:  -   x   -   -   -   -   -   -
        // D:  -   -   x   -   -   -   -   -
        // E:  -   -   -   x   -   -   -   x
        // F:  -   -   -   -   x   -   x   -
        // G:  -   -   -   -   -   x   -   -
        // H:  -   -   -   -   -   -   -   -

        __m128i row2 = shuffle(B, 12,  z,  z,  z,  z,  z,  z,  z) |
                       shuffle(C,  z, 19,  z,  z,  z,  z,  z,  z) |
                       shuffle(D,  z,  z, 26,  z,  z,  z,  z,  z) |
                       shuffle(E,  z,  z,  z, 33,  z,  z,  z, 34) |
                       shuffle(F,  z,  z,  z,  z, 40,  z, 41,  z) |
                       shuffle(G,  z,  z,  z,  z,  z, 48,  z,  z);

        // ------------------------------------------------------------------------
        //    27, 20, 13,  6,  7, 14, 21, 28,
        // ------------------------------------------------------------------------
        // A:  -   -   -   x   x   -   -   -
        // B:  -   -   x   -   -   x   -   -
        // C:  -   x   -   -   -   -   x   -
        // D:  x   -   -   -   -   -   -   x
        // E:  -   -   -   -   -   -   -   -
        // F:  -   -   -   -   -   -   -   -
        // G:  -   -   -   -   -   -   -   -
        // H:  -   -   -   -   -   -   -   -

        __m128i row3 = shuffle(A,  z,  z,  z, 6, 7,  z,  z,  z) |
                       shuffle(B,  z,  z, 13, z, z, 14,  z,  z) |
                       shuffle(C,  z, 20,  z, z, z,  z, 21,  z) |
                       shuffle(D, 27,  z,  z, z, z,  z,  z, 28);

        // ------------------------------------------------------------------------
        //    35, 42, 49, 56, 57, 50, 43, 36,
        // ------------------------------------------------------------------------
        // A:  -   -   -   -   -   -   -   -
        // B:  -   -   -   -   -   -   -   -
        // C:  -   -   -   -   -   -   -   -
        // D:  -   -   -   -   -   -   -   -
        // E:  x   -   -   -   -   -   -   x
        // F:  -   x   -   -   -   -   x   -
        // G:  -   -   x   -   -   x   -   -
        // H:  -   -   -   x   x   -   -   -

        __m128i row4 = shuffle(E, 35,  z,  z,  z,  z,  z,  z, 36) |
                       shuffle(F,  z, 42,  z,  z,  z,  z, 43,  z) |
                       shuffle(G,  z,  z, 49,  z,  z, 50,  z,  z) |
                       shuffle(H,  z,  z,  z, 56, 57,  z,  z,  z);

        // ------------------------------------------------------------------------
        //    29, 22, 15, 23, 30, 37, 44, 51,
        // ------------------------------------------------------------------------
        // A:  -   -   -   -   -   -   -   -
        // B:  -   -   x   -   -   -   -   -
        // C:  -   x   -   x   -   -   -   -
        // D:  x   -   -   -   x   -   -   -
        // E:  -   -   -   -   -   x   -   -
        // F:  -   -   -   -   -   -   x   -
        // G:  -   -   -   -   -   -   -   x
        // H:  -   -   -   -   -   -   -   -

        __m128i row5 = shuffle(B,  z,  z, 15,  z,  z,  z,  z,  z) |
                       shuffle(C,  z, 22,  z, 23,  z,  z,  z,  z) |
                       shuffle(D, 29,  z,  z,  z, 30,  z,  z,  z) |
                       shuffle(E,  z,  z,  z,  z,  z, 37,  z,  z) |
                       shuffle(F,  z,  z,  z,  z,  z,  z, 44,  z) |
                       shuffle(G,  z,  z,  z,  z,  z,  z,  z, 51);

        // ------------------------------------------------------------------------
        //    58, 59, 52, 45, 38, 31, 39, 46,
        // ------------------------------------------------------------------------
        // A:  -   -   -   -   -   -   -   -
        // B:  -   -   -   -   -   -   -   -
        // C:  -   -   -   -   -   -   -   -
        // D:  -   -   -   -   -   x   -   -
        // E:  -   -   -   -   x   -   x   -
        // F:  -   -   -   x   -   -   -   x
        // G:  -   -   x   -   -   -   -   -
        // H:  x   x   -   -   -   -   -   -

        __m128i row6 = shuffle(D,  z,  z,  z,  z,  z, 31,  z,  z) |
                       shuffle(E,  z,  z,  z,  z, 38,  z, 39,  z) |
                       shuffle(F,  z,  z,  z, 45,  z,  z,  z, 46) |
                       shuffle(G,  z,  z, 52,  z,  z,  z,  z,  z) |
                       shuffle(H, 58, 59,  z,  z,  z,  z,  z,  z);

        // ------------------------------------------------------------------------
        //    53, 60, 61, 54, 47, 55, 62, 63,
        // ------------------------------------------------------------------------
        // A:  -   -   -   -   -   -   -   -
        // B:  -   -   -   -   -   -   -   -
        // C:  -   -   -   -   -   -   -   -
        // D:  -   -   -   -   -   -   -   -
        // E:  -   -   -   -   -   -   -   -
        // F:  -   -   -   -   x   -   -   -
        // G:  x   -   -   x   -   x   -   -
        // H:  -   x   x   -   -   -   x   x

        __m128i row7 = shuffle(F,  z,  z,  z,  z, 47,  z,  z,  z) |
                       shuffle(G, 53,  z,  z, 54,  z, 55,  z,  z) |
                       shuffle(H,  z, 60, 61,  z,  z,  z, 62, 63);

        // compute zeromask
        const __m128i zero = _mm_setzero_si128();
        __m128i zero0 = _mm_packs_epi16(_mm_cmpeq_epi16(row0, zero), _mm_cmpeq_epi16(row1, zero));
        __m128i zero1 = _mm_packs_epi16(_mm_cmpeq_epi16(row2, zero), _mm_cmpeq_epi16(row3, zero));
        __m128i zero2 = _mm_packs_epi16(_mm_cmpeq_epi16(row4, zero), _mm_cmpeq_epi16(row5, zero));
        __m128i zero3 = _mm_packs_epi16(_mm_cmpeq_epi16(row6, zero), _mm_cmpeq_epi16(row7, zero));
        u32 mask_lo = _mm_movemask_epi8(zero0) | ((_mm_movemask_epi8(zero1)) << 16);
        u32 mask_hi = _mm_movemask_epi8(zero2) | ((_mm_movemask_epi8(zero3)) << 16);
        u64 zeromask = mask_lo;

        __m128i* dest = reinterpret_cast<__m128i *>(out);

        _mm_storeu_si128(dest + 0, row0);
        _mm_storeu_si128(dest + 1, row1);
        _mm_storeu_si128(dest + 2, row2);
        _mm_storeu_si128(dest + 3, row3);

        if (mask_hi)
        {
            zeromask |= (u64(mask_hi) << 32);
            _mm_storeu_si128(dest + 4, row4);
            _mm_storeu_si128(dest + 5, row5);
            _mm_storeu_si128(dest + 6, row6);
            _mm_storeu_si128(dest + 7, row7);
        }

        return ~zeromask;
    }

    static
    u8* encode_ac_ssse3(HuffmanEncoder& encoder, u8* p, const s16* input, const u16* ac_code, const u16* ac_size)
    {
        s16 temp[64];
        u64 zeromask = zigzag_ssse3(input, temp);

        zeromask >>= 1; // skip DC

        for (int i = 1; i < 64; )
        {
            if (!zeromask)
            {
                // only zeros left
                p = encoder.putBits(p, ac_code[0], ac_size[0]);
                break;
            }

            int runLength = u64_tzcnt(zeromask); // BMI
            zeromask >>= (runLength + 1);
            i += runLength;

            while (runLength > 15)
            {
                runLength -= 16;
                p = encoder.putBits(p, ac_code[176], ac_size[176]);
            }

            int coeff = temp[i++];
            int absCoeff = std::abs(coeff);
            coeff -= (absCoeff != coeff);

            u32 size = u32_log2(absCoeff) + 1;
            u32 mask = (1 << size) - 1;

            int index = runLength + size * 16;
            p = encoder.putBits(p, ac_code[index], ac_size[index]);
            p = encoder.putBits(p, coeff & mask, size);
        }

        return p;
    }

#if defined(JPEG_ENABLE_AVX2)

    // ----------------------------------------------------------------------------
    // encode_ac_avx2
    // ----------------------------------------------------------------------------

    // NOTE: The zigzag is still 128 bit wide only because of the retarded 128+128 way the AVX2 works
    // TODO: re-arrange (if possible) so that 256 bit shuffle can be used, in this form this is useless

    /*
    inline int16x16 shuffle(__m256i v, s8 c0, s8 c1, s8 c2, s8 c3, s8 c4, s8 c5, s8 c6, s8 c7,
                                       s8 c8, s8 c9, s8 ca, s8 cb, s8 cc, s8 cd, s8 ce, s8 cf)
    {
        const __m256i s = _mm256_setr_epi8(
            lane(c0, 0), lane(c0, 1), lane(c1, 0), lane(c1, 1),
            lane(c2, 0), lane(c2, 1), lane(c3, 0), lane(c3, 1),
            lane(c4, 0), lane(c4, 1), lane(c5, 0), lane(c5, 1),
            lane(c6, 0), lane(c6, 1), lane(c7, 0), lane(c7, 1),
            lane(c8, 0), lane(c8, 1), lane(c9, 0), lane(c9, 1),
            lane(ca, 0), lane(ca, 1), lane(cb, 0), lane(cb, 1),
            lane(cc, 0), lane(cc, 1), lane(cd, 0), lane(cd, 1),
            lane(ce, 0), lane(ce, 1), lane(cf, 0), lane(cf, 1));
        return int16x16(_mm256_shuffle_epi8(v, s));
    }
    */

    u64 zigzag_avx2(const s16* in, s16* out)
    {
        const __m256i* src = reinterpret_cast<const __m256i *>(in);

        const __m256i AB = _mm256_loadu_si256(src + 0); //  0 .. 15
        const __m256i CD = _mm256_loadu_si256(src + 1); // 16 .. 31
        const __m256i EF = _mm256_loadu_si256(src + 2); // 32 .. 47
        const __m256i GH = _mm256_loadu_si256(src + 3); // 48 .. 63

        const __m128i A = _mm256_extracti128_si256(AB, 0);
        const __m128i B = _mm256_extracti128_si256(AB, 1);
        const __m128i C = _mm256_extracti128_si256(CD, 0);
        const __m128i D = _mm256_extracti128_si256(CD, 1);
        const __m128i E = _mm256_extracti128_si256(EF, 0);
        const __m128i F = _mm256_extracti128_si256(EF, 1);
        const __m128i G = _mm256_extracti128_si256(GH, 0);
        const __m128i H = _mm256_extracti128_si256(GH, 1);

        constexpr s8 z = -1;

        // ------------------------------------------------------------------------
        //     0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
        // ------------------------------------------------------------------------
        // A:  x   x   x   -   x   x   x   x   -   -   -   -   -   x   x   x
        // B:  -   -   -   x   -   -   -   -   x   x   -   x   x   -   -   -
        // C:  -   -   -   -   -   -   -   -   -   -   x   -   -   -   -   -
        // D:  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

        __m128i row0 = shuffle(A, 0, 1, z,  z, z, 2, 3,  z) |
                       shuffle(B, z, z, 8,  z, 9, z, z, 10) |
                       shuffle(C, z, z, z, 16, z, z, z,  z);

        __m128i row1 = shuffle(A,  z,  z, z,  z,  z,  z, 4, 5) |
                       shuffle(B,  z,  z, z,  z,  z, 11, z, z) |
                       shuffle(C, 17,  z, z,  z, 18,  z, z, z) |
                       shuffle(D,  z, 24, z, 25,  z,  z, z, z) |
                       shuffle(E,  z, z, 32,  z,  z,  z, z, z);

        const __m256i row01 = _mm256_setr_m128i(row0, row1);

        // ------------------------------------------------------------------------
        //    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
        // ------------------------------------------------------------------------
        // A:  x   -   -   -   -   -   -   -   -   -   x   x   x   x   -   -
        // B:  -   x   x   -   -   -   -   -   x   x   -   -   -   -   x   x
        // C:  -   -   -   x   x   -   x   x   -   -   -   -   -   -   -   -
        // D:  -   -   -   -   -   x   -   -   -   -   -   -   -   -   -   -

        __m128i row2 = shuffle(B, 12,  z,  z,  z,  z,  z,  z,  z) |
                       shuffle(C,  z, 19,  z,  z,  z,  z,  z,  z) |
                       shuffle(D,  z,  z, 26,  z,  z,  z,  z,  z) |
                       shuffle(E,  z,  z,  z, 33,  z,  z,  z, 34) |
                       shuffle(F,  z,  z,  z,  z, 40,  z, 41,  z) |
                       shuffle(G,  z,  z,  z,  z,  z, 48,  z,  z);

        __m128i row3 = shuffle(A,  z,  z,  z, 6, 7,  z,  z,  z) |
                       shuffle(B,  z,  z, 13, z, z, 14,  z,  z) |
                       shuffle(C,  z, 20,  z, z, z,  z, 21,  z) |
                       shuffle(D, 27,  z,  z, z, z,  z,  z, 28);

        const __m256i row23 = _mm256_setr_m128i(row2, row3);

        // ------------------------------------------------------------------------
        //    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
        // ------------------------------------------------------------------------
        // A:  -   -   -   -   -   -   -   -   -   -   x   -   -   -   -   -
        // B:  -   -   -   -   -   -   -   -   x   x   -   x   x   -   -   -
        // C:  x   x   -   -   -   -   x   x   -   -   -   -   -   x   x   -
        // D:  -   -   x   x   x   x   -   -   -   -   -   -   -   -   -   x

        __m128i row4 = shuffle(E, 35,  z,  z,  z,  z,  z,  z, 36) |
                       shuffle(F,  z, 42,  z,  z,  z,  z, 43,  z) |
                       shuffle(G,  z,  z, 49,  z,  z, 50,  z,  z) |
                       shuffle(H,  z,  z,  z, 56, 57,  z,  z,  z);

        __m128i row5 = shuffle(B,  z,  z, 15,  z,  z,  z,  z,  z) |
                       shuffle(C,  z, 22,  z, 23,  z,  z,  z,  z) |
                       shuffle(D, 29,  z,  z,  z, 30,  z,  z,  z) |
                       shuffle(E,  z,  z,  z,  z,  z, 37,  z,  z) |
                       shuffle(F,  z,  z,  z,  z,  z,  z, 44,  z) |
                       shuffle(G,  z,  z,  z,  z,  z,  z,  z, 51);

        const __m256i row45 = _mm256_setr_m128i(row4, row5);

        // ------------------------------------------------------------------------
        //    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
        // ------------------------------------------------------------------------
        // A:  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
        // B:  -   -   -   -   -   x   -   -   -   -   -   -   -   -   -   -
        // C:  -   -   -   x   x   -   x   x   -   -   -   -   x   -   -   -
        // D:  x   x   x   -   -   -   -   -   x   x   x   x   -   x   x   x

        __m128i row6 = shuffle(D,  z,  z,  z,  z,  z, 31,  z,  z) |
                       shuffle(E,  z,  z,  z,  z, 38,  z, 39,  z) |
                       shuffle(F,  z,  z,  z, 45,  z,  z,  z, 46) |
                       shuffle(G,  z,  z, 52,  z,  z,  z,  z,  z) |
                       shuffle(H, 58, 59,  z,  z,  z,  z,  z,  z);

        __m128i row7 = shuffle(F,  z,  z,  z,  z, 47,  z,  z,  z) |
                       shuffle(G, 53,  z,  z, 54,  z, 55,  z,  z) |
                       shuffle(H,  z, 60, 61,  z,  z,  z, 62, 63);

        const __m256i row67 = _mm256_setr_m128i(row6, row7);

        // compute zeromask
        const __m256i zero = _mm256_setzero_si256();
        __m256i zero0 = _mm256_packs_epi16(_mm256_cmpeq_epi16(row01, zero), _mm256_cmpeq_epi16(row23, zero));
        __m256i zero1 = _mm256_packs_epi16(_mm256_cmpeq_epi16(row45, zero), _mm256_cmpeq_epi16(row67, zero));
        zero0 = _mm256_permute4x64_epi64(zero0, 0xd8);
        zero1 = _mm256_permute4x64_epi64(zero1, 0xd8);
        u32 mask_lo = _mm256_movemask_epi8(zero0);
        u32 mask_hi = _mm256_movemask_epi8(zero1);
        u64 zeromask = mask_lo;

        __m256i* dest = reinterpret_cast<__m256i *>(out);

        _mm256_storeu_si256(dest + 0, row01);
        _mm256_storeu_si256(dest + 1, row23);

        if (mask_hi)
        {
            zeromask |= (u64(mask_hi) << 32);
            _mm256_storeu_si256(dest + 2, row45);
            _mm256_storeu_si256(dest + 3, row67);
        }

        return ~zeromask;
    }

    static
    u8* encode_ac_avx2(HuffmanEncoder& encoder, u8* p, const s16* input, const u16* ac_code, const u16* ac_size)
    {
        s16 temp[64];
        u64 zeromask = zigzag_avx2(input, temp);

        zeromask >>= 1; // skip DC

        for (int i = 1; i < 64; )
        {
            if (!zeromask)
            {
                // only zeros left
                p = encoder.putBits(p, ac_code[0], ac_size[0]);
                break;
            }

            int runLength = u64_tzcnt(zeromask); // BMI
            zeromask >>= (runLength + 1);
            i += runLength;

            while (runLength > 15)
            {
                runLength -= 16;
                p = encoder.putBits(p, ac_code[176], ac_size[176]);
            }

            int coeff = temp[i++];
            int absCoeff = std::abs(coeff);
            coeff -= (absCoeff != coeff);

            u32 size = u32_log2(absCoeff) + 1;
            u32 mask = (1 << size) - 1;

            int index = runLength + size * 16;
            p = encoder.putBits(p, ac_code[index], ac_size[index]);
            p = encoder.putBits(p, coeff & mask, size);
        }

        return p;
    }

#if defined(JPEG_ENABLE_AVX512)

    // ----------------------------------------------------------------------------
    // encode_ac_avx512
    // ----------------------------------------------------------------------------

    /*
    // NOTE: parallel symbol size computation prototype
    // NOTE: try this with AVX512 - 128 bit wide is +- same performance as scalar (sparse input)

    inline __m128i getSymbolSize(__m128i absCoeff)
    {
        int16x8 value(absCoeff);
        int16x8 base(0);
        int16x8 temp;
        mask16x8 mask;

        temp = value & 0xff00;
        mask = temp != 0;
        base = select(mask, base | 8, base);
        value = select(mask, temp, value);

        temp = value & 0xf0f0;
        mask = temp != 0;
        base = select(mask, base | 4, base);
        value = select(mask, temp, value);

        temp = value & 0xcccc;
        mask = temp != 0;
        base = select(mask, base | 2, base);
        value = select(mask, temp, value);

        temp = value & 0xaaaa;
        mask = temp != 0;
        base = select(mask, base | 1, base);
        value = select(mask, temp, value);

        base += 1;

        return base;
    }

    inline __m128i absSymbolSize(__m128i* ptr_sz, __m128i coeff)
    {
        __m128i absCoeff = _mm_abs_epi16(coeff);
        __m128i mask = _mm_cmpeq_epi16(absCoeff, coeff);
        mask = _mm_xor_si128(mask, _mm_cmpeq_epi8(mask, mask)); // not
        coeff = _mm_add_epi16(coeff, mask);

        __m128 sz = getSymbolSize(absCoeff);
        _mm_storeu_si128(ptr_sz, sz);

        return coeff;
    }
    */

    u64 zigzag_avx512bw(const s16* in, s16* out)
    {
        static const u16 zigzag_shuffle [] =
        {
             0,  1,  8, 16,  9,  2,  3, 10,
            17, 24, 32, 25, 18, 11,  4,  5,
            12, 19, 26, 33, 40, 48, 41, 34,
            27, 20, 13,  6,  7, 14, 21, 28,
            35, 42, 49, 56, 57, 50, 43, 36,
            29, 22, 15, 23, 30, 37, 44, 51,
            58, 59, 52, 45, 38, 31, 39, 46,
            53, 60, 61, 54, 47, 55, 62, 63
        };

        const __m512i* src = reinterpret_cast<const __m512i *>(in);
        const __m512i* table = reinterpret_cast<const __m512i *>(zigzag_shuffle);

        const __m512i src0 = _mm512_loadu_si512(src + 0);
        const __m512i src1 = _mm512_loadu_si512(src + 1);
        const __m512i table0 = _mm512_loadu_si512(table + 0);
        const __m512i table1 = _mm512_loadu_si512(table + 1);
        const __m512i v0  = _mm512_permutex2var_epi16(src0, table0, src1);
        const __m512i v1  = _mm512_permutex2var_epi16(src0, table1, src1);

        // compute zeromask
        const __m512i zero = _mm512_setzero_si512();
        __mmask32 mask_lo = _mm512_cmpneq_epu16_mask(v0, zero);
        __mmask32 mask_hi = _mm512_cmpneq_epu16_mask(v1, zero);
        u64 zeromask = mask_lo;

        __m512i* dest = reinterpret_cast<__m512i *>(out);

        _mm512_storeu_si512(dest + 0, v0);

        if (!mask_hi)
        {
            zeromask |= (u64(mask_hi) << 32);
            _mm512_storeu_si512(dest + 1, v1);
        }

        return zeromask;
    }

    static
    u8* encode_ac_avx512bw(HuffmanEncoder& encoder, u8* p, const s16* input, const u16* ac_code, const u16* ac_size)
    {
        s16 temp[64];
        u64 zeromask = zigzag_avx512bw(input, temp);

        zeromask >>= 1; // skip DC

        for (int i = 1; i < 64; )
        {
            if (!zeromask)
            {
                // only zeros left
                p = encoder.putBits(p, ac_code[0], ac_size[0]);
                break;
            }

            int runLength = u64_tzcnt(zeromask); // BMI
            zeromask >>= (runLength + 1);
            i += runLength;

            while (runLength > 15)
            {
                runLength -= 16;
                p = encoder.putBits(p, ac_code[176], ac_size[176]);
            }

            int coeff = temp[i++];
            int absCoeff = std::abs(coeff);
            coeff -= (absCoeff != coeff);

            u32 size = u32_log2(absCoeff) + 1;
            u32 mask = (1 << size) - 1;

            int index = runLength + size * 16;
            p = encoder.putBits(p, ac_code[index], ac_size[index]);
            p = encoder.putBits(p, coeff & mask, size);
        }

        return p;
    }

#endif // defined(JPEG_ENABLE_AVX512)
#endif // defined(JPEG_ENABLE_AVX2)
#endif // defined(JPEG_ENABLE_SSE4)

#if defined(JPEG_ENABLE_NEON__todo)

    // ----------------------------------------------------------------------------
    // encode_ac_neon
    // ----------------------------------------------------------------------------

    static
    u8* encode_ac_neon(HuffmanEncoder& encoder, u8* p, const s16* input, const u16* ac_code, const u16* ac_size)
    {
        // TODO
    }

#endif // defined(JPEG_ENABLE_NEON)

    // ----------------------------------------------------------------------------
    // fdct_scalar
    // ----------------------------------------------------------------------------

    static
    void fdct_scalar(s16* dest, const s16* data, const s16* quant_table)
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

#if defined(JPEG_ENABLE_SSE2)

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

    static
    void fdct_sse2(s16* dest, const s16* data, const s16* quant_table)
    {
        constexpr s16 c1 = 1420; // cos 1PI/16 * root(2)
        constexpr s16 c2 = 1338; // cos 2PI/16 * root(2)
        constexpr s16 c3 = 1204; // cos 3PI/16 * root(2)
        constexpr s16 c5 = 805;  // cos 5PI/16 * root(2)
        constexpr s16 c6 = 554;  // cos 6PI/16 * root(2)
        constexpr s16 c7 = 283;  // cos 7PI/16 * root(2)

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
    }

#if defined(JPEG_ENABLE_AVX2)

    // ----------------------------------------------------------------------------
    // fdct_avx2
    // ----------------------------------------------------------------------------

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

    static
    void fdct_avx2(s16* dest, const s16* data, const s16* quant_table)
    {
        constexpr s16 c1 = 1420; // cos 1PI/16 * root(2)
        constexpr s16 c2 = 1338; // cos 2PI/16 * root(2)
        constexpr s16 c3 = 1204; // cos 3PI/16 * root(2)
        constexpr s16 c5 = 805;  // cos 5PI/16 * root(2)
        constexpr s16 c6 = 554;  // cos 6PI/16 * root(2)
        constexpr s16 c7 = 283;  // cos 7PI/16 * root(2)

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
    }

#endif // defined(JPEG_ENABLE_AVX2)
#endif // defined(JPEG_ENABLE_SSE2)

#if defined(JPEG_ENABLE_NEON)

    // ----------------------------------------------------------------------------
    // fdct_neon
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
    void fdct_neon(s16* dest, const s16* data, const s16* quant_table)
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

#endif // defined(JPEG_ENABLE_NEON)

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
    // jpegEncoder
    // ----------------------------------------------------------------------------

    jpegEncoder::jpegEncoder(SampleType sample, u32 width, u32 height, size_t stride, u32 quality)
        : ILqt(64)
        , ICqt(64)
    {
        MANGO_UNREFERENCED(stride);

        int bytes_per_pixel = 0;

        components = 0;

        channel[0].component = 1;
        channel[0].qtable = ILqt;
        channel[0].dc_code = g_luminance_dc_code_table;
        channel[0].dc_size = g_luminance_dc_size_table;
        channel[0].ac_code = g_luminance_ac_code_table;
        channel[0].ac_size = g_luminance_ac_size_table;

        channel[1].component = 2;
        channel[1].qtable = ICqt;
        channel[1].dc_code = g_chrominance_dc_code_table;
        channel[1].dc_size = g_chrominance_dc_size_table;
        channel[1].ac_code = g_chrominance_ac_code_table;
        channel[1].ac_size = g_chrominance_ac_size_table;

        channel[2].component = 3;
        channel[2].qtable = ICqt;
        channel[2].dc_code = g_chrominance_dc_code_table;
        channel[2].dc_size = g_chrominance_dc_size_table;
        channel[2].ac_code = g_chrominance_ac_code_table;
        channel[2].ac_size = g_chrominance_ac_size_table;

        read_8x8 = nullptr;

        u64 flags = getCPUFlags();
        MANGO_UNREFERENCED(flags);

        // select sampler

        const char* sampler_name = "Scalar";

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
                components = 1;
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
                components = 3;
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
                components = 3;
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
                components = 3;
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
                components = 3;
                break;
        }

        if (!read_8x8)
        {
            read_8x8 = read;
        }

        // select fdct

        fdct = fdct_scalar;
        const char* fdct_name = "Scalar";

#if defined(JPEG_ENABLE_SSE2)
        if (flags & INTEL_SSE2)
        {
            fdct = fdct_sse2;
            fdct_name = "SSE2";
        }
#endif

#if defined(JPEG_ENABLE_AVX2)
        if (flags & INTEL_AVX2)
        {
            fdct = fdct_avx2;
            fdct_name = "AVX2";
        }
#endif

#if defined(JPEG_ENABLE_NEON)
        {
            fdct = fdct_neon;
            fdct_name = "NEON";
        }
#endif

        // select block encoder

        encode_ac = encode_ac_scalar;
        const char* encode_name = "Scalar";

#if defined(JPEG_ENABLE_SSE4)
        if (flags & INTEL_SSSE3)
        {
            encode_ac = encode_ac_ssse3;
            encode_name = "SSSE3";
        }
#endif

#if defined(JPEG_ENABLE_AVX2)
        if (flags & INTEL_AVX2)
        {
            encode_ac = encode_ac_avx2;
            encode_name = "AVX2";
        }
#endif

#if defined(JPEG_ENABLE_AVX512)
        if (flags & INTEL_AVX512BW)
        {
            encode_ac = encode_ac_avx512bw;
            encode_name = "AVX512BW";
        }
#endif

#if defined(JPEG_ENABLE_NEON__todo)
        {
            encode_ac = encode_ac_neon;
            encode_name = "NEON";
        }
#endif

        // build encoder info string
        info = "[JPEG Encoder] FDCT: ";
        info += fdct_name;

        info += ", Sampler: ";
        info += sampler_name;

        info += ", Encoder: ";
        info += encode_name;

        mcu_width = 8;
        mcu_height = 8;

        horizontal_mcus = (width + mcu_width - 1) >> 3;
        vertical_mcus   = (height + mcu_height - 1) >> 3;

        rows_in_bottom_mcus = height - (vertical_mcus - 1) * mcu_height;
        cols_in_right_mcus  = width  - (horizontal_mcus - 1) * mcu_width;

        mcu_width_size = mcu_width * bytes_per_pixel;

        // initialize quantization tables

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

        for (int i = 0; i < 64; ++i)
        {
            u16 index = zigzag_table[i];

            // luminance
            u32 L = u32_clamp((g_luminance_quant_table[i] * quality + 0x200) >> 10, 2, 255);
            Lqt[index] = u8(L);
            ILqt[i] = u16(0x8000 / L);

            // chrominance
            u32 C = u32_clamp((g_chrominance_quant_table [i] * quality + 0x200) >> 10, 2, 255);
            Cqt[index] = u8(C);
            ICqt[i] = u16(0x8000 / C);
        }
    }

    jpegEncoder::~jpegEncoder()
    {
    }

    void jpegEncoder::writeMarkers(BigEndianStream& p, SampleType sample, u32 width, u32 height)
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
                p.write16(u16(size + magicICCLength + 4));
                p.write(magicICC, magicICCLength);
                p.write8(u8(i + 1)); // segment index, 1-based
                p.write8(u8(iccSegments));
                p.write(icc.slice(i * iccMaxSegmentSize), size);
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
    // encode_jpeg()
    // ----------------------------------------------------------------------------

    void encode_jpeg(ImageEncodeStatus& status, const Surface& surface, Stream& stream, int quality, SampleType sample, const ImageEncodeOptions& options)
    {
        jpegEncoder jp(sample, surface.width, surface.height, surface.stride, quality);
        jp.icc = options.icc;

        const u8* input = surface.image;
        size_t stride = surface.stride;

        // bitstream for each MCU scan
        std::vector<EncodeBuffer> buffers(jp.vertical_mcus);

        auto fdct = jp.fdct;
        auto encode_ac = jp.encode_ac;

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

            queue.enqueue([&jp, y, &buffers, input, stride, rows, read_func, fdct, encode_ac]
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

                int last_dc_value[3] = { 0, 0, 0 };

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
                    for (int i = 0; i < jp.components; ++i)
                    {
                        s16 temp[BLOCK_SIZE];
                        fdct(temp, block + i * BLOCK_SIZE, jp.channel[i].qtable);

                        const int component = jp.channel[i].component - 1;

                        const u16* dc_code = jp.channel[i].dc_code;
                        const u16* dc_size = jp.channel[i].dc_size;
                        const u16* ac_code = jp.channel[i].ac_code;
                        const u16* ac_size = jp.channel[i].ac_size;

                        int last_dc = last_dc_value[component];
                        last_dc_value[component] = temp[0];

                        ptr = encode_dc(huffman, ptr, temp, dc_code, dc_size, last_dc);
                        ptr = encode_ac(huffman, ptr, temp, ac_code, ac_size);
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
        jp.writeMarkers(s, sample, surface.width, surface.height);

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
            encode_jpeg(status, surface, stream, iq, sf.sample, options);
            status.direct = true;
        }
        else
        {
            // convert source surface to format supported in the encoder
            Bitmap temp(surface, sf.format);
            encode_jpeg(status, temp, stream, iq, sf.sample, options);
        }

        return status;
    }

} // namespace jpeg
} // namespace mango

#endif // MANGO_ENABLE_LICENSE_GPL
