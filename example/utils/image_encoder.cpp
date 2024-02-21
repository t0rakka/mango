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
        // TODO: print help message
        return 0;
    }

    ConcurrentQueue q;

    for (int i = 1; i < argc; ++i)
    {
        // TODO: check if supported image format
        std::string filename = argv[i];

        q.enqueue([=]
        {
            printLine("Processing: {}", filename);

            Bitmap bitmap(filename);

            // TODO: select encoding options from command line
            ImageEncodeOptions options;
            options.compression = 10;

            // NOTE: always override input file, might want to be able to configure output filename
            //       - we could also change formats here:  .jpg -> .png, etc.
            bitmap.save(filename, options);
        });
    }

    q.wait();
}
