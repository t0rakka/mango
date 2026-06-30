/*
 *    Copyright (c) Microsoft. All rights reserved.
 *    This code is licensed under the MIT License.
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 *    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *    PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

static void* inflatelib_default_alloc(void* unusedUserData, size_t bytes, size_t alignment)
{
    void* result;

    (void)unusedUserData; /* C doesn't allow unnamed parameters */
    (void)alignment;      /* Currently, we don't require any alignment that malloc can't provide */

    result = malloc(bytes);
    assert(((uintptr_t)result % alignment) == 0);

    return result;
}

static void inflatelib_default_free(void* unusedUserData, void* ptr, size_t bytes, size_t alignment)
{
    (void)unusedUserData; /* C doesn't allow unnamed parameters */
    (void)bytes;          /* free does not need size/alignment information */
    (void)alignment;
    free(ptr);
}

int inflatelib_init(inflatelib_stream* stream)
{
    int result;
    inflatelib_state* state;

    /* Start with no error message, in case it was set before (or contains uninitialized memory) */
    stream->error_msg = NULL;

    /* Setup allocation functions */
    if (stream->alloc == NULL)
    {
        stream->alloc = inflatelib_default_alloc;
    }
    if (stream->free == NULL)
    {
        stream->free = inflatelib_default_free;
    }

    /* Setup our internal state */
    state = INFLATELIB_ALLOC(stream, inflatelib_state, 1);
    if (state == NULL)
    {
        stream->error_msg = "Failed to allocate storage for internal state";
        errno = ENOMEM;
        return INFLATELIB_ERROR_OOM;
    }

    memset(state, 0, sizeof(*state));
    stream->internal = state;

    result = huffman_tree_init(&state->code_length_tree, stream, CODE_LENGTH_TREE_ELEMENT_COUNT);
    if (result >= 0)
    {
        result = huffman_tree_init(&state->literal_length_tree, stream, LITERAL_TREE_MAX_ELEMENT_COUNT);
    }
    if (result >= 0)
    {
        result = huffman_tree_init(&state->distance_tree, stream, DIST_TREE_MAX_ELEMENT_COUNT);
    }

    if (result >= 0)
    {
        bitstream_init(&state->bitstream);
        window_init(&state->window);

        state->ifstate = ifstate_init;
    }

    if (result < 0)
    {
        /* This will take care of deallocating any allocated memory */
        inflatelib_destroy(stream);
        return result;
    }

    return INFLATELIB_OK;
}

int inflatelib_reset(inflatelib_stream* stream)
{
    inflatelib_state* state = stream->internal;

    if (state == NULL)
    {
        stream->error_msg = "Internal state is null; ensure inflatelib_init has been called first";
        errno = EINVAL;
        return INFLATELIB_ERROR_ARG;
    }

    bitstream_reset(&state->bitstream);
    window_reset(&state->window);

    // NOTE: The Huffman trees do not need to be reset as they are reset on demand as needed. If we've made it this far,
    // all of their internal state has been allocated, and that's the best that we can ask for

    state->ifstate = ifstate_init;

    return INFLATELIB_OK;
}

int inflatelib_destroy(inflatelib_stream* stream)
{
    inflatelib_state* state = stream->internal;

    /* NOTE: It should not be possible for the pointer to be non-null unless alloc/free were initialized, at least so
             long as the caller zero-initialized the pointer */
    if (state)
    {
        if (state->error_msg_fmt)
        {
            if (stream->error_msg == state->error_msg_fmt)
            {
                /* Don't leave a dangling pointer, however also don't fully null out the error message in case the
                   caller is depending on being able to read it. Realistically, this should never cause problems in
                   practice as we should never need to format an error message AND destroy the stream in one op */
                stream->error_msg = "Generic failure";
            }

            INFLATELIB_FREE(stream, char, state->error_msg_fmt, state->error_msg_len);
        }

        huffman_tree_destroy(&state->code_length_tree, stream);
        huffman_tree_destroy(&state->literal_length_tree, stream);
        huffman_tree_destroy(&state->distance_tree, stream);

        INFLATELIB_FREE(stream, inflatelib_state, stream->internal, 1);
        stream->internal = NULL;
    }

    return INFLATELIB_OK;
}

int format_error_message(inflatelib_stream* stream, const char* fmt, ...)
{
    va_list args;
    int bufferSize;
    inflatelib_state* state = stream->internal;

    /* Before starting, clear out the previously allocated error message, if present */
    if (state->error_msg_fmt)
    {
        if (stream->error_msg == state->error_msg_fmt)
        {
            stream->error_msg = NULL;
        }

        INFLATELIB_FREE(stream, char, state->error_msg_fmt, state->error_msg_len);
        state->error_msg_fmt = NULL;
    }

    va_start(args, fmt);
    bufferSize = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (bufferSize < 0)
    {
        stream->error_msg = "Failed to format error message";
        return INFLATELIB_ERROR_ARG;
    }

    ++bufferSize; /* Return value does not include space for null terminator */

    state->error_msg_fmt = INFLATELIB_ALLOC(stream, char, bufferSize);
    if (!state->error_msg_fmt)
    {
        stream->error_msg = "Failed to allocate space for error message";
        errno = ENOMEM;
        return INFLATELIB_ERROR_OOM;
    }
    state->error_msg_len = bufferSize;

    va_start(args, fmt);
    bufferSize = vsnprintf(state->error_msg_fmt, bufferSize, fmt, args);
    va_end(args);

    if (bufferSize < 0)
    {
        stream->error_msg = "Failed to format error message";
        return INFLATELIB_ERROR_ARG;
    }

    stream->error_msg = state->error_msg_fmt;
    return INFLATELIB_OK;
}

static int inflater_process_data(inflatelib_stream* stream);
static int inflater_read_uncompressed(inflatelib_stream* stream);
static void inflater_init_static_tables(inflatelib_stream* stream);
static int inflater_read_dynamic_header(inflatelib_stream* stream);
static int inflater_read_compressed(inflatelib_stream* stream);

static int do_inflate(inflatelib_stream* stream)
{
    int result;
    inflatelib_state* state = stream->internal;
    const uint8_t *finalInData, *initialInData = (const uint8_t*)stream->next_in;
    size_t finalInSize, initialInSize = stream->avail_in, initialOutSize = stream->avail_out;

    assert(state->ifstate != ifstate_init);
    state->need_more_data = 0;

    /* The last call to inflatelib_inflate* may not have read all data, e.g. if we've filled up the output buffer,
     * however we should have reset the buffer to avoid the dangling pointer */
    bitstream_set_data(&state->bitstream, initialInData, initialInSize);

    result = inflater_process_data(stream);

    /* When making it this far, we've potentially read/written data that we want to report, even on failure */
    stream->total_out += initialOutSize - stream->avail_out;

    /* NOTE: In the event of error, we don't know how many bits were needed to surface said error. Just assume that all bits we've
     * read thus far were necessary, so don't reclaim in that case */
    bitstream_clear_data(&state->bitstream, !state->need_more_data && (result >= 0), &finalInData, &finalInSize);
    assert(finalInData >= initialInData);
    assert(finalInSize <= initialInSize);

    stream->total_in += stream->avail_in - finalInSize;
    stream->next_in = finalInData;
    stream->avail_in = finalInSize;

    return result;
}

int inflatelib_inflate(inflatelib_stream* stream)
{
    inflatelib_state* state = stream->internal;

    if (state == NULL)
    {
        stream->error_msg = "Internal state is null; ensure inflatelib_init has been called first";
        errno = EINVAL;
        return INFLATELIB_ERROR_ARG;
    }

    /* Ensure that we're not mixing inflate/inflate64 calls */
    switch (state->ifstate)
    {
    case ifstate_init:
        /* Not yet initialized */
        state->mode = INFLATELIB_MODE_DEFLATE;
        state->ifstate = ifstate_reading_bfinal;
        break;

    default:
        /* Already initialized */
        if (state->mode != INFLATELIB_MODE_DEFLATE)
        {
            stream->error_msg =
                "inflatelib_stream is initialized for Deflate64 and cannot be called with Deflate encoded data. First call inflatelib_reset to reset the stream";
            errno = EINVAL;
            return INFLATELIB_ERROR_ARG;
        }
        break;
    }

    return do_inflate(stream);
}

int inflatelib_inflate64(inflatelib_stream* stream)
{
    inflatelib_state* state = stream->internal;

    if (state == NULL)
    {
        stream->error_msg = "Internal state is null; ensure inflatelib_init has been called first";
        errno = EINVAL;
        return INFLATELIB_ERROR_ARG;
    }

    /* Ensure that we're not mixing inflate/inflate64 calls */
    switch (state->ifstate)
    {
    case ifstate_init:
        /* Not yet initialized */
        state->mode = INFLATELIB_MODE_DEFLATE64;
        state->ifstate = ifstate_reading_bfinal;
        break;

    default:
        /* Already initialized */
        if (state->mode != INFLATELIB_MODE_DEFLATE64)
        {
            stream->error_msg =
                "inflatelib_stream is initialized for Deflate and cannot be called with Deflate64 encoded data. First call inflatelib_reset to reset the stream";
            errno = EINVAL;
            return INFLATELIB_ERROR_ARG;
        }
        break;
    }

    return do_inflate(stream);
}

static int inflater_process_data(inflatelib_stream* stream)
{
    inflatelib_state* state = stream->internal;
    int result;
    uint16_t data;

    do
    {
        switch (state->ifstate)
        {
        case ifstate_reading_bfinal:
            if (!bitstream_read_bits(&state->bitstream, 1, &data))
            {
                state->need_more_data = 1;
                return INFLATELIB_OK; /* Not enough input data */
            }

            state->bfinal = (uint8_t)data;
            state->ifstate = ifstate_reading_btype;
            /* Fallthrough */

        case ifstate_reading_btype:
            if (!bitstream_read_bits(&state->bitstream, 2, &data))
            {
                state->need_more_data = 1;
                return INFLATELIB_OK; /* Not enough input data */
            }
            else if (data > 2)
            {
                if (format_error_message(stream, "Unexpected block type '%u'", data) < 0)
                {
                    stream->error_msg = "Unexpected block type";
                }
                errno = EINVAL;
                return INFLATELIB_ERROR_DATA;
            }

            state->btype = (block_type)data;
            switch (state->btype)
            {
            case btype_uncompressed:
                bitstream_byte_align(&state->bitstream);
                state->ifstate = ifstate_reading_uncompressed_block_len;
                break;

            case btype_static:
                inflater_init_static_tables(stream);
                state->ifstate = ifstate_reading_literal_length_code;
                break;

            case btype_dynamic:
                state->ifstate = ifstate_reading_num_lit_codes;
                break;
            }
            break; /* Handled below */

        case ifstate_eof:
            assert(state->need_more_data == 0); /* Unused data needs to go back to the caller */
            return INFLATELIB_EOF;              /* Already read all data */

        default:
            /* Otherwise, 'btype' is known & we're in the process of decoding; handled below */
            break;
        }

        switch (state->btype)
        {
        case btype_uncompressed:
            result = inflater_read_uncompressed(stream);
            break;

        default:
            assert(0); /* Otherwise invalid block_type */
            INFLATELIB_UNREACHABLE();
        case btype_dynamic:
            if (state->ifstate < ifstate_reading_literal_length_code)
            {
                /* We have not fully initialized the dynamic Huffman tables yet */
                result = inflater_read_dynamic_header(stream);
                if (result < 0)
                {
                    return result; /* Error string, etc. already set */
                }
                else if (state->ifstate < ifstate_reading_literal_length_code)
                {
                    assert(state->need_more_data == 1); /* inflater_read_dynamic_header should have set this */
                    return INFLATELIB_OK;               /* Not enough input data */
                }
            }
            /* Fallthrough */

        case btype_static:
            result = inflater_read_compressed(stream);
            break;
        }
    } while ((result == INFLATELIB_OK) && (state->ifstate == ifstate_reading_bfinal));

    if ((result == INFLATELIB_OK) && (state->ifstate == ifstate_eof))
    {
        result = INFLATELIB_EOF;
    }

    return result;
}

static int inflater_read_uncompressed(inflatelib_stream* stream)
{
    inflatelib_state* state = stream->internal;
    size_t bytesCopied;
    uint16_t data;

    assert(state->btype == btype_uncompressed);

    switch (state->ifstate)
    {
    case ifstate_reading_uncompressed_block_len:
        if (!bitstream_read_bits(&state->bitstream, 16, &data))
        {
            state->need_more_data = 1;
            return INFLATELIB_OK; /* Not enough input data */
        }

        state->data.uncompressed.block_len = data;
        state->ifstate = ifstate_reading_uncompressed_block_len_complement;
        /* Fallthrough */

    case ifstate_reading_uncompressed_block_len_complement:
        if (!bitstream_read_bits(&state->bitstream, 16, &data))
        {
            state->need_more_data = 1;
            return INFLATELIB_OK; /* Not enough data */
        }

        if (state->data.uncompressed.block_len != (uint16_t)~data)
        {
            if (format_error_message(
                    stream,
                    "Uncompressed block length (%04X) does not match its encoded one's complement value (%04X)",
                    state->data.uncompressed.block_len,
                    data) < 0)
            {
                stream->error_msg = "Uncompressed block length does not match its encoded one's complement value";
            }
            errno = EINVAL;
            return INFLATELIB_ERROR_DATA;
        }

        state->ifstate = ifstate_reading_uncompressed_data;
        /* Fallthrough */

    case ifstate_reading_uncompressed_data:
        /* NOTE: Both these function calls are safe to call with sizes of zero */
        state->data.uncompressed.block_len -=
            (uint16_t)window_copy_bytes(&state->window, &state->bitstream, state->data.uncompressed.block_len);

        bytesCopied = window_copy_output(&state->window, (uint8_t*)stream->next_out, stream->avail_out);
        stream->next_out = (uint8_t*)stream->next_out + bytesCopied;
        stream->avail_out -= bytesCopied;

        /* It's safe to continue only if we've read and written all data */
        if ((state->data.uncompressed.block_len == 0) && (state->window.unconsumed_bytes == 0))
        {
            state->ifstate = state->bfinal ? ifstate_eof : ifstate_reading_bfinal;
        }
        break;

    default:
        assert(0); /* Invalid state for 'btype_uncompressed' */
        INFLATELIB_UNREACHABLE();
    }

    return INFLATELIB_OK;
}

static void inflater_init_static_tables(inflatelib_stream* stream)
{
    int result;
    uint8_t buffer[LITERAL_TREE_MAX_ELEMENT_COUNT];
    inflatelib_state* state = stream->internal;

    (void)result; /* Only used for asserts */

    /* TODO: We can encode both of these tables in static data; it's not clear yet if/how much that might improve things
     * and all indications are that this code path is insignificant enough to warrent such optimizations */

    /* The static literal/length code lengths are specified by RFC 1951, section 3.2.6 as follows: */
    memset(buffer, 8, 144 - 0);         /* 0-143: 8 bits long */
    memset(buffer + 144, 9, 256 - 144); /* 144-255: 9 bits long */
    memset(buffer + 256, 7, 280 - 256); /* 256-279: 7 bits long */
    memset(buffer + 280, 8, 288 - 280); /* 280-287: 8 bits long */

    result = huffman_tree_reset(&state->literal_length_tree, stream, buffer, 288);
    assert(result == 0); /* We control the inputs; this can never fail */

    /* The distance code lengths are also specified by RFC 1951, section 3.2.6 as being 5 bits each */
    memset(buffer, 5, 32);

    result = huffman_tree_reset(&state->distance_tree, stream, buffer, 32);
    assert(result == 0); /* We control the inputs; this can never fail */
}

/* The order that the code length alphabe's code lengths are specified in, as per RFC 1951, section 3.2.7 */
static const uint8_t code_order[CODE_LENGTH_TREE_ELEMENT_COUNT] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

static int inflater_read_dynamic_header(inflatelib_stream* stream)
{
    int result = INFLATELIB_OK;
    inflatelib_state* state = stream->internal;
    uint16_t data, codeArraySize;
    uint8_t prevCode;

    assert(state->btype == btype_dynamic);

    switch (state->ifstate)
    {
    case ifstate_reading_num_lit_codes:
        if (!bitstream_read_bits(&state->bitstream, 5, &data))
        {
            state->need_more_data = 1;
            return INFLATELIB_OK; /* Not enough input data */
        }
        state->data.dynamic_codes.literal_length_code_count = data + 257;
        /* Fallthrough */

    case ifstate_reading_num_dist_codes:
        if (!bitstream_read_bits(&state->bitstream, 5, &data))
        {
            state->need_more_data = 1;
            state->ifstate = ifstate_reading_num_dist_codes;
            return INFLATELIB_OK; /* Not enough input data */
        }
        state->data.dynamic_codes.distance_code_count = (uint8_t)(data + 1);
        /* Fallthrough */

    case ifstate_reading_num_code_len_codes:
        if (!bitstream_read_bits(&state->bitstream, 4, &data))
        {
            state->need_more_data = 1;
            state->ifstate = ifstate_reading_num_code_len_codes;
            return INFLATELIB_OK; /* Not enough input data */
        }
        state->data.dynamic_codes.code_length_code_count = (uint8_t)(data + 4);
        state->data.dynamic_codes.loop_counter = 0;
        /* Fallthrough */

    case ifstate_reading_code_len_codes:
        /* NOTE: Should be impossible since we read 4 bits (0-15) then add 4 */
        assert(state->data.dynamic_codes.code_length_code_count <= CODE_LENGTH_TREE_ELEMENT_COUNT);
        while (state->data.dynamic_codes.loop_counter < state->data.dynamic_codes.code_length_code_count)
        {
            if (!bitstream_read_bits(&state->bitstream, 3, &data))
            {
                state->need_more_data = 1;
                state->ifstate = ifstate_reading_code_len_codes;
                return INFLATELIB_OK; /* Not enough input data */
            }

            state->data.dynamic_codes.code_lengths[code_order[state->data.dynamic_codes.loop_counter]] = (uint8_t)data;
            ++state->data.dynamic_codes.loop_counter;
        }

        /* Fill rest of the array with zeroes */
        while (state->data.dynamic_codes.loop_counter < inflatelib_arraysize(code_order))
        {
            state->data.dynamic_codes.code_lengths[code_order[state->data.dynamic_codes.loop_counter]] = 0;
            ++state->data.dynamic_codes.loop_counter;
        }

        result = huffman_tree_reset(&state->code_length_tree, stream, state->data.dynamic_codes.code_lengths, CODE_LENGTH_TREE_ELEMENT_COUNT);
        if (result < 0)
        {
            state->ifstate = ifstate_reading_code_len_codes; /* TODO: Error state? */
            return result;                                   /* Error message, etc. already set */
        }

        state->data.dynamic_codes.loop_counter = 0; /* Reset for next operation */
        state->ifstate = ifstate_reading_tree_codes_before;
        /* Fallthrough */

    case ifstate_reading_tree_codes_before:
    case ifstate_reading_tree_codes_after:
        codeArraySize = state->data.dynamic_codes.literal_length_code_count + state->data.dynamic_codes.distance_code_count;
        assert(codeArraySize <= inflatelib_arraysize(state->data.dynamic_codes.code_lengths));
        while (state->data.dynamic_codes.loop_counter < codeArraySize)
        {
            if (state->ifstate == ifstate_reading_tree_codes_before)
            {
                result = huffman_tree_lookup(&state->code_length_tree, stream, &data);
                if (!result)
                {
                    state->need_more_data = 1;
                    return INFLATELIB_OK; /* Not enough input data */
                }
                else if (result < 0)
                {
                    return INFLATELIB_ERROR_DATA; /* Error message, etc. already set */
                }

                state->data.dynamic_codes.length_code = (uint8_t)data;
            }

            /* Decode values from the code length array, as specified by RFC 1951, section 3.2.7 */
            if (state->data.dynamic_codes.length_code <= 15)
            {
                /* Literal value */
                state->data.dynamic_codes.code_lengths[state->data.dynamic_codes.loop_counter++] = state->data.dynamic_codes.length_code;
            }
            else if (state->data.dynamic_codes.length_code == 16)
            {
                /* Repeat the previous code length 3-6 times as specified by the next two bits */
                if (!bitstream_read_bits(&state->bitstream, 2, &data))
                {
                    /* Not enough input data; ensure we don't read a new length code next time */
                    state->need_more_data = 1;
                    state->ifstate = ifstate_reading_tree_codes_after;
                    return INFLATELIB_OK;
                }

                if (state->data.dynamic_codes.loop_counter == 0)
                {
                    stream->error_msg = "Code length repeat code encountered at beginning of data";
                    errno = EINVAL;
                    return INFLATELIB_ERROR_DATA;
                }
                prevCode = state->data.dynamic_codes.code_lengths[state->data.dynamic_codes.loop_counter - 1];

                data += 3;
                if ((state->data.dynamic_codes.loop_counter + data) > codeArraySize)
                {
                    if (format_error_message(
                            stream,
                            "Code length repeat code specifies %u repetitions, but only %u codes remain",
                            data,
                            codeArraySize - state->data.dynamic_codes.loop_counter) < 0)
                    {
                        stream->error_msg = "Code length repeat code specifies more repetitions than codes remain";
                    }
                    errno = EINVAL;
                    return INFLATELIB_ERROR_DATA;
                }

                for (uint16_t i = 0; i < data; ++i)
                {
                    state->data.dynamic_codes.code_lengths[state->data.dynamic_codes.loop_counter++] = prevCode;
                }
            }
            else
            {
                /* Repeat zero some number of times */
                size_t bitCount;
                uint16_t repeatBase;
                if (state->data.dynamic_codes.length_code == 17)
                {
                    /* Repeat '0' 3-10 times as specified by the next 3 bits */
                    bitCount = 3;
                    repeatBase = 3;
                }
                else
                {
                    /* Repeat '0' 11-138 times*/
                    assert(state->data.dynamic_codes.length_code == 18);
                    bitCount = 7;
                    repeatBase = 11;
                }

                if (!bitstream_read_bits(&state->bitstream, bitCount, &data))
                {
                    /* Not enough input data; ensure we don't read a new length code next time */
                    state->need_more_data = 1;
                    state->ifstate = ifstate_reading_tree_codes_after;
                    return INFLATELIB_OK;
                }

                data += repeatBase;
                if ((state->data.dynamic_codes.loop_counter + data) > codeArraySize)
                {
                    if (format_error_message(
                            stream,
                            "Zero repeat code specifies %u repetitions, but only %u codes remain",
                            data,
                            codeArraySize - state->data.dynamic_codes.loop_counter) < 0)
                    {
                        stream->error_msg = "Zero repeat code specifies more repetitions than codes remain";
                    }
                    errno = EINVAL;
                    return INFLATELIB_ERROR_DATA;
                }

                for (uint16_t i = 0; i < data; ++i)
                {
                    state->data.dynamic_codes.code_lengths[state->data.dynamic_codes.loop_counter++] = 0;
                }
            }

            /* If we get this far, it means that we're done with the current code and ready for the next one */
            state->ifstate = ifstate_reading_tree_codes_before;
        }

        /* If we break out of the loop, we're done reading the code lengths arrays and are ready to init & move on */
        result = huffman_tree_reset(
            &state->literal_length_tree, stream, state->data.dynamic_codes.code_lengths, state->data.dynamic_codes.literal_length_code_count);
        if (result < 0)
        {
            return result; /* Error message, etc. already set */
        }

        result = huffman_tree_reset(
            &state->distance_tree,
            stream,
            state->data.dynamic_codes.code_lengths + state->data.dynamic_codes.literal_length_code_count,
            state->data.dynamic_codes.distance_code_count);
        if (result < 0)
        {
            return result; /* Error message, etc. already set */
        }

        state->ifstate = ifstate_reading_literal_length_code;
        break;

    default:
        assert(0); /* Otherwise should not have called this function */
        INFLATELIB_UNREACHABLE();
    }

    return INFLATELIB_OK;
}

typedef struct
{
    /* The data for reading encoded lengths. For some symbol N, N >= 257, the length of the block is:
     * table.lengths[N - 257].base + bitstream_read_bits(..., table.lengths[N - 257].extra_bits) */
    struct
    {
        uint16_t base;
        uint8_t extra_bits;
    } lengths[29];

    /* The data for reading encoded distances. For some symbol N, 0 <= N <= 31, the distance is:
     * table.distances[N].base + bitstream_read_bits(..., table.distances[N].extra_bits) */
    struct
    {
        uint16_t base;
        uint8_t extra_bits;
    } distances[32];
} inflater_tables;

static const inflater_tables deflate_tables = {
    .lengths = {{3, 0},  {4, 0},  {5, 0},  {6, 0},   {7, 0},   {8, 0},   {9, 0},   {10, 0},  {11, 1}, {13, 1},
                {15, 1}, {17, 1}, {19, 2}, {23, 2},  {27, 2},  {31, 2},  {35, 3},  {43, 3},  {51, 3}, {59, 3},
                {67, 4}, {83, 4}, {99, 4}, {115, 4}, {131, 5}, {163, 5}, {195, 5}, {227, 5}, {258, 0}},
    /* NOTE: We choose an size of 32 for the distances array because that's what Deflate64 needs, however Deflate only
     * makes use of the first 30. That said, HDIST is 5 bits, meaning it's possible to specify Huffman codes for symbols
     * 30 and 31, even for Deflate, so to make things simpler, we include entries for them here, but define their bases
     * to zero so that we can identify these error conditions */
    .distances = {{1, 0},     {2, 0},     {3, 0},     {4, 0},      {5, 1},      {7, 1},      {9, 2},     {13, 2},
                  {17, 3},    {25, 3},    {33, 4},    {49, 4},     {65, 5},     {97, 5},     {129, 6},   {193, 6},
                  {257, 7},   {385, 7},   {513, 8},   {769, 8},    {1025, 9},   {1537, 9},   {2049, 10}, {3073, 10},
                  {4097, 11}, {6145, 11}, {8193, 12}, {12289, 12}, {16385, 13}, {24577, 13}, {0, 0},     {0, 0}}};

static const inflater_tables deflate64_tables = {
    /* NOTE: The primary difference between Deflate and Deflate64 w.r.t. the length encoding is that the final entry
     * (the entry for symbol 285) has a base/extra bits of 258/0 for Deflate, whereas it's 3/16 for Deflate64 */
    .lengths = {{3, 0},  {4, 0},  {5, 0},  {6, 0},   {7, 0},   {8, 0},   {9, 0},   {10, 0},  {11, 1}, {13, 1},
                {15, 1}, {17, 1}, {19, 2}, {23, 2},  {27, 2},  {31, 2},  {35, 3},  {43, 3},  {51, 3}, {59, 3},
                {67, 4}, {83, 4}, {99, 4}, {115, 4}, {131, 5}, {163, 5}, {195, 5}, {227, 5}, {3, 16}},
    /* NOTE: The only difference between Deflate and Deflate64 w.r.t. the distance encoding is that Deflate64 makes use
     * of symbols 30 and 31 */
    .distances = {{1, 0},     {2, 0},     {3, 0},     {4, 0},      {5, 1},      {7, 1},      {9, 2},      {13, 2},
                  {17, 3},    {25, 3},    {33, 4},    {49, 4},     {65, 5},     {97, 5},     {129, 6},    {193, 6},
                  {257, 7},   {385, 7},   {513, 8},   {769, 8},    {1025, 9},   {1537, 9},   {2049, 10},  {3073, 10},
                  {4097, 11}, {6145, 11}, {8193, 12}, {12289, 12}, {16385, 13}, {24577, 13}, {32769, 14}, {49153, 14}}};

/* The "active" table is indexed using the current "mode" */
static const inflater_tables* const inflate_tables[] = {&deflate_tables, &deflate64_tables};

/* The maximum number of bytes that a single compressed block operation can consume. These values are used to optimize
 * the likely path where we have enough data for a single operation so we don't have to continuously check to see if we
 * have enough data. These values are calculated as follows:
 * Deflate:
 *      15 bit length:      0 bits in stream  -> read 2 bytes = 16 bits in stream -> 1 bit leftover
 *      5 extra bits:       1 bit in stream   -> read 2 bytes = 17 bits in stream -> 12 bits leftover
 *      15 bit distance:    12 bits in stream -> read 2 bytes = 28 bits in stream -> 13 bits leftover
 *      13 extra bits:      13 bits in stream -> read 2 bytes = 29 bits in stream -> 16 bits leftover
 *          Total Bytes Needed:                       8 bytes
 * Deflate64:
 *      15 bit length:      0 bits in stream  -> read 2 bytes = 16 bits in stream -> 1 bit leftover
 *      16 extra bits:      1 bit in stream   -> read 2 bytes = 17 bits in stream -> 1 bit leftover
 *      15 bit distance:    1 bit in stream   -> read 2 bytes = 17 bits in stream -> 2 bits leftover
 *      14 extra bits:      2 bits in stream  -> read 2 bytes = 18 bits in stream -> 4 bits leftover
 *          Total Bytes Needed:                       8 bytes */
static const size_t max_compressed_op_size = 8;

/* static int inflater_read_compressed_fast(inflatelib_stream* stream); */
static int inflater_read_compressed_fast(inflatelib_stream* stream);

static int inflater_read_compressed(inflatelib_stream* stream)
{
    int result = INFLATELIB_OK;
    inflatelib_state* state = stream->internal;
    uint8_t* out = (uint8_t*)stream->next_out;
    size_t bytesCopied, outSize = stream->avail_out;
    uint16_t symbol;
    int opResult, keepGoing = 1;
    const inflater_tables* tables = inflate_tables[state->mode];

    /* On entry, try and write any data we previously wrote to the window, but did not consume */
    bytesCopied = window_copy_output(&state->window, out, outSize);
    out += bytesCopied;
    outSize -= bytesCopied;

    while (keepGoing)
    {
        switch (state->ifstate)
        {
        case ifstate_reading_literal_length_code:
            /* The fast path requires that we start in 'ifstate_reading_literal_length_code' */
            if ((state->bitstream.length >= max_compressed_op_size) && outSize)
            {
                stream->next_out = out;
                stream->avail_out = outSize;
                result = inflater_read_compressed_fast(stream);
                out = stream->next_out;
                outSize = stream->avail_out;

                if (result < INFLATELIB_OK)
                {
                    keepGoing = 0; /* Error message, etc. already set */
                    break;
                }

                /* NOTE: It's possible for 'inflater_read_compressed_fast' to exit in a state other than
                 * 'ifstate_reading_literal_length_code', so need to re-evaluate */
                break;
            }

            /* We're in the process of reading a value from the literal/length tree */
            opResult = huffman_tree_lookup(&state->literal_length_tree, stream, &state->data.compressed.symbol);
            if (opResult == 0)
            {
                state->need_more_data = 1;
                keepGoing = 0; /* Not enough data in the input */
                break;
            }
            else if (opResult < 0)
            {
                /* Error in the data; NOTE: We've already set the error message */
                keepGoing = 0;
                result = INFLATELIB_ERROR_DATA;
                break;
            }
            /* Fallthrough */

        case ifstate_decoding_literal_length_code:
            if (state->data.compressed.symbol < 256) /* Literal */
            {
                if (!window_write_byte(&state->window, (uint8_t)state->data.compressed.symbol))
                {
                    /* Not enough data in the window; try and read some data to free up space */
                    bytesCopied = window_copy_output(&state->window, out, outSize);
                    if (!bytesCopied)
                    {
                        keepGoing = 0; /* Not enough data in the output */
                        state->ifstate = ifstate_decoding_literal_length_code;
                        break;
                    }

                    out += bytesCopied;
                    outSize -= bytesCopied;

                    /* Otherwise, we copyied at least one byte and therefore this write should succeed */
                    opResult = window_write_byte(&state->window, (uint8_t)state->data.compressed.symbol);
                    assert(opResult);
                }

                state->ifstate = ifstate_reading_literal_length_code;
                break;
            }
            else if (state->data.compressed.symbol == 256) /* End of block */
            {
                state->ifstate = ifstate_copying_output_from_window;
                break;
            }
            else if (state->data.compressed.symbol > 285)
            {
                /* NOTE: HLIT is 5 bits, which means that there are at most 288 code lengths specified for the
                 * literal/length tree (257 + 31). This means that in theory, someone could author a block where symbols
                 * can go from 0 to 287. If we move this error "up" and error out if HLIT is greater than 29, we can
                 * eliminate this error check, which could potentially give us some perf wins at the cost of potentially
                 * rejecting otherwise valid inputs. */
                if (format_error_message(stream, "Invalid symbol '%u' from literal/length tree", state->data.compressed.symbol) < 0)
                {
                    stream->error_msg = "Invalid symbol from literal/length tree";
                }
                keepGoing = 0;
                errno = EINVAL;
                result = INFLATELIB_ERROR_DATA;
                break;
            }

            /* Otherwise, 'symbol' references a length */
            symbol = state->data.compressed.symbol - 257;
            assert(symbol < inflatelib_arraysize(tables->lengths)); /* Shouldn't have passed check above */
            state->data.compressed.block_length = tables->lengths[symbol].base;
            state->data.compressed.extra_bits = tables->lengths[symbol].extra_bits;
            /* Fallthrough */

        case ifstate_reading_length_extra_bits:
            if (state->data.compressed.extra_bits > 0)
            {
                if (!bitstream_read_bits(&state->bitstream, state->data.compressed.extra_bits, &symbol))
                {
                    state->need_more_data = 1;
                    keepGoing = 0; /* Not enough data in the input */
                    state->ifstate = ifstate_reading_length_extra_bits;
                    break;
                }

                state->data.compressed.block_length += symbol;
            }
            /* Fallthrough */

        case ifstate_reading_distance_code:
            /* Now we need to read a distance */
            opResult = huffman_tree_lookup(&state->distance_tree, stream, &symbol);
            if (opResult == 0)
            {
                state->need_more_data = 1;
                keepGoing = 0; /* Not enough data in the input */
                state->ifstate = ifstate_reading_distance_code;
                break;
            }
            else if (opResult < 0)
            {
                /* Error in the data; NOTE: We've already set the error message */
                keepGoing = 0;
                result = INFLATELIB_ERROR_DATA;
                break;
            }

            /* NOTE: HDIST is 5 bits, giving a maximum of 32 distance symbols, the size of the 'distances' table */
            assert(symbol < inflatelib_arraysize(tables->distances));
            state->data.compressed.block_distance = tables->distances[symbol].base;
            state->data.compressed.extra_bits = tables->distances[symbol].extra_bits;

            if (!state->data.compressed.block_distance)
            {
                keepGoing = 0;
                if (format_error_message(stream, "Distance code %u is not valid in Deflate", symbol) < 0)
                {
                    stream->error_msg = "Distance code is not valid in Deflate";
                }
                errno = EINVAL;
                result = INFLATELIB_ERROR_DATA;
                break;
            }
            /* Fallthrough */

        case ifstate_reading_distance_extra_bits:
            if (state->data.compressed.extra_bits > 0)
            {
                if (!bitstream_read_bits(&state->bitstream, state->data.compressed.extra_bits, &symbol))
                {
                    state->need_more_data = 1;
                    keepGoing = 0; /* Not enough data in the input */
                    state->ifstate = ifstate_reading_distance_extra_bits;
                    break;
                }

                state->data.compressed.block_distance += symbol;
            }
            /* Fallthrough */

            /* NOTE: It's not guaranteed we have enough space available in 'out' to write all data, hence the need for a
             * dedicated state for copying the data from the window */
        case ifstate_copying_length_distance_from_window:
            opResult =
                window_copy_length_distance(&state->window, state->data.compressed.block_distance, state->data.compressed.block_length);
            if (opResult < 0)
            {
                keepGoing = 0;
                if (format_error_message(
                        stream,
                        "Compressed block has a distance '%u' which exceeds the size of the window (%llu bytes)",
                        state->data.compressed.block_distance,
                        state->window.total_bytes) < 0)
                {
                    stream->error_msg = "Compressed block has a distance which exceeds the size of the window";
                }
                errno = EINVAL;
                result = INFLATELIB_ERROR_DATA;
                break;
            }

            state->data.compressed.block_length -= (uint32_t)opResult;

            bytesCopied = window_copy_output(&state->window, out, outSize);
            out += bytesCopied;
            outSize -= bytesCopied;

            /* There are two scenarios where the operation is not yet complete at this point: (1) 'block_length' was too
             * long to copy all data in a single operation, or (2) we ran out of space in the output buffer */
            if ((state->data.compressed.block_length == 0) && (state->window.unconsumed_bytes == 0))
            {
                /* Repeat the process until we hit the end of block symbol (256) */
                state->ifstate = ifstate_reading_literal_length_code;
            }
            else
            {
                state->ifstate = ifstate_copying_length_distance_from_window;
                assert((state->data.compressed.block_length != 0) || (outSize == 0));

                if (((state->data.compressed.block_length == 0) || (opResult == 0)) && (outSize == 0))
                {
                    /* Can't copy any more data in the window and can't copy any more data to the output... need to
                     * return to the caller so they can give us a larger output buffer to write to */
                    keepGoing = 0;
                }
            }
            break;

        case ifstate_copying_output_from_window:
            /* This state means we've read all input; we just need to finish copying data to the output */
            bytesCopied = window_copy_output(&state->window, out, outSize);
            out += bytesCopied;
            outSize -= bytesCopied;
            if (state->window.unconsumed_bytes == 0)
            {
                /* All data consumed; go back to reading bfinal */
                state->ifstate = state->bfinal ? ifstate_eof : ifstate_reading_bfinal;
            }

            /* Even if we're not done reading bytes, we've run out of space in the output and need to return */
            keepGoing = 0;
            break;

        default:
            assert(0); /* Should not be evaluating this function then */
            INFLATELIB_UNREACHABLE();
        }
    }

    /* Copy as much data from the window as we can before returning */
    bytesCopied = window_copy_output(&state->window, out, outSize);
    out += bytesCopied;
    outSize -= bytesCopied;

    /* Update the output buffers to reflect what we wrote */
    stream->next_out = out;
    stream->avail_out = outSize;

    return result;
}

static int inflater_read_compressed_fast(inflatelib_stream* stream)
{
    int result = INFLATELIB_OK;
    inflatelib_state* state = stream->internal;
    uint8_t* out = (uint8_t*)stream->next_out;
    size_t bytesCopied, outSize = stream->avail_out;
    size_t extraBits;
    uint16_t symbol;
    uint32_t blockLength, blockDistance;
    int opResult;
    const inflater_tables* tables = inflate_tables[state->mode];

    assert(state->ifstate == ifstate_reading_literal_length_code);
    while ((state->bitstream.length >= max_compressed_op_size) && outSize)
    {
        opResult = huffman_tree_lookup_unchecked(&state->literal_length_tree, stream, &symbol);
        if (opResult < 0)
        {
            /* Error in the data; NOTE: We've already set the error message */
            result = INFLATELIB_ERROR_DATA;
            break;
        }
        assert(opResult != 0); /* Impossible to return 0 */

        if (symbol < 256) /* Literal */
        {
            window_write_byte_consume(&state->window, (uint8_t)symbol);
            *out++ = (uint8_t)symbol;
            --outSize;

            /* Go back to reading a new symbol */
            continue;
        }
        else if (symbol == 256) /* End of block */
        {
            /* Let the slow path take care of copying data */
            /* NOTE: We should only be in this state if we've already read all data from the window, however this will
             * correctly take care of transitioning to EOF state etc. */
            state->ifstate = ifstate_copying_output_from_window;
            break;
        }
        else if (symbol > 285)
        {
            /* NOTE: HLIT is 5 bits, which means that there are at most 288 code lengths specified for the
             * literal/length tree (257 + 31). This means that in theory, someone could author a block where symbols can
             * go from 0 to 287. If we move this error "up" and error out if HLIT is greater than 29, we can eliminate
             * this error check, which could potentially give us some perf wins at the cost of potentially rejecting
             * otherwise valid inputs. */
            /* NOTE: From experimentation, the benefit is very minor - slightly over a 1% speed up */
            if (format_error_message(stream, "Invalid symbol '%u' from literal/length tree", symbol) < 0)
            {
                stream->error_msg = "Invalid symbol from literal/length tree";
            }
            errno = EINVAL;
            result = INFLATELIB_ERROR_DATA;
            break;
        }

        /* Otherwise, 'symbol' references a length */
        symbol -= 257;
        assert(symbol < inflatelib_arraysize(tables->lengths)); /* Shouldn't have passed check above */
        blockLength = tables->lengths[symbol].base;
        extraBits = tables->lengths[symbol].extra_bits;

        if (extraBits > 0)
        {
            blockLength += bitstream_read_bits_unchecked(&state->bitstream, extraBits);
        }

        /* Now we need to read a distance */
        opResult = huffman_tree_lookup_unchecked(&state->distance_tree, stream, &symbol);
        if (opResult < 0)
        {
            /* Error in the data; NOTE: We've already set the error message */
            result = INFLATELIB_ERROR_DATA;
            break;
        }
        assert(opResult != 0); /* Impossible to return 0 */

        /* NOTE: HDIST is 5 bits, giving a maximum of 32 distance symbols, the size of the 'distances' table */
        assert(symbol < inflatelib_arraysize(tables->distances));
        blockDistance = tables->distances[symbol].base;
        extraBits = tables->distances[symbol].extra_bits;

        if (!blockDistance)
        {
            if (format_error_message(stream, "Distance code %u is not valid in Deflate", symbol) < 0)
            {
                stream->error_msg = "Distance code is not valid in Deflate";
            }
            errno = EINVAL;
            result = INFLATELIB_ERROR_DATA;
            break;
        }

        if (extraBits > 0)
        {
            blockDistance += bitstream_read_bits_unchecked(&state->bitstream, extraBits);
        }

        /* NOTE: In Deflate64, the longest possible length is greater than the window size by two bytes, meaning we may
         * not be able to copy a full length/distance with a single copy call. This is assumed to be unlikely and we
         * optimize for the case where a single copy can copy all bytes */
        opResult = window_copy_length_distance(&state->window, blockDistance, blockLength);

        if (opResult < 0)
        {
            if (format_error_message(
                    stream,
                    "Compressed block has a distance '%u' which exceeds the size of the window (%llu bytes)",
                    blockDistance,
                    state->window.total_bytes) < 0)
            {
                stream->error_msg = "Compressed block has a distance which exceeds the size of the window";
            }
            errno = EINVAL;
            result = INFLATELIB_ERROR_DATA;
            break;
        }

        bytesCopied = window_copy_output(&state->window, out, outSize);
        out += bytesCopied;
        outSize -= bytesCopied;

        /* There are two scenarios where the operation is not yet complete at this point: (1) 'block_length' was too
         * long to copy all data in a single operation, or (2) we ran out of space in the output buffer */
        if (((uint32_t)opResult < blockLength) || (state->window.unconsumed_bytes != 0))
        {
            state->data.compressed.block_length = blockLength - (uint32_t)opResult;
            state->data.compressed.block_distance = blockDistance;
            state->ifstate = ifstate_copying_length_distance_from_window;
            assert((state->data.compressed.block_length != 0) || (outSize == 0));

            /* Let the slow path take care of this */
            break;
        }
    }

    /* Update the output buffers to reflect what we wrote */
    stream->next_out = out;
    stream->avail_out = outSize;

    return result;
}
