/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/hash.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>

namespace {
    using namespace mango;

    constexpr uint32 rotateRight(uint32 value, int count)
    {
        return (value >> count) | (value << (32 - count));
    }

    void sha2_transform(uint32 state[8], const uint8* data)
    {
        static const uint32 k[] =
        {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        uint32 w[64];

        uint32 a = state[0];
        uint32 b = state[1];
        uint32 c = state[2];
        uint32 d = state[3];
        uint32 e = state[4];
        uint32 f = state[5];
        uint32 g = state[6];
        uint32 h = state[7];

        for (int i = 0; i < 16; ++i)
        {
            w[i] = uload32be(data + i * 4);

            uint32 s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            uint32 ch = (e & f) ^ ((~e) & g);
            uint32 x = h + s1 + ch + k[i] + w[i];
            uint32 s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            uint32 maj = (a & b) ^ (a & c) ^ (b & c);
            uint32 y = s0 + maj;
            
            h = g;
            g = f;
            f = e;
            e = d + x;
            d = c;
            c = b;
            b = a;
            a = x + y;
        }

        for (int i = 16; i < 64; ++i)
        {
            uint32 t0 = rotateRight(w[i - 15], 7) ^ rotateRight(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32 t1 = rotateRight(w[i - 2], 17) ^ rotateRight(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + t0 + w[i - 7] + t1;

            uint32 s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            uint32 ch = (e & f) ^ ((~e) & g);
            uint32 x = h + s1 + ch + k[i] + w[i];
            uint32 s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            uint32 maj = (a & b) ^ (a & c) ^ (b & c);
            uint32 y = s0 + maj;
            
            h = g;
            g = f;
            f = e;
            e = d + x;
            d = c;
            c = b;
            b = a;
            a = x + y;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

} // namespace

namespace mango {

    void sha2(uint32 hash[8], Memory memory)
    {
        hash[0] = 0x6a09e667;
        hash[1] = 0xbb67ae85;
        hash[2] = 0x3c6ef372;
        hash[3] = 0xa54ff53a;
        hash[4] = 0x510e527f;
        hash[5] = 0x9b05688c;
        hash[6] = 0x1f83d9ab;
        hash[7] = 0x5be0cd19;

        uint32 size = uint32(memory.size);
        const uint8* data = memory.address;

        while (size >= 64)
        {
            sha2_transform(hash, data);
            data += 64;
            size -= 64;
        }

        uint8 buffer[64];
        std::memcpy(buffer, data, size);
        std::memset(buffer + size, 0, 64 - size);
        buffer[size] = 0x80;

        if (size >= 56)
        {
            sha2_transform(hash, buffer);
            std::memset(buffer, 0, 56);
        }

        ustore64be(buffer + 56, memory.size * 8);
        sha2_transform(hash, buffer);

#ifdef MANGO_LITTLE_ENDIAN
        hash[0] = byteswap(hash[0]);
        hash[1] = byteswap(hash[1]);
        hash[2] = byteswap(hash[2]);
        hash[3] = byteswap(hash[3]);
        hash[4] = byteswap(hash[4]);
        hash[5] = byteswap(hash[5]);
        hash[6] = byteswap(hash[6]);
        hash[7] = byteswap(hash[7]);
#endif
    }

} // namespace mango
