/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::image;

// sRGB to linear table
const u8 decode_srgb_table [] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
    0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c,
    0x0c, 0x0d, 0x0d, 0x0e, 0x0e, 0x0f, 0x0f, 0x10, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x14,
    0x14, 0x15, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1b, 0x1c, 0x1c, 0x1d,
    0x1e, 0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 0x27, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3d, 0x3e, 0x3f, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
    0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x5c, 0x5d, 0x5e, 0x5f, 0x61, 0x62, 0x63, 0x65, 0x66, 0x67, 0x69, 0x6a, 0x6b, 0x6d,
    0x6e, 0x70, 0x71, 0x72, 0x74, 0x75, 0x77, 0x78, 0x7a, 0x7b, 0x7d, 0x7e, 0x80, 0x81, 0x83, 0x84,
    0x86, 0x87, 0x89, 0x8a, 0x8c, 0x8e, 0x8f, 0x91, 0x93, 0x94, 0x96, 0x98, 0x99, 0x9b, 0x9d, 0x9e,
    0xa0, 0xa2, 0xa4, 0xa5, 0xa7, 0xa9, 0xab, 0xad, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xb9, 0xbb,
    0xbd, 0xbf, 0xc1, 0xc3, 0xc5, 0xc7, 0xc9, 0xcb, 0xcd, 0xcf, 0xd1, 0xd3, 0xd5, 0xd7, 0xd9, 0xdb,
    0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xed, 0xef, 0xf1, 0xf3, 0xf5, 0xf8, 0xfa, 0xfc, 0xff 
};

void srgbToLinear(Bitmap& bitmap)
{
    for (int y = 0; y < bitmap.height; ++y)
    {
        u8* scan = bitmap.address(0, y);

        for (int x = 0; x < bitmap.width; ++x)
        {
            u8 r = scan[x * 4 + 0];
            u8 g = scan[x * 4 + 1];
            u8 b = scan[x * 4 + 2];
            scan[x * 4 + 0] = decode_srgb_table[r];
            scan[x * 4 + 1] = decode_srgb_table[g];
            scan[x * 4 + 2] = decode_srgb_table[b];
        }
    }
}

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
                filesystem::File file(filename);
                ImageDecoder decoder(file, filename);
                ImageHeader header = decoder.header();

                if (header.linear != linear)
                {
                    bitmap = std::make_unique<Bitmap>(header.width, header.height, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
                    decoder.decode(*bitmap);
                    srgbToLinear(*bitmap);
                }
                else
                {
                    bitmap = std::make_unique<Bitmap>(header.width, header.height, header.format);
                    decoder.decode(*bitmap);
                }
            }

            bitmap->save(output, options);
        });
    }

    q.wait();
}
