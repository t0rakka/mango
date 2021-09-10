/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

// TODO: encoder status
// TODO: decoder status
// TODO: encode: Y, RGB, BGR, RGBA, BGRA
// TODO: decode: Y, RGB, BGR, RGBA, BGRA

void decode(ConstMemory memory, const std::string& filename)
{
    ImageDecoder decoder(memory, filename);
    if (decoder.isDecoder())
    {
        ImageHeader header = decoder.header();
        Bitmap bitmap(header.width, header.height, header.format);

        ImageDecodeOptions options;
        options.simd = true;
        options.multithread = true;

        ImageDecodeStatus status = decoder.decode(bitmap, options);

        printf("Status: %s\n", status.info.c_str());
    }
}

void jpeg_analyze(const std::string& filename)
{
    File file(filename);

    u64 time0 = Time::ms();

    decode(file, filename);

    u64 time1 = Time::ms();
    printf("Decode: %d ms\n", int(time1 - time0));
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Too few arguments. Usage: %s <filename.jpg>\n", argv[0]);
        return 1;
    }

    //debugPrintEnable(true);

    std::string filename = argv[1];
    jpeg_analyze(filename);
}
