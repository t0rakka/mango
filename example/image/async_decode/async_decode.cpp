/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/filesystem/filesystem.hpp>

using namespace mango;
using namespace mango::image;
using namespace mango::filesystem;

void test(const char* filename, int cancel_ms)
{
    File file(filename);
    AsyncImageDecoder decoder(file, filename);

    ImageHeader header = decoder.header();
    Bitmap bitmap(header);

    u64 time0 = Time::ms();

    auto future = decoder.launch([] (const ImageDecodeRect& rect)
    {
        printLine("[+] Update: [{} x {}] .. ({}, {})",
            rect.width, rect.height, rect.x, rect.y);
    }, bitmap);

    if (cancel_ms >= 0)
    {
        Sleep::ms(cancel_ms);
        decoder.cancel();
    }

    future.get();

    u64 time1 = Time::ms();
    printLine("Decoding: {} ms", time1 - time0);

    bitmap.save("test.png");
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printLine("Too few arguments. usage: <filename.jpg> <cancel_ms>");
        exit(1);
    }

    const char* filename = argv[1];

    int cancel_ms = -1;
    if (argc == 3)
    {
        cancel_ms = std::atoi(argv[2]);
    }

    test(filename, cancel_ms);
}
