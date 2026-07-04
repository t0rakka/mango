/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>
#include <mango/core/compress.hpp>
#include <mango/core/cpuinfo.hpp>
#include <vector>
#include <numeric>
#include <memory>

#include "ccitt_fax_decode.hpp"
#include "../jpeg/jpeg.hpp"

using namespace mango;
using namespace mango::image;
using namespace mango::math;

namespace
{

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    // Specification:
    // https://www.itu.int/itudoc/itu-t/com16/tiff-fx/docs/tiff6.pdf

    const u32 StripHeightNoLimit = 0xffffffff;

    enum class ByteOrder : u16
    {
        TIFF_LITTLE_ENDIAN = 0x4949,  // "II"
        TIFF_BIG_ENDIAN    = 0x4D4D,  // "MM"
    };

    enum class Type : u16
    {
        BYTE  = 1,
        ASCII = 2,
        SHORT = 3,
        LONG  = 4,
        RATIONAL = 5,
        SBYTE = 6,
        UNDEFINED = 7,
        SSHORT = 8,
        SLONG = 9,
        SRATIONAL = 10,
        FLOAT = 11,
        DOUBLE = 12,
        LONG8 = 16,
        SLONG8 = 17,
        IFD8 = 18,
    };

    enum class SampleFormat : u16
    {
        UINT = 1,
        SINT = 2,
        FLOAT = 3,
        UNDEFINED = 4,
    };

    enum class Compression : u16
    {
        NONE = 1,
        CCITT_RLE = 2,    // CCITT Group 3 (T.4) 1D RLE
        CCITT_GROUP3 = 3, // CCITT Group 3 (T.4) 2D
        CCITT_GROUP4 = 4, // CCITT Group 4 (T.6)
        LZW = 5,
        JPEG_LEGACY = 6,
        JPEG_MODERN = 7,
        ZIP = 8,
        CCITT_RLE_W = 32771, // CCITT Group 3 (T.4) 1D RLE (word aligned)
        PIXARLOG = 32909,    // Pixar companded 11-bit log + zlib (HDR)
        PACKBITS = 32773,
        DEFLATE = 32946,
        SGILOG = 34676,      // SGI LogLuv / LogL adaptive RLE (HDR)
        SGILOG24 = 34677,    // SGI LogLuv 24-bit packed (HDR)
        WEBP = 50001,
    };

    enum class FillOrder : u16
    {
        MSB2LSB = 1,
        LSB2MSB = 2,
    };

    enum class PhotometricInterpretation
    {
        WHITE_IS_ZERO = 0,
        BLACK_IS_ZERO = 1,
        RGB = 2,
        PALETTE = 3,
        TRANSPARENCY_MASK = 4,
        SEPARATED = 5,
        YCBCR = 6,
        CIELAB = 8,
        ICCLAB = 9,
        ITULAB = 10,
        CFA = 32803,
        LOGL = 32844,    // CIE Log2(L) - SGI LogLuv grayscale (HDR)
        LOGLUV = 32845,  // CIE Log2(L) (u',v') - SGI LogLuv color (HDR)
        LINEAR_RAW = 34892,
    };

    struct IFDContext
    {
        u32 width = 0;
        u32 height = 0;
        u16 compression = 1;
        u16 photometric = 2;
        bool photometric_specified = false;
        u16 resolution_unit = 0;
        float x_position = 0;
        float y_position = 0;
        float x_resolution = 0;
        float y_resolution = 0;
        u32 group3_options = 0;
        u32 group4_options = 0;
        //float reference_black_white = 0;
        u32 orientation = 1; // default
        /* same conversion as exif orientation:
        1 =	Top-left            No rotation or mirroring.
        2 =	Top-right	        Mirrored horizontally.
        3 =	Bottom-right        Rotated 180 degrees.
        4 =	Bottom-left         Mirrored vertically.
        5 =	Left-top            Mirrored along the main diagonal.
        6 =	Right-top           Rotated 90 degrees clockwise.
        7 =	Right-bottom        Mirrored along the anti-diagonal.
        8 =	Left-bottom         Rotated 90 degrees counterclockwise.        
        */
        u32 planar_configuration = 1;
        u32 predictor = 1;
        u16 fill_order = 1;
        u32 page_number = 0;
        std::string page_name;

        std::vector<u64> sample_format;
        std::vector<float> s_min_sample_value;
        std::vector<float> s_max_sample_value;

        std::vector<u64> extra_samples;
        bool associated_alpha = false;
        size_t associated_alpha_index = 0;

        u32 ink_set = 1;
        std::string ink_names;
        u32 number_of_inks = 4;
        u32 dot_range = 2;

        // Chromaticity support
        float32x2 white_point;
        float32x2 red_primary;
        float32x2 green_primary;
        float32x2 blue_primary;

        // compression 6: JPEG_LEGACY
        u32 jpeg_proc = 0;
        u32 jpeg_interchange_format = 0;
        u32 jpeg_interchange_format_length = 0;
        u32 jpeg_restart_interval = 0;
        u32 jpeg_lossless_predictors = 0;
        u32 jpeg_point_transforms = 0;
        std::vector<u64> jpeg_qt_tables;
        std::vector<u64> jpeg_dc_tables;
        std::vector<u64> jpeg_ac_tables;
        std::vector<u64> y_cb_cr_sub_sampling;
        float y_cb_cr_coefficients[3] = { 0.299f, 0.587f, 0.114f };
        float reference_black_white[6] = { 0.f, 255.f, 128.f, 255.f, 128.f, 255.f };

        // compression 7: JPEG_MODERN
        ConstMemory jpeg_tables;

        u32 new_subfile_type = 0;
        u16 subfile_type = 0;

        u32 samples_per_pixel = 1; // channels
        u32 bpp = 0; // sum of bits_per_sample
        std::vector<u64> bits_per_sample; // bits in each channel
        u32 sample_bits = 8; // default to 8 bits per sample

        u32 rows_per_strip = StripHeightNoLimit;
        std::vector<u64> strip_offsets;
        std::vector<u64> strip_byte_counts;

        // Tile support
        u32 tile_width = 0;
        u32 tile_length = 0;
        std::vector<u64> tile_offsets;
        std::vector<u64> tile_byte_counts;

        Palette palette;
        /// Raw SHORT ColorMap in file byte order (RRR…, GGG…, BBB…). Used when index bits > 8.
        ConstMemory colormap_in_file;
        u32 colormap_entry_count = 0;

        ConstMemory icc_profile;

        std::string image_description;
        std::string software;
        std::string document_name;
    };

    enum class Tag : u16
    {
        NewSubfileType = 254, // LONG
        SubfileType = 255, // SHORT
        ImageWidth = 256,
        ImageLength = 257,
        BitsPerSample = 258,
        Compression = 259,
        PhotometricInterpretation = 262,
        //Threshholding = 263, // SHORT
        //CellWidth = 264, // SHORT
        //CellLength = 265, // SHORT
        FillOrder = 266, // SHORT
        DocumentName = 269,
        ImageDescription = 270,
        //Make = 271, // ASCII
        //Model = 272, // ASCII
        StripOffsets = 273,
        Orientation = 274,
        SamplesPerPixel = 277,
        RowsPerStrip = 278,
        StripByteCounts = 279,
        //MinSampleValue = 280, // SHORT
        //MaxSampleValue = 281, // SHORT
        XResolution = 282,
        YResolution = 283,
        PlanarConfiguration = 284,
        PageName = 285, // ASCII
        XPosition = 286,
        YPosition = 287,
        FreeOffsets = 288,
        FreeByteCounts = 289,
        GrayResponseUnit = 290,
        GrayResponseCurve = 291,
        Group3Options = 292, // LONG - T4Options in tiff 6.0 spec
        Group4Options = 293, // LONG - T6Options in tiff 6.0 spec
        ResolutionUnit = 296,
        PageNumber = 297,
        TransferFunction = 301,
        Software = 305,
        DateTime = 306,
        Artist = 315,
        HostComputer = 316,
        Predictor = 317,
        WhitePoint = 318,
        PrimaryChromaticities = 319,
        ColorMap = 320,
        HalftoneHints = 321,
        TileWidth = 322,
        TileLength = 323,
        TileOffsets = 324,
        TileByteCounts = 325,
        InkSet = 332,
        InkNames = 333, // ASCII
        NumberOfInks = 334,
        DotRange = 336,
        ExtraSamples = 338, // SHORT
        SampleFormat = 339,
        SMinSampleValue = 340,
        SMaxSampleValue = 341,
        JPEGProc = 512,
        JPEGInterchangeFormat = 513,
        JPEGInterchangeFormatLength = 514,
        JPEGRestartInterval = 515,
        JPEGLosslessPredictors = 517,
        JPEGPointTransforms = 518,
        JPEGQTables = 519,
        JPEGDCTables = 520,
        JPEGACTables = 521,
        YCbCrCoefficients = 529,
        YCbCrSubSampling = 530,
        ReferenceBlackWhite = 532,
        JPEGTables = 347,
        XMP = 700,
        //Copyright = 33432, // ASCII
        Matteing = 32995,
        DataType = 32996,
        ImageDepth = 32997,
        TileDepth = 32998,
        PhotoshopImageResources = 34377,
        EXIF = 34665,
        ICCProfile = 34675,

        IPTC = 33723,
        ImageSourceData = 37724, // Adobe
        ModelPixelScaleTag = 33550,
        ModelTiepointTag = 33922,
        GeoKeyDirectoryTag = 34735,
        GeoAsciiParamsTag = 34737,
        GDAL_METADATA = 42113,
        GraphicsMagickPhotometricInterpretation = 65535, // vendor tag; PhotometricInterpretation when IFD is unsorted
    };

    static inline
    u64 getSize(Type type, u64 count)
    {
        u64 size = 0;

        switch (type)
        {
            case Type::BYTE:
            case Type::ASCII:
            case Type::SBYTE:
            case Type::UNDEFINED:
                size = count;
                break;

            case Type::SHORT:
            case Type::SSHORT:
                size = count * 2;
                break;

            case Type::LONG:
            case Type::SLONG:
            case Type::FLOAT:
                size = count * 4;
                break;

            case Type::RATIONAL:
            case Type::SRATIONAL:
            case Type::DOUBLE:
            case Type::LONG8:
            case Type::SLONG8:
            case Type::IFD8:
                size = count * 8;
                break;
        }

        return size;
    }

    template <typename Pointer>
    u64 getOffset(Pointer p, bool is_big_tiff)
    {
        return is_big_tiff ? p.read64() : p.read32();
    }

    template <typename Pointer>
    u64 getUnsigned(Pointer& p, Type type)
    {
        u64 value = 0;

        switch (type)
        {
            case Type::BYTE:
                value = p.read8();
                break;
            case Type::SHORT:
                value = p.read16();
                break;
            case Type::LONG:
                value = p.read32();
                break;
            case Type::LONG8:
                value = p.read64();
                break;
            default:
                // MANGO TODO: parsing failure
                printLine(Print::Error, "    [getUnsigned] Unsupported type: {}", int(type));
                break;
        }

        return value;
    }

    template <typename Pointer>
    float getFloat(Pointer& p)
    {
        float value = p.read32f();
        return value;
    }

    template <typename Pointer>
    std::vector<u64> getUnsignedArray(Pointer p, ConstMemory memory, Type type, u64 count, bool is_big_tiff)
    {
        std::vector<u64> values;

        const u64 offset_size = is_big_tiff ? 8 : 4;

        if (getSize(type, count) > offset_size)
        {
            p = memory.address + getOffset(p, is_big_tiff);
        }

        for (u64 i = 0; i < count; ++i)
        {
            u64 value = getUnsigned(p, type);
            values.push_back(value);
        }

        return values;
    }

    template <typename Pointer>
    std::vector<float> getFloatArray(Pointer p, ConstMemory memory, Type type, u64 count, bool is_big_tiff)
    {
        std::vector<float> values;

        const u64 offset_size = is_big_tiff ? 8 : 4;

        if (getSize(type, count) > offset_size)
        {
            p = memory.address + getOffset(p, is_big_tiff);
        }

        for (u64 i = 0; i < count; ++i)
        {
            float value = getFloat(p);
            values.push_back(value);
        }

        return values;
    }

    template <typename Pointer>
    float getRational(Pointer p, ConstMemory memory, Type type, bool is_big_tiff)
    {
        const u64 offset_size = is_big_tiff ? 8 : 4;

        if (getSize(type, 1) > offset_size)
        {
            p = memory.address + getOffset(p, is_big_tiff);
        }

        u32 numerator = getUnsigned(p, Type::LONG);
        u32 denominator = getUnsigned(p, Type::LONG);
        float value = float(numerator) / denominator;
        return value;
    }

    template <typename Pointer>
    std::vector<float> getRationalArray(Pointer p, ConstMemory memory, Type type, u64 count, bool is_big_tiff)
    {
        std::vector<float> values;

        const u64 offset_size = is_big_tiff ? 8 : 4;

        if (getSize(type, count) > offset_size)
        {
            p = memory.address + getOffset(p, is_big_tiff);
        }

        for (u64 i = 0; i < count; ++i)
        {
            u32 numerator = getUnsigned(p, Type::LONG);
            u32 denominator = getUnsigned(p, Type::LONG);
            float value = float(numerator) / denominator;
            values.push_back(value);
        }

        return values;
    }

    template <typename Pointer>
    std::string getAscii(Pointer p, ConstMemory memory, Type type, bool is_big_tiff)
    {
        const char* temp = reinterpret_cast<const char*>(memory.address + getOffset(p, is_big_tiff));
        std::string value = temp;
        return value;
    }

#define TIFF_CASE_UNSIGNED(name, value) \
    case Tag::name: \
        context.value = getUnsigned(p, type); \
        printLine(Print::Info, "    [" #name "]"); \
        printLine(Print::Info, "      value: {}", context.value); \
        break

    template <typename Pointer>
    void parse_ifd(IFDContext& context, ConstMemory memory, Pointer p, bool is_big_tiff, bool is_little_endian)
    {
        Tag tag = Tag(p.read16());
        Type type = Type(p.read16());
        u64 count = is_big_tiff ? p.read64() : p.read32();

        bool suppress_info = true;

        switch (tag)
        {
            TIFF_CASE_UNSIGNED(NewSubfileType, new_subfile_type);
            TIFF_CASE_UNSIGNED(SubfileType, subfile_type);
            TIFF_CASE_UNSIGNED(ImageWidth, width);
            TIFF_CASE_UNSIGNED(ImageLength, height);

            case Tag::BitsPerSample:
            {
                std::vector<u64> values = getUnsignedArray(p, memory, type, count, is_big_tiff);
                context.bits_per_sample = values;
                context.bpp = std::accumulate(values.begin(), values.end(), 0);
                printLine(Print::Info, "    [BitsPerSample]");

                std::string channels_str;
                for (size_t i = 0; i < values.size(); ++i)
                {
                    if (i > 0) channels_str += ", ";
                    channels_str += std::to_string(values[i]);
                }
                printLine(Print::Info, "      values: [{}] -> {} bits", channels_str, context.bpp);
                break;
            }

            TIFF_CASE_UNSIGNED(Compression, compression);

            case Tag::PhotometricInterpretation:
                context.photometric = u16(getUnsigned(p, type));
                context.photometric_specified = true;
                printLine(Print::Info, "    [PhotometricInterpretation]");
                printLine(Print::Info, "      value: {}", context.photometric);
                break;

            case Tag::GraphicsMagickPhotometricInterpretation:
            {
                // GraphicsMagick can write PhotometricInterpretation at tag 65535 when the IFD is unsorted.
                if (!context.photometric_specified)
                {
                    u32 v = getUnsigned(p, type);
                    if (v <= u32(PhotometricInterpretation::YCBCR) ||
                        v == u32(PhotometricInterpretation::CFA) ||
                        v == u32(PhotometricInterpretation::LOGL) ||
                        v == u32(PhotometricInterpretation::LOGLUV) ||
                        v == u32(PhotometricInterpretation::LINEAR_RAW))
                    {
                        context.photometric = u16(v);
                        printLine(Print::Info, "    [PhotometricInterpretation] (tag 65535)");
                        printLine(Print::Info, "      value: {}", context.photometric);
                    }
                }
                break;
            }

            TIFF_CASE_UNSIGNED(FillOrder, fill_order);

            case Tag::DocumentName:
                context.document_name = getAscii(p, memory, type, is_big_tiff);
                break;

            case Tag::ImageDescription:
                context.image_description = getAscii(p, memory, type, is_big_tiff);
                break;

            case Tag::PageName:
                context.page_name = getAscii(p, memory, type, is_big_tiff);
                break;

            TIFF_CASE_UNSIGNED(Orientation, orientation);
            TIFF_CASE_UNSIGNED(SamplesPerPixel, samples_per_pixel);
            TIFF_CASE_UNSIGNED(RowsPerStrip, rows_per_strip);
            
            TIFF_CASE_UNSIGNED(TileWidth, tile_width);
            TIFF_CASE_UNSIGNED(TileLength, tile_length);

            case Tag::TileOffsets:
            {
                printLine(Print::Info, "    [TileOffsets]");
                context.tile_offsets = getUnsignedArray(p, memory, type, count, is_big_tiff);
                suppress_info = false;
                break;
            }

            case Tag::TileByteCounts:
            {
                printLine(Print::Info, "    [TileByteCounts]");
                context.tile_byte_counts = getUnsignedArray(p, memory, type, count, is_big_tiff);
                suppress_info = false;
                break;
            }

            case Tag::StripOffsets:
            {
                printLine(Print::Info, "    [StripOffsets]");
                context.strip_offsets = getUnsignedArray(p, memory, type, count, is_big_tiff);
                suppress_info = false;
                break;
            }

            case Tag::StripByteCounts:
            {
                printLine(Print::Info, "    [StripByteCounts]");
                context.strip_byte_counts = getUnsignedArray(p, memory, type, count, is_big_tiff);
                suppress_info = false;
                break;
            }

            TIFF_CASE_UNSIGNED(ResolutionUnit, resolution_unit);
            TIFF_CASE_UNSIGNED(PageNumber, page_number);

            case Tag::XPosition:
                context.x_position = getRational(p, memory, type, is_big_tiff);
                printLine(Print::Info, "    [XPosition]");
                printLine(Print::Info, "      value: {}", context.x_position);
                break;

            case Tag::YPosition:
                context.y_position = getRational(p, memory, type, is_big_tiff);
                printLine(Print::Info, "    [YPosition]");
                printLine(Print::Info, "      value: {}", context.y_position);
                break;

            case Tag::XResolution:
                context.x_resolution = getRational(p, memory, type, is_big_tiff);
                printLine(Print::Info, "    [XResolution]");
                printLine(Print::Info, "      value: {}", context.x_resolution);
                break;

            case Tag::YResolution:
                context.y_resolution = getRational(p, memory, type, is_big_tiff);
                printLine(Print::Info, "    [YResolution]");
                printLine(Print::Info, "      value: {}", context.y_resolution);
                break;

            TIFF_CASE_UNSIGNED(Group3Options, group3_options);
            TIFF_CASE_UNSIGNED(Group4Options, group4_options);

            case Tag::ReferenceBlackWhite:
            {
                std::vector<float> values = getRationalArray(p, memory, type, count, is_big_tiff);
                if (values.size() >= 6)
                {
                    for (int i = 0; i < 6; ++i)
                    {
                        context.reference_black_white[i] = values[i];
                    }
                }
                break;
            }

            case Tag::YCbCrCoefficients:
            {
                std::vector<float> values = getRationalArray(p, memory, type, count, is_big_tiff);
                if (values.size() >= 3)
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        context.y_cb_cr_coefficients[i] = values[i];
                    }
                }
                break;
            }

            TIFF_CASE_UNSIGNED(PlanarConfiguration, planar_configuration);
            TIFF_CASE_UNSIGNED(Predictor, predictor);

            case Tag::Software:
                context.software = getAscii(p, memory, type, is_big_tiff);
                break;

            case Tag::ColorMap:
            {
                printLine(Print::Info, "    [ColorMap]");

                if (context.bits_per_sample.empty())
                {
                    printLine(Print::Warning, "      ColorMap before BitsPerSample; cannot size palette.");
                    break;
                }

                if (type != Type::SHORT && type != Type::SSHORT)
                {
                    printLine(Print::Warning, "      ColorMap type {} is not SHORT; ignored.", u32(type));
                    break;
                }

                const int bitsPerSample = int(context.bits_per_sample[0]);
                if (bitsPerSample < 1 || bitsPerSample > 16)
                {
                    printLine(Print::Error, "      Unsupported indexed BitsPerSample: {} (supported: 1..16).", bitsPerSample);
                    break;
                }

                const u32 theoretical_entries = 1u << bitsPerSample;
                const u64 expected_ifd_values = 3ull * u64(theoretical_entries);

                if (count != expected_ifd_values)
                {
                    printLine(Print::Warning,
                        "      ColorMap IFD count {} != expected {} for {}-bit indices; using available data.",
                        count, expected_ifd_values, bitsPerSample);
                }

                const u64 offset_size = is_big_tiff ? 8 : 4;
                const u64 cmap_byte_size = getSize(type, count);
                if (cmap_byte_size < 6)
                {
                    printLine(Print::Error, "      ColorMap size too small.");
                    break;
                }

                const u8* cmap_base = nullptr;
                if (cmap_byte_size > offset_size)
                {
                    u64 off = getOffset(p, is_big_tiff);
                    if (off >= memory.size || off + cmap_byte_size > memory.size)
                    {
                        printLine(Print::Error, "      ColorMap data out of bounds.");
                        break;
                    }
                    cmap_base = memory.address + off;
                }
                else
                {
                    printLine(Print::Warning, "      Inline ColorMap not supported for this entry size.");
                    break;
                }

                context.colormap_in_file = ConstMemory(cmap_base, size_t(cmap_byte_size));
                context.colormap_entry_count = u32(std::min<u64>(theoretical_entries, count / 3));
                if (!context.colormap_entry_count)
                {
                    context.colormap_in_file = ConstMemory();
                    printLine(Print::Error, "      ColorMap has no entries.");
                    break;
                }

                const u32 nstride = context.colormap_entry_count;

                auto read_cmap_short = [cmap_base, is_little_endian](u32 index_in_plane, u32 plane, u32 plane_len) -> u16
                {
                    const u64 offset = u64(plane) * u64(plane_len) + u64(index_in_plane);
                    return is_little_endian ? littleEndian::uload16(cmap_base + offset * 2)
                                            : bigEndian::uload16(cmap_base + offset * 2);
                };

                if (bitsPerSample <= 8)
                {
                    context.palette.size = theoretical_entries;
                    for (u32 i = 0; i < theoretical_entries; ++i)
                    {
                        const u32 index = std::min(i, nstride > 0 ? nstride - 1 : 0u);
                        u32 r = read_cmap_short(index, 0, nstride) >> 8;
                        u32 g = read_cmap_short(index, 1, nstride) >> 8;
                        u32 b = read_cmap_short(index, 2, nstride) >> 8;
                        context.palette[i] = Color(r, g, b, 0xff);
                    }
                }
                else
                {
                    context.palette.size = 0;
                }

                printLine(Print::Info, "      palette entries used: {}", context.colormap_entry_count);
                break;
            }

            TIFF_CASE_UNSIGNED(InkSet, ink_set);
            TIFF_CASE_UNSIGNED(NumberOfInks, number_of_inks);
            TIFF_CASE_UNSIGNED(DotRange, dot_range);

            case Tag::InkNames:
                context.ink_names = getAscii(p, memory, type, is_big_tiff);
                break;

            case Tag::ExtraSamples:
            {
                context.extra_samples = getUnsignedArray(p, memory, type, count, is_big_tiff);
                printLine(Print::Info, "    [ExtraSamples]");
                print(Print::Info, "      values: ");
                for (auto value : context.extra_samples)
                {
                    print(Print::Info, "{} ", value);
                }
                printLine(Print::Info, "");

                for (size_t i = 0; i < context.extra_samples.size(); ++i)
                {
                    // ExtraSamples 0: unspecified
                    //              1: associated alpha (premultiplied color)
                    if (context.extra_samples[i] == 1)
                    {
                        context.associated_alpha = true;
                        // Absolute channel index is resolved in parseIFDs() once SamplesPerPixel is known.
                        context.associated_alpha_index = i;
                    }
                }

                break;
            }

            case Tag::SampleFormat:
            {
                context.sample_format = getUnsignedArray(p, memory, type, count, is_big_tiff);
                printLine(Print::Info, "    [SampleFormat]");
                print(Print::Info, "      values: ");
                for (auto value : context.sample_format)
                {
                    print(Print::Info, "{} ", value);
                }
                printLine(Print::Info, "");
                break;
            }

            case Tag::SMinSampleValue:
            {
                context.s_min_sample_value = getFloatArray(p, memory, type, count, is_big_tiff);
                printLine(Print::Info, "    [SMinSampleValue]");
                print(Print::Info, "      values: ");
                for (auto value : context.s_min_sample_value)
                {
                    print(Print::Info, "{} ", value);
                }
                printLine(Print::Info, "");
                break;
            }

            case Tag::SMaxSampleValue:
            {
                context.s_max_sample_value = getFloatArray(p, memory, type, count, is_big_tiff);
                printLine(Print::Info, "    [SMaxSampleValue]");
                print(Print::Info, "      values: ");
                for (auto value : context.s_max_sample_value)
                {
                    print(Print::Info, "{} ", value);
                }
                printLine(Print::Info, "");
                break;
            }

            case Tag::WhitePoint:
            {
                std::vector<float> values = getRationalArray(p, memory, type, count, is_big_tiff);
                context.white_point = float32x2(values[0], values[1]);
                printLine(Print::Info, "    [WhitePoint]");
                printLine(Print::Info, "      white point: {}, {}", float(context.white_point.x), float(context.white_point.y));
                break;
            }

            case Tag::PrimaryChromaticities:
            {
                std::vector<float> values = getRationalArray(p, memory, type, count, is_big_tiff);
                context.red_primary = float32x2(values[0], values[1]);
                context.green_primary = float32x2(values[2], values[3]);
                context.blue_primary = float32x2(values[4], values[5]);
                printLine(Print::Info, "    [PrimaryChromaticities]");
                printLine(Print::Info, "      red primary: {}, {}", float(context.red_primary.x), float(context.red_primary.y));
                printLine(Print::Info, "      green primary: {}, {}", float(context.green_primary.x), float(context.green_primary.y));
                printLine(Print::Info, "      blue primary: {}, {}", float(context.blue_primary.x), float(context.blue_primary.y));
                break;
            }

            TIFF_CASE_UNSIGNED(JPEGProc, jpeg_proc);
            TIFF_CASE_UNSIGNED(JPEGInterchangeFormat, jpeg_interchange_format);
            TIFF_CASE_UNSIGNED(JPEGInterchangeFormatLength, jpeg_interchange_format_length);
            TIFF_CASE_UNSIGNED(JPEGRestartInterval, jpeg_restart_interval);
            TIFF_CASE_UNSIGNED(JPEGLosslessPredictors, jpeg_lossless_predictors);
            TIFF_CASE_UNSIGNED(JPEGPointTransforms, jpeg_point_transforms);

            case Tag::JPEGQTables:
            {
                context.jpeg_qt_tables = getUnsignedArray(p, memory, type, count, is_big_tiff);
                printLine(Print::Info, "    [JPEGQTables]\n      {} tables", context.jpeg_qt_tables.size());
                break;
            }

            case Tag::JPEGDCTables:
            {
                context.jpeg_dc_tables = getUnsignedArray(p, memory, type, count, is_big_tiff);
                printLine(Print::Info, "    [JPEGDCTables]\n      {} tables", context.jpeg_dc_tables.size());
                break;
            }

            case Tag::JPEGACTables:
            {
                context.jpeg_ac_tables = getUnsignedArray(p, memory, type, count, is_big_tiff);
                printLine(Print::Info, "    [JPEGACTables]\n      {} tables", context.jpeg_ac_tables.size());
                break;
            }

            case Tag::YCbCrSubSampling:
            {
                context.y_cb_cr_sub_sampling = getUnsignedArray(p, memory, type, count, is_big_tiff);
                printLine(Print::Info, "    [YCbCrSubSampling]");

                // Print individual channel values
                std::string channels_str;
                for (size_t i = 0; i < context.y_cb_cr_sub_sampling.size(); ++i)
                {
                    if (i > 0) channels_str += ", ";
                    channels_str += std::to_string(context.y_cb_cr_sub_sampling[i]);
                }
                printLine(Print::Info, "      values: [{}]", channels_str);
                break;
            }

            case Tag::JPEGTables:
            {
                printLine(Print::Info, "    [JPEGTables]");
                u64 offset = getOffset(p, is_big_tiff);
                context.jpeg_tables = ConstMemory(memory.address + offset, count);
                printLine(Print::Info, "      offset: {}, length: {} bytes", offset, count);
                break;
            }

            case Tag::DateTime:
            case Tag::Artist:
            case Tag::ModelPixelScaleTag:
            case Tag::ModelTiepointTag:
            case Tag::GeoKeyDirectoryTag:
            case Tag::GeoAsciiParamsTag:
            case Tag::GDAL_METADATA:
            case Tag::IPTC:
            case Tag::ImageSourceData:
                // ignored tags
                break;

            case Tag::Matteing:
            case Tag::DataType:
            case Tag::ImageDepth:
            case Tag::TileDepth:
                // deprecated tags
                break;

            case Tag::XMP:
            {
                u64 offset = getOffset(p, is_big_tiff);
                //context.xmp = ConstMemory(memory.address + offset, count);
                printLine(Print::Info, "    [XMP]");
                printLine(Print::Info, "      length: {} bytes", offset, count);
                break;
            }

            case Tag::EXIF:
            {
                u64 offset = getOffset(p, is_big_tiff);
                //context.exif = ConstMemory(memory.address + offset, count);
                printLine(Print::Info, "    [EXIF]");
                printLine(Print::Info, "      length: {} bytes", offset, count);
                break;
            }

            case Tag::PhotoshopImageResources:
            {
                u64 offset = getOffset(p, is_big_tiff);
                //context.photoshop_image_resources = ConstMemory(memory.address + offset, count);
                printLine(Print::Info, "    [PhotoshopImageResources]");
                printLine(Print::Info, "      length: {} bytes", offset, count);
                break;
            }

            case Tag::ICCProfile:
            {
                u64 offset = getOffset(p, is_big_tiff);
                context.icc_profile = ConstMemory(memory.address + offset, count);
                printLine(Print::Info, "    [ICCProfile]");
                printLine(Print::Info, "      length: {} bytes", count);
                break;
            }

            default:
                printLine(Print::Info, "    [UNKNOWN: {}]", int(tag));
                suppress_info = false;
                break;
        }

        if (!suppress_info)
        {
            printLine(Print::Info, "      type: {}, count: {}", int(type), count);
        }
    }

    static
    int read_first_lzw_code_9(ConstMemory input, bool msb_first)
    {
        if (input.size < 2)
            return -1;
        const u8* p = input.address;
        if (msb_first)
            return ((p[0] << 1) | (p[1] >> 7)) & 0x1ff;
        return (p[0] | ((p[1] & 1) << 8)) & 0x1ff;
    }

    static
    bool lzw_decompress(Memory output, ConstMemory input)
    {
        const int ClearCode = 256;
        const int EoiCode = 257;

        // Auto-detect bit order: which interpretation yields ClearCode (256) as the first code
        bool is_msb_first = true;
        int code_msb = read_first_lzw_code_9(input, true);
        int code_lsb = read_first_lzw_code_9(input, false);
        if (code_msb == ClearCode)
            is_msb_first = true;
        else if (code_lsb == ClearCode)
            is_msb_first = false;

        struct lzw_entry
        {
            s16 prefix;
            u8 first;
            u8 suffix;
        } codes[4096];

        u8 decode_stack[4096];

        for (int i = 0; i < 256; i++)
        {
            codes[i].prefix = -1;
            codes[i].first = static_cast<u8>(i);
            codes[i].suffix = static_cast<u8>(i);
        }

        int next_table_entry = 258;

        const u8* src_ptr = input.address;
        const u8* src_end = input.address + input.size;
        u8* dest_ptr = output.address;
        u8* dest_end = output.address + output.size;

        u64 data = 0;
        int data_bits = 0;
        int codesize = 9;
        int codemask = (1 << codesize) - 1;

        auto GetNextCode = [&]() -> int
        {
            // MSB-first: widen before reading when the next dictionary index needs it.
            if (is_msb_first)
            {
                if ((next_table_entry == 511 && codesize == 9) ||
                    (next_table_entry == 1023 && codesize == 10) ||
                    (next_table_entry == 2047 && codesize == 11))
                {
                    ++codesize;
                    codemask = (1 << codesize) - 1;
                }
            }

            // Fill bit buffer (adaptive: MSB-first or LSB-first)
            while (data_bits < codesize && src_ptr < src_end)
            {
                const u8 byte_val = *src_ptr++;
                if (!is_msb_first)
                {
                    data |= u64(byte_val) << data_bits;
                }
                else
                {
                    data = (data << 8) | byte_val;
                }
                data_bits += 8;
            }

            if (data_bits < codesize)
                return -1; // EOF

            // Extract code (adaptive)
            int code;
            if (!is_msb_first)
            {
                code = data & codemask;
                data >>= codesize;
            }
            else
            {
                code = (data >> (data_bits - codesize)) & codemask;
            }
            data_bits -= codesize;

            return code;
        };

        auto WriteString = [&](int code) -> bool
        {
            if (code < 0 || code >= 4096)
                return false;

            u8* sp = decode_stack;
            int chain_length = 0;
            
            // Build string by following prefix chain
            while (code >= 0)
            {
                if (chain_length >= 4096)
                    return false; // Infinite loop protection
                
                if (sp >= decode_stack + 4096)
                    return false; // Stack overflow protection
                
                *sp++ = codes[code].suffix;
                if (codes[code].prefix < 0)
                    break;
                code = codes[code].prefix;
                chain_length++;
            }

            // Output string in reverse order
            if (dest_ptr + (sp - decode_stack) > dest_end)
                return false; // Buffer overflow protection

            while (sp > decode_stack)
                *dest_ptr++ = *--sp;

            return true;
        };

        auto AddStringToTable = [&](int oldcode, u8 first_char)
        {
            if (next_table_entry < 4096)
            {
                if (oldcode < 0 || oldcode >= 4096)
                    return;

                codes[next_table_entry].prefix = oldcode;
                codes[next_table_entry].first = codes[oldcode].first;
                codes[next_table_entry].suffix = first_char;
                next_table_entry++;

                // LSB-first: check transitions after adding entry
                if (!is_msb_first)
                {
                    if ((next_table_entry == 512 && codesize == 9) ||
                        (next_table_entry == 1024 && codesize == 10) ||
                        (next_table_entry == 2048 && codesize == 11))
                    {
                        codesize++;
                        codemask = (1 << codesize) - 1;
                    }
                }
            }
        };

        int OldCode = -1;
        bool first_code = true;

        for (;;)
        {
            int Code = GetNextCode();
            if (Code < 0 || Code == EoiCode)
                break;

            // Handle non-compliant streams that don't start with Clear code (LSB-first compatibility)
            if (!is_msb_first && first_code && Code != ClearCode)
            {
                next_table_entry = 258;
                codesize = 9;
                codemask = (1 << codesize) - 1;
                if (!WriteString(Code))
                    return false;
                OldCode = Code;
                first_code = false;
                continue;
            }
            first_code = false;

            if (Code >= next_table_entry + 1)
                return false;

            if (Code == ClearCode)
            {
                next_table_entry = 258;
                codesize = 9;
                codemask = (1 << codesize) - 1;
                OldCode = -1;
                Code = GetNextCode();
                if (Code < 0 || Code == EoiCode)
                    break;
                if (!WriteString(Code))
                    return false;
                OldCode = Code;
            }
            else
            {
                if (Code < next_table_entry)
                {
                    if (!WriteString(Code))
                        return false;
                    if (OldCode >= 0)
                        AddStringToTable(OldCode, codes[Code].first);
                    OldCode = Code;
                }
                else
                {
                    if (OldCode >= 0)
                    {
                        AddStringToTable(OldCode, codes[OldCode].first);
                        if (!WriteString(Code))
                            return false;
                        OldCode = Code;
                    }
                }
            }
        }

        return true;
    }

    static
    void webp_decompress(ImageDecodeStatus& status, const Surface& target, ConstMemory input)
    {
        ImageDecoder imageDecoder(input, ".webp");

        // Check that the decoder exists; mango could be configured without WebP support
        if (!imageDecoder.isDecoder())
        {
            status.setError("[WebP] ImageDecoder not found.");
            return;
        }

        ImageDecodeStatus webp_status = imageDecoder.decode(target, ImageDecodeOptions(), 0, 0, 0);
        if (!webp_status.success)
        {
            status.setError(webp_status.info);
            return;
        }

        status.direct = webp_status.direct;
    }

    struct TIFFHeader
    {
        ByteOrder byte_order;
        u16 version;
        u64 first_ifd_offset;
        bool is_little_endian;
        bool is_big_tiff = false;

        ImageHeader header;

        bool parse(ConstMemory memory)
        {
            if (memory.size < 8)
            {
                header.setError("[ImageDecoder.TIFF] File too small.");
                return false;
            }

            const u8* p = memory.address;

            // Check byte order
            u16 byte_order_value = bigEndian::uload16(p);
            if (byte_order_value == u16(ByteOrder::TIFF_LITTLE_ENDIAN))
            {
                is_little_endian = true;
                byte_order = ByteOrder::TIFF_LITTLE_ENDIAN;
            }
            else if (byte_order_value == u16(ByteOrder::TIFF_BIG_ENDIAN))
            {
                is_little_endian = false;
                byte_order = ByteOrder::TIFF_BIG_ENDIAN;
            }
            else
            {
                header.setError("[ImageDecoder.TIFF] Invalid byte order.");
                return false;
            }

            // Check TIFF version
            if (is_little_endian)
            {
                version = littleEndian::uload16(p + 2);
            }
            else
            {
                version = bigEndian::uload16(p + 2);
            }

            switch (version)
            {
                case 42:
                    is_big_tiff = false;
                    break;
                case 43:
                    is_big_tiff = true;
                    break;
                default:
                    header.setError("[ImageDecoder.TIFF] Invalid TIFF version ({}).", version);
                    return false;
            }

            // Get first IFD offset
            if (is_big_tiff)
            {
                u16 bytesize_per_offset;
                u16 reserved;

                if (is_little_endian)
                {
                    bytesize_per_offset = littleEndian::uload16(p + 4);
                    reserved = littleEndian::uload16(p + 6);
                    first_ifd_offset = littleEndian::uload64(p + 8);
                }
                else
                {
                    bytesize_per_offset = bigEndian::uload16(p + 4);
                    reserved = bigEndian::uload16(p + 6);
                    first_ifd_offset = bigEndian::uload64(p + 8);
                }

                if (bytesize_per_offset != 8)
                {
                    header.setError("[ImageDecoder.TIFF] Invalid bytesize per offset ({}).", bytesize_per_offset);
                    return false;
                }

                if (reserved != 0)
                {
                    header.setError("[ImageDecoder.TIFF] Invalid reserved ({}).", reserved);
                    return false;
                }

                if (first_ifd_offset < 8)
                {
                    header.setError("[ImageDecoder.TIFF] Invalid first IFD offset ({}).", first_ifd_offset);
                    return false;
                }

                if (first_ifd_offset >= memory.size)
                {
                    header.setError("[ImageDecoder.TIFF] First IFD offset out of bounds ({}).", first_ifd_offset);
                    return false;
                }
            }
            else
            {
                if (is_little_endian)
                {
                    first_ifd_offset = littleEndian::uload32(p + 4);
                }
                else
                {
                    first_ifd_offset = bigEndian::uload32(p + 4);
                }
            }

            printLine(Print::Info, "[TIFF]");
            printLine(Print::Info, "  Byte Order:    {}", is_little_endian ? "LittleEndian" : "BigEndian");
            printLine(Print::Info, "  Version:       {}", version);
            printLine(Print::Info, "  First IFD:     {}", first_ifd_offset);

            return true;
        }
    };

    // ------------------------------------------------------------------------
    // PixarLog (COMPRESSION_PIXARLOG, 32909)
    //
    // Pixar's companded 11-bit log encoding wrapped in a zlib stream. The codec
    // stores extended-range linear color (values up to ~25.0) as 11-bit tokens
    // with horizontal differencing, then zlib-compresses them. The 11-bit token
    // -> linear mapping is the "ToLinearF" table below. We always decode to
    // linear float to preserve the HDR range.
    //
    // ------------------------------------------------------------------------

    // 2049-entry token -> linear table (2048 codes + one slop entry).
    const float* pixarLogToLinearTable()
    {
        static const std::vector<float> table = []
        {
            constexpr int TSIZE = 2048;
            constexpr double RATIO = 1.004;
            constexpr double ONE = 1250.0;

            std::vector<float> t(TSIZE + 1);

            double c = std::log(RATIO);
            int nlin = int(1.0 / c); // must be integer
            c = 1.0 / nlin;
            double b = std::exp(-c * ONE); // b * exp(c * ONE) == 1
            double linstep = b * c * std::exp(1.0);

            int j = 0;
            for (int i = 0; i < nlin; ++i)
                t[j++] = float(i * linstep);
            for (int i = nlin; i < TSIZE; ++i)
                t[j++] = float(b * std::exp(c * i));
            t[TSIZE] = t[TSIZE - 1];

            return t;
        }();

        return table.data();
    }

    // Undo horizontal differencing for one row and convert tokens to linear
    // float. Accumulators wrap modulo 2048 (the 11-bit code space).
    void pixarLogAccumulateRow(const u16* wp, int n, int stride, float* op, const float* toLinear)
    {
        constexpr u32 mask = 0x7ff;

        if (n < stride)
            return;

        u32 acc [4] = { 0, 0, 0, 0 };

        for (int k = 0; k < stride; ++k)
        {
            acc[k] = wp[k] & mask;
            op[k] = toLinear[acc[k]];
        }

        for (int x = stride; x < n; x += stride)
        {
            for (int k = 0; k < stride; ++k)
            {
                acc[k] = (acc[k] + wp[x + k]) & mask;
                op[x + k] = toLinear[acc[k]];
            }
        }
    }

    // ------------------------------------------------------------------------
    // SGI LogLuv / LogL (COMPRESSION_SGILOG 34676, COMPRESSION_SGILOG24 34677)
    //
    // Greg Ward Larson's high dynamic range encoding. Luminance is stored as a
    // log2 value; chromaticity as CIE (u',v'). Three on-disk variants:
    //
    //   LogL  (16-bit)   PHOTOMETRIC_LOGL  + SGILOG     grayscale luminance
    //   LogLuv(32-bit)   PHOTOMETRIC_LOGLUV+ SGILOG     sign+15 logL, 8 u, 8 v
    //   LogLuv(24-bit)   PHOTOMETRIC_LOGLUV+ SGILOG24   10 logL', 14 (u,v) index
    //
    // The 16/32-bit forms use a byte-plane PackBits-style RLE; the 24-bit form
    // is stored raw. We decode to linear float: luminance via 2^x, chromaticity
    // to CIE XYZ, then to linear RGB with the SGI primaries matrix (which maps
    // the equal-energy white [1,1,1] to RGB white). Negative (out-of-gamut)
    // components are clamped to 0; highlights above 1 are preserved.
    //
    // ------------------------------------------------------------------------

    // 24-bit (u',v') decode grid from uvcode.h (version 1.0, 1997).
    struct UVRow { float ustart; short nus, ncum; };

    static constexpr float UV_SQSIZ  = 0.003500f;
    static constexpr float UV_VSTART = 0.016940f;
    static constexpr int   UV_NVS    = 163;
    static constexpr int   UV_NDIVS  = 16289;

    const UVRow* logLuvUVRow()
    {
        static const UVRow uv_row[UV_NVS] =
        {
            {0.247663f,4,0},{0.243779f,6,4},{0.241684f,7,10},{0.237874f,9,17},
            {0.235906f,10,26},{0.232153f,12,36},{0.228352f,14,48},{0.226259f,15,62},
            {0.222371f,17,77},{0.220410f,18,94},{0.214710f,21,112},{0.212714f,22,133},
            {0.210721f,23,155},{0.204976f,26,178},{0.202986f,27,204},{0.199245f,29,231},
            {0.195525f,31,260},{0.193560f,32,291},{0.189878f,34,323},{0.186216f,36,357},
            {0.186216f,36,393},{0.182592f,38,429},{0.179003f,40,467},{0.175466f,42,507},
            {0.172001f,44,549},{0.172001f,44,593},{0.168612f,46,637},{0.168612f,46,683},
            {0.163575f,49,729},{0.158642f,52,778},{0.158642f,52,830},{0.158642f,52,882},
            {0.153815f,55,934},{0.153815f,55,989},{0.149097f,58,1044},{0.149097f,58,1102},
            {0.142746f,62,1160},{0.142746f,62,1222},{0.142746f,62,1284},{0.138270f,65,1346},
            {0.138270f,65,1411},{0.138270f,65,1476},{0.132166f,69,1541},{0.132166f,69,1610},
            {0.126204f,73,1679},{0.126204f,73,1752},{0.126204f,73,1825},{0.120381f,77,1898},
            {0.120381f,77,1975},{0.120381f,77,2052},{0.120381f,77,2129},{0.112962f,82,2206},
            {0.112962f,82,2288},{0.112962f,82,2370},{0.107450f,86,2452},{0.107450f,86,2538},
            {0.107450f,86,2624},{0.107450f,86,2710},{0.100343f,91,2796},{0.100343f,91,2887},
            {0.100343f,91,2978},{0.095126f,95,3069},{0.095126f,95,3164},{0.095126f,95,3259},
            {0.095126f,95,3354},{0.088276f,100,3449},{0.088276f,100,3549},{0.088276f,100,3649},
            {0.088276f,100,3749},{0.081523f,105,3849},{0.081523f,105,3954},{0.081523f,105,4059},
            {0.081523f,105,4164},{0.074861f,110,4269},{0.074861f,110,4379},{0.074861f,110,4489},
            {0.074861f,110,4599},{0.068290f,115,4709},{0.068290f,115,4824},{0.068290f,115,4939},
            {0.068290f,115,5054},{0.063573f,119,5169},{0.063573f,119,5288},{0.063573f,119,5407},
            {0.063573f,119,5526},{0.057219f,124,5645},{0.057219f,124,5769},{0.057219f,124,5893},
            {0.057219f,124,6017},{0.050985f,129,6141},{0.050985f,129,6270},{0.050985f,129,6399},
            {0.050985f,129,6528},{0.050985f,129,6657},{0.044859f,134,6786},{0.044859f,134,6920},
            {0.044859f,134,7054},{0.044859f,134,7188},{0.040571f,138,7322},{0.040571f,138,7460},
            {0.040571f,138,7598},{0.040571f,138,7736},{0.036339f,142,7874},{0.036339f,142,8016},
            {0.036339f,142,8158},{0.036339f,142,8300},{0.032139f,146,8442},{0.032139f,146,8588},
            {0.032139f,146,8734},{0.032139f,146,8880},{0.027947f,150,9026},{0.027947f,150,9176},
            {0.027947f,150,9326},{0.023739f,154,9476},{0.023739f,154,9630},{0.023739f,154,9784},
            {0.023739f,154,9938},{0.019504f,158,10092},{0.019504f,158,10250},{0.019504f,158,10408},
            {0.016976f,161,10566},{0.016976f,161,10727},{0.016976f,161,10888},{0.016976f,161,11049},
            {0.012639f,165,11210},{0.012639f,165,11375},{0.012639f,165,11540},{0.009991f,168,11705},
            {0.009991f,168,11873},{0.009991f,168,12041},{0.009016f,170,12209},{0.009016f,170,12379},
            {0.009016f,170,12549},{0.006217f,173,12719},{0.006217f,173,12892},{0.005097f,175,13065},
            {0.005097f,175,13240},{0.005097f,175,13415},{0.003909f,177,13590},{0.003909f,177,13767},
            {0.002340f,177,13944},{0.002389f,170,14121},{0.001068f,164,14291},{0.001653f,157,14455},
            {0.000717f,150,14612},{0.001614f,143,14762},{0.000270f,136,14905},{0.000484f,129,15041},
            {0.001103f,123,15170},{0.001242f,115,15293},{0.001188f,109,15408},{0.001011f,103,15517},
            {0.000709f,97,15620},{0.000301f,89,15717},{0.002416f,82,15806},{0.003251f,76,15888},
            {0.003246f,69,15964},{0.004141f,62,16033},{0.005963f,55,16095},{0.008839f,47,16150},
            {0.010490f,40,16197},{0.016994f,31,16237},{0.023659f,21,16268},
        };
        return uv_row;
    }

    inline double logLuvL16toY(int p16)
    {
        constexpr double M_LN2_ = 0.69314718055994530942;
        int Le = p16 & 0x7fff;
        if (!Le)
            return 0.0;
        double Y = std::exp(M_LN2_ / 256.0 * (Le + 0.5) - M_LN2_ * 64.0);
        return (p16 & 0x8000) ? -Y : Y;
    }

    inline double logLuvL10toY(int p10)
    {
        constexpr double M_LN2_ = 0.69314718055994530942;
        if (p10 == 0)
            return 0.0;
        return std::exp(M_LN2_ / 64.0 * (p10 + 0.5) - M_LN2_ * 12.0);
    }

    // Decode a 14-bit (u',v') chromaticity index via binary search in the grid.
    bool logLuvUVDecode(double& u, double& v, int c)
    {
        if (c < 0 || c >= UV_NDIVS)
            return false;

        const UVRow* uv_row = logLuvUVRow();

        unsigned lower = 0;
        unsigned upper = UV_NVS;
        while (upper - lower > 1)
        {
            unsigned vi = (lower + upper) >> 1;
            int ui = c - uv_row[vi].ncum;
            if (ui > 0)
                lower = vi;
            else if (ui < 0)
                upper = vi;
            else
            {
                lower = vi;
                break;
            }
        }

        unsigned vi = lower;
        int ui = c - uv_row[vi].ncum;
        u = double(uv_row[vi].ustart) + (double(ui) + 0.5) * double(UV_SQSIZ);
        v = double(UV_VSTART) + (double(vi) + 0.5) * double(UV_SQSIZ);
        return true;
    }

    void logLuv32toXYZ(u32 p, float* XYZ)
    {
        double L = logLuvL16toY(int(p) >> 16);
        if (L <= 0.0)
        {
            XYZ[0] = XYZ[1] = XYZ[2] = 0.0f;
            return;
        }

        constexpr double UVSCALE = 410.0;
        double u = 1.0 / UVSCALE * ((p >> 8 & 0xff) + 0.5);
        double v = 1.0 / UVSCALE * ((p & 0xff) + 0.5);
        double s = 1.0 / (6.0 * u - 16.0 * v + 12.0);
        double x = 9.0 * u * s;
        double y = 4.0 * v * s;

        XYZ[0] = float(x / y * L);
        XYZ[1] = float(L);
        XYZ[2] = float((1.0 - x - y) / y * L);
    }

    void logLuv24toXYZ(u32 p, float* XYZ)
    {
        constexpr double U_NEU = 0.210526316;
        constexpr double V_NEU = 0.473684211;

        double L = logLuvL10toY(p >> 14 & 0x3ff);
        if (L <= 0.0)
        {
            XYZ[0] = XYZ[1] = XYZ[2] = 0.0f;
            return;
        }

        double u, v;
        if (!logLuvUVDecode(u, v, int(p & 0x3fff)))
        {
            u = U_NEU;
            v = V_NEU;
        }

        double s = 1.0 / (6.0 * u - 16.0 * v + 12.0);
        double x = 9.0 * u * s;
        double y = 4.0 * v * s;

        XYZ[0] = float(x / y * L);
        XYZ[1] = float(L);
        XYZ[2] = float((1.0 - x - y) / y * L);
    }

    // CIE XYZ (equal-energy white) -> linear RGB, SGI CCIR-709 primaries.
    void logLuvXYZtoLinearRGB(const float* xyz, float* rgb)
    {
        double r =  2.690 * xyz[0] - 1.276 * xyz[1] - 0.414 * xyz[2];
        double g = -1.022 * xyz[0] + 1.978 * xyz[1] + 0.044 * xyz[2];
        double b =  0.061 * xyz[0] - 0.224 * xyz[1] + 1.163 * xyz[2];

        rgb[0] = float(r > 0.0 ? r : 0.0);
        rgb[1] = float(g > 0.0 ? g : 0.0);
        rgb[2] = float(b > 0.0 ? b : 0.0);
    }

    // Byte-plane PackBits-style RLE used by SGILOG 16/32-bit. Fills `planes`
    // byte planes (high to low) of `npixels` tokens. Advances bp/cc.
    bool logLuvDecodeRLE(u32* tp, int npixels, const u8*& bp, size_t& cc, int planes)
    {
        for (int i = 0; i < npixels; ++i)
            tp[i] = 0;

        for (int shft = (planes - 1) * 8; shft >= 0; shft -= 8)
        {
            int i = 0;
            while (i < npixels && cc > 0)
            {
                if (*bp >= 128)
                {
                    // run
                    if (cc < 2)
                        break;
                    int rc = *bp++ + (2 - 128);
                    u32 b = u32(*bp++) << shft;
                    cc -= 2;
                    while (rc-- && i < npixels)
                        tp[i++] |= b;
                }
                else
                {
                    // non-run
                    int rc = *bp++;
                    while (--cc && rc-- && i < npixels)
                        tp[i++] |= u32(*bp++) << shft;
                }
            }
            if (i != npixels)
                return false;
        }
        return true;
    }

    static
    u32 resolve_jpeg_interchange_length(ConstMemory memory, u32 offset, u32 stated_length, u32 strip_offset = 0)
    {
        if (!offset || offset >= memory.size)
        {
            return 0;
        }

        const u8* start = memory.address + offset;
        const u8* file_end = memory.end();
        u32 avail = u32(file_end - start);

        auto ends_with_eoi = [](const u8* p, u32 len) -> bool
        {
            return len >= 2 && p[len - 2] == 0xff && p[len - 1] == 0xd9;
        };

        // Old-style JPEG: interchange segment is often header-only; entropy data follows in strips.
        if (stated_length && strip_offset && strip_offset == offset + stated_length)
        {
            return stated_length;
        }

        if (stated_length && stated_length <= avail)
        {
            if (strip_offset && strip_offset >= offset + stated_length)
            {
                return stated_length;
            }

            if (ends_with_eoi(start, stated_length))
            {
                return stated_length;
            }
        }

        u32 length = stated_length;
        if (!length || length > avail)
        {
            length = avail;
        }

        if (ends_with_eoi(start, length))
        {
            return length;
        }

        const u8* scan_end = file_end;
        if (stated_length && stated_length <= avail)
        {
            scan_end = start + stated_length;
        }
        else if (strip_offset && strip_offset > offset)
        {
            scan_end = memory.address + strip_offset;
        }

        for (const u8* q = start + 1; q < scan_end; ++q)
        {
            if (q[-1] == 0xff && q[0] == 0xd9)
            {
                return u32(q + 1 - start);
            }
        }

        return length;
    }

    bool is_complete_jpeg(ConstMemory memory)
    {
        return memory.size >= 4 &&
            memory.address[0] == 0xff && memory.address[1] == 0xd8 &&
            memory.address[memory.size - 2] == 0xff && memory.address[memory.size - 1] == 0xd9;
    }

    bool jpeg_contains_marker(ConstMemory memory, u8 marker)
    {
        const u8* p = memory.address;
        const u8* end = memory.address + memory.size;

        if (!p || memory.size < 4 || p[0] != 0xff || p[1] != 0xd8)
        {
            return false;
        }

        p += 2;

        while (p + 4 <= end)
        {
            if (p[0] != 0xff)
            {
                ++p;
                continue;
            }

            u8 m = p[1];

            if (m == 0xd9 || m == 0xda)
            {
                break;
            }

            u16 len = bigEndian::uload16(p + 2);

            if (m == marker)
            {
                return true;
            }

            p += 2 + len;
        }

        return false;
    }

    // Photoshop compression=7 often stores DQT/DHT in JPEGTables while each strip is an
    // otherwise complete JPEG (SOI..EOI) that references those tables but omits them.
    bool merge_jpeg_with_tables(ConstMemory strip, ConstMemory tables, Buffer& buffer)
    {
        buffer.reset();

        if (!strip.address || strip.size < 4 || strip.address[0] != 0xff || strip.address[1] != 0xd8)
        {
            return false;
        }

        const bool need_dqt = !jpeg_contains_marker(strip, 0xdb);
        const bool need_dht = !jpeg_contains_marker(strip, 0xc4);

        if (!need_dqt && !need_dht)
        {
            return false;
        }

        if (!tables.address || tables.size < 4)
        {
            return false;
        }

        buffer.append(strip.address, 2); // SOI

        const u8* p = tables.address + 2;
        const u8* end = tables.address + tables.size;

        while (p + 4 <= end)
        {
            if (p[0] != 0xff)
            {
                ++p;
                continue;
            }

            u8 m = p[1];

            if (m == 0xd9)
            {
                break;
            }

            if (m == 0xd8)
            {
                p += 2;
                continue;
            }

            u16 len = bigEndian::uload16(p + 2);

            if (p + 2 + len > end)
            {
                break;
            }

            if ((m == 0xdb && need_dqt) || (m == 0xc4 && need_dht))
            {
                buffer.append(p, 2 + len);
            }

            p += 2 + len;
        }

        buffer.append(strip.address + 2, strip.size - 2);
        return true;
    }

    void resolve_strip_byte_counts(IFDContext& ctx, size_t file_size)
    {
        if (!ctx.strip_byte_counts.empty() || ctx.strip_offsets.empty())
        {
            return;
        }

        if (ctx.strip_offsets.size() == 1)
        {
            u64 offset = ctx.strip_offsets[0];
            if (offset < file_size)
            {
                ctx.strip_byte_counts.push_back(file_size - offset);
            }
            return;
        }

        for (size_t i = 0; i < ctx.strip_offsets.size(); ++i)
        {
            u64 start = ctx.strip_offsets[i];
            u64 end = (i + 1 < ctx.strip_offsets.size())
                ? ctx.strip_offsets[i + 1]
                : file_size;
            if (end > start)
            {
                ctx.strip_byte_counts.push_back(end - start);
            }
        }
    }

    static void tiff_cielab16_to_xyz(u32 l, s32 a, s32 b,
        float X0, float Y0, float Z0, float& X, float& Y, float& Z)
    {
        // libtiff TIFFCIELab16ToXYZ (D50 reference white).
        float L = float(l) * 100.0f / 65535.0f;
        float cby;
        float tmp;

        if (L < 8.856f)
        {
            Y = (L * Y0) / 903.292f;
            cby = 7.787f * (Y / Y0) + 16.0f / 116.0f;
        }
        else
        {
            cby = (L + 16.0f) / 116.0f;
            Y = Y0 * cby * cby * cby;
        }

        tmp = float(a) / 256.0f / 500.0f + cby;
        if (tmp < 0.2069f)
        {
            X = X0 * (tmp - 0.13793f) / 7.787f;
        }
        else
        {
            X = X0 * tmp * tmp * tmp;
        }

        tmp = cby - float(b) / 256.0f / 200.0f;
        if (tmp < 0.2069f)
        {
            Z = Z0 * (tmp - 0.13793f) / 7.787f;
        }
        else
        {
            Z = Z0 * tmp * tmp * tmp;
        }
    }

    static void tiff_cielab8_to_xyz(u8 l, s8 a, s8 b, float X0, float Y0, float Z0, float& X, float& Y, float& Z)
    {
        tiff_cielab16_to_xyz(u32(l) * 257, s32(a) * 256, s32(b) * 256, X0, Y0, Z0, X, Y, Z);
    }

    struct CielabDisplayLuts
    {
        static constexpr int range = 1500;

        float Yr2r[range + 1];
        float rstep;
        float gstep;
        float bstep;

        CielabDisplayLuts()
        {
            constexpr float YCR = 100.0f;
            constexpr float Y0 = 1.0f;
            constexpr float inv_gamma = 1.0f / 2.4f;

            rstep = (YCR - Y0) / float(range);
            gstep = rstep;
            bstep = rstep;

            for (int i = 0; i <= range; ++i)
            {
                const float v = 255.0f * std::pow(float(i) / float(range), inv_gamma);
                Yr2r[i] = v;
            }
        }
    };

    static const CielabDisplayLuts& tiff_cielab_luts()
    {
        static const CielabDisplayLuts luts;
        return luts;
    }

    static void tiff_xyz_to_display_rgb(float X, float Y, float Z, u8& r, u8& g, u8& b)
    {
        // libtiff display_sRGB + TIFFXYZToRGB (1500-step LUT).
        constexpr float m[9] =
        {
            3.2410f, -1.5374f, -0.4986f,
            -0.9692f, 1.8760f, 0.0416f,
            0.0556f, -0.2040f, 1.0570f,
        };

        constexpr float YCR = 100.0f;
        constexpr float Y0 = 1.0f;

        float Yr = m[0] * X + m[1] * Y + m[2] * Z;
        float Yg = m[3] * X + m[4] * Y + m[5] * Z;
        float Yb = m[6] * X + m[7] * Y + m[8] * Z;

        Yr = std::clamp(Yr, Y0, YCR);
        Yg = std::clamp(Yg, Y0, YCR);
        Yb = std::clamp(Yb, Y0, YCR);

        auto to_channel = [] (float Yl, float step, const float* lut) -> u8
        {
            size_t i = size_t((Yl - Y0) / step);
            i = std::min(size_t(CielabDisplayLuts::range), i);
            return u8(std::clamp(lut[i] + 0.5f, 0.0f, 255.0f));
        };

        r = to_channel(Yr, tiff_cielab_luts().rstep, tiff_cielab_luts().Yr2r);
        g = to_channel(Yg, tiff_cielab_luts().gstep, tiff_cielab_luts().Yr2r);
        b = to_channel(Yb, tiff_cielab_luts().bstep, tiff_cielab_luts().Yr2r);
    }

    static void tiff_cielab_ref_white(const IFDContext& ctx, float& X0, float& Y0, float& Z0)
    {
        float wx = 0.3457f;
        float wy = 0.3585f;

        if (ctx.white_point.y > 0.0f)
        {
            wx = ctx.white_point.x;
            wy = ctx.white_point.y;
        }

        Y0 = 100.0f;
        X0 = wx / wy * Y0;
        Z0 = (1.0f - wx - wy) / wy * Y0;
    }

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        TIFFHeader m_header;
        IFDContext m_context;
        bool m_is_little_endian;

        Interface(ConstMemory memory)
            : m_memory(memory)
        {
            //printEnable(Print::Info, true); // enable for debugging
            if (m_header.parse(memory))
            {
                m_is_little_endian = m_header.is_little_endian;
                parseIFDs();
            }
            else
            {
                header = m_header.header;
            }
        }

        ~Interface()
        {
        }

        bool is_tiled_via_strips() const
        {
            if (m_context.tile_width == 0 || m_context.tile_length == 0)
            {
                return false;
            }

            if (!m_context.tile_offsets.empty())
            {
                return false;
            }

            if (m_context.strip_offsets.empty())
            {
                return false;
            }

            const u32 xtiles = div_ceil(m_context.width, m_context.tile_width);
            const u32 ytiles = div_ceil(m_context.height, m_context.tile_length);
            size_t expected = size_t(xtiles) * ytiles;

            if (m_context.planar_configuration == 2)
            {
                expected *= m_context.samples_per_pixel;
            }

            return m_context.strip_offsets.size() == expected &&
                   m_context.strip_byte_counts.size() == expected;
        }

        bool suppress_icc_after_decode() const
        {
            // Decoded pixels are already display RGBA; don't apply source-space ICC profiles.
            if (m_context.compression == u32(Compression::JPEG_LEGACY) ||
                m_context.compression == u32(Compression::JPEG_MODERN))
            {
                if (m_context.photometric == u32(PhotometricInterpretation::SEPARATED) ||
                    m_context.photometric == u32(PhotometricInterpretation::YCBCR))
                {
                    return true;
                }
            }

            return m_context.photometric == u32(PhotometricInterpretation::CIELAB) ||
                   m_context.photometric == u32(PhotometricInterpretation::ICCLAB) ||
                   m_context.photometric == u32(PhotometricInterpretation::ITULAB);
        }

        u64 tile_data_offset(size_t i) const
        {
            return m_context.tile_offsets.empty()
                ? m_context.strip_offsets[i]
                : m_context.tile_offsets[i];
        }

        u64 tile_data_bytes(size_t i) const
        {
            return m_context.tile_byte_counts.empty()
                ? m_context.strip_byte_counts[i]
                : m_context.tile_byte_counts[i];
        }

        void parseIFDs()
        {
            if (m_header.first_ifd_offset >= m_memory.size)
            {
                header.setError("[ImageDecoder.TIFF] IFD offset out of bounds.");
                return;
            }

            const u8* p = m_memory.address + m_header.first_ifd_offset;
            const u8* end = m_memory.end();

            if (p + 2 > end)
            {
                header.setError("[ImageDecoder.TIFF] Cannot read IFD count.");
                return;
            }

            // Read IFD count
            u64 ifd_count;
            if (m_is_little_endian)
            {
                ifd_count = m_header.is_big_tiff ? littleEndian::uload64(p)
                                                 : littleEndian::uload16(p);
            }
            else
            {
                ifd_count = m_header.is_big_tiff ? bigEndian::uload64(p)
                                                 : bigEndian::uload16(p);
            }
            p += m_header.is_big_tiff ? 8 : 2;

            printLine(Print::Info, "  IFD entries: {}", ifd_count);

            size_t ifd_entry_size = m_header.is_big_tiff ? 20 : 12;

            // Parse each IFD entry
            for (u64 i = 0; i < ifd_count; ++i)
            {
                if (p + ifd_entry_size > end)
                {
                    header.setError("[ImageDecoder.TIFF] IFD entry {} out of bounds.", i);
                    return;
                }

                if (m_is_little_endian)
                {
                    parse_ifd(m_context, m_memory, LittleEndianConstPointer(p), m_header.is_big_tiff, m_is_little_endian);
                }
                else
                {
                    parse_ifd(m_context, m_memory, BigEndianConstPointer(p), m_header.is_big_tiff, m_is_little_endian);
                }

                p += ifd_entry_size;
            }

            u64 offset_to_next_ifd = 0;
            if (m_header.is_big_tiff)
            {
                offset_to_next_ifd = bigEndian::uload64(p);
            }
            else
            {
                offset_to_next_ifd = bigEndian::uload32(p);
            }

            // MANGO TODO: read ALL IFDs (multiple images)

            // Validate required fields
            if (m_context.width == 0 || m_context.height == 0)
            {
                header.setError("[ImageDecoder.TIFF] Missing required image dimensions.");
                return;
            }

            if (m_context.bits_per_sample.empty())
            {
                header.setError("[ImageDecoder.TIFF] Missing required bits per sample.");
                return;
            }

            // check channel sizes
            u32 sample_bits = m_context.bits_per_sample[0];

            // Yes, we check first sample twice to keep the code simple
            for (auto bits : m_context.bits_per_sample)
            {
                if (bits != sample_bits)
                {
                    // We only support images with the same bit depth per channel
                    header.setError("[ImageDecoder.TIFF] Unsupported channel configuration.");
                    return;
                }
            }

            m_context.sample_bits = sample_bits;

            if (!m_context.extra_samples.empty())
            {
                for (size_t i = 0; i < m_context.extra_samples.size(); ++i)
                {
                    if (m_context.extra_samples[i] == 1)
                    {
                        const u32 color_channels = m_context.samples_per_pixel - u32(m_context.extra_samples.size());
                        m_context.associated_alpha_index = color_channels + i;
                        m_context.associated_alpha = true;
                        break;
                    }
                }
            }

            if (!m_context.photometric_specified &&
                m_context.photometric == u32(PhotometricInterpretation::RGB) &&
                m_context.ink_set == 1 &&
                m_context.samples_per_pixel == 4)
            {
                m_context.photometric = u16(PhotometricInterpretation::SEPARATED);
            }

            if (m_context.photometric == u32(PhotometricInterpretation::PALETTE))
            {
                if (sample_bits > 8)
                {
                    const u64 need = u64(m_context.colormap_entry_count) * 6;
                    if (!m_context.colormap_entry_count || m_context.colormap_in_file.size < need)
                    {
                        header.setError("[ImageDecoder.TIFF] Palette image with wide indices requires a valid ColorMap.");
                        return;
                    }
                }
                else if (m_context.palette.size == 0)
                {
                    header.setError("[ImageDecoder.TIFF] Indexed TIFF requires a ColorMap.");
                    return;
                }
            }

            // Set header info
            header.width = m_context.width;
            header.height = m_context.height;
            header.depth = 0;
            header.levels = 0;
            header.faces = 0;
            header.format = getImageFormat();
            header.compression = TextureCompression::NONE;
            header.premultiplied = m_context.associated_alpha;

            // Forward the ICC profile at header() time (decode() also sets it). Color space:
            // integer RGB/grayscale TIFF is sRGB by convention (the default); floating-point
            // samples carry linear scene data. An embedded ICC profile defines it exactly.
            icc = suppress_icc_after_decode() ? ConstMemory() : m_context.icc_profile;

            if (m_context.icc_profile.size)
            {
                header.color.primaries = ColorPrimaries::Unspecified;
                header.color.transfer = TransferFunction::Unspecified;
            }
            else if (header.format.isFloat())
            {
                header.linear = true;
                header.color.transfer = TransferFunction::Linear;
            }

            resolve_strip_byte_counts(m_context, m_memory.size);

            u32 data_size = std::accumulate(m_context.strip_byte_counts.begin(), m_context.strip_byte_counts.end(), 0u);
            data_size += std::accumulate(m_context.tile_byte_counts.begin(), m_context.tile_byte_counts.end(), 0u);

            printLine(Print::Info, "  Image: {} x {} ({} bpp, {} channels)", 
                     m_context.width, m_context.height, header.format.bits, m_context.samples_per_pixel);
            printLine(Print::Info, "  Data: {} bytes", data_size);
        }

        Format getImageFormat()
        {
            if (m_context.compression == u32(Compression::JPEG_LEGACY) ||
                m_context.compression == u32(Compression::JPEG_MODERN))
            {
                if (m_context.samples_per_pixel == 1)
                {
                    return LuminanceFormat(8, Format::UNORM, 8, 0);
                }
                else
                {
                    return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                }
            }

            // PixarLog carries companded 11-bit log samples with extended range.
            // The stored BitsPerSample/SampleFormat is only a compatibility hint
            if (m_context.compression == u32(Compression::PIXARLOG))
            {
                switch (m_context.samples_per_pixel)
                {
                    case 1: return LuminanceFormat(32, Format::FLOAT32, 32, 0);
                    case 2: return LuminanceFormat(64, Format::FLOAT32, 32, 32);
                    case 3: return Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0);
                    default: return Format(128, Format::FLOAT32, Format::RGBA, 32, 32, 32, 32);
                }
            }

            // SGI LogLuv / LogL: high dynamic range, always decode to linear float.
            if (m_context.photometric == u32(PhotometricInterpretation::LOGLUV))
            {
                return Format(96, Format::FLOAT32, Format::RGB, 32, 32, 32, 0);
            }
            if (m_context.photometric == u32(PhotometricInterpretation::LOGL))
            {
                return LuminanceFormat(32, Format::FLOAT32, 32, 0);
            }

            // bit-packed formats resolve to at least 8 bits per channel
            //u32 bits = m_context.sample_bits >= 8 ? m_context.sample_bits : 8;
            u32 bits = round_ceil(m_context.sample_bits, 8);

            Format::Type type = Format::UNORM;

            u64 sample_format = u64(SampleFormat::UINT);
            if (!m_context.sample_format.empty())
            {
                // assume all channels have the same sample format
                // MANGO TODO: support different sample formats per channel
                sample_format = m_context.sample_format[0];
            }

            switch (SampleFormat(sample_format))
            {
                case SampleFormat::UINT:
                    type = Format::UNORM;
                    bits = std::min(bits, 16u); // max 16 bits per channel
                    break;

                case SampleFormat::SINT:
                    type = Format::SNORM;
                    bits = std::min(bits, 16u); // max 16 bits per channel
                    break;

                case SampleFormat::FLOAT:
                    if (bits == 16)
                    {
                        type = Format::FLOAT16;
                    }
                    else if (bits == 32)
                    {
                        type = Format::FLOAT32;
                    }
                    else if (bits == 64)
                    {
                        type = Format::FLOAT64;
                    }
                    else
                    {
                        header.setError("Unsupported float: {} bits", bits);
                        return Format();
                    }
                    break;

                case SampleFormat::UNDEFINED:
                default:
                    header.setError("Unsupported sample format: {}", sample_format);
                    return Format();
            }

            switch (PhotometricInterpretation(m_context.photometric))
            {
                case PhotometricInterpretation::WHITE_IS_ZERO:
                case PhotometricInterpretation::BLACK_IS_ZERO:
                {
                    if (m_context.samples_per_pixel == 1)
                    {
                        return LuminanceFormat(bits * 1, type, bits, 0);
                    }
                    else if (m_context.samples_per_pixel == 2)
                    {
                        return LuminanceFormat(bits * 2, type, bits, bits);
                    }
                }

                case PhotometricInterpretation::RGB:
                {
                    if (m_context.samples_per_pixel == 3)
                    {
                        return Format(bits * 3, type, Format::RGB, bits, bits, bits, 0);
                    }
                    else if (m_context.samples_per_pixel >= 4)
                    {
                        return Format(bits * 4, type, Format::RGBA, bits, bits, bits, bits);
                    }
                    else
                    {
                        header.setError("Unsupported number of channels: {}", m_context.samples_per_pixel);
                        return Format();
                    }   
                }

                case PhotometricInterpretation::PALETTE:
                    if (m_context.sample_bits > 8)
                    {
                        return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    }
                    return IndexedFormat(8);

                case PhotometricInterpretation::SEPARATED:
                    return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                case PhotometricInterpretation::LOGLUV:
                case PhotometricInterpretation::LOGL:
                    // Note: LOGLUV / LOGL are handled earlier (decoded to linear float),
                    // so they never reach this unsupported list.
                    return Format();

                case PhotometricInterpretation::CIELAB:
                case PhotometricInterpretation::ICCLAB:
                case PhotometricInterpretation::ITULAB:
                    return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                case PhotometricInterpretation::TRANSPARENCY_MASK:
                case PhotometricInterpretation::CFA:
                case PhotometricInterpretation::LINEAR_RAW:
                    // MANGO TODO: support different sample formats per channel
                    header.setError("Unsupported PhotometricInterpretation: {}", m_context.photometric);
                    return Format();

                case PhotometricInterpretation::YCBCR:
                    if (m_context.samples_per_pixel == 3)
                    {
                        return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    }
                    header.setError("Unsupported number of channels: {}", m_context.samples_per_pixel);
                    return Format();

                default:
                    header.setError("Unknown PhotometricInterpretation: {}", m_context.photometric);
                    return Format();
            }
        }

        ConstMemory memory(int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(face);
            MANGO_UNREFERENCED(depth);
            return ConstMemory();
        }

        ImageDecodeStatus decode(const Surface& dest, const ImageDecodeOptions& options, int level, int depth, int face) override
        {
            MANGO_UNREFERENCED(face);
            MANGO_UNREFERENCED(depth);
            MANGO_UNREFERENCED(level);
            MANGO_UNREFERENCED(options);

            ImageDecodeStatus status;

            if (m_context.rows_per_strip == StripHeightNoLimit)
            {
                m_context.rows_per_strip = header.height;
            }

            // xxx
            printLine(Print::Info, "    [decode]");
            printLine(Print::Info, "      image: {} x {}", header.width, header.height);
            printLine(Print::Info, "      compression: {}", m_context.compression);
            printLine(Print::Info, "      planar_configuration: {} ({})", m_context.planar_configuration,
                m_context.planar_configuration == 1 ? "chunky" : "planar");
            printLine(Print::Info, "      predictor: {} ({})", m_context.predictor,
                m_context.predictor == 1 ? "no prediction" :
                m_context.predictor == 2 ? "horizontal differencing" :
                m_context.predictor == 3 ? "float differencing" : "unknown");
            printLine(Print::Info, "      tile: {} x {}", m_context.tile_width, m_context.tile_length);
            printLine(Print::Info, "      tile_offsets: {}, tile_byte_counts: {}", m_context.tile_offsets.size(), m_context.tile_byte_counts.size());
            printLine(Print::Info, "      strip: {} x {}", m_context.width, m_context.rows_per_strip);
            printLine(Print::Info, "      strip_offsets: {}, strip_byte_counts: {}", m_context.strip_offsets.size(), m_context.strip_byte_counts.size());

            /*
            for (auto byte_count : m_context.strip_byte_counts)
            {
                printLine(Print::Info, "        strip: {} bytes", byte_count);
            }
            */

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            // Calculate target width and height
            int target_width = header.width;
            int target_height = header.height;

            // TIFF compresses complete tiles, even if they only partially overlap the image.
            // This keeps the decoding loops simple as they don't have to clip.
            if (m_context.tile_width > 0 && m_context.tile_length > 0)
            {
                target_width = div_ceil(header.width, m_context.tile_width) * m_context.tile_width;
                target_height = div_ceil(header.height, m_context.tile_length) * m_context.tile_length;
            }

            const bool separated_planar_wide_cmyk =
                m_context.photometric == u32(PhotometricInterpretation::SEPARATED) &&
                m_context.planar_configuration == 2 &&
                m_context.samples_per_pixel == 4 &&
                m_context.sample_bits > 8;

            const bool planar_ycbcr =
                m_context.photometric == u32(PhotometricInterpretation::YCBCR) &&
                m_context.planar_configuration == 2 &&
                m_context.samples_per_pixel == 3;

            const bool chunky_ycbcr =
                m_context.photometric == u32(PhotometricInterpretation::YCBCR) &&
                m_context.planar_configuration == 1 &&
                m_context.samples_per_pixel == 3;

            std::unique_ptr<Bitmap> wide_cmyk_buffer;
            if (separated_planar_wide_cmyk)
            {
                wide_cmyk_buffer = std::make_unique<Bitmap>(
                    target_width,
                    target_height,
                    Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16));
            }

            Surface& decode_surface = wide_cmyk_buffer
                ? static_cast<Surface&>(*wide_cmyk_buffer)
                : const_cast<Surface&>(dest);
            const Format& decode_format = wide_cmyk_buffer ? wide_cmyk_buffer->format : header.format;

            DecodeTargetBitmap target(decode_surface, target_width, target_height, decode_format, m_context.palette, false);

            if (m_context.compression == u32(Compression::JPEG_LEGACY) ||
                m_context.compression == u32(Compression::JPEG_MODERN))
            {
                if (!decompress_jpeg(target, options, level, depth, face))
                {
                    status.setError("JPEG decoding failed");
                    return status;
                }
            }
            else if (planar_ycbcr)
            {
                decodePlanarYCbCr(status, dest);
            }
            else if (chunky_ycbcr)
            {
                decodeChunkyYCbCr(status, dest);
            }
            else if (m_context.tile_offsets.size() > 0 || is_tiled_via_strips())
            {
                // tiles (classic TileOffsets, or legacy SGI tiles stored in StripOffsets)

                if (!is_tiled_via_strips())
                {
                    if (m_context.tile_byte_counts.size() != m_context.tile_offsets.size() ||
                        m_context.tile_byte_counts.empty())
                    {
                        // We could try to infer the tiles from the remaining data in the file but we drop broken files.
                        status.setError("Incorrect or missing tile data.");
                        return status;
                    }
                }

                const u32 tile_width = m_context.tile_width;
                const u32 tile_length = m_context.tile_length;
                const u32 xtiles = div_ceil(header.width, tile_width);
                const u32 ytiles = div_ceil(header.height, tile_length);
                const size_t num_tiles = m_context.tile_offsets.empty()
                    ? m_context.strip_offsets.size()
                    : m_context.tile_offsets.size();

                if (m_context.planar_configuration == 1)
                {
                    // chunky format
    
                    for (size_t i = 0; i < num_tiles; ++i)
                    {
                        ConstMemory memory(m_memory.address + tile_data_offset(i), tile_data_bytes(i));

                        u32 x = (u32(i) % xtiles) * tile_width;
                        u32 y = (u32(i) / xtiles) * tile_length;
    
                        printLine(Print::Info, "    [Tile] {}, {}", x, y);
                        printLine(Print::Info, "      offset: {}, length: {} bytes", tile_data_offset(i), tile_data_bytes(i));
    
                        Surface tile(target, x, y, tile_width, tile_length);
                        decodeRect(status, tile, memory, tile_width, tile_length);
                    }
                }
                else
                {
                    // planar format

                    // MANGO TODO: clear the target surface correctly
                    std::memset(target.image, 0, target.stride * header.height);

                    size_t count = xtiles * ytiles;

                    for (size_t i = 0; i < count; ++i)
                    {
                        u32 x = (u32(i) % xtiles) * tile_width;
                        u32 y = (u32(i) / xtiles) * tile_length;

                        for (u32 channel = 0; channel < m_context.samples_per_pixel; ++channel)
                        {
                            size_t index = channel * count + i;
                            ConstMemory memory(m_memory.address + tile_data_offset(index), tile_data_bytes(index));

                            Surface tile(target, x, y, tile_width, tile_length);
                            decodeRect(status, tile, memory, tile_width, tile_length, channel);
                        }
                    }
                }
            }
            else
            {
                // strips

                if (m_context.strip_byte_counts.size() != m_context.strip_offsets.size() ||
                    m_context.strip_byte_counts.empty())
                {
                    // We could try to infer the strips from the remaining data in the file but we drop broken files.
                    status.setError("Incorrect or missing strip data.");
                    return status;
                }

                if (m_context.planar_configuration == 1)
                {
                    // chunky format

                    u32 y = 0;
                
                    for (size_t i = 0; i < m_context.strip_offsets.size(); ++i)
                    {
                        u32 strip_height = std::min(m_context.rows_per_strip, header.height - y);
    
                        const u8* src = m_memory.address + m_context.strip_offsets[i];
                        u32 bytes = m_context.strip_byte_counts[i];
    
                        Surface strip(target, 0, y, header.width, strip_height);
                        decodeRect(status, strip, ConstMemory(src, bytes), header.width, strip_height);

                        if (!status)
                            return status;
    
                        y += strip_height;
                    }
                }
                else
                {
                    // planar format

                    // MANGO TODO: clear the target surface correctly
                    std::memset(target.image, 0, target.stride * header.height);

                    // Separate planes (PlanarConfiguration 2): all strips for component 0, then all
                    // for component 1, etc. (TIFF 6 §PlanarConfiguration). Same ordering as tiles above
                    // (channel * count + tile). Do not use spatial_strip * spp + channel — that
                    // interleaves strips per region and pairs the wrong plane data with each channel.
                    u32 strips_per_spatial_region = u32(m_context.strip_offsets.size() / m_context.samples_per_pixel);

                    for (size_t spatial_strip = 0; spatial_strip < strips_per_spatial_region; ++spatial_strip)
                    {
                        u32 y = u32(spatial_strip * m_context.rows_per_strip);
                        u32 strip_height = std::min(m_context.rows_per_strip, header.height - y);

                        for (u32 channel = 0; channel < m_context.samples_per_pixel; ++channel)
                        {
                            size_t strip_index = channel * strips_per_spatial_region + spatial_strip;

                            const u8* src = m_memory.address + m_context.strip_offsets[strip_index];
                            u32 bytes = m_context.strip_byte_counts[strip_index];

                            Surface strip(target, 0, y, header.width, strip_height);
                            decodeRect(status, strip, ConstMemory(src, bytes), header.width, strip_height, channel);
                        }
                    }
                }
            }

            if (separated_planar_wide_cmyk)
            {
                convertSeparatedPlanarCmyk16ToRgba(*wide_cmyk_buffer, const_cast<Surface&>(dest), header.width, header.height);
            }
            else if (m_context.photometric == u32(PhotometricInterpretation::SEPARATED) &&
                     m_context.planar_configuration == 2 &&
                     m_context.samples_per_pixel == 4)
            {
                convertSeparatedPlanarCmykInBufferToRgba(target, header.width, header.height);
            }

            target.resolve();

            // Store ICC profile into the ImageDecodeInterface
            icc = suppress_icc_after_decode() ? ConstMemory() : m_context.icc_profile;

            return status;
        }

        static void tiff_ycbcr_to_rgb(u8& r, u8& g, u8& b, int Y, int Cb, int Cr,
            const float luma[3], const float ref[6])
        {
            constexpr int SHIFT = 16;
            constexpr int32_t ONE_HALF = 1 << (SHIFT - 1);

            auto fix = [] (double x) -> int32_t
            {
                return int32_t(x * double(1 << SHIFT) + 0.5);
            };

            auto code2v = [] (float c, float rb, float rw, float cr) -> float
            {
                const float denom = (rw != rb) ? (rw - rb) : 1.0f;
                return (c - rb) * cr / denom;
            };

            const float lumaR = luma[0];
            const float lumaG = luma[1];
            const float lumaB = luma[2];

            const float f1 = 2.0f - 2.0f * lumaR;
            const int32_t D1 = fix(std::clamp(f1, 0.0f, 2.0f));
            const float f2 = lumaR * f1 / lumaG;
            const int32_t D2 = -fix(std::clamp(f2, 0.0f, 2.0f));
            const float f3 = 2.0f - 2.0f * lumaB;
            const int32_t D3 = fix(std::clamp(f3, 0.0f, 2.0f));
            const float f4 = lumaB * f3 / lumaG;
            const int32_t D4 = -fix(std::clamp(f4, 0.0f, 2.0f));

            const int32_t Yi = int32_t(std::clamp(code2v(float(Y), ref[0], ref[1], 255.0f), -128.0f * 32, 128.0f * 32));
            const int32_t Cbi = int32_t(std::clamp(
                code2v(float(Cb - 128), ref[2] - 128.0f, ref[3] - 128.0f, 127.0f), -128.0f * 32, 128.0f * 32));
            const int32_t Cri = int32_t(std::clamp(
                code2v(float(Cr - 128), ref[4] - 128.0f, ref[5] - 128.0f, 127.0f), -128.0f * 32, 128.0f * 32));

            int i = Yi + ((D1 * Cri + ONE_HALF) >> SHIFT);
            r = u8_clamp(i);
            i = Yi + ((D2 * Cri + D4 * Cbi + ONE_HALF) >> SHIFT);
            g = u8_clamp(i);
            i = Yi + ((D3 * Cbi + ONE_HALF) >> SHIFT);
            b = u8_clamp(i);
        }

        static void convertPlanarYCbCrToRgba(Surface& surface, int width, int height,
            const Surface& y_plane, const Surface& cb_plane, const Surface& cr_plane,
            u32 h_sub, u32 v_sub, const float luma[3], const float ref[6])
        {
            const u32 chroma_w = u32((width + int(h_sub) - 1) / int(h_sub));
            const u32 chroma_h = u32((height + int(v_sub) - 1) / int(v_sub));

            for (int y = 0; y < height; ++y)
            {
                u8* drow = surface.image + surface.stride * y;
                const u8* yrow = y_plane.image + y_plane.stride * y;
                const u8* cbrow = cb_plane.image + cb_plane.stride * (y / int(v_sub));
                const u8* crrow = cr_plane.image + cr_plane.stride * (y / int(v_sub));

                for (int x = 0; x < width; ++x)
                {
                    const int yv = yrow[x];
                    const int cb = cbrow[x / int(h_sub)];
                    const int cr = crrow[x / int(h_sub)];
                    u8 r;
                    u8 g;
                    u8 b;
                    tiff_ycbcr_to_rgb(r, g, b, yv, cb, cr, luma, ref);
                    u8* d = drow + x * 4;
                    d[0] = r;
                    d[1] = g;
                    d[2] = b;
                    d[3] = 0xff;
                }
            }

            MANGO_UNREFERENCED(chroma_w);
            MANGO_UNREFERENCED(chroma_h);
        }

        static void convertChunkyYCbCrToRgba(Surface& surface, const u8* packed, int width, int height,
            u32 h_sub, u32 v_sub, const float luma[3], const float ref[6])
        {
            if (h_sub < 1) h_sub = 1;
            if (v_sub < 1) v_sub = 1;

            const u32 chroma_w = u32((width + int(h_sub) - 1) / int(h_sub));
            const u32 mcu_bytes = h_sub * v_sub + 2;
            const u32 macro_row_bytes = chroma_w * mcu_bytes;

            for (int y0 = 0; y0 < height; y0 += int(v_sub))
            {
                const u8* macro = packed + (y0 / int(v_sub)) * macro_row_bytes;

                for (int x0 = 0; x0 < width; x0 += int(h_sub))
                {
                    const u8* mcu = macro + (x0 / int(h_sub)) * mcu_bytes;
                    const int cb = mcu[h_sub * v_sub + 0];
                    const int cr = mcu[h_sub * v_sub + 1];

                    for (int dy = 0; dy < int(v_sub) && y0 + dy < height; ++dy)
                    {
                        u8* drow = surface.image + surface.stride * (y0 + dy);

                        for (int dx = 0; dx < int(h_sub) && x0 + dx < width; ++dx)
                        {
                            const int yv = mcu[dy * int(h_sub) + dx];
                            u8 r;
                            u8 g;
                            u8 b;
                            tiff_ycbcr_to_rgb(r, g, b, yv, cb, cr, luma, ref);
                            u8* d = drow + (x0 + dx) * 4;
                            d[0] = r;
                            d[1] = g;
                            d[2] = b;
                            d[3] = 0xff;
                        }
                    }
                }
            }
        }

        void getYCbCrSubsampling(u32& h_sub, u32& v_sub) const
        {
            if (m_context.y_cb_cr_sub_sampling.size() >= 2)
            {
                h_sub = std::max(u32(1), u32(m_context.y_cb_cr_sub_sampling[0]));
                v_sub = std::max(u32(1), u32(m_context.y_cb_cr_sub_sampling[1]));
            }
            else
            {
                h_sub = 2;
                v_sub = 2;
            }
        }

        void decodePlanarYCbCr(ImageDecodeStatus& status, Surface dest)
        {
            u32 h_sub;
            u32 v_sub;
            getYCbCrSubsampling(h_sub, v_sub);

            const int width = header.width;
            const int height = header.height;
            const u32 chroma_w = u32((width + int(h_sub) - 1) / int(h_sub));
            const u32 chroma_h = u32((height + int(v_sub) - 1) / int(v_sub));

            Bitmap y_plane(width, height, LuminanceFormat(8, Format::UNORM, 8, 0));
            Bitmap cb_plane(int(chroma_w), int(chroma_h), LuminanceFormat(8, Format::UNORM, 8, 0));
            Bitmap cr_plane(int(chroma_w), int(chroma_h), LuminanceFormat(8, Format::UNORM, 8, 0));

            const Surface planes[3] = { y_plane, cb_plane, cr_plane };
            const int plane_width[3] = { width, int(chroma_w), int(chroma_w) };

            if (m_context.strip_byte_counts.size() != m_context.strip_offsets.size() ||
                m_context.strip_byte_counts.empty())
            {
                status.setError("Incorrect or missing strip data.");
                return;
            }

            const u32 strips_per_spatial_region = u32(m_context.strip_offsets.size() / m_context.samples_per_pixel);

            for (u32 spatial_strip = 0; spatial_strip < strips_per_spatial_region; ++spatial_strip)
            {
                const u32 y = spatial_strip * m_context.rows_per_strip;
                const u32 y_strip_height = std::min(m_context.rows_per_strip, u32(header.height) - y);
                const u32 chroma_y = y / v_sub;
                const u32 chroma_strip_height = std::min((y_strip_height + v_sub - 1) / v_sub, chroma_h - chroma_y);

                for (u32 channel = 0; channel < m_context.samples_per_pixel; ++channel)
                {
                    const size_t strip_index = channel * strips_per_spatial_region + spatial_strip;
                    const u8* src = m_memory.address + m_context.strip_offsets[strip_index];
                    const u32 bytes = m_context.strip_byte_counts[strip_index];

                    const int plane_w = plane_width[channel];
                    const u32 plane_y = (channel == 0) ? y : chroma_y;
                    const u32 plane_h = (channel == 0) ? y_strip_height : chroma_strip_height;

                    Surface strip(planes[channel], 0, int(plane_y), plane_w, int(plane_h));
                    decodeRect(status, strip, ConstMemory(src, bytes), plane_w, int(plane_h), channel);
                }
            }

            convertPlanarYCbCrToRgba(dest, width, height, y_plane, cb_plane, cr_plane, h_sub, v_sub,
                m_context.y_cb_cr_coefficients, m_context.reference_black_white);
        }

        void decodeChunkyYCbCr(ImageDecodeStatus& status, Surface dest)
        {
            u32 h_sub;
            u32 v_sub;
            getYCbCrSubsampling(h_sub, v_sub);

            const int width = header.width;
            const int height = header.height;
            const u32 chroma_w = u32((width + int(h_sub) - 1) / int(h_sub));
            const u32 macro_row_bytes = chroma_w * (h_sub * v_sub + 2);
            const u32 macro_rows_total = u32((height + int(v_sub) - 1) / int(v_sub));

            Buffer packed(macro_row_bytes * macro_rows_total);
            Surface packed_surface(width, int(macro_rows_total), LuminanceFormat(8, Format::UNORM, 8, 0), macro_row_bytes, packed.data());

            if (m_context.strip_byte_counts.size() != m_context.strip_offsets.size() ||
                m_context.strip_byte_counts.empty())
            {
                status.setError("Incorrect or missing strip data.");
                return;
            }

            u32 y = 0;

            for (size_t i = 0; i < m_context.strip_offsets.size(); ++i)
            {
                const u32 strip_height = std::min(m_context.rows_per_strip, u32(header.height) - y);
                const u32 macro_y = y / v_sub;
                const u32 macro_strip_rows = (strip_height + v_sub - 1) / v_sub;

                const u8* src = m_memory.address + m_context.strip_offsets[i];
                const u32 bytes = m_context.strip_byte_counts[i];

                Surface strip(packed_surface, 0, int(macro_y), width, int(macro_strip_rows));
                decodeRect(status, strip, ConstMemory(src, bytes), width, int(strip_height));

                y += strip_height;
            }

            convertChunkyYCbCrToRgba(dest, packed.data(), width, height, h_sub, v_sub,
                m_context.y_cb_cr_coefficients, m_context.reference_black_white);
        }

        static void convertSeparatedPlanarCmykInBufferToRgba(Surface& surface, int width, int height)
        {
            const u8* lookup = math::get_linear_to_srgb_table();

            for (int y = 0; y < height; ++y)
            {
                u8* row = surface.image + surface.stride * y;

                for (int x = 0; x < width; ++x)
                {
                    u8* p = row + x * 4;

                    const int C = 255 - p[0];
                    const int M = 255 - p[1];
                    const int Y = 255 - p[2];
                    const int K = 255 - p[3];

                    const int R = (C * K + 127) / 255;
                    const int G = (M * K + 127) / 255;
                    const int B = (Y * K + 127) / 255;

                    p[0] = lookup[R];
                    p[1] = lookup[G];
                    p[2] = lookup[B];
                    p[3] = 0xff;
                }
            }
        }

        static void convertSeparatedPlanarCmyk16ToRgba(const Surface& src, Surface& dst, int width, int height)
        {
            const u8* lookup = math::get_linear_to_srgb_table();

            for (int y = 0; y < height; ++y)
            {
                const u8* srow = src.image + src.stride * y;
                u8* drow = dst.image + dst.stride * y;

                for (int x = 0; x < width; ++x)
                {
                    const u8* p = srow + x * 8;

                    const u32 Cin = uload16(p + 0);
                    const u32 Min = uload16(p + 2);
                    const u32 Yin = uload16(p + 4);
                    const u32 Kin = uload16(p + 6);

                    const u32 C = 65535u - Cin;
                    const u32 M = 65535u - Min;
                    const u32 Y = 65535u - Yin;
                    const u32 K = 65535u - Kin;

                    const u32 R = u32((u64(C) * K + 32767u) / 65535u);
                    const u32 G = u32((u64(M) * K + 32767u) / 65535u);
                    const u32 B = u32((u64(Y) * K + 32767u) / 65535u);

                    const u8 r8 = u8((R * 255u + 32767u) / 65535u);
                    const u8 g8 = u8((G * 255u + 32767u) / 65535u);
                    const u8 b8 = u8((B * 255u + 32767u) / 65535u);

                    u8* d = drow + x * 4;
                    d[0] = lookup[r8];
                    d[1] = lookup[g8];
                    d[2] = lookup[b8];
                    d[3] = 0xff;
                }
            }
        }

        bool decompress_jpeg(DecodeTargetBitmap& target, ImageDecodeOptions options, int level, int depth, int face)
        {
            if (m_context.photometric == 2)
            {
                options.jpeg_colorspace_rgb = true;
            }

            ConstMemory separated_icc;
            if (m_context.photometric == u32(PhotometricInterpretation::SEPARATED))
            {
                separated_icc = m_context.icc_profile;
            }

            auto prepare_jpeg_parser = [&](jpeg::Parser& parser)
            {
                if (separated_icc.size)
                {
                    parser.setSourceIcc(separated_icc);
                }
            };

            std::function<ImageDecodeStatus(ConstMemory, Surface)> decodeJPEG;

            auto writeDHT = [] (Buffer& buffer, u8 id, const u8* table)
            {
                u8 num_codes = 0;

                for (int j = 0; j < 16; ++j)
                {
                    num_codes += table[j];
                }

                if (num_codes > 200)
                {
                    return;
                }

                u16 length = 3 + 16 + num_codes;

                u8* p = buffer.append(2 + length);
                bigEndian::ustore16(p + 0, jpeg::MARKER_DHT);
                bigEndian::ustore16(p + 2, length);
                p[4] = id;

                std::memcpy(p + 5, table, 16 + num_codes);
            };

            auto writeDQT = [] (Buffer& buffer, u8 id, const u8* table)
            {
                u8* p = buffer.append(69);
                bigEndian::ustore16(p + 0, jpeg::MARKER_DQT);
                bigEndian::ustore16(p + 2, 0x43); // length
                p[4] = id; // Table ID + precision (0 = 8-bit)

                std::memcpy(p + 5, table, 64);
            };

            auto writeSOF1 = [](Buffer& buffer, int width, int height, u32 h_sub, u32 v_sub)
            {
                const u8 y_sampling = u8((std::min(h_sub, u32(15)) << 4) | std::min(v_sub, u32(15)));

                u8* p = buffer.append(10);
                bigEndian::ustore16(p + 0, jpeg::MARKER_SOF1);
                bigEndian::ustore16(p + 2, 0x11); // length
                p[4] = 0x08; // Sample precision (8 bits)
                bigEndian::ustore16(p + 5, height);
                bigEndian::ustore16(p + 7, width);
                p[9] = 0x03; // Number of components

                p = buffer.append(3);
                p[0] = 0x01;
                p[1] = y_sampling;
                p[2] = 0x00;

                p = buffer.append(3);
                p[0] = 0x02;
                p[1] = 0x11;
                p[2] = 0x01;

                p = buffer.append(3);
                p[0] = 0x03;
                p[1] = 0x11;
                p[2] = 0x02;
            };

            auto writeSOS = [] (Buffer& buffer)
            {
                u8* p = buffer.append(14);
                bigEndian::ustore16(p + 0, jpeg::MARKER_SOS);
                bigEndian::ustore16(p + 2, 0x0c); // length
                p[4] = 0x03; // Number of components

                p[5] = 0x01;
                p[6] = 0x00;

                p[7] = 0x02;
                p[8] = 0x11;

                p[9] = 0x03;
                p[10] = 0x22;

                p[11] = 0x00;
                p[12] = 0x3f;
                p[13] = 0x00;
            };

            if (m_context.compression == u32(Compression::JPEG_LEGACY))
            {
                if (m_context.jpeg_interchange_format)
                {
                    // Mode A: JIF carries frame/tables; strip/tile data is entropy.
                    u32 block_offset = 0;
                    u32 block_bytes = 0;
                    if (!m_context.tile_offsets.empty())
                    {
                        block_offset = u32(m_context.tile_offsets[0]);
                        block_bytes = u32(m_context.tile_byte_counts[0]);
                    }
                    else if (!m_context.strip_offsets.empty())
                    {
                        block_offset = u32(m_context.strip_offsets[0]);
                        block_bytes = u32(m_context.strip_byte_counts[0]);
                    }

                    u32 header_length = m_context.jpeg_interchange_format_length;
                    if (!header_length)
                    {
                        header_length = resolve_jpeg_interchange_length(
                            m_memory, m_context.jpeg_interchange_format, 0, block_offset);
                    }

                    const u8* header_ptr = m_memory.address + m_context.jpeg_interchange_format;
                    const bool header_is_complete = header_length >= 2 &&
                        header_ptr[header_length - 2] == 0xff &&
                        header_ptr[header_length - 1] == 0xd9;

                    const bool jif_is_complete_block =
                        m_context.jpeg_interchange_format == block_offset &&
                        (m_context.jpeg_interchange_format_length == 0 ||
                         m_context.jpeg_interchange_format_length == block_bytes);

                    ConstMemory jif_memory(m_memory.address + m_context.jpeg_interchange_format, header_length);

                    if (header_is_complete && jif_is_complete_block)
                    {
                        decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                        {
                            MANGO_UNREFERENCED(memory);

                            ImageDecodeInterface tempInterface;
                            jpeg::Parser parser(&tempInterface, jif_memory, jpeg::Parser::RELAXED_PARSER);
                            prepare_jpeg_parser(parser);
                            return parser.decode(surface, options);
                        };
                    }
                    else if (block_offset == m_context.jpeg_interchange_format + header_length)
                    {
                        // Strip/tile follows JIF header contiguously: one JPEG bitstream.
                        decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                        {
                            Buffer buffer;
                            buffer.append(jif_memory);
                            buffer.append(memory);

                            if (memory.size < 2 ||
                                bigEndian::uload16(memory.address + memory.size - 2) != jpeg::MARKER_EOI)
                            {
                                u8* eoi = buffer.append(2);
                                bigEndian::ustore16(eoi, jpeg::MARKER_EOI);
                            }

                            ImageDecodeInterface tempInterface;
                            jpeg::Parser parser(&tempInterface, buffer, jpeg::Parser::RELAXED_PARSER);
                            prepare_jpeg_parser(parser);
                            return parser.decode(surface, options);
                        };
                    }
                    else
                    {
                        decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                        {
                            Buffer entropy;
                            for (size_t i = 0; i < m_context.jpeg_dc_tables.size(); ++i)
                            {
                                u32 offset = m_context.jpeg_dc_tables[i];
                                writeDHT(entropy, 0x00 | u8(i), m_memory.address + offset);
                            }

                            for (size_t i = 0; i < m_context.jpeg_ac_tables.size(); ++i)
                            {
                                u32 offset = m_context.jpeg_ac_tables[i];
                                writeDHT(entropy, 0x10 | u8(i), m_memory.address + offset);
                            }

                            // Strip/tile data often includes its own SOS; don't duplicate it.
                            const bool strip_has_sos = memory.size >= 2 &&
                                bigEndian::uload16(memory.address) == jpeg::MARKER_SOS;
                            if (!strip_has_sos)
                            {
                                writeSOS(entropy);
                            }
                            entropy.append(memory);

                            ImageDecodeInterface tempInterface;
                            jpeg::Parser parser(&tempInterface, jif_memory, jpeg::Parser::RELAXED_PARSER);
                            prepare_jpeg_parser(parser);
                            parser.setMemory(entropy);
                            return parser.decode(surface, options);
                        };
                    }
                }
                else
                {
                    // Mode B: no JIF; rebuild JPEG from TIFF JPEG tags per strip/tile.
                    u32 h_sub;
                    u32 v_sub;
                    getYCbCrSubsampling(h_sub, v_sub);

                    decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                    {
                        Buffer buffer;

                        u8* soi = buffer.append(2);
                        bigEndian::ustore16(soi, jpeg::MARKER_SOI);

                        writeSOF1(buffer, surface.width, surface.height, h_sub, v_sub);

                        for (size_t i = 0; i < m_context.jpeg_qt_tables.size(); ++i)
                        {
                            u32 offset = m_context.jpeg_qt_tables[i];
                            writeDQT(buffer, u8(i), m_memory.address + offset);
                        }

                        for (size_t i = 0; i < m_context.jpeg_dc_tables.size(); ++i)
                        {
                            u32 offset = m_context.jpeg_dc_tables[i];
                            writeDHT(buffer, 0x00 | u8(i), m_memory.address + offset);
                        }

                        for (size_t i = 0; i < m_context.jpeg_ac_tables.size(); ++i)
                        {
                            u32 offset = m_context.jpeg_ac_tables[i];
                            writeDHT(buffer, 0x10 | u8(i), m_memory.address + offset);
                        }

                        const bool strip_has_sos = memory.size >= 2 &&
                            bigEndian::uload16(memory.address) == jpeg::MARKER_SOS;
                        if (!strip_has_sos)
                        {
                            writeSOS(buffer);
                        }
                        buffer.append(memory);

                        const bool strip_has_eoi = memory.size >= 2 &&
                            bigEndian::uload16(memory.address + memory.size - 2) == jpeg::MARKER_EOI;
                        if (!strip_has_eoi)
                        {
                            u8* eoi = buffer.append(2);
                            bigEndian::ustore16(eoi, jpeg::MARKER_EOI);
                        }

                        ImageDecodeInterface tempInterface;
                        jpeg::Parser parser(&tempInterface, buffer, jpeg::Parser::RELAXED_PARSER);
                        prepare_jpeg_parser(parser);
                        return parser.decode(surface, options);
                    };
                }
            }
            else
            {
                // compression = 7: JPEG
                if (m_context.jpeg_tables.size > 0)
                {
                    decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                    {
                        ImageDecodeInterface tempInterface;

                        Buffer bitstream_buffer;
                        if (merge_jpeg_with_tables(memory, m_context.jpeg_tables, bitstream_buffer))
                        {
                            ConstMemory bitstream(bitstream_buffer);
                            jpeg::Parser parser(&tempInterface, bitstream, jpeg::Parser::RELAXED_PARSER);
                            prepare_jpeg_parser(parser);
                            return parser.decode(surface, options);
                        }

                        // Strip is a complete JPEG and already carries its own tables.
                        if (is_complete_jpeg(memory))
                        {
                            jpeg::Parser parser(&tempInterface, memory, jpeg::Parser::RELAXED_PARSER);
                            prepare_jpeg_parser(parser);
                            return parser.decode(surface, options);
                        }

                        jpeg::Parser parser(&tempInterface, m_context.jpeg_tables, jpeg::Parser::RELAXED_PARSER);
                        prepare_jpeg_parser(parser);
                        parser.setMemory(memory);
                        return parser.decode(surface, options);
                    };
                }
                else if (m_context.jpeg_interchange_format)
                {
                    // Lossless / complete-JIF compression=7 files often omit JPEGTables.
                    u32 block_offset = 0;
                    u32 block_bytes = 0;
                    if (!m_context.tile_offsets.empty())
                    {
                        block_offset = u32(m_context.tile_offsets[0]);
                        block_bytes = u32(m_context.tile_byte_counts[0]);
                    }
                    else if (!m_context.strip_offsets.empty())
                    {
                        block_offset = u32(m_context.strip_offsets[0]);
                        block_bytes = u32(m_context.strip_byte_counts[0]);
                    }

                    u32 header_length = m_context.jpeg_interchange_format_length;
                    if (!header_length)
                    {
                        header_length = resolve_jpeg_interchange_length(
                            m_memory, m_context.jpeg_interchange_format, 0, block_offset);
                    }

                    const u8* header_ptr = m_memory.address + m_context.jpeg_interchange_format;
                    const bool header_is_complete = header_length >= 2 &&
                        header_ptr[header_length - 2] == 0xff &&
                        header_ptr[header_length - 1] == 0xd9;

                    ConstMemory jif_memory(m_memory.address + m_context.jpeg_interchange_format, header_length);

                    if (header_is_complete)
                    {
                        decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                        {
                            MANGO_UNREFERENCED(memory);

                            ImageDecodeInterface tempInterface;
                            jpeg::Parser parser(&tempInterface, jif_memory, jpeg::Parser::RELAXED_PARSER);
                            prepare_jpeg_parser(parser);
                            return parser.decode(surface, options);
                        };
                    }
                    else if (block_offset == m_context.jpeg_interchange_format + header_length)
                    {
                        decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                        {
                            Buffer buffer;
                            buffer.append(jif_memory);
                            buffer.append(memory);

                            if (memory.size < 2 ||
                                bigEndian::uload16(memory.address + memory.size - 2) != jpeg::MARKER_EOI)
                            {
                                u8* eoi = buffer.append(2);
                                bigEndian::ustore16(eoi, jpeg::MARKER_EOI);
                            }

                            ImageDecodeInterface tempInterface;
                            jpeg::Parser parser(&tempInterface, buffer, jpeg::Parser::RELAXED_PARSER);
                            prepare_jpeg_parser(parser);
                            return parser.decode(surface, options);
                        };
                    }
                    else
                    {
                        decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                        {
                            Buffer buffer;
                            buffer.append(jif_memory);
                            buffer.append(memory);

                            if (memory.size < 2 ||
                                bigEndian::uload16(memory.address + memory.size - 2) != jpeg::MARKER_EOI)
                            {
                                u8* eoi = buffer.append(2);
                                bigEndian::ustore16(eoi, jpeg::MARKER_EOI);
                            }

                            ImageDecodeInterface tempInterface;
                            jpeg::Parser parser(&tempInterface, buffer, jpeg::Parser::RELAXED_PARSER);
                            prepare_jpeg_parser(parser);
                            return parser.decode(surface, options);
                        };
                    }
                }
                else
                {
                    printLine(Print::Error, "JPEGTables not found for Compression=7");
                    return false;
                }
            }

            // Check if using tiles or strips
            bool use_tiles = !m_context.tile_offsets.empty() || is_tiled_via_strips();
            size_t num_data_blocks = use_tiles
                ? (m_context.tile_offsets.empty() ? m_context.strip_offsets.size() : m_context.tile_offsets.size())
                : m_context.strip_offsets.size();

            printLine(Print::Info, "  Processing {} {}", num_data_blocks, use_tiles ? "tiles" : "strips");

            if (use_tiles)
            {
                // Handle tiled image
                u32 tiles_across = (header.width + m_context.tile_width - 1) / m_context.tile_width;
                u32 tiles_down = (header.height + m_context.tile_length - 1) / m_context.tile_length;

                printLine(Print::Info, "  Tile grid: {}x{} ({}x{} pixels per tile)", tiles_across, tiles_down, m_context.tile_width, m_context.tile_length);

                for (size_t i = 0; i < num_data_blocks; ++i)
                {
                    u32 tile_x = (u32(i) % tiles_across) * m_context.tile_width;
                    u32 tile_y = (u32(i) / tiles_across) * m_context.tile_length;
                    
                    u32 tile_w = std::min(m_context.tile_width, header.width - tile_x);
                    u32 tile_h = std::min(m_context.tile_length, header.height - tile_y);

                    const u8* tile_data = m_memory.address + tile_data_offset(i);
                    u32 tile_bytes = u32(tile_data_bytes(i));

                    printLine(Print::Info, "    Tile {}: {}x{} at ({},{}) - {} bytes", i, tile_w, tile_h, tile_x, tile_y, tile_bytes);

                    Surface surface(target, tile_x, tile_y, tile_w, tile_h);

                    ImageDecodeStatus status = decodeJPEG(ConstMemory(tile_data, tile_bytes), surface);
                    if (!status)
                    {
                        printLine(Print::Error, "JPEG decode failed for tile {}: {}", i, status.info);
                        return false;
                    }
                }
            }
            else
            {
                // Handle strip image
                u32 y = 0;

                for (size_t i = 0; i < num_data_blocks; ++i)
                {
                    u32 strip_height = std::min(m_context.rows_per_strip, header.height - y);
                    if (!m_context.rows_per_strip)
                    {
                        // assume the strip is the entire image
                        strip_height = header.height;// - y;
                    }

                    const u8* strip_data = m_memory.address + m_context.strip_offsets[i];
                    u32 strip_bytes = m_context.strip_byte_counts[i];

                    Surface surface(target, 0, y, header.width, strip_height);

                    ImageDecodeStatus status = decodeJPEG(ConstMemory(strip_data, strip_bytes), surface);
                    if (!status)
                    {
                        printLine(Print::Error, "JPEG decode failed for strip {}: {}", i, status.info);
                        return false;
                    }

                    y += strip_height;
                }
            }

            return true;
        }

        void shrinkPixels(Memory dest, ConstMemory src, int width, int height, int source_bits, int target_bits)
        {
            assert(target_bits == 16);

            u32 channels = m_context.samples_per_pixel;
            if (m_context.planar_configuration == 2)
            {
                channels = 1;
            }

            // Precompute bytes per scanline (source)
            u32 bits_per_scanline = width * source_bits * channels;
            u32 bytes_per_scanline = div_ceil(bits_per_scanline, 8);

            u32 dest_stride = width * channels * (target_bits / 8);

            if (source_bits == 24)
            {
                // TIFF multi-byte samples use the file byte order (II / MM).
                // Convert each 24-bit unsigned sample to 16-bit with the same scaling
                // convention as expandPixels (full range 0..2^24-1 -> 0..65535).
                const u64 max_src = (1u << 24) - 1;

                for (int y = 0; y < height; ++y)
                {
                    const u8* s = src.address;
                    u8* d = dest.address;

                    for (int x = 0; x < width; ++x)
                    {
                        for (u32 c = 0; c < channels; ++c)
                        {
                            u32 v;
                            if (m_is_little_endian)
                            {
                                v = u32(s[0]) | (u32(s[1]) << 8) | (u32(s[2]) << 16);
                            }
                            else
                            {
                                v = (u32(s[0]) << 16) | (u32(s[1]) << 8) | u32(s[2]);
                            }

                            u16 out = u16((u64(v) * 65535u) / max_src);
                            mango::littleEndian::ustore16(d, out);
                            s += 3;
                            d += 2;
                        }
                    }

                    src.address += bytes_per_scanline;
                    src.size -= bytes_per_scanline;
                    dest.address += dest_stride;
                }

                return;
            }

            if (source_bits == 32)
            {
                // Unsigned 32-bit sample in file byte order -> 16-bit (full range, matches expand inverse).
                const u64 max_src = (1ull << 32) - 1;

                for (int y = 0; y < height; ++y)
                {
                    const u8* s = src.address;
                    u8* d = dest.address;

                    for (int x = 0; x < width; ++x)
                    {
                        for (u32 c = 0; c < channels; ++c)
                        {
                            u32 v;
                            if (m_is_little_endian)
                            {
                                v = u32(s[0]) | (u32(s[1]) << 8) | (u32(s[2]) << 16) | (u32(s[3]) << 24);
                            }
                            else
                            {
                                v = (u32(s[0]) << 24) | (u32(s[1]) << 16) | (u32(s[2]) << 8) | u32(s[3]);
                            }

                            u16 out = u16((u64(v) * 65535ull) / max_src);
                            mango::littleEndian::ustore16(d, out);
                            s += 4;
                            d += 2;
                        }
                    }

                    src.address += bytes_per_scanline;
                    src.size -= bytes_per_scanline;
                    dest.address += dest_stride;
                }

                return;
            }

            printLine(Print::Warning, "[TIFF] shrinkPixels: unsupported source_bits {} -> {}", source_bits, target_bits);
        }

        void expandPixels(Memory dest, ConstMemory src, int width, int height, int source_bits, int target_bits)
        {
            assert(target_bits == 8 || target_bits == 16);
            //printLine(Print::Info, "  expandPixels()\n    source_bits: {}, target_bits: {}", source_bits, target_bits);

            if (source_bits > target_bits)
            {
                shrinkPixels(dest, src, width, height, source_bits, target_bits);
                return;
            }

            u32 max_source = (1 << source_bits) - 1;
            u32 max_target = (1 << target_bits) - 1;

            u32 channels = m_context.samples_per_pixel;
            if (m_context.planar_configuration == 2)
            {
                channels = 1;
            }

            // Precompute bytes per scanline
            u32 bits_per_scanline = width * source_bits * channels;
            u32 bytes_per_scanline = div_ceil(bits_per_scanline, 8);

            // Precompute destination stride
            u32 dest_stride = width * channels * (target_bits / 8);

            //printLine(Print::Info, "    input stride: {}", bytes_per_scanline);
            //printLine(Print::Info, "    output stride: {}", dest_stride);

            // MANGO TODO: optimize
            // - we reverse bits so that extraction loop is more efficient
            // - The bits are extracted in reverse order, so we need to reverse them back to the original order
            Buffer temp(src.size);
            u8_reverse_bits(temp, src);
            src = temp;

            u8* dest_ptr = dest.address;

            const int x1 = width * channels;

            for (int y = 0; y < height; ++y)
            {
                DataRegister dataRegister(src);

                for (int x = 0; x < x1; ++x)
                {
                    dataRegister.ensureBits(source_bits);
                    u32 sample = dataRegister.getBits(source_bits);
                    dataRegister.consumeBits(source_bits);
                    sample = u32_reverse_bits(sample) >> (32 - source_bits);

                    // Expand and write output
                    if (PhotometricInterpretation(m_context.photometric) == PhotometricInterpretation::PALETTE)
                    {
                        if (target_bits == 8)
                        {
                            dest_ptr[x] = u8(sample);
                        }
                        else
                        {
                            ustore16(dest_ptr + x * 2, u16(sample));
                        }
                    }
                    else if (target_bits == 8)
                    {
                        dest_ptr[x] = u8(sample * max_target / max_source);
                    }
                    else
                    {
                        u16 value = u16(sample * max_target / max_source);
                        ustore16(dest_ptr + x * 2, value);
                    }
                }

                src.address += bytes_per_scanline;
                src.size -= bytes_per_scanline;
                dest_ptr += dest_stride;
            }
        }

        void resolve_wide_palette_chunky_row(u8* rgba_row, const u8* index_row, int width) const
        {
            const u32 nstride = m_context.colormap_entry_count;
            const u8* cmap = m_context.colormap_in_file.address;
            const size_t cmap_size = m_context.colormap_in_file.size;

            auto read_cmap_short = [cmap, cmap_size, nstride](u32 plane, u32 idx, bool is_little_endian) -> u16
            {
                const u64 offset = u64(plane) * u64(nstride) + u64(idx);
                if (offset * 2 + 2 > cmap_size)
                {
                    return 0;
                }

                return is_little_endian ? littleEndian::uload16(cmap + offset * 2)
                                        : bigEndian::uload16(cmap + offset * 2);
            };

            for (int x = 0; x < width; ++x)
            {
                u32 idx = u32(uload16(index_row + x * 2));
                if (nstride)
                {
                    idx = std::min(idx, u32(nstride - 1));
                }

                rgba_row[x * 4 + 0] = u8(read_cmap_short(0, idx, m_is_little_endian) >> 8);
                rgba_row[x * 4 + 1] = u8(read_cmap_short(1, idx, m_is_little_endian) >> 8);
                rgba_row[x * 4 + 2] = u8(read_cmap_short(2, idx, m_is_little_endian) >> 8);
                rgba_row[x * 4 + 3] = 0xff;
            }
        }

        // Decode one PixarLog strip/tile directly into the (float) target.
        void pixarlog_decompress(ImageDecodeStatus& status, Surface target, ConstMemory memory, int width, int height)
        {
            if (m_context.planar_configuration != 1)
            {
                status.setError("[ImageDecoder.TIFF] PixarLog planar configuration is not supported.");
                return;
            }

            const int stride = int(m_context.samples_per_pixel);
            if (stride < 1 || stride > 4)
            {
                status.setError("[ImageDecoder.TIFF] PixarLog unsupported channel count: {}", stride);
                return;
            }

            const size_t nsamples = size_t(stride) * width * height;

            // +stride samples of slop
            Buffer raw((nsamples + stride) * sizeof(u16), 0);

            CompressionStatus cs = zlib::decompress(raw, memory);
            if (!cs.success)
            {
                status.setError("[ImageDecoder.TIFF] PixarLog zlib decompression failed: {}", cs.info);
                return;
            }

            u16* up = reinterpret_cast<u16*>(raw.data());

            // The codec stores the tokens in the file byte order.
            if (m_is_little_endian != cpu::isLittleEndian())
            {
                byteswap(up, nsamples);
            }

            const float* table = pixarLogToLinearTable();
            const int llen = stride * width;

            std::vector<float> row(llen);

            for (int y = 0; y < height; ++y)
            {
                pixarLogAccumulateRow(up + size_t(y) * llen, llen, stride, row.data(), table);
                std::memcpy(target.image, row.data(), size_t(llen) * sizeof(float));
                target.image += target.stride;
            }
        }

        // Decode one SGI LogLuv / LogL strip/tile directly into the (float) target.
        void logluv_decompress(ImageDecodeStatus& status, Surface target, ConstMemory memory, int width, int height)
        {
            const bool is_grayscale = (m_context.photometric == u32(PhotometricInterpretation::LOGL));
            const bool is_24bit = (m_context.compression == u32(Compression::SGILOG24));

            const u8* bp = memory.address;
            size_t cc = memory.size;

            std::vector<u32> tokens(width);

            for (int y = 0; y < height; ++y)
            {
                if (is_grayscale)
                {
                    // 16-bit log luminance, two byte planes.
                    if (!logLuvDecodeRLE(tokens.data(), width, bp, cc, 2))
                    {
                        status.setError("[ImageDecoder.TIFF] LogL truncated row {}.", y);
                        return;
                    }

                    float* dest = target.address<float>(0, y);
                    for (int x = 0; x < width; ++x)
                    {
                        double Y = logLuvL16toY(s16(tokens[x]));
                        dest[x] = float(Y > 0.0 ? Y : 0.0);
                    }
                }
                else if (is_24bit)
                {
                    // 24-bit packed, no RLE: 3 bytes per pixel, big-endian order.
                    int i = 0;
                    for (; i < width && cc >= 3; ++i)
                    {
                        tokens[i] = u32(bp[0]) << 16 | u32(bp[1]) << 8 | u32(bp[2]);
                        bp += 3;
                        cc -= 3;
                    }
                    if (i != width)
                    {
                        status.setError("[ImageDecoder.TIFF] LogLuv24 truncated row {}.", y);
                        return;
                    }

                    float* dest = target.address<float>(0, y);
                    for (int x = 0; x < width; ++x)
                    {
                        float xyz[3];
                        logLuv24toXYZ(tokens[x], xyz);
                        logLuvXYZtoLinearRGB(xyz, dest + x * 3);
                    }
                }
                else
                {
                    // 32-bit LogLuv, four byte planes.
                    if (!logLuvDecodeRLE(tokens.data(), width, bp, cc, 4))
                    {
                        status.setError("[ImageDecoder.TIFF] LogLuv32 truncated row {}.", y);
                        return;
                    }

                    float* dest = target.address<float>(0, y);
                    for (int x = 0; x < width; ++x)
                    {
                        float xyz[3];
                        logLuv32toXYZ(tokens[x], xyz);
                        logLuvXYZtoLinearRGB(xyz, dest + x * 3);
                    }
                }
            }
        }

        void decodeRect(ImageDecodeStatus& status, Surface target, ConstMemory memory, int width, int height, u32 channel = 0)
        {
            u32 sample_bits = m_context.sample_bits;
            u32 expanded_sample_bits = round_ceil(m_context.sample_bits, 8); 

            if (!header.format.isFloat())
            {
                expanded_sample_bits = std::min(expanded_sample_bits, 16u); // max 16 bits per channel
            }

            //printLine(Print::Info, "  sample_bits: {}, expanded_sample_bits: {}", sample_bits, expanded_sample_bits);

            u32 bytes_per_row = 0;
            u32 expanded_bytes_per_row = 0;

            const bool chunky_ycbcr =
                m_context.photometric == u32(PhotometricInterpretation::YCBCR) &&
                m_context.planar_configuration == 1 &&
                m_context.samples_per_pixel == 3;

            u32 ycbcr_macro_row_bytes = 0;
            u32 ycbcr_macro_rows = 0;

            if (chunky_ycbcr)
            {
                u32 h_sub;
                u32 v_sub;
                getYCbCrSubsampling(h_sub, v_sub);

                const u32 chroma_w = u32((width + int(h_sub) - 1) / int(h_sub));
                ycbcr_macro_row_bytes = chroma_w * (h_sub * v_sub + 2);
                ycbcr_macro_rows = (u32(height) + v_sub - 1) / v_sub;
                bytes_per_row = ycbcr_macro_row_bytes;
                expanded_bytes_per_row = ycbcr_macro_row_bytes;
            }
            else if (m_context.planar_configuration == 1)
            {
                // Chunky format contains all channels
                bytes_per_row = (width * sample_bits * m_context.samples_per_pixel + 7) / 8;
                expanded_bytes_per_row = (width * expanded_sample_bits * m_context.samples_per_pixel) / 8;
            } 
            else
            {
                // Planar format contains one channel
                bytes_per_row = (width * sample_bits + 7) / 8;
                expanded_bytes_per_row = (width * expanded_sample_bits) / 8;
            }

            u32 uncompressed_bytes = height * bytes_per_row;
            u32 expanded_bytes = height * expanded_bytes_per_row;

            /*
            printLine(Print::Info, "  bytes_per_row: {}, uncompressed_bytes: {}",
                bytes_per_row, uncompressed_bytes);
            printLine(Print::Info, "  expanded_bytes_per_row: {}, expanded_bytes: {}",
                expanded_bytes_per_row, expanded_bytes);
            */

            Buffer buffer(uncompressed_bytes);
            Buffer expanded_buffer;

            bool needs_expansion = sample_bits != expanded_sample_bits;
            if (needs_expansion)
            {
                expanded_buffer.resize(expanded_bytes);
            }

            switch (Compression(m_context.compression))
            {
                case Compression::NONE:
                {
                    if (needs_expansion)
                    {
                        expandPixels(expanded_buffer, memory, width, height, sample_bits, expanded_sample_bits);
                        memory = expanded_buffer;
                        needs_expansion = false;
                    }
                    else
                    {
                        std::memcpy(buffer, memory.address, uncompressed_bytes);
                        memory = buffer;
                    }

                    break;
                }

                case Compression::CCITT_RLE:
                case Compression::CCITT_RLE_W:
                {
                    bool word_aligned = (Compression(m_context.compression) == Compression::CCITT_RLE_W);
                    bool success = true;

                    if (m_context.fill_order == u16(FillOrder::MSB2LSB))
                    {
                        Buffer temp(memory.size);
                        u8_reverse_bits(temp, memory);
                        success = ccitt_rle_decompress(expanded_buffer, temp, width, height, word_aligned);
                    }
                    else
                    {
                        success = ccitt_rle_decompress(expanded_buffer, memory, width, height, word_aligned);
                    }

                    if (!success)
                    {
                        printLine(Print::Error, "[CCITT-RLE{}] Decompression failed", word_aligned ? "_W" : "");
                        return;
                    }

                    memory = expanded_buffer;
                    needs_expansion = false; // CCITT already expands to 8-bit
                    break;
                }

                case Compression::CCITT_GROUP3:
                {
                    bool is_2d = (m_context.group3_options & 0x00000001) != 0;
                    bool success = true;

                    if (m_context.fill_order == u16(FillOrder::MSB2LSB))
                    {
                        Buffer temp(memory.size);
                        u8_reverse_bits(temp, memory);
                        success = ccitt_group3_decompress(expanded_buffer, temp, width, height, is_2d);
                        if (!success)
                        {
                            success = ccitt_rle_decompress(expanded_buffer, temp, width, height, false);
                        }
                    }
                    else
                    {
                        success = ccitt_group3_decompress(expanded_buffer, memory, width, height, is_2d);
                        if (!success)
                        {
                            success = ccitt_rle_decompress(expanded_buffer, memory, width, height, false);
                        }
                    }

                    if (!success)
                    {
                        printLine(Print::Error, "[CCITT_GROUP3] Decompression failed");
                        return;
                    }

                    memory = expanded_buffer;
                    needs_expansion = false;
                    break;
                }

                case Compression::CCITT_GROUP4:
                {
                    bool success = true;

                    if (m_context.fill_order == u16(FillOrder::MSB2LSB))
                    {
                        Buffer temp(memory.size);
                        u8_reverse_bits(temp, memory);
                        success = ccitt_group4_decompress(expanded_buffer, temp, width, height);
                    }
                    else
                    {
                        success = ccitt_group4_decompress(expanded_buffer, memory, width, height);
                    }

                    if (!success)
                    {
                        printLine(Print::Error, "[CCITT_GROUP4] Decompression failed");
                        return;
                    }

                    memory = expanded_buffer;
                    needs_expansion = false;
                    break;
                }

                case Compression::LZW:
                {
                    std::memset(buffer, 0, uncompressed_bytes);
                    bool success = lzw_decompress(buffer, memory);
                    if (!success)
                    {
                        status.setError("[LZW] Decompression failed.");
                        return;
                    }

                    memory = buffer;
                    break;
                }

                case Compression::JPEG_LEGACY:
                case Compression::JPEG_MODERN:
                {
                    // JPEG handled elsewhere via stream reconstruction
                    return;
                }

                case Compression::ZIP:
                    zlib::decompress(buffer, memory);
                    memory = buffer;
                    break;

                case Compression::PACKBITS:
                {
                    bool success = true;

                    if (m_context.fill_order != u16(FillOrder::MSB2LSB))
                    {
                        Buffer temp(memory.size);
                        u8_reverse_bits(temp, memory);
                        success = packbits_decompress(buffer, temp);
                    }
                    else
                    {
                        success = packbits_decompress(buffer, memory);
                    }

                    if (!success)
                    {
                        printLine(Print::Error, "[PackBits] Decompression failed");
                        return;
                    }

                    memory = buffer;
                    break;
                }

                case Compression::PIXARLOG:
                    pixarlog_decompress(status, target, memory, width, height);
                    return;

                case Compression::SGILOG:
                case Compression::SGILOG24:
                    logluv_decompress(status, target, memory, width, height);
                    return;

                case Compression::DEFLATE:
                    printLine(Print::Info, "    Unsupported compression: {}", m_context.compression);
                    return;

                case Compression::WEBP:
                {
                    webp_decompress(status, target, memory);
                    return;
                }

                default:
                    printLine(Print::Info, "    Unknown compression: {}", m_context.compression);
                    //status.setError("Unknown compression.");
                    //return status;
                    return;
            }

            // Post-decompression expansion if needed
            if (needs_expansion)
            {
                expandPixels(expanded_buffer, memory, width, height, sample_bits, expanded_sample_bits);
                memory = expanded_buffer;
            }

            if (m_is_little_endian != cpu::isLittleEndian())
            {
                // shrinkPixels() / expand already emits host-endian u16 when narrowing (e.g. 24 or 32 -> 16).
                // Do not run tag-depth u32/16 swap on narrowed buffers (would misinterpret width-16 data).
                const bool packed_at_tag_bits = (sample_bits == expanded_sample_bits);

                if (!needs_expansion && packed_at_tag_bits)
                {
                    switch (m_context.sample_bits)
                    {
                        case 16:
                            byteswap(reinterpret_cast<u16*>(const_cast<u8*>(memory.address)), memory.size / 2);
                            break;
                        case 32:
                            byteswap(reinterpret_cast<u32*>(const_cast<u8*>(memory.address)), memory.size / 4);
                            break;
                        case 64:
                            byteswap(reinterpret_cast<u64*>(const_cast<u8*>(memory.address)), memory.size / 8);
                            break;
                    }
                }
            }

            // Apply PhotometricInterpretation color inversion for grayscale images
            if (m_context.photometric == 0 && m_context.samples_per_pixel == 1)
            {
                u8* data = const_cast<u8*>(memory.address);
                for (u32 i = 0; i < memory.size; ++i)
                {
                    data[i] = 255 - data[i];
                }
            }

            // RGB can be decoded directly, other formats need to be resolved.
            // RGBA (4 samples) used to force the u8 repack path below — wrong for float/half
            // where scanline bytes must be copied as-is into the destination surface.
            // CIELAB is stored as 3 (or 3+N) byte chunky samples; never blit it directly into RGBA.
            const bool is_cielab =
                m_context.photometric == u32(PhotometricInterpretation::CIELAB) ||
                m_context.photometric == u32(PhotometricInterpretation::ICCLAB) ||
                m_context.photometric == u32(PhotometricInterpretation::ITULAB);

            bool is_direct = m_context.samples_per_pixel < 4 && !is_cielab;
            if (m_context.planar_configuration == 1 &&
                m_context.photometric == u32(PhotometricInterpretation::RGB) &&
                !m_context.sample_format.empty() &&
                SampleFormat(m_context.sample_format[0]) == SampleFormat::FLOAT)
            {
                is_direct = true; // chunky RGB/RGBA float: blit to target, not u8 repack
            }

            if (m_context.photometric == u32(PhotometricInterpretation::PALETTE) && m_context.sample_bits > 8)
            {
                // Indices are wide (e.g. 16-bit); decode to scratch then resolve to RGBA8 in target.
                is_direct = false;
            }

            // Planar: each decodeRect pass is one plane.
            // resolvePlanarScanline interleaves into a multi-channel layout and must write the surface.
            if (m_context.planar_configuration == 2)
            {
                is_direct = true;
            }

            const bool cielab_inplace =
                !is_direct && is_cielab &&
                m_context.planar_configuration == 1 &&
                expanded_sample_bits <= 8 &&
                m_context.predictor <= 2;

            Buffer scanline;
            if (!cielab_inplace)
            {
                scanline.resize(expanded_bytes_per_row);
            }

            const bool plane_direct =
                m_context.planar_configuration == 2 &&
                m_context.samples_per_pixel > 1 &&
                target.format.bytes() == 1;

            if (plane_direct)
            {
                for (int y = 0; y < height; ++y)
                {
                    u8* dest = target.image + target.stride * y;
                    const u8* src = memory.address + y * expanded_bytes_per_row;

                    if (m_context.predictor == 1)
                    {
                        std::memcpy(dest, src, expanded_bytes_per_row);
                    }
                    else if (m_context.predictor == 2)
                    {
                        dest[0] = src[0];
                        for (u32 i = 1; i < expanded_bytes_per_row; ++i)
                        {
                            dest[i] = u8(src[i] + dest[i - 1]);
                        }
                    }
                    else
                    {
                        resolvePlanarScanline(dest, src, expanded_bytes_per_row,
                            m_context.samples_per_pixel, channel, expanded_sample_bits);
                    }
                }
                return;
            }

            if (chunky_ycbcr)
            {
                for (u32 mr = 0; mr < ycbcr_macro_rows; ++mr)
                {
                    u8* dest = target.image + target.stride * mr;
                    const u8* src = memory.address + mr * ycbcr_macro_row_bytes;

                    if (m_context.predictor == 1)
                    {
                        std::memcpy(dest, src, ycbcr_macro_row_bytes);
                    }
                    else
                    {
                        resolveChunkyScanline(dest, src, ycbcr_macro_row_bytes, m_context.samples_per_pixel);
                    }
                }
                return;
            }

            for (int y = 0; y < height; ++y)
            {
                u8* dest = is_direct ? target.image
                    : cielab_inplace ? const_cast<u8*>(memory.address)
                    : scanline.data();

                if (m_context.planar_configuration == 1)
                {
                    resolveChunkyScanline(dest, memory.address, expanded_bytes_per_row, m_context.samples_per_pixel);
                }
                else
                {
                    resolvePlanarScanline(dest, memory.address, expanded_bytes_per_row,
                        m_context.samples_per_pixel, channel, expanded_sample_bits);
                }

                if (!is_direct)
                {
                    if (is_cielab)
                    {
                        const u32 lab_channels = m_context.samples_per_pixel - u32(m_context.extra_samples.size());
                        const u32 chunky_bpp = (m_context.samples_per_pixel * expanded_sample_bits) / 8;
                        const u8* row = cielab_inplace ? memory.address : scanline.data();

                        float X0;
                        float Y0;
                        float Z0;
                        tiff_cielab_ref_white(m_context, X0, Y0, Z0);

                        size_t base = 0;

                        if (expanded_sample_bits <= 8)
                        {
                            for (int x = 0; x < width; ++x)
                            {
                                float X;
                                float Y;
                                float Z;
                                tiff_cielab8_to_xyz(row[base + 0], s8(row[base + 1]), s8(row[base + 2]),
                                    X0, Y0, Z0, X, Y, Z);
                                int a = 0xff;

                                if (lab_channels < m_context.samples_per_pixel)
                                {
                                    a = row[base + lab_channels];
                                }
                                else if (m_context.associated_alpha)
                                {
                                    a = row[base + m_context.associated_alpha_index];
                                }

                                u8 r;
                                u8 g;
                                u8 bv;
                                tiff_xyz_to_display_rgb(X, Y, Z, r, g, bv);

                                target.image[x * 4 + 0] = r;
                                target.image[x * 4 + 1] = g;
                                target.image[x * 4 + 2] = bv;
                                target.image[x * 4 + 3] = u8(a);

                                base += chunky_bpp;
                            }
                        }
                        else
                        {
                            for (int x = 0; x < width; ++x)
                            {
                                const u8* p = row + base;

                                float X;
                                float Y;
                                float Z;
                                tiff_cielab16_to_xyz(uload16(p + 0), s16(uload16(p + 2)), s16(uload16(p + 4)),
                                    X0, Y0, Z0, X, Y, Z);
                                int a = 0xff;

                                if (lab_channels < m_context.samples_per_pixel)
                                {
                                    a = int(uload16(p + lab_channels * 2) >> 8);
                                }
                                else if (m_context.associated_alpha)
                                {
                                    a = int(uload16(p + m_context.associated_alpha_index * 2) >> 8);
                                }

                                u8 r;
                                u8 g;
                                u8 bv;
                                tiff_xyz_to_display_rgb(X, Y, Z, r, g, bv);

                                target.image[x * 4 + 0] = r;
                                target.image[x * 4 + 1] = g;
                                target.image[x * 4 + 2] = bv;
                                target.image[x * 4 + 3] = u8(a);

                                base += chunky_bpp;
                            }
                        }
                    }
                    else if (m_context.photometric == u32(PhotometricInterpretation::RGB))
                    {
                        const u32 chunky_bpp_rgb = (m_context.samples_per_pixel * expanded_sample_bits) / 8;
                        size_t base = 0;
                        for (int x = 0; x < width; ++x)
                        {
                            int r;
                            int g;
                            int b;
                            int a = 0xff;

                            if (expanded_sample_bits <= 8)
                            {
                                r = scanline[base + 0];
                                g = scanline[base + 1];
                                b = scanline[base + 2];
                                if (m_context.associated_alpha)
                                {
                                    a = scanline[base + m_context.associated_alpha_index];
                                }
                            }
                            else
                            {
                                const u8* p = scanline.data() + base;
                                r = u8(uload16(p + 0) >> 8);
                                g = u8(uload16(p + 2) >> 8);
                                b = u8(uload16(p + 4) >> 8);
                                if (m_context.associated_alpha)
                                {
                                    a = u8(uload16(p + m_context.associated_alpha_index * 2) >> 8);
                                }
                            }

                            target.image[x * 4 + 0] = u8(r);
                            target.image[x * 4 + 1] = u8(g);
                            target.image[x * 4 + 2] = u8(b);
                            target.image[x * 4 + 3] = u8(a);

                            base += chunky_bpp_rgb;
                        }
                    }
                    else if (m_context.photometric == u32(PhotometricInterpretation::SEPARATED) &&
                             m_context.planar_configuration == 1)
                    {
                        // Chunky CMYK: full CMYK4 scanline (8 or 16 bits per sample after expand).
                        const u8* lookup = math::get_linear_to_srgb_table();
                        const u32 chunky_bpp = (m_context.samples_per_pixel * expanded_sample_bits) / 8;

                        size_t base = 0;

                        if (expanded_sample_bits <= 8)
                        {
                            for (int x = 0; x < width; ++x)
                            {
                                int C = 255 - scanline[base + 0];
                                int M = 255 - scanline[base + 1];
                                int Y = 255 - scanline[base + 2];
                                int K = 255 - scanline[base + 3];

                                int R = (C * K + 127) / 255;
                                int G = (M * K + 127) / 255;
                                int B = (Y * K + 127) / 255;

                                target.image[x * 4 + 0] = lookup[R];
                                target.image[x * 4 + 1] = lookup[G];
                                target.image[x * 4 + 2] = lookup[B];
                                target.image[x * 4 + 3] = 0xff;

                                base += chunky_bpp;
                            }
                        }
                        else
                        {
                            for (int x = 0; x < width; ++x)
                            {
                                const u8* p = scanline.data() + base;

                                const u32 Cin = uload16(p + 0);
                                const u32 Min = uload16(p + 2);
                                const u32 Yin = uload16(p + 4);
                                const u32 Kin = uload16(p + 6);

                                const u32 C = 65535u - Cin;
                                const u32 M = 65535u - Min;
                                const u32 Y = 65535u - Yin;
                                const u32 K = 65535u - Kin;

                                const u32 R = u32((u64(C) * K + 32767u) / 65535u);
                                const u32 G = u32((u64(M) * K + 32767u) / 65535u);
                                const u32 B = u32((u64(Y) * K + 32767u) / 65535u);

                                const u8 r8 = u8((R * 255u + 32767u) / 65535u);
                                const u8 g8 = u8((G * 255u + 32767u) / 65535u);
                                const u8 b8 = u8((B * 255u + 32767u) / 65535u);

                                target.image[x * 4 + 0] = lookup[r8];
                                target.image[x * 4 + 1] = lookup[g8];
                                target.image[x * 4 + 2] = lookup[b8];
                                target.image[x * 4 + 3] = 0xff;

                                base += chunky_bpp;
                            }
                        }
                    }
                    else if (m_context.photometric == u32(PhotometricInterpretation::PALETTE) && m_context.sample_bits > 8)
                    {
                        resolve_wide_palette_chunky_row(target.image, scanline.data(), width);
                    }
                }

                memory.address += expanded_bytes_per_row;
                target.image += target.stride;
            }
        }
        
        void resolveChunkyScanline(u8* output, const u8* input, u32 bytes, u32 channels)
        {
            if (m_context.predictor == 1)
            {
                // chunky, no prediction

                std::memcpy(output, input, bytes);
            }
            else if (m_context.predictor == 2)
            {
                // chunky, horizontal differencing

                std::memcpy(output, input, channels); // copy first sample

                for (u32 x = channels; x < bytes; x += channels)
                {
                    for (u32 c = 0; c < channels; ++c)
                    {
                        output[x + c] = input[x + c] + output[x - channels + c];
                    }
                }
            }
            else if (m_context.predictor == 3)
            {
                // chunky, float differencing

                u32 bytesPerFloat = m_context.sample_bits / 8;
                u32 bytesPerSample = channels * bytesPerFloat;
                u32 width = bytes / bytesPerSample;

                // undo byte difference on input
                u8* data = const_cast<u8*>(input);
                u32 offset = channels;

                const u32 x1 = width * bytesPerFloat;

                for (u32 x = 1; x < x1; ++x)
                {
                    for (u32 c = 0; c < channels; ++c)
                    {
                        data[offset] += data[offset - channels];
                        ++offset;
                    }
                }

                // reorder the semi-BigEndian bytes into the output buffer
                u32 rowIncrement = width * channels;

#ifdef MANGO_BIG_ENDIAN
                for (u32 x = 0; x < rowIncrement; ++x)
                {
                    u32 offset = x;

                    for (u32 BYTE = 0; BYTE < bytesPerFloat; ++BYTE)
                    {
                        output[BYTE] = input[offset];
                        offset += rowIncrement;
                    }

                    output += bytesPerFloat;
                }
#else
                for (u32 x = 0; x < rowIncrement; ++x)
                {
                    u32 offset = (bytesPerFloat - 1) * rowIncrement + x;

                    for (u32 BYTE = 0; BYTE < bytesPerFloat; ++BYTE)
                    {
                        output[BYTE] = input[offset];
                        offset -= rowIncrement;
                    }

                    output += bytesPerFloat;
                }
#endif
            }
        }

        void resolvePlanarScanline(u8* output, const u8* input, u32 bytes, u32 channels, u32 channel,
            u32 storage_bits_per_sample)
        {
            // `storage_bits_per_sample` is the width of each sample in `input` after expand/shrink (decodeRect's
            // expanded_sample_bits). Tag BitsPerSample can be 10–14 while storage is 16 — use storage here.
            // MANGO TODO: Planar and prediction requires prediction before expansion

            if (m_context.predictor == 1)
            {
                // planar, no prediction

                if (storage_bits_per_sample <= 8)
                {
                    u8* dest = output + channel;

                    for (u32 i = 0; i < bytes; ++i)
                    {
                        dest[0] = input[i];
                        dest += channels;
                    }
                }
                else
                {
                    u8* dest = output + channel * 2;

                    for (u32 i = 0; i < bytes; i += 2)
                    {
                        dest[0] = input[i + 0];
                        dest[1] = input[i + 1];
                        dest += channels * 2;
                    }
                }
            }
            else if (m_context.predictor == 2)
            {
                // planar, horizontal differencing

                u8* dest = output + channel;
                u8 prev = 0;

                for (u32 i = 0; i < bytes; ++i)
                {
                    *dest = input[i] + prev;
                    prev = *dest;
                    dest += channels;
                }
            }
            else if (m_context.predictor == 3)
            {
                // planar, float differencing

                /* MANGO TODO: implement, need test image
                */
            }
        }
    };

    ImageDecodeInterface* createInterface(ConstMemory memory)
    {
        ImageDecodeInterface* x = new Interface(memory);
        return x;
    }

} // namespace

namespace mango::image
{

    void registerImageCodecTIFF()
    {
        registerImageDecoder(createInterface, ".tiff");
        registerImageDecoder(createInterface, ".tif");
    }

} // namespace mango::image
