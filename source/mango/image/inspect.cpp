/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cmath>
#include <map>
#include <sstream>
#include <mango/core/string.hpp>
#include <mango/filesystem/filesystem.hpp>
#include <mango/image/image.hpp>

namespace mango::image
{

    namespace
    {
        constexpr float kWhiteTolerance = 0.004f;

        bool isNear(float a, float b, float tolerance = kWhiteTolerance)
        {
            return std::fabs(a - b) < tolerance;
        }

        ColorPoint whitePointForPrimaries(ColorPrimaries primaries)
        {
            switch (primaries)
            {
                case ColorPrimaries::DCI_P3:
                    return { 0.314f, 0.351f };
                default:
                    return { 0.3127f, 0.3290f };
            }
        }

        int formatBitDepth(const Format& format)
        {
            if (format.isIndexed())
                return 8;

            const int rgb = std::max({ format.size.r, format.size.g, format.size.b });
            if (rgb > 0)
                return rgb;

            return format.bits;
        }

        u64 estimateDecodeBytes(const ImageHeader& header)
        {
            if (!header.success || header.width <= 0 || header.height <= 0)
                return 0;

            const int bytes = header.format.bytes();
            if (bytes <= 0)
                return 0;

            const int frames = header.frames > 0 ? header.frames : 1;
            const int levels = header.levels > 0 ? header.levels : 1;
            const int depth  = header.depth > 0 ? header.depth : 1;
            const int faces  = header.faces > 0 ? header.faces : 1;

            return u64(header.width) * u64(header.height) * u64(bytes) * u64(frames) * u64(levels) * u64(depth) * u64(faces);
        }

        void populateColorStrings(ImageInspect& report, const ColorInfo& color, const Format& format)
        {
            ColorPrimaries primaries = color.primaries;
            if (color.has_chromaticities && color.white.y > 1e-6f)
            {
                const ColorPrimaries identified = identifyPrimaries(color.white, color.red, color.green, color.blue);
                if (identified != ColorPrimaries::Unspecified)
                    primaries = identified;
            }

            if (primaries != ColorPrimaries::Unspecified)
                report.primaries = toString(primaries);
            else if (color.has_chromaticities)
                report.primaries = "custom";
            else if (format.isIndexed() || format.isLuminance())
                report.primaries = "grayscale";
            else
                report.primaries = "unspecified";

            report.transfer = formatTransferFunction(color);
            report.white_point = identifyWhitePoint(color);
        }

        void populateUniversal(ImageInspect& report, const ImageDecodeInterface& interface, ConstMemory memory)
        {
            report.header = interface.header;
            if (!report.header.success)
            {
                report.setError(report.header.info);
                return;
            }

            report.exif = interface.exif.size > 0;
            report.icc = interface.icc.size > 0;
            report.xmp = detectXmp(memory);

            report.bit_depth = formatBitDepth(report.header.format);
            report.alpha = report.header.alpha;
            report.decode_bytes = estimateDecodeBytes(report.header);

            populateColorStrings(report, report.header.color, report.header.format);
        }

        std::string formatBytes(u64 bytes)
        {
            if (bytes < 1024)
                return fmt::format("{} B", bytes);

            const double kb = double(bytes) / 1024.0;
            if (kb < 1024.0)
                return fmt::format("{:.0f} KB", kb);

            const double mb = kb / 1024.0;
            if (mb < 1024.0)
                return fmt::format("{:.0f} MB", mb);

            const double gb = mb / 1024.0;
            return fmt::format("{:.2f} GB", gb);
        }

        std::string formatTiling(const ImageTileInfo& tiling)
        {
            switch (tiling.tiled)
            {
                case InspectTriState::Yes:
                    if (tiling.width > 0 && tiling.height > 0)
                        return fmt::format("yes ({} x {})", tiling.width, tiling.height);
                    return "yes";
                case InspectTriState::No:
                    return "no";
                default:
                    return "unknown";
            }
        }

    } // namespace

    void populateImageInspect(ImageInspect& report, const ImageDecodeInterface& interface, ConstMemory memory)
    {
        populateUniversal(report, interface, memory);
    }

    const char* toString(ColorPrimaries primaries) noexcept
    {
        switch (primaries)
        {
            case ColorPrimaries::BT709:     return "BT.709 / sRGB";
            case ColorPrimaries::BT470M:    return "BT.470 System M";
            case ColorPrimaries::BT601_625: return "BT.601 625-line";
            case ColorPrimaries::BT601_525: return "BT.601 525-line";
            case ColorPrimaries::BT2020:    return "BT.2020";
            case ColorPrimaries::SMPTE428:  return "CIE XYZ (ST 428)";
            case ColorPrimaries::DCI_P3:    return "DCI-P3";
            case ColorPrimaries::DisplayP3: return "Display P3";
            case ColorPrimaries::AdobeRGB:  return "Adobe RGB";
            case ColorPrimaries::ACES_AP0:  return "ACES AP0";
            case ColorPrimaries::ACES_AP1:  return "ACES AP1";
            default:                        return "unspecified";
        }
    }

    const char* toString(TransferFunction transfer) noexcept
    {
        switch (transfer)
        {
            case TransferFunction::BT709:   return "BT.709";
            case TransferFunction::Gamma22: return "Gamma 2.2";
            case TransferFunction::Gamma28: return "Gamma 2.8";
            case TransferFunction::Linear:  return "Linear";
            case TransferFunction::sRGB:    return "sRGB";
            case TransferFunction::PQ:      return "PQ";
            case TransferFunction::HLG:     return "HLG";
            default:                        return "unspecified";
        }
    }

    const char* toString(InspectTriState state) noexcept
    {
        switch (state)
        {
            case InspectTriState::Yes: return "yes";
            case InspectTriState::No:  return "no";
            default:                   return "unknown";
        }
    }

    std::string formatDisplayName(const std::string& extension)
    {
        static const std::map<std::string, std::string> names =
        {
            { ".jpg", "JPEG" }, { ".jpeg", "JPEG" }, { ".jfif", "JPEG" }, { ".jpe", "JPEG" },
            { ".jps", "JPEG Stereo" }, { ".mpo", "JPEG Multi-Picture" },
            { ".png", "PNG" },
            { ".jxl", "JPEG XL" },
            { ".jp2", "JPEG 2000" },
            { ".j2k", "JPEG 2000" },
            { ".webp", "WebP" },
            { ".avif", "AVIF" },
            { ".heic", "HEIF" }, { ".heif", "HEIF" },
            { ".gif", "GIF" },
            { ".bmp", "BMP" },
            { ".tiff", "TIFF" }, { ".tif", "TIFF" },
            { ".exr", "OpenEXR" },
            { ".hdr", "Radiance HDR" },
            { ".dds", "DirectDraw Surface" },
            { ".ktx", "KTX" }, { ".ktx2", "KTX2" },
            { ".astc", "ASTC" },
            { ".qoi", "QOI" },
            { ".psd", "Photoshop" }, { ".psb", "Photoshop Large" },
            { ".tga", "TGA" },
        };

        const std::string key = toLower(extension);
        const auto it = names.find(key);
        if (it != names.end())
            return it->second;

        if (!extension.empty())
            return toUpper(extension.substr(1));

        return "Unknown";
    }

    std::string formatTransferFunction(const ColorInfo& color) noexcept
    {
        if (color.gamma > 0.0f)
            return fmt::format("Gamma {:.3g}", color.gamma);

        return toString(color.transfer);
    }

    std::string identifyWhitePoint(const ColorInfo& color) noexcept
    {
        ColorPoint white = color.white;
        if (!(color.has_chromaticities && white.y > 1e-6f))
            white = whitePointForPrimaries(color.primaries);

        struct Reference
        {
            const char* name;
            float x;
            float y;
        };

        static const Reference references [] =
        {
            { "D65", 0.3127f, 0.3290f },
            { "D50", 0.34567f, 0.35850f },
            { "DCI", 0.314f, 0.351f },
            { "E", 1.0f / 3.0f, 1.0f / 3.0f },
        };

        for (const Reference& ref : references)
        {
            if (isNear(white.x, ref.x) && isNear(white.y, ref.y))
                return ref.name;
        }

        if (white.y > 1e-6f)
            return fmt::format("{:.4f}, {:.4f}", white.x, white.y);

        return "unspecified";
    }

    bool detectXmp(ConstMemory memory) noexcept
    {
        if (!memory.address || memory.size < 16)
            return false;

        static const char* patterns [] =
        {
            "http://ns.adobe.com/xap/1.0/",
            "<?xpacket begin",
            "adobe:ns:meta/",
            "x:xmpmeta",
        };

        const char* begin = reinterpret_cast<const char*>(memory.address);
        const char* end = begin + memory.size;

        for (const char* pattern : patterns)
        {
            const size_t length = std::strlen(pattern);
            if (length > memory.size)
                continue;

            for (const char* p = begin; p + length <= end; ++p)
            {
                if (std::memcmp(p, pattern, length) == 0)
                    return true;
            }
        }

        return false;
    }

    std::string formatImageInspect(const ImageInspect& report)
    {
        if (!report.success)
            return report.info;

        std::ostringstream out;

        out << "Format: " << report.format << "\n";
        out << "Dimensions: " << report.header.width << " x " << report.header.height;

        if (report.header.depth > 1)
            out << " x " << report.header.depth;
        out << "\n";

        if (report.header.frames > 0)
            out << "Frames: " << report.header.frames << "\n";
        if (report.header.levels > 1)
            out << "Mipmap levels: " << report.header.levels << "\n";
        if (report.header.faces == 6)
            out << "Cubemap faces: 6\n";

        out << "\nColor:\n";
        out << "  Primaries: " << report.primaries << "\n";
        out << "  Transfer: " << report.transfer << "\n";
        out << "  White point: " << report.white_point << "\n";
        out << "  Bit depth: " << report.bit_depth << "\n";

        out << "\nCompression:\n";
        out << "  Lossless: " << toString(report.lossless) << "\n";
        if (!report.chroma_subsampling.empty())
            out << "  Chroma: " << report.chroma_subsampling << "\n";
        if (!report.encoding.empty())
            out << "  Encoding: " << report.encoding << "\n";

        out << "\nMetadata:\n";
        out << "  EXIF: " << (report.exif ? "yes" : "no") << "\n";
        out << "  ICC: " << (report.icc ? "yes" : "no") << "\n";
        out << "  XMP: " << (report.xmp ? "yes" : "no") << "\n";

        out << "\nMemory:\n";
        out << "  Full decode: " << formatBytes(report.decode_bytes) << "\n";

        out << "\nFeatures:\n";
        out << "  Progressive: " << toString(report.progressive) << "\n";
        out << "  Tiled: " << formatTiling(report.tiling) << "\n";
        out << "  Alpha: " << (report.alpha ? "yes" : "no") << "\n";

        return out.str();
    }

    ImageInspect inspect(ConstMemory memory, const std::string& filename)
    {
        ImageInspect report;

        report.format_extension = detectImageFormatExtension(memory);
        if (report.format_extension.empty() && !filename.empty())
            report.format_extension = toLower(filesystem::getExtension(filename));

        report.format = formatDisplayName(report.format_extension);

        ImageDecoder decoder(memory, filename);
        if (!decoder.isDecoder())
        {
            report.setError("[ImageInspect] Unsupported or unrecognized image format.");
            return report;
        }

        ImageInspect details = decoder.inspect(memory);
        if (!details.success)
            return details;

        details.format = report.format;
        details.format_extension = report.format_extension;
        return details;
    }

} // namespace mango::image
