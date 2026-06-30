/*
 *    Copyright (c) Microsoft. All rights reserved.
 *    This code is licensed under the MIT License.
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 *    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *    PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
#ifndef INFLATELIB_H
#define INFLATELIB_H

#include <stdint.h>

#ifdef __has_attribute
#if __has_attribute(visibility)
#define INFLATELIB_HAS_VISIBILITY_ATTR 1
#endif
#endif

/*
 * INFLATELIB_BUILD_SHARED      The library is being built as a shared library
 * INFLATELIB_CONSUME_SHARED    The library is being consumed as a shared library
 * Otherwise...                 The library is either being built as a static library or it is being consumed in an
 *                              unspecified manner; either as a shared or static library.
 */
#if defined(_WIN32) && defined(_MSC_VER)
#ifdef INFLATELIB_BUILD_SHARED
#define INFLATELIB_EXPORT __declspec(dllexport)
#elif defined(INFLATELIB_CONSUME_SHARED)
#define INFLATELIB_EXPORT __declspec(dllimport)
#else
#define INFLATELIB_EXPORT /* Unknown or static - don't decorate function declarations */
#endif
#elif INFLATELIB_HAS_VISIBILITY_ATTR
#define INFLATELIB_EXPORT __attribute__((visibility("default")))
#else
#define INFLATELIB_EXPORT /* Not Windows and no visibility attribute... don't decorate function declarations */
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#define INFLATELIB_CALLCONV __cdecl
#else
#define INFLATELIB_CALLCONV
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define INFLATELIB_VERSION_STRING "0.2.0"
#define INFLATELIB_VERSION_MAJOR 0
#define INFLATELIB_VERSION_MINOR 2
#define INFLATELIB_VERSION_PATCH 0

    typedef void* (*inflatelib_alloc)(void* userData, size_t bytes, size_t alignment);
    typedef void (*inflatelib_free)(void* userData, void* allocatedPtr, size_t bytes, size_t alignment);

    struct inflatelib_state; /* Opaque to client applications */

    /*
     * This struct stores all data and state for decompressing Deflate/Deflate64 encoded data. With the exception of
     * success/error codes, all input and output data is passed through this struct. Specifically, input and output
     * buffers are set by the caller via 'next_in'/'avail_in' and 'next_out'/'avail_out' respectively. The "inflate" and
     * "inflate64" functions communicate the amount of data consumed/written by updating these buffers and lengths. For
     * example, you can determine the amount of data consumed/written by a call to 'inflatelib_inflate' or
     * 'inflatelib_inflate64' by either subtracting 'next_in'/'next_out' after the call by their original pointers
     * (interpreted as uintptr_t values) or by subtracting 'avail_in'/'avail_out' after the call from the sizes passed
     * in. Alternatively, you can subtract 'total_in'/'total_out' after the call by their values before the call (or
     * equivalently by setting them to zero before the call and reading their values after the call).
     *
     * Internally, these objects behave as a state machine. After being initialized, they transition to either "Deflate"
     * or "Deflate64" decoding states via calls to 'inflatelib_inflate' and 'inflatelib_inflate64' respectively. When
     * in these states, the only valid operations are to destroy the object, put the object back into the initialized
     * state via 'inflatelib_reset', or to decompress more data using the function that corresponds to the current
     * state: either 'inflatelib_inflate' for the "Deflate" state or 'inflatelib_inflate64' for the "Deflate64" state.
     * If you attempt to call 'inflatelib_inflate64' when in the "Deflate" state or vice-versa, the call will fail.
     */
    typedef struct inflatelib_stream
    {
        /*
         * Pointer to the next byte of input. This pointer is set by the caller and updated by the library based on the
         * number of bytes consumed.
         */
        const void* next_in;
        /*
         * The number of valid bytes available for read in 'next_in'. This value is set by the caller and updated by the
         * library based on the number of bytes consumed.
         */
        size_t avail_in;
        /*
         * Total number of bytes read by the library so far. This value is never consumed by the library; it is only
         * ever incremented.
         */
        uintmax_t total_in;

        /*
         * Pointer to the next byte of output data written by the library. This pointer is set by the caller and updated
         * by the library based on the number of bytes written so that this points one byte past the last byte of output
         * written.
         */
        void* next_out;
        /*
         * The number of bytes, starting at 'next_out', that can be written to. This value is set by the caller and
         * updated by the library based on the number of bytes written.
         */
        size_t avail_out;
        /*
         * Total number of bytes written by the library so far. This value is never consumed by the library; it is only
         * ever incremented.
         */
        uintmax_t total_out;

        /*
         * Optional user data passed to any callback functions (such as alloc/free below)
         */
        void* user_data;

        /*
         * Custom memory allocation functions. Set to null to get malloc/free respectively
         */
        inflatelib_alloc alloc;
        inflatelib_free free;

        /*
         * A string describing the last error encountered. This pointer is only valid if a library function returned
         * failure
         */
        const char* error_msg;

        /*
         * Internal state used by the library
         */
        struct inflatelib_state* internal;
    } inflatelib_stream;

/*
 * Return values. Non-negative values indicate success while negative values indicate some sort of error. When a
 * negative value is returned, the 'error_msg' member of the 'inflatelib_stream' will be set. Otherwise, the 'error_msg'
 * will remain unchanged. A positive return value indicates an "interesting" change in state that is not considered a
 * failure, while a return value of zero indicates generic success.
 */
#define INFLATELIB_OK 0          /* No error occurred */
#define INFLATELIB_EOF 1         /* No error occurred; reached the end of the stream */
#define INFLATELIB_ERROR_ARG -1  /* Invalid argument */
#define INFLATELIB_ERROR_DATA -2 /* Error in the input data */
#define INFLATELIB_ERROR_OOM -3  /* Failed to allocate data */

    /*
     * Initializes the stream. The 'user_data', 'alloc', and 'free' members MUST be set prior to the init call and MUST
     * NOT be changed after the init call completes. This function returns one of the status values specified above.
     */
    INFLATELIB_EXPORT int INFLATELIB_CALLCONV inflatelib_init(inflatelib_stream* stream);

    /*
     * Resets the stream's state back to its initialized state. This allows the stream to be reused for multiple inflate
     * calls without having to destroy and reinitialize it. Typically, this is used to reset the stream after an inflate
     * call returns EOF, however it can also be used to reset the stream after an error or even during the middle of
     * inflating a stream. This function also allows the caller to switch between Deflate and Deflate64.
     */
    INFLATELIB_EXPORT int INFLATELIB_CALLCONV inflatelib_reset(inflatelib_stream* stream);

    /*
     * Cleans up any data allocated/initialized by the library. This function must be called if 'inflatelib_init'
     * returns success. After this function is called, the 'inflatelib_stream' cannot be used for any function call
     * unless 'inflatelib_init' is called again. This function can only return success.
     */
    INFLATELIB_EXPORT int INFLATELIB_CALLCONV inflatelib_destroy(inflatelib_stream* stream);

    /*
     * "Inflates" the Deflate encoded data from next_in/avail_in, writing the decoded data to next_out/avail_out. The
     * 'stream' MUST be in either the "initialized" state or the "Deflate" state. This function returns one of the
     * status values defined above, notably it returns 'INFLATELIB_EOF' when the last block of data has been fully
     * processed.
     */
    INFLATELIB_EXPORT int INFLATELIB_CALLCONV inflatelib_inflate(inflatelib_stream* stream);

    /*
     * "Inflates" the Deflate64 encoded data from next_in/avail_in, writing the decoded data to next_out/avail_out. The
     * 'stream' MUST be in either the "initialized" state or the "Deflate64" state. This function returns one of the
     * status values defined above, notably it returns 'INFLATELIB_EOF' when the last block of data has been fully
     * processed.
     */
    INFLATELIB_EXPORT int INFLATELIB_CALLCONV inflatelib_inflate64(inflatelib_stream* stream);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // INFLATELIB_H
