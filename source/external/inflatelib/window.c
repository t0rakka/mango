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

#include "window.h"

void window_init(window* window)
{
    window_reset(window);
}

void window_reset(window* window)
{
    window->read_offset = 0;
    window->write_offset = 0;
    window->unconsumed_bytes = 0;
    window->total_bytes = 0;
}

size_t window_copy_output(window* window, uint8_t* output, size_t outputSize)
{
    size_t totalBytesToCopy = (outputSize <= window->unconsumed_bytes) ? outputSize : window->unconsumed_bytes;

    for (size_t bytesRemaining = totalBytesToCopy; bytesRemaining > 0;)
    {
        uint32_t buffRemaining = DEFLATE64_WINDOW_SIZE - window->read_offset; /* Space until end of buffer */
        size_t bytesToCopy = (bytesRemaining <= buffRemaining) ? bytesRemaining : buffRemaining;

        memcpy(output, window->data + window->read_offset, bytesToCopy);
        output += bytesToCopy;
        bytesRemaining -= bytesToCopy;
        window->read_offset += (uint16_t)bytesToCopy; /* This will overflow back to zero correctly */
        /* Wait until after the loop to update 'unconsumed_bytes' since it's not used by the loop*/
    }

    window->unconsumed_bytes -= totalBytesToCopy;

    return totalBytesToCopy;
}

size_t window_copy_bytes(window* window, bitstream* bitstream, size_t count)
{
    size_t result = 0;

    assert((window->unconsumed_bytes + count) <= DEFLATE64_WINDOW_SIZE); /* Otherwise we will clobber data */

    while (count > 0)
    {
        size_t buffRemaining = DEFLATE64_WINDOW_SIZE - window->write_offset; /* Space until end of buffer */
        size_t bytesToCopy = (count <= buffRemaining) ? count : buffRemaining;

        size_t bytesCopied = bitstream_copy_bytes(bitstream, bytesToCopy, window->data + window->write_offset);
        count -= bytesCopied;
        result += bytesCopied;
        window->write_offset = (uint16_t)(window->write_offset + bytesCopied); /* This will overflow back to zero correctly */
        /* Wait until after the loop to update other counts as these values are not used by the loop */

        if (bytesCopied < bytesToCopy)
        {
            /* Less data available than requested; exit early */
            break;
        }
    }

    window->total_bytes += result;
    window->unconsumed_bytes += result;

    return result;
}

int window_copy_length_distance(window* window, size_t distance, size_t length)
{
    size_t result = 0;
    uint16_t copyIndex;
    size_t writeSpaceRemaining = DEFLATE64_WINDOW_SIZE - window->unconsumed_bytes;
    assert(window->unconsumed_bytes <= DEFLATE64_WINDOW_SIZE);
    assert(distance <= DEFLATE64_WINDOW_SIZE);

    /* The distance can't reference data that hasn't been written yet */
    if ((distance > window->total_bytes))
    {
        return -1; /* Invalid distance */
    }

    /* Figure out the index where we should start copying data from. This is rather easy since we can rely on unsigned
     * integer overflow to give us the correct value */
    copyIndex = (uint16_t)(window->write_offset - distance);

    /*
     * We can't just copy all bytes in one fell swoop for several reasons:
     *      1.  The distance between 'copyIndex' and the end of the buffer may be less than 'length', in which case we
     *          need to wrap back around and copy from the start of the buffer.
     *      2.  'length' may be greater than 'distance'. I.e. some of the data we are copying from has not been written
     *          yet.
     *      3.  With Deflate64, the maximum length is greater than 65,536 - the size of the buffer - and we can't write
     *          all of that data without writing over data that hasn't been consumed yet.
     */
    while ((length > 0) && (writeSpaceRemaining > 0))
    {
        size_t readRemaining, writeRemaining, copySize;

        if (copyIndex < window->write_offset)
        {
            /* The amount of valid data for us to copy in a single operation is up to the write offset */
            readRemaining = window->write_offset - copyIndex;
        }
        else
        {
            /* All data up to the end of the buffer can be copied in a single operation */
            readRemaining = DEFLATE64_WINDOW_SIZE - copyIndex;
        }

        writeRemaining = DEFLATE64_WINDOW_SIZE - window->write_offset;
        if (writeRemaining > writeSpaceRemaining)
        {
            /* Prevent ourselves from overwriting data */
            writeRemaining = writeSpaceRemaining;
        }

        /* The size we can copy is the minimum of: (1) 'readRemaining' (max amount of data we can read in one
         * operation), (2) 'writeRemaining' (max amount of data we can write in one operation), and (3) 'length' */
        copySize = length;
        if (copySize > readRemaining)
        {
            copySize = readRemaining;
        }
        if (copySize > writeRemaining)
        {
            copySize = writeRemaining;
        }

        /* We need to use a memmove because the data we are copying from may overlap with what we are copying to */
        memmove(window->data + window->write_offset, window->data + copyIndex, copySize);

        /* Integer overflow will take care of resetting each of these back to zero properly */
        window->write_offset = (uint16_t)(window->write_offset + copySize);
        window->unconsumed_bytes += copySize;
        window->total_bytes += copySize;
        copyIndex = (uint16_t)(copyIndex + copySize);
        writeSpaceRemaining -= copySize;
        length -= copySize;
        result += copySize;
    }

    return (int)result;
}

int window_write_byte(window* window, uint8_t byte)
{
    if (window->unconsumed_bytes >= DEFLATE64_WINDOW_SIZE)
    {
        return 0; /* Buffer is full */
    }

    window->data[window->write_offset] = byte;
    ++window->write_offset; /* This will overflow back to zero correctly */
    ++window->unconsumed_bytes;
    ++window->total_bytes;

    return 1;
}

void window_write_byte_consume(window* window, uint8_t byte)
{
    assert(window->unconsumed_bytes == 0);               /* Pre-condition */
    assert(window->write_offset == window->read_offset); /* Sanity check */

    window->data[window->write_offset] = byte;
    ++window->write_offset; /* These will overflow back to zero correctly */
    ++window->read_offset;
    ++window->total_bytes;
}
