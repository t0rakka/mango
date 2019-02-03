/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstring>
#include "jpeg.hpp"

namespace jpeg
{

    // ----------------------------------------------------------------------------
    // bextr()
    // ----------------------------------------------------------------------------

	#ifdef MANGO_CPU_64BIT
	    #define bextr mango::u64_extract_bits
	#else
	    #define bextr mango::u32_extract_bits
	#endif

    // ----------------------------------------------------------------------------
    // huffman decoder macros
    // ----------------------------------------------------------------------------

    constexpr int huff_extend(int v, int nbits)
    {
        return v - ((((v + v) >> nbits) - 1) & ((1 << nbits) - 1));
    }

#if 0

    static inline int GET_BITS(jpegBuffer &buffer, int nbits)
    {
        return int(bextr(buffer.data, buffer.remain -= nbits, nbits));
    }

    static inline int PEEK_BITS(jpegBuffer &buffer, int nbits)
    {
        return int(bextr(buffer.data, buffer.remain - nbits, nbits));
    }

#else

    #define GET_BITS(buffer, nbits) \
        int(bextr(buffer.data, buffer.remain -= nbits, nbits))

    #define PEEK_BITS(buffer, nbits) \
        int(bextr(buffer.data, buffer.remain - nbits, nbits))

#endif

    #define HUFF_RECEIVE(buffer, nbits) \
    { \
        buffer.ensure16(); \
        s = huff_extend(GET_BITS(buffer, nbits), nbits); \
    }

#ifndef JPEG_ENABLE_MODERN_HUFFMAN

    constexpr int LOOKAHEAD_MASK0 = ((1 << JPEG_HUFF_LOOKUP_BITS) - 1);
    constexpr int LOOKAHEAD_MASK1 = ((2 << JPEG_HUFF_LOOKUP_BITS) - 1);

    #define HUFF_DECODE(symbol, h) \
    {                                                              \
        buffer.ensure16();                                         \
        symbol = PEEK_BITS(buffer, JPEG_HUFF_LOOKUP_BITS);         \
        symbol = h->lookup[symbol];                                \
        int size = symbol >> JPEG_HUFF_LOOKUP_BITS;                \
        buffer.remain -= size;                                     \
        symbol = symbol & LOOKAHEAD_MASK0;                         \
        if (size == JPEG_HUFF_LOOKUP_BITS + 1) {                   \
            symbol = (buffer.data >> buffer.remain) & LOOKAHEAD_MASK1; \
            while (symbol > h->maxcode[size++]) {                  \
                symbol += symbol;                                  \
                symbol |= GET_BITS(buffer, 1);                     \
            }                                                      \
            symbol = h->value[ symbol + h->valoffset[size] ];      \
        }                                                          \
    }

#else

    #define HUFF_DECODE(symbol, h) \
    { \
        buffer.ensure16(); \
        int v = PEEK_BITS(buffer, JPEG_HUFF_LOOKUP_BITS); \
        symbol = h->lookupValue[v]; \
        int size = h->lookupSize[v]; \
        if (size == JPEG_HUFF_LOOKUP_BITS + 1) { \
            DataType x = (buffer.data << (JPEG_REGISTER_SIZE - buffer.remain)); \
            while (x > h->maxcode[size]) { \
                size++; \
            }  \
            v = int(x >> (JPEG_REGISTER_SIZE - size)); \
            symbol = h->valueAddress[size][v]; \
        } \
        buffer.remain -= size; \
    }

#endif

    // ----------------------------------------------------------------------------
    // huffman decoder
    // ----------------------------------------------------------------------------

    void huff_decode_mcu_lossless(BlockType* output, DecodeState* state)
    {
        Huffman& huffman = state->huffman;
        jpegBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;
            const HuffTable* dc_table = block->table.dc;

            int s;
            HUFF_DECODE(s, dc_table);
            if (s)
            {
                HUFF_RECEIVE(buffer, s);
            }

            s += huffman.last_dc_value[block->pred];
            output[j] = s;
        }
    }

    void huff_decode_mcu(BlockType* output, DecodeState* state)
    {
        const int* zigzagTable = state->zigzagTable;
        Huffman& huffman = state->huffman;
        jpegBuffer& buffer = state->buffer;

        std::memset(output, 0, state->blocks * 64 * sizeof(BlockType));

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;

            const HuffTable* dc_table = block->table.dc;
            const HuffTable* ac_table = block->table.ac;

            // DC
            int s;
            HUFF_DECODE(s, dc_table);
            if (s)
            {
                HUFF_RECEIVE(buffer, s);
            }

            s += huffman.last_dc_value[block->pred];
            huffman.last_dc_value[block->pred] = s;

            output[0] = static_cast<BlockType>(s);

            // AC
            for (int i = 1; i < 64; )
            {
                int s;
                HUFF_DECODE(s, ac_table);

                int r = s >> 4;
                s &= 15;
                
                if (s)
                {
                    i += r;
                    HUFF_RECEIVE(buffer, s);
                    output[zigzagTable[i++]] = static_cast<BlockType>(s);
                }
                else
                {
                    if (!r) break;
                    i += 16;
                }
            }

            output += 64;
        }
    }
    
    void huff_decode_dc_first(BlockType* output, DecodeState* state)
    {
        Huffman& huffman = state->huffman;
        jpegBuffer& buffer = state->buffer;

        for (int j = 0; j < state->blocks; ++j)
        {
            const DecodeBlock* block = state->block + j;

            BlockType* dest = output + block->offset;
            const HuffTable* dc_table = block->table.dc;
            
            std::memset(dest, 0, 64 * sizeof(BlockType));

            int s;
            HUFF_DECODE(s, dc_table);
            if (s)
            {
                HUFF_RECEIVE(buffer, s);
            }

            s += huffman.last_dc_value[block->pred];
            huffman.last_dc_value[block->pred] = s;

            dest[0] = static_cast<BlockType>(s << state->successiveLow);
        }
    }

    void huff_decode_dc_refine(BlockType* output, DecodeState* state)
    {
        jpegBuffer& buffer = state->buffer;

        buffer.ensure16();

        for (int j = 0; j < state->blocks; ++j)
        {
            BlockType* dest = output + state->block[j].offset;
            dest[0] |= (GET_BITS(buffer, 1) << state->successiveLow);
        }
    }
    
    void huff_decode_ac_first(BlockType* output, DecodeState* state)
    {
        const int* zigzagTable = state->zigzagTable;
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
                int s;
                HUFF_DECODE(s, ac_table);

                int r = s >> 4;
                s &= 15;

                i += r;

                if (s)
                {
                    HUFF_RECEIVE(buffer, s);
                    output[zigzagTable[i]] = static_cast<BlockType>(s << state->successiveLow);
                }
                else
                {
                    if (r != 15)
                    {
                        huffman.eob_run = 1 << r;
                        if (r)
                        {
                            buffer.ensure16();
                            huffman.eob_run += GET_BITS(buffer, r);
                        }
                        
                        --huffman.eob_run;
                        break;
                    }
                }
            }
        }
    }

    void huff_decode_ac_refine(BlockType* output, DecodeState* state)
    {
        const int* zigzagTable = state->zigzagTable;
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
                int s;
                HUFF_DECODE(s, ac_table);

                int r = s >> 4;
                s &= 15;

                if (s)
                {
                    buffer.ensure16();
                    if (GET_BITS(buffer, 1))
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
                            huffman.eob_run += GET_BITS(buffer, r);
                        }

                        break;
                    }
                }

                do
                {
                    BlockType* coef = output + zigzagTable[k];

                    if (*coef != 0)
                    {
                        buffer.ensure16();
                        if (GET_BITS(buffer, 1))
                        {
                            if ((*coef & p1) == 0)
                            {
                                const int d = *coef >= 0 ? p1 : m1;
                                *coef += static_cast<BlockType>(d);
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
                    output[zigzagTable[k]] = static_cast<BlockType>(s);
                }
            }
        }

        if (huffman.eob_run > 0)
        {
            for ( ; k <= end; k++)
            {
                BlockType* coef = output + zigzagTable[k];
                
                if (*coef != 0)
                {
                    buffer.ensure16();
                    if (GET_BITS(buffer, 1))
                    {
                        if ((*coef & p1) == 0)
                        {
                            const int d = *coef >= 0 ? p1 : m1;
                            *coef += static_cast<BlockType>(d);
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
    
#ifndef JPEG_ENABLE_MODERN_HUFFMAN

    void HuffTable::configure()
    {
        int p, i, l, si;//, numsymbols;
        int lookbits, ctr;
        char huffsize[257];
        unsigned int huffcode[257];
        unsigned int code;

        // Figure C.1: make table of Huffman code length for each symbol
        p = 0;
        for (l = 1; l <= 16; l++)
        {
            i = (int) size[l];
            while (i--)
                huffsize[p++] = (char) l;
        }
        huffsize[p] = 0;
        //numsymbols = p;

        // Figure C.2: generate the codes themselves
        code = 0;
        si = huffsize[0];
        p = 0;
        while (huffsize[p])
        {
            while (((int) huffsize[p]) == si)
            {
                huffcode[p++] = code;
                code++;
            }
            code <<= 1;
            si++;
        }

        // Figure F.15: generate decoding tables for bit-sequential decoding
        p = 0;
        for (l = 1; l <= 16; l++)
        {
            if (size[l])
            {
                valoffset[l+1] = (int) p - (int) huffcode[p];
                p += size[l];
                maxcode[l] = huffcode[p-1]; // maximum code of length l
            }
            else
            {
                maxcode[l] = -1; // -1 if no codes of this length
            }
        }
        valoffset[17+1] = 0;
        maxcode[17] = 0xfffff; // ensures jpeg_huff_decode terminates

        // Compute lookahead tables to speed up decoding.
        // First we set all the table entries to 0, indicating "too long";
        // then we iterate through the Huffman codes that are short enough and
        // fill in all the entries that correspond to bit sequences starting
        // with that code.

        for (i = 0; i < JPEG_HUFF_LOOKUP_SIZE; i++)
        {
            lookup[i] = (JPEG_HUFF_LOOKUP_BITS + 1) << JPEG_HUFF_LOOKUP_BITS;
        }
        
        p = 0;
        for (l = 1; l <= JPEG_HUFF_LOOKUP_BITS; l++)
        {
            for (i = 1; i <= (int) size[l]; i++, p++)
            {
                // l = current code's length, p = its index in huffcode[] & huffval[].
                // Generate left-justified code followed by all possible bit sequences
                lookbits = huffcode[p] << (JPEG_HUFF_LOOKUP_BITS - l);
                for (ctr = 1 << (JPEG_HUFF_LOOKUP_BITS - l); ctr > 0; ctr--)
                {
                    lookup[lookbits] = (l << JPEG_HUFF_LOOKUP_BITS) | value[p];
                    lookbits++;
                }
            }
        }
    }

#else

    void HuffTable::configure()
    {
        int p, si;
        char huffsize[257];
        unsigned int huffcode[257];
        unsigned int code;

        // Figure C.1: make table of Huffman code length for each symbol
        p = 0;
        for (int l = 1; l <= 16; l++)
        {
            int i = (int) size[l];
            while (i--)
            {
                huffsize[p++] = (char) l;
            }
        }
        huffsize[p] = 0;

        // Figure C.2: generate the codes themselves
        code = 0;
        si = huffsize[0];
        p = 0;
        while (huffsize[p])
        {
            while (((int) huffsize[p]) == si)
            {
                huffcode[p++] = code;
                code++;
            }
            code <<= 1;
            si++;
        }

        // Figure F.15: generate decoding tables for bit-sequential decoding
        p = 0;
        for (int l = 1; l <= 16; l++)
        {
            if (size[l])
            {
                int offset = p - int(huffcode[p]);
                valueAddress[l] = value + offset;
                p += size[l];
                maxcode[l] = huffcode[p - 1]; // maximum code of length l
                maxcode[l] <<= (JPEG_REGISTER_SIZE - l); // left justify
                maxcode[l] |= (DataType(1) << (JPEG_REGISTER_SIZE - l)) - 1;
            }
            else
            {
                maxcode[l] = 0; // TODO: should be -1 if no codes of this length
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
        for (int l = 1; l <= JPEG_HUFF_LOOKUP_BITS; l++)
        {
            for (int i = 1; i <= (int) size[l]; i++, p++)
            {
                // l = current code's length, p = its index in huffcode[] & huffval[].
                // Generate left-justified code followed by all possible bit sequences
                int lookbits = huffcode[p] << (JPEG_HUFF_LOOKUP_BITS - l);
                for (int ctr = 1 << (JPEG_HUFF_LOOKUP_BITS - l); ctr > 0; ctr--)
                {
                    lookupSize[lookbits] = u8(l);
                    lookupValue[lookbits] = value[p];
                    lookbits++;
                }
            }
        }
    }

#endif
    
} // namespace jpeg
