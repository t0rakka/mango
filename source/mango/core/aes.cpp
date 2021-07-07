/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/aes.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/exception.hpp>
#include "../../external/aes/bc_aes.h"

namespace
{
    using namespace mango;

#if defined(__ARM_FEATURE_CRYPTO)

// ----------------------------------------------------------------------------------------
// ARM AES
// ----------------------------------------------------------------------------------------

// Forward S-box
static const u8 FSb[256] =
{
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,
    0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
    0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
    0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,
    0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
    0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,
    0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
    0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
    0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
    0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
    0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
    0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

static const u32 RT[] =
{
    0x50A7F451, 0x5365417E, 0xC3A4171A, 0x965E273A,
    0xCB6BAB3B, 0xF1459D1F, 0xAB58FAAC, 0x9303E34B,
    0x55FA3020, 0xF66D76AD, 0x9176CC88, 0x254C02F5,
    0xFCD7E54F, 0xD7CB2AC5, 0x80443526, 0x8FA362B5,
    0x495AB1DE, 0x671BBA25, 0x980EEA45, 0xE1C0FE5D,
    0x02752FC3, 0x12F04C81, 0xA397468D, 0xC6F9D36B,
    0xE75F8F03, 0x959C9215, 0xEB7A6DBF, 0xDA595295,
    0x2D83BED4, 0xD3217458, 0x2969E049, 0x44C8C98E,
    0x6A89C275, 0x78798EF4, 0x6B3E5899, 0xDD71B927,
    0xB64FE1BE, 0x17AD88F0, 0x66AC20C9, 0xB43ACE7D,
    0x184ADF63, 0x82311AE5, 0x60335197, 0x457F5362,
    0xE07764B1, 0x84AE6BBB, 0x1CA081FE, 0x942B08F9,
    0x58684870, 0x19FD458F, 0x876CDE94, 0xB7F87B52,
    0x23D373AB, 0xE2024B72, 0x578F1FE3, 0x2AAB5566,
    0x0728EBB2, 0x03C2B52F, 0x9A7BC586, 0xA50837D3,
    0xF2872830, 0xB2A5BF23, 0xBA6A0302, 0x5C8216ED,
    0x2B1CCF8A, 0x92B479A7, 0xF0F207F3, 0xA1E2694E,
    0xCDF4DA65, 0xD5BE0506, 0x1F6234D1, 0x8AFEA6C4,
    0x9D532E34, 0xA055F3A2, 0x32E18A05, 0x75EBF6A4,
    0x39EC830B, 0xAAEF6040, 0x069F715E, 0x51106EBD,
    0xF98A213E, 0x3D06DD96, 0xAE053EDD, 0x46BDE64D,
    0xB58D5491, 0x055DC471, 0x6FD40604, 0xFF155060,
    0x24FB9819, 0x97E9BDD6, 0xCC434089, 0x779ED967,
    0xBD42E8B0, 0x888B8907, 0x385B19E7, 0xDBEEC879,
    0x470A7CA1, 0xE90F427C, 0xC91E84F8, 0x00000000,
    0x83868009, 0x48ED2B32, 0xAC70111E, 0x4E725A6C,
    0xFBFF0EFD, 0x5638850F, 0x1ED5AE3D, 0x27392D36,
    0x64D90F0A, 0x21A65C68, 0xD1545B9B, 0x3A2E3624,
    0xB1670A0C, 0x0FE75793, 0xD296EEB4, 0x9E919B1B,
    0x4FC5C080, 0xA220DC61, 0x694B775A, 0x161A121C,
    0x0ABA93E2, 0xE52AA0C0, 0x43E0223C, 0x1D171B12,
    0x0B0D090E, 0xADC78BF2, 0xB9A8B62D, 0xC8A91E14,
    0x8519F157, 0x4C0775AF, 0xBBDD99EE, 0xFD607FA3,
    0x9F2601F7, 0xBCF5725C, 0xC53B6644, 0x347EFB5B,
    0x7629438B, 0xDCC623CB, 0x68FCEDB6, 0x63F1E4B8,
    0xCADC31D7, 0x10856342, 0x40229713, 0x2011C684,
    0x7D244A85, 0xF83DBBD2, 0x1132F9AE, 0x6DA129C7,
    0x4B2F9E1D, 0xF330B2DC, 0xEC52860D, 0xD0E3C177,
    0x6C16B32B, 0x99B970A9, 0xFA489411, 0x2264E947,
    0xC48CFCA8, 0x1A3FF0A0, 0xD82C7D56, 0xEF903322,
    0xC74E4987, 0xC1D138D9, 0xFEA2CA8C, 0x360BD498,
    0xCF81F5A6, 0x28DE7AA5, 0x268EB7DA, 0xA4BFAD3F,
    0xE49D3A2C, 0x0D927850, 0x9BCC5F6A, 0x62467E54,
    0xC2138DF6, 0xE8B8D890, 0x5EF7392E, 0xF5AFC382,
    0xBE805D9F, 0x7C93D069, 0xA92DD56F, 0xB31225CF,
    0x3B99ACC8, 0xA77D1810, 0x6E639CE8, 0x7BBB3BDB,
    0x097826CD, 0xF418596E, 0x01B79AEC, 0xA89A4F83,
    0x656E95E6, 0x7EE6FFAA, 0x08CFBC21, 0xE6E815EF,
    0xD99BE7BA, 0xCE366F4A, 0xD4099FEA, 0xD67CB029,
    0xAFB2A431, 0x31233F2A, 0x3094A5C6, 0xC066A235,
    0x37BC4E74, 0xA6CA82FC, 0xB0D090E0, 0x15D8A733,
    0x4A9804F1, 0xF7DAEC41, 0x0E50CD7F, 0x2FF69117,
    0x8DD64D76, 0x4DB0EF43, 0x544DAACC, 0xDF0496E4,
    0xE3B5D19E, 0x1B886A4C, 0xB81F2CC1, 0x7F516546,
    0x04EA5E9D, 0x5D358C01, 0x737487FA, 0x2E410BFB,
    0x5A1D67B3, 0x52D2DB92, 0x335610E9, 0x1347D66D,
    0x8C61D79A, 0x7A0CA137, 0x8E14F859, 0x893C13EB,
    0xEE27A9CE, 0x35C961B7, 0xEDE51CE1, 0x3CB1477A,
    0x59DFD29C, 0x3F73F255, 0x79CE1418, 0xBF37C773,
    0xEACDF753, 0x5BAAFD5F, 0x146F3DDF, 0x86DB4478,
    0x81F3AFCA, 0x3EC468B9, 0x2C342438, 0x5F40A3C2,
    0x72C31D16, 0x0C25E2BC, 0x8B493C28, 0x41950DFF,
    0x7101A839, 0xDEB30C08, 0x9CE4B4D8, 0x90C15664,
    0x6184CB7B, 0x70B632D5, 0x745C6C48, 0x4257B8D0,
};

static const u32 RCON[10] =
{
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x0000001B, 0x00000036
};

static inline
u32 fsb(u32 v)
{
    return u32(FSb[(v      ) & 0xff]      ) ^
           u32(FSb[(v >>  8) & 0xff] <<  8) ^
           u32(FSb[(v >> 16) & 0xff] << 16) ^
           u32(FSb[(v >> 24) & 0xff] << 24);
}

static inline
u32 fsb(u32 v, int index)
{
    return u32(FSb[(v >>  8) & 0xff]      ) ^
           u32(FSb[(v >> 16) & 0xff] <<  8) ^
           u32(FSb[(v >> 24) & 0xff] << 16) ^
           u32(FSb[(v      ) & 0xff] << 24) ^ RCON[index];
}

static
void arm_aes_setkey(u32* enc, u32* dec, const u8* key, int bits)
{

    // encoding schedule

    const int words = bits / 32;
    const int nr = words + 6;

    u32* RK = enc;

    for (int i = 0; i < words; ++i)
    {
        RK[i] = uload32le(key + i * 4);
    }

    switch (nr)
    {
        case 10:
            for (int i = 0; i < 10; ++i)
            {
                RK[4]  = RK[0] ^ fsb(RK[3], i);
                RK[5]  = RK[1] ^ RK[4];
                RK[6]  = RK[2] ^ RK[5];
                RK[7]  = RK[3] ^ RK[6];
                RK += 4;
            }
            break;

        case 12:
            for (int i = 0; i < 8; ++i)
            {
                RK[6]  = RK[0] ^ fsb(RK[5], i);
                RK[7]  = RK[1] ^ RK[6];
                RK[8]  = RK[2] ^ RK[7];
                RK[9]  = RK[3] ^ RK[8];
                RK[10] = RK[4] ^ RK[9];
                RK[11] = RK[5] ^ RK[10];
                RK += 6;
            }
            break;

        case 14:
            for (int i = 0; i < 7; ++i)
            {
                RK[8]  = RK[0] ^ fsb(RK[7], i);
                RK[9]  = RK[1] ^ RK[8];
                RK[10] = RK[2] ^ RK[9];
                RK[11] = RK[3] ^ RK[10];
                RK[12] = RK[4] ^ fsb(RK[11]);
                RK[13] = RK[5] ^ RK[12];
                RK[14] = RK[6] ^ RK[13];
                RK[15] = RK[7] ^ RK[14];
                RK += 8;
            }
            break;
    }

    // decoding schedule

    u32* SK = enc + nr * 4;
    RK = dec;

    RK[0] = SK[0];
    RK[1] = SK[1];
    RK[2] = SK[2];
    RK[3] = SK[3];
    RK += 4;
    SK -= 4;

    for (int i = nr - 1; i > 0; --i)
    {
        const u8* s = reinterpret_cast<const u8*>(SK);
        for (int j = 0; j < 4; ++j)
        {
            u32 s0 = RT[FSb[s[0]]];
            u32 s1 = RT[FSb[s[1]]];
            u32 s2 = RT[FSb[s[2]]];
            u32 s3 = RT[FSb[s[3]]];
            s1 = (s1 <<  8) | (s1 >> 24);
            s2 = (s2 << 16) | (s2 >> 16);
            s3 = (s3 << 24) | (s3 >>  8);
            *RK++ = s0 ^ s1 ^ s2 ^ s3;
            s += 4;
        }
        SK -= 4;
    }

    RK[0] = SK[0];
    RK[1] = SK[1];
    RK[2] = SK[2];
    RK[3] = SK[3];
}

// ECB encrypt

void arm_ecb128_encrypt(u8* output, const u8* input, size_t length, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesmcq_u8(vaeseq_u8(v, k0));
        v = vaesmcq_u8(vaeseq_u8(v, k1));
        v = vaesmcq_u8(vaeseq_u8(v, k2));
        v = vaesmcq_u8(vaeseq_u8(v, k3));
        v = vaesmcq_u8(vaeseq_u8(v, k4));
        v = vaesmcq_u8(vaeseq_u8(v, k5));
        v = vaesmcq_u8(vaeseq_u8(v, k6));
        v = vaesmcq_u8(vaeseq_u8(v, k7));
        v = vaesmcq_u8(vaeseq_u8(v, k8));
        v = veorq_u8(vaeseq_u8(v, k9), k10);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_ecb192_encrypt(u8* output, const u8* input, size_t length, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);
    const uint8x16_t k11 = vld1q_u8(keys + 16 * 11);
    const uint8x16_t k12 = vld1q_u8(keys + 16 * 12);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesmcq_u8(vaeseq_u8(v, k0));
        v = vaesmcq_u8(vaeseq_u8(v, k1));
        v = vaesmcq_u8(vaeseq_u8(v, k2));
        v = vaesmcq_u8(vaeseq_u8(v, k3));
        v = vaesmcq_u8(vaeseq_u8(v, k4));
        v = vaesmcq_u8(vaeseq_u8(v, k5));
        v = vaesmcq_u8(vaeseq_u8(v, k6));
        v = vaesmcq_u8(vaeseq_u8(v, k7));
        v = vaesmcq_u8(vaeseq_u8(v, k8));
        v = vaesmcq_u8(vaeseq_u8(v, k9));
        v = vaesmcq_u8(vaeseq_u8(v, k10));
        v = veorq_u8(vaeseq_u8(v, k11), k12);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_ecb256_encrypt(u8* output, const u8* input, size_t length, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);
    const uint8x16_t k11 = vld1q_u8(keys + 16 * 11);
    const uint8x16_t k12 = vld1q_u8(keys + 16 * 12);
    const uint8x16_t k13 = vld1q_u8(keys + 16 * 13);
    const uint8x16_t k14 = vld1q_u8(keys + 16 * 14);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesmcq_u8(vaeseq_u8(v, k0));
        v = vaesmcq_u8(vaeseq_u8(v, k1));
        v = vaesmcq_u8(vaeseq_u8(v, k2));
        v = vaesmcq_u8(vaeseq_u8(v, k3));
        v = vaesmcq_u8(vaeseq_u8(v, k4));
        v = vaesmcq_u8(vaeseq_u8(v, k5));
        v = vaesmcq_u8(vaeseq_u8(v, k6));
        v = vaesmcq_u8(vaeseq_u8(v, k7));
        v = vaesmcq_u8(vaeseq_u8(v, k8));
        v = vaesmcq_u8(vaeseq_u8(v, k9));
        v = vaesmcq_u8(vaeseq_u8(v, k10));
        v = vaesmcq_u8(vaeseq_u8(v, k11));
        v = vaesmcq_u8(vaeseq_u8(v, k12));
        v = veorq_u8(vaeseq_u8(v, k13), k14);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_ecb_encrypt(u8* output, const u8* input, size_t length, const u32* keys, int bits)
{
    switch (bits)
    {
        case 128:
            arm_ecb128_encrypt(output, input, length, reinterpret_cast<const u8*>(keys));
            break;
        case 192:
            arm_ecb192_encrypt(output, input, length, reinterpret_cast<const u8*>(keys));
            break;
        case 256:
            arm_ecb256_encrypt(output, input, length, reinterpret_cast<const u8*>(keys));
            break;
        default:
            break;
    }
}

// ECB decrypt

void arm_ecb128_decrypt(u8* output, const u8* input, size_t length, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesimcq_u8(vaesdq_u8(v, k0));
        v = vaesimcq_u8(vaesdq_u8(v, k1));
        v = vaesimcq_u8(vaesdq_u8(v, k2));
        v = vaesimcq_u8(vaesdq_u8(v, k3));
        v = vaesimcq_u8(vaesdq_u8(v, k4));
        v = vaesimcq_u8(vaesdq_u8(v, k5));
        v = vaesimcq_u8(vaesdq_u8(v, k6));
        v = vaesimcq_u8(vaesdq_u8(v, k7));
        v = vaesimcq_u8(vaesdq_u8(v, k8));
        v = veorq_u8(vaesdq_u8(v, k9), k10);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_ecb192_decrypt(u8* output, const u8* input, size_t length, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);
    const uint8x16_t k11 = vld1q_u8(keys + 16 * 11);
    const uint8x16_t k12 = vld1q_u8(keys + 16 * 12);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesimcq_u8(vaesdq_u8(v, k0));
        v = vaesimcq_u8(vaesdq_u8(v, k1));
        v = vaesimcq_u8(vaesdq_u8(v, k2));
        v = vaesimcq_u8(vaesdq_u8(v, k3));
        v = vaesimcq_u8(vaesdq_u8(v, k4));
        v = vaesimcq_u8(vaesdq_u8(v, k5));
        v = vaesimcq_u8(vaesdq_u8(v, k6));
        v = vaesimcq_u8(vaesdq_u8(v, k7));
        v = vaesimcq_u8(vaesdq_u8(v, k8));
        v = vaesimcq_u8(vaesdq_u8(v, k9));
        v = vaesimcq_u8(vaesdq_u8(v, k10));
        v = veorq_u8(vaesdq_u8(v, k11), k12);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_ecb256_decrypt(u8* output, const u8* input, size_t length, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);
    const uint8x16_t k11 = vld1q_u8(keys + 16 * 11);
    const uint8x16_t k12 = vld1q_u8(keys + 16 * 12);
    const uint8x16_t k13 = vld1q_u8(keys + 16 * 13);
    const uint8x16_t k14 = vld1q_u8(keys + 16 * 14);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesimcq_u8(vaesdq_u8(v, k0));
        v = vaesimcq_u8(vaesdq_u8(v, k1));
        v = vaesimcq_u8(vaesdq_u8(v, k2));
        v = vaesimcq_u8(vaesdq_u8(v, k3));
        v = vaesimcq_u8(vaesdq_u8(v, k4));
        v = vaesimcq_u8(vaesdq_u8(v, k5));
        v = vaesimcq_u8(vaesdq_u8(v, k6));
        v = vaesimcq_u8(vaesdq_u8(v, k7));
        v = vaesimcq_u8(vaesdq_u8(v, k8));
        v = vaesimcq_u8(vaesdq_u8(v, k9));
        v = vaesimcq_u8(vaesdq_u8(v, k10));
        v = vaesimcq_u8(vaesdq_u8(v, k11));
        v = vaesimcq_u8(vaesdq_u8(v, k12));
        v = veorq_u8(vaesdq_u8(v, k13), k14);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_ecb_decrypt(u8* output, const u8* input, size_t length, const u32* keys, int bits)
{
    switch (bits)
    {
        case 128:
            arm_ecb128_decrypt(output, input, length, reinterpret_cast<const u8*>(keys));
            break;
        case 192:
            arm_ecb192_decrypt(output, input, length, reinterpret_cast<const u8*>(keys));
            break;
        case 256:
            arm_ecb256_decrypt(output, input, length, reinterpret_cast<const u8*>(keys));
            break;
        default:
            break;
    }
}

// CBC encrypt

void arm_cbc128_encrypt(u8* output, const u8* input, size_t length, uint8x16_t iv, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = veorq_u8(v, iv); // NOTE: could combine with k0
        v = vaesmcq_u8(vaeseq_u8(v, k0));
        v = vaesmcq_u8(vaeseq_u8(v, k1));
        v = vaesmcq_u8(vaeseq_u8(v, k2));
        v = vaesmcq_u8(vaeseq_u8(v, k3));
        v = vaesmcq_u8(vaeseq_u8(v, k4));
        v = vaesmcq_u8(vaeseq_u8(v, k5));
        v = vaesmcq_u8(vaeseq_u8(v, k6));
        v = vaesmcq_u8(vaeseq_u8(v, k7));
        v = vaesmcq_u8(vaeseq_u8(v, k8));
        v = veorq_u8(vaeseq_u8(v, k9), k10);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_cbc192_encrypt(u8* output, const u8* input, size_t length, uint8x16_t iv, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);
    const uint8x16_t k11 = vld1q_u8(keys + 16 * 11);
    const uint8x16_t k12 = vld1q_u8(keys + 16 * 12);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = veorq_u8(v, iv); // NOTE: could combine with k0
        v = vaesmcq_u8(vaeseq_u8(v, k0));
        v = vaesmcq_u8(vaeseq_u8(v, k1));
        v = vaesmcq_u8(vaeseq_u8(v, k2));
        v = vaesmcq_u8(vaeseq_u8(v, k3));
        v = vaesmcq_u8(vaeseq_u8(v, k4));
        v = vaesmcq_u8(vaeseq_u8(v, k5));
        v = vaesmcq_u8(vaeseq_u8(v, k6));
        v = vaesmcq_u8(vaeseq_u8(v, k7));
        v = vaesmcq_u8(vaeseq_u8(v, k8));
        v = vaesmcq_u8(vaeseq_u8(v, k9));
        v = vaesmcq_u8(vaeseq_u8(v, k10));
        v = veorq_u8(vaeseq_u8(v, k11), k12);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_cbc256_encrypt(u8* output, const u8* input, size_t length, uint8x16_t iv, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);
    const uint8x16_t k11 = vld1q_u8(keys + 16 * 11);
    const uint8x16_t k12 = vld1q_u8(keys + 16 * 12);
    const uint8x16_t k13 = vld1q_u8(keys + 16 * 13);
    const uint8x16_t k14 = vld1q_u8(keys + 16 * 14);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = veorq_u8(v, iv); // NOTE: could combine with k0
        v = vaesmcq_u8(vaeseq_u8(v, k0));
        v = vaesmcq_u8(vaeseq_u8(v, k1));
        v = vaesmcq_u8(vaeseq_u8(v, k2));
        v = vaesmcq_u8(vaeseq_u8(v, k3));
        v = vaesmcq_u8(vaeseq_u8(v, k4));
        v = vaesmcq_u8(vaeseq_u8(v, k5));
        v = vaesmcq_u8(vaeseq_u8(v, k6));
        v = vaesmcq_u8(vaeseq_u8(v, k7));
        v = vaesmcq_u8(vaeseq_u8(v, k8));
        v = vaesmcq_u8(vaeseq_u8(v, k9));
        v = vaesmcq_u8(vaeseq_u8(v, k10));
        v = vaesmcq_u8(vaeseq_u8(v, k11));
        v = vaesmcq_u8(vaeseq_u8(v, k12));
        v = veorq_u8(vaeseq_u8(v, k13), k14);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_cbc_encrypt(u8* output, const u8* input, size_t length, const u8* ivec, const u32* keys, int bits)
{
    uint8x16_t iv = vld1q_u8(ivec);
    switch (bits)
    {
        case 128:
            arm_cbc128_encrypt(output, input, length, iv, reinterpret_cast<const u8*>(keys));
            break;
        case 192:
            arm_cbc192_encrypt(output, input, length, iv, reinterpret_cast<const u8*>(keys));
            break;
        case 256:
            arm_cbc256_encrypt(output, input, length, iv, reinterpret_cast<const u8*>(keys));
            break;
        default:
            break;
    }
}

// CBC decrypt

void arm_cbc128_decrypt(u8* output, const u8* input, size_t length, uint8x16_t iv, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesimcq_u8(vaesdq_u8(v, k0));
        v = vaesimcq_u8(vaesdq_u8(v, k1));
        v = vaesimcq_u8(vaesdq_u8(v, k2));
        v = vaesimcq_u8(vaesdq_u8(v, k3));
        v = vaesimcq_u8(vaesdq_u8(v, k4));
        v = vaesimcq_u8(vaesdq_u8(v, k5));
        v = vaesimcq_u8(vaesdq_u8(v, k6));
        v = vaesimcq_u8(vaesdq_u8(v, k7));
        v = vaesimcq_u8(vaesdq_u8(v, k8));
        v = veorq_u8(vaesdq_u8(v, k9), k10);
        v = veorq_u8(v, iv);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_cbc192_decrypt(u8* output, const u8* input, size_t length, uint8x16_t iv, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);
    const uint8x16_t k11 = vld1q_u8(keys + 16 * 11);
    const uint8x16_t k12 = vld1q_u8(keys + 16 * 12);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesimcq_u8(vaesdq_u8(v, k0));
        v = vaesimcq_u8(vaesdq_u8(v, k1));
        v = vaesimcq_u8(vaesdq_u8(v, k2));
        v = vaesimcq_u8(vaesdq_u8(v, k3));
        v = vaesimcq_u8(vaesdq_u8(v, k4));
        v = vaesimcq_u8(vaesdq_u8(v, k5));
        v = vaesimcq_u8(vaesdq_u8(v, k6));
        v = vaesimcq_u8(vaesdq_u8(v, k7));
        v = vaesimcq_u8(vaesdq_u8(v, k8));
        v = vaesimcq_u8(vaesdq_u8(v, k9));
        v = vaesimcq_u8(vaesdq_u8(v, k10));
        v = veorq_u8(vaesdq_u8(v, k11), k12);
        v = veorq_u8(v, iv);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_cbc256_decrypt(u8* output, const u8* input, size_t length, uint8x16_t iv, const u8* keys)
{
    const uint8x16_t k0 = vld1q_u8(keys + 16 * 0);
    const uint8x16_t k1 = vld1q_u8(keys + 16 * 1);
    const uint8x16_t k2 = vld1q_u8(keys + 16 * 2);
    const uint8x16_t k3 = vld1q_u8(keys + 16 * 3);
    const uint8x16_t k4 = vld1q_u8(keys + 16 * 4);
    const uint8x16_t k5 = vld1q_u8(keys + 16 * 5);
    const uint8x16_t k6 = vld1q_u8(keys + 16 * 6);
    const uint8x16_t k7 = vld1q_u8(keys + 16 * 7);
    const uint8x16_t k8 = vld1q_u8(keys + 16 * 8);
    const uint8x16_t k9 = vld1q_u8(keys + 16 * 9);
    const uint8x16_t k10 = vld1q_u8(keys + 16 * 10);
    const uint8x16_t k11 = vld1q_u8(keys + 16 * 11);
    const uint8x16_t k12 = vld1q_u8(keys + 16 * 12);
    const uint8x16_t k13 = vld1q_u8(keys + 16 * 13);
    const uint8x16_t k14 = vld1q_u8(keys + 16 * 14);

    while (length >= 16)
    {
        uint8x16_t v = vld1q_u8(input);
        v = vaesimcq_u8(vaesdq_u8(v, k0));
        v = vaesimcq_u8(vaesdq_u8(v, k1));
        v = vaesimcq_u8(vaesdq_u8(v, k2));
        v = vaesimcq_u8(vaesdq_u8(v, k3));
        v = vaesimcq_u8(vaesdq_u8(v, k4));
        v = vaesimcq_u8(vaesdq_u8(v, k5));
        v = vaesimcq_u8(vaesdq_u8(v, k6));
        v = vaesimcq_u8(vaesdq_u8(v, k7));
        v = vaesimcq_u8(vaesdq_u8(v, k8));
        v = vaesimcq_u8(vaesdq_u8(v, k9));
        v = vaesimcq_u8(vaesdq_u8(v, k10));
        v = vaesimcq_u8(vaesdq_u8(v, k11));
        v = vaesimcq_u8(vaesdq_u8(v, k12));
        v = veorq_u8(vaesdq_u8(v, k13), k14);
        v = veorq_u8(v, iv);
        vst1q_u8(output, v);
		input += 16;
        output += 16;
		length -= 16;
    }
}

void arm_cbc_decrypt(u8* output, const u8* input, size_t length, const u8* ivec, const u32* keys, int bits)
{
    uint8x16_t iv = vld1q_u8(ivec);
    switch (bits)
    {
        case 128:
            arm_cbc128_decrypt(output, input, length, iv, reinterpret_cast<const u8*>(keys));
            break;
        case 192:
            arm_cbc192_decrypt(output, input, length, iv, reinterpret_cast<const u8*>(keys));
            break;
        case 256:
            arm_cbc256_decrypt(output, input, length, iv, reinterpret_cast<const u8*>(keys));
            break;
        default:
            break;
    }
}

#endif // __ARM_FEATURE_CRYPTO

#if defined(MANGO_ENABLE_AES)

// ----------------------------------------------------------------------------------------
// Intel AES-NI
// ----------------------------------------------------------------------------------------

#define aesni_shuffle_epi64(a, b, s) \
    (__m128i) _mm_shuffle_pd((__m128d)a, (__m128d)b, s)

template <int R>
inline __m128i aesni_key128_expand(__m128i key)
{
    __m128i temp = _mm_aeskeygenassist_si128(key, R);
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	return _mm_xor_si128(key, _mm_shuffle_epi32(temp, 0xff));
}

template <int R>
inline void aesni_key192_expand(__m128i* temp1, __m128i* temp2)
{
    __m128i temp3 = *temp1;
    __m128i temp4 = _mm_aeskeygenassist_si128(*temp2, R);
    *temp1 = _mm_xor_si128(*temp1, _mm_slli_si128(temp3, 0x4));
    *temp1 = _mm_xor_si128(*temp1, _mm_slli_si128(temp3, 0x8));
    *temp1 = _mm_xor_si128(*temp1, _mm_slli_si128(temp3, 0xc));
    *temp1 = _mm_xor_si128(*temp1, _mm_shuffle_epi32(temp4, 0x55));
    *temp2 = _mm_xor_si128(*temp2, _mm_slli_si128(*temp2, 0x4));
    *temp2 = _mm_xor_si128(*temp2, _mm_shuffle_epi32(*temp1, 0xff));
}

template <int R, int S>
inline __m128i aesni_key256_expand(__m128i temp1, __m128i temp2)
{
    __m128i temp3 = temp1;
    __m128i temp4 = _mm_aeskeygenassist_si128(temp2, R);
    temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp3, 0x4));
    temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp3, 0x8));
    temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp3, 0xc));
    temp1 = _mm_xor_si128(temp1, _mm_shuffle_epi32(temp4, S));
    return temp1;
}

void aesni_key128_expand(__m128i* schedule, const u8* key)
{
    // encryption schedule
    schedule[0]  = _mm_loadu_si128(reinterpret_cast<const __m128i *> (key));
	schedule[1]  = aesni_key128_expand<0x01>(schedule[0]);
	schedule[2]  = aesni_key128_expand<0x02>(schedule[1]);
	schedule[3]  = aesni_key128_expand<0x04>(schedule[2]);
	schedule[4]  = aesni_key128_expand<0x08>(schedule[3]);
	schedule[5]  = aesni_key128_expand<0x10>(schedule[4]);
	schedule[6]  = aesni_key128_expand<0x20>(schedule[5]);
	schedule[7]  = aesni_key128_expand<0x40>(schedule[6]);
	schedule[8]  = aesni_key128_expand<0x80>(schedule[7]);
	schedule[9]  = aesni_key128_expand<0x1b>(schedule[8]);
	schedule[10] = aesni_key128_expand<0x36>(schedule[9]);

    // decryption schedule
    schedule[11] = _mm_aesimc_si128(schedule[9]);
    schedule[12] = _mm_aesimc_si128(schedule[8]);
    schedule[13] = _mm_aesimc_si128(schedule[7]);
    schedule[14] = _mm_aesimc_si128(schedule[6]);
    schedule[15] = _mm_aesimc_si128(schedule[5]);
    schedule[16] = _mm_aesimc_si128(schedule[4]);
    schedule[17] = _mm_aesimc_si128(schedule[3]);
    schedule[18] = _mm_aesimc_si128(schedule[2]);
    schedule[19] = _mm_aesimc_si128(schedule[1]);
}

void aesni_key192_expand(__m128i* schedule, const u8* key)
{
    // encryption schedule
    __m128i temp1 = _mm_loadu_si128(reinterpret_cast<const __m128i *> (key + 0));
    __m128i temp2 = _mm_set_epi64x(0, *reinterpret_cast<const u64 *> (key + 16));

    schedule[0] = temp1;
    schedule[1] = temp2;
    aesni_key192_expand<0x1>(&temp1, &temp2);
    schedule[1] = aesni_shuffle_epi64(schedule[1], temp1, 0);
    schedule[2] = aesni_shuffle_epi64(temp1, temp2, 1);
    aesni_key192_expand<0x2>(&temp1, &temp2);
    schedule[3] = temp1;
    schedule[4] = temp2;
    aesni_key192_expand<0x4>(&temp1, &temp2);
    schedule[4] = aesni_shuffle_epi64(schedule[4], temp1, 0);
    schedule[5] = aesni_shuffle_epi64(temp1, temp2, 1);
    aesni_key192_expand<0x8>(&temp1, &temp2);
    schedule[6] = temp1;
    schedule[7] = temp2;
    aesni_key192_expand<0x10>(&temp1, &temp2);
    schedule[7] = aesni_shuffle_epi64(schedule[7], temp1, 0);
    schedule[8] = aesni_shuffle_epi64(temp1, temp2, 1);
    aesni_key192_expand<0x20>(&temp1, &temp2);
    schedule[9]  = temp1;
    schedule[10] = temp2;
    aesni_key192_expand<0x40>(&temp1, &temp2);
    schedule[10] = aesni_shuffle_epi64(schedule[10], temp1, 0);
    schedule[11] = aesni_shuffle_epi64(temp1, temp2, 1);
    aesni_key192_expand<0x80>(&temp1, &temp2);
    schedule[12] = temp1;

    // decryption schedule
    schedule[13] = _mm_aesimc_si128(schedule[11]);
    schedule[14] = _mm_aesimc_si128(schedule[10]);
    schedule[15] = _mm_aesimc_si128(schedule[9]);
    schedule[16] = _mm_aesimc_si128(schedule[8]);
    schedule[17] = _mm_aesimc_si128(schedule[7]);
    schedule[18] = _mm_aesimc_si128(schedule[6]);
    schedule[19] = _mm_aesimc_si128(schedule[5]);
    schedule[20] = _mm_aesimc_si128(schedule[4]);
    schedule[21] = _mm_aesimc_si128(schedule[3]);
    schedule[22] = _mm_aesimc_si128(schedule[2]);
    schedule[23] = _mm_aesimc_si128(schedule[1]);
}

void aesni_key256_expand(__m128i* schedule, const u8* key)
{
    // encryption schedule
    schedule[ 0] = _mm_loadu_si128(reinterpret_cast<const __m128i *>(key + 0));
    schedule[ 1] = _mm_loadu_si128(reinterpret_cast<const __m128i *>(key + 16));
    schedule[ 2] = aesni_key256_expand<0x01, 0xff>(schedule[ 0], schedule[ 1]);
    schedule[ 3] = aesni_key256_expand<0x00, 0xaa>(schedule[ 1], schedule[ 2]);
    schedule[ 4] = aesni_key256_expand<0x02, 0xff>(schedule[ 2], schedule[ 3]);
    schedule[ 5] = aesni_key256_expand<0x00, 0xaa>(schedule[ 3], schedule[ 4]);
    schedule[ 6] = aesni_key256_expand<0x04, 0xff>(schedule[ 4], schedule[ 5]);
    schedule[ 7] = aesni_key256_expand<0x00, 0xaa>(schedule[ 5], schedule[ 6]);
    schedule[ 8] = aesni_key256_expand<0x08, 0xff>(schedule[ 6], schedule[ 7]);
    schedule[ 9] = aesni_key256_expand<0x00, 0xaa>(schedule[ 7], schedule[ 8]);
    schedule[10] = aesni_key256_expand<0x10, 0xff>(schedule[ 8], schedule[ 9]);
    schedule[11] = aesni_key256_expand<0x00, 0xaa>(schedule[ 9], schedule[10]);
    schedule[12] = aesni_key256_expand<0x20, 0xff>(schedule[10], schedule[11]);
    schedule[13] = aesni_key256_expand<0x00, 0xaa>(schedule[11], schedule[12]);
    schedule[14] = aesni_key256_expand<0x40, 0xff>(schedule[12], schedule[13]);

    // decryption schedule
    schedule[15] = _mm_aesimc_si128(schedule[13]);
    schedule[16] = _mm_aesimc_si128(schedule[12]);
    schedule[17] = _mm_aesimc_si128(schedule[11]);
    schedule[18] = _mm_aesimc_si128(schedule[10]);
    schedule[19] = _mm_aesimc_si128(schedule[9]);
    schedule[20] = _mm_aesimc_si128(schedule[8]);
    schedule[21] = _mm_aesimc_si128(schedule[7]);
    schedule[22] = _mm_aesimc_si128(schedule[6]);
    schedule[23] = _mm_aesimc_si128(schedule[5]);
    schedule[24] = _mm_aesimc_si128(schedule[4]);
    schedule[25] = _mm_aesimc_si128(schedule[3]);
    schedule[26] = _mm_aesimc_si128(schedule[2]);
    schedule[27] = _mm_aesimc_si128(schedule[1]);
}

void aesni_key_expand(__m128i* schedule, const u8* key, int bits)
{
    switch (bits)
    {
        case 128:
            aesni_key128_expand(schedule, key);
            break;
        case 192:
            aesni_key192_expand(schedule, key);
            break;
        case 256:
            aesni_key256_expand(schedule, key);
            break;
        default:
            break;
    }
}

// ECB encrypt

void aesni_ecb128_encrypt(u8* output, const u8* input, size_t length, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k1 = schedule[1];
    const __m128i k2 = schedule[2];
    const __m128i k3 = schedule[3];
    const __m128i k4 = schedule[4];
    const __m128i k5 = schedule[5];
    const __m128i k6 = schedule[6];
    const __m128i k7 = schedule[7];
    const __m128i k8 = schedule[8];
    const __m128i k9 = schedule[9];
    const __m128i k10 = schedule[10];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k0);
        data = _mm_aesenc_si128(data, k1);
        data = _mm_aesenc_si128(data, k2);
        data = _mm_aesenc_si128(data, k3);
        data = _mm_aesenc_si128(data, k4);
        data = _mm_aesenc_si128(data, k5);
        data = _mm_aesenc_si128(data, k6);
        data = _mm_aesenc_si128(data, k7);
        data = _mm_aesenc_si128(data, k8);
        data = _mm_aesenc_si128(data, k9);
        data = _mm_aesenclast_si128(data, k10);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_ecb192_encrypt(u8* output, const u8* input, size_t length, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k1 = schedule[1];
    const __m128i k2 = schedule[2];
    const __m128i k3 = schedule[3];
    const __m128i k4 = schedule[4];
    const __m128i k5 = schedule[5];
    const __m128i k6 = schedule[6];
    const __m128i k7 = schedule[7];
    const __m128i k8 = schedule[8];
    const __m128i k9 = schedule[9];
    const __m128i k10 = schedule[10];
    const __m128i k11 = schedule[11];
    const __m128i k12 = schedule[12];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k0);
        data = _mm_aesenc_si128(data, k1);
        data = _mm_aesenc_si128(data, k2);
        data = _mm_aesenc_si128(data, k3);
        data = _mm_aesenc_si128(data, k4);
        data = _mm_aesenc_si128(data, k5);
        data = _mm_aesenc_si128(data, k6);
        data = _mm_aesenc_si128(data, k7);
        data = _mm_aesenc_si128(data, k8);
        data = _mm_aesenc_si128(data, k9);
        data = _mm_aesenc_si128(data, k10);
        data = _mm_aesenc_si128(data, k11);
        data = _mm_aesenclast_si128(data, k12);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_ecb256_encrypt(u8* output, const u8* input, size_t length, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k1 = schedule[1];
    const __m128i k2 = schedule[2];
    const __m128i k3 = schedule[3];
    const __m128i k4 = schedule[4];
    const __m128i k5 = schedule[5];
    const __m128i k6 = schedule[6];
    const __m128i k7 = schedule[7];
    const __m128i k8 = schedule[8];
    const __m128i k9 = schedule[9];
    const __m128i k10 = schedule[10];
    const __m128i k11 = schedule[11];
    const __m128i k12 = schedule[12];
    const __m128i k13 = schedule[13];
    const __m128i k14 = schedule[14];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k0);
        data = _mm_aesenc_si128(data, k1);
        data = _mm_aesenc_si128(data, k2);
        data = _mm_aesenc_si128(data, k3);
        data = _mm_aesenc_si128(data, k4);
        data = _mm_aesenc_si128(data, k5);
        data = _mm_aesenc_si128(data, k6);
        data = _mm_aesenc_si128(data, k7);
        data = _mm_aesenc_si128(data, k8);
        data = _mm_aesenc_si128(data, k9);
        data = _mm_aesenc_si128(data, k10);
        data = _mm_aesenc_si128(data, k11);
        data = _mm_aesenc_si128(data, k12);
        data = _mm_aesenc_si128(data, k13);
        data = _mm_aesenclast_si128(data, k14);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_ecb_encrypt(u8* output, const u8* input, size_t length, const __m128i* schedule, int keybits)
{
    switch (keybits)
    {
        case 128:
            aesni_ecb128_encrypt(output, input, length, schedule);
            break;
        case 192:
            aesni_ecb192_encrypt(output, input, length, schedule);
            break;
        case 256:
            aesni_ecb256_encrypt(output, input, length, schedule);
            break;
        default:
            break;
    }
}

// ECB decrypt

void aesni_ecb128_decrypt(u8* output, const u8* input, size_t length, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k10 = schedule[10];
    const __m128i k11 = schedule[11];
    const __m128i k12 = schedule[12];
    const __m128i k13 = schedule[13];
    const __m128i k14 = schedule[14];
    const __m128i k15 = schedule[15];
    const __m128i k16 = schedule[16];
    const __m128i k17 = schedule[17];
    const __m128i k18 = schedule[18];
    const __m128i k19 = schedule[19];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k10);
        data = _mm_aesdec_si128(data, k11);
        data = _mm_aesdec_si128(data, k12);
        data = _mm_aesdec_si128(data, k13);
        data = _mm_aesdec_si128(data, k14);
        data = _mm_aesdec_si128(data, k15);
        data = _mm_aesdec_si128(data, k16);
        data = _mm_aesdec_si128(data, k17);
        data = _mm_aesdec_si128(data, k18);
        data = _mm_aesdec_si128(data, k19);
        data = _mm_aesdeclast_si128(data, k0);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_ecb192_decrypt(u8* output, const u8* input, size_t length, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k12 = schedule[12];
    const __m128i k13 = schedule[13];
    const __m128i k14 = schedule[14];
    const __m128i k15 = schedule[15];
    const __m128i k16 = schedule[16];
    const __m128i k17 = schedule[17];
    const __m128i k18 = schedule[18];
    const __m128i k19 = schedule[19];
    const __m128i k20 = schedule[20];
    const __m128i k21 = schedule[21];
    const __m128i k22 = schedule[22];
    const __m128i k23 = schedule[23];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k12);
        data = _mm_aesdec_si128(data, k13);
        data = _mm_aesdec_si128(data, k14);
        data = _mm_aesdec_si128(data, k15);
        data = _mm_aesdec_si128(data, k16);
        data = _mm_aesdec_si128(data, k17);
        data = _mm_aesdec_si128(data, k18);
        data = _mm_aesdec_si128(data, k19);
        data = _mm_aesdec_si128(data, k20);
        data = _mm_aesdec_si128(data, k21);
        data = _mm_aesdec_si128(data, k22);
        data = _mm_aesdec_si128(data, k23);
        data = _mm_aesdeclast_si128(data, k0);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_ecb256_decrypt(u8* output, const u8* input, size_t length, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k14 = schedule[14];
    const __m128i k15 = schedule[15];
    const __m128i k16 = schedule[16];
    const __m128i k17 = schedule[17];
    const __m128i k18 = schedule[18];
    const __m128i k19 = schedule[19];
    const __m128i k20 = schedule[20];
    const __m128i k21 = schedule[21];
    const __m128i k22 = schedule[22];
    const __m128i k23 = schedule[23];
    const __m128i k24 = schedule[24];
    const __m128i k25 = schedule[25];
    const __m128i k26 = schedule[26];
    const __m128i k27 = schedule[27];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k14);
        data = _mm_aesdec_si128(data, k15);
        data = _mm_aesdec_si128(data, k16);
        data = _mm_aesdec_si128(data, k17);
        data = _mm_aesdec_si128(data, k18);
        data = _mm_aesdec_si128(data, k19);
        data = _mm_aesdec_si128(data, k20);
        data = _mm_aesdec_si128(data, k21);
        data = _mm_aesdec_si128(data, k22);
        data = _mm_aesdec_si128(data, k23);
        data = _mm_aesdec_si128(data, k24);
        data = _mm_aesdec_si128(data, k25);
        data = _mm_aesdec_si128(data, k26);
        data = _mm_aesdec_si128(data, k27);
        data = _mm_aesdeclast_si128(data, k0);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_ecb_decrypt(u8* output, const u8* input, size_t length, const __m128i* schedule, int keybits)
{
    switch (keybits)
    {
        case 128:
            aesni_ecb128_decrypt(output, input, length, schedule);
            break;
        case 192:
            aesni_ecb192_decrypt(output, input, length, schedule);
            break;
        case 256:
            aesni_ecb256_decrypt(output, input, length, schedule);
            break;
        default:
            break;
    }
}

// CBC encrypt

void aesni_cbc128_encrypt(u8* output, const u8* input, size_t length, __m128i iv, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k1 = schedule[1];
    const __m128i k2 = schedule[2];
    const __m128i k3 = schedule[3];
    const __m128i k4 = schedule[4];
    const __m128i k5 = schedule[5];
    const __m128i k6 = schedule[6];
    const __m128i k7 = schedule[7];
    const __m128i k8 = schedule[8];
    const __m128i k9 = schedule[9];
    const __m128i k10 = schedule[10];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, iv); // NOTE: could combine with k0
        data = _mm_xor_si128(data, k0);
        data = _mm_aesenc_si128(data, k1);
        data = _mm_aesenc_si128(data, k2);
        data = _mm_aesenc_si128(data, k3);
        data = _mm_aesenc_si128(data, k4);
        data = _mm_aesenc_si128(data, k5);
        data = _mm_aesenc_si128(data, k6);
        data = _mm_aesenc_si128(data, k7);
        data = _mm_aesenc_si128(data, k8);
        data = _mm_aesenc_si128(data, k9);
        data = _mm_aesenclast_si128(data, k10);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_cbc192_encrypt(u8* output, const u8* input, size_t length, __m128i iv, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k1 = schedule[1];
    const __m128i k2 = schedule[2];
    const __m128i k3 = schedule[3];
    const __m128i k4 = schedule[4];
    const __m128i k5 = schedule[5];
    const __m128i k6 = schedule[6];
    const __m128i k7 = schedule[7];
    const __m128i k8 = schedule[8];
    const __m128i k9 = schedule[9];
    const __m128i k10 = schedule[10];
    const __m128i k11 = schedule[11];
    const __m128i k12 = schedule[12];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, iv); // NOTE: could combine with k0
        data = _mm_xor_si128(data, k0);
        data = _mm_aesenc_si128(data, k1);
        data = _mm_aesenc_si128(data, k2);
        data = _mm_aesenc_si128(data, k3);
        data = _mm_aesenc_si128(data, k4);
        data = _mm_aesenc_si128(data, k5);
        data = _mm_aesenc_si128(data, k6);
        data = _mm_aesenc_si128(data, k7);
        data = _mm_aesenc_si128(data, k8);
        data = _mm_aesenc_si128(data, k9);
        data = _mm_aesenc_si128(data, k10);
        data = _mm_aesenc_si128(data, k11);
        data = _mm_aesenclast_si128(data, k12);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_cbc256_encrypt(u8* output, const u8* input, size_t length, __m128i iv, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k1 = schedule[1];
    const __m128i k2 = schedule[2];
    const __m128i k3 = schedule[3];
    const __m128i k4 = schedule[4];
    const __m128i k5 = schedule[5];
    const __m128i k6 = schedule[6];
    const __m128i k7 = schedule[7];
    const __m128i k8 = schedule[8];
    const __m128i k9 = schedule[9];
    const __m128i k10 = schedule[10];
    const __m128i k11 = schedule[11];
    const __m128i k12 = schedule[12];
    const __m128i k13 = schedule[13];
    const __m128i k14 = schedule[14];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, iv); // NOTE: could combine with k0
        data = _mm_xor_si128(data, k0);
        data = _mm_aesenc_si128(data, k1);
        data = _mm_aesenc_si128(data, k2);
        data = _mm_aesenc_si128(data, k3);
        data = _mm_aesenc_si128(data, k4);
        data = _mm_aesenc_si128(data, k5);
        data = _mm_aesenc_si128(data, k6);
        data = _mm_aesenc_si128(data, k7);
        data = _mm_aesenc_si128(data, k8);
        data = _mm_aesenc_si128(data, k9);
        data = _mm_aesenc_si128(data, k10);
        data = _mm_aesenc_si128(data, k11);
        data = _mm_aesenc_si128(data, k12);
        data = _mm_aesenc_si128(data, k13);
        data = _mm_aesenclast_si128(data, k14);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_cbc_encrypt(u8* output, const u8* input, size_t length, const u8* ivec, const __m128i* schedule, int keybits)
{
    __m128i iv = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ivec));
    switch (keybits)
    {
        case 128:
            aesni_cbc128_encrypt(output, input, length, iv, schedule);
            break;
        case 192:
            aesni_cbc192_encrypt(output, input, length, iv, schedule);
            break;
        case 256:
            aesni_cbc256_encrypt(output, input, length, iv, schedule);
            break;
        default:
            break;
    }
}

// CBC decrypt

void aesni_cbc128_decrypt(u8* output, const u8* input, size_t length, __m128i iv, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k10 = schedule[10];
    const __m128i k11 = schedule[11];
    const __m128i k12 = schedule[12];
    const __m128i k13 = schedule[13];
    const __m128i k14 = schedule[14];
    const __m128i k15 = schedule[15];
    const __m128i k16 = schedule[16];
    const __m128i k17 = schedule[17];
    const __m128i k18 = schedule[18];
    const __m128i k19 = schedule[19];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k10);
        data = _mm_aesdec_si128(data, k11);
        data = _mm_aesdec_si128(data, k12);
        data = _mm_aesdec_si128(data, k13);
        data = _mm_aesdec_si128(data, k14);
        data = _mm_aesdec_si128(data, k15);
        data = _mm_aesdec_si128(data, k16);
        data = _mm_aesdec_si128(data, k17);
        data = _mm_aesdec_si128(data, k18);
        data = _mm_aesdec_si128(data, k19);
        data = _mm_aesdeclast_si128(data, k0);
        data = _mm_xor_si128(data, iv);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_cbc192_decrypt(u8* output, const u8* input, size_t length, __m128i iv, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k12 = schedule[12];
    const __m128i k13 = schedule[13];
    const __m128i k14 = schedule[14];
    const __m128i k15 = schedule[15];
    const __m128i k16 = schedule[16];
    const __m128i k17 = schedule[17];
    const __m128i k18 = schedule[18];
    const __m128i k19 = schedule[19];
    const __m128i k20 = schedule[20];
    const __m128i k21 = schedule[21];
    const __m128i k22 = schedule[22];
    const __m128i k23 = schedule[23];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k12);
        data = _mm_aesdec_si128(data, k13);
        data = _mm_aesdec_si128(data, k14);
        data = _mm_aesdec_si128(data, k15);
        data = _mm_aesdec_si128(data, k16);
        data = _mm_aesdec_si128(data, k17);
        data = _mm_aesdec_si128(data, k18);
        data = _mm_aesdec_si128(data, k19);
        data = _mm_aesdec_si128(data, k20);
        data = _mm_aesdec_si128(data, k21);
        data = _mm_aesdec_si128(data, k22);
        data = _mm_aesdec_si128(data, k23);
        data = _mm_aesdeclast_si128(data, k0);
        data = _mm_xor_si128(data, iv);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_cbc256_decrypt(u8* output, const u8* input, size_t length, __m128i iv, const __m128i* schedule)
{
    const __m128i k0 = schedule[0];
    const __m128i k14 = schedule[14];
    const __m128i k15 = schedule[15];
    const __m128i k16 = schedule[16];
    const __m128i k17 = schedule[17];
    const __m128i k18 = schedule[18];
    const __m128i k19 = schedule[19];
    const __m128i k20 = schedule[20];
    const __m128i k21 = schedule[21];
    const __m128i k22 = schedule[22];
    const __m128i k23 = schedule[23];
    const __m128i k24 = schedule[24];
    const __m128i k25 = schedule[25];
    const __m128i k26 = schedule[26];
    const __m128i k27 = schedule[27];

    while (length >= 16)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input));
        data = _mm_xor_si128(data, k14);
        data = _mm_aesdec_si128(data, k15);
        data = _mm_aesdec_si128(data, k16);
        data = _mm_aesdec_si128(data, k17);
        data = _mm_aesdec_si128(data, k18);
        data = _mm_aesdec_si128(data, k19);
        data = _mm_aesdec_si128(data, k20);
        data = _mm_aesdec_si128(data, k21);
        data = _mm_aesdec_si128(data, k22);
        data = _mm_aesdec_si128(data, k23);
        data = _mm_aesdec_si128(data, k24);
        data = _mm_aesdec_si128(data, k25);
        data = _mm_aesdec_si128(data, k26);
        data = _mm_aesdec_si128(data, k27);
        data = _mm_aesdeclast_si128(data, k0);
        data = _mm_xor_si128(data, iv);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output), data);
        output += 16;
        input += 16;
        length -= 16;
    }
}

void aesni_cbc_decrypt(u8* output, const u8* input, size_t length, const u8* ivec, const __m128i* schedule, int keybits)
{
    __m128i iv = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ivec));
    switch (keybits)
    {
        case 128:
            aesni_cbc128_decrypt(output, input, length, iv, schedule);
            break;
        case 192:
            aesni_cbc192_decrypt(output, input, length, iv, schedule);
            break;
        case 256:
            aesni_cbc256_decrypt(output, input, length, iv, schedule);
            break;
        default:
            break;
    }
}

#endif // defined(MANGO_ENABLE_AES)

} // namespace

namespace mango
{

struct KeyScheduleAES
{

#if defined(MANGO_ENABLE_AES)
    __m128i aesni_schedule[28];
    bool aesni_supported;
#endif

#if defined(__ARM_FEATURE_CRYPTO)
    u32 arm_encode_schedule[60];
    u32 arm_decode_schedule[60];
#endif

    u32 schedule[60];
};

AES::AES(const u8* key, int bits)
    : m_schedule(new KeyScheduleAES())
    , m_bits(bits)
{
    // check key length
    switch (bits)
    {
        case 128:
        case 192:
        case 256:
            break;
        default:
            MANGO_EXCEPTION("[AES] Incorrect encryption key length: %d", bits);
            break;
    }

#if defined(MANGO_ENABLE_AES)
    m_schedule->aesni_supported = (getCPUFlags() & INTEL_AES) != 0;
    if (m_schedule->aesni_supported)
    {
        aesni_key_expand(m_schedule->aesni_schedule, key, bits);
    }
#endif

#if defined(__ARM_FEATURE_CRYPTO)
    arm_aes_setkey(m_schedule->arm_encode_schedule,
                   m_schedule->arm_decode_schedule, key, bits);
#endif

    aes_key_setup(key, m_schedule->schedule, bits);
}

AES::~AES()
{
    delete m_schedule;
}

void AES::ecb_block_encrypt(u8* output, const u8* input, size_t length)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

#if defined(MANGO_ENABLE_AES)
    if (m_schedule->aesni_supported)
    {
        aesni_ecb_encrypt(output, input, length, m_schedule->aesni_schedule, m_bits);
    }
    else
#endif
#if defined(__ARM_FEATURE_CRYPTO)
    if (true)
    {
        arm_ecb_encrypt(output, input, length, m_schedule->arm_encode_schedule, m_bits);
    }
    else
#endif
    {
        for (size_t i = 0; i < length; i += 16)
        {
            aes_encrypt(input + i, output + i, m_schedule->schedule, m_bits);
        }
    }
}

void AES::ecb_block_decrypt(u8* output, const u8* input, size_t length)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

#if defined(MANGO_ENABLE_AES)
    if (m_schedule->aesni_supported)
    {
        aesni_ecb_decrypt(output, input, length, m_schedule->aesni_schedule, m_bits);
    }
    else
#endif
#if defined(__ARM_FEATURE_CRYPTO)
    if (true)
    {
        arm_ecb_decrypt(output, input, length, m_schedule->arm_decode_schedule, m_bits);
    }
    else
#endif
    {
        for (size_t i = 0; i < length; i += 16)
        {
            aes_decrypt(input + i, output + i, m_schedule->schedule, m_bits);
        }
    }
}

void AES::cbc_block_encrypt(u8* output, const u8* input, size_t length, const u8* iv)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

#if defined(MANGO_ENABLE_AES)
    if (m_schedule->aesni_supported)
    {
        aesni_cbc_encrypt(output, input, length, iv, m_schedule->aesni_schedule, m_bits);
    }
    else
#endif
#if defined(__ARM_FEATURE_CRYPTO)
    if (true)
    {
        arm_cbc_encrypt(output, input, length, iv, m_schedule->arm_encode_schedule, m_bits);
    }
    else
#endif
    {
        aes_encrypt_cbc(input, length, output, m_schedule->schedule, m_bits, iv);
    }
}

void AES::cbc_block_decrypt(u8* output, const u8* input, size_t length, const u8* iv)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

#if defined(MANGO_ENABLE_AES)
    if (m_schedule->aesni_supported)
    {
        aesni_cbc_decrypt(output, input, length, iv, m_schedule->aesni_schedule, m_bits);
    }
    else
#endif
#if defined(__ARM_FEATURE_CRYPTO)
    if (true)
    {
        arm_cbc_decrypt(output, input, length, iv, m_schedule->arm_decode_schedule, m_bits);
    }
    else
#endif
    {
        aes_decrypt_cbc(input, length, output, m_schedule->schedule, m_bits, iv);
    }
}

void AES::ctr_block_encrypt(u8* output, const u8* input, size_t length, const u8* iv)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

    aes_encrypt_ctr(input, length, output, m_schedule->schedule, m_bits, iv);
}

void AES::ctr_block_decrypt(u8* output, const u8* input, size_t length, const u8* iv)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

    aes_decrypt_ctr(input, length, output, m_schedule->schedule, m_bits, iv);
}

void AES::ccm_block_encrypt(Memory output, ConstMemory input, ConstMemory associated, ConstMemory nonce, int mac_length)
{
    aes_u32 cipher_length = aes_u32(output.size);
    if (cipher_length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

    aes_encrypt_ccm(input.address, aes_u32(input.size),
                    associated.address, u16(associated.size),
                    nonce.address, u16(nonce.size),
                    output.address, &cipher_length, mac_length,
                    m_schedule->schedule, m_bits);
}

void AES::ccm_block_decrypt(Memory output, ConstMemory input, ConstMemory associated, ConstMemory nonce, int mac_length)
{
    aes_u32 plaintext_length = aes_u32(output.size);
    if (plaintext_length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

    int mac_authorized = 0;
    aes_decrypt_ccm(input.address, aes_u32(input.size),
                    associated.address, u16(associated.size),
                    nonce.address, u16(nonce.size),
                    output.address, &plaintext_length,
                    mac_length, &mac_authorized,
                    m_schedule->schedule, m_bits);
}

void AES::ecb_encrypt(u8* output, const u8* input, size_t length)
{
    const size_t blocks = length / 16;
    const size_t left = length % 16;
    ecb_block_encrypt(output, input, blocks * 16);
    if (left)
    {
        u8 temp[16] = { 0 };
        std::memcpy(temp, input + blocks * 16, left);

        u8 result[16];
        ecb_block_encrypt(result, temp, 16);
        std::memcpy(output + blocks * 16, result, left);
    }
}

void AES::ecb_decrypt(u8* output, const u8* input, size_t length)
{
    const size_t blocks = length / 16;
    const size_t left = length % 16;
    ecb_block_decrypt(output, input, blocks * 16);
    if (left)
    {
        u8 temp[16] = { 0 };
        std::memcpy(temp, input + blocks * 16, left);

        u8 result[16];
        ecb_block_decrypt(result, temp, 16);
        std::memcpy(output + blocks * 16, result, left);
    }
}

} // namespace mango
