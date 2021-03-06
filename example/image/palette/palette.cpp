/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::image;

void encode_indexed(Surface surface, const std::string& filename)
{
    Bitmap temp(surface.width, surface.height, IndexedFormat(8));

    u64 time0 = Time::ms();

    printf("Quantizing: ");
    image::ColorQuantizer quantizer(surface, 0.80f);
    quantizer.quantize(temp, surface, true);

    u64 time1 = Time::ms();
    printf("%d ms\n", int(time1 - time0));

    printf("Encoding: ");
    ImageEncodeOptions options;
    options.palette = quantizer.getPalette();

    temp.save(filename, options);

    u64 time2 = Time::ms();
    printf("%d ms\n", int(time2 - time1));
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Too few arguments. usage: <image filename>\n");
        exit(1);
    }

    const char* filename = argv[1];

    Bitmap bitmap(filename, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

    encode_indexed(bitmap, "output-palette.gif");
    encode_indexed(bitmap, "output-palette.png");
}
