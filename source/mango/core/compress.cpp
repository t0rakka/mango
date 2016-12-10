/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/compress.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>

#define MINIZ_HEADER_FILE_ONLY
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "../../miniz/miniz.cpp"

#ifdef MANGO_ENABLE_LICENSE_BSD
#include "../../lz4/lz4.h"
#include "../../lz4/lz4hc.h"
#include "../../lzo/minilzo.h"
#include "../../zstd/common/zstd.h"
#include "../../zstd/common/zbuff.h"
#endif

#ifdef MANGO_ENABLE_LICENSE_ZLIB
#include "../../bzip2/bzlib.h"
#include "../../lzfse/lzfse.h"
#endif

namespace mango {

// ----------------------------------------------------------------------------
// miniz
// ----------------------------------------------------------------------------

namespace miniz {

    size_t bound(size_t size)
    {
        const mz_ulong s = static_cast<mz_ulong>(size);
		return mz_compressBound(s);
    }

	Memory compress(const Memory& dest, const Memory& source, int level)
	{
        level = clamp(level, 0, 10);

        mz_ulong size = static_cast<mz_ulong>(dest.size);
        int status = mz_compress2((unsigned char*)dest.address, &size, source.address,
                                  static_cast<mz_ulong>(source.size), level);

        if (status != MZ_OK)
        {
            MANGO_EXCEPTION("miniz: compression failed.");
        }

        return Memory(dest.address, size);
	}

    void decompress(const Memory& dest, const Memory& source)
    {
        mz_ulong zd = static_cast<mz_ulong>(dest.size);
        uint8* buffer = const_cast<uint8 *>(dest.address);

        int status = mz_uncompress(buffer, &zd, source.address,
                                   static_cast<mz_ulong>(source.size));

        const char* msg;

        switch (status)
        {
            case MZ_OK:
                msg = NULL;
                break;
            case MZ_MEM_ERROR:
                msg = "miniz: not enough memory.";
                break;
            case MZ_BUF_ERROR:
                msg = "miniz: not enough room in the output buffer.";
                break;
            case MZ_DATA_ERROR:
                msg = "miniz: corrupted input data.";
                break;
            default:
                msg = "miniz: undefined error.";
                break;
        }

        if (msg)
        {
            MANGO_EXCEPTION(msg);
        }
    }

} // namespace miniz

#ifdef MANGO_ENABLE_LICENSE_BSD

// ----------------------------------------------------------------------------
// lz4
// ----------------------------------------------------------------------------

namespace lz4 {

    size_t bound(size_t size)
    {
        const int s = int(size);
		return LZ4_compressBound(s);
    }

    Memory compress(const Memory& dest, const Memory& source, int level)
    {
        const int source_size = int(source.size);
        const int dest_size = int(dest.size);

        size_t written = 0;

        level = clamp(level, 0, 10);

        if (level > 6)
        {
            const int compressionLevel = 1 + (level - 7) * 5;
            written = LZ4_compress_HC(source, (char*)dest.address,
                source_size, dest_size, compressionLevel);
        }
        else
        {
            const int acceleration = 19 - level * 3;
            written = LZ4_compress_fast(source, (char*)dest.address,
                source_size, dest_size, acceleration);
        }

	    if (written <= 0 || written > dest.size)
        {
			MANGO_EXCEPTION("lz4: compression failed.");
		}

        return Memory(dest.address, written);
	}

    void decompress(const Memory& dest, const Memory& source)
    {
        uint8* buffer = const_cast<uint8 *>(dest.address);
        int status = LZ4_decompress_fast(source, reinterpret_cast<char *>(buffer), int(dest.size));
        if (status < 0)
        {
            MANGO_EXCEPTION("lz4: decompression failed.");
        }
    }

} // namespace lz4

// ----------------------------------------------------------------------------
// lzo
// ----------------------------------------------------------------------------

namespace lzo {

    size_t bound(size_t size)
    {
        return size + (size / 16) + 128;
    }

    Memory compress(const Memory& dest, const Memory& source, int level)
    {
        void* workmem = aligned_malloc(LZO1X_MEM_COMPRESS);

        lzo_uint dst_len = (lzo_uint)dest.size;
        int x = lzo1x_1_compress(
            source.address,
            static_cast<lzo_uint>(source.size),
            const_cast<uint8*>(dest.address),
            &dst_len,
            workmem);

        aligned_free(workmem);
        if (x != LZO_E_OK)
        {
            MANGO_EXCEPTION("lzo: compression failed.");
        }

        return Memory(dest.address, (size_t)dst_len);
	}

    void decompress(const Memory& dest, const Memory& source)
    {
        lzo_uint dst_len = (lzo_uint)dest.size;
        int x = lzo1x_decompress(
            source.address,
            static_cast<lzo_uint>(source.size),
            const_cast<uint8*>(dest.address),
            &dst_len,
            NULL);
        if (x != LZO_E_OK)
        {
            MANGO_EXCEPTION("lzo: decompression failed.");
        }
    }

} // namespace lzo

// ----------------------------------------------------------------------------
// zstd
// ----------------------------------------------------------------------------

namespace zstd {

    size_t bound(size_t size)
    {
        // compression is faster when it has more temporary buffer
        const size_t turbo = 1024 * 128;
		return ZSTD_compressBound(size) + turbo;
    }

    Memory compress(const Memory& dest, const Memory& source, int level)
    {
        level = clamp(level * 2, 1, 20);

        const size_t x = ZSTD_compress((void*)dest.address, dest.size,
                                       source.address, source.size, level);
        if (ZSTD_isError(x))
        {
            const char* error = ZSTD_getErrorName(x);
            std::string s = "ZSTD: ";
            s += error;
            MANGO_EXCEPTION(s);
        }

        return Memory(dest.address, x);
	}

    void decompress(const Memory& dest, const Memory& source)
    {
        size_t x = ZSTD_decompress((void*)dest.address, dest.size,
                                   source.address, source.size);
        if (ZSTD_isError(x))
        {
            const char* error = ZSTD_getErrorName(x);
            std::string s = "ZSTD: ";
            s += error;
            MANGO_EXCEPTION(s);
        }
    }

    class StreamEncoderZSTD : public StreamEncoder
    {
    public:
        StreamEncoderZSTD(int level)
        {
            level = clamp(level * 2, 1, 20);
            z = ZBUFF_createCCtx();
            ZBUFF_compressInit(z, level);
        }

        ~StreamEncoderZSTD()
        {
            //ZBUFF_compressEnd(z, ..., ...);
            ZBUFF_freeCCtx(z);
        }

        size_t bound(size_t size) const
        {
            // compression is faster when it has more temporary buffer
            const size_t turbo = 1024 * 128;
            return ZSTD_compressBound(size) + turbo;
        }

        size_t encode(const Memory& dest, const Memory& source)
        {
            const uint8* src = source.address;
            size_t src_bytes = source.size;

            uint8* dst = const_cast<uint8 *>(dest.address);
            size_t dst_bytes = dest.size;

            size_t dstCapacity;
            size_t srcSize;

            // compression loop
            for (; src_bytes > 0;)
            {
                dstCapacity = dst_bytes;
                srcSize = src_bytes;
                ZBUFF_compressContinue(z, dst, &dstCapacity, src, &srcSize);
                dst += dstCapacity;
                src += srcSize;
                dst_bytes -= dstCapacity;
                src_bytes -= srcSize;
            }

            // flush compressed stream
            dstCapacity = dst_bytes;
            ZBUFF_compressFlush(z, dst, &dstCapacity);
            dst += dstCapacity;
            dst_bytes -= dstCapacity;

            // calculate how many bytes we wrote
            size_t written = dest.size - dst_bytes;

            return written;
        }

    protected:
        ZBUFF_CCtx* z;
    };

    class StreamDecoderZSTD : public StreamDecoder
    {
    public:
        StreamDecoderZSTD()
        {
            z = ZBUFF_createDCtx();
            ZBUFF_decompressInit(z);
        }

        ~StreamDecoderZSTD()
        {
            ZBUFF_freeDCtx(z);
        }

        size_t decode(const Memory& dest, const Memory& source)
        {
            const uint8* src = source.address;
            size_t src_bytes = source.size;

            uint8* dst = const_cast<uint8 *>(dest.address);
            size_t dst_bytes = dest.size;

            for (; src_bytes > 0;)
            {
                size_t dstCapacity = dst_bytes;
                size_t srcSize = src_bytes;
                ZBUFF_decompressContinue(z, dst, &dstCapacity, src, &srcSize);
                dst += dstCapacity;
                src += srcSize;
                dst_bytes -= dstCapacity;
                src_bytes -= srcSize;
            }

            return dest.size;
        }

    protected:
        ZBUFF_DCtx* z;
    };

    StreamEncoder* createStreamEncoder(int level)
    {
        StreamEncoder* encoder = new StreamEncoderZSTD(level);
        return encoder;
    }

    StreamDecoder* createStreamDecoder()
    {
        StreamDecoder* decoder = new StreamDecoderZSTD();
        return decoder;
    }
    
} // namespace zstd

#endif // MANGO_ENABLE_LICENSE_BSD

#ifdef MANGO_ENABLE_LICENSE_ZLIB

// ----------------------------------------------------------------------------
// bzip2
// ----------------------------------------------------------------------------

namespace bzip2 {

    size_t bound(size_t size)
    {
        return size + (size / 100) + 600;
    }

    Memory compress(const Memory& dest, const Memory& source, int level)
    {
        const int blockSize100k = clamp(level, 1, 9);

        const int verbosity = 0;
        const int workFactor = 30;

        bz_stream strm;

        strm.bzalloc = NULL;
        strm.bzfree = NULL;
        strm.opaque = NULL;

        int x = BZ2_bzCompressInit(&strm, blockSize100k, verbosity, workFactor);
        if (x != BZ_OK)
        {
            MANGO_EXCEPTION("bzip2: compression failed.");
        }

        unsigned int destLength = static_cast<unsigned int>(dest.size);

        strm.next_in = (char*)source.address;
        strm.next_out = (char*)dest.address;
        strm.avail_in = static_cast<unsigned int>(source.size);
        strm.avail_out = destLength;

        x = BZ2_bzCompress(&strm, BZ_FINISH);
        if (x == BZ_FINISH_OK)
        {
            BZ2_bzCompressEnd(&strm);
            MANGO_EXCEPTION("bzip2: compression failed.");
        }
        else if (x != BZ_STREAM_END)
        {
            BZ2_bzCompressEnd(&strm);
            MANGO_EXCEPTION("bzip2: compression failed.");
        }

        destLength -= strm.avail_out;
        BZ2_bzCompressEnd(&strm);

        return Memory(dest.address, static_cast<size_t>(destLength));
    }

    void decompress(const Memory& dest, const Memory& source)
    {
        bz_stream strm;

        strm.bzalloc = NULL;
        strm.bzfree = NULL;
        strm.opaque = NULL;

        int x = BZ2_bzDecompressInit(&strm, 0, 0);
        if (x != BZ_OK)
        {
            MANGO_EXCEPTION("bzip2: decompression failed.");
        }

        strm.next_in = (char*)source.address;
        strm.next_out = (char*)dest.address;
        strm.avail_in = static_cast<unsigned int>(source.size);
        strm.avail_out = static_cast<unsigned int>(dest.size);

        x = BZ2_bzDecompress(&strm);
        if (x == BZ_OK)
        {
            BZ2_bzDecompressEnd(&strm);
            MANGO_EXCEPTION("bzip2: decompression failed.");
        }
        else if (x != BZ_STREAM_END)
        {
            BZ2_bzDecompressEnd(&strm);
            MANGO_EXCEPTION("bzip2: decompression failed.");
        }

        BZ2_bzDecompressEnd(&strm);
    }

} // namespace bzip2

// ----------------------------------------------------------------------------
// lzfse
// ----------------------------------------------------------------------------

namespace lzfse {

    size_t bound(size_t size)
    {
        // conservative estimate
        return 1024 + size;
    }

    Memory compress(const Memory& dest, const Memory& source, int level)
    {
        MANGO_UNREFERENCED_PARAMETER(level);

        const size_t scratch_size = lzfse_encode_scratch_size();
        char* scratch = new char[scratch_size];
        size_t written = lzfse_encode_buffer(const_cast<uint8_t *>(dest.address), dest.size,
                                             reinterpret_cast<const uint8_t *>(source.address), source.size, scratch);
        delete[] scratch;
        return Memory(dest.address, written);
    }

    void decompress(const Memory& dest, const Memory& source)
    {
        const size_t scratch_size = lzfse_decode_scratch_size();
        char* scratch = new char[scratch_size];
        size_t written = lzfse_decode_buffer(const_cast<uint8_t *>(dest.address), dest.size,
                                             reinterpret_cast<const uint8_t *>(source.address), source.size, scratch);
        delete[] scratch;
        MANGO_UNREFERENCED_PARAMETER(written);
    }

} // namespace lzfse

#endif // MANGO_ENABLE_LICENSE_ZLIB

} // namespace mango
