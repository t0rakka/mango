/*
 *    Copyright (c) Microsoft. All rights reserved.
 *    This code is licensed under the MIT License.
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 *    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *    PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
#ifndef INFLATELIB_WINDOW_H
#define INFLATELIB_WINDOW_H

#include <stdint.h>

#include "bitstream.h"

/* Deflate64 allows up to a 64k offset and up to a 64k length. In theory, we can get away with a single 64k buffer,
 * however we allocate twice that to simplify our logic and don't have to worry about overlapping reads and writes */
#define DEFLATE64_WINDOW_SIZE 0x10000
#define DEFLATE64_WINDOW_MASK 0x0FFFF

#ifdef __cplusplus
// Needed for the tests
extern "C"
{
#endif

    /* Conceptually, data gets written in two steps: input data gets written to the window and then that data gets
     * written to the output. These two steps may or may not happen concurrently. For example, uncompressed data may get
     * read in chunks (i.e. not all at once) and is written to the output as it is read, however compressed data must
     * first be read in full (either literal or length/distance pairs), which then gets written in full before being
     * written to the output */
    typedef struct window
    {
        /* Ultimately, we only _need_ two values: a read or write offset and an "unconsumed" count. We maintain both a
         * read and write offset so that we don't have to continuously re-calculate one or the other */
        uint16_t read_offset;
        uint16_t write_offset;

        /* NOTE: We can't infer this from the two offsets because it's possible the window is full and therefore the two
         * offsets are the same (would be ambiguous if empty or full). This is also why this can't be 16-bits */
        size_t unconsumed_bytes;

        /* Total bytes written to 'data' that should be considered valid. This is used to ensure that a length/distance
         * pair does not refer to garbage data. This may be larger than the buffer size, which is okay; we still enforce
         * the maximum read size. This is primarily an optimization so that we don't have to constantly verify that this
         * value is greater than the buffer size */
        uintmax_t total_bytes;

        uint8_t data[DEFLATE64_WINDOW_SIZE];
    } window;

    void window_init(window* window);
    void window_reset(window* window);

    /* Copies up to 'outputSize' bytes to 'output', returning the number of bytes that were copied */
    size_t window_copy_output(window* window, uint8_t* output, size_t outputSize);

    /* Attempts to copy 'count' bytes from the bitstream into the window, returning the number of bytes successfully
     * copied */
    size_t window_copy_bytes(window* window, bitstream* bitstream, size_t count);

    /* Copies at most 'length' bytes from the window back to itself starting at the negative offset of 'distance',
     * returning the number of bytes that were successfully copied (e.g. before running out of unconsumed space) */
    int window_copy_length_distance(window* window, size_t distance, size_t length);

    /* Writes a single byte to the window */
    int window_write_byte(window* window, uint8_t byte);

    /* Same as the above, only it assumes that 'unconsumed_bytes' is 0 and it immediately consumes the byte */
    void window_write_byte_consume(window* window, uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif // INFLATELIB_WINDOW_H
