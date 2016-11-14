/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    class StreamEncoder : public Object
    {
    public:
        StreamEncoder() {}
        virtual ~StreamEncoder() {}
        virtual size_t bound(size_t size) const = 0;
        virtual size_t encode(const Memory& dest, const Memory& source) = 0;
    };

    class StreamDecoder : public Object
    {
    public:
        StreamDecoder() {}
        virtual ~StreamDecoder() {}
        virtual size_t decode(const Memory& dest, const Memory& source) = 0;
    };

    // -----------------------------------------------------------------------
    // memory block compression
    // -----------------------------------------------------------------------

    // Compression levels are clamped to range [0, 10]
    // Level 6: default
    // Level 10: maximum compression
    // Other levels are implementation defined

    namespace miniz
    {
        size_t bound(size_t size);
        Memory compress(const Memory& dest, const Memory& source, int level = 6);
        void decompress(const Memory& dest, const Memory& source);
    }

#ifdef MANGO_ENABLE_LICENSE_BSD
    namespace lz4
    {
        size_t bound(size_t size);
        Memory compress(const Memory& dest, const Memory& source, int level = 6);
        void decompress(const Memory& dest, const Memory& source);
    }

    namespace lzo
    {
        size_t bound(size_t size);
        Memory compress(const Memory& dest, const Memory& source, int level = 6);
        void decompress(const Memory& dest, const Memory& source);
    }

    namespace zstd
    {
        size_t bound(size_t size);
        Memory compress(const Memory& dest, const Memory& source, int level = 6);
        void decompress(const Memory& dest, const Memory& source);

        StreamEncoder* createStreamEncoder(int level);
        StreamDecoder* createStreamDecoder();
    }
#endif

#ifdef MANGO_ENABLE_LICENSE_ZLIB
    namespace bzip2
    {
        size_t bound(size_t size);
        Memory compress(const Memory& dest, const Memory& source, int level = 6);
        void decompress(const Memory& dest, const Memory& source);
    }

    namespace lzfse
    {
        size_t bound(size_t size);
        Memory compress(const Memory& dest, const Memory& source, int level = 6);
        void decompress(const Memory& dest, const Memory& source);
    }
#endif

    // -----------------------------------------------------------------------
    // hashing algorithms
    // -----------------------------------------------------------------------

    uint32 crc32(uint32 crc, const uint8* data, size_t size);
    uint32 crc32c(uint32 crc, const uint8* data, size_t size);
    void sha1(uint32 hash[5], const uint8* data, size_t size);
    void md5(uint32 hash[4], const uint8* data, size_t size);

} // namespace mango
