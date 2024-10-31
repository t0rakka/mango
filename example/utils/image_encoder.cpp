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
    printLine("    -format <.extension>");
    printLine("    -astc <width> <height>");
    printLine("    -output <filename>");
    printLine("    -compression <level:0..10>");
    printLine("    --luminance");
    printLine("    --linear");
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

    std::string output_filename;
    bool luminance = false;
    bool linear = false;

    int index = 1;

    std::vector<std::string> filenames;

    while (index < argc)
    {
        if (std::string_view(argv[index]) == "-output")
        {
            if (++index < argc)
            {
                output_filename = argv[index++];
            }
            else
            {
                printHelp(program);
                return 0;
            }
        }
        else if (std::string_view(argv[index]) == "-format")
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
        else if (std::string_view(argv[index]) == "-compression")
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
        else if (std::string_view(argv[index]) == "-astc")
        {
            ++index;

            if (index + 2 > argc)
            {
                printHelp(program);
                return 0;
            }

            options.astc_block_width = std::atoi(argv[index++]);
            options.astc_block_height = std::atoi(argv[index++]);
        }
        else if (std::string_view(argv[index]) == "--luminance")
        {
            ++index;
            luminance = true;
        }
        else if (std::string_view(argv[index]) == "--linear")
        {
            ++index;
            linear = true;
        }
        else if (std::string_view(argv[index]) == "--info")
        {
            ++index;
            printEnable(Print::Info, true);
        }
        else
        {
            std::string filename = argv[index++];
            if (isImageDecoder(filename))
            {
                filenames.push_back(filename);
            }
        }
    }

    ConcurrentQueue q;

    for (auto filename : filenames)
    {
        std::string output = filesystem::removePath(filename);
        if (!output_filename.empty() && filenames.size() == 1)
        {
            output = output_filename;
        }
        else if (!request_format.empty())
        {
            output = filesystem::removeExtension(output) + request_format;
        }

        printLine("Processing: \"{}\" --> \"{}\"", filename, output);

        q.enqueue([=]
        {
            std::unique_ptr<Bitmap> bitmap;

            if (luminance)
            {
                Bitmap temp(filename);
                bitmap = std::make_unique<LuminanceBitmap>(temp, false, linear);
            }
            else
            {
                bitmap = std::make_unique<Bitmap>(filename);

                if (linear && !bitmap->format.isLinear())
                {
                    srgbToLinear(*bitmap);
                }
            }

            bitmap->save(output, options);
        });
    }

    q.wait();
}
