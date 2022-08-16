/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <vector>

#include <mango/core/compress.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/buffer.hpp>
#include <mango/core/bits.hpp>
#include <mango/core/endian.hpp>
#include <mango/core/pointer.hpp>
#include <mango/math/math.hpp>

#include "../../external/lz4/lz4.h"
#include "../../external/lz4/lz4hc.h"
#include "../../external/lzo/minilzo.h"

#define ZSTD_DISABLE_DEPRECATE_WARNINGS
#include "../../external/zstd/zstd.h"

#include "../../external/bzip2/bzlib.h"
#include "../../external/lzfse/lzfse.h"

#include "../../external/lzma/Alloc.h"
#include "../../external/lzma/LzmaDec.h"
#include "../../external/lzma/LzmaEnc.h"
#include "../../external/lzma/Lzma2Dec.h"
#include "../../external/lzma/Lzma2Enc.h"
#include "../../external/lzma/Ppmd8.h"

#include "../../external/zlib/zlib.h"

#include "../../external/libdeflate/libdeflate.h"

namespace mango
{

// ----------------------------------------------------------------------------
// nocompress
// ----------------------------------------------------------------------------

namespace nocompress
{

    size_t bound(size_t size)
    {
        return size;
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        MANGO_UNREFERENCED(level);

        std::memcpy(dest.address, source.address, source.size);

        CompressionStatus status;
        status.size = source.size;
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        std::memcpy(dest.address, source.address, source.size);

        CompressionStatus status;
        status.size = source.size;
        return status;
    }

} // namespace nocompress

// ----------------------------------------------------------------------------
// lz4
// ----------------------------------------------------------------------------

namespace lz4
{

    size_t bound(size_t size)
    {
        const int s = int(size);
		return LZ4_compressBound(s);
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        const int source_size = int(source.size);
        const int dest_size = int(dest.size);

        CompressionStatus status;

        level = math::clamp(level, 0, 10);

        if (level > 6)
        {
            const int compression_level = 1 + (level - 7) * 5;
            status.size = LZ4_compress_HC(source.cast<const char>(), dest.cast<char>(), source_size, dest_size, compression_level);
        }
        else
        {
            const int acceleration = 19 - level * 3;
            status.size = LZ4_compress_fast(source.cast<const char>(), dest.cast<char>(), source_size, dest_size, acceleration);
        }

	    if (status.size > dest.size)
        {
            status.setError("[lz4] compression failed.");
		}

        return status;
	}

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        CompressionStatus status;

        int result = LZ4_decompress_safe(source.cast<const char>(), dest.cast<char>(), int(source.size), int(dest.size));
        if (result < 0)
        {
            status.setError("[lz4] decompression failed.");
        }

        status.size = dest.size;
        return status;
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

        size_t encode(Memory dest, ConstMemory source)
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

                char* dst = dest.cast<char>();
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

        size_t decode(Memory dest, ConstMemory source)
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
                int bytes = LZ4_decompress_safe_continue(m_stream, src, temp, int(source.size), int(block_size));

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

    std::shared_ptr<StreamEncoder> createStreamEncoder(int level)
    {
        return std::make_shared<StreamEncoderLZ4>(level);
    }

    std::shared_ptr<StreamDecoder> createStreamDecoder()
    {
        return std::make_shared<StreamDecoderLZ4>();
    }

} // namespace lz4

// ----------------------------------------------------------------------------
// lzo
// ----------------------------------------------------------------------------

namespace lzo
{

    size_t bound(size_t size)
    {
        return size + (size / 16) + 128;
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        MANGO_UNREFERENCED(level);

        void* workmem = aligned_malloc(LZO1X_MEM_COMPRESS);

        lzo_uint dst_len = (lzo_uint)dest.size;
        int x = lzo1x_1_compress(source.address, lzo_uint(source.size),
            dest.address, &dst_len, workmem);

        aligned_free(workmem);

        CompressionStatus status;

        if (x != LZO_E_OK)
        {
            status.setError("[lzo] compression failed.");
        }

        status.size = size_t(dst_len);
        return status;
	}

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        lzo_uint dst_len = (lzo_uint)dest.size;
        int x = lzo1x_decompress(source.address, lzo_uint(source.size),
            dest.address, &dst_len, nullptr);

        CompressionStatus status;

        if (x != LZO_E_OK)
        {
            status.setError("[lzo] decompression failed.");
        }

        status.size = dest.size;
        return status;
    }

} // namespace lzo

// ----------------------------------------------------------------------------
// zstd
// ----------------------------------------------------------------------------

namespace zstd
{

    size_t bound(size_t size)
    {
        // compression is faster when it has more temporary buffer
        const size_t turbo = 1024 * 128;
		return ZSTD_compressBound(size) + turbo;
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        CompressionStatus status;

        // zstd compress does not support encoding of empty source
        if (source.size)
        {
            level = math::clamp(level * 2, 1, 20);

            const size_t x = ZSTD_compress(dest.address, dest.size,
                                        source.address, source.size, level);

            if (ZSTD_isError(x))
            {
                status.setError("[zstd] %s", ZSTD_getErrorName(x));
            }

            status.size = x;
        }

        return status;
	}

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        size_t x = ZSTD_decompress((void*)dest.address, dest.size,
                                   source.address, source.size);

        CompressionStatus status;

        if (ZSTD_isError(x))
        {
            status.setError("[zstd] %s", ZSTD_getErrorName(x));
        }

        status.size = dest.size;
        return status;
    }

    // stream

    class StreamEncoderZSTD : public StreamEncoder
    {
    protected:
        ZSTD_CStream* z;

    public:
        StreamEncoderZSTD(int level)
        {
            level = math::clamp(level * 2, 1, 20);
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

        size_t encode(Memory dest, ConstMemory source)
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

        size_t decode(Memory dest, ConstMemory source)
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

    std::shared_ptr<StreamEncoder> createStreamEncoder(int level)
    {
        return std::make_shared<StreamEncoderZSTD>(level);
    }

    std::shared_ptr<StreamDecoder> createStreamDecoder()
    {
        return std::make_shared<StreamDecoderZSTD>();
    }

} // namespace zstd

// ----------------------------------------------------------------------------
// bzip2
// ----------------------------------------------------------------------------

namespace bzip2
{

    size_t bound(size_t size)
    {
        return size + (size / 100) + 600;
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        const int blockSize100k = math::clamp(level, 1, 9);

        const int verbosity = 0;
        const int workFactor = 30;

        bz_stream strm;

        strm.bzalloc = nullptr;
        strm.bzfree = nullptr;
        strm.opaque = nullptr;

        CompressionStatus status;

        int x = BZ2_bzCompressInit(&strm, blockSize100k, verbosity, workFactor);
        if (x != BZ_OK)
        {
            status.setError("[bzip2] compression failed.");
            return status;
        }

        unsigned int destLength = static_cast<unsigned int>(dest.size);

        strm.next_in = const_cast<char*>(source.cast<const char>());
        strm.next_out = dest.cast<char>();
        strm.avail_in = static_cast<unsigned int>(source.size);
        strm.avail_out = destLength;

        x = BZ2_bzCompress(&strm, BZ_FINISH);
        if (x == BZ_FINISH_OK)
        {
            BZ2_bzCompressEnd(&strm);
            status.setError("[bzip2] compression failed.");
        }
        else if (x != BZ_STREAM_END)
        {
            BZ2_bzCompressEnd(&strm);
            status.setError("[bzip2] compression failed.");
        }

        destLength -= strm.avail_out;
        BZ2_bzCompressEnd(&strm);

        status.size = size_t(destLength);
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        bz_stream strm;

        strm.bzalloc = nullptr;
        strm.bzfree = nullptr;
        strm.opaque = nullptr;

        CompressionStatus status;

        int x = BZ2_bzDecompressInit(&strm, 0, 0);
        if (x != BZ_OK)
        {
            status.setError("[bzip2] decompression failed.");
            return status;
        }

        strm.next_in = const_cast<char*>(source.cast<const char>());
        strm.next_out = dest.cast<char>();
        strm.avail_in = static_cast<unsigned int>(source.size);
        strm.avail_out = static_cast<unsigned int>(dest.size);

        x = BZ2_bzDecompress(&strm);
        if (x == BZ_OK)
        {
            BZ2_bzDecompressEnd(&strm);
            status.setError("[bzip2] decompression failed.");
        }
        else if (x != BZ_STREAM_END)
        {
            BZ2_bzDecompressEnd(&strm);
            status.setError("[bzip2] decompression failed.");
        }

        BZ2_bzDecompressEnd(&strm);

        status.size = size_t(strm.next_out - dest.cast<char>());
        return status;
    }

} // namespace bzip2

// ----------------------------------------------------------------------------
// lzfse
// ----------------------------------------------------------------------------

namespace lzfse
{

    size_t bound(size_t size)
    {
        // conservative estimate
        return 1024 + size;
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        MANGO_UNREFERENCED(level);

        const size_t scratch_size = lzfse_encode_scratch_size();
        Buffer scratch(scratch_size);
        size_t written = lzfse_encode_buffer(dest.address, dest.size, source, source.size, scratch);

        CompressionStatus status;
        status.size = written;
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        const size_t scratch_size = lzfse_decode_scratch_size();
        Buffer scratch(scratch_size);
        size_t written = lzfse_decode_buffer(dest.address, dest.size, source, source.size, scratch);

        CompressionStatus status;
        status.size = written;
        return status;
    }

} // namespace lzfse

// ----------------------------------------------------------------------------
// lzma
// ----------------------------------------------------------------------------

namespace lzma
{

    static const char* get_error_string(SRes result)
    {
        const char* error = nullptr;
        if (result != SZ_OK)
        {
            switch (result)
            {
                case SZ_ERROR_DATA: error = "data error"; break;
                case SZ_ERROR_MEM: error = "memory allocation failed"; break;
                case SZ_ERROR_UNSUPPORTED: error = "unsupported properties"; break;
                case SZ_ERROR_INPUT_EOF: error = "insufficient input"; break;
                default: error = "undefined error"; break;
            }
        }
        return error;
    }

    size_t bound(size_t size)
    {
        // NOTE: conservative estimate since the lzma-sdk
        //       cannot provide more accurate value.
        return (size * 3) / 2 + 1024 * 16;
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        CLzmaEncProps props;
        LzmaEncProps_Init(&props);

        level = math::clamp(level - 1, 0, 9);

        props.level = level; // [0, 9] (default: 5)
        props.dictSize = 2048 << level; // use (1 << N) or (3 << N). 4 KB < dictSize <= 128 MB
        props.lc = 3; // [0, 8] (default: 3)
        props.lp = 0; // [0, 4] (default: 0)
        props.pb = 2; // [0, 4] (default: 2)
        props.fb = 32; // [5, 273] (default: 32)
        props.numThreads = 1;

        u8* start = dest.address;

        // write the 5 byte props header before compressed data
        u8* props_output = dest.address;
        SizeT props_output_size = LZMA_PROPS_SIZE;
        dest.address += LZMA_PROPS_SIZE;
        dest.size -= LZMA_PROPS_SIZE;

        SizeT dest_length = dest.size;
        SizeT source_length = source.size;

        SRes result = LzmaEncode(
            dest.address, &dest_length, source.address, source_length,
            &props, props_output, &props_output_size, 0,
            nullptr, &g_Alloc, &g_Alloc);

        CompressionStatus status;

        const char* error = get_error_string(result);
        if (error)
        {
            status.setError("[lzma] %s", error);
        }

        status.size = dest.address + dest_length - start;
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        // read props header
        const u8* prop = source.address;
        source.address += LZMA_PROPS_SIZE;
        source.size -= LZMA_PROPS_SIZE;

        SizeT destLen = dest.size;
        SizeT srcLen = source.size;

        ELzmaStatus st;
        SRes result = LzmaDecode(dest.address, &destLen, source.address, &srcLen,
            prop, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &st, &g_Alloc);

        CompressionStatus status;

        const char* error = get_error_string(result);
        if (error)
        {
            status.setError("[lzma] %s", error);
        }

        status.size = dest.size;
        return status;
    }

} // namespace lzma

// ----------------------------------------------------------------------------
// lzma2
// ----------------------------------------------------------------------------

namespace lzma2
{

    size_t bound(size_t size)
    {
        return lzma::bound(size);
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        CLzma2EncProps props;
        Lzma2EncProps_Init(&props);
        Lzma2EncProps_Normalize(&props);

        level = math::clamp(level, 0, 10);

        CLzma2EncHandle encoder = Lzma2Enc_Create(&g_Alloc, &g_Alloc);

        Lzma2Enc_SetProps(encoder, &props);
        Byte p = Lzma2Enc_WriteProperties(encoder);

        u8* start = dest.address;

        // write props header
        dest.address[0] = p;
        dest.address++;
        dest.size--;

        Byte *outBuf = dest.address;
        size_t outBufSize = dest.size;

        const Byte *inData = source.address;
        size_t inDataSize = source.size;

        SRes result = Lzma2Enc_Encode2(encoder,
            nullptr, outBuf, &outBufSize,
            nullptr, inData, inDataSize, nullptr);

        Lzma2Enc_Destroy(encoder);

        CompressionStatus status;

        const char* error = lzma::get_error_string(result);
        if (error)
        {
            status.setError("[lzma2] %s", error);
        }

        status.size = dest.address + outBufSize - start;
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        // read props header
        Byte prop = source.address[0];
        source.address++;
        source.size--;

        SizeT destLen = dest.size;
        SizeT srcLen = source.size;

        ELzmaStatus st;
        SRes result = Lzma2Decode(dest.address, &destLen, source.address, &srcLen,
            prop, LZMA_FINISH_ANY, &st, &g_Alloc);

        CompressionStatus status;

        const char* error = lzma::get_error_string(result);
        if (error)
        {
            status.setError("[lzma2] %s", error);
        }

        status.size = dest.size;
        return status;
    }

} // namespace lzma2

// ----------------------------------------------------------------------------
// ppmd8
// ----------------------------------------------------------------------------

namespace ppmd8
{

    struct OutputStreamPPMD8 : IByteOut
    {
        Memory memory;
        size_t offset;

        OutputStreamPPMD8(Memory memory)
            : memory(memory)
            , offset(0)
        {
            Write = write_byte;
        }

        static void write_byte(const IByteOut *p, Byte b)
        {
            OutputStreamPPMD8* stream = (OutputStreamPPMD8 *) p;
            if (stream->offset < stream->memory.size)
            {
                stream->memory.address[stream->offset++] = b;
            }
        }
    };

    struct InputStreamPPMD8 : IByteIn
    {
        ConstMemory memory;
        size_t offset;

        InputStreamPPMD8(ConstMemory memory)
            : memory(memory)
            , offset(0)
        {
            Read = read_byte;
        }

        static u8 read_byte(const IByteIn *p)
        {
            InputStreamPPMD8* stream = (InputStreamPPMD8 *) p;
            u8 value = 0;
            if (stream->offset < stream->memory.size)
            {
                value = stream->memory.address[stream->offset++];
            }
            return value;
        }
    };

    size_t bound(size_t size)
    {
        return lzma::bound(size);
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        u8* start = dest.address;

        level = math::clamp(level, 0, 10);

        // encoding parameters
        u16 opt_order = level + 2; // 2..16
        u16 opt_mem = 8 + level * 12; // 1..256 MB
        u16 opt_restore = 0; // 0..2 (only restore mode 0 works reliably)

        // compute header
        u16 header = (opt_restore << 12) | ((opt_mem - 1) << 4) | (opt_order - 1);

        // write header
        LittleEndianPointer p = dest.address;
        p.write16(header);
        dest.address += 2;
        dest.size -= 2;

        CPpmd8 ppmd;

        OutputStreamPPMD8 stream(dest);
        ppmd.Stream.Out = &stream;

        Ppmd8_Construct(&ppmd);
        Ppmd8_Alloc(&ppmd, opt_mem << 20, &g_Alloc);
        Ppmd8_RangeEnc_Init(&ppmd);
        Ppmd8_Init(&ppmd, opt_order, 0);

        for (size_t i = 0; i < source.size; ++i)
        {
            Ppmd8_EncodeSymbol(&ppmd, source.address[i]);
        }

        Ppmd8_EncodeSymbol(&ppmd, -1); // EndMark
        Ppmd8_RangeEnc_FlushData(&ppmd);
        Ppmd8_Free(&ppmd, &g_Alloc);

        CompressionStatus status;
        status.size = stream.memory.address + stream.offset - start;
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        // read 2 byte header
        LittleEndianConstPointer p = source.address;
        u16 header = p.read16();
        source.address += 2;
        source.size -= 2;

        // parse header
        u16 opt_order = (header & 0x000f) + 1;
        u16 opt_mem = ((header & 0x0ff0) >> 4) + 1;
        u16 opt_restore = ((header & 0xf000) >> 12);

        CPpmd8 ppmd;

        InputStreamPPMD8 stream(source);
        ppmd.Stream.In = &stream;

        Ppmd8_Construct(&ppmd);
        Ppmd8_Alloc(&ppmd, opt_mem << 20, &g_Alloc);
        Ppmd8_RangeDec_Init(&ppmd);
        Ppmd8_Init(&ppmd, opt_order, opt_restore);

        size_t offset = 0;
        for ( ;; )
        {
            int c = Ppmd8_DecodeSymbol(&ppmd);
            if (c < 0 || offset >= dest.size)
                break;
            dest.address[offset++] = c;
        }

        CompressionStatus status;

        if (!Ppmd8_RangeDec_IsFinishedOK(&ppmd))
        {
            status.setError("[PPMd] decoding error.");
        }

        status.size = dest.size;
        return status;
    }

} // namespace ppmd

// ----------------------------------------------------------------------------
// zlib
// ----------------------------------------------------------------------------

namespace zlib
{

    static
    const char* get_error_string(int result)
    {
        const char* error = nullptr;
        switch (result)
        {
            default:
            case Z_OK:
                break;
            case Z_MEM_ERROR:
                error = "Z_MEM_ERROR";
                break;
            case Z_BUF_ERROR:
                error = "Z_BUF_ERROR";
                break;
            case Z_STREAM_ERROR:
                error = "Z_STREAM_ERROR";
                break;
        }
        return error;
    }

    size_t bound(size_t size)
    {
        return uLong(::compressBound(uLong(size)));
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        level = math::clamp(level, 0, 9);

        uLongf destLen = uLongf(dest.size);

        int result = ::compress2(dest.address, &destLen, source.address, uLong(source.size), level);
        const char* error = get_error_string(result);

        CompressionStatus status;

        if (error)
        {
            status.setError("[zlib] %s.", error);
        }
        else
        {
            status.size = size_t(destLen);
        }

        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        uLongf sourceLen = uLongf(source.size);
        uLongf destLen = uLongf(dest.size);

        int result = uncompress2(dest.address, &destLen, source.address, &sourceLen);
        const char* error = get_error_string(result);

        CompressionStatus status;

        if (error)
        {
            status.setError("[zlib] %s.", error);
        }
        else
        {
            status.size = size_t(destLen);
        }

        return status;
    }

} // namespace zlib

// ----------------------------------------------------------------------------
// deflate
// ----------------------------------------------------------------------------

namespace deflate
{

    static
    const char* get_error_string(libdeflate_result result)
    {
        const char* error = nullptr; // default: no error
        switch (result)
        {
            default:
            case LIBDEFLATE_SUCCESS:
                break;
            case LIBDEFLATE_BAD_DATA:
                error = "Bad data";
                break;
            case LIBDEFLATE_SHORT_OUTPUT:
                error = "Short output";
                break;
            case LIBDEFLATE_INSUFFICIENT_SPACE:
                error = "Insufficient space";
                break;
        }
        return error;
    }

    size_t bound(size_t size)
    {
        return libdeflate_deflate_compress_bound(nullptr, size);
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        level = math::clamp(level, 1, 10);
        if (level >= 8) level = (level * 12) / 10;

        libdeflate_compressor* compressor = libdeflate_alloc_compressor(level);
        size_t bytes_out = libdeflate_deflate_compress(compressor, source, source.size, dest, dest.size);
        libdeflate_free_compressor(compressor);

        CompressionStatus status;
        status.size = bytes_out;
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();

        size_t bytes_out = 0;
        libdeflate_result result = libdeflate_deflate_decompress(decompressor, source, source.size, dest, dest.size, &bytes_out);
        libdeflate_free_decompressor(decompressor);

        CompressionStatus status;

        const char* error = deflate::get_error_string(result);
        if (error)
        {
            status.setError("[deflate] %s.", error);
        }

        status.size = bytes_out;
        return status;
    }

} // namespace deflate

// ----------------------------------------------------------------------------
// deflate_zlib
// ----------------------------------------------------------------------------

namespace deflate_zlib
{

    size_t bound(size_t size)
    {
        return libdeflate_zlib_compress_bound(nullptr, size);
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        level = math::clamp(level, 1, 10);
        if (level >= 8) level = (level * 12) / 10;

        libdeflate_compressor* compressor = libdeflate_alloc_compressor(level);
        size_t bytes_out = libdeflate_zlib_compress(compressor, source, source.size, dest, dest.size);
        libdeflate_free_compressor(compressor);

        CompressionStatus status;
        status.size = bytes_out;
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();

        size_t bytes_out = 0;
        libdeflate_result result = libdeflate_zlib_decompress(decompressor, source, source.size, dest, dest.size, &bytes_out);
        libdeflate_free_decompressor(decompressor);

        CompressionStatus status;

        const char* error = deflate::get_error_string(result);
        if (error)
        {
            status.setError("[deflate_zlib] %s.", error);
        }

        status.size = bytes_out;
        return status;
    }

} // namespace deflate_zlib

// ----------------------------------------------------------------------------
// deflate_gzip
// ----------------------------------------------------------------------------

namespace deflate_gzip
{

    size_t bound(size_t size)
    {
        return libdeflate_gzip_compress_bound(nullptr, size);
    }

    CompressionStatus compress(Memory dest, ConstMemory source, int level)
    {
        level = math::clamp(level, 1, 10);
        if (level >= 8) level = (level * 12) / 10;

        libdeflate_compressor* compressor = libdeflate_alloc_compressor(level);
        size_t bytes_out = libdeflate_gzip_compress(compressor, source, source.size, dest, dest.size);
        libdeflate_free_compressor(compressor);

        CompressionStatus status;
        status.size = bytes_out;
        return status;
    }

    CompressionStatus decompress(Memory dest, ConstMemory source)
    {
        libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();

        size_t bytes_out = 0;
        libdeflate_result result = libdeflate_gzip_decompress(decompressor, source, source.size, dest, dest.size, &bytes_out);
        libdeflate_free_decompressor(decompressor);

        CompressionStatus status;

        const char* error = deflate::get_error_string(result);
        if (error)
        {
            status.setError("[deflate_gzip] %s.", error);
        }

        status.size = bytes_out;
        return status;
    }

} // namespace deflate_gzip

    const std::vector<Compressor> g_compressors =
    {
        { Compressor::NONE,    "none",  nocompress::bound, nocompress::compress, nocompress::decompress },
        { Compressor::BZIP2,   "bzip2", bzip2::bound, bzip2::compress, bzip2::decompress },
        { Compressor::LZ4,     "lz4",   lz4::bound,   lz4::compress,   lz4::decompress },
        { Compressor::LZO,     "lzo",   lzo::bound,   lzo::compress,   lzo::decompress },
        { Compressor::ZSTD,    "zstd",  zstd::bound,  zstd::compress,  zstd::decompress },
        { Compressor::LZFSE,   "lzfse", lzfse::bound, lzfse::compress, lzfse::decompress },
        { Compressor::LZMA,    "lzma",  lzma::bound,  lzma::compress,  lzma::decompress },
        { Compressor::LZMA2,   "lzma2", lzma2::bound, lzma2::compress, lzma2::decompress },
        { Compressor::PPMD8,   "ppmd8", ppmd8::bound, ppmd8::compress, ppmd8::decompress },
        { Compressor::ZLIB,    "zlib",  zlib::bound,  zlib::compress,  zlib::decompress },
        { Compressor::DEFLATE, "deflate",      deflate::bound,      deflate::compress,      deflate::decompress },
        { Compressor::DEFLATE_ZLIB, "deflate_zlib", deflate_zlib::bound, deflate_zlib::compress, deflate_zlib::decompress },
        { Compressor::DEFLATE_GZIP, "deflate_gzip", deflate_gzip::bound, deflate_gzip::compress, deflate_gzip::decompress },
    };

    std::vector<Compressor> getCompressors()
    {
        return g_compressors;
    }

    Compressor getCompressor(Compressor::Method method)
    {
        return g_compressors[method];
    }

    Compressor getCompressor(const std::string& name)
    {
        Compressor compressor;

        auto i = std::find_if(g_compressors.begin(), g_compressors.end(),[&] (const Compressor& compressor)
        {
            return name == compressor.name;
        });

        if (i != g_compressors.end())
        {
            compressor = *i;
        }
        else
        {
            MANGO_EXCEPTION("[WARNING] Incorrect compressor (%s).", name.c_str());
        }

        return compressor;
    }

} // namespace mango
