/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <vector>

#include <mango/core/compress.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>

#include "../../external/miniz/miniz.h"

#ifdef MANGO_ENABLE_LICENSE_BSD
#include "../../external/lz4/lz4.h"
#include "../../external/lz4/lz4hc.h"
#include "../../external/lzo/minilzo.h"
#include "../../external/zstd/zstd.h"
#endif

#ifdef MANGO_ENABLE_LICENSE_ZLIB
#include "../../external/bzip2/bzlib.h"
#include "../../external/lzfse/lzfse.h"
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

	size_t compress(Memory dest, Memory source, int level)
	{
        level = clamp(level, 0, 10);

        mz_ulong size = static_cast<mz_ulong>(dest.size);
        int status = mz_compress2(dest, &size, source, static_cast<mz_ulong>(source.size), level);

        if (status != MZ_OK)
        {
            MANGO_EXCEPTION("miniz: compression failed.");
        }

        return size;
	}

    void decompress(Memory dest, Memory source)
    {
        mz_ulong zd = static_cast<mz_ulong>(dest.size);
        int status = mz_uncompress(dest, &zd, source, static_cast<mz_ulong>(source.size));

        const char* msg;

        switch (status)
        {
            case MZ_OK:
                msg = nullptr;
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

    size_t compress(Memory dest, Memory source, int level)
    {
        const int source_size = int(source.size);
        const int dest_size = int(dest.size);

        size_t written = 0;

        level = clamp(level, 0, 10);

        if (level > 6)
        {
            const int compression_level = 1 + (level - 7) * 5;
            written = LZ4_compress_HC(source, dest, source_size, dest_size, compression_level);
        }
        else
        {
            const int acceleration = 19 - level * 3;
            written = LZ4_compress_fast(source, dest, source_size, dest_size, acceleration);
        }

	    if (written <= 0 || written > dest.size)
        {
			MANGO_EXCEPTION("lz4: compression failed.");
		}

        return written;
	}

    void decompress(Memory dest, Memory source)
    {
        int status = LZ4_decompress_fast(source, dest, int(dest.size));
        if (status < 0)
        {
            MANGO_EXCEPTION("lz4: decompression failed.");
        }
    }

    // stream

    class StreamEncoderLZ4 : public StreamEncoder
    {
    protected:
        LZ4_stream_t* m_stream;
        int m_acceleration;

        char m_buffer[1024 * 128];
        size_t m_offset { 0 };

    public:
        StreamEncoderLZ4(int level)
            : m_acceleration(level)
        {
            m_stream = LZ4_createStream();
        }

        ~StreamEncoderLZ4()
        {
            LZ4_freeStream(m_stream);
        }

        size_t bound(size_t size) const
        {
            const int s = int(size);
            return LZ4_compressBound(s);
        }

        size_t encode(Memory dest, Memory source)
        {
            size_t written = 0;

            for (; source.size > 0;)
            {
                size_t block_size = std::min(size_t(1024 * 64), source.size);

                if (m_offset + block_size > 1024 * 128)
                {
                    m_offset = 0;
                }

                char* temp = m_buffer + m_offset;
                m_offset += block_size;

                std::memcpy(temp, source.address, block_size);
                source.address += block_size;
                source.size -= block_size;

                char* dst = reinterpret_cast<char *>(dest.address);
                int bytes = LZ4_compress_fast_continue(m_stream, temp, dst, int(block_size), int(dest.size), m_acceleration);

                dest.address += bytes;
                dest.size -= bytes;

                written += bytes;
            }

            return written;
        }
    };

    class StreamDecoderLZ4 : public StreamDecoder
    {
    protected:
        LZ4_streamDecode_t* m_stream;

        char m_buffer[1024 * 128];
        size_t m_offset { 0 };

    public:
        StreamDecoderLZ4()
        {
            m_stream = LZ4_createStreamDecode();
        }

        ~StreamDecoderLZ4()
        {
            LZ4_freeStreamDecode(m_stream);
        }

        size_t decode(Memory dest, Memory source)
        {
            size_t written = 0;

            for (; dest.size > 0;)
            {
                size_t block_size = std::min(size_t(1024 * 64), dest.size);

                if (m_offset + block_size > 1024 * 128)
                {
                    m_offset = 0;
                }

                char* temp = m_buffer + m_offset;
                m_offset += block_size;

                const char* src = reinterpret_cast<const char *>(source.address);
                int bytes = LZ4_decompress_fast_continue(m_stream, src, temp, int(block_size));

                source.address += bytes;
                source.size -= bytes;

                std::memcpy(dest.address, temp, block_size);
                dest.address += block_size;
                dest.size -= block_size;

                written += block_size;
            }

            return written;
        }
    };

    StreamEncoder* createStreamEncoder(int level)
    {
        StreamEncoder* encoder = new StreamEncoderLZ4(level);
        return encoder;
    }

    StreamDecoder* createStreamDecoder()
    {
        StreamDecoder* decoder = new StreamDecoderLZ4();
        return decoder;
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

    size_t compress(Memory dest, Memory source, int level)
    {
        void* workmem = aligned_malloc(LZO1X_MEM_COMPRESS);

        lzo_uint dst_len = (lzo_uint)dest.size;
        int x = lzo1x_1_compress(
            source.address,
            static_cast<lzo_uint>(source.size),
            dest.address,
            &dst_len,
            workmem);

        aligned_free(workmem);
        if (x != LZO_E_OK)
        {
            MANGO_EXCEPTION("lzo: compression failed.");
        }

        return static_cast<size_t>(dst_len);
	}

    void decompress(Memory dest, Memory source)
    {
        lzo_uint dst_len = (lzo_uint)dest.size;
        int x = lzo1x_decompress(
            source.address,
            static_cast<lzo_uint>(source.size),
            dest.address,
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

    size_t compress(Memory dest, Memory source, int level)
    {
        level = clamp(level * 2, 1, 20);

        const size_t x = ZSTD_compress(dest.address, dest.size,
                                       source.address, source.size, level);
        if (ZSTD_isError(x))
        {
            const char* error = ZSTD_getErrorName(x);
            std::string s = "ZSTD: ";
            s += error;
            MANGO_EXCEPTION(s);
        }

        return x;
	}

    void decompress(Memory dest, Memory source)
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

    // stream

    class StreamEncoderZSTD : public StreamEncoder
    {
    protected:
        ZSTD_CStream* z;

    public:
        StreamEncoderZSTD(int level)
        {
            level = clamp(level * 2, 1, 20);
            z = ZSTD_createCStream();
            ZSTD_initCStream(z, level);
        }

        ~StreamEncoderZSTD()
        {
            ZSTD_freeCStream(z);
        }

        size_t bound(size_t size) const
        {
            // compression is faster when it has more temporary buffer
            const size_t turbo = 1024 * 128;
            return ZSTD_compressBound(size) + turbo;
        }

        size_t encode(Memory dest, Memory source)
        {
            ZSTD_inBuffer input;

            input.src = source.address;
            input.size = source.size;
            input.pos = 0;

            ZSTD_outBuffer output;

            output.dst = dest.address;
            output.size = dest.size;
            output.pos = 0;

            for (; input.pos < input.size;)
            {
                ZSTD_compressStream(z, &output, &input);
            }

            ZSTD_flushStream(z, &output);

            return output.pos;
        }
    };

    class StreamDecoderZSTD : public StreamDecoder
    {
    protected:
        ZSTD_DStream* z;

    public:
        StreamDecoderZSTD()
        {
            z = ZSTD_createDStream();
            ZSTD_initDStream(z);
        }

        ~StreamDecoderZSTD()
        {
            ZSTD_freeDStream(z);
        }

        size_t decode(Memory dest, Memory source)
        {
            ZSTD_inBuffer input;

            input.src = source.address;
            input.size = source.size;
            input.pos = 0;

            ZSTD_outBuffer output;

            output.dst = dest.address;
            output.size = dest.size;
            output.pos = 0;

            for (; input.pos < input.size;)
            {
                ZSTD_decompressStream(z, &output, &input);
            }

            return output.pos;
        }
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

    size_t compress(Memory dest, Memory source, int level)
    {
        const int blockSize100k = clamp(level, 1, 9);

        const int verbosity = 0;
        const int workFactor = 30;

        bz_stream strm;

        strm.bzalloc = nullptr;
        strm.bzfree = nullptr;
        strm.opaque = nullptr;

        int x = BZ2_bzCompressInit(&strm, blockSize100k, verbosity, workFactor);
        if (x != BZ_OK)
        {
            MANGO_EXCEPTION("bzip2: compression failed.");
        }

        unsigned int destLength = static_cast<unsigned int>(dest.size);

        strm.next_in = source;
        strm.next_out = dest;
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

        return static_cast<size_t>(destLength);
    }

    void decompress(Memory dest, Memory source)
    {
        bz_stream strm;

        strm.bzalloc = nullptr;
        strm.bzfree = nullptr;
        strm.opaque = nullptr;

        int x = BZ2_bzDecompressInit(&strm, 0, 0);
        if (x != BZ_OK)
        {
            MANGO_EXCEPTION("bzip2: decompression failed.");
        }

        strm.next_in = source;
        strm.next_out = dest;
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

    size_t compress(Memory dest, Memory source, int level)
    {
        MANGO_UNREFERENCED_PARAMETER(level);

        const size_t scratch_size = lzfse_encode_scratch_size();
        Buffer scratch(scratch_size);
        size_t written = lzfse_encode_buffer(dest.address, dest.size, source, source.size, scratch);
        return written;
    }

    void decompress(Memory dest, Memory source)
    {
        const size_t scratch_size = lzfse_decode_scratch_size();
        Buffer scratch(scratch_size);
        size_t written = lzfse_decode_buffer(dest.address, dest.size, source, source.size, scratch);
        MANGO_UNREFERENCED_PARAMETER(written);
    }

} // namespace lzfse

#endif // MANGO_ENABLE_LICENSE_ZLIB

} // namespace mango
