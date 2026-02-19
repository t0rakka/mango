/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>
#include "jpeg.hpp"

namespace mango::image::jpeg
{

    // ----------------------------------------------------------------------------
    // HuffmanDecoder
    // ----------------------------------------------------------------------------

    void HuffmanDecoder::restart()
    {
        for (int i = 0; i < JPEG_MAX_COMPS_IN_SCAN; ++i)
        {
            last_dc_value[i] = 0;
        }

        eob_run = 0;
    }

    // ----------------------------------------------------------------------------
    // HuffmanTable
    // ----------------------------------------------------------------------------

    bool HuffmanTable::configure()
    {
#if defined(JPEG_ENABLE_IGJ_HUFFMAN)
        int p, i, l, si;
        int lookbits, ctr;
        char huffsize[257];
        unsigned int huffcode[257];
        unsigned int code;

        /* Figure C.1: make table of Huffman code length for each symbol */
        p = 0;
        for (l = 1; l <= 16; l++) {
            i = size[l];
            while (i--)
                huffsize[p++] = (char)l;
            }
            huffsize[p] = 0;

            /* Figure C.2: generate the codes themselves */
            /* We also validate that the counts represent a legal Huffman code tree. */

            code = 0;
            si = huffsize[0];
            p = 0;
            while (huffsize[p]) {
                while (((int)huffsize[p]) == si) {
                    huffcode[p++] = code;
                    code++;
                }
                code <<= 1;
                si++;
            }

            /* Figure F.15: generate decoding tables for bit-sequential decoding */

            p = 0;
            for (l = 1; l <= 16; l++) {
            if (size[l]) {
                valoffset[l] = (int)p - (int)huffcode[p];
                p += size[l];
                maxcode[l] = huffcode[p - 1]; /* maximum code of length l */
            } else {
                maxcode[l] = -1;    /* -1 if no codes of this length */
            }
        }
        valoffset[17] = 0;
        maxcode[17] = 0xFFFFFL; /* ensures jpeg_huff_decode terminates */

        /* Compute lookahead tables to speed up decoding.
        * First we set all the table entries to 0, indicating "too long";
        * then we iterate through the Huffman codes that are short enough and
        * fill in all the entries that correspond to bit sequences starting
        * with that code.
        */

        for (i = 0; i < JPEG_HUFF_LOOKUP_SIZE; i++)
            lookup[i] = (JPEG_HUFF_LOOKUP_BITS + 1) << JPEG_HUFF_LOOKUP_BITS;

        p = 0;
        for (l = 1; l <= JPEG_HUFF_LOOKUP_BITS; l++) {
            for (i = 1; i <= (int)size[l]; i++, p++) {
                /* l = current code's length, p = its index in huffcode[] & huffval[]. */
                /* Generate left-justified code followed by all possible bit sequences */
                lookbits = huffcode[p] << (JPEG_HUFF_LOOKUP_BITS - l);
                for (ctr = 1 << (JPEG_HUFF_LOOKUP_BITS - l); ctr > 0; ctr--) {
                    lookup[lookbits] = (l << JPEG_HUFF_LOOKUP_BITS) | value[p];
                    lookbits++;
                }
            }
        }

        return true;

#else

        u8 huffsize[257];
        u32 huffcode[257];

        // Figure C.1: make table of Huffman code length for each symbol
        int p = 0;
        for (int j = 1; j <= 16; ++j)
        {
            int count = int(size[j]);
            while (count-- > 0)
            {
                huffsize[p++] = u8(j);
            }
        }
        huffsize[p] = 0;

        // Figure C.2: generate the codes themselves
        u32 code = 0;
        int si = huffsize[0];
        p = 0;
        while (huffsize[p])
        {
            while ((int(huffsize[p])) == si)
            {
                huffcode[p++] = code;
                code++;
            }
            code <<= 1;
            si++;
        }

        // Figure F.15: generate decoding tables for bit-sequential decoding
        p = 0;
        for (int j = 1; j <= 16; j++)
        {
            if (size[j])
            {
                valueOffset[j] = p - int(huffcode[p]);
                p += size[j];
                maxcode[j] = huffcode[p - 1]; // maximum code of length j
                maxcode[j] <<= (JPEG_REGISTER_BITS - j); // left justify
                maxcode[j] |= (HuffmanType(1) << (JPEG_REGISTER_BITS - j)) - 1;
            }
            else
            {
                maxcode[j] = 0xfffff;
            }
        }
        valueOffset[18] = 0;
        maxcode[17] = 0xfffff; // ensures jpeg_huff_decode terminates

        // Compute lookahead tables to speed up decoding.
        // First we set all the table entries to 0, indicating "too long";
        // then we iterate through the Huffman codes that are short enough and
        // fill in all the entries that correspond to bit sequences starting
        // with that code.

        for (int i = 0; i < JPEG_HUFF_LOOKUP_SIZE; i++)
        {
            lookupSize[i] = JPEG_HUFF_LOOKUP_BITS + 1;
            lookupValue[i] = 0;
        }

        p = 0;

        for (int bits = 1; bits <= JPEG_HUFF_LOOKUP_BITS; ++bits)
        {
            int ishift = JPEG_HUFF_LOOKUP_BITS - bits;
            int isize = size[bits];

            u8 current_size = u8(bits);

            for (int i = 1; i <= isize; ++i)
            {
                u8 current_value = value[p];
                int lookbits = huffcode[p] << ishift;
                ++p;

                // Generate left-justified code followed by all possible bit sequences
                int count = 1 << ishift;
                for (int mask = 0; mask < count; ++mask)
                {
                    int x = lookbits | mask;
                    if (x >= JPEG_HUFF_LOOKUP_SIZE)
                    {
                        // overflow
                        return false;
                    }

                    lookupSize[x] = current_size;
                    lookupValue[x] = current_value;
                }
            }
        }

        return true;
#endif
    }

    int HuffmanTable::decode(BitBuffer& buffer) const
    {
#if defined(JPEG_ENABLE_IGJ_HUFFMAN)
        int s;

        buffer.ensure();
        s = buffer.peekBits(JPEG_HUFF_LOOKUP_BITS);
        s = lookup[s];
        int nb = s >> JPEG_HUFF_LOOKUP_BITS;
        buffer.remain -= nb;
        s = s & (JPEG_HUFF_LOOKUP_SIZE - 1);
        if (nb > JPEG_HUFF_LOOKUP_BITS)
        {
            s = (buffer.data >> buffer.remain) & ((1 << (nb)) - 1);
            while (s > maxcode[nb])
            {
                s <<= 1;
                s |= buffer.getBits(1);
                nb++;
            }
            if (nb > 16)
                s = 0;
            else
                s = value[(s + valoffset[nb]) & 0xff];
        }

        return s;
#else
        buffer.ensure();

        int index = buffer.peekBits(JPEG_HUFF_LOOKUP_BITS);
        int c_size = lookupSize[index];

        int symbol;

        if (c_size <= JPEG_HUFF_LOOKUP_BITS)
        {
            symbol = lookupValue[index];
        }
        else
        {
            HuffmanType x = (buffer.data << (JPEG_REGISTER_BITS - buffer.remain));
            while (x > maxcode[c_size])
            {
                ++c_size;
            }

            HuffmanType offset = (x >> (JPEG_REGISTER_BITS - c_size)) + valueOffset[c_size];
            if (offset > 255)
                return 0; // decoding error
            symbol = value[offset];
        }

        buffer.remain -= c_size;

        return symbol;
#endif
    }

    // ----------------------------------------------------------------------------
    // huffman decoding functions
    // ----------------------------------------------------------------------------

    void huff_decode_mcu_lossless(s16* output, DecodeState* state)
    {
        HuffmanDecoder& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;
            const HuffmanTable* dc = &huffman.table[0][block->dc];

            int s = dc->decode(buffer);
            if (s)
            {
                s = buffer.receive(s);
            }

            s += huffman.last_dc_value[block->pred];
            output[j] = s16(s);
        }
    }

    void huff_decode_mcu(s16* output, DecodeState* state)
    {
        HuffmanDecoder& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        std::memset(output, 0, state->blocks * 64 * sizeof(s16));

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;

            const HuffmanTable* dc = &huffman.table[0][block->dc];
            const HuffmanTable* ac = &huffman.table[1][block->ac];

            // DC
            int s = dc->decode(buffer);
            if (s)
            {
                s = buffer.receive(s);
            }

            s += huffman.last_dc_value[block->pred];
            huffman.last_dc_value[block->pred] = s;

            output[0] = s16(s);

            // AC
            for (int i = 1; i < 64; )
            {
                //int symbol = ac->decode(buffer);
#if defined(JPEG_ENABLE_IGJ_HUFFMAN)
                int symbol;

                buffer.ensure();
                symbol = buffer.peekBits(JPEG_HUFF_LOOKUP_BITS);
                symbol = ac->lookup[symbol];
                int nb = symbol >> JPEG_HUFF_LOOKUP_BITS;
                buffer.remain -= nb;
                symbol &= (JPEG_HUFF_LOOKUP_SIZE - 1);
                if (nb > JPEG_HUFF_LOOKUP_BITS)
                {
                    symbol = (buffer.data >> buffer.remain) & ((1 << (nb)) - 1);
                    while (symbol > ac->maxcode[nb])
                    {
                        symbol <<= 1;
                        symbol |= buffer.getBits(1);
                        nb++;
                    }
                    if (nb > 16)
                        symbol = 0; // invalid symbol
                    else
                        symbol = ac->value[(s + ac->valoffset[nb]) & 0xff];
                }
#else
                buffer.ensure();

                int index = buffer.peekBits(JPEG_HUFF_LOOKUP_BITS);
                int size = ac->lookupSize[index];

                int symbol;

                if (size <= JPEG_HUFF_LOOKUP_BITS)
                {
                    symbol = ac->lookupValue[index];
                }
                else
                {
                    HuffmanType x = (buffer.data << (JPEG_REGISTER_BITS - buffer.remain));
                    while (x > ac->maxcode[size])
                    {
                        ++size;
                    }

                    HuffmanType offset = (x >> (JPEG_REGISTER_BITS - size)) + ac->valueOffset[size];
                    symbol = ac->value[offset];
                }

                buffer.remain -= size;
#endif

                if (symbol)
                {
                    int bits = symbol & 15;
                    if (bits)
                    {
                        i += (symbol >> 4);
                        symbol = buffer.receive(bits);
                        output[zigzagTable[i++]] = s16(symbol);
                    }
                    else
                    {
                        // ZRL (sixteen zeroes)
                        i += 16;
                    }
                }
                else
                {
                    // EOB (End of Block)
                    break;
                }
            }

            output += 64;
        }
    }

    void huff_decode_dc_first(s16* output, DecodeState* state)
    {
        HuffmanDecoder& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;

            s16* dest = output + block->offset;
            const HuffmanTable* dc = &huffman.table[0][block->dc];

            std::memset(dest, 0, 64 * sizeof(s16));

            int s = dc->decode(buffer);
            if (s)
            {
                s = buffer.receive(s);
            }

            s += huffman.last_dc_value[block->pred];
            huffman.last_dc_value[block->pred] = s;

            dest[0] = s16(s << state->successive_low);
        }
    }

    void huff_decode_dc_refine(s16* output, DecodeState* state)
    {
        BitBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            s16* dest = output + state->block[j].offset;
            dest[0] |= (buffer.getBits(1) << state->successive_low);
        }
    }

    void huff_decode_ac_first(s16* output, DecodeState* state)
    {
        HuffmanDecoder& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        const HuffmanTable* ac = &huffman.table[1][state->block[0].ac];

        const int start = state->spectral_start;
        const int end = state->spectral_end;

        if (huffman.eob_run)
        {
            --huffman.eob_run;
        }
        else
        {
            for (int i = start; i <= end; ++i)
            {
                int s = ac->decode(buffer);
                int r = s >> 4;
                s &= 15;

                i += r;

                if (s)
                {
                    s = buffer.receive(s);
                    output[zigzagTable[i]] = s16(s << state->successive_low);
                }
                else
                {
                    if (r != 15)
                    {
                        huffman.eob_run = 1 << r;
                        if (r)
                        {
                            huffman.eob_run += buffer.getBits(r);
                        }

                        --huffman.eob_run;
                        break;
                    }
                }
            }
        }
    }

    void huff_decode_ac_refine(s16* output, DecodeState* state)
    {
        HuffmanDecoder& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        const HuffmanTable* ac = &huffman.table[1][state->block[0].ac];

        const int start = state->spectral_start;
        const int end = state->spectral_end;

        const int p1 = 1 << state->successive_low;
        const int m1 = (-1) << state->successive_low;

        int k = start;

        if (!huffman.eob_run)
        {
            for ( ; k <= end; ++k)
            {
                int s = ac->decode(buffer);
                int r = s >> 4;
                s &= 15;

                if (s)
                {
                    if (buffer.getBits(1))
                        s = p1;
                    else
                        s = m1;
                }
                else
                {
                    if (r != 15)
                    {
                        huffman.eob_run = 1 << r;

                        if (r)
                        {
                            huffman.eob_run += buffer.getBits(r);
                        }

                        break;
                    }
                }

                do
                {
                    s16* coef = output + zigzagTable[k];
                    if (*coef != 0)
                    {
                        if (buffer.getBits(1))
                        {
                            if ((*coef & p1) == 0)
                            {
                                const int d = *coef >= 0 ? p1 : m1;
                                *coef += s16(d);
                            }
                        }
                    }
                    else
                    {
                        if (--r < 0)
                            break;
                    }

                    ++k;
                } while (k <= end);

                if (s && (k < 64))
                {
                    output[zigzagTable[k]] = s16(s);
                }
            }
        }

        if (huffman.eob_run > 0)
        {
            for ( ; k <= end; ++k)
            {
                s16* coef = output + zigzagTable[k];

                if (*coef != 0)
                {
                    if (buffer.getBits(1))
                    {
                        if ((*coef & p1) == 0)
                        {
                            const int d = *coef >= 0 ? p1 : m1;
                            *coef += s16(d);
                        }
                    }
                }
            }

            --huffman.eob_run;
        }
    }

} // namespace mango::image::jpeg
