/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/image/decoder.hpp>

namespace mango::image
{

    // Three-valued flag for properties that are not universal across codecs.
    enum class InspectTriState : u8
    {
        No,
        Yes,
        Unknown,
    };

    struct ImageTileInfo
    {
        InspectTriState tiled = InspectTriState::Unknown;
        int width = 0;
        int height = 0;
    };

    /*
        Structured summary of an image file without decoding pixels.

        Universal fields are filled for every supported format. Codec-specific
        fields (lossless, progressive, tiling, chroma subsampling) remain
        InspectTriState::Unknown until a decoder provides them.
    */
    struct ImageInspect : Status
    {
        std::string format;            // e.g. "JPEG XL"
        std::string format_extension;  // e.g. ".jxl"

        ImageHeader header;

        // Color (human-readable)
        std::string primaries;
        std::string transfer;
        std::string white_point;

        int bit_depth = 0;
        bool alpha = false;

        // Compression / encoding
        InspectTriState lossless = InspectTriState::Unknown;
        std::string chroma_subsampling;
        std::string encoding;          // codec-specific label, e.g. "Progressive DCT"

        // Metadata presence
        bool exif = false;
        bool icc = false;
        bool xmp = false;

        // Derived decode footprint (header.format, single frame/level/face)
        u64 decode_bytes = 0;

        // Features
        InspectTriState progressive = InspectTriState::Unknown;
        ImageTileInfo tiling;
    };

    // Recognize container format from file signature (empty when unknown).
    std::string detectImageFormatExtension(ConstMemory memory);

    const char* toString(ColorPrimaries primaries) noexcept;
    const char* toString(TransferFunction transfer) noexcept;
    const char* toString(InspectTriState state) noexcept;

    std::string formatDisplayName(const std::string& extension);
    std::string formatTransferFunction(const ColorInfo& color) noexcept;
    std::string identifyWhitePoint(const ColorInfo& color) noexcept;

    bool detectXmp(ConstMemory memory) noexcept;

    // Populate universal inspect fields from a parsed decoder interface.
    void populateImageInspect(ImageInspect& report, const ImageDecodeInterface& interface, ConstMemory memory);

    std::string formatImageInspect(const ImageInspect& report);

    ImageInspect inspect(ConstMemory memory, const std::string& filename = "");

} // namespace mango::image
