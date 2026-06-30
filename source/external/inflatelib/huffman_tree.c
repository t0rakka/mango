/*
 *    Copyright (c) Microsoft. All rights reserved.
 *    This code is licensed under the MIT License.
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 *    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *    PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
#include <assert.h>
#include <string.h>

#include "internal.h"

/*
 * The size of the each lookup table, N, is decided as follows: 7 bits for the code length and distance tree, and 10
 * bits for the literal/length tree. To calculate the size needed for the binary tree portion of the array, we need to
 * calculate the maximum number of nodes that can be in all of the subtrees combined. Thanks to the way that the Huffman
 * codes are specified, all but the "right side" of the corresponding Huffman tree is optimally "packed." That is, all
 * nodes have either zero or two children. This means that, with the exception of the right-most path, we can relate the
 * total number of nodes to the number of leaves using the equation 'N = 2L - 1', where 'N' is the total number of nodes
 * and 'L' is the number of leaves. Since the root of each subtree is effectively stored in the lookup table, this
 * equation reduces to 'N = 2L - 2' for all but (potentially) the "last" subtree. Expanding this equation out to account
 * for 'M' subtrees, we get the equation 'N = M * (2L - 2)'. This implies that the total number of nodes _decreases_ as
 * the number of subtrees increases. I.e. the maximum amount of memory usage occurs when M is smallest. The sole
 * exception is the "right-most" path, which can have nodes that only have one child. Therefore, in order to determine
 * the maximum number of nodes in all subtrees, and therefore the total amount of memory we need to allocate for the
 * lookup table/binary tree array, we can consider the fewest number of subtrees that can hold 'K - 1' nodes (where 'K'
 * is the size of the dictionary) and a final subtree with a single node at max height. This final tree maximizes the
 * number of "dead" nodes that we have to allocate thanks to our indexing scheme. Note that it's possible for other
 * configuration(s) to require the same amount of memory[1], however it's not possible for them to require more memory.
 * By considering such trees, we can calculate the maximum amount of memory needed for each array:
 *
 *      1.  Code Length Tree: These code lengths are encoded using 3 bits (0-7) and can therefore all fit in the lookup
 *          table. Therefore, the max array size needed is 128.
 *      2.  Distance Tree: These code lengths are encoded using the code length tree and therefore have a maximum length
 *          of 15 bits. We use 7 bits for the indexing into the lookup table, leaving 8 bits for the binary tree portion
 *          of the array. The distance alphabet has a size of 32, so one tree structure that give us max memory usage
 *          is: one subtree contains 31 nodes and another contains a single node at max height. Solving for the total
 *          number of nodes gives '(31 * 2 - 2) + (2 * 8) = 76'. This gives a max array size of 128 + 76 = 204.
 *      3.  Literal/Length Tree: These code lengths are also encoded using the code length tree and therefore have a
 *          maximum of 15 bits. Unlike the above two trees, the lookup table uses 10-bit indices for the lookup table,
 *          leaving 5 bits for the binary tree portion of the array. The literal/length alphabet has a size of 288.
 *          Dividing this size by 32 - the maximum number of leaves in a single subtree - gives 9 remainder 0. One tree
 *          structure that gives us max memory usage is: one subtree with 31 leaves, 8 subtrees with 32 leaves, and a
 *          final subtree with a single leaf at max height. Solving for the total number of nodes gives
 *          '(31 * 2 - 2) + 8 * (32 * 2 - 2) + (2 * 5) = 566'. This gives a max array size of 1024 + 574 = 1590.
 *
 * [1]  This can occur if the last subtree can be arranged such that the left half is "optimally packed" and the right
 *      half consists of the single, final node at max height. This trades off one "dead" node from the last tree for an
 *      additional node gained by being able to increase a single node's height by one. The net change from the maximums
 *      calculated above is zero.
 */
#define CODE_LENGTH_TREE_ARRAY_SIZE 128
#define DISTANCE_TREE_ARRAY_SIZE 204
#define LITERAL_LENGTH_TREE_ARRAY_SIZE 1590

static inline uint16_t reverse_bits(uint16_t value, int bitCount);

int huffman_tree_init(huffman_tree* tree, inflatelib_stream* stream, size_t dictionarySize)
{
    /* We only support three "forms" of initialization */
    assert(
        (dictionarySize == LITERAL_TREE_MAX_ELEMENT_COUNT) || (dictionarySize == DIST_TREE_MAX_ELEMENT_COUNT) ||
        (dictionarySize == CODE_LENGTH_TREE_ELEMENT_COUNT));

    if (dictionarySize == LITERAL_TREE_MAX_ELEMENT_COUNT)
    {
        tree->table_bits = 10;
        tree->data_size = LITERAL_LENGTH_TREE_ARRAY_SIZE;
    }
    else if (dictionarySize == DIST_TREE_MAX_ELEMENT_COUNT)
    {
        tree->table_bits = 7;
        tree->data_size = DISTANCE_TREE_ARRAY_SIZE;
    }
    else
    {
        tree->table_bits = 7;
        tree->data_size = CODE_LENGTH_TREE_ARRAY_SIZE;
    }

    tree->table_mask = ((uint16_t)0x01 << tree->table_bits) - 1;

    tree->data = INFLATELIB_ALLOC(stream, huffman_table_entry, tree->data_size);
    if (!tree->data)
    {
        stream->error_msg = "Failed to allocate space for Huffman table";
        errno = ENOMEM;
        return INFLATELIB_ERROR_OOM;
    }
    /* NOTE: 'reset' should clear data in the table */

    return INFLATELIB_OK;
}

int huffman_tree_reset(huffman_tree* tree, inflatelib_stream* stream, const uint8_t* codeLengths, size_t codeLengthsSize)
{
    uint16_t bitLengthCount[MAX_CODE_LENGTH + 1]; /* NOTE: +1 because we index by length (index 0 effectively "wasted") */
    uint16_t nextCodes[MAX_CODE_LENGTH + 1];
    uint16_t nextCode;
    huffman_table_entry* treeBase = tree->data + ((size_t)0x01 << tree->table_bits);
    huffman_table_entry* nextTreeInsertPtr = treeBase;
    uint16_t nextTreeInsertIndex = 0;

    /* Zero out the lookup table. This ensures that data is listed as "invalid" by default */
    memset(tree->data, 0, ((size_t)0x01 << tree->table_bits) * sizeof(huffman_table_entry));

    /* Calculate the Huffman code for each symbol based on the code length for each symbol. This algorithm is described
     * in RFC 1951, section 3.2.2 */
    /* STEP 1: Calculate number of codes for each code length */
    memset(bitLengthCount, 0, sizeof(bitLengthCount));
    for (size_t i = 0; i < codeLengthsSize; ++i)
    {
        /* NOTE: Maximum code length is controlled by the number of bits we read, not user input */
        assert(codeLengths[i] < inflatelib_arraysize(bitLengthCount));
        ++bitLengthCount[codeLengths[i]];
    }

    /* STEP 2: Calculate the numerical value of the smallest code for each code length */
    nextCode = 0;
    for (size_t i = 1; i < inflatelib_arraysize(bitLengthCount); ++i)
    {
        nextCode <<= 1;
        nextCodes[i] = nextCode;

        /* It's possible that some malicious input specifies too many codes with a bit count than will actually fit. */
        /* NOTE: A max code length of 15 ensures this will never overflow (high bit always unused) */
        nextCode += bitLengthCount[i];
        if (nextCode > (0x01 << i))
        {
            if (format_error_message(
                    stream,
                    "Too many symbols with code length %zu. %u symbols starting at 0x%X exceeds the specified number of bits",
                    i,
                    bitLengthCount[i],
                    nextCodes[i]) < 0)
            {
                stream->error_msg = "Not all symbols can be represented using the specified number of bits";
            }
            errno = EINVAL;
            return INFLATELIB_ERROR_DATA;
        }
    }

    /* STEP 3: Assign numerical values to codes for symbols that are used in the table/tree */
    for (size_t i = 0; i < codeLengthsSize; ++i)
    {
        uint8_t len = codeLengths[i];
        if (len != 0)
        {
            size_t code = reverse_bits(nextCodes[len]++, len);
            assert((code & ((uint16_t)0xFFFF << len)) == 0); /* Should have errored above */

            if (len <= tree->table_bits)
            {
                /* This can fit in the table */
                uint16_t increment = 0x01 << len;

                assert((code & ~tree->table_mask) == 0); /* Sanity check; should be impossible if the above assert passes*/

                /* See comment in huffman_tree.h - need to set all entries whose indices begin with the code */
                while (code <= tree->table_mask)
                {
                    huffman_table_entry* entry = &tree->data[code];

                    assert(entry->code_length == 0); /* Impossible to be in use given how codes are calculated */
                    entry->code_length = len;
                    entry->symbol = (uint16_t)i;

                    code += increment;
                }
            }
            else
            {
                /* This needs to go into the binary tree portion */
                huffman_table_entry* entry = &tree->data[code & tree->table_mask];

                /* Should be impossible given how codes are calculated */
                assert((entry->code_length == 0) || (entry->code_length > tree->table_bits));

                code >>= tree->table_bits;
                for (size_t currentLen = tree->table_bits; currentLen < len; ++currentLen)
                {
                    if (entry->code_length == 0)
                    {
                        /* Not set yet; allocate space in the binary tree array */
                        assert(nextTreeInsertPtr <= (tree->data + tree->data_size)); /* Should be mathematically impossible */
                        entry->code_length = 0x0F; /* For simplicity; always indicates this is a pointer */
                        entry->symbol = nextTreeInsertIndex;

                        /* We need to initialize TWO elements because that's what we're effectively  */
                        nextTreeInsertPtr[0].code_length = 0;
                        nextTreeInsertPtr[1].code_length = 0;

                        entry = nextTreeInsertPtr + (code & 0x01);
                        ++nextTreeInsertIndex; /* Index is for pairs of entries */
                        nextTreeInsertPtr += 2;
                    }
                    else
                    {
                        /* Already set; due to how codes are assigned, it should be impossible for overlap */
                        assert(entry->code_length > currentLen);
                        assert(entry->symbol < nextTreeInsertIndex); /* Sanity check */
                        entry = treeBase + ((size_t)entry->symbol * 2) + (code & 0x01);
                    }

                    code >>= 1;
                }

                /* entry now points to where we're inserting the data */
                assert(entry->code_length == 0); /* Again, overlaps impossible due to how codes are assigned */
                entry->code_length = len;
                entry->symbol = (uint16_t)i;
            }
        }
    }

    return INFLATELIB_OK;
}

void huffman_tree_destroy(huffman_tree* tree, inflatelib_stream* stream)
{
    if (tree->data)
    {
        INFLATELIB_FREE(stream, huffman_table_entry, tree->data, tree->data_size);
        tree->data = NULL;
        tree->data_size = 0;
    }
}

int huffman_tree_lookup(huffman_tree* tree, inflatelib_stream* stream, uint16_t* symbol)
{
    bitstream* bitstream = &stream->internal->bitstream;
    huffman_table_entry* tableEntry;
    uint16_t inputRaw;
    size_t input, bits;

    bits = bitstream_peek(bitstream, &inputRaw);
    input = (size_t)inputRaw;
    tableEntry = &tree->data[input & tree->table_mask];
    if ((tableEntry->code_length > bits) && (bits <= tree->table_bits))
    {
        return 0; /* Not enough data */
    }

    if (tableEntry->code_length > tree->table_bits)
    {
        /* This is a "pointer" inside the tree */
        huffman_table_entry* tableBase = tree->data + ((size_t)0x01 << tree->table_bits);
        size_t bitsRead = tree->table_bits;
        size_t remainingInput = input >> tree->table_bits;

        do
        {
            assert(bits >= bitsRead); /* Otherwise, how'd we get this far? */
            if (bitsRead >= bits)     /* I.e. == */
            {
                return 0; /* Not enough data */
            }

            tableEntry = tableBase + (2 * (size_t)tableEntry->symbol) + (remainingInput & 0x01);
            assert(tableEntry < (tree->data + tree->data_size)); /* Otherwise data in the table is corrupt */
            ++bitsRead;
            remainingInput >>= 1;
        } while (tableEntry->code_length > bitsRead);

        assert((tableEntry->code_length == bitsRead) || !tableEntry->code_length); /* Otherwise we wrote bad data or indexed something wrong */
    }
    /* Otherwise, error or the data fit in the table and we have enough bits in the input to know that's the "full" code */

    if (tableEntry->code_length == 0)
    {
        /* Zero means unassigned; this is an error */
        if (format_error_message(
                stream,
                "Input bit sequence 0x%.*zX is not a valid Huffman code for the encoded table",
                (bits + 7) / 8,
                input & ((0x01 << bits) - 1)) < 0)
        {
            stream->error_msg = "Input bit sequence is not a valid Huffman code for the encoded table";
        }
        errno = EINVAL;
        return -1;
    }

    /* Success if we've gotten this far */
    *symbol = tableEntry->symbol;
    bitstream_consume_bits(bitstream, tableEntry->code_length);
    return 1;
}

int huffman_tree_lookup_unchecked(huffman_tree* tree, inflatelib_stream* stream, uint16_t* symbol)
{
    bitstream* bitstream = &stream->internal->bitstream;
    huffman_table_entry* tableEntry;
    size_t input;

    input = bitstream_peek_unchecked(bitstream);
    tableEntry = &tree->data[input & tree->table_mask];

    if (tableEntry->code_length > tree->table_bits)
    {
        /* This is a "pointer" inside the tree */
        huffman_table_entry* tableBase = tree->data + ((size_t)0x01 << tree->table_bits);
        size_t bitsRead = tree->table_bits;
        size_t remainingInput = input >> tree->table_bits;

        do
        {
            assert(bitsRead < 15); /* Largest code is 15 bits */

            tableEntry = tableBase + (2 * (size_t)tableEntry->symbol) + (remainingInput & 0x01);
            assert(tableEntry < (tree->data + tree->data_size)); /* Otherwise data in the table is corrupt */
            ++bitsRead;
            remainingInput >>= 1;
        } while (tableEntry->code_length > bitsRead);

        assert((tableEntry->code_length == bitsRead) || !tableEntry->code_length); /* Otherwise we wrote bad data or indexed something wrong */
    }
    /* Otherwise, error or the data fit in the table */

    if (tableEntry->code_length == 0)
    {
        /* Zero means unassigned; this is an error */
        if (format_error_message(stream, "Input bit sequence 0x%02zX is not a valid Huffman code for the encoded table", input) < 0)
        {
            stream->error_msg = "Input bit sequence is not a valid Huffman code for the encoded table";
        }
        errno = EINVAL;
        return -1;
    }

    /* Success if we've gotten this far */
    *symbol = tableEntry->symbol;
    bitstream_consume_bits(bitstream, tableEntry->code_length);
    return 1;
}

static inline uint16_t reverse_bits(uint16_t value, int bitCount)
{
    uint16_t result = 0;
    int count = 0;

    /* Flip 4 bits at a time, using a lookup table */
    /* NOTE: From            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F*/
    const uint8_t table[] = {0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E, 0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F};

    while (count < bitCount)
    {
        result = (result << 4) | table[value & 0x0F];
        value >>= 4;
        count += 4;
    }

    /* We may have "overshot" */
    result >>= (count - bitCount);
    return result;
}
