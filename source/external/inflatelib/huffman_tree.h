/*
 *    Copyright (c) Microsoft. All rights reserved.
 *    This code is licensed under the MIT License.
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 *    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *    PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
#ifndef INFLATELIB_HUFFMAN_TREE_H
#define INFLATELIB_HUFFMAN_TREE_H

#include <stdint.h>

// NOTE: We can't include 'internal.h' since it includes us, so forward declare what we need
struct inflatelib_stream;

/* Code length codes are encoded using 3 bits (length of 0-7 bits), and the code length alphabet, which defines how the
 * literal/length code lengths are defined, only allows values 0-15 to be defined, as per RFC 1951, section 3.2.7 */
#define MAX_CODE_LENGTH 15

/* NOTE: RFC 1951 defines a max literal/length symbol of 285 (for a maximum of 286 codes), however HLIT is represented
 * using 5 bits (0-31). Adding the additional 257 yields a representable maximum of 288 codes (symbol range 0-287). We
 * define using this representable maximum as opposed to the documented maximum to ensure our buffers are safe without
 * needing to check HLIT's validity */
/* TODO: This has the downside that we therefore need to check each symbol's value against this maximum before using it.
 * Perf-wise, this tradeoff may not be worth it, however doing it this way has the nice side effect that we won't error
 * out on data that might end up being valid (e.g. if the code sizes for symbols 286 and 287 are both zero, or if these
 * symbols don't occur in the compressed portion of the data).
 * It's also worth noting that RFC 1951, section 3.2.6 specifies 288 code lengths for the static literal/length tree,
 * which is consistent with what we specify here. */
#define LITERAL_TREE_MAX_ELEMENT_COUNT 288
#define DIST_TREE_MAX_ELEMENT_COUNT 32
#define CODE_LENGTH_TREE_ELEMENT_COUNT 19

#ifdef __cplusplus
// Needed for the tests
extern "C"
{
#endif

    /*
     * All data is stored in a single table, however the meaning of each entry, as well as how to access each entry,
     * depends on where in the table each entry resides. Conceptually, the table looks like the following:
     *
     *          Lookup Table        Binary Tree
     *      +-------------------+-------------------+
     *      |   |   |  ...  |   |   |  ...  |   |   |
     *      +-------------------+-------------------+
     *      |<-- 2^N elements ->|
     *
     * N - the size of the lookup table - is heuristically chosen based on the size of the alphabet that the Huffman
     * tree is meant to encode. Huffman codes that are N bits or less have their data stored directly in the lookup
     * table. Codes that are less than N bits are stored at 2^(N-len) locations in the table: one for each index where
     * the lower bits match the (reversed) code. For example, if some symbol has the Huffman code '110', then all
     * indices that begin with '011' will reference that symbol: 0000011, 0001011, 0010011, 0011011, ..., 1111011. For
     * codes that are longer than N bits, the table entry indexed by the first N bits (reversed) will reference an
     * offset in the binary tree portion of the array divided by two. E.g. a value of '5' would yield the index created
     * by adding the length of the lookup table with 10. The next bit indicates which index this offset refers to: index
     * 10 for '0' and index 11 for '1' in the above example. This continues until a terminal node is encountered.
     */

    typedef struct huffman_table_entry
    {
        /* The code length is needed to ensure we consume the proper number of bits and to ensure we don't consume a
         * value too early. A value of 0 means invalid/unused. In the lookup table, a value greater than the table
         * bit-size means that 'symbol' is an offset in the binary tree array. In the binary tree table, a value equal
         * to the current bit count required to get that far means that the node is a leaf node and 'symbol' is the
         * final value, whereas a value greater than the current bit count means that 'symbol' is another index into the
         * binary tree array. */
        uint16_t code_length;
        uint16_t symbol;
    } huffman_table_entry;

    typedef struct huffman_tree
    {
        size_t table_bits;         /* Either 7 or 9; see huffman_tree.c for more details */
        size_t table_mask;         /* = (1 << table_bits) - 1 */
        size_t data_size;          /* For assertions & deallocation; it's mathematically impossible to read/write past the end */
        huffman_table_entry* data; /* See above for data layout */
    } huffman_tree;

    /* The order of calls must follow: init, reset, reset, ..., reset, destroy */
    int huffman_tree_init(huffman_tree* tree, struct inflatelib_stream* stream, size_t dictionarySize);
    int huffman_tree_reset(huffman_tree* tree, struct inflatelib_stream* stream, const uint8_t* codeLengths, size_t codeLengthsSize);
    void huffman_tree_destroy(huffman_tree* tree, struct inflatelib_stream* stream);

    /* Looks up a symbol from the table, returning -1 on failure (symbol does not exist), 0 if not enough input, and 1
     * on success. The difference between these two functions is that the 'unchecked' function assumes there's enough
     * bits in the input to read any given symbol. */
    int huffman_tree_lookup(huffman_tree* tree, struct inflatelib_stream* stream, uint16_t* symbol);
    int huffman_tree_lookup_unchecked(huffman_tree* tree, struct inflatelib_stream* stream, uint16_t* symbol);

#ifdef __cplusplus
}
#endif

#endif
