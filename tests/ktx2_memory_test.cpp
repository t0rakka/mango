/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <cstdlib>

using namespace mango;
using namespace mango::image;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool test_truncated_header_fails()
    {
        ConstMemory empty;
        ImageDecoder decoder(empty, ".ktx2");
        CHECK(decoder.isDecoder());
        ImageHeader h = decoder.header();
        CHECK(!h.success);
        return true;
    }

    bool test_optional_ktx2_file(const char* path)
    {
        filesystem::File file(path);
        if (!file.size())
        {
            printLine("ktx2_memory_test: skip (missing or empty \"{}\")", path);
            return true;
        }

        ConstMemory memory(file);
        ImageDecoder decoder(memory, ".ktx2");
        CHECK(decoder.isDecoder());

        ImageHeader h = decoder.header();
        CHECK(h.success);
        CHECK(h.width > 0);
        CHECK(h.height > 0);

        const bool expect_native_blob = h.compression != TextureCompression::NONE ||
            (h.supercompression & SUPERCOMPRESS_BASISU_UASTC_HDR) != 0;

        ImageDecodeOptions gpu;
        gpu.compression = TextureCompression::NONE;

        if (expect_native_blob)
        {
            ConstMemory level0 = decoder.memory(0, 0, 0, gpu);
            CHECK(level0.address != nullptr);
            CHECK(level0.size > 0);
            CHECK(decoder.header().compression != TextureCompression::NONE);
        }

        ImageDecodeOptions bc7;
        bc7.compression = TextureCompression::BC7_UNORM;
        if (h.supercompression & SUPERCOMPRESS_BC7_UNORM)
        {
            ConstMemory bc7_level = decoder.memory(0, 0, 0, bc7);
            CHECK(bc7_level.address != nullptr);
            CHECK(bc7_level.size > 0);
        }

        ImageDecodeOptions bc6h;
        bc6h.compression = TextureCompression::BC6H_UF16;
        if (h.supercompression & SUPERCOMPRESS_BC6H_UF16)
        {
            ConstMemory bc6h_level = decoder.memory(0, 0, 0, bc6h);
            CHECK(bc6h_level.address != nullptr);
            CHECK(bc6h_level.size > 0);
        }

        printLine("ktx2_memory_test: ok \"{}\" ({} x {})", path, h.width, h.height);
        return true;
    }

} // namespace

int main(int argc, char* argv[])
{
    static const Case cases[] =
    {
        { "truncated", test_truncated_header_fails },
    };

    const int status = run_cases("ktx2_memory_test", cases, std::size(cases), argc, argv);
    if (status != 0)
    {
        return status;
    }

    const char* path = std::getenv("MANGO_TEST_KTX2");
    if (!path && argc > 1)
    {
        path = argv[1];
    }

    if (path && !test_optional_ktx2_file(path))
    {
        return 1;
    }

    return 0;
}
