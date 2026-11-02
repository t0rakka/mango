/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>
#include <algorithm>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

namespace
{

    // Floyd-Steinberg is on by default for low palette counts unless --no-dithering.
    constexpr int quantize_dither_default_max_colors = 32;

    struct ImageEncoderArgs
    {
        ImageEncodeOptions options;
        std::string output_filename;
        std::string request_format;
        std::vector<std::string> filenames;

        bool luminance = false;
        bool linear = false;
        bool lossless = false;

        bool list_encoders = false;
        std::string probe_extension;

        int quantize_colors = 0;
        bool dither = false;
        bool dither_set = false;

        std::string error;
    };

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

    const char* yesNo(bool value)
    {
        return value ? "yes" : "no";
    }

    void printEncodeCapabilities(const std::string& extension)
    {
        const ImageEncodeCapabilities caps = getImageEncodeCapabilities(extension);

        printLine("extension: {}", extension);
        printLine("  encoder: {}", yesNo(caps.encoder));

        if (!caps.encoder)
            return;

        printLine("  indexed: {}", yesNo(caps.indexed()));
        printLine("  indexed_auto_quantize: {}", yesNo(caps.indexedAutoQuantize()));
        printLine("  float: {}", yesNo(caps.floatFormat()));
        printLine("  unorm16: {}", yesNo(caps.unorm16()));
        printLine("  luminance: {}", yesNo(caps.luminance()));
        printLine("  alpha: {}", yesNo(caps.alpha()));
        printLine("  linear: {}", yesNo(caps.linear()));
        printLine("  lossless: {}", yesNo(caps.lossless()));
        printLine("  lossy: {}", yesNo(caps.lossy()));
    }

    void listEncoders()
    {
        std::vector<std::string> extensions = listImageEncoderExtensions();
        std::sort(extensions.begin(), extensions.end());

        printLine("{:<8}  {:^3} {:^3} {:^3} {:^3} {:^3} {:^3} {:^3} {:^3} {:^3}",
            "ext", "idx", "aut", "flt", "u16", "lum", "alp", "lin", "ls", "lsy");
        printLine("{:-<8}  {:-^3} {:-^3} {:-^3} {:-^3} {:-^3} {:-^3} {:-^3} {:-^3} {:-^3}",
            "", "", "", "", "", "", "", "", "", "");

        for (const std::string& extension : extensions)
        {
            const ImageEncodeCapabilities caps = getImageEncodeCapabilities(extension);
            printLine("{:<8}  {:^3} {:^3} {:^3} {:^3} {:^3} {:^3} {:^3} {:^3} {:^3}",
                extension,
                yesNo(caps.indexed()),
                yesNo(caps.indexedAutoQuantize()),
                yesNo(caps.floatFormat()),
                yesNo(caps.unorm16()),
                yesNo(caps.luminance()),
                yesNo(caps.alpha()),
                yesNo(caps.linear()),
                yesNo(caps.lossless()),
                yesNo(caps.lossy()));
        }
    }

    void configureParser(const CommandLine& commands, ImageEncoderArgs& args)
    {
        args.options.compression = 8;

        commands.usage("[options] <inputs...>");

        commands.option("--format", "output format (png, .png, ...)",
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

        commands.option("--output", "output filename (single input only)",
            [&](std::string_view value)
            {
                args.output_filename = value;
            });

        commands.optionInt("--compression", "compression level (0..10)",
            [&](int value)
            {
                args.options.compression = value;
            });

        commands.optionInt("--quality", "quality level (0..100)",
            [&](int value)
            {
                args.options.quality = value / 100.0f;
            });

        commands.option2D("--astc", "ASTC block size (e.g. 4x4)",
            [&](int width, int height)
            {
                args.options.astc_block_width = width;
                args.options.astc_block_height = height;
            });

        commands.flag("--luminance", "encode as luminance",
            [&]()
            {
                args.luminance = true;
            });

        commands.flag("--lossless", "lossless encoding",
            [&]()
            {
                args.lossless = true;
            });

        commands.flag("--linear", "treat input as sRGB and convert to linear",
            [&]()
            {
                args.linear = true;
            });

        commands.optionInt("--quantize", "quantize to N palette colors (1..256) before encode; "
            "dithering defaults on for N<=" + std::to_string(quantize_dither_default_max_colors),
            [&](int value)
            {
                args.quantize_colors = std::max(1, std::min(value, 256));
            });

        commands.flag("--dithering", "force Floyd-Steinberg dithering during --quantize",
            [&]()
            {
                args.dither = true;
                args.dither_set = true;
            });

        commands.flag("--no-dithering", "disable dithering (overrides low-palette default)",
            [&]()
            {
                args.dither = false;
                args.dither_set = true;
            });

        commands.flag("--info", "enable decoder/encoder diagnostic output",
            [&]()
            {
                printEnable(Print::Debug, true);
            });

        commands.flag("--list-encoders", "list registered output formats and indexed encode support",
            [&]()
            {
                args.list_encoders = true;
            });

        commands.option("--probe", "print encode capabilities for a format (e.g. png or .png)",
            [&](std::string_view value)
            {
                args.probe_extension = normalizeFormatExtension(value);
            });

        commands.positional([&](std::string_view token)
        {
            addInputFilename(args, token);
        });
    }

    bool resolveQuantizeDithering(const ImageEncoderArgs& args)
    {
        if (args.dither_set)
        {
            return args.dither;
        }

        return args.quantize_colors > 0 &&
               args.quantize_colors <= quantize_dither_default_max_colors;
    }

    ImageEncodeStatus saveWithQuantize(Bitmap& bitmap, const std::string& output, const ImageEncoderArgs& args)
    {
        ImageEncodeOptions encode_options = args.options;
        encode_options.lossless = args.lossless;

        const std::string extension = normalizeFormatExtension(getExtension(output));

        if (args.quantize_colors > 0)
        {
            const bool dither = resolveQuantizeDithering(args);
            encode_options.dithering = dither;

            const ImageEncodeCapabilities caps = getImageEncodeCapabilities(extension);

            if (!caps.indexed() && !caps.indexedAutoQuantize())
            {
                ImageEncodeStatus status;
                status.setError("[image_encoder] Format \"{}\" does not support indexed encoding.", extension);
                return status;
            }

            if (caps.indexed())
            {
                IndexedEncodeTarget target;

                std::unique_ptr<Bitmap> indexed = quantizeToIndexedBitmap(
                    bitmap, args.quantize_colors, dither, encode_options.quality, extension, &target);

                printLine("  quantize: {} colors -> table {} ({}-bit indices), dithering: {}",
                    target.color_count, target.table_size, target.index_bit_depth,
                    dither ? "on" : "off");

                return indexed->save(output, encode_options);
            }

            // indexed_auto (GIF): tool-side quantize for explicit color count / dither control.
            std::unique_ptr<Bitmap> indexed = quantizeToIndexedBitmap(
                bitmap, args.quantize_colors, dither, encode_options.quality, extension);

            printLine("  quantize: {} colors, dithering: {}", args.quantize_colors, dither ? "on" : "off");

            return indexed->save(output, encode_options);
        }

        encode_options.dithering = args.dither;

        return bitmap.save(output, encode_options);
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

                ImageEncodeStatus status = saveWithQuantize(*bitmap, output, args);

                if (!status)
                {
                    printLine(Print::Warning, "{}", status.info);
                }
            });
        }

        q.wait();
    }

} // namespace

int main(int argc, const char* argv[])
{
    CommandLine commands(argc, argv);
    ImageEncoderArgs args;

    configureParser(commands, args);

    if (argc < 2)
    {
        commands.printHelp();
        return 0;
    }

    if (!commands.parse())
    {
        return 1;
    }

    if (!args.error.empty())
    {
        printLine("{}", args.error);
        return 1;
    }

    if (args.list_encoders)
    {
        listEncoders();
        return 0;
    }

    if (!args.probe_extension.empty())
    {
        printEncodeCapabilities(args.probe_extension);
        return 0;
    }

    runEncoder(args);
    return 0;
}
