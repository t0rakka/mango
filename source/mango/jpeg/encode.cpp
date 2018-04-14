/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
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

// TODO: Implement CPU architecture specific optimizations
// TODO: Implement ThreadPool assisted asynchronous encoding

#include <mango/core/pointer.hpp>
#include "jpeg.hpp"
#include <cstring>

#define BLOCK_SIZE  64

namespace
{

    using namespace mango;
    using namespace jpeg;

    enum
    {
        JPEG_FORMAT_YUV400,
        JPEG_FORMAT_RGB888,
        JPEG_FORMAT_ARGB8888,
        JPEG_FORMAT_ABGR8888,
    };

    struct xxx
    {
        Format source;
        int dest;
    } g_format_table[] =
    {
        { FORMAT_L8, JPEG_FORMAT_YUV400 },
        { FORMAT_B8G8R8, JPEG_FORMAT_RGB888 },
        { FORMAT_B8G8R8A8, JPEG_FORMAT_ARGB8888 },
        { FORMAT_R8G8B8A8, JPEG_FORMAT_ABGR8888 },
    };
    const int g_format_table_size = sizeof(g_format_table) / sizeof(g_format_table[0]);

    const uint16 luminance_dc_code_table [] =
    {
        0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006,
        0x000E, 0x001E, 0x003E, 0x007E, 0x00FE, 0x01FE
    };

    const uint16 luminance_dc_size_table [] =
    {
        0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
        0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009
    };

    const uint16 chrominance_dc_code_table [] =
    {
        0x0000, 0x0001, 0x0002, 0x0006, 0x000E, 0x001E,
        0x003E, 0x007E, 0x00FE, 0x01FE, 0x03FE, 0x07FE
    };

    const uint16 chrominance_dc_size_table [] =
    {
        0x0002, 0x0002, 0x0002, 0x0003, 0x0004, 0x0005,
        0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B
    };

    const uint16 luminance_ac_code_table [] =
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

    const uint16 luminance_ac_size_table [] =
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

    const uint16 chrominance_ac_code_table [] =
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

    const uint16 chrominance_ac_size_table [] =
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

    const uint8 bitsize [] =
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

    const uint8 markerdata [] =
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

    const uint8 zigzag_table [] =
    {
        0,  1,   5,  6, 14, 15, 27, 28,
        2,  4,   7, 13, 16, 26, 29, 42,
        3,  8,  12, 17, 25, 30, 41, 43,
        9,  11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54,
        20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61,
        35, 36, 48, 49, 57, 58, 62, 63
    };

    const uint8 luminance_quant_table [] =
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

    const uint8 chrominance_quant_table [] =
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
        int         component;
        uint16*     qtable;
    };

    struct jpeg_encode
    {
        int         mcu_width;
        int         mcu_height;
        int         horizontal_mcus;
        int         vertical_mcus;
        int         cols_in_right_mcus;
        int         rows_in_bottom_mcus;

        int         length_minus_mcu_width;
        int         length_minus_width;
        int         mcu_width_size;
        int         offset;

        uint8       Lqt [BLOCK_SIZE];
        uint8       Cqt [BLOCK_SIZE];
        uint16      ILqt [BLOCK_SIZE];
        uint16      ICqt [BLOCK_SIZE];

        // MCU configuration
        jpeg_chan   channel[3];
        int         channel_count;

        void (*read_format) (jpeg_encode* jp, BlockType *block, uint8* input, int rows, int cols, int incr);

        jpeg_encode(uint32 format, uint32 width, uint32 height, uint32 quality);
        ~jpeg_encode();

        void init_quantization_tables(uint32 quality);
        void write_markers(BigEndianStream& p, uint32 format, uint32 width, uint32 height);
    };

    struct HuffmanEncoder
    {
        int     ldc1;
        int     ldc2;
        int     ldc3;
        uint32  lcode;
        uint16  bitindex;

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

        uint8* putbits(uint8* output, uint32 data, int numbits)
        {
            int bits_in_next_word = bitindex + numbits - 32;
            if (bits_in_next_word < 0)
            {
                lcode = (lcode << numbits) | data;
                bitindex += static_cast<uint16>(numbits);
            }
            else
            {
                lcode = (lcode << (32 - bitindex)) | (data >> bits_in_next_word);

                // NOTE: we trade some arithmetic here to avoid
                // branching in the statistically more common case.
                if (u32_has_zero_byte(~lcode))
                {
                    // lcode contains at least one 0xff byte, which each require a stuff byte (0x00) to follow.
                    if ((*output++ = static_cast<uint8>(lcode >> 24)) == 0xff) *output++ = 0;
                    if ((*output++ = static_cast<uint8>(lcode >> 16)) == 0xff) *output++ = 0;
                    if ((*output++ = static_cast<uint8>(lcode >>  8)) == 0xff) *output++ = 0;
                    if ((*output++ = static_cast<uint8>(lcode >>  0)) == 0xff) *output++ = 0;
                }
                else
                {
                    ustore32be(output, lcode);
                    output += 4;
                }

                lcode = data;
                bitindex = static_cast<uint16>(bits_in_next_word);
            }

            return output;
        }

        u8* flush(u8* p)
        {
            if (bitindex > 0)
            {
                lcode <<= (32 - bitindex);

                uint16 count = (bitindex + 7) >> 3;
                uint8* ptr = reinterpret_cast<uint8*>(&lcode) + 3;

                for (int i = 0; i < count; ++i)
                {
                    uint8 v = *ptr--;
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

        u8* encode(u8* p, int component, BlockType* temp)
        {
            const uint16* DcCodeTable;
            const uint16* DcSizeTable;
            const uint16* AcCodeTable;
            const uint16* AcSizeTable;

            int Coeff, LastDc;
            uint16 AbsCoeff, HuffCode, HuffSize, RunLength = 0, DataSize = 0, index;

            Coeff = *temp++;

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
            AbsCoeff = static_cast<uint16>((Coeff < 0) ? -Coeff-- : Coeff);

            while (AbsCoeff != 0)
            {
                AbsCoeff >>= 1;
                DataSize++;
            }

            HuffCode = DcCodeTable [DataSize];
            HuffSize = DcSizeTable [DataSize];

            Coeff &= (1 << DataSize) - 1;

            uint32 data = (HuffCode << DataSize) | Coeff;
            int numbits = HuffSize + DataSize;
            p = putbits(p, data, numbits);

            for (int i = 0; i < 63; ++i)
            {
                if ((Coeff = *temp++) != 0)
                {
                    while (RunLength > 15)
                    {
                        RunLength -= 16;

                        data = AcCodeTable [161];
                        numbits = AcSizeTable [161];
                        p = putbits(p, data, numbits);
                    }

                    AbsCoeff = static_cast<uint16>((Coeff < 0) ? -Coeff-- : Coeff);
                    if (AbsCoeff >> 8 == 0)
                        DataSize = bitsize [AbsCoeff];
                    else
                        DataSize = bitsize [AbsCoeff >> 8] + 8;

                    index = RunLength * 10 + DataSize;
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

    void DCT(BlockType* dest, BlockType* data, const uint16* quant_table)
    {
        const uint16 c1 = 1420;  // cos  PI/16 * root(2)
        const uint16 c2 = 1338;  // cos  PI/8  * root(2)
        const uint16 c3 = 1204;  // cos 3PI/16 * root(2)
        const uint16 c5 = 805;   // cos 5PI/16 * root(2)
        const uint16 c6 = 554;   // cos 3PI/8  * root(2)
        const uint16 c7 = 283;   // cos 7PI/16 * root(2)

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
            data[0] = BlockType(x4 + x5);
            data[4] = BlockType(x4 - x5);
            data[2] = BlockType((x8*c2 + x7*c6) >> 10);
            data[6] = BlockType((x8*c6 - x7*c2) >> 10);
            data[7] = BlockType((x0*c7 - x1*c5 + x2*c3 - x3*c1) >> 10);
            data[5] = BlockType((x0*c5 - x1*c1 + x2*c7 + x3*c3) >> 10);
            data[3] = BlockType((x0*c3 - x1*c7 - x2*c1 - x3*c5) >> 10);
            data[1] = BlockType((x0*c1 + x1*c3 + x2*c5 + x3*c7) >> 10);
            data += 8;
        }

        data -= 64;

        for (int i = 0; i < 8; ++i)
        {
            int x8 = data [i +  0] + data [i + 56];
            int x0 = data [i +  0] - data [i + 56];
            int x7 = data [i +  8] + data [i + 48];
            int x1 = data [i +  8] - data [i + 48];
            int x6 = data [i + 16] + data [i + 40];
            int x2 = data [i + 16] - data [i + 40];
            int x5 = data [i + 24] + data [i + 32];
            int x3 = data [i + 24] - data [i + 32];
            int x4 = x8 + x5;
            x8 = x8 - x5;
            x5 = x7 + x6;
            x7 = x7 - x6;
            auto v0 = BlockType((x4 + x5) >> 3);
            auto v4 = BlockType((x4 - x5) >> 3);
            auto v2 = BlockType((x8*c2 + x7*c6) >> 13);
            auto v6 = BlockType((x8*c6 - x7*c2) >> 13);
            auto v7 = BlockType((x0*c7 - x1*c5 + x2*c3 - x3*c1) >> 13);
            auto v5 = BlockType((x0*c5 - x1*c1 + x2*c7 + x3*c3) >> 13);
            auto v3 = BlockType((x0*c3 - x1*c7 - x2*c1 - x3*c5) >> 13);
            auto v1 = BlockType((x0*c1 + x1*c3 + x2*c5 + x3*c7) >> 13);
            dest[zigzag_table[i + 0 * 8]] = BlockType((v0 * quant_table[i + 0 * 8] + 0x4000) >> 15);
            dest[zigzag_table[i + 4 * 8]] = BlockType((v4 * quant_table[i + 4 * 8] + 0x4000) >> 15);
            dest[zigzag_table[i + 2 * 8]] = BlockType((v2 * quant_table[i + 2 * 8] + 0x4000) >> 15);
            dest[zigzag_table[i + 6 * 8]] = BlockType((v6 * quant_table[i + 6 * 8] + 0x4000) >> 15);
            dest[zigzag_table[i + 7 * 8]] = BlockType((v7 * quant_table[i + 7 * 8] + 0x4000) >> 15);
            dest[zigzag_table[i + 5 * 8]] = BlockType((v5 * quant_table[i + 5 * 8] + 0x4000) >> 15);
            dest[zigzag_table[i + 3 * 8]] = BlockType((v3 * quant_table[i + 3 * 8] + 0x4000) >> 15);
            dest[zigzag_table[i + 1 * 8]] = BlockType((v1 * quant_table[i + 1 * 8] + 0x4000) >> 15);
        }
    }

    // ----------------------------------------------------------------------------
    // read_xxx_format
    // ----------------------------------------------------------------------------

    void read_400_format(jpeg_encode* jp, BlockType* block, uint8* input, int rows, int cols, int incr)
    {
        BlockType* Y1 = block;

        for (int i = 0; i < rows; ++i)
        {
            for (int j = cols; j > 0; --j)
            {
                *Y1++ = (*input++) - 128;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                *Y1 = *(Y1 - 1);
                ++Y1;
            }

            input += incr;
        }

        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                *Y1 = *(Y1 - 8);
                ++Y1;
            }
        }
    }

    void read_rgb888_format(jpeg_encode* jp, BlockType* block, uint8* input, int rows, int cols, int incr)
    {
        BlockType* Y  = block + 0 * BLOCK_SIZE;
        BlockType* CB = block + 1 * BLOCK_SIZE;
        BlockType* CR = block + 2 * BLOCK_SIZE;

        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                int y = (76*input[2] + 151*input[1] + 29*input[0]) >> 8;
                int cr = ((input[2] + - y) * 182) >> 8;
                int cb = ((input[0] + - y) * 144) >> 8;

                *Y++ = static_cast<BlockType>(y - 128);
                *CB++ = static_cast<BlockType>(cb);
                *CR++ = static_cast<BlockType>(cr);

                input += 3;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                *Y = *(Y - 1);
                ++Y;
            }

            for (int j = 8 - cols; j > 0; --j)
            {
                *CB = *(CB - 1); ++CB;
                *CR = *(CR - 1); ++CR;
            }

            input += incr;
        }

        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                *Y = *(Y - 8); ++Y;
                *CB = *(CB - 8); ++CB;
                *CR = *(CR - 8); ++CR;
            }
        }
    }

    void read_argb8888_format(jpeg_encode* jp, BlockType* block, uint8* input, int rows, int cols, int incr)
    {
        BlockType* Y  = block + 0 * BLOCK_SIZE;
        BlockType* CB = block + 1 * BLOCK_SIZE;
        BlockType* CR = block + 2 * BLOCK_SIZE;

        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                int y = (76*input[2] + 151*input[1] + 29*input[0]) >> 8;
                int cr = ((input[2] + - y) * 182) >> 8;
                int cb = ((input[0] + - y) * 144) >> 8;

                *Y++ = static_cast<BlockType>(y - 128);
                *CB++ = static_cast<BlockType>(cb);
                *CR++ = static_cast<BlockType>(cr);

                input += 4;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                *Y = *(Y - 1);
                ++Y;
            }

            for (int j = 8 - cols; j > 0; --j)
            {
                *CB = *(CB - 1); ++CB;
                *CR = *(CR - 1); ++CR;
            }

            input += incr;
        }

        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                *Y = *(Y - 8); ++Y;
                *CB = *(CB - 8); ++CB;
                *CR = *(CR - 8); ++CR;
            }
        }
    }

    void read_abgr8888_format(jpeg_encode* jp, BlockType* block, uint8* input, int rows, int cols, int incr)
    {
        BlockType* Y  = block + 0 * BLOCK_SIZE;
        BlockType* CB = block + 1 * BLOCK_SIZE;
        BlockType* CR = block + 2 * BLOCK_SIZE;

        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                int y = (76*input[0] + 151*input[1] + 29*input[2]) >> 8;
                int cr = ((input[0] + - y) * 182) >> 8;
                int cb = ((input[2] + - y) * 144) >> 8;

                *Y++ = static_cast<BlockType>(y - 128);
                *CB++ = static_cast<BlockType>(cb);
                *CR++ = static_cast<BlockType>(cr);

                input += 4;
            }

            // replicate last column
            for (int j = 8 - cols; j > 0; --j)
            {
                *Y = *(Y - 1);
                ++Y;
            }

            for (int j = 8 - cols; j > 0; --j)
            {
                *CB = *(CB - 1); ++CB;
                *CR = *(CR - 1); ++CR;
            }

            input += incr;
        }

        // replicate last row
        for (int i = 8 - rows; i > 0; --i)
        {
            for (int j = 8; j > 0; --j)
            {
                *Y = *(Y - 8); ++Y;
                *CB = *(CB - 8); ++CB;
                *CR = *(CR - 8); ++CR;
            }
        }
    }

    // ----------------------------------------------------------------------------
    // jpeg_encode methods
    // ----------------------------------------------------------------------------

    jpeg_encode::jpeg_encode(uint32 format, uint32 width, uint32 height, uint32 quality)
    {
        int bytes_per_pixel = 0;

        channel_count = 0;

        channel[0].component = 1;
        channel[0].qtable = ILqt;

        channel[1].component = 2;
        channel[1].qtable = ICqt;

        channel[2].component = 3;
        channel[2].qtable = ICqt;

        switch (format)
        {
            case JPEG_FORMAT_YUV400:
                read_format = read_400_format;
                bytes_per_pixel = 1;
                channel_count = 1;
                break;

            case JPEG_FORMAT_RGB888:
                read_format = read_rgb888_format;
                bytes_per_pixel = 3;
                channel_count = 3;
                break;

            case JPEG_FORMAT_ARGB8888:
                read_format = read_argb8888_format;
                bytes_per_pixel = 4;
                channel_count = 3;
                break;

            case JPEG_FORMAT_ABGR8888:
                read_format = read_abgr8888_format;
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

        length_minus_mcu_width = (width - mcu_width         ) * bytes_per_pixel;
        length_minus_width     = (width - cols_in_right_mcus) * bytes_per_pixel;

        mcu_width_size = mcu_width * bytes_per_pixel;

        offset = (width * (mcu_height - 1) - (mcu_width - cols_in_right_mcus)) * bytes_per_pixel;

        init_quantization_tables(quality);
    }

    jpeg_encode::~jpeg_encode()
    {
    }

    void jpeg_encode::init_quantization_tables(uint32 quality)
    {
        if (quality > 1024)
            quality = 1024;
        quality *= 16;

        for (int i = 0; i < 64; ++i)
        {
            uint16 index = zigzag_table [i];
            uint32 value;

            // luminance quantization table * quality factor
            value = luminance_quant_table [i] * quality;
            value = (value + 0x200) >> 10;

            if (value < 2)
                value = 2;
            else if (value > 255)
                value = 255;

            Lqt [index] = (uint8) value;
            ILqt [i] = static_cast<uint16>(0x8000 / value);

            // chrominance quantization table * quality factor
            value = chrominance_quant_table [i] * quality;
            value = (value + 0x200) >> 10;

            if (value < 2)
                value = 2;
            else if (value > 255)
                value = 255;

            Cqt [index] = (uint8) value;
            ICqt [i] = static_cast<uint16>(0x8000 / value);
        }
    }

    void jpeg_encode::write_markers(BigEndianStream& p, uint32 format, uint32 width, uint32 height)
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

        uint8 number_of_components = 0;

        switch (format)
        {
            case JPEG_FORMAT_YUV400:
                number_of_components = 1;
                break;

            case JPEG_FORMAT_RGB888:
            case JPEG_FORMAT_ARGB8888:
            case JPEG_FORMAT_ABGR8888:
                number_of_components = 3;
                break;
        }

        uint16 header_length = 8 + 3 * number_of_components;

        p.write16(header_length); // frame header length
        p.write8(8); // precision
        p.write16(static_cast<uint16>(height)); // image height
        p.write16(static_cast<uint16>(width)); // image width
        p.write8(number_of_components); // Nf

        const uint8 nfdata[] =
        {
            0x01, 0x11, 0x00, // component 1
            0x00, 0x00, 0x00, // padding
            0x01, 0x11, 0x00, // component 1
            0x02, 0x11, 0x01, // component 2
            0x03, 0x11, 0x01, // component 3
        };

        p.write(nfdata + (number_of_components - 1) * 3, number_of_components * 3);

        // huffman table(DHT)
        p.write(markerdata, sizeof(markerdata));

        // Define Restart Interval marker
        p.write16(0xffdd);
        p.write16(4);
        p.write16(horizontal_mcus);

        // Start of scan marker
        p.write16(0xffda);
        p.write16(6 + number_of_components * 2); // header length
        p.write8(number_of_components); // Ns

        const uint8 nsdata[] =
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

    void encodeJPEG(const Surface& surface, Stream& stream, int quality, uint32 image_format)
    {
        u8* input = surface.image;

        jpeg_encode jp(image_format, surface.width, surface.height, quality);

        BigEndianStream s(stream);

        // writing marker data
        jp.write_markers(s, image_format, surface.width, surface.height);

        ConcurrentQueue queue;

        int rows;

        // bitstream for each MCU scan
        Buffer* buffers = new Buffer[jp.vertical_mcus];

        // encode MCUs
        for (int y = 0; y < jp.vertical_mcus; ++y)
        {
            if (y < jp.vertical_mcus - 1)
            {
                rows = jp.mcu_height;
            }
            else
            {
                // clipping
                rows = jp.rows_in_bottom_mcus;
            }

            queue.enqueue([&jp, y, buffers, input, rows] {
                u8* image = input;
                int cols;
                int incr;

                HuffmanEncoder huffman;

                u8 huff_temp[1024];
                u8* ptr = huff_temp;

                for (int x = 0; x < jp.horizontal_mcus; ++x)
                {
                    if (x < jp.horizontal_mcus - 1)
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

                    BlockType block[BLOCK_SIZE * 3];

                    // read MCU data
                    jp.read_format(&jp, block, image, rows, cols, incr);

                    // encode the data in MCU
                    for (int i = 0; i < jp.channel_count; ++i)
                    {
                        BlockType temp[BLOCK_SIZE];
                        DCT(temp, block + i * BLOCK_SIZE, jp.channel[i].qtable);
                        ptr = huffman.encode(ptr, jp.channel[i].component, temp);
                    }

                    if (ptr - huff_temp > 512)
                    {
                        buffers[y].write(huff_temp, ptr - huff_temp);
                        ptr = huff_temp;
                    }

                    image += jp.mcu_width_size;
                }

                ptr = huffman.flush(ptr);
                buffers[y].write(huff_temp, ptr - huff_temp);
            });

            input += jp.horizontal_mcus * jp.mcu_width_size;
            input += jp.offset;
        }

        queue.wait();

        for (int y = 0; y < jp.vertical_mcus; ++y)
        {
            Buffer& buffer = buffers[y];

            // write huffman bitstream
            s.write(buffer, buffer.size());

            // write restart marker
            int index = y & 7;
            s.write16(0xffd0 + index);
        }

        delete[] buffers;

        // EOI marker
        s.write16(0xffd9);
    }

} // namespace

namespace jpeg
{

    void EncodeImage(Stream& stream, const Surface& surface, float quality)
    {
        // configure quality
        quality = clamp(1.0f - quality, 0.0f, 1.0f);
        const uint32 iq = uint32(quality * 1024);

        // set default format
        Format sourceFormat = FORMAT_R8G8B8A8;
        int destFormat = JPEG_FORMAT_ABGR8888;

        // search for a better match
        for (int i = 0; i < g_format_table_size; ++i)
        {
            if (surface.format == g_format_table[i].source)
            {
                sourceFormat = g_format_table[i].source;
                destFormat = g_format_table[i].dest;
            }
        }

        // encode
        if (surface.format == sourceFormat)
        {
            encodeJPEG(surface, stream, iq, destFormat);
        }
        else
        {
            // convert source surface to format supported in the encoder
            Bitmap temp(surface.width, surface.height, sourceFormat);
            temp.blit(0, 0, surface);
            encodeJPEG(temp, stream, iq, destFormat);
        }
    }

} // namespace jpeg
