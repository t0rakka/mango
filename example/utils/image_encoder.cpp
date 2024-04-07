/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::image;

void printHelp(std::string_view program)
{
    printLine("Usage: {} <options> <inputs>", program);
    printLine("  Options:");
    printLine("    -format .extension");
    printLine("    -compression level(0..10)");
    printLine("    --info");
}

int main(int argc, const char* argv[])
{
    std::string_view program = argv[0];

    if (argc < 2)
    {
        printHelp(program);
        return 0;
    }

    std::string request_format;

    ImageEncodeOptions options;
    options.compression = 8;

    int index = 1;

    while (index < argc)
    {
        if (std::string_view(argv[index]) == "-format")
        {
            if (++index < argc)
            {
                std::string extension(argv[index++]);
                if (isImageEncoder(extension))
                {
                    printLine("Active output format: {}", extension);
                    request_format = extension;
                }
                else
                {
                    printLine("Unsupported output format: {}", extension);
                }
            }
            else
            {
                printHelp(program);
                return 0;
            }
        }
        if (std::string_view(argv[index]) == "-compression")
        {
            if (++index < argc)
            {
                int level = std::atoi(argv[index++]);
                options.compression = level;
            }
            else
            {
                printHelp(program);
                return 0;
            }
        }
        else if (std::string_view(argv[index]) == "--info")
        {
            ++index;
            printEnable(Print::Info, true);
        }
        else
        {
            break;
        }
    }

    ConcurrentQueue q;

    for (int i = index; i < argc; ++i)
    {
        std::string filename = argv[i];

        if (!isImageDecoder(filename))
        {
            printLine("Skipping: \"{}\"", filename);
            continue;
        }

        std::string output = filesystem::removePath(filename);
        if (!request_format.empty())
        {
            output = filesystem::removeExtension(output) + request_format;
        }

        printLine("Processing: \"{}\" --> \"{}\"", filename, output);

        q.enqueue([=]
        {
            Bitmap bitmap(filename);
            bitmap.save(output, options);
        });
    }

    q.wait();
}
