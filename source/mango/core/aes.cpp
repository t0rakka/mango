/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/aes.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/exception.hpp>
#include "../../external/aes/bc_aes.h"

namespace
{
    using namespace mango;

#if defined(MANGO_ENABLE_AES)

// ----------------------------------------------------------------------------------------
// Intel AES-NI
// ----------------------------------------------------------------------------------------

#define aesni_shuffle_epi64(a, b, s) \
    (__m128i) _mm_shuffle_pd((__m128d)a, (__m128d)b, s)

template <int R>
inline __m128i aesni128_key_expand(__m128i key)
{
    __m128i temp = _mm_aeskeygenassist_si128(key, R);
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	return _mm_xor_si128(key, _mm_shuffle_epi32(temp, 0xff));
}

template <int R>
inline void aesni192_key_expand(__m128i* temp1, __m128i* temp2)
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
inline __m128i aesni256_key_expand(__m128i temp1, __m128i temp2)
{
    __m128i temp3 = temp1;
    __m128i temp4 = _mm_aeskeygenassist_si128(temp2, R);
    temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp3, 0x4));
    temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp3, 0x8));
    temp1 = _mm_xor_si128(temp1, _mm_slli_si128(temp3, 0xc));
    temp1 = _mm_xor_si128(temp1, _mm_shuffle_epi32(temp4, S));
    return temp1;
}

void aesni128_key_expand(__m128i* schedule, const u8* key)
{
    // encryption schedule
    schedule[0]  = _mm_loadu_si128(reinterpret_cast<const __m128i *> (key));
	schedule[1]  = aesni128_key_expand<0x01>(schedule[0]);
	schedule[2]  = aesni128_key_expand<0x02>(schedule[1]);
	schedule[3]  = aesni128_key_expand<0x04>(schedule[2]);
	schedule[4]  = aesni128_key_expand<0x08>(schedule[3]);
	schedule[5]  = aesni128_key_expand<0x10>(schedule[4]);
	schedule[6]  = aesni128_key_expand<0x20>(schedule[5]);
	schedule[7]  = aesni128_key_expand<0x40>(schedule[6]);
	schedule[8]  = aesni128_key_expand<0x80>(schedule[7]);
	schedule[9]  = aesni128_key_expand<0x1b>(schedule[8]);
	schedule[10] = aesni128_key_expand<0x36>(schedule[9]);

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

void aesni192_key_expand(__m128i* schedule, const u8* key)
{
    // encryption schedule
    __m128i temp1 = _mm_loadu_si128(reinterpret_cast<const __m128i *> (key + 0));
    __m128i temp2 = _mm_set_epi64x(0, *reinterpret_cast<const u64 *> (key + 16));

    schedule[0] = temp1;
    schedule[1] = temp2;
    aesni192_key_expand<0x1>(&temp1, &temp2);
    schedule[1] = aesni_shuffle_epi64(schedule[1], temp1, 0);
    schedule[2] = aesni_shuffle_epi64(temp1, temp2, 1);
    aesni192_key_expand<0x2>(&temp1, &temp2);
    schedule[3] = temp1;
    schedule[4] = temp2;
    aesni192_key_expand<0x4>(&temp1, &temp2);
    schedule[4] = aesni_shuffle_epi64(schedule[4], temp1, 0);
    schedule[5] = aesni_shuffle_epi64(temp1, temp2, 1);
    aesni192_key_expand<0x8>(&temp1, &temp2);
    schedule[6] = temp1;
    schedule[7] = temp2;
    aesni192_key_expand<0x10>(&temp1, &temp2);
    schedule[7] = aesni_shuffle_epi64(schedule[7], temp1, 0);
    schedule[8] = aesni_shuffle_epi64(temp1, temp2, 1);
    aesni192_key_expand<0x20>(&temp1, &temp2);
    schedule[9]  = temp1;
    schedule[10] = temp2;
    aesni192_key_expand<0x40>(&temp1, &temp2);
    schedule[10] = aesni_shuffle_epi64(schedule[10], temp1, 0);
    schedule[11] = aesni_shuffle_epi64(temp1, temp2, 1);
    aesni192_key_expand<0x80>(&temp1, &temp2);
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

void aesni256_key_expand(__m128i* schedule, const u8* key)
{
    // encryption schedule
    schedule[ 0] = _mm_loadu_si128(reinterpret_cast<const __m128i *>(key + 0));
    schedule[ 1] = _mm_loadu_si128(reinterpret_cast<const __m128i *>(key + 16));
    schedule[ 2] = aesni256_key_expand<0x01, 0xff>(schedule[ 0], schedule[ 1]);
    schedule[ 3] = aesni256_key_expand<0x00, 0xaa>(schedule[ 1], schedule[ 2]);
    schedule[ 4] = aesni256_key_expand<0x02, 0xff>(schedule[ 2], schedule[ 3]);
    schedule[ 5] = aesni256_key_expand<0x00, 0xaa>(schedule[ 3], schedule[ 4]);
    schedule[ 6] = aesni256_key_expand<0x04, 0xff>(schedule[ 4], schedule[ 5]);
    schedule[ 7] = aesni256_key_expand<0x00, 0xaa>(schedule[ 5], schedule[ 6]);
    schedule[ 8] = aesni256_key_expand<0x08, 0xff>(schedule[ 6], schedule[ 7]);
    schedule[ 9] = aesni256_key_expand<0x00, 0xaa>(schedule[ 7], schedule[ 8]);
    schedule[10] = aesni256_key_expand<0x10, 0xff>(schedule[ 8], schedule[ 9]);
    schedule[11] = aesni256_key_expand<0x00, 0xaa>(schedule[ 9], schedule[10]);
    schedule[12] = aesni256_key_expand<0x20, 0xff>(schedule[10], schedule[11]);
    schedule[13] = aesni256_key_expand<0x00, 0xaa>(schedule[11], schedule[12]);
    schedule[14] = aesni256_key_expand<0x40, 0xff>(schedule[12], schedule[13]);

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

// ECB block

template <int NR>
inline __m128i aesni_ecb_encrypt_block(__m128i data, const __m128i* schedule);

template <>
inline __m128i aesni_ecb_encrypt_block<10>(__m128i data, const __m128i* schedule)
{
    data = _mm_xor_si128(data, schedule[0]);
    data = _mm_aesenc_si128(data, schedule[1]);
    data = _mm_aesenc_si128(data, schedule[2]);
    data = _mm_aesenc_si128(data, schedule[3]);
    data = _mm_aesenc_si128(data, schedule[4]);
    data = _mm_aesenc_si128(data, schedule[5]);
    data = _mm_aesenc_si128(data, schedule[6]);
    data = _mm_aesenc_si128(data, schedule[7]);
    data = _mm_aesenc_si128(data, schedule[8]);
    data = _mm_aesenc_si128(data, schedule[9]);
    return _mm_aesenclast_si128(data, schedule[10]);
}

template <>
inline __m128i aesni_ecb_encrypt_block<12>(__m128i data, const __m128i* schedule)
{
    data = _mm_xor_si128(data, schedule[0]);
    data = _mm_aesenc_si128(data, schedule[1]);
    data = _mm_aesenc_si128(data, schedule[2]);
    data = _mm_aesenc_si128(data, schedule[3]);
    data = _mm_aesenc_si128(data, schedule[4]);
    data = _mm_aesenc_si128(data, schedule[5]);
    data = _mm_aesenc_si128(data, schedule[6]);
    data = _mm_aesenc_si128(data, schedule[7]);
    data = _mm_aesenc_si128(data, schedule[8]);
    data = _mm_aesenc_si128(data, schedule[9]);
    data = _mm_aesenc_si128(data, schedule[10]);
    data = _mm_aesenc_si128(data, schedule[11]);
    return _mm_aesenclast_si128(data, schedule[12]);
}

template <>
inline __m128i aesni_ecb_encrypt_block<14>(__m128i data, const __m128i* schedule)
{
    data = _mm_xor_si128(data, schedule[0]);
    data = _mm_aesenc_si128(data, schedule[1]);
    data = _mm_aesenc_si128(data, schedule[2]);
    data = _mm_aesenc_si128(data, schedule[3]);
    data = _mm_aesenc_si128(data, schedule[4]);
    data = _mm_aesenc_si128(data, schedule[5]);
    data = _mm_aesenc_si128(data, schedule[6]);
    data = _mm_aesenc_si128(data, schedule[7]);
    data = _mm_aesenc_si128(data, schedule[8]);
    data = _mm_aesenc_si128(data, schedule[9]);
    data = _mm_aesenc_si128(data, schedule[10]);
    data = _mm_aesenc_si128(data, schedule[11]);
    data = _mm_aesenc_si128(data, schedule[12]);
    data = _mm_aesenc_si128(data, schedule[13]);
    return _mm_aesenclast_si128(data, schedule[14]);
}

template <int NR>
inline __m128i aesni_ecb_decrypt_block(__m128i data, const __m128i* schedule);

template <>
inline __m128i aesni_ecb_decrypt_block<10>(__m128i data, const __m128i* schedule)
{
    data = _mm_xor_si128(data, schedule[10]);
    data = _mm_aesdec_si128(data, schedule[11]);
    data = _mm_aesdec_si128(data, schedule[12]);
    data = _mm_aesdec_si128(data, schedule[13]);
    data = _mm_aesdec_si128(data, schedule[14]);
    data = _mm_aesdec_si128(data, schedule[15]);
    data = _mm_aesdec_si128(data, schedule[16]);
    data = _mm_aesdec_si128(data, schedule[17]);
    data = _mm_aesdec_si128(data, schedule[18]);
    data = _mm_aesdec_si128(data, schedule[19]);
    return _mm_aesdeclast_si128(data, schedule[0]);
}

template <>
inline __m128i aesni_ecb_decrypt_block<12>(__m128i data, const __m128i* schedule)
{
    data = _mm_xor_si128(data, schedule[10]);
    data = _mm_aesdec_si128(data, schedule[11]);
    data = _mm_aesdec_si128(data, schedule[12]);
    data = _mm_aesdec_si128(data, schedule[13]);
    data = _mm_aesdec_si128(data, schedule[14]);
    data = _mm_aesdec_si128(data, schedule[15]);
    data = _mm_aesdec_si128(data, schedule[16]);
    data = _mm_aesdec_si128(data, schedule[17]);
    data = _mm_aesdec_si128(data, schedule[18]);
    data = _mm_aesdec_si128(data, schedule[19]);
    data = _mm_aesdec_si128(data, schedule[20]);
    data = _mm_aesdec_si128(data, schedule[21]);
    return _mm_aesdeclast_si128(data, schedule[0]);
}

template <>
inline __m128i aesni_ecb_decrypt_block<14>(__m128i data, const __m128i* schedule)
{
    data = _mm_xor_si128(data, schedule[10]);
    data = _mm_aesdec_si128(data, schedule[11]);
    data = _mm_aesdec_si128(data, schedule[12]);
    data = _mm_aesdec_si128(data, schedule[13]);
    data = _mm_aesdec_si128(data, schedule[14]);
    data = _mm_aesdec_si128(data, schedule[15]);
    data = _mm_aesdec_si128(data, schedule[16]);
    data = _mm_aesdec_si128(data, schedule[17]);
    data = _mm_aesdec_si128(data, schedule[18]);
    data = _mm_aesdec_si128(data, schedule[19]);
    data = _mm_aesdec_si128(data, schedule[20]);
    data = _mm_aesdec_si128(data, schedule[21]);
    data = _mm_aesdec_si128(data, schedule[22]);
    data = _mm_aesdec_si128(data, schedule[23]);
    return _mm_aesdeclast_si128(data, schedule[0]);
}

// ECB buffer

template <int NR>
void aesni_ecb_encrypt(u8* output, const u8* input, size_t blocks, const __m128i* schedule)
{
    for (size_t i = 0; i < blocks; ++i)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input) + i);
        data = aesni_ecb_encrypt_block<NR>(data, schedule);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output) + i, data);
    }
}

template <int NR>
void aesni_ecb_decrypt(u8* output, const u8* input, size_t blocks, const __m128i* schedule)
{
    for (size_t i = 0; i < blocks; ++i)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input) + i);
        data = aesni_ecb_decrypt_block<NR>(data, schedule);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output) + i, data);
    }
}

// CBC buffer

template <int NR>
void aesni_cbc_encrypt(u8* output, const u8* input, size_t blocks, __m128i iv, const __m128i* schedule)
{
    for (size_t i = 0; i < blocks; ++i)
    {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input) + i);
        iv = _mm_xor_si128(data, iv);
        iv = aesni_ecb_encrypt_block<NR>(iv, schedule);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output) + i, iv);
    }
}

template <int NR>
void aesni_cbc_decrypt(u8* output, const u8* input, size_t blocks, __m128i iv, const __m128i* schedule)
{
    for (size_t i = 0; i < blocks; ++i)
    {
        __m128i temp = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input) + i);
        __m128i data = aesni_ecb_decrypt_block<NR>(temp, schedule);
        data = _mm_xor_si128(data, iv);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output) + i, data);
        iv = temp;
    }
}

// EBC selector

void aesni_ecb_encrypt(u8* output, const u8* input, size_t length, const __m128i* schedule, int keybits)
{
    const size_t blocks = (length + 15) / 16;
    switch (keybits)
    {
        case 128:
            aesni_ecb_encrypt<10>(output, input, blocks, schedule);
            break;
        case 192:
            aesni_ecb_encrypt<12>(output, input, blocks, schedule);
            break;
        case 256:
            aesni_ecb_encrypt<14>(output, input, blocks, schedule);
            break;
        default:
            break;
    }
}

void aesni_ecb_decrypt(u8* output, const u8* input, size_t length, const __m128i* schedule, int keybits)
{
    const size_t blocks = (length + 15) / 16;
    switch (keybits)
    {
        case 128:
            aesni_ecb_decrypt<10>(output, input, blocks, schedule);
            break;
        case 192:
            aesni_ecb_decrypt<12>(output, input, blocks, schedule);
            break;
        case 256:
            aesni_ecb_decrypt<14>(output, input, blocks, schedule);
            break;
        default:
            break;
    }
}

// CBC selector

void aesni_cbc_encrypt(u8* output, const u8* input, size_t length, const u8* ivec, const __m128i* schedule, int keybits)
{
    const size_t blocks = (length + 15) / 16;
    __m128i iv = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ivec));
    switch (keybits)
    {
        case 128:
            aesni_cbc_encrypt<10>(output, input, blocks, iv, schedule);
            break;
        case 192:
            aesni_cbc_encrypt<12>(output, input, blocks, iv, schedule);
            break;
        case 256:
            aesni_cbc_encrypt<14>(output, input, blocks, iv, schedule);
            break;
        default:
            break;
    }
}

void aesni_cbc_decrypt(u8* output, const u8* input, size_t length, const u8* ivec, const __m128i* schedule, int keybits)
{
    const size_t blocks = (length + 15) / 16;
    __m128i iv = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ivec));
    switch (keybits)
    {
        case 128:
            aesni_cbc_decrypt<10>(output, input, blocks, iv, schedule);
            break;
        case 192:
            aesni_cbc_decrypt<12>(output, input, blocks, iv, schedule);
            break;
        case 256:
            aesni_cbc_decrypt<14>(output, input, blocks, iv, schedule);
            break;
        default:
            break;
    }
}

void aesni_key_expand(__m128i* schedule, const u8* key, int bits)
{
    switch (bits)
    {
        case 128:
            aesni128_key_expand(schedule, key);
            break;
        case 192:
            aesni192_key_expand(schedule, key);
            break;
        case 256:
            aesni256_key_expand(schedule, key);
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
    union
    {
#if defined(MANGO_ENABLE_AES)
        __m128i schedule[28];
#endif
        u32 w[60];
    };
#if defined(MANGO_ENABLE_AES)
    bool aes_supported;
#endif
};

AES::AES(const u8* key, int bits)
    : m_schedule(new KeyScheduleAES())
    , m_bits(bits)
{
#if defined(MANGO_ENABLE_AES)
    m_schedule->aes_supported = (getCPUFlags() & CPU_AES) != 0;
    if (m_schedule->aes_supported)
    {
        aesni_key_expand(m_schedule->schedule, key, bits);
    }
    else
#endif
    {
        aes_key_setup(key, m_schedule->w, bits);
    }
}

AES::~AES()
{
    delete m_schedule;
}

void AES::ecb_encrypt(u8* output, const u8* input, size_t length)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

#if defined(MANGO_ENABLE_AES)
    if (m_schedule->aes_supported)
    {
        aesni_ecb_encrypt(output, input, length, m_schedule->schedule, m_bits);
    }
    else
#endif
    {
        for (size_t i = 0; i < length; i += 16)
        {
            aes_encrypt(input + i, output + i, m_schedule->w, m_bits);
        }
    }
}

void AES::ecb_decrypt(u8* output, const u8* input, size_t length)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

#if defined(MANGO_ENABLE_AES)
    if (m_schedule->aes_supported)
    {
        aesni_ecb_decrypt(output, input, length, m_schedule->schedule, m_bits);
    }
    else
#endif
    {
        for (size_t i = 0; i < length; i += 16)
        {
            aes_decrypt(input + i, output + i, m_schedule->w, m_bits);
        }
    }
}

void AES::cbc_encrypt(u8* output, const u8* input, size_t length, const u8* iv)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

#if defined(MANGO_ENABLE_AES)
    if (m_schedule->aes_supported)
    {
        aesni_cbc_encrypt(output, input, length, iv, m_schedule->schedule, m_bits);
    }
    else
#endif
    {
        aes_encrypt_cbc(input, length, output, m_schedule->w, m_bits, iv);
    }
}

void AES::cbc_decrypt(u8* output, const u8* input, size_t length, const u8* iv)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }

#if defined(MANGO_ENABLE_AES)
    if (m_schedule->aes_supported)
    {
        aesni_cbc_decrypt(output, input, length, iv, m_schedule->schedule, m_bits);
    }
    else
#endif
    {
        aes_decrypt_cbc(input, length, output, m_schedule->w, m_bits, iv);
    }
}

void AES::ctr_encrypt(u8* output, const u8* input, size_t length, const u8* iv)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }
    aes_encrypt_ctr(input, length, output, m_schedule->w, m_bits, iv);
}

void AES::ctr_decrypt(u8* output, const u8* input, size_t length, const u8* iv)
{
    if (length & 15)
    {
        MANGO_EXCEPTION("[AES] The length must be multiple of 16 bytes.");
    }
    aes_decrypt_ctr(input, length, output, m_schedule->w, m_bits, iv);
}

void AES::ccm_encrypt(Memory output, Memory input, Memory associated, Memory nonce, int mac_length)
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
                    m_schedule->w, m_bits);
}

void AES::ccm_decrypt(Memory output, Memory input, Memory associated, Memory nonce, int mac_length)
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
                    m_schedule->w, m_bits);
}

} // namespace mango
