/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

namespace
{

    struct ImageEncoderArgs
    {
        ImageEncodeOptions options;
        std::string output_filename;
        std::string request_format;
        std::vector<std::string> filenames;

        bool luminance = false;
        bool linear = false;
        bool lossless = false;

        std::string error;
    };

    bool parseAstcBlockSize(std::string_view value, int& width, int& height)
    {
        const size_t sep = value.find_first_of("xX,");
        if (sep == std::string_view::npos)
        {
            return false;
        }

        width = std::atoi(value.substr(0, sep).data());
        height = std::atoi(value.substr(sep + 1).data());

        return width > 0 && height > 0;
    }

    std::string normalizeFormatExtension(std::string_view value)
    {
        if (value.empty())
        {
            return {};
        }

        std::string extension(value);
        if (extension.front() != '.')
        {
            extension.insert(extension.begin(), '.');
        }

        return toLower(extension);
    }

    bool hasWildcard(std::string_view value)
    {
        return value.find_first_of("*?") != std::string_view::npos;
    }

    void addInputFilename(ImageEncoderArgs& args, std::string_view token)
    {
        if (!hasWildcard(token))
        {
            if (isImageDecoder(std::string(token)))
            {
                args.filenames.emplace_back(token);
            }
            return;
        }

        // Windows (and some shells) do not expand globs into argv; do it here.
        std::string directory(getPath(token));
        if (directory.empty())
        {
            directory = "./";
        }

        Path path(directory);
        const FileIndex& index = path;
        auto filenames = index.matchFilenames(removePath(token));

        size_t matches = 0;

        for (const std::string& name : filenames)
        {
            if (!isImageDecoder(name))
            {
                continue;
            }

            args.filenames.emplace_back(directory + name);
            ++matches;
        }

        if (!matches)
        {
            printLine("No files matched: \"{}\"", token);
        }
    }

    void configureParser(CommandLineParser& parser, ImageEncoderArgs& args)
    {
        args.options.compression = 8;

        parser.usage("[options] <inputs...>");

        parser.option("--format", "output format (png, .png, ...)",
            [&](std::string_view value)
            {
                const std::string extension = normalizeFormatExtension(value);

                if (!isImageEncoder(extension))
                {
                    args.error = fmt::format("Unsupported output format: {}", value);
                    return;
                }

                printLine("Active output format: {}", extension);
                args.request_format = extension;
            });

        parser.option("--output", "output filename (single input only)",
            [&](std::string_view value)
            {
                args.output_filename = value;
            });

        parser.option("--compression", "compression level (0..10)",
            [&](std::string_view value)
            {
                args.options.compression = std::atoi(value.data());
            });

        parser.option("--quality", "quality level (0..100)",
            [&](std::string_view value)
            {
                args.options.quality = std::atoi(value.data()) / 100.0f;
            });

        parser.option("--astc", "ASTC block size (e.g. 4x4)",
            [&](std::string_view value)
            {
                int width = 0;
                int height = 0;

                if (!parseAstcBlockSize(value, width, height))
                {
                    args.error = fmt::format("Invalid ASTC block size: {}", value);
                    return;
                }

                args.options.astc_block_width = width;
                args.options.astc_block_height = height;
            });

        parser.flag("--luminance", "encode as luminance",
            [&]()
            {
                args.luminance = true;
            });

        parser.flag("--lossless", "lossless encoding",
            [&]()
            {
                args.lossless = true;
            });

        parser.flag("--linear", "treat input as sRGB and convert to linear",
            [&]()
            {
                args.linear = true;
            });

        parser.flag("--info", "enable decoder/encoder diagnostic output",
            [&]()
            {
                printEnable(Print::Debug, true);
            });

        parser.positional([&](std::string_view token)
        {
            addInputFilename(args, token);
        });
    }

    void runEncoder(const ImageEncoderArgs& args)
    {
        ConcurrentQueue q;

        for (const auto& filename : args.filenames)
        {
            std::string output(removePath(filename));

            if (!args.output_filename.empty() && args.filenames.size() == 1)
            {
                output = args.output_filename;
            }
            else if (!args.request_format.empty())
            {
                output = std::string(removeExtension(output)) + args.request_format;
            }

            printLine("Processing: \"{}\" --> \"{}\"", filename, output);

            q.enqueue([=]
            {
                std::unique_ptr<Bitmap> bitmap;
                ImageEncodeOptions encode_options = args.options;
                encode_options.lossless = args.lossless;

                if (args.luminance)
                {
                    Bitmap temp(filename);
                    bitmap = std::make_unique<LuminanceBitmap>(temp, false, args.linear);
                }
                else
                {
                    bitmap = std::make_unique<Bitmap>(filename);

                    if (args.linear && !bitmap->format.isLinear())
                    {
                        srgbToLinear(*bitmap);
                    }
                }

                bitmap->save(output, encode_options);
            });
        }

        q.wait();
    }

} // namespace

int main(int argc, const char* argv[])
{
    ImageEncoderArgs args;

    CommandLineParser parser;
    configureParser(parser, args);

    if (argc < 2)
    {
        parser.printHelp();
        return 0;
    }

    if (!parser.parse(argc, argv))
    {
        return 1;
    }

    if (!args.error.empty())
    {
        printLine("{}", args.error);
        return 1;
    }

    runEncoder(args);
    return 0;
}
