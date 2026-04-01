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
        PACKBITS = 32773,
        DEFLATE = 32946,
        SGILOG = 34676,
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
        LOGLUV = 32845,
        LINEAR_RAW = 34892,
    };

    struct IFDContext
    {
        u32 width = 0;
        u32 height = 0;
        u16 compression = 1;
        u16 photometric = 2;
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
        bool associated_alpha = true;
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
                // TODO: parsing failure
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
    void parse_ifd(IFDContext& context, ConstMemory memory, Pointer p, bool is_big_tiff)
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
            TIFF_CASE_UNSIGNED(PhotometricInterpretation, photometric);
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
                //context.reference_black_white = getRational(p, memory, type, is_big_tiff);
                //printLine(Print::Info, "    [ReferenceBlackWhite]");
                //printLine(Print::Info, "      value: {}", context.reference_black_white);
                break;

            TIFF_CASE_UNSIGNED(PlanarConfiguration, planar_configuration);
            TIFF_CASE_UNSIGNED(Predictor, predictor);

            case Tag::Software:
                context.software = getAscii(p, memory, type, is_big_tiff);
                break;

            case Tag::ColorMap:
            {
                printLine(Print::Info, "    [ColorMap]");

                const int bitsPerSample = context.bits_per_sample[0];
                if (bitsPerSample != 4 && bitsPerSample != 8)
                {
                    printLine(Print::Error, "      Incorrect bits per sample: {}.", bitsPerSample);
                    break;
                }

                const u32 count = 1 << bitsPerSample;
                printLine(Print::Info, "      values: {}", count);

                context.palette.size = count;
                std::vector<u64> values = getUnsignedArray(p, memory, type, count * 3, is_big_tiff);

                for (u32 i = 0; i < count; ++i)
                {
                    u32 r = values[i + count * 0] >> 8;
                    u32 g = values[i + count * 1] >> 8;
                    u32 b = values[i + count * 2] >> 8;
                    context.palette[i] = Color(r, g, b, 0xff);
                }
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
                        // found associated alpha in extra samples
                        context.associated_alpha = true;
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

        s32 data = 0;
        s32 data_bits = 0;
        s32 codesize = 9;
        s32 codemask = (1 << codesize) - 1;

        auto GetNextCode = [&]() -> int
        {
            // MSB-first: check transitions before reading (original TIFF style)
            if (is_msb_first)
            {
                if ((next_table_entry == 511 && codesize == 9) ||
                    (next_table_entry == 1023 && codesize == 10) ||
                    (next_table_entry == 2047 && codesize == 11))
                {
                    codesize++;
                    codemask = (1 << codesize) - 1;
                }
            }

            // Fill bit buffer (adaptive: MSB-first or LSB-first)
            while (data_bits < codesize && src_ptr < src_end)
            {
                s32 byte_val = *src_ptr++;
                if (!is_msb_first)
                {
                    data = data | (byte_val << data_bits);  // LSB-first: new bits at top
                }
                else
                {
                    data = (data << 8) | byte_val;  // MSB-first: standard TIFF
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
                    parse_ifd(m_context, m_memory, LittleEndianConstPointer(p), m_header.is_big_tiff);
                }
                else
                {
                    parse_ifd(m_context, m_memory, BigEndianConstPointer(p), m_header.is_big_tiff);
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

            // TODO: read ALL IFDs (multiple images)

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

            // Set header info
            header.width = m_context.width;
            header.height = m_context.height;
            header.depth = 0;
            header.levels = 0;
            header.faces = 0;
            header.format = getImageFormat();
            header.compression = TextureCompression::NONE;
            header.premultiplied = m_context.associated_alpha;

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

            // bit-packed formats resolve to at least 8 bits per channel
            //u32 bits = m_context.sample_bits >= 8 ? m_context.sample_bits : 8;
            u32 bits = round_ceil(m_context.sample_bits, 8);

            Format::Type type = Format::UNORM;

            u64 sample_format = u64(SampleFormat::UINT);
            if (!m_context.sample_format.empty())
            {
                // assume all channels have the same sample format
                // TODO: support different sample formats per channel
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
                    return IndexedFormat(8);

                case PhotometricInterpretation::SEPARATED:
                    return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);

                case PhotometricInterpretation::CIELAB:
                    //return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                case PhotometricInterpretation::TRANSPARENCY_MASK:
                case PhotometricInterpretation::YCBCR:
                case PhotometricInterpretation::ICCLAB:
                case PhotometricInterpretation::ITULAB:
                case PhotometricInterpretation::CFA:
                case PhotometricInterpretation::LOGLUV:
                case PhotometricInterpretation::LINEAR_RAW:
                    // TODO
                    header.setError("Unsupported PhotometricInterpretation: {}", m_context.photometric);
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

            DecodeTargetBitmap target(dest, target_width, target_height, header.format, m_context.palette, false);

            if (m_context.compression == u32(Compression::JPEG_LEGACY) ||
                m_context.compression == u32(Compression::JPEG_MODERN))
            {
                if (!decompress_jpeg(target, options, level, depth, face))
                {
                    status.setError("JPEG decoding failed");
                    return status;
                }
            }
            else if (m_context.tile_offsets.size() > 0)
            {
                // tiles

                if (m_context.tile_byte_counts.size() != m_context.tile_offsets.size() ||
                    m_context.tile_byte_counts.empty())
                {
                    // We could try to infer the tiles from the remaining data in the file but we drop broken files.
                    status.setError("Incorrect or missing tile data.");
                    return status;
                }

                const u32 tile_width = m_context.tile_width;
                const u32 tile_length = m_context.tile_length;
                const u32 xtiles = div_ceil(header.width, tile_width);
                const u32 ytiles = div_ceil(header.height, tile_length);

                if (m_context.planar_configuration == 1)
                {
                    // chunky format
    
                    for (size_t i = 0; i < m_context.tile_offsets.size(); ++i)
                    {
                        ConstMemory memory(m_memory.address + m_context.tile_offsets[i], m_context.tile_byte_counts[i]);
    
                        u32 x = (i % xtiles) * tile_width;
                        u32 y = (i / xtiles) * tile_length;
    
                        printLine(Print::Info, "    [Tile] {}, {}", x, y);
                        printLine(Print::Info, "      offset: {}, length: {} bytes", m_context.tile_offsets[i], m_context.tile_byte_counts[i]);
    
                        Surface tile(target, x, y, tile_width, tile_length);
                        decodeRect(status, tile, memory, tile_width, tile_length);
                    }
                }
                else
                {
                    // planar format

                    // TODO: clear the target surface correctly
                    std::memset(target.image, 0, target.stride * header.height);

                    // TODO: verify count == m_context.tile_offsets.size()
                    size_t count = xtiles * ytiles;

                    for (size_t i = 0; i < count; ++i)
                    {
                        u32 x = (i % xtiles) * tile_width;
                        u32 y = (i / xtiles) * tile_length;

                        for (u32 channel = 0; channel < m_context.samples_per_pixel; ++channel)
                        {
                            size_t index = channel * count + i;
                            ConstMemory memory(m_memory.address + m_context.tile_offsets[index], m_context.tile_byte_counts[index]);

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
    
                        y += strip_height;
                    }
                }
                else
                {
                    // planar format

                    // TODO: clear the target surface correctly
                    std::memset(target.image, 0, target.stride * header.height);

                    // Separate planes (PlanarConfiguration 2): all strips for component 0, then all
                    // for component 1, etc. (TIFF 6 §PlanarConfiguration). Same ordering as tiles above
                    // (channel * count + tile). Do not use spatial_strip * spp + channel — that
                    // interleaves strips per region and pairs the wrong plane data with each channel.
                    u32 strips_per_spatial_region = u32(m_context.strip_offsets.size() / m_context.samples_per_pixel);

                    for (size_t spatial_strip = 0; spatial_strip < strips_per_spatial_region; ++spatial_strip)
                    {
                        u32 y = spatial_strip * m_context.rows_per_strip;
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

            target.resolve();

            // Store ICC profile into the ImageDecodeInterface
            icc = m_context.icc_profile;

            return status;
        }

        bool decompress_jpeg(DecodeTargetBitmap& target, ImageDecodeOptions options, int level, int depth, int face)
        {
            if (m_context.photometric == 2)
            {
                options.jpeg_colorspace_rgb = true;
            }

            std::function<ImageDecodeStatus(ConstMemory, Surface)> decodeJPEG;

            if (m_context.compression == u32(Compression::JPEG_LEGACY))
            {
                // compression = 6: JPEG_LEGACY

                auto writeDHT = [] (Buffer& buffer, u8 id, const u8* table)
                {
                    u8 num_codes = 0;

                    for (int j = 0; j < 16; ++j)
                    {
                        num_codes += table[j];
                    }

                    if (num_codes > 200) 
                    {
                        // Skip invalid tables
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

                auto writeSOF1 = [] (Buffer& buffer, int width, int height)
                {
                    u8* p = buffer.append(10);
                    bigEndian::ustore16(p + 0, jpeg::MARKER_SOF1);
                    bigEndian::ustore16(p + 2, 0x11); // length
                    p[4] = 0x08; // Sample precision (8 bits)
                    bigEndian::ustore16(p + 5, height);
                    bigEndian::ustore16(p + 7, width);
                    p[9] = 0x03; // Number of components

                    // Component 1: Y (luminance) - full resolution
                    p = buffer.append(3);
                    p[0] = 0x01; // Component ID
                    p[1] = 0x22; // Sampling factors (2:2) to match YCbCrSubSampling
                    p[2] = 0x00; // Quantization table 0

                    // Component 2: Cb (chrominance) - subsampled by 2:2  
                    p = buffer.append(3);
                    p[0] = 0x02; // Component ID
                    p[1] = 0x11; // Sampling factors (1:1) relative to Y
                    p[2] = 0x01; // Quantization table 1

                    // Component 3: Cr (chrominance) - subsampled by 2:2
                    p = buffer.append(3);
                    p[0] = 0x03; // Component ID
                    p[1] = 0x11; // Sampling factors (1:1) relative to Y  
                    p[2] = 0x02; // Quantization table 2
                };

                auto writeSOS = [] (Buffer& buffer)
                {
                    u8* p = buffer.append(14);
                    bigEndian::ustore16(p + 0, jpeg::MARKER_SOS);
                    bigEndian::ustore16(p + 2, 0x0c); // length
                    p[4] = 0x03; // Number of components

                    // Component 1: Y  
                    p[5] = 0x01; // Component ID
                    p[6] = 0x00; // DC table 0, AC table 0

                    // Component 2: Cb
                    p[7] = 0x02; // Component ID  
                    p[8] = 0x11; // DC table 1, AC table 1

                    // Component 3: Cr
                    p[9] = 0x03; // Component ID
                    p[10] = 0x22; // DC table 2, AC table 2

                    // Scan parameters
                    p[11] = 0x00; // Spectral selection start (0)
                    p[12] = 0x3f; // Spectral selection end (63)  
                    p[13] = 0x00; // Successive approximation
                };

                if (m_context.jpeg_interchange_format != 0)
                {
                    //
                    // Mode A: Copy JPEG from JPEGInterchangeFormat
                    //
                    printLine(Print::Info, "  Using headers from JPEGInterchangeFormat");

                    const u8* header_data = m_memory.address + m_context.jpeg_interchange_format;
                    u32 header_length = m_context.jpeg_interchange_format_length;
                    printLine(Print::Info, "  Header length: {}", header_length);

                    decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                    {
                        Buffer buffer;

                        for (size_t i = 0; i < m_context.jpeg_dc_tables.size(); ++i)
                        {
                            u32 offset = m_context.jpeg_dc_tables[i];
                            writeDHT(buffer, 0x00 | i, m_memory.address + offset);
                        }

                        for (size_t i = 0; i < m_context.jpeg_ac_tables.size(); ++i)
                        {
                            u32 offset = m_context.jpeg_ac_tables[i];
                            writeDHT(buffer, 0x10 | i, m_memory.address + offset);
                        }

                        writeSOS(buffer);
                        buffer.append(memory);

                        ConstMemory jpeg_memory(m_memory.address + m_context.jpeg_interchange_format, m_context.jpeg_interchange_format_length);

                        ImageDecodeInterface tempInterface;
                        jpeg::Parser parser(&tempInterface, jpeg_memory, jpeg::Parser::RELAXED_PARSER);
                        parser.setMemory(buffer);

                        ImageDecodeStatus status = parser.decode(surface, options);
                        return status;
                    };
                }
                else
                {
                    //
                    // Mode B: Reconstruct JPEG from TIFF tags
                    //

                    decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                    {
                        Buffer buffer;

                        // SOI
                        u8* soi = buffer.append(2);
                        bigEndian::ustore16(soi, jpeg::MARKER_SOI);

                        // SOF1 (Extended sequential DCT)
                        writeSOF1(buffer, m_context.width, m_context.height);

                        for (size_t i = 0; i < m_context.jpeg_qt_tables.size(); ++i)
                        {
                            u32 offset = m_context.jpeg_qt_tables[i];
                            writeDQT(buffer, i, m_memory.address + offset);
                        }

                        for (size_t i = 0; i < m_context.jpeg_dc_tables.size(); ++i)
                        {
                            u32 offset = m_context.jpeg_dc_tables[i];
                            writeDHT(buffer, 0x00 | i, m_memory.address + offset);
                        }

                        for (size_t i = 0; i < m_context.jpeg_ac_tables.size(); ++i)
                        {
                            u32 offset = m_context.jpeg_ac_tables[i];
                            writeDHT(buffer, 0x10 | i, m_memory.address + offset);
                        }

                        writeSOS(buffer);

                        buffer.append(memory);

                        // EOI
                        u8* eoi = buffer.append(2);
                        bigEndian::ustore16(eoi, jpeg::MARKER_EOI);

                        ImageDecodeInterface tempInterface;
                        jpeg::Parser parser(&tempInterface, buffer, jpeg::Parser::RELAXED_PARSER);

                        ImageDecodeStatus status = parser.decode(surface, options);
                        return status;
                    };
                }
            }
            else
            {
                // compression = 7: JPEG_MODERN
                if (m_context.jpeg_tables.size == 0)
                {
                    printLine(Print::Error, "JPEGTables not found for Compression=7");
                    return false;
                }

                decodeJPEG = [=, this] (ConstMemory memory, Surface surface) -> ImageDecodeStatus
                {
                    ImageDecodeInterface tempInterface;
                    jpeg::Parser parser(&tempInterface, m_context.jpeg_tables, jpeg::Parser::RELAXED_PARSER);
                    parser.setMemory(memory);

                    ImageDecodeStatus status = parser.decode(surface, options);
                    return status;
                };
            }

            // Check if using tiles or strips
            bool use_tiles = !m_context.tile_offsets.empty();
            size_t num_data_blocks = use_tiles ? m_context.tile_offsets.size() : m_context.strip_offsets.size();

            printLine(Print::Info, "  Processing {} {}", num_data_blocks, use_tiles ? "tiles" : "strips");

            if (use_tiles)
            {
                // Handle tiled image
                u32 tiles_across = (header.width + m_context.tile_width - 1) / m_context.tile_width;
                u32 tiles_down = (header.height + m_context.tile_length - 1) / m_context.tile_length;

                printLine(Print::Info, "  Tile grid: {}x{} ({}x{} pixels per tile)", tiles_across, tiles_down, m_context.tile_width, m_context.tile_length);

                for (size_t i = 0; i < num_data_blocks; ++i)
                {
                    u32 tile_x = (i % tiles_across) * m_context.tile_width;
                    u32 tile_y = (i / tiles_across) * m_context.tile_length;
                    
                    u32 tile_w = std::min(m_context.tile_width, header.width - tile_x);
                    u32 tile_h = std::min(m_context.tile_length, header.height - tile_y);

                    const u8* tile_data = m_memory.address + m_context.tile_offsets[i];
                    u32 tile_bytes = m_context.tile_byte_counts[i];

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

            // TODO: optimize
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
                        dest_ptr[x] = u8(sample);
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

            if (m_context.planar_configuration == 1)
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
                    }
                    else
                    {
                        success = ccitt_group3_decompress(expanded_buffer, memory, width, height, is_2d);
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
                    bool success = lzw_decompress(buffer, memory);
                    if (!success)
                    {
                        printLine(Print::Error, "[LZW] Decompression failed");
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

                case Compression::DEFLATE:
                case Compression::SGILOG:
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
            bool is_direct = m_context.samples_per_pixel < 4;
            if (m_context.planar_configuration == 1 &&
                m_context.photometric == u32(PhotometricInterpretation::RGB) &&
                !m_context.sample_format.empty() &&
                SampleFormat(m_context.sample_format[0]) == SampleFormat::FLOAT)
            {
                is_direct = true; // chunky RGB/RGBA float: blit to target, not u8 repack
            }
            Buffer scanline(expanded_bytes_per_row);

            for (u32 y = 0; y < height; ++y)
            {
                u8* dest = is_direct ? target.image : scanline.data();

                if (m_context.planar_configuration == 1)
                {
                    resolveChunkyScanline(dest, memory.address, expanded_bytes_per_row, m_context.samples_per_pixel);
                }
                else
                {
                    resolvePlanarScanline(dest, memory.address, expanded_bytes_per_row, m_context.samples_per_pixel, channel);
                }

                if (!is_direct)
                {
                    if (m_context.photometric == u32(PhotometricInterpretation::RGB))
                    {
                        size_t base = 0;
                        for (int x = 0; x < width; ++x)
                        {
                            int r = scanline[base + 0];
                            int g = scanline[base + 1];
                            int b = scanline[base + 2];

                            int a = 0xff;
                            if (m_context.associated_alpha)
                            {
                                a = scanline[base + m_context.associated_alpha_index];
                            }
        
                            target.image[x * 4 + 0] = r;
                            target.image[x * 4 + 1] = g;
                            target.image[x * 4 + 2] = b;
                            target.image[x * 4 + 3] = a;

                            base += m_context.samples_per_pixel;
                        }
                    }
                    else if (m_context.photometric == u32(PhotometricInterpretation::SEPARATED))
                    {
                        // TODO: We decode as CMYK, the channel information is in the Ink tags
    
                        const u8* lookup = math::get_linear_to_srgb_table();

                        size_t base = 0;

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

                            base += m_context.samples_per_pixel;
                        }
                    }
                    /*
                    else if (m_context.photometric == u32(PhotometricInterpretation::CIELAB))
                    {
                        for (int x = 0; x < width; ++x)
                        {
                        }
                    }
                    */
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

        void resolvePlanarScanline(u8* output, const u8* input, u32 bytes, u32 channels, u32 channel)
        {
            // TODO: Planar and prediction requires prediction before expansion
            //       Here non-predicted samples can be either 8 or 16 bits

            if (m_context.predictor == 1)
            {
                // planar, no prediction

                if (m_context.sample_bits <= 8)
                {
                    u8* dest = output + channel;

                    for (u32 i = 0; i < bytes; ++i)
                    {
                        dest[0] = input[i];
                        dest += channels;
                    }
                }
                else if (m_context.sample_bits >= 16)
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

                /* TODO: implement, need test image
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
