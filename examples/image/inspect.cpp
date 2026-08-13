/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <iostream>
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::image;

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: inspect <image> [...]\n";
        return 1;
    }

    int status = 0;

    for (int i = 1; i < argc; ++i)
    {
        const std::string filename = argv[i];

        try
        {
            filesystem::File file(filename);
            ImageInspect report = inspect(ConstMemory(file), filename);

            if (argc > 2)
                std::cout << filename << ":\n";

            std::cout << formatImageInspect(report) << "\n";

            if (!report.success)
                status = 1;
        }
        catch (const Exception& e)
        {
            std::cout << filename << ": " << e.what() << "\n";
            status = 1;
        }
    }

    return status;
}
