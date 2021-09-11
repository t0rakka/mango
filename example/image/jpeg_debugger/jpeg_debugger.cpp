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

void warmup(ConstMemory memory)
{
    debugPrintEnable(true);

    ImageDecoder decoder(memory, ".jpg");
    if (decoder.isDecoder())
    {
        ImageHeader header = decoder.header();
        Bitmap bitmap(header.width, header.height, header.format);

        ImageDecodeOptions options;
        options.simd = true;
        options.multithread = true;

        ImageDecodeStatus status = decoder.decode(bitmap, options);
        MANGO_UNREFERENCED(status);
    }

    debugPrintEnable(false);
}

void decode(ConstMemory memory, const std::string& name, const Format& format)
{
    u64 decode_time = 0;
    u64 encode_time = 0;

    std::string decode_info;
    std::string encode_info;

    ImageDecoder decoder(memory, ".jpg");
    if (decoder.isDecoder())
    {
        ImageHeader header = decoder.header();
        Bitmap bitmap(header.width, header.height, format);

        ImageDecodeOptions options;
        options.simd = true;
        options.multithread = false;

        u64 time0 = Time::us();

        ImageDecodeStatus status = decoder.decode(bitmap, options);
        u64 time1 = Time::us();

        decode_time = time1 - time0;
        decode_info = status.info;

        ImageEncoder encoder(".jpg");
        if (encoder.isEncoder())
        {
            std::string filename = name + ".jpg";
            filesystem::OutputFileStream file(filename);

            ImageEncodeOptions options;
            options.simd = true;
            options.multithread = false;

            u64 time0 = Time::us();

            ImageEncodeStatus status = encoder.encode(file, bitmap, options);
            u64 time1 = Time::us();

            encode_time = time1 - time0;
            encode_info = status.info;
        }
    }

    printf("Decode: %7d us  [%s]\n", int(decode_time), decode_info.c_str());
    printf("Encode: %7d us  [%s]\n", int(encode_time), encode_info.c_str());
    printf("\n");
}

void jpeg_analyze(const std::string& filename)
{
    File file(filename);

    warmup(file);

    decode(file, "L   ", LuminanceFormat(8, Format::UNORM, 8, 0));
    decode(file, "RGBA", Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
    decode(file, "BGRA", Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));
    decode(file, "RGB ", Format(24, Format::UNORM, Format::RGB, 8, 8, 8));
    decode(file, "BGR ", Format(24, Format::UNORM, Format::BGR, 8, 8, 8));
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Too few arguments. Usage: %s <filename.jpg>\n", argv[0]);
        return 1;
    }

    std::string filename = argv[1];
    jpeg_analyze(filename);
}
