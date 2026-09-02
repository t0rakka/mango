/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <array>
#include <vector>

using namespace mango;
using namespace mango::image;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    // 1x1 indexed PNG, 2-entry PLTE (black, red), pixel index 1.
    constexpr u8 kPngIndexed1x1[] =
    {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x03, 0x00, 0x00, 0x00, 0x28, 0xcb, 0x34,
        0xbb, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x1b,
        0xff, 0x8d, 0x22, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0x60, 0x04,
        0x00, 0x00, 0x03, 0x00, 0x02, 0xe6, 0x7d, 0xa7, 0x67, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e,
        0x44, 0xae, 0x42, 0x60, 0x82
    };

    // 1x1 8 bpp BMP, 2 color table entries, preferred decode format is BGRA.
    constexpr u8 kBmpIndexed1x1[] =
    {
        0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x01, 0x00,
        0x00, 0x00
    };

    bool check_header_palette(ConstMemory memory, const char* extension, int expected_palette, bool expect_indexed_format)
    {
        ImageDecoder decoder(memory, extension);
        CHECK(decoder.isDecoder());

        ImageHeader header = decoder.header();
        CHECK(header.success);
        CHECK(header.width == 1);
        CHECK(header.height == 1);
        CHECK(header.palette == expected_palette);
        CHECK(header.format.isIndexed() == expect_indexed_format);

        ImageInspect report = inspect(memory, extension);
        CHECK(report.success);
        CHECK(report.palette_colors == expected_palette);
        CHECK(report.header.palette == expected_palette);

        return true;
    }

    bool test_png_indexed_header_and_inspect()
    {
        return check_header_palette(ConstMemory(kPngIndexed1x1, sizeof(kPngIndexed1x1)), "fixture.png", 2, true);
    }

    bool test_bmp_indexed_header_and_inspect()
    {
        // BMP expands indexed sources to BGRA, but still reports palette size.
        return check_header_palette(ConstMemory(kBmpIndexed1x1, sizeof(kBmpIndexed1x1)), "fixture.bmp", 2, false);
    }

    bool test_gif_encode_roundtrip_palette()
    {
        Palette palette(16);
        for (int i = 0; i < 16; ++i)
        {
            palette[i] = Color(u8(i * 15), 0, 0, 255);
        }

        std::array<u8, 1> indices { 3 };
        Surface surface(1, 1, IndexedFormat(8), 1, indices.data(), &palette);

        Buffer encoded;
        MemoryStream stream;
        ImageEncodeStatus status = surface.save(stream, ".gif");
        CHECK(status.success);
        CHECK(stream.size() > 0);

        ConstMemory encoded_memory = stream;
        ImageDecoder decoder(encoded_memory, ".gif");
        CHECK(decoder.isDecoder());

        ImageHeader header = decoder.header();
        CHECK(header.success);
        CHECK(header.width == 1);
        CHECK(header.height == 1);
        CHECK(header.palette > 0);
        CHECK(header.format.isIndexed());

        ImageInspect report = inspect(encoded_memory, "roundtrip.gif");
        CHECK(report.success);
        CHECK(report.palette_colors == header.palette);

        return true;
    }

    bool test_pcx_indexed_header_and_inspect()
    {
        std::vector<u8> pcx(128, 0);
        pcx[0] = 0x0a; // manufacturer
        pcx[1] = 5;    // version
        pcx[2] = 1;    // RLE
        pcx[3] = 8;    // bits per pixel
        // xmin/ymin/xmax/ymax = 0 => 1x1
        pcx[65] = 1;   // NPlanes
        pcx[66] = 1;   // BytesPerLine low
        pcx[67] = 0;   // BytesPerLine high
        pcx.push_back(1); // one raw index byte
        pcx.push_back(0x0c); // VGA palette marker
        pcx.insert(pcx.end(), 768, 0);

        return check_header_palette(ConstMemory(pcx.data(), pcx.size()), "fixture.pcx", 256, true);
    }

    bool test_truecolor_bmp_has_no_palette()
    {
        Bitmap bitmap(2, 2, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));
        bitmap.clear(0xff112233);

        MemoryStream stream;
        CHECK(bitmap.save(stream, ".bmp").success);

        ConstMemory encoded_memory = stream;
        ImageDecoder decoder(encoded_memory, ".bmp");
        CHECK(decoder.isDecoder());

        ImageHeader header = decoder.header();
        CHECK(header.success);
        CHECK(header.palette == 0);

        ImageInspect report = inspect(encoded_memory, "truecolor.bmp");
        CHECK(report.success);
        CHECK(report.palette_colors == 0);

        return true;
    }

    const Case cases[] =
    {
        { "png_indexed_header_and_inspect", test_png_indexed_header_and_inspect },
        { "bmp_indexed_header_and_inspect", test_bmp_indexed_header_and_inspect },
        { "gif_encode_roundtrip_palette", test_gif_encode_roundtrip_palette },
        { "pcx_indexed_header_and_inspect", test_pcx_indexed_header_and_inspect },
        { "truecolor_bmp_has_no_palette", test_truecolor_bmp_has_no_palette },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("image_inspect", cases, std::size(cases), argc, argv);
}
