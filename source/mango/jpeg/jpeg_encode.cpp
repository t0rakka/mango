/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include "jpeg.hpp"
#include <cstring>

namespace
{
    using namespace mango;
    using namespace mango::math;
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

    const u32 g_luminance_dc_code_table [] =
    {
        0x00000000, 0x00000004, 0x0000000c, 0x00000020, 0x00000050, 0x000000c0, 0x00000380, 0x00000f00, 0x00003e00, 0x0000fc00, 0x0003f800, 0x000ff000,
    };

    const u16 g_luminance_dc_size_table [] =
    {
        0x0002, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x000a, 0x000c, 0x000e, 0x0010, 0x0012, 0x0014,
    };

    // Table K.4 – Table for chrominance DC coefficient differences

    const u32 g_chrominance_dc_code_table [] =
    {
        0x00000000, 0x00000002, 0x00000008, 0x00000030, 0x000000e0, 0x000003c0, 0x00000f80, 0x00003f00, 0x0000fe00, 0x0003fc00, 0x000ff800, 0x003ff000,
    };

    const u16 g_chrominance_dc_size_table [] =
    {
        0x0002, 0x0003, 0x0004, 0x0006, 0x0008, 0x000a, 0x000c, 0x000e, 0x0010, 0x0012, 0x0014, 0x0016,
    };

    // Table K.5 – Table for luminance AC coefficients

    alignas(64)
    const u32 g_luminance_ac_code_table [] =
    {
        0x0000000a, 0x000007f9, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000018, 0x00000038, 0x00000074, 0x00000076, 0x000000f4, 0x000000f6, 0x000001f4, 0x000003f0, 0x000003f2, 0x000003f4, 0x000007f2, 0x000007f4, 0x00000ff0, 0x0001ffd6, 0x0001ffea,
        0x00000004, 0x0000006c, 0x000003e4, 0x000007dc, 0x00000fe0, 0x00001fdc, 0x00003fd8, 0x00003fdc, 0x0001ff00, 0x0003fef8, 0x0003ff1c, 0x0003ff40, 0x0003ff64, 0x0003ff88, 0x0003ffb0, 0x0003ffd8,
        0x00000020, 0x000003c8, 0x00001fb8, 0x00007fa8, 0x0007fcb0, 0x0007fcf0, 0x0007fd30, 0x0007fd70, 0x0007fdb0, 0x0007fdf8, 0x0007fe40, 0x0007fe88, 0x0007fed0, 0x0007ff18, 0x0007ff68, 0x0007ffb8,
        0x000000b0, 0x00001f60, 0x0000ff40, 0x000ff8f0, 0x000ff970, 0x000ff9f0, 0x000ffa70, 0x000ffaf0, 0x000ffb70, 0x000ffc00, 0x000ffc90, 0x000ffd20, 0x000ffdb0, 0x000ffe40, 0x000ffee0, 0x000fff80,
        0x00000340, 0x0000fec0, 0x001ff120, 0x001ff200, 0x001ff300, 0x001ff400, 0x001ff500, 0x001ff600, 0x001ff700, 0x001ff820, 0x001ff940, 0x001ffa60, 0x001ffb80, 0x001ffca0, 0x001ffde0, 0x001fff20,
        0x00001e00, 0x003fe100, 0x003fe280, 0x003fe440, 0x003fe640, 0x003fe840, 0x003fea40, 0x003fec40, 0x003fee40, 0x003ff080, 0x003ff2c0, 0x003ff500, 0x003ff740, 0x003ff980, 0x003ffc00, 0x003ffe80,
        0x00007c00, 0x007fc280, 0x007fc580, 0x007fc900, 0x007fcd00, 0x007fd100, 0x007fd500, 0x007fd900, 0x007fdd00, 0x007fe180, 0x007fe600, 0x007fea80, 0x007fef00, 0x007ff380, 0x007ff880, 0x007ffd80,
        0x0003f600, 0x00ff8600, 0x00ff8c00, 0x00ff9300, 0x00ff9b00, 0x00ffa300, 0x00ffab00, 0x00ffb300, 0x00ffbb00, 0x00ffc400, 0x00ffcd00, 0x00ffd600, 0x00ffdf00, 0x00ffe800, 0x00fff200, 0x00fffc00,
        0x01ff0400, 0x01ff0e00, 0x01ff1a00, 0x01ff2800, 0x01ff3800, 0x01ff4800, 0x01ff5800, 0x01ff6800, 0x01ff7800, 0x01ff8a00, 0x01ff9c00, 0x01ffae00, 0x01ffc000, 0x01ffd200, 0x01ffe600, 0x01fffa00,
        0x03fe0c00, 0x03fe2000, 0x03fe3800, 0x03fe5400, 0x03fe7400, 0x03fe9400, 0x03feb400, 0x03fed400, 0x03fef400, 0x03ff1800, 0x03ff3c00, 0x03ff6000, 0x03ff8400, 0x03ffa800, 0x03ffd000, 0x03fff800,
    };

    alignas(64)
    const u16 g_luminance_ac_size_table [] =
    {
        0x0004, 0x000b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0003, 0x0005, 0x0006, 0x0007, 0x0007, 0x0008, 0x0008, 0x0009, 0x000a, 0x000a, 0x000a, 0x000b, 0x000b, 0x000c, 0x0011, 0x0011,
        0x0004, 0x0007, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000e, 0x0011, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012,
        0x0006, 0x000a, 0x000d, 0x000f, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013,
        0x0008, 0x000d, 0x0010, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014,
        0x000a, 0x0010, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015,
        0x000d, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016,
        0x000f, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017,
        0x0012, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018,
        0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019,
        0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a,
    };

    // Table K.6 – Table for chrominance AC coefficients

    alignas(64)
    const u32 g_chrominance_ac_code_table [] =
    {
        0x00000000, 0x000003fa, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000002, 0x00000016, 0x00000034, 0x00000036, 0x00000074, 0x00000076, 0x000000f2, 0x000000f4, 0x000001f2, 0x000003ee, 0x000003f0, 0x000003f2, 0x000003f4, 0x00000ff2, 0x00007fc0, 0x0000ff86,
        0x00000010, 0x000000e4, 0x000003dc, 0x000003e0, 0x000007d8, 0x00000fe4, 0x00001fdc, 0x00001fe0, 0x0003fedc, 0x0003ff00, 0x0003ff24, 0x0003ff48, 0x0003ff6c, 0x0003ff90, 0x0003ffb4, 0x0003ffd8,
        0x00000050, 0x000007b0, 0x00001fb8, 0x00001fc0, 0x0007fcb8, 0x0007fcf8, 0x0007fd38, 0x0007fd78, 0x0007fdc0, 0x0007fe08, 0x0007fe50, 0x0007fe98, 0x0007fee0, 0x0007ff28, 0x0007ff70, 0x0007ffb8,
        0x00000180, 0x00001f50, 0x0000ff60, 0x0000ff70, 0x000ff980, 0x000ffa00, 0x000ffa80, 0x000ffb00, 0x000ffb90, 0x000ffc20, 0x000ffcb0, 0x000ffd40, 0x000ffdd0, 0x000ffe60, 0x000ffef0, 0x000fff80,
        0x00000320, 0x0000fec0, 0x000ff840, 0x001ff220, 0x001ff320, 0x001ff420, 0x001ff520, 0x001ff620, 0x001ff740, 0x001ff860, 0x001ff980, 0x001ffaa0, 0x001ffbc0, 0x001ffce0, 0x001ffe00, 0x001fff20,
        0x00000e00, 0x0003fd40, 0x003fe300, 0x003fe480, 0x003fe680, 0x003fe880, 0x003fea80, 0x003fec80, 0x003feec0, 0x003ff100, 0x003ff340, 0x003ff580, 0x003ff7c0, 0x003ffa00, 0x003ffc40, 0x003ffe80,
        0x00003c00, 0x007fc400, 0x007fc680, 0x007fc980, 0x007fcd80, 0x007fd180, 0x007fd580, 0x007fd980, 0x007fde00, 0x007fe280, 0x007fe700, 0x007feb80, 0x007ff000, 0x007ff480, 0x007ff900, 0x007ffd80,
        0x0001f400, 0x00ff8900, 0x00ff8e00, 0x00ff9400, 0x00ff9c00, 0x00ffa400, 0x00ffac00, 0x00ffb400, 0x00ffbd00, 0x00ffc600, 0x00ffcf00, 0x00ffd800, 0x00ffe100, 0x00ffea00, 0x00fff300, 0x00fffc00,
        0x0007ec00, 0x01ff1400, 0x01ff1e00, 0x01ff2a00, 0x01ff3a00, 0x01ff4a00, 0x01ff5a00, 0x01ff6a00, 0x01ff7c00, 0x01ff8e00, 0x01ffa000, 0x01ffb200, 0x01ffc400, 0x01ffd600, 0x01ffe800, 0x01fffa00,
        0x003fd000, 0x03fe2c00, 0x03fe4000, 0x03fe5800, 0x03fe7800, 0x03fe9800, 0x03feb800, 0x03fed800, 0x03fefc00, 0x03ff2000, 0x03ff4400, 0x03ff6800, 0x03ff8c00, 0x03ffb000, 0x03ffd400, 0x03fff800,
    };

    alignas(64)
    const u16 g_chrominance_ac_size_table [] =
    {
        0x0002, 0x000a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0003, 0x0005, 0x0006, 0x0006, 0x0007, 0x0007, 0x0008, 0x0008, 0x0009, 0x000a, 0x000a, 0x000a, 0x000a, 0x000c, 0x000f, 0x0010,
        0x0005, 0x0008, 0x000a, 0x000a, 0x000b, 0x000c, 0x000d, 0x000d, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012,
        0x0007, 0x000b, 0x000d, 0x000d, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013,
        0x0009, 0x000d, 0x0010, 0x0010, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014,
        0x000a, 0x0010, 0x0014, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015,
        0x000c, 0x0012, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016,
        0x000e, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017,
        0x0011, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018,
        0x0013, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019,
        0x0016, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a,
    };

    const u8 g_marker_data [] =
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

    // ----------------------------------------------------------------------------
    // HuffmanEncoder
    // ----------------------------------------------------------------------------

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

#ifdef MANGO_CPU_64BIT

    static inline
    u8* flushStuffedBytes(u8* output, DataType code)
    {
        if (code & 0x8080808080808080ull & ~(code + 0x0101010101010101ull))
        {
            output = writeStuffedBytes(output, code, 8);
        }
        else
        {
            ustore64be(output, code);
            output += 8;
        }
        return output;
    }

#else

    static inline
    u8* flushStuffedBytes(u8* output, DataType code)
    {
        if (code & 0x80808080 & ~(code + 0x01010101))
        {
            output = writeStuffedBytes(output, code, 4);
        }
        else
        {
            ustore32be(output, code);
            output += 4;
        }
        return output;
    }

#endif

    struct EncodeBuffer : Buffer
    {
        std::atomic<bool> ready { false };
    };

    struct HuffmanEncoder
    {
        DataType code;
        int space;

        int last_dc_value[3] = { 0, 0, 0 };
        void (*fdct)(s16* dest, const s16* data, const s16* qtable);

        HuffmanEncoder()
        {
            code = 0;
            space = JPEG_REGISTER_BITS;
        }

        ~HuffmanEncoder()
        {
        }

        u8* putBits(u8* output, DataType data, int numbits)
        {
            space -= numbits;
            if (space < 0)
            {
                output = flushStuffedBytes(output, code | (data >> -space));
                space += JPEG_REGISTER_BITS;
                code = data << space;
            }
            else
            {
                code |= (data << space);
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
        Surface m_surface;
        SampleType m_sample;
        const ImageEncodeOptions& m_options;

        int mcu_width;
        int mcu_height;
        int mcu_stride;
        int horizontal_mcus;
        int vertical_mcus;
        int cols_in_right_mcus;
        int rows_in_bottom_mcus;

        u8 luminance_qtable[BLOCK_SIZE];
        u8 chrominance_qtable[BLOCK_SIZE];
        AlignedStorage<s16> inverse_luminance_qtable;
        AlignedStorage<s16> inverse_chrominance_qtable;

        // MCU configuration
        struct Channel
        {
            int component;
            const s16* qtable;
            const u32* dc_code;
            const u16* dc_size;
            const u32* ac_code;
            const u16* ac_size;
        };
        Channel channel[3];
        int components;

        std::string info;

        using ReadFunc = void (*)(s16*, const u8*, size_t, int, int);

        void (*read_8x8) (s16* block, const u8* input, size_t stride, int rows, int cols);
        void (*read)     (s16* block, const u8* input, size_t stride, int rows, int cols);
        void (*fdct)     (s16* dest, const s16* data, const s16* qtable);
        u8*  (*encode)   (HuffmanEncoder& encoder, u8* p, const s16* input, const Channel& channel);

        jpegEncoder(const Surface& surface, SampleType sample, const ImageEncodeOptions& options);
        ~jpegEncoder();

        void encodeInterval(EncodeBuffer& buffer, const u8* src, size_t stride, ReadFunc read_func, int rows);
        void writeMarkers(BigEndianStream& p);
        ImageEncodeStatus encodeImage(Stream& stream);
    };

    // ----------------------------------------------------------------------------
    // fdct_scalar
    // ----------------------------------------------------------------------------

    static
    void fdct_scalar(s16* dest, const s16* data, const s16* qtable)
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
            dest[i + 8 * 0] = (v0 * qtable[i + 8 * 0] + 0x4000) >> 15;
            dest[i + 8 * 1] = (v1 * qtable[i + 8 * 1] + 0x4000) >> 15;
            dest[i + 8 * 2] = (v2 * qtable[i + 8 * 2] + 0x4000) >> 15;
            dest[i + 8 * 3] = (v3 * qtable[i + 8 * 3] + 0x4000) >> 15;
            dest[i + 8 * 4] = (v4 * qtable[i + 8 * 4] + 0x4000) >> 15;
            dest[i + 8 * 5] = (v5 * qtable[i + 8 * 5] + 0x4000) >> 15;
            dest[i + 8 * 6] = (v6 * qtable[i + 8 * 6] + 0x4000) >> 15;
            dest[i + 8 * 7] = (v7 * qtable[i + 8 * 7] + 0x4000) >> 15;
        }
    }

#if defined(MANGO_ENABLE_SSE2)

    // ----------------------------------------------------------------------------
    // fdct sse2
    // ----------------------------------------------------------------------------

    static inline void interleave16(__m128i& a, __m128i& b)
    {
        __m128i c = a;
        a = _mm_unpacklo_epi16(a, b);
        b = _mm_unpackhi_epi16(c, b);
    }

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

    #define JPEG_CONST16_SSE2(x, y) \
        _mm_setr_epi16(x, y, x, y, x, y, x, y)

    #define JPEG_TRANSFORM_SSE2(n) { \
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
    void fdct_sse2(s16* dest, const s16* data, const s16* qtable)
    {
        constexpr s16 c1 = 1420; // cos 1PI/16 * root(2)
        constexpr s16 c2 = 1338; // cos 2PI/16 * root(2)
        constexpr s16 c3 = 1204; // cos 3PI/16 * root(2)
        constexpr s16 c5 = 805;  // cos 5PI/16 * root(2)
        constexpr s16 c6 = 554;  // cos 6PI/16 * root(2)
        constexpr s16 c7 = 283;  // cos 7PI/16 * root(2)

        const __m128i c26p = JPEG_CONST16_SSE2(c2, c6);
        const __m128i c62n = JPEG_CONST16_SSE2(c6,-c2);
        const __m128i c75n = JPEG_CONST16_SSE2(c7,-c5);
        const __m128i c31n = JPEG_CONST16_SSE2(c3,-c1);
        const __m128i c51n = JPEG_CONST16_SSE2(c5,-c1);
        const __m128i c73p = JPEG_CONST16_SSE2(c7, c3);
        const __m128i c37n = JPEG_CONST16_SSE2(c3,-c7);
        const __m128i c15p = JPEG_CONST16_SSE2(c1, c5);
        const __m128i c13p = JPEG_CONST16_SSE2(c1, c3);
        const __m128i c57p = JPEG_CONST16_SSE2(c5, c7);

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

        __m128i x4;
        x4 = _mm_add_epi16(x8, x5);
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

        JPEG_TRANSFORM_SSE2(10);

        // pass 2

        JPEG_TRANSPOSE16();

        x8 = _mm_add_epi16(v0, v7);
        x7 = _mm_add_epi16(v1, v6);
        x6 = _mm_add_epi16(v2, v5);
        x5 = _mm_add_epi16(v3, v4);
        x0 = _mm_sub_epi16(v0, v7);
        x1 = _mm_sub_epi16(v1, v6);
        x2 = _mm_sub_epi16(v2, v5);
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

        JPEG_TRANSFORM_SSE2(13);

        // quantize

        const __m128i one = _mm_set1_epi16(1);
        const __m128i bias = _mm_set1_epi16(0x4000);
        const __m128i* q = reinterpret_cast<const __m128i*>(qtable);

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

#endif // defined(MANGO_ENABLE_SSE2)

#if defined(MANGO_ENABLE_AVX2__disabled)

    // ----------------------------------------------------------------------------
    // fdct_avx2
    // ----------------------------------------------------------------------------

    static inline
    void transpose_8x8_avx2(__m256i& v0, __m256i& v1, __m256i& v2, __m256i& v3)
    {
        __m256i x0 = _mm256_permute2x128_si256(v0, v2, 0x20);
        __m256i x1 = _mm256_permute2x128_si256(v0, v2, 0x31);
        __m256i x2 = _mm256_permute2x128_si256(v1, v3, 0x20);
        __m256i x3 = _mm256_permute2x128_si256(v1, v3, 0x31);
        __m256i v4 = _mm256_unpacklo_epi16(x0, x1);
        __m256i v5 = _mm256_unpackhi_epi16(x0, x1);
        __m256i v6 = _mm256_unpacklo_epi16(x2, x3);
        __m256i v7 = _mm256_unpackhi_epi16(x2, x3);
        x0 = _mm256_unpacklo_epi32(v4, v6);
        x1 = _mm256_unpackhi_epi32(v4, v6);
        x2 = _mm256_unpacklo_epi32(v5, v7);
        x3 = _mm256_unpackhi_epi32(v5, v7);
        v0 = _mm256_permute4x64_epi64(x0, 0xd8);
        v1 = _mm256_permute4x64_epi64(x1, 0xd8);
        v2 = _mm256_permute4x64_epi64(x2, 0xd8);
        v3 = _mm256_permute4x64_epi64(x3, 0xd8);
    }

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

    #define JPEG_CONST16_AVX2(x, y) \
        _mm256_setr_epi16(x, y, x, y, x, y, x, y, x, y, x, y, x, y, x, y)

    template <int nbits>
    __m256i transform_sub_term(__m256i c0, __m256i c1, __m256i c2, __m256i c3, __m256i c4, __m256i c5)
    {
        __m256i a;
        __m256i b;

        a = _mm256_madd_epi16(c0, c1);
        a = _mm256_srai_epi32(a, nbits);
        __m128i v2 = _mm_packs_epi32(_mm256_extracti128_si256(a, 0), _mm256_extracti128_si256(a, 1));

        a = _mm256_madd_epi16(c2, c3);
        b = _mm256_madd_epi16(c4, c5);
        a = _mm256_sub_epi32(a, b); // <-- sub
        a = _mm256_srai_epi32(a, nbits);
        __m128i v3 = _mm_packs_epi32(_mm256_extracti128_si256(a, 0), _mm256_extracti128_si256(a, 1));

        return _mm256_setr_m128i(v2, v3);
    }

    template <int nbits>
    __m256i transform_add_term(__m256i c0, __m256i c1, __m256i c2, __m256i c3, __m256i c4, __m256i c5)
    {
        __m256i a;
        __m256i b;

        a = _mm256_madd_epi16(c0, c1);
        a = _mm256_srai_epi32(a, nbits);
        __m128i v2 = _mm_packs_epi32(_mm256_extracti128_si256(a, 0), _mm256_extracti128_si256(a, 1));

        a = _mm256_madd_epi16(c2, c3);
        b = _mm256_madd_epi16(c4, c5);
        a = _mm256_add_epi32(a, b); // <-- add
        a = _mm256_srai_epi32(a, nbits);
        __m128i v3 = _mm_packs_epi32(_mm256_extracti128_si256(a, 0), _mm256_extracti128_si256(a, 1));

        return _mm256_setr_m128i(v2, v3);
    }

    template <int nbits>
    __m256i transform_term(__m128i v, __m256i c0, __m256i c1, __m256i c2, __m256i c3)
    {
        __m256i a;
        __m256i b;

        a = _mm256_madd_epi16(c0, c1);
        b = _mm256_madd_epi16(c2, c3);
        a = _mm256_add_epi32(a, b);
        a = _mm256_srai_epi32(a, nbits);
        __m128i v5 = _mm_packs_epi32(_mm256_extracti128_si256(a, 0), _mm256_extracti128_si256(a, 1));
        return _mm256_setr_m128i(v, v5);
    }

    static inline
    __m256i unpack(__m256i  x)
    {
        __m128i x0 = _mm256_extracti128_si256(x, 0);
        __m128i x1 = _mm256_extracti128_si256(x, 1);
        __m128i lo = _mm_unpacklo_epi16(x0, x1);
        __m128i hi = _mm_unpackhi_epi16(x0, x1);
        return _mm256_setr_m128i(lo, hi);
    }

    static
    void fdct_avx2(s16* dest, const s16* data, const s16* qtable)
    {
        constexpr s16 c1 = 1420; // cos 1PI/16 * root(2)
        constexpr s16 c2 = 1338; // cos 2PI/16 * root(2)
        constexpr s16 c3 = 1204; // cos 3PI/16 * root(2)
        constexpr s16 c5 = 805;  // cos 5PI/16 * root(2)
        constexpr s16 c6 = 554;  // cos 6PI/16 * root(2)
        constexpr s16 c7 = 283;  // cos 7PI/16 * root(2)

        const __m256i c26p = JPEG_CONST16_AVX2(c2, c6);
        const __m256i c62n = JPEG_CONST16_AVX2(c6,-c2);
        const __m256i c75n = JPEG_CONST16_AVX2(c7,-c5);
        const __m256i c31n = JPEG_CONST16_AVX2(c3,-c1);
        const __m256i c51n = JPEG_CONST16_AVX2(c5,-c1);
        const __m256i c73p = JPEG_CONST16_AVX2(c7, c3);
        const __m256i c37n = JPEG_CONST16_AVX2(c3,-c7);
        const __m256i c15p = JPEG_CONST16_AVX2(c1, c5);
        const __m256i c13p = JPEG_CONST16_AVX2(c1, c3);
        const __m256i c57p = JPEG_CONST16_AVX2(c5, c7);

        // load

        const __m256i* s = reinterpret_cast<const __m256i *>(data);
        __m256i s0 = _mm256_loadu_si256(s + 0);
        __m256i s1 = _mm256_loadu_si256(s + 1);
        __m256i s2 = _mm256_loadu_si256(s + 2);
        __m256i s3 = _mm256_loadu_si256(s + 3);

        transpose_8x8_avx2(s0, s1, s2, s3);

        // pass 1

        __m256i v76 = _mm256_permute4x64_epi64(s3, 0x4e);
        __m256i v54 = _mm256_permute4x64_epi64(s2, 0x4e);

        __m256i x87 = _mm256_add_epi16(s0, v76);
        __m256i x65 = _mm256_add_epi16(s1, v54);
        __m256i x01 = _mm256_sub_epi16(s0, v76);
        __m256i x23 = _mm256_sub_epi16(s1, v54);
        __m256i x56 = _mm256_permute4x64_epi64(x65, 0x4e);

        __m256i x45;
        x45 = _mm256_add_epi16(x87, x56);
        x87 = _mm256_sub_epi16(x87, x56);

        __m128i x4 = _mm256_extracti128_si256(x45, 0);
        __m128i x5 = _mm256_extracti128_si256(x45, 1);
        __m128i v0 = _mm_add_epi16(x4, x5);
        __m128i v4 = _mm_sub_epi16(x4, x5);

        // transform

        x87 = unpack(x87);
        x01 = unpack(x01);
        x23 = unpack(x23);

        s0 = transform_term<10>(v0, x01, c13p, x23, c57p);
        s2 = transform_term<10>(v4, x01, c51n, x23, c73p);
        s1 = transform_sub_term<10>(x87, c26p, x01, c37n, x23, c15p);
        s3 = transform_add_term<10>(x87, c62n, x01, c75n, x23, c31n);

        // pass 2

        transpose_8x8_avx2(s0, s1, s2, s3);

        v76 = _mm256_permute4x64_epi64(s3, 0x4e);
        v54 = _mm256_permute4x64_epi64(s2, 0x4e);

        x87 = _mm256_add_epi16(s0, v76);
        x65 = _mm256_add_epi16(s1, v54);
        x01 = _mm256_sub_epi16(s0, v76);
        x23 = _mm256_sub_epi16(s1, v54);
        x56 = _mm256_permute4x64_epi64(x65, 0x4e);

        x45 = _mm256_add_epi16(x87, x56);
        x87 = _mm256_sub_epi16(x87, x56);

        x4 = _mm256_extracti128_si256(x45, 0);
        x5 = _mm256_extracti128_si256(x45, 1);
        v0 = _mm_add_epi16(x4, x5);
        v4 = _mm_sub_epi16(x4, x5);
        v0 = _mm_srai_epi16(v0, 3);
        v4 = _mm_srai_epi16(v4, 3);

        // transform

        x87 = unpack(x87);
        x01 = unpack(x01);
        x23 = unpack(x23);

        s0 = transform_term<13>(v0, x01, c13p, x23, c57p);
        s2 = transform_term<13>(v4, x01, c51n, x23, c73p);
        s1 = transform_sub_term<13>(x87, c26p, x01, c37n, x23, c15p);
        s3 = transform_add_term<13>(x87, c62n, x01, c75n, x23, c31n);

        // quantize

        const __m256i one = _mm256_set1_epi16(1);
        const __m256i bias = _mm256_set1_epi16(0x4000);
        const __m256i* q = reinterpret_cast<const __m256i*>(qtable);

        s0 = quantize(s0, q[0], one, bias);
        s1 = quantize(s1, q[1], one, bias);
        s2 = quantize(s2, q[2], one, bias);
        s3 = quantize(s3, q[3], one, bias);

        // store

        __m256i* d = reinterpret_cast<__m256i *>(dest);
        _mm256_storeu_si256(d + 0, s0);
        _mm256_storeu_si256(d + 1, s1);
        _mm256_storeu_si256(d + 2, s2);
        _mm256_storeu_si256(d + 3, s3);
    }

#endif // defined(MANGO_ENABLE_AVX2)

#if defined(MANGO_ENABLE_NEON)

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
    void fdct_neon(s16* dest, const s16* data, const s16* qtable)
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
        const int16x8_t* q = reinterpret_cast<const int16x8_t *>(qtable);

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

#endif // defined(MANGO_ENABLE_NEON)

    static inline
    u8* encode_dc(HuffmanEncoder& encoder, u8* p, s16 dc, const jpegEncoder::Channel& channel)
    {
        int coeff = dc - encoder.last_dc_value[channel.component];
        encoder.last_dc_value[channel.component] = dc;

        int absCoeff = std::abs(coeff);
        coeff -= (absCoeff != coeff);

        u32 size = absCoeff ? u32_log2(absCoeff) + 1 : 0;
        u32 mask = (1 << size) - 1;

        p = encoder.putBits(p, channel.dc_code[size] | (coeff & mask), channel.dc_size[size]);

        return p;
    }

    // ----------------------------------------------------------------------------
    // encode_block_scalar
    // ----------------------------------------------------------------------------

    static
    u8* encode_block_scalar(HuffmanEncoder& encoder, u8* p, const s16* input, const jpegEncoder::Channel& channel)
    {
        s16 block[64];
        encoder.fdct(block, input, channel.qtable);

        p = encode_dc(encoder, p, block[0], channel);

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

        const u32* ac_code = channel.ac_code;
        const u16* ac_size = channel.ac_size;
        const u32 zero16_code = ac_code[1];
        const u32 zero16_size = ac_size[1];

        int counter = 0;

        for (int i = 1; i < 64; ++i)
        {
            int coeff = block[zigzag_table_inverse[i]];
            if (coeff)
            {
                while (counter > 15)
                {
                    counter -= 16;
                    p = encoder.putBits(p, zero16_code, zero16_size);
                }

                int absCoeff = std::abs(coeff);
                coeff -= (absCoeff != coeff);

                u32 size = u32_log2(absCoeff) + 1;
                u32 mask = (1 << size) - 1;

                int index = counter + size * 16;
                p = encoder.putBits(p, ac_code[index] | (coeff & mask), ac_size[index]);

                counter = 0;
            }
            else
            {
                ++counter;
            }
        }

        if (counter)
        {
            p = encoder.putBits(p, ac_code[0], ac_size[0]);
        }

        return p;
    }

#if defined(MANGO_ENABLE_SSE4_1)

    // ----------------------------------------------------------------------------
    // encode_block_ssse3
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

    u64 zigzag_ssse3(s16* out, const s16* in)
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
        u64 mask_lo = u64(_mm_movemask_epi8(zero0)) | ((u64(_mm_movemask_epi8(zero1))) << 16);
        u64 mask_hi = u64(_mm_movemask_epi8(zero2)) | ((u64(_mm_movemask_epi8(zero3))) << 16);
        u64 zeromask = ~((mask_hi << 32) | mask_lo);

        __m128i* dest = reinterpret_cast<__m128i *>(out);
        _mm_storeu_si128(dest + 0, row0);
        _mm_storeu_si128(dest + 1, row1);
        _mm_storeu_si128(dest + 2, row2);
        _mm_storeu_si128(dest + 3, row3);
        _mm_storeu_si128(dest + 4, row4);
        _mm_storeu_si128(dest + 5, row5);
        _mm_storeu_si128(dest + 6, row6);
        _mm_storeu_si128(dest + 7, row7);

        return zeromask;
    }

    static
    u8* encode_block_ssse3(HuffmanEncoder& encoder, u8* p, const s16* input, const jpegEncoder::Channel& channel)
    {
        s16 block[64];
        encoder.fdct(block, input, channel.qtable);

        s16 temp[64];
        u64 zeromask = zigzag_ssse3(temp, block);

        p = encode_dc(encoder, p, temp[0], channel);
        zeromask >>= 1;

        const u32* ac_code = channel.ac_code;
        const u16* ac_size = channel.ac_size;
        const u32 zero16_code = ac_code[1];
        const u32 zero16_size = ac_size[1];

        for (int i = 1; i < 64; ++i)
        {
            if (!zeromask)
            {
                // only zeros left
                p = encoder.putBits(p, ac_code[0], ac_size[0]);
                break;
            }

            int counter = u64_tzcnt(zeromask); // BMI
            zeromask >>= (counter + 1);
            i += counter;

            while (counter > 15)
            {
                counter -= 16;
                p = encoder.putBits(p, zero16_code, zero16_size);
            }

            int coeff = temp[i];
            int absCoeff = std::abs(coeff);
            coeff -= (absCoeff != coeff);

            u32 size = u32_log2(absCoeff) + 1;
            u32 mask = (1 << size) - 1;

            int index = counter + size * 16;
            p = encoder.putBits(p, ac_code[index] | (coeff & mask), ac_size[index]);
        }

        return p;
    }

#endif // defined(MANGO_ENABLE_SSE4_1)

#if defined(MANGO_ENABLE_AVX512)

    // ----------------------------------------------------------------------------
    // encode_block_avx512
    // ----------------------------------------------------------------------------

//#define PROTOTYPE_PARALLEL_COEFFICIENTS
#ifdef PROTOTYPE_PARALLEL_COEFFICIENTS

    static
    inline __m512i getSymbolSize(__m512i absCoeff)
    {
        int16x32 value(absCoeff);
        int16x32 base(0);
        int16x32 temp;
        mask16x32 mask;

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

    static
    inline __m512i absSymbolSize(__m512i* ptr_sz, __m512i coeff)
    {
        __m512i absCoeff = _mm512_abs_epi16(coeff);

        __m512i one = _mm512_set1_epi16(1);
        __mmask32 mask = ~_mm512_cmpeq_epi16_mask(absCoeff, coeff);
        coeff = _mm512_mask_sub_epi16(coeff, mask, coeff, one);

        __m512i sz = getSymbolSize(absCoeff);
        _mm512_storeu_si512(ptr_sz, sz);

        return coeff;
    }

#endif // PROTOTYPE_PARALLEL_COEFFICIENTS

    u64 zigzag_avx512bw(s16* out, const s16* in)
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
        const __m512i v0 = _mm512_permutex2var_epi16(src0, table0, src1);
        const __m512i v1 = _mm512_permutex2var_epi16(src0, table1, src1);

        // compute zeromask
        const __m512i zero = _mm512_setzero_si512();
        __mmask32 mask_lo = _mm512_cmpneq_epu16_mask(v0, zero);
        __mmask32 mask_hi = _mm512_cmpneq_epu16_mask(v1, zero);
        u64 zeromask = (u64(mask_hi) << 32) | mask_lo;

        __m512i* dest = reinterpret_cast<__m512i *>(out);

        _mm512_storeu_si512(dest + 0, v0);
        _mm512_storeu_si512(dest + 1, v1);

        return zeromask;
    }

    static
    u8* encode_block_avx512bw(HuffmanEncoder& encoder, u8* p, const s16* input, const jpegEncoder::Channel& channel)
    {
        s16 block[64];
        encoder.fdct(block, input, channel.qtable);

        s16 temp[64];
        u64 zeromask = zigzag_avx512bw(temp, block);

        p = encode_dc(encoder, p, temp[0], channel);
        zeromask >>= 1;

        const u32* ac_code = channel.ac_code;
        const u16* ac_size = channel.ac_size;
        const u32 zero16_code = ac_code[1];
        const u32 zero16_size = ac_size[1];

#ifdef PROTOTYPE_PARALLEL_COEFFICIENTS
        s16 s_coeff[64];
        s16 s_size[64];
        __m512i c0 = *(__m512i*)(temp + 0);
        __m512i c1 = *(__m512i*)(temp + 32);
        c0 = absSymbolSize( (__m512i*)(s_size + 0), c0);
        c1 = absSymbolSize( (__m512i*)(s_size + 32), c1);
        *(__m512i*)(s_coeff + 0) = c0;
        *(__m512i*)(s_coeff + 32) = c1;
#endif

        for (int i = 1; i < 64; ++i)
        {
            if (!zeromask)
            {
                // only zeros left
                p = encoder.putBits(p, ac_code[0], ac_size[0]);
                break;
            }

            int counter = u64_tzcnt(zeromask); // BMI
            zeromask >>= (counter + 1);
            i += counter;

            while (counter > 15)
            {
                counter -= 16;
                p = encoder.putBits(p, zero16_code, zero16_size);
            }

#ifdef PROTOTYPE_PARALLEL_COEFFICIENTS
            int coeff = s_coeff[i];
            u32 size = s_size[i];
            u32 mask = (1 << size) - 1;
#else
            int coeff = temp[i];
            int absCoeff = std::abs(coeff);
            coeff -= (absCoeff != coeff);

            u32 size = u32_log2(absCoeff) + 1;
            u32 mask = (1 << size) - 1;
#endif

            int index = counter + size * 16;
            p = encoder.putBits(p, ac_code[index] | (coeff & mask), ac_size[index]);
        }

        return p;
    }

#endif // defined(MANGO_ENABLE_AVX512)

#if defined(MANGO_ENABLE_NEON64)

    // ----------------------------------------------------------------------------
    // encode_block_neon
    // ----------------------------------------------------------------------------

#if defined(MANGO_COMPILER_GCC)

    static inline
    uint8x16x4_t jpeg_vld1q_u8_x4(const u8* p)
    {
        uint8x16x4_t result;
        result.val[0] = vld1q_u8(p +  0);
        result.val[1] = vld1q_u8(p + 16);
        result.val[2] = vld1q_u8(p + 32);
        result.val[3] = vld1q_u8(p + 48);
        return result;
    }

    static inline
    int8x16x4_t jpeg_vld1q_s8_x4(const s8* p)
    {
        int8x16x4_t result;
        result.val[0] = vld1q_s8(p +  0);
        result.val[1] = vld1q_s8(p + 16);
        result.val[2] = vld1q_s8(p + 32);
        result.val[3] = vld1q_s8(p + 48);
        return result;
    }

#else

    static inline
    uint8x16x4_t jpeg_vld1q_u8_x4(const u8 *p)
    {
        return vld1q_u8_x4(p);
    }

    static inline
    int8x16x4_t jpeg_vld1q_s8_x4(const s8 *p)
    {
        return vld1q_s8_x4(p);
    }

#endif // MANGO_COMPILER_GCC

    u64 zigzag_neon64(s16* out, const s16* in)
    {

#define ID(x) x * 2 + 0, x * 2 + 1
#define ID_255 255, 255

        const u8 zigzag_shuffle [] =
        {
            ID( 0), ID( 1), ID( 8), ID(16), ID(9),  ID( 2), ID( 3), ID(10),
            ID(17), ID(24), ID_255, ID(25), ID(18), ID(11), ID( 4), ID( 5),
            ID_255, ID( 3), ID(10), ID(17), ID(24), ID_255, ID(25), ID(18),
            ID(27), ID(20), ID(13), ID( 6), ID( 7), ID(14), ID(21), ID(28),
            ID( 3), ID(10), ID(17), ID(24), ID(25), ID(18), ID(11), ID( 4),
            ID(13), ID( 6), ID_255, ID( 7), ID(14), ID(21), ID(28), ID_255,
            ID(26), ID(27), ID(20), ID(13), ID( 6), ID_255, ID(7),  ID(14),
            ID(13), ID(20), ID(21), ID(14), ID( 7), ID(15), ID(22), ID(23),
        };

#undef ID
#undef ID_255

        const uint8x16x4_t idx_rows_0123 = jpeg_vld1q_u8_x4(zigzag_shuffle + 0 * 8);
        const uint8x16x4_t idx_rows_4567 = jpeg_vld1q_u8_x4(zigzag_shuffle + 8 * 8);

        const int8x16x4_t tbl_rows_0123 = jpeg_vld1q_s8_x4((s8 *)(in + 0 * 8));
        const int8x16x4_t tbl_rows_4567 = jpeg_vld1q_s8_x4((s8 *)(in + 4 * 8));

        const int8x16x4_t tbl_rows_2345 =
        {{
            tbl_rows_0123.val[2], tbl_rows_0123.val[3],
            tbl_rows_4567.val[0], tbl_rows_4567.val[1]
        }};
        const int8x16x3_t tbl_rows_567 = {{ tbl_rows_4567.val[1], tbl_rows_4567.val[2], tbl_rows_4567.val[3] }};

        // shuffle coefficients
        int16x8_t row0 = vreinterpretq_s16_s8(vqtbl4q_s8(tbl_rows_0123, idx_rows_0123.val[0]));
        int16x8_t row1 = vreinterpretq_s16_s8(vqtbl4q_s8(tbl_rows_0123, idx_rows_0123.val[1]));
        int16x8_t row2 = vreinterpretq_s16_s8(vqtbl4q_s8(tbl_rows_2345, idx_rows_0123.val[2]));
        int16x8_t row3 = vreinterpretq_s16_s8(vqtbl4q_s8(tbl_rows_0123, idx_rows_0123.val[3]));
        int16x8_t row4 = vreinterpretq_s16_s8(vqtbl4q_s8(tbl_rows_4567, idx_rows_4567.val[0]));
        int16x8_t row5 = vreinterpretq_s16_s8(vqtbl4q_s8(tbl_rows_2345, idx_rows_4567.val[1]));
        int16x8_t row6 = vreinterpretq_s16_s8(vqtbl4q_s8(tbl_rows_4567, idx_rows_4567.val[2]));
        int16x8_t row7 = vreinterpretq_s16_s8(vqtbl3q_s8(tbl_rows_567, idx_rows_4567.val[3]));

        // patch "holes" left in the shuffle table (ID_255)
        row1 = vsetq_lane_s16(vgetq_lane_s16(vreinterpretq_s16_s8(tbl_rows_4567.val[0]), 0), row1, 2);
        row2 = vsetq_lane_s16(vgetq_lane_s16(vreinterpretq_s16_s8(tbl_rows_0123.val[1]), 4), row2, 0);
        row2 = vsetq_lane_s16(vgetq_lane_s16(vreinterpretq_s16_s8(tbl_rows_4567.val[2]), 0), row2, 5);
        row5 = vsetq_lane_s16(vgetq_lane_s16(vreinterpretq_s16_s8(tbl_rows_0123.val[1]), 7), row5, 2);
        row5 = vsetq_lane_s16(vgetq_lane_s16(vreinterpretq_s16_s8(tbl_rows_4567.val[2]), 3), row5, 7);
        row6 = vsetq_lane_s16(vgetq_lane_s16(vreinterpretq_s16_s8(tbl_rows_0123.val[3]), 7), row6, 5);

        // zeromask
        const uint16x8_t zero = vdupq_n_u16(0);
        uint8x8_t gt0 = vmovn_u16(vcgtq_u16(vreinterpretq_u16_s16(row0), zero));
        uint8x8_t gt1 = vmovn_u16(vcgtq_u16(vreinterpretq_u16_s16(row1), zero));
        uint8x8_t gt2 = vmovn_u16(vcgtq_u16(vreinterpretq_u16_s16(row2), zero));
        uint8x8_t gt3 = vmovn_u16(vcgtq_u16(vreinterpretq_u16_s16(row3), zero));
        uint8x8_t gt4 = vmovn_u16(vcgtq_u16(vreinterpretq_u16_s16(row4), zero));
        uint8x8_t gt5 = vmovn_u16(vcgtq_u16(vreinterpretq_u16_s16(row5), zero));
        uint8x8_t gt6 = vmovn_u16(vcgtq_u16(vreinterpretq_u16_s16(row6), zero));
        uint8x8_t gt7 = vmovn_u16(vcgtq_u16(vreinterpretq_u16_s16(row7), zero));

        const uint8x16_t mask = { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };
        uint8x16_t gt01 = vandq_u8(vcombine_u8(gt0, gt1), mask);
        uint8x16_t gt23 = vandq_u8(vcombine_u8(gt2, gt3), mask);
        uint8x16_t gt45 = vandq_u8(vcombine_u8(gt4, gt5), mask);
        uint8x16_t gt67 = vandq_u8(vcombine_u8(gt6, gt7), mask);
        uint8x16_t gt0123 = vpaddq_u8(gt01, gt23);
        uint8x16_t gt4567 = vpaddq_u8(gt45, gt67);
        uint8x16_t gt01234567 = vpaddq_u8(gt0123, gt4567);
        uint8x16_t x = vpaddq_u8(gt01234567, gt01234567);
        u64 zeromask = vgetq_lane_u64(vreinterpretq_u64_u8(x), 0);

        vst1q_s16(out + 0 * 8, row0);
        vst1q_s16(out + 1 * 8, row1);
        vst1q_s16(out + 2 * 8, row2);
        vst1q_s16(out + 3 * 8, row3);
        vst1q_s16(out + 4 * 8, row4);
        vst1q_s16(out + 5 * 8, row5);
        vst1q_s16(out + 6 * 8, row6);
        vst1q_s16(out + 7 * 8, row7);

        return zeromask;
    }

    static
    u8* encode_block_neon64(HuffmanEncoder& encoder, u8* p, const s16* input, const jpegEncoder::Channel& channel)
    {
        s16 block[64];
        encoder.fdct(block, input, channel.qtable);

        s16 temp[64];
        u64 zeromask = zigzag_neon64(temp, block);

        p = encode_dc(encoder, p, temp[0], channel);
        zeromask >>= 1;

        const u32* ac_code = channel.ac_code;
        const u16* ac_size = channel.ac_size;
        const u32 zero16_code = ac_code[1];
        const u32 zero16_size = ac_size[1];

        for (int i = 1; i < 64; ++i)
        {
            if (!zeromask)
            {
                // only zeros left
                p = encoder.putBits(p, ac_code[0], ac_size[0]);
                break;
            }

            int counter = u64_tzcnt(zeromask);
            zeromask >>= (counter + 1);
            i += counter;

            while (counter > 15)
            {
                counter -= 16;
                p = encoder.putBits(p, zero16_code, zero16_size);
            }

            int coeff = temp[i];
            int absCoeff = std::abs(coeff);
            coeff -= (absCoeff != coeff);

            u32 size = u32_log2(absCoeff) + 1;
            u32 mask = (1 << size) - 1;

            int index = counter + size * 16;
            p = encoder.putBits(p, ac_code[index] | (coeff & mask), ac_size[index]);
        }

        return p;
    }

#endif // defined(MANGO_ENABLE_NEON64)

    // ----------------------------------------------------------------------------
    // read_xxx_format
    // ----------------------------------------------------------------------------

    static
    void compute_ycbcr(s16* dest, int r, int g, int b)
    {
        int y = (76 * r + 151 * g + 29 * b) >> 8;
        int cr = ((r - y) * 182) >> 8;
        int cb = ((b - y) * 144) >> 8;
        dest[0 * 64] = s16(y - 128);
        dest[1 * 64] = s16(cb);
        dest[2 * 64] = s16(cr);
    }

    static
    void read_y_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int y = 0; y < rows; ++y)
        {
            for (int x = 0; x < cols; ++x)
            {
                *block++ = input[x] - 128;
            }

            // replicate last column
            if (cols < 8)
            {
                const int count = 8 - cols;
                std::fill_n(block, count, block[-1]);
                block += count;
            }

            input += stride;
        }

        // replicate last row
        for (int y = rows; y < 8; ++y)
        {
            std::memcpy(block, block - 8, 16);
            block += 8;
        }
    }

    static
    void read_bgr_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int y = 0; y < rows; ++y)
        {
            const u8* scan = input;

            for (int x = 0; x < cols; ++x)
            {
                int r = scan[2];
                int g = scan[1];
                int b = scan[0];
                compute_ycbcr(block, r, g, b);
                ++block;
                scan += 3;
            }

            // replicate last column
            if (cols < 8)
            {
                const int count = 8 - cols;
                std::fill_n(block + 0 * 64, count, block[0 * 64 - 1]);
                std::fill_n(block + 1 * 64, count, block[1 * 64 - 1]);
                std::fill_n(block + 2 * 64, count, block[2 * 64 - 1]);
                block += count;
            }

            input += stride;
        }

        // replicate last row
        for (int y = rows; y < 8; ++y)
        {
            std::memcpy(block + 0 * 64, block + 0 * 64 - 8, 16);
            std::memcpy(block + 1 * 64, block + 1 * 64 - 8, 16);
            std::memcpy(block + 2 * 64, block + 2 * 64 - 8, 16);
            block += 8;
        }
    }

    static
    void read_rgb_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int y = 0; y < rows; ++y)
        {
            const u8* scan = input;
            for (int x = 0; x < cols; ++x)
            {
                int r = scan[0];
                int g = scan[1];
                int b = scan[2];
                compute_ycbcr(block, r, g, b);
                ++block;
                scan += 3;
            }

            // replicate last column
            if (cols < 8)
            {
                const int count = 8 - cols;
                std::fill_n(block + 0 * 64, count, block[0 * 64 - 1]);
                std::fill_n(block + 1 * 64, count, block[1 * 64 - 1]);
                std::fill_n(block + 2 * 64, count, block[2 * 64 - 1]);
                block += count;
            }

            input += stride;
        }

        // replicate last row
        for (int y = rows; y < 8; ++y)
        {
            std::memcpy(block + 0 * 64, block + 0 * 64 - 8, 16);
            std::memcpy(block + 1 * 64, block + 1 * 64 - 8, 16);
            std::memcpy(block + 2 * 64, block + 2 * 64 - 8, 16);
            block += 8;
        }
    }

    static
    void read_bgra_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int y = 0; y < rows; ++y)
        {
            const u8* scan = input;
            for (int x = 0; x < cols; ++x)
            {
                s16 r = scan[2];
                s16 g = scan[1];
                s16 b = scan[0];
                compute_ycbcr(block, r, g, b);
                ++block;
                scan += 4;
            }

            // replicate last column
            if (cols < 8)
            {
                const int count = 8 - cols;
                std::fill_n(block + 0 * 64, count, block[0 * 64 - 1]);
                std::fill_n(block + 1 * 64, count, block[1 * 64 - 1]);
                std::fill_n(block + 2 * 64, count, block[2 * 64 - 1]);
                block += count;
            }

            input += stride;
        }

        // replicate last row
        for (int y = rows; y < 8; ++y)
        {
            std::memcpy(block + 0 * 64, block + 0 * 64 - 8, 16);
            std::memcpy(block + 1 * 64, block + 1 * 64 - 8, 16);
            std::memcpy(block + 2 * 64, block + 2 * 64 - 8, 16);
            block += 8;
        }
    }

    static
    void read_rgba_format(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        for (int y = 0; y < rows; ++y)
        {
            const u8* scan = input;
            for (int x = 0; x < cols; ++x)
            {
                int r = scan[0];
                int g = scan[1];
                int b = scan[2];
                compute_ycbcr(block, r, g, b);
                ++block;
                scan += 4;
            }

            // replicate last column
            if (cols < 8)
            {
                const int count = 8 - cols;
                std::fill_n(block + 0 * 64, count, block[0 * 64 - 1]);
                std::fill_n(block + 1 * 64, count, block[1 * 64 - 1]);
                std::fill_n(block + 2 * 64, count, block[2 * 64 - 1]);
                block += count;
            }

            input += stride;
        }

        // replicate last row
        for (int y = rows; y < 8; ++y)
        {
            std::memcpy(block + 0 * 64, block + 0 * 64 - 8, 16);
            std::memcpy(block + 1 * 64, block + 1 * 64 - 8, 16);
            std::memcpy(block + 2 * 64, block + 2 * 64 - 8, 16);
            block += 8;
        }
    }

#if defined(MANGO_ENABLE_SSE2) || defined(MANGO_ENABLE_SSE4_1)

    static
    void compute_ycbcr(s16* dest, __m128i r, __m128i g, __m128i b)
    {
        const __m128i c076 = _mm_set1_epi16(76);
        const __m128i c151 = _mm_set1_epi16(151);
        const __m128i c029 = _mm_set1_epi16(29);
        const __m128i c128 = _mm_set1_epi16(128);
        const __m128i c182 = _mm_set1_epi16(182);
        const __m128i c144 = _mm_set1_epi16(144);

        // compute luminance
        __m128i s0 = _mm_mullo_epi16(r, c076);
        __m128i s1 = _mm_mullo_epi16(g, c151);
        __m128i s2 = _mm_mullo_epi16(b, c029);
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
        __m128i* ptr = reinterpret_cast<__m128i*>(dest);
        _mm_storeu_si128(ptr +  0, s);
        _mm_storeu_si128(ptr +  8, cb);
        _mm_storeu_si128(ptr + 16, cr);
    }

#endif // defined(MANGO_ENABLE_SSE2) || defined(MANGO_ENABLE_SSE4_1)

#if defined(MANGO_ENABLE_SSE2)

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
#if defined(MANGO_ENABLE_SSE4_1)
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

        const __m128i mask = _mm_set1_epi32(0xff);

        for (int y = 0; y < 8; ++y)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(input);

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

            compute_ycbcr(block, r, g, b);

            block += 8;
            input += stride;
        }
    }

    static
    void read_rgba_format_sse2(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        const __m128i mask = _mm_set1_epi32(0xff);

        for (int y = 0; y < 8; ++y)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(input);

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

            compute_ycbcr(block, r, g, b);

            block += 8;
            input += stride;
        }
    }

#endif // MANGO_ENABLE_SSE2

#if defined(MANGO_ENABLE_SSE4_1)

    static
    void read_bgr_format_ssse3(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        for (int y = 0; y < 8; ++y)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(input);

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

            compute_ycbcr(block, r, g, b);

            block += 8;
            input += stride;
        }
    }

    static
    void read_rgb_format_ssse3(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        for (int y = 0; y < 8; ++y)
        {
            const __m128i* ptr = reinterpret_cast<const __m128i*>(input);

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

            compute_ycbcr(block, r, g, b);

            block += 8;
            input += stride;
        }
    }

#endif // MANGO_ENABLE_SSE4_1

#if defined(MANGO_ENABLE_NEON)

    static
    void compute_ycbcr(s16* dest, int16x8_t r, int16x8_t g, int16x8_t b)
    {
        const int16x8_t c076 = vdupq_n_s16(76);
        const int16x8_t c151 = vdupq_n_s16(151);
        const int16x8_t c029 = vdupq_n_s16(29);
        const int16x8_t c182 = vdupq_n_s16(182);
        const int16x8_t c144 = vdupq_n_s16(144);
        const int16x8_t c128 = vdupq_n_s16(128);

        // compute luminance
        int16x8_t s = vmulq_s16(r, c076);
        s = vmlaq_s16(s, g, c151);
        s = vmlaq_s16(s, b, c029);
        s = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(s), 8));

        // compute chroma
        int16x8_t cr = vmulq_s16(vsubq_s16(r, s), c182);
        int16x8_t cb = vmulq_s16(vsubq_s16(b, s), c144);
        cr = vshrq_n_s16(cr, 8);
        cb = vshrq_n_s16(cb, 8);

        // adjust bias
        s = vsubq_s16(s, c128);

        // store
        vst1q_s16(dest +   0, s);
        vst1q_s16(dest +  64, cb);
        vst1q_s16(dest + 128, cr);
    }

    static
    void read_y_format_neon(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        const int16x8_t c128 = vdupq_n_s16(128);

        for (int y = 0; y < 8; ++y)
        {
            uint8x8_t v = vld1_u8(input);
            int16x8_t s = vreinterpretq_s16_u16(vmovl_u8(v));
            s = vsubq_s16(s, c128);
            vst1q_s16(block, s);

            input += stride;
            block += 8;
        }
    }

    static
    void read_bgra_format_neon(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        for (int y = 0; y < 8; ++y)
        {
            const uint8x8x4_t temp = vld4_u8(input);
            int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(temp.val[2]));
            int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(temp.val[1]));
            int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(temp.val[0]));

            compute_ycbcr(block, r, g, b);

            input += stride;
            block += 8;
        }
    }

    static
    void read_rgba_format_neon(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        for (int y = 0; y < 8; ++y)
        {
            const uint8x8x4_t temp = vld4_u8(input);
            int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(temp.val[0]));
            int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(temp.val[1]));
            int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(temp.val[2]));

            compute_ycbcr(block, r, g, b);

            input += stride;
            block += 8;
        }
    }

    static
    void read_bgr_format_neon(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        for (int y = 0; y < 8; ++y)
        {
            const uint8x8x3_t temp = vld3_u8(input);
            int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(temp.val[2]));
            int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(temp.val[1]));
            int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(temp.val[0]));

            compute_ycbcr(block, r, g, b);

            input += stride;
            block += 8;
        }
    }

    static
    void read_rgb_format_neon(s16* block, const u8* input, size_t stride, int rows, int cols)
    {
        MANGO_UNREFERENCED(rows);
        MANGO_UNREFERENCED(cols);

        for (int y = 0; y < 8; ++y)
        {
            const uint8x8x3_t temp = vld3_u8(input);
            int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(temp.val[0]));
            int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(temp.val[1]));
            int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(temp.val[2]));

            compute_ycbcr(block, r, g, b);

            input += stride;
            block += 8;
        }
    }

#endif // MANGO_ENABLE_NEON

    // ----------------------------------------------------------------------------
    // jpegEncoder
    // ----------------------------------------------------------------------------

    jpegEncoder::jpegEncoder(const Surface& surface, SampleType sample, const ImageEncodeOptions& options)
        : m_surface(surface)
        , m_sample(sample)
        , m_options(options)
        , inverse_luminance_qtable(64)
        , inverse_chrominance_qtable(64)
    {
        components = 0;

        channel[0].component = 0;
        channel[0].qtable = inverse_luminance_qtable;
        channel[0].dc_code = g_luminance_dc_code_table;
        channel[0].dc_size = g_luminance_dc_size_table;
        channel[0].ac_code = g_luminance_ac_code_table;
        channel[0].ac_size = g_luminance_ac_size_table;

        channel[1].component = 1;
        channel[1].qtable = inverse_chrominance_qtable;
        channel[1].dc_code = g_chrominance_dc_code_table;
        channel[1].dc_size = g_chrominance_dc_size_table;
        channel[1].ac_code = g_chrominance_ac_code_table;
        channel[1].ac_size = g_chrominance_ac_size_table;

        channel[2].component = 2;
        channel[2].qtable = inverse_chrominance_qtable;
        channel[2].dc_code = g_chrominance_dc_code_table;
        channel[2].dc_size = g_chrominance_dc_size_table;
        channel[2].ac_code = g_chrominance_ac_code_table;
        channel[2].ac_size = g_chrominance_ac_size_table;

        int bytes_per_pixel = 0;
        read_8x8 = nullptr;

        u64 flags = options.simd ? getCPUFlags() : 0;
        MANGO_UNREFERENCED(flags);

        // select sampler

        const char* sampler_name = "Scalar";

        switch (sample)
        {
            case JPEG_U8_Y:
#if defined(MANGO_ENABLE_SSE2)
                if (flags & INTEL_SSE2)
                {
                    read_8x8 = read_y_format_sse2;
                    sampler_name = "SSE2 Y 8x8";
                }
#endif
#if defined(MANGO_ENABLE_NEON)
                if (flags & ARM_NEON)
                {
                    read_8x8 = read_y_format_neon;
                    sampler_name = "NEON Y 8x8";
                }
#endif
                read = read_y_format;
                bytes_per_pixel = 1;
                components = 1;
                break;

            case JPEG_U8_BGR:
#if defined(MANGO_ENABLE_SSE4_1)
				if (flags & INTEL_SSSE3)
                {
                    read_8x8 = read_bgr_format_ssse3;
                    sampler_name = "SSSE3 BGR 8x8";
                }
#endif
#if defined(MANGO_ENABLE_NEON)
				if (flags & ARM_NEON)
                {
                    read_8x8 = read_bgr_format_neon;
                    sampler_name = "NEON BGR 8x8";
                }
#endif
                read = read_bgr_format;
                bytes_per_pixel = 3;
                components = 3;
                break;

            case JPEG_U8_RGB:
#if defined(MANGO_ENABLE_SSE4_1)
                if (flags & INTEL_SSSE3)
                {
                    read_8x8 = read_rgb_format_ssse3;
                    sampler_name = "SSSE3 RGB 8x8";
                }
#endif
#if defined(MANGO_ENABLE_NEON)
                if (flags & ARM_NEON)
                {
                    read_8x8 = read_rgb_format_neon;
                    sampler_name = "NEON RGB 8x8";
                }
#endif
                read = read_rgb_format;
                bytes_per_pixel = 3;
                components = 3;
                break;

            case JPEG_U8_BGRA:
#if defined(MANGO_ENABLE_SSE2)
                if (flags & INTEL_SSE2)
                {
                    read_8x8 = read_bgra_format_sse2;
                    sampler_name = "SSE2 BGRA 8x8";
                }
#endif
#if defined(MANGO_ENABLE_NEON)
                if (flags & ARM_NEON)
                {
                    read_8x8 = read_bgra_format_neon;
                    sampler_name = "NEON BGRA 8x8";
                }
#endif
                read = read_bgra_format;
                bytes_per_pixel = 4;
                components = 3;
                break;

            case JPEG_U8_RGBA:
#if defined(MANGO_ENABLE_SSE2)
                if (flags & INTEL_SSE2)
                {
                    read_8x8 = read_rgba_format_sse2;
                    sampler_name = "SSE2 RGBA 8x8";
                }
#endif
#if defined(MANGO_ENABLE_NEON)
                if (flags & ARM_NEON)
                {
                    read_8x8 = read_rgba_format_neon;
                    sampler_name = "NEON RGBA 8x8";
                }
#endif
                read = read_rgba_format;
                bytes_per_pixel = 4;
                components = 3;
                break;
        }

        if (!read_8x8)
        {
            // no accelerated 8x8 sampler found; use the default
            read_8x8 = read;
        }

        // select fdct

        fdct = fdct_scalar;
        const char* fdct_name = "Scalar";

#if defined(MANGO_ENABLE_SSE2)
        if (flags & INTEL_SSE2)
        {
            fdct = fdct_sse2;
            fdct_name = "SSE2";
        }
#endif

#if defined(MANGO_ENABLE_AVX2__disabled)
        if (flags & INTEL_AVX2)
        {
            fdct = fdct_avx2;
            fdct_name = "AVX2";
        }
#endif

#if defined(MANGO_ENABLE_NEON)
        {
            fdct = fdct_neon;
            fdct_name = "NEON";
        }
#endif

        // select block encoder

        encode = encode_block_scalar;
        const char* encode_name = "Scalar";

#if defined(MANGO_ENABLE_SSE4_1)
        if (flags & INTEL_SSSE3)
        {
            encode = encode_block_ssse3;
            encode_name = "SSSE3";
        }
#endif

#if defined(MANGO_ENABLE_AVX512)
        if (flags & INTEL_AVX512BW)
        {
            encode = encode_block_avx512bw;
            encode_name = "AVX512BW";
        }
#endif

#if defined(MANGO_ENABLE_NEON64)
        {
            encode = encode_block_neon64;
            encode_name = "NEON64";
        }
#endif

        // build encoder info string
        info = "fDCT: ";
        info += fdct_name;

        info += ", Color: ";
        info += sampler_name;

        info += ", Encoder: ";
        info += encode_name;

        mcu_width = 8;
        mcu_height = 8;

        horizontal_mcus = (m_surface.width + mcu_width - 1) >> 3;
        vertical_mcus   = (m_surface.height + mcu_height - 1) >> 3;

        rows_in_bottom_mcus = m_surface.height - (vertical_mcus - 1) * mcu_height;
        cols_in_right_mcus  = m_surface.width  - (horizontal_mcus - 1) * mcu_width;

        mcu_stride = mcu_width * bytes_per_pixel;

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

        // configure quality
        u32 quality = u32(std::pow(1.0f + clamp(1.0f - options.quality, 0.0f, 1.0f), 11.0f) * 8.0f);

        for (int i = 0; i < 64; ++i)
        {
            u8 index = zigzag_table[i];

            // luminance
            u32 L = u32_clamp((g_luminance_quant_table[i] * quality + 0x200) >> 10, 2, 255);
            luminance_qtable[index] = u8(L);
            inverse_luminance_qtable[i] = u16(0x8000 / L);

            // chrominance
            u32 C = u32_clamp((g_chrominance_quant_table [i] * quality + 0x200) >> 10, 2, 255);
            chrominance_qtable[index] = u8(C);
            inverse_chrominance_qtable[i] = u16(0x8000 / C);
        }
    }

    jpegEncoder::~jpegEncoder()
    {
    }

    void jpegEncoder::writeMarkers(BigEndianStream& p)
    {
        // Start of image marker
        p.write16(MARKER_SOI);

        // ICC profile.  splitting to multiple markers if necessary
        if (m_options.icc.size)
        {
            const size_t magicICCLength = 12;
            const u8 magicICC[magicICCLength] = { 0x49, 0x43, 0x43, 0x5f, 0x50, 0x52, 0x4f, 0x46, 0x49, 0x4c, 0x45, 0 }; // 'ICC_PROFILE', 0

            const size_t iccMaxSegmentSize = 65000;  // marker size is 16bit minus icc stuff, if larger we need to split
            const size_t iccSegments = (m_options.icc.size + iccMaxSegmentSize - 1) / iccMaxSegmentSize;

            for(size_t i=0; i < iccSegments; i++)
            {
                const size_t size = i < (iccSegments - 1) ? iccMaxSegmentSize :  m_options.icc.size % iccMaxSegmentSize;
                p.write16(MARKER_APP2);
                p.write16(u16(size + magicICCLength + 4));
                p.write(magicICC, magicICCLength);
                p.write8(u8(i + 1)); // segment index, 1-based
                p.write8(u8(iccSegments));
                p.write(m_options.icc.slice(i * iccMaxSegmentSize), size);
            }
        }

        // Quantization table marker
        p.write16(MARKER_DQT);
        p.write16(0x43); // quantization table length
        p.write8(0x00); // Pq, Tq

        // Lqt table
        p.write(luminance_qtable, 64);

        // Quantization table marker
        p.write16(MARKER_DQT);
        p.write16(0x43); // quantization table length
        p.write8(0x01); // Pq, Tq

        // Cqt table
        p.write(chrominance_qtable, 64);

        // Start of frame marker
        p.write16(MARKER_SOF0);

        u8 number_of_components = 0;

        switch (m_sample)
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
        p.write16(u16(m_surface.height)); // image height
        p.write16(u16(m_surface.width)); // image width
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
        p.write(g_marker_data, sizeof(g_marker_data));

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

    void jpegEncoder::encodeInterval(EncodeBuffer& buffer, const u8* image, size_t stride, ReadFunc read_func, int rows)
    {
        HuffmanEncoder huffman;
        huffman.fdct = fdct;

        const int right_mcu = horizontal_mcus - 1;

        constexpr int buffer_size = 2048;
        constexpr int flush_threshold = buffer_size - 512;

        u8 huff_temp[buffer_size]; // encoding buffer
        u8* ptr = huff_temp;

        int cols = mcu_width;
        auto reader = read_func;

        for (int x = 0; x < horizontal_mcus; ++x)
        {
            if (x >= right_mcu)
            {
                // horizontal clipping
                cols = cols_in_right_mcus;
                reader = read; // clipping reader
            }

            // read MCU data
            s16 block[BLOCK_SIZE * 3];
            reader(block, image, stride, rows, cols);

            // encode the data in MCU
            for (int i = 0; i < components; ++i)
            {
                ptr = encode(huffman, ptr, block + i * BLOCK_SIZE, channel[i]);
            }

            // flush encoding buffer
            if (ptr - huff_temp > flush_threshold)
            {
                buffer.append(huff_temp, ptr - huff_temp);
                ptr = huff_temp;
            }

            image += mcu_stride;
        }

        // flush encoding buffer
        ptr = huffman.flush(ptr);
        buffer.append(huff_temp, ptr - huff_temp);

        // mark buffer ready for writing
        buffer.ready = true;
    }

    ImageEncodeStatus jpegEncoder::encodeImage(Stream& stream)
    {
        const u8* image = m_surface.image;
        size_t stride = m_surface.stride;

        BigEndianStream s(stream);

        // writing marker data
        writeMarkers(s);

        // encode MCUs

        if (m_options.multithread)
        {
            ConcurrentQueue queue;

            // bitstream for each MCU scan
            std::vector<EncodeBuffer> buffers(vertical_mcus);

            for (int y = 0; y < vertical_mcus; ++y)
            {
                int rows = mcu_height;
                auto read_func = read_8x8; // default: optimized 8x8 reader

                if (y >= vertical_mcus - 1)
                {
                    // vertical clipping
                    rows = rows_in_bottom_mcus;
                    read_func = read; // clipping reader
                }

                queue.enqueue([this, y, &buffers, image, stride, rows, read_func]
                {
                    EncodeBuffer& buffer = buffers[y];
                    encodeInterval(buffer, image, stride, read_func, rows);
                });

                image += m_surface.stride * mcu_height;
            }

            for (int y = 0; y < vertical_mcus; ++y)
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
        }
        else
        {
            for (int y = 0; y < vertical_mcus; ++y)
            {
                int rows = mcu_height;
                auto read_func = read_8x8; // default: optimized 8x8 reader

                if (y >= vertical_mcus - 1)
                {
                    // vertical clipping
                    rows = rows_in_bottom_mcus;
                    read_func = read; // clipping reader
                }

                EncodeBuffer buffer;
                encodeInterval(buffer, image, stride, read_func, rows);

                // write huffman bitstream
                s.write(buffer, buffer.size());

                // write restart marker
                int index = y & 7;
                s.write16(MARKER_RST0 + index);

                image += m_surface.stride * mcu_height;
            }
        }

        // EOI marker
        s.write16(MARKER_EOI);

        ImageEncodeStatus status;
        status.info = info;

        return status;
    }

} // namespace

namespace mango::jpeg
{

    ImageEncodeStatus encodeImage(Stream& stream, const Surface& surface, const ImageEncodeOptions& options)
    {
        SampleFormat sf = getSampleFormat(surface.format);

        ImageEncodeStatus status;

        // encode
        if (surface.format == sf.format)
        {
            jpegEncoder encoder(surface, sf.sample, options);
            status = encoder.encodeImage(stream);
            status.direct = true;
        }
        else
        {
            // convert source surface to format supported in the encoder
            Bitmap temp(surface, sf.format);
            jpegEncoder encoder(temp, sf.sample, options);
            status = encoder.encodeImage(stream);
        }

        return status;
    }

} // namespace mango::jpeg
