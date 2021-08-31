/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>
#include "jpeg.hpp"

namespace mango::jpeg
{

    // ----------------------------------------------------------------------------
    // Huffman
    // ----------------------------------------------------------------------------

    void Huffman::restart()
    {
        for (int i = 0; i < JPEG_MAX_COMPS_IN_SCAN; ++i)
        {
            last_dc_value[i] = 0;
        }

        eob_run = 0;
    }

    // ----------------------------------------------------------------------------
    // HuffTable
    // ----------------------------------------------------------------------------

    bool HuffTable::configure()
    {
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
                maxcode[j] |= (DataType(1) << (JPEG_REGISTER_BITS - j)) - 1;
            }
            else
            {
                maxcode[j] = 0; // TODO: should be -1 if no codes of this length
            }
        }
        valueOffset[18] = 0;
        maxcode[17] = ~DataType(0); //0xfffff; // ensures jpeg_huff_decode terminates

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
    }

    int HuffTable::decode(BitBuffer& buffer) const
    {
        buffer.ensure();

        int index = buffer.peekBits(JPEG_HUFF_LOOKUP_BITS);
        int size = lookupSize[index];

        int symbol;

        if (size <= JPEG_HUFF_LOOKUP_BITS)
        {
            symbol = lookupValue[index];
        }
        else
        {
            DataType x = (buffer.data << (JPEG_REGISTER_BITS - buffer.remain));
            while (x > maxcode[size])
            {
                ++size;
            }

            DataType offset = (x >> (JPEG_REGISTER_BITS - size)) + valueOffset[size];
#if 0
            if (offset > 255)
                return 0; // decoding error
#endif
            symbol = value[offset];
        }

        buffer.remain -= size;
        return symbol;
    }

    // ----------------------------------------------------------------------------
    // huffman decoding functions
    // ----------------------------------------------------------------------------

    void huff_decode_mcu_lossless(s16* output, DecodeState* state)
    {
        Huffman& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;
            const HuffTable* dc = &huffman.table[0][block->dc];

            int s = dc->decode(buffer);
            if (s)
            {
                s = buffer.receive(s);
            }

            s += huffman.last_dc_value[block->pred];
            output[j] = s;
        }
    }

    void huff_decode_mcu(s16* output, DecodeState* state)
    {
        const u8* zigzagTable = state->zigzagTable;
        Huffman& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        std::memset(output, 0, state->blocks * 64 * sizeof(s16));

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;

            const HuffTable* dc = &huffman.table[0][block->dc];
            const HuffTable* ac = &huffman.table[1][block->ac];

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
                int s = ac->decode(buffer);
                int x = s & 15;

                if (x)
                {
                    i += (s >> 4);
                    s = buffer.receive(x);
                    output[zigzagTable[i++]] = s16(s);
                }
                else
                {
                    if (s < 16) break;
                    i += 16;
                }
            }

            output += 64;
        }
    }

    void huff_decode_dc_first(s16* output, DecodeState* state)
    {
        Huffman& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;

            s16* dest = output + block->offset;
            const HuffTable* dc = &huffman.table[0][block->dc];

            std::memset(dest, 0, 64 * sizeof(s16));

            int s = dc->decode(buffer);
            if (s)
            {
                s = buffer.receive(s);
            }

            s += huffman.last_dc_value[block->pred];
            huffman.last_dc_value[block->pred] = s;

            dest[0] = s16(s << state->successiveLow);
        }
    }

    void huff_decode_dc_refine(s16* output, DecodeState* state)
    {
        BitBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            s16* dest = output + state->block[j].offset;
            dest[0] |= (buffer.getBits(1) << state->successiveLow);
        }
    }

    void huff_decode_ac_first(s16* output, DecodeState* state)
    {
        const u8* zigzagTable = state->zigzagTable;
        Huffman& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        const HuffTable* ac = &huffman.table[1][state->block[0].ac];

        const int start = state->spectralStart;
        const int end = state->spectralEnd;

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
                    output[zigzagTable[i]] = s16(s << state->successiveLow);
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
        const u8* zigzagTable = state->zigzagTable;
        Huffman& huffman = state->huffman;
        BitBuffer& buffer = state->buffer;

        const HuffTable* ac = &huffman.table[1][state->block[0].ac];

        const int start = state->spectralStart;
        const int end = state->spectralEnd;

        const int p1 = 1 << state->successiveLow;
        const int m1 = (-1) << state->successiveLow;

        int k = start;

        if (!huffman.eob_run)
        {
            for (; k <= end; k++)
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

                    k++;
                } while (k <= end);

                if ((s) && (k < 64))
                {
                    output[zigzagTable[k]] = s16(s);
                }
            }
        }

        if (huffman.eob_run > 0)
        {
            for ( ; k <= end; k++)
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

} // namespace mango::jpeg
