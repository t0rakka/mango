/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::image;

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Too few arguments. usage: <image>\n");
        return 1;
    }

    printf("%s\n", getSystemInfo().c_str());

    std::string filename = argv[1];
    if (filesystem::getExtension(filename) != ".jpg")
    {
        printf("This is a jpeg/webp comparison; the file must be in .jpg format.\n");
        return 1;
    }

    u64 time0 = Time::ms();

    Bitmap bitmap(filename);
    u64 time1 = Time::ms();

    bitmap.save("output.webp");
    u64 time2 = Time::ms();

    Bitmap bitmap2("output.webp");
    u64 time3 = Time::ms();

    bitmap2.save("output.jpg");
    u64 time4 = Time::ms();

    printf("jpeg decode: %d ms\n", u32(time1 - time0));
    printf("jpeg encode: %d ms\n", u32(time4 - time3));
    printf("webp decode: %d ms\n", u32(time3 - time2));
    printf("webp encode: %d ms\n", u32(time2 - time1));
}
