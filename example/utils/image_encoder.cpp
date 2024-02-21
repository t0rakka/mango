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
        return 0;
    }

    ConcurrentQueue q;

    for (int i = 1; i < argc; ++i)
    {
        std::string filename = argv[i];
        printLine("Processing: {}", filename);

        q.enqueue([=]
        {
            Bitmap bitmap(filename);


            ImageEncodeOptions options;
            options.compression = 10;

            bitmap.save(filename, options);
        });
    }

    q.wait();
}
