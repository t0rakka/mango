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

#include "bitstream.h"

void bitstream_init(bitstream* stream)
{
    /* Currently, there's no difference between "init" and "reset" */
    bitstream_reset(stream);
}

void bitstream_reset(bitstream* stream)
{
    stream->data = 0;
    stream->length = 0;
    stream->buffer = 0;
    stream->bits_in_buffer = 0;
}

void bitstream_set_data(bitstream* stream, const uint8_t* data, size_t length)
{
    /* Should have consumed all data before calling */
    assert(stream->length == 0);

    stream->data = data;
    stream->length = length;
    /* Don't touch buffer; it may have valid data */
}

void bitstream_clear_data(bitstream* stream, int reclaimData, const uint8_t** finalData, size_t* finalLength)
{
    size_t reclaimSize = 0;

    if (reclaimData)
    {
        reclaimSize = stream->bits_in_buffer / 8;
        stream->bits_in_buffer = stream->bits_in_buffer % 8;
        stream->buffer &= ((1u << stream->bits_in_buffer) - 1);
    }

    *finalData = stream->data - reclaimSize;
    *finalLength = stream->length + reclaimSize;

    stream->data = NULL;
    stream->length = 0;
}

void bitstream_byte_align(bitstream* stream)
{
    /* It's possible to have more than one byte in the buffer, so we can't just set 'bits_in_buffer' to zero */
    size_t bitsToConsume = stream->bits_in_buffer % 8;

    stream->buffer >>= bitsToConsume;
    stream->bits_in_buffer -= bitsToConsume;
}

size_t bitstream_copy_bytes(bitstream* stream, size_t bytesToRead, uint8_t* dest)
{
    size_t bytesFromBuffer, bytesFromData;

    /* The caller should ensure that the stream is byte-aligned before calling this function. It may be the case that
     * some data is already in the buffer - e.g. if the previous operation was a peek - in which case there should be
     * a multiple of 8-bits in the buffer */
    assert((stream->bits_in_buffer % 8) == 0);
    assert(bytesToRead > 0);

    bytesFromBuffer = stream->bits_in_buffer / 8;
    bytesFromBuffer = (bytesFromBuffer > bytesToRead) ? bytesToRead : bytesFromBuffer;

    for (size_t i = 0; i < bytesFromBuffer; ++i)
    {
        *dest++ = (uint8_t)stream->buffer;
        stream->buffer >>= 8;
        stream->bits_in_buffer -= 8;
    }
    bytesToRead -= bytesFromBuffer;

    bytesFromData = (stream->length > bytesToRead) ? bytesToRead : stream->length;

    memcpy(dest, stream->data, bytesFromData);

    stream->data += bytesFromData;
    stream->length -= bytesFromData;

    return bytesFromBuffer + bytesFromData;
}

/* Tries to fill the buffer such that there's at least two bytes of data */
static inline void bitstream_fill_buffer(bitstream* stream)
{
    if ((stream->bits_in_buffer < 16) && (stream->length != 0))
    {
        /* We have enough data to copy at least one more byte */
        stream->buffer |= ((uint32_t)*stream->data) << stream->bits_in_buffer;
        ++stream->data;
        --stream->length;
        stream->bits_in_buffer += 8;

        if ((stream->bits_in_buffer < 16) && (stream->length != 0))
        {
            /* We can copy another one */
            stream->buffer |= ((uint32_t)*stream->data) << stream->bits_in_buffer;
            ++stream->data;
            --stream->length;
            stream->bits_in_buffer += 8;

            assert(stream->bits_in_buffer >= 16); /* Should have two bytes by now */
        }
    }
}

static inline void bitstream_fill_buffer_unchecked(bitstream* stream)
{
    if (stream->bits_in_buffer < 16)
    {
        uint32_t newData;

        assert(stream->length >= 2); /* Caller should have verified */
        newData = ((uint32_t)stream->data[0]) | (((uint32_t)stream->data[1]) << 8);
        stream->buffer |= newData << stream->bits_in_buffer;
        stream->bits_in_buffer += 16;
        stream->data += 2;
        stream->length -= 2;
    }
}

size_t bitstream_read_bits(bitstream* stream, size_t bitsToRead, uint16_t* result)
{
    uint32_t mask;

    assert((bitsToRead > 0) && (bitsToRead <= (sizeof(*result) * 8)));

    bitstream_fill_buffer(stream);
    if (stream->bits_in_buffer < bitsToRead)
    {
        return 0; /* Not enough data */
    }

    mask = ((uint32_t)1 << bitsToRead) - 1;
    *result = (uint16_t)(stream->buffer & mask);
    stream->buffer >>= bitsToRead;
    stream->bits_in_buffer -= bitsToRead;

    return 1;
}

uint16_t bitstream_read_bits_unchecked(bitstream* stream, size_t bitsToRead)
{
    uint16_t result;
    uint32_t mask = ((uint32_t)1 << bitsToRead) - 1;

    assert((bitsToRead > 0) && (bitsToRead <= (sizeof(result) * 8)));

    bitstream_fill_buffer_unchecked(stream);
    assert(bitsToRead <= stream->bits_in_buffer);

    result = (uint16_t)(stream->buffer & mask);
    stream->buffer >>= bitsToRead;
    stream->bits_in_buffer -= bitsToRead;

    return result;
}

size_t bitstream_peek(bitstream* stream, uint16_t* result)
{
    bitstream_fill_buffer(stream);

    *result = (uint16_t)stream->buffer;
    return (stream->bits_in_buffer <= 16) ? stream->bits_in_buffer : 16;
}

uint16_t bitstream_peek_unchecked(bitstream* stream)
{
    bitstream_fill_buffer_unchecked(stream);
    return (uint16_t)stream->buffer;
}
