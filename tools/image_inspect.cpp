/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <iostream>
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::image;

namespace
{

    void configureParser(CommandLineParser& parser)
    {
        parser.usage("<image> [...]");
    }

} // namespace

int main(int argc, const char* argv[])
{
    CommandLineParser parser;
    configureParser(parser);

    if (argc < 2)
    {
        parser.printHelp();
        return 1;
    }

    if (!parser.parse(argc, argv))
    {
        return 1;
    }

    const auto& filenames = parser.positionals();
    if (filenames.empty())
    {
        parser.printHelp();
        return 1;
    }

    int status = 0;

    for (std::string_view filename : filenames)
    {
        try
        {
            const std::string path(filename);
            filesystem::File input(path);
            ImageInspect report = inspect(ConstMemory(input), path);

            if (filenames.size() > 1)
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
