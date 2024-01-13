/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::image;

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printLine("Too few arguments. usage: <image>");
        return 1;
    }

    printLine(getSystemInfo());

    std::string filename = argv[1];
    if (filesystem::getExtension(filename) != ".jpg")
    {
        printLine("This is a jpeg/webp comparison; the file must be in .jpg format.");
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

    printLine("jpeg decode: {} ms", time1 - time0);
    printLine("jpeg encode: {} ms", time4 - time3);
    printLine("webp decode: {} ms", time3 - time2);
    printLine("webp encode: {} ms", time2 - time1);
}
