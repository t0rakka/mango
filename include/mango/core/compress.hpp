/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "configure.hpp"
#include "memory.hpp"
#include "object.hpp"

namespace mango
{

    // -----------------------------------------------------------------------
    // stream compression
    // -----------------------------------------------------------------------

    // WARNING! The memory allocation is caller's responsibility; use bound()
    // to get conservative estimate for destination size - the bound is
    // guaranteed to be sufficient.

    // The stream compression interface is abstraction for a stateful block compressor.
    // Each call to encode() will compress a block which can be decoded. The unique
    // feature is that the compressor and decompressor are stateful; the compressor
    // keeps track of what it had compressed before so that data can be compressed
    // efficiently in smaller chunks, which can be decoded individually. The caller
    // is responsible for transmitting the compressed block size - that is NOT part of
    // the protocol. This information is required so that the decompressor can allocate
    // enough memory for the decompressed data. The compressed blocks MUST be decompressed
    // at the same order they were compressed - this is a state machine with two end points:
    // the sender and receiver.

    // This API is useful for transmitting compressed realtime data stream over high latency,
    // low bandwidth connection.

    class StreamEncoder : public RefCounted
    {
    public:
        StreamEncoder() {}
        virtual ~StreamEncoder() {}
        virtual size_t bound(size_t size) const = 0;
        virtual size_t encode(Memory dest, Memory source) = 0;
    };

    class StreamDecoder : public RefCounted
    {
    public:
        StreamDecoder() {}
        virtual ~StreamDecoder() {}
        virtual size_t decode(Memory dest, Memory source) = 0;
    };

#ifdef MANGO_ENABLE_LICENSE_BSD

    namespace lz4
    {
        StreamEncoder* createStreamEncoder(int level);
        StreamDecoder* createStreamDecoder();
    }

    namespace zstd
    {
        StreamEncoder* createStreamEncoder(int level);
        StreamDecoder* createStreamDecoder();
    }

#endif

    // -----------------------------------------------------------------------
    // memory block compression
    // -----------------------------------------------------------------------

    // WARNING! The memory allocation is caller's responsibility; use bound()
    // to get conservative estimate for destination size - the bound is
    // guaranteed to be sufficient.

    // Compression levels are clamped to range [0, 10]
    // Level 6: default
    // Level 10: maximum compression
    // Other levels are implementation defined

    namespace miniz
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

#ifdef MANGO_ENABLE_LICENSE_BSD

    namespace lz4
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

    namespace lzo
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

    namespace zstd
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

#endif

#ifdef MANGO_ENABLE_LICENSE_ZLIB

    namespace bzip2
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

    namespace lzfse
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

#endif

    namespace lzma
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

#if 0
    namespace lzma2
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

    namespace ppmd
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }

    namespace deflate64
    {
        size_t bound(size_t size);
        size_t compress(Memory dest, Memory source, int level = 6);
        void decompress(Memory dest, Memory source);
    }
#endif

} // namespace mango
