/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>
#include "jpeg.hpp"

namespace mango {
namespace jpeg {

    // ----------------------------------------------------------------------------
    // huffman functions
    // ----------------------------------------------------------------------------

    constexpr
    int huff_extend(int v, int nbits)
    {
        return v - ((((v + v) >> nbits) - 1) & ((1 << nbits) - 1));
    }

    static inline
    int huff_receive(jpegBuffer& buffer, int nbits)
    {
        buffer.ensure16();
        int value = buffer.getBits(nbits);
        return huff_extend(value, nbits);
    }

    static inline
    int huff_decode(jpegBuffer& buffer, const HuffTable* h)
    {
        buffer.ensure16();

        int index = buffer.peekBits(JPEG_HUFF_LOOKUP_BITS);
        int size = h->lookupSize[index];

        int symbol;

        if (size <= JPEG_HUFF_LOOKUP_BITS)
        {
            symbol = h->lookupValue[index];
        }
        else
        {
            DataType x = (buffer.data << (JPEG_REGISTER_BITS - buffer.remain));
            while (x > h->maxcode[size])
            {
                size++;
            }

            symbol = h->valueAddress[size][x >> (JPEG_REGISTER_BITS - size)];
        }

        buffer.remain -= size;
        return symbol;
    }

    // ----------------------------------------------------------------------------
    // huffman decoder
    // ----------------------------------------------------------------------------

    void huff_decode_mcu_lossless(s16* output, DecodeState* state)
    {
        Huffman& huffman = state->huffman;
        jpegBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;
            const HuffTable* dc_table = block->table.dc;

            int s = huff_decode(buffer, dc_table);
            if (s)
            {
                s = huff_receive(buffer, s);
            }

            s += huffman.last_dc_value[block->pred];
            output[j] = s;
        }
    }

    void huff_decode_mcu(s16* output, DecodeState* state)
    {
        const u8* zigzagTable = state->zigzagTable;
        Huffman& huffman = state->huffman;
        jpegBuffer& buffer = state->buffer;

        std::memset(output, 0, state->blocks * 64 * sizeof(s16));

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;

            const HuffTable* dc_table = block->table.dc;
            const HuffTable* ac_table = block->table.ac;

            // DC
            int s = huff_decode(buffer, dc_table);
            if (s)
            {
                s = huff_receive(buffer, s);
            }

            s += huffman.last_dc_value[block->pred];
            huffman.last_dc_value[block->pred] = s;

            output[0] = s16(s);

            // AC
            for (int i = 1; i < 64; )
            {
                int s = huff_decode(buffer, ac_table);
                int x = s & 15;

                if (x)
                {
                    i += (s >> 4);
                    s = huff_receive(buffer, x);
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
        jpegBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;

            s16* dest = output + block->offset;
            const HuffTable* dc_table = block->table.dc;
            
            std::memset(dest, 0, 64 * sizeof(s16));

            int s = huff_decode(buffer, dc_table);
            if (s)
            {
                s = huff_receive(buffer, s);
            }

            s += huffman.last_dc_value[block->pred];
            huffman.last_dc_value[block->pred] = s;

            dest[0] = s16(s << state->successiveLow);
        }
    }

    void huff_decode_dc_refine(s16* output, DecodeState* state)
    {
        jpegBuffer& buffer = state->buffer;

        buffer.ensure16();

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
        jpegBuffer& buffer = state->buffer;

        const HuffTable* ac_table = state->block[0].table.ac;

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
                int s = huff_decode(buffer, ac_table);

                int r = s >> 4;
                s &= 15;

                i += r;

                if (s)
                {
                    s = huff_receive(buffer, s);
                    output[zigzagTable[i]] = s16(s << state->successiveLow);
                }
                else
                {
                    if (r != 15)
                    {
                        huffman.eob_run = 1 << r;
                        if (r)
                        {
                            buffer.ensure16();
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
        jpegBuffer& buffer = state->buffer;

        const HuffTable* ac_table = state->block[0].table.ac;

        const int start = state->spectralStart;
        const int end = state->spectralEnd;

        const int p1 = 1 << state->successiveLow;
        const int m1 = (-1) << state->successiveLow;

        int k = start;
        
        if (!huffman.eob_run)
        {
            for (; k <= end; k++)
            {
                int s = huff_decode(buffer, ac_table);

                int r = s >> 4;
                s &= 15;

                if (s)
                {
                    buffer.ensure16();
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
                            buffer.ensure16();
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
                        buffer.ensure16();
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
                    buffer.ensure16();
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

    void HuffTable::configure()
    {
        char huffsize[257];
        u32 huffcode[257];

        // Figure C.1: make table of Huffman code length for each symbol
        int p = 0;
        for (int j = 1; j <= 16; j++)
        {
            int i = int(size[j]);
            while (i--)
            {
                huffsize[p++] = char(j);
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
                int offset = p - int(huffcode[p]);
                valueAddress[j] = value + offset;
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
        valueAddress[18] = value + 0;
        maxcode[17] = ~DataType(0);//0xfffff; // ensures jpeg_huff_decode terminates

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
        for (int j = 1; j <= JPEG_HUFF_LOOKUP_BITS; j++)
        {
            int jshift = JPEG_HUFF_LOOKUP_BITS - j;
            for (int i = 1; i <= int(size[j]); i++, p++)
            {
                // j = current code's length, p = its index in huffcode[] & huffval[].
                // Generate left-justified code followed by all possible bit sequences
                int lookbits = huffcode[p] << jshift;
                for (int ctr = 1 << jshift; ctr > 0; ctr--)
                {
                    lookupSize[lookbits] = u8(j);
                    lookupValue[lookbits] = value[p];
                    lookbits++;
                }
            }
        }
    }
    
} // namespace jpeg
} // namespace mango
