/*
 *    Copyright (c) Microsoft. All rights reserved.
 *    This code is licensed under the MIT License.
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 *    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *    PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
#ifndef INFLATELIB_STATE_H
#define INFLATELIB_STATE_H

#include <inflatelib.h>

#include <errno.h>
#include <stdalign.h>

#include "bitstream.h"
#include "huffman_tree.h"
#include "window.h"

#define inflatelib_arraysize(arr) (sizeof(arr) / sizeof(*arr))

typedef enum block_type
{
    /* NOTE: Values must be kept identical to how they appear in the format */
    btype_uncompressed = 0,
    btype_static = 1,
    btype_dynamic = 2,
} block_type;

typedef enum inflate_state
{
    /* NOTE: Outside of 'ifstate_init' which must be zero, the values of these states don't really matter, however they
     * relative order of some of them do. */
    ifstate_init = 0,

    ifstate_reading_bfinal,
    ifstate_reading_btype,

    /* States for 'btype_uncompressed' */
    ifstate_reading_uncompressed_block_len,
    ifstate_reading_uncompressed_block_len_complement,
    ifstate_reading_uncompressed_data,

    /* States specific to 'btype_dynamic' for setting up the Huffman tables */
    ifstate_reading_num_lit_codes,
    ifstate_reading_num_dist_codes,
    ifstate_reading_num_code_len_codes,
    ifstate_reading_code_len_codes,
    ifstate_reading_tree_codes_before,
    ifstate_reading_tree_codes_after,

    /* States shared for both 'btype_static' and 'btype_dynamic' */
    ifstate_reading_literal_length_code,
    ifstate_decoding_literal_length_code,
    ifstate_reading_length_extra_bits,
    ifstate_reading_distance_code,
    ifstate_reading_distance_extra_bits,
    ifstate_copying_length_distance_from_window,
    ifstate_copying_output_from_window,

    /* Indicates we've finished reading the last block */
    ifstate_eof,
} inflate_state;

#define INFLATELIB_MODE_DEFLATE 0x0000
#define INFLATELIB_MODE_DEFLATE64 0x0001

/* Internal state */
typedef struct inflatelib_state
{
    struct bitstream bitstream;
    struct window window;

    /* Formatted (allocated) error message */
    char* error_msg_fmt;
    size_t error_msg_len;

    /* Inflater state */
    inflate_state ifstate;
    uint8_t mode : 1;  /* See 'INFLATELIB_MODE*' for possible values */
    uint8_t btype : 2; /* block_type, but 'block_type' is signed and any value gretaer than 1 is negative... */
    uint8_t bfinal : 1;
    uint8_t need_more_data : 1; /* Set when we are terminating due to not enough input data & we need to mark all as consumed */

    /* Compressed block state */
    huffman_tree code_length_tree;
    huffman_tree literal_length_tree;
    huffman_tree distance_tree;

    /* Reusable data, depending on the operation being done */
    union
    {
        /* Info when reading an uncompressed block */
        struct
        {
            uint16_t block_len;
        } uncompressed;

        /* Info when reading dynamic Huffman codes */
        struct
        {
            uint16_t literal_length_code_count : 9; /* HLIT (257-288) */
            uint8_t distance_code_count : 6;        /* HDIST (1-32) */
            uint8_t code_length_code_count : 5;     /* HCLEN (4-19) */

            /* The last symbol read from the code length tree (0-18) */
            uint8_t length_code : 5;

            /* Loop counter that persists across multiple calls. This needs to control loops that range from 0 to 320 */
            uint32_t loop_counter : 9;

            /* We only need the array to initialize the Huffman trees, so we don't need to keep 3 around. However,
             * distance and literal/length codes are specified in "one chunk" which dictates the array size */
            uint8_t code_lengths[LITERAL_TREE_MAX_ELEMENT_COUNT + DIST_TREE_MAX_ELEMENT_COUNT];
        } dynamic_codes;

        /* Info when reading compressed blocks */
        struct
        {
            uint8_t extra_bits;
            uint16_t symbol;
            uint32_t block_length;
            uint32_t block_distance;
        } compressed;
    } data;
} inflatelib_state;

int format_error_message(inflatelib_stream* stream, const char* fmt, ...);

#define INFLATELIB_ALLOC(stream, type, count) (type*)stream->alloc(stream->user_data, sizeof(type) * count, alignof(type))
#define INFLATELIB_FREE(stream, type, ptr, count) stream->free(stream->user_data, ptr, sizeof(type) * count, alignof(type))

#ifdef __has_builtin
#if __has_builtin(__builtin_unreachable)
#define INFLATELIB_UNREACHABLE() __builtin_unreachable()
#endif
#endif

#ifndef INFLATELIB_UNREACHABLE
#ifdef _MSC_VER
#define INFLATELIB_UNREACHABLE() __assume(0)
#else
#define INFLATELIB_UNREACHABLE()
#endif
#endif

#endif
