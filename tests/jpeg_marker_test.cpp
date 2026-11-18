// Copyright 2026 Distance Technologies Oy. For internal use only.
//
#include "core_test.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

using namespace mango;
using namespace mango::image;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    ImageHeader open_header(const u8* data, size_t size)
    {
        ConstMemory memory(data, size);
        ImageDecoder decoder(memory, "sample.jpg");
        return decoder.header();
    }

    bool header_rejects(const u8* data, size_t size)
    {
        ImageHeader header = open_header(data, size);
        CHECK(!header);
        return true;
    }

    bool test_truncated_dqt()
    {
        // SOI, DQT length 3 (one payload byte). The 64 table entries are absent.
        const u8 data[] = { 0xff, 0xd8, 0xff, 0xdb, 0x00, 0x03, 0x00, 0xff, 0xd9 };
        return header_rejects(data, sizeof(data));
    }

    bool test_dqt_length_past_eof()
    {
        // Length claims 67 bytes; four payload bytes follow the length word.
        const u8 data[] = { 0xff, 0xd8, 0xff, 0xdb, 0x00, 0x43, 0x00, 0x01, 0x02, 0x03 };
        return header_rejects(data, sizeof(data));
    }

    bool test_truncated_sof()
    {
        const u8 data[] = { 0xff, 0xd8, 0xff, 0xc0, 0x00, 0x08, 0x08, 0x00 };
        return header_rejects(data, sizeof(data));
    }

    bool test_short_app2()
    {
        // APP2 payload is 6 bytes. The ICC identifier is 12.
        const u8 data[] =
        {
            0xff, 0xd8, 0xff, 0xe2, 0x00, 0x08,
            0x49, 0x43, 0x43, 0x5f, 0x50, 0x52,
            0xff, 0xd9
        };
        ImageHeader header = open_header(data, sizeof(data));
        CHECK(header.width == 0);
        return true;
    }

    bool test_roundtrip()
    {
        const Format rgba(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
        Bitmap bitmap(8, 8, rgba);
        std::memset(bitmap.image, 0x3c, bitmap.stride * bitmap.height);

        MemoryStream encoded;
        ImageEncodeOptions encode_options;
        encode_options.quality = 0.90f;
        encode_options.multithread = false;
        encode_options.simd = true;

        ImageEncoder encoder("jpg");
        ImageEncodeStatus encoded_status = encoder.encode(encoded, bitmap, encode_options);
        CHECK(encoded_status);

        ConstMemory jpg = encoded;
        ImageDecoder decoder(jpg, "round.jpg");
        ImageHeader header = decoder.header();
        CHECK(header);
        CHECK(header.width == 8);
        CHECK(header.height == 8);

        Bitmap decoded(header.width, header.height, rgba);
        ImageDecodeOptions decode_options;
        decode_options.multithread = false;
        decode_options.simd = true;
        ImageDecodeStatus status = decoder.decode(decoded, decode_options);
        CHECK(status);
        return true;
    }

}

int main(int argc, char* argv[])
{
    const Case cases[] =
    {
        { "truncated DQT", test_truncated_dqt },
        { "DQT length past end", test_dqt_length_past_eof },
        { "truncated SOF", test_truncated_sof },
        { "short APP2", test_short_app2 },
        { "roundtrip", test_roundtrip },
    };

    return run_cases("jpeg marker", cases, std::size(cases), argc, argv);
}
