/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <cstring>
#include <functional>
#include <memory>

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool mem_equal(ConstMemory a, ConstMemory b)
    {
        return a.size == b.size && std::memcmp(a.address, b.address, a.size) == 0;
    }

    bool roundtrip_block(
        size_t (*bound_fn)(size_t),
        CompressionStatus (*compress_fn)(Memory, ConstMemory, int),
        CompressionStatus (*decompress_fn)(Memory, ConstMemory),
        ConstMemory source,
        int level = 6)
    {
        Buffer compressed(bound_fn(source.size));

        CompressionStatus encoded = compress_fn(compressed, source, level);
        CHECK(encoded);
        if (source.size > 0)
        {
            CHECK(encoded.size > 0);
        }
        CHECK(encoded.size <= compressed.size());

        Buffer output(source.size);
        CompressionStatus decoded = decompress_fn(output, Memory(compressed, encoded.size));
        CHECK(decoded);
        CHECK(decoded.size == source.size);
        CHECK(mem_equal(source, output));

        return true;
    }

    void fill_pattern(Buffer& buffer)
    {
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            buffer.data()[i] = u8((i * 131 + 17) ^ (i >> 3));
        }
    }

    bool roundtrip_buffer(
        size_t (*bound_fn)(size_t),
        CompressionStatus (*compress_fn)(Memory, ConstMemory, int),
        CompressionStatus (*decompress_fn)(Memory, ConstMemory),
        size_t size,
        int level = 6)
    {
        Buffer source(size);
        fill_pattern(source);
        return roundtrip_block(bound_fn, compress_fn, decompress_fn, source, level);
    }

    bool test_nocompress_roundtrip()
    {
        const u8 source_bytes[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        ConstMemory source(source_bytes, sizeof(source_bytes));

        return roundtrip_block(nocompress::bound, nocompress::compress, nocompress::decompress, source);
    }

    bool test_zlib_roundtrip()
    {
        return roundtrip_buffer(zlib::bound, zlib::compress, zlib::decompress, 4096);
    }

    bool test_zstd_roundtrip()
    {
        return roundtrip_buffer(zstd::bound, zstd::compress, zstd::decompress, 8192);
    }

    bool test_deflate_roundtrip()
    {
        return roundtrip_buffer(deflate::bound, deflate::compress, deflate::decompress, 2048);
    }

    bool test_lzav_roundtrip()
    {
        return roundtrip_buffer(lzav::bound, lzav::compress, lzav::decompress, 16384);
    }

    bool test_get_compressor_by_name()
    {
        Compressor zstd = getCompressor("zstd");
        CHECK(zstd.method == Compressor::ZSTD);
        CHECK(zstd.bound != nullptr);
        CHECK(zstd.compress != nullptr);
        CHECK(zstd.decompress != nullptr);

        Buffer source(512);
        fill_pattern(source);
        Buffer compressed(zstd.bound(source.size()));
        Buffer output(source.size());

        CompressionStatus encoded = zstd.compress(compressed, source, 4);
        CHECK(encoded);

        CompressionStatus decoded = zstd.decompress(output, Memory(compressed, encoded.size));
        CHECK(decoded);
        CHECK(mem_equal(source, output));

        return true;
    }

    bool stream_roundtrip_chunks(
        const std::function<std::shared_ptr<StreamEncoder>(int)>& create_encoder,
        const std::function<std::shared_ptr<StreamDecoder>()>& create_decoder,
        ConstMemory source,
        size_t chunk_size)
    {
        auto encoder = create_encoder(4);
        auto decoder = create_decoder();
        CHECK(encoder != nullptr);
        CHECK(decoder != nullptr);

        Buffer compressed(encoder->bound(source.size) * 4);
        std::vector<size_t> compressed_sizes;
        std::vector<size_t> uncompressed_sizes;
        size_t compressed_offset = 0;

        for (size_t offset = 0; offset < source.size; offset += chunk_size)
        {
            const size_t bytes = std::min(chunk_size, source.size - offset);
            ConstMemory chunk(source.address + offset, bytes);
            Memory dest(compressed.data() + compressed_offset, compressed.size() - compressed_offset);

            const size_t written = encoder->encode(dest, chunk);
            CHECK(written > 0);

            compressed_sizes.push_back(written);
            uncompressed_sizes.push_back(bytes);
            compressed_offset += written;
        }

        Buffer output(source.size);
        size_t output_offset = 0;
        compressed_offset = 0;

        for (size_t i = 0; i < compressed_sizes.size(); ++i)
        {
            ConstMemory block(compressed.data() + compressed_offset, compressed_sizes[i]);
            Memory dest(output.data() + output_offset, uncompressed_sizes[i]);

            const size_t written = decoder->decode(dest, block);
            CHECK(written == uncompressed_sizes[i]);

            output_offset += written;
            compressed_offset += compressed_sizes[i];
        }

        CHECK(output_offset == source.size);
        CHECK(mem_equal(source, output));

        return true;
    }

    bool stream_incremental_decode(
        const std::function<std::shared_ptr<StreamEncoder>(int)>& create_encoder,
        const std::function<std::shared_ptr<StreamDecoder>()>& create_decoder,
        size_t chunk_size)
    {
        Buffer chunk_a(chunk_size);
        Buffer chunk_b(chunk_size);

        for (size_t i = 0; i < chunk_size; ++i)
        {
            chunk_a[i] = u8(i & 0xff);
            chunk_b[i] = u8((i * 7 + 13) & 0xff);
        }

        auto encoder = create_encoder(4);
        CHECK(encoder != nullptr);

        Buffer compressed(encoder->bound(chunk_size) * 2);
        std::vector<size_t> compressed_sizes;
        size_t compressed_offset = 0;

        for (ConstMemory chunk : { ConstMemory(chunk_a), ConstMemory(chunk_b) })
        {
            Memory dest(compressed.data() + compressed_offset, compressed.size() - compressed_offset);
            const size_t written = encoder->encode(dest, chunk);
            CHECK(written > 0);
            compressed_sizes.push_back(written);
            compressed_offset += written;
        }

        CHECK(compressed_sizes[0] != compressed_sizes[1] || !mem_equal(chunk_a, chunk_b));

        auto decoder = create_decoder();
        CHECK(decoder != nullptr);

        Buffer partial(chunk_size);
        ConstMemory first_block(compressed.data(), compressed_sizes[0]);
        CHECK(decoder->decode(partial, first_block) == chunk_size);
        CHECK(mem_equal(chunk_a, partial));

        Buffer tail(chunk_size);
        ConstMemory second_block(compressed.data() + compressed_sizes[0], compressed_sizes[1]);
        CHECK(decoder->decode(tail, second_block) == chunk_size);
        CHECK(mem_equal(chunk_b, tail));

        return true;
    }

    bool stream_stateful_encoding(
        const std::function<std::shared_ptr<StreamEncoder>(int)>& create_encoder,
        size_t chunk_size)
    {
        Buffer chunk(chunk_size, 0x42);

        auto encoder_a = create_encoder(4);
        auto encoder_b = create_encoder(4);
        CHECK(encoder_a != nullptr);
        CHECK(encoder_b != nullptr);

        Buffer compressed_a(encoder_a->bound(chunk_size));
        Buffer compressed_b(encoder_b->bound(chunk_size));

        const size_t bytes_a = encoder_a->encode(compressed_a, chunk);
        const size_t bytes_b = encoder_b->encode(compressed_b, chunk);

        CHECK(bytes_a > 0);
        CHECK(bytes_b > 0);

        Buffer prefix(16, 0x5a);
        auto encoder_prefixed = create_encoder(4);
        Buffer compressed_prefixed(encoder_prefixed->bound(chunk_size + prefix.size()));

        size_t offset = encoder_prefixed->encode(compressed_prefixed, prefix);
        offset += encoder_prefixed->encode(Memory(compressed_prefixed.data() + offset, compressed_prefixed.size() - offset), chunk);

        CHECK(offset != bytes_a);
        CHECK(std::memcmp(compressed_a.data(), compressed_prefixed.data(), std::min(bytes_a, offset)) != 0 ||
              bytes_a != offset);

        return true;
    }

    bool test_get_compressors_registry()
    {
        const std::vector<Compressor> compressors = getCompressors();
        CHECK(!compressors.empty());

        for (const Compressor& compressor : compressors)
        {
            CHECK(!compressor.name.empty());
            CHECK(compressor.bound != nullptr);
            CHECK(compressor.compress != nullptr);
            CHECK(compressor.decompress != nullptr);
        }

        Compressor zlib = getCompressor(Compressor::ZLIB);
        CHECK(zlib.method == Compressor::ZLIB);
        CHECK(zlib.name == "zlib");

        return true;
    }

    bool test_zstd_stream_chunks()
    {
        const size_t chunk_size = 1024;
        Buffer source(chunk_size * 3);
        fill_pattern(source);

        return stream_roundtrip_chunks(zstd::createStreamEncoder, zstd::createStreamDecoder, source, chunk_size);
    }

    bool test_zstd_stream_variable_chunks()
    {
        Buffer source(6000);
        fill_pattern(source);

        auto encoder = zstd::createStreamEncoder(4);
        auto decoder = zstd::createStreamDecoder();
        CHECK(encoder != nullptr);
        CHECK(decoder != nullptr);

        const size_t chunk_sizes[] = { 512, 2048, 1024, 2424 };
        Buffer compressed(encoder->bound(source.size()) * 4);

        std::vector<size_t> compressed_sizes;
        std::vector<size_t> uncompressed_sizes;
        size_t compressed_offset = 0;
        size_t source_offset = 0;

        for (size_t chunk_size : chunk_sizes)
        {
            ConstMemory chunk(source.data() + source_offset, chunk_size);
            Memory dest(compressed.data() + compressed_offset, compressed.size() - compressed_offset);

            const size_t written = encoder->encode(dest, chunk);
            CHECK(written > 0);

            compressed_sizes.push_back(written);
            uncompressed_sizes.push_back(chunk_size);
            compressed_offset += written;
            source_offset += chunk_size;
        }

        Buffer output(source.size());
        size_t output_offset = 0;
        compressed_offset = 0;

        for (size_t i = 0; i < compressed_sizes.size(); ++i)
        {
            ConstMemory block(compressed.data() + compressed_offset, compressed_sizes[i]);
            Memory dest(output.data() + output_offset, uncompressed_sizes[i]);

            const size_t written = decoder->decode(dest, block);
            CHECK(written == uncompressed_sizes[i]);

            output_offset += written;
            compressed_offset += compressed_sizes[i];
        }

        CHECK(mem_equal(source, output));

        return true;
    }

    bool test_zstd_stream_incremental_decode()
    {
        return stream_incremental_decode(zstd::createStreamEncoder, zstd::createStreamDecoder, 1024);
    }

    bool test_zstd_stream_stateful_encoding()
    {
        return stream_stateful_encoding(zstd::createStreamEncoder, 1024);
    }

    bool test_packbits_literal_run()
    {
        const u8 packed[] = { 0x02, 'A', 'B', 'C' };
        u8 output_bytes[3] = {};

        bool ok = packbits_decompress(Memory(output_bytes, sizeof(output_bytes)), ConstMemory(packed, sizeof(packed)));
        CHECK(ok);
        CHECK(output_bytes[0] == 'A');
        CHECK(output_bytes[1] == 'B');
        CHECK(output_bytes[2] == 'C');

        return true;
    }

    bool test_packbits_repeat_run()
    {
        const u8 packed[] = { 0xfe, 0xaa };
        u8 output_bytes[3] = {};

        bool ok = packbits_decompress(Memory(output_bytes, sizeof(output_bytes)), ConstMemory(packed, sizeof(packed)));
        CHECK(ok);
        CHECK(output_bytes[0] == 0xaa);
        CHECK(output_bytes[1] == 0xaa);
        CHECK(output_bytes[2] == 0xaa);

        return true;
    }

    bool test_packbits_mixed()
    {
        const u8 packed[] = { 0x00, 0x11, 0xfe, 0x22, 0x00, 0x33 };
        u8 output_bytes[5] = {};

        bool ok = packbits_decompress(Memory(output_bytes, sizeof(output_bytes)), ConstMemory(packed, sizeof(packed)));
        CHECK(ok);
        CHECK(output_bytes[0] == 0x11);
        CHECK(output_bytes[1] == 0x22);
        CHECK(output_bytes[2] == 0x22);
        CHECK(output_bytes[3] == 0x22);
        CHECK(output_bytes[4] == 0x33);

        return true;
    }

    bool test_packbits_noop()
    {
        const u8 packed[] = { 0x80, 0x00, 0x42 };
        u8 output_bytes[1] = {};

        bool ok = packbits_decompress(Memory(output_bytes, sizeof(output_bytes)), ConstMemory(packed, sizeof(packed)));
        CHECK(ok);
        CHECK(output_bytes[0] == 0x42);

        return true;
    }

    bool test_packbits_overflow()
    {
        const u8 packed[] = { 0x02, 'A', 'B', 'C' };
        u8 output_bytes[2] = {};

        bool ok = packbits_decompress(Memory(output_bytes, sizeof(output_bytes)), ConstMemory(packed, sizeof(packed)));
        CHECK(!ok);

        return true;
    }

    bool test_empty_roundtrip()
    {
        ConstMemory source;

        CHECK(roundtrip_block(nocompress::bound, nocompress::compress, nocompress::decompress, source));
        CHECK(roundtrip_block(zlib::bound, zlib::compress, zlib::decompress, source));
        CHECK(roundtrip_block(zstd::bound, zstd::compress, zstd::decompress, source));
        CHECK(roundtrip_block(deflate::bound, deflate::compress, deflate::decompress, source));

        return true;
    }

    bool test_tiny_roundtrip()
    {
        const u8 one[] = { 0x61 };
        const u8 three[] = { 0x61, 0x62, 0x63 };

        CHECK(roundtrip_block(zlib::bound, zlib::compress, zlib::decompress, ConstMemory(one, 1)));
        CHECK(roundtrip_block(zlib::bound, zlib::compress, zlib::decompress, ConstMemory(three, 3)));
        CHECK(roundtrip_block(zstd::bound, zstd::compress, zstd::decompress, ConstMemory(one, 1)));
        CHECK(roundtrip_block(zstd::bound, zstd::compress, zstd::decompress, ConstMemory(three, 3)));

        return true;
    }

    bool test_compression_levels()
    {
        Buffer source(1024);
        fill_pattern(source);

        CHECK(roundtrip_block(zstd::bound, zstd::compress, zstd::decompress, source, 0));
        CHECK(roundtrip_block(zstd::bound, zstd::compress, zstd::decompress, source, 10));
        CHECK(roundtrip_block(zlib::bound, zlib::compress, zlib::decompress, source, 0));
        CHECK(roundtrip_block(zlib::bound, zlib::compress, zlib::decompress, source, 10));

        return true;
    }

    bool test_corrupt_input_fails()
    {
        Buffer source(256);
        fill_pattern(source);

        Compressor zstd = getCompressor("zstd");
        Buffer compressed(zstd.bound(source.size()));
        CompressionStatus encoded = zstd.compress(compressed, source, 6);
        CHECK(encoded);
        CHECK(encoded.size > 0);

        compressed[0] ^= 0xff;

        Buffer output(source.size());
        CompressionStatus decoded = zstd.decompress(output, Memory(compressed, encoded.size));
        if (decoded)
        {
            CHECK(!mem_equal(source, output));
        }

        return true;
    }

#if defined(MANGO_ENABLE_LZ4)

    bool test_lz4_roundtrip()
    {
        return roundtrip_buffer(lz4::bound, lz4::compress, lz4::decompress, 4096);
    }

    bool test_lz4_stream_chunks()
    {
        Buffer source(4096);
        fill_pattern(source);
        return stream_roundtrip_chunks(lz4::createStreamEncoder, lz4::createStreamDecoder, source, 512);
    }

    bool test_lz4_stream_incremental_decode()
    {
        return stream_incremental_decode(lz4::createStreamEncoder, lz4::createStreamDecoder, 512);
    }

    bool test_lz4_stream_stateful_encoding()
    {
        return stream_stateful_encoding(lz4::createStreamEncoder, 512);
    }

#endif

    const Case g_cases [] =
    {
        { "nocompress roundtrip", test_nocompress_roundtrip },
        { "zlib roundtrip", test_zlib_roundtrip },
        { "zstd roundtrip", test_zstd_roundtrip },
        { "deflate roundtrip", test_deflate_roundtrip },
        { "lzav roundtrip", test_lzav_roundtrip },
        { "get compressor by name", test_get_compressor_by_name },
        { "get compressors registry", test_get_compressors_registry },
        { "zstd stream chunks", test_zstd_stream_chunks },
        { "zstd stream variable chunks", test_zstd_stream_variable_chunks },
        { "zstd stream incremental decode", test_zstd_stream_incremental_decode },
        { "zstd stream stateful encoding", test_zstd_stream_stateful_encoding },
        { "packbits literal run", test_packbits_literal_run },
        { "packbits repeat run", test_packbits_repeat_run },
        { "packbits mixed", test_packbits_mixed },
        { "packbits noop", test_packbits_noop },
        { "packbits overflow", test_packbits_overflow },
        { "empty roundtrip", test_empty_roundtrip },
        { "tiny roundtrip", test_tiny_roundtrip },
        { "compression levels", test_compression_levels },
        { "corrupt input fails", test_corrupt_input_fails },
#if defined(MANGO_ENABLE_LZ4)
        { "lz4 roundtrip", test_lz4_roundtrip },
        { "lz4 stream chunks", test_lz4_stream_chunks },
        { "lz4 stream incremental decode", test_lz4_stream_incremental_decode },
        { "lz4 stream stateful encoding", test_lz4_stream_stateful_encoding },
#endif
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_compress", g_cases, std::size(g_cases), argc, argv);
}
