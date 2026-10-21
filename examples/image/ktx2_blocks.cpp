/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstdio>
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::image;

namespace
{

    std::string formatSizeKb(size_t bytes)
    {
        char text[32];
        if (bytes < 1024)
        {
            std::snprintf(text, sizeof(text), "%zu B", bytes);
        }
        else
        {
            std::snprintf(text, sizeof(text), "%.1f KB", double(bytes) / 1024.0);
        }

        return text;
    }

    struct FormatEntry
    {
        const char* cli_name;
        u32 compression;
        u32 supercompress_flag;
        const char* basisu_name;
        const char* vulkan_note;
        bool recommended_desktop; // good default for Win / Linux / MoltenVK test matrix
    };

    static constexpr FormatEntry g_basis_transcode_targets[] =
    {
        { "bc1",      TextureCompression::BC1_UNORM,                   SUPERCOMPRESS_BC1_UNORM,        "BC1 / DXT1",            "VK_FORMAT_BC1_RGBA_UNORM_BLOCK", true },
        { "bc3",      TextureCompression::BC3_UNORM,                   SUPERCOMPRESS_BC3_UNORM,        "BC3 / DXT5",            "VK_FORMAT_BC3_UNORM_BLOCK", true },
        { "bc4",      TextureCompression::BC4_UNORM,                   SUPERCOMPRESS_BC4_UNORM,        "BC4 R",                 "VK_FORMAT_BC4_UNORM_BLOCK", true },
        { "bc5",      TextureCompression::BC5_UNORM,                   SUPERCOMPRESS_BC5_UNORM,        "BC5 RG",                "VK_FORMAT_BC5_UNORM_BLOCK", true },
        { "bc7",      TextureCompression::BC7_UNORM,                   SUPERCOMPRESS_BC7_UNORM,        "BC7 RGBA",              "VK_FORMAT_BC7_UNORM_BLOCK", true },
        { "etc1",     TextureCompression::ETC1_RGB,                    SUPERCOMPRESS_ETC1_RGB,         "ETC1 RGB",              "ETC2 block; often missing on MoltenVK", false },
        { "etc2",     TextureCompression::ETC2_RGBA,                   SUPERCOMPRESS_ETC2_RGBA,        "ETC2 RGBA",             "often missing on MoltenVK", false },
        { "astc4x4",  TextureCompression::ASTC_UNORM_4x4,              SUPERCOMPRESS_ASTC_RGBA_4x4,    "ASTC 4x4 LDR",          "VK_EXT_texture_compression_astc / Metal", false },
        { "eac_r11",  TextureCompression::EAC_R11,                     SUPERCOMPRESS_EAC_R11,          "ETC2 EAC R11",          "GLES-oriented; spotty on desktop", false },
        { "eac_rg11", TextureCompression::EAC_RG11,                    SUPERCOMPRESS_EAC_RG11,         "ETC2 EAC RG11",         "GLES-oriented; spotty on desktop", false },
        { "pvrtc_rgb",  TextureCompression::PVRTC_RGB_4BPP,            SUPERCOMPRESS_PVRTC_RGB_4BPP,   "PVRTC1 4bpp RGB",       "VK_IMG_format_pvrtc (Apple)", false },
        { "pvrtc_rgba", TextureCompression::PVRTC_RGBA_4BPP,           SUPERCOMPRESS_PVRTC_RGBA_4BPP,  "PVRTC1 4bpp RGBA",      "VK_IMG_format_pvrtc (Apple)", false },
        { "pvrtc2_rgba", TextureCompression::PVRTC2_RGBA_4BPP,         SUPERCOMPRESS_PVRTC2_RGBA_4BPP, "PVRTC2 4bpp RGBA",      "VK_IMG_format_pvrtc", false },
        { "atc_rgb",  TextureCompression::ATC_RGB,                     SUPERCOMPRESS_ATC_RGB,          "ATC RGB",               "legacy mobile; rare in Vulkan", false },
        { "atc_rgba", TextureCompression::ATC_RGBA_INTERPOLATED_ALPHA, SUPERCOMPRESS_ATC_RGBA,         "ATC RGBA",              "legacy mobile; rare in Vulkan", false },
        { "fxt1",     TextureCompression::FXT1_RGB,                    SUPERCOMPRESS_FXT1_RGB,         "FXT1 RGB",              "legacy; rarely in Vulkan", false },
        { "bc6h",     TextureCompression::BC6H_UF16,                   SUPERCOMPRESS_BC6H_UF16,        "BC6H UF16 HDR",         "VK_FORMAT_BC6H_UFLOAT_BLOCK", true },
        { "astc_hdr4", TextureCompression::ASTC_FLOAT_4x4,           SUPERCOMPRESS_ASTC_HDR_4x4,     "ASTC 4x4 HDR",          "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK", false },
        { "astc_hdr6", TextureCompression::ASTC_FLOAT_6x6,           SUPERCOMPRESS_ASTC_HDR_6x6,     "ASTC 6x6 HDR",          "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK", false },
    };

    void printCatalog()
    {
        printLine("OSS Basis Universal transcode targets (ImageDecoder::memory + options.compression):");
        printLine("");
        printLine("  [desktop]  = typical Win / Linux / MoltenVK smoke-test set");
        printLine("");
        for (const FormatEntry& e : g_basis_transcode_targets)
        {
            printLine("  {:<12}{}  super=0x{:04x}  {}", e.cli_name,
                e.recommended_desktop ? " [desktop]" : "          ",
                e.supercompress_flag, e.basisu_name);
            printLine("              {}", e.vulkan_note);
        }
        printLine("");
        printLine("KTX2 native upload (options.compression = NONE):");
        printLine("  Stored vkFormat blocks (BC*, BC6H HDR, ASTC, …) — no Basis transcode.");
        printLine("  Basis UASTC LDR payload is also exposed as native (ASTC 4x4 blocks).");
        printLine("  UASTC HDR (FastHDR): memory(NONE) → native ASTC HDR; bc6h / astc_hdr* transcode targets.");
        printLine("");
        printLine("Minimum test files to generate (covers most paths):");
        printLine("  1. basis_etc1s_rgba.ktx2     — BasisLZ supercompression");
        printLine("  2. uastc.ktx2                 — UASTC DFD (Zstd optional second file)");
        printLine("  3. native_bc7.ktx2            — vkFormat BC7, scheme none");
        printLine("  4. native_bc6h.ktx2           — HDR BC6H (GPU path, no fp16 decode)");
        printLine("  5. uastc_hdr.ktx2             — DFD 167 / FastHDR (BasisLZ + ASTC HDR or BC6H transcode)");
        printLine("  6. mips_basis.ktx2            — ETC1S or UASTC, 3+ mip levels");
        printLine("");
        printLine("Run:  ktx2_blocks --all your.ktx2   (failures are OK — diagnostics only)");
    }

    const FormatEntry* findFormat(const std::string& name)
    {
        for (const FormatEntry& e : g_basis_transcode_targets)
        {
            if (name == e.cli_name)
            {
                return &e;
            }
        }
        if (name == "native")
        {
            return nullptr;
        }
        return nullptr;
    }

    size_t expectedBlockBytes(int width, int height, u32 compression)
    {
        if (compression == TextureCompression::NONE)
        {
            return 0;
        }

        TextureCompression info(compression);
        const int bw = std::max(1, info.width);
        const int bh = std::max(1, info.height);
        const int xblocks = (width + bw - 1) / bw;
        const int yblocks = (height + bh - 1) / bh;
        return size_t(xblocks) * size_t(yblocks) * info.bytes;
    }

    void printHeaderSummary(const ImageHeader& h)
    {
        printLine("  dimensions: {} x {} (levels {}, faces {})", h.width, h.height, h.levels, h.faces);
        printLine("  supercompression mask: 0x{:x}", h.supercompression);
        if (h.supercompression & SUPERCOMPRESS_BASISU_ETC1S)
            printLine("  source: Basis ETC1S");
        if (h.supercompression & SUPERCOMPRESS_BASISU_UASTC)
            printLine("  source: Basis UASTC LDR");
        if (h.supercompression & SUPERCOMPRESS_BASISU_UASTC_HDR)
            printLine("  source: Basis UASTC HDR");
        printLine("  file compression (native): {}", h.compression);
        printLine("  decode format: {}-bit {}", h.format.bits, h.format.isFloat() ? "float" : "integer");
    }

    void probeMemory(ImageDecoder& decoder, const ImageHeader& h, u32 compression,
        const char* label, const FormatEntry* entry)
    {
        if (entry && compression != TextureCompression::NONE &&
            (h.supercompression & entry->supercompress_flag) == 0)
        {
            printLine("  [{}] not in file mask (decoder does not offer this transcode for this KTX2)", label);
            return;
        }

        ImageDecodeOptions options;
        options.compression = compression;

        ConstMemory mem = decoder.memory(0, 0, 0, options);
        const u32 out_compression = decoder.header().compression;

        if (!mem.address || !mem.size)
        {
            printLine("  [{}] transcode/extract failed (basisu rejected target, bad slice, or unsupported for this source)", label);
            if (entry && !entry->recommended_desktop)
            {
                printLine("              note: {} — often unsupported on MoltenVK / desktop GPU", entry->vulkan_note);
            }
            return;
        }

        const size_t expected = expectedBlockBytes(h.width, h.height, out_compression);
        std::string size_note;
        if (expected && mem.size != expected)
        {
            size_note = " (expected " + formatSizeKb(expected) + ")";
        }

        printLine("  [{}] OK — {}, TextureCompression={}{}",
            label, formatSizeKb(mem.size), out_compression, size_note);

        if (entry && !entry->recommended_desktop)
        {
            printLine("              (decode succeeded; GPU may still lack Vulkan format — check device features)");
        }
    }

    bool hasNativeGpuBlob(const ImageHeader& h)
    {
        if (h.compression != TextureCompression::NONE)
        {
            return true;
        }

        return (h.supercompression & (SUPERCOMPRESS_BASISU_UASTC | SUPERCOMPRESS_BASISU_UASTC_HDR)) != 0;
    }

} // namespace

int main(int argc, const char* argv[])
{
    CommandLine commands(argc, argv);

    std::string filename;
    std::string format = "bc7";
    bool list_catalog = false;
    bool probe_all = false;
    bool desktop_only = false;

    commands.usage("[options] <file.ktx2>");
    commands.flag("--list-formats", "print format catalog and suggested test files", [&]() { list_catalog = true; });
    commands.flag("--all", "probe native + every OSS Basis transcode target", [&]() { probe_all = true; });
    commands.flag("--desktop", "with --all, only probe [desktop] BC targets + native", [&]() { desktop_only = true; });
    commands.option("--format", "native | bc7 | bc3 | … (see --list-formats; default bc7)", [&](std::string_view value)
    {
        format = std::string(value);
    });
    commands.positional([&](std::string_view token)
    {
        filename = token;
    });

    if (!commands.parse())
    {
        return 1;
    }

    if (list_catalog)
    {
        printCatalog();
        if (filename.empty())
        {
            return 0;
        }
    }

    if (filename.empty())
    {
        commands.printHelp();
        return 1;
    }

    filesystem::File file(filename);
    ImageDecoder decoder(file, filename);
    if (!decoder.isDecoder())
    {
        printLine(Print::Error, "Not a KTX2 decoder input.");
        return 1;
    }

    ImageHeader header = decoder.header();
    if (!header.success)
    {
        printLine(Print::Error, "{}", header.info);
        return 1;
    }

    printLine("{}", filename);
    printHeaderSummary(header);

    if (probe_all)
    {
        if (hasNativeGpuBlob(header))
        {
            printLine("");
            printLine("Native GPU blob (compression = NONE):");
            probeMemory(decoder, header, TextureCompression::NONE, "native", nullptr);
        }

        printLine("");
        printLine("Basis transcode targets:");
        for (const FormatEntry& e : g_basis_transcode_targets)
        {
            if (desktop_only && !e.recommended_desktop)
            {
                continue;
            }

            probeMemory(decoder, header, e.compression, e.cli_name, &e);
        }

        return 0;
    }

    printLine("");
    if (format == "native")
    {
        if (!hasNativeGpuBlob(header))
        {
            printLine("  [native] no native block payload (Basis ETC1S needs --format bc7, etc.)");
            return 0;
        }

        probeMemory(decoder, header, TextureCompression::NONE, "native", nullptr);
        return 0;
    }

    const FormatEntry* entry = findFormat(format);
    if (!entry)
    {
        printLine(Print::Error, "Unknown --format \"{}\". Use --list-formats.", format);
        return 1;
    }

    probeMemory(decoder, header, entry->compression, entry->cli_name, entry);
    return 0;
}
