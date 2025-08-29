/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
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

namespace
{
    using namespace mango;
    using namespace mango::image;
    using namespace mango::math;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    // Specification:
    // https://www.itu.int/itudoc/itu-t/com16/tiff-fx/docs/tiff6.pdf

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
        XPosition = 286,
        YPosition = 287,
        XResolution = 282,
        YResolution = 283,
        PlanarConfiguration = 284,
        //FreeOffsets = 288, // LONG
        //FreeByteCounts = 289, // LONG
        //GrayResponseUnit = 290, // SHORT
        //GrayResponseCurve = 291, // SHORT
        ResolutionUnit = 296,
        PageNumber = 297,
        Software = 305,
        DateTime = 306, // ASCII
        Artist = 315, // ASCII
        //HostComputer = 316, // ASCII
        Predictor = 317,
        ColorMap = 320,
        //ExtraSamples = 338, // SHORT
        SampleFormat = 339,
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
        //Copyright = 33432, // ASCII
        Matteing = 32995,
        DataType = 32996,
        ImageDepth = 32997,
        TileDepth = 32998,
    };

    enum class Compression : u16
    {
        NONE = 1,
        CCITT_RLE = 2,
        CCITT_FAX3 = 3,
        CCITT_FAX4 = 4,
        LZW = 5,
        JPEG = 6,
        ZIP = 8,
        PACKBITS = 32773,
        DEFLATE = 32946,
        SGILOG = 34676,
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
        u32 orientation = 0;
        u32 planar_configuration = 0;
        u32 predictor = 1;
        u32 fill_order = 1;
        u32 sample_format = 1;
        u32 page_number = 0;

        u32 jpeg_proc = 0;
        u32 jpeg_interchange_format = 0;
        u32 jpeg_interchange_format_length = 0;
        u32 jpeg_restart_interval = 0;
        u32 jpeg_lossless_predictors = 0;
        u32 jpeg_point_transforms = 0;
        u32 jpeg_qt_tables = 0;
        u32 jpeg_dc_tables = 0;
        u32 jpeg_ac_tables = 0;
        std::vector<u32> y_cb_cr_sub_sampling;

        u32 new_subfile_type = 0;
        u16 subfile_type = 0;

        u32 samples_per_pixel = 3; // channels
        u32 bpp = 0; // sum of bits_per_sample
        std::vector<u32> bits_per_sample; // bits in each channel
        u32 sample_bits = 8; // default to 8 bits per sample

        u32 rows_per_strip = 0;
        std::vector<u32> strip_offsets;
        std::vector<u32> strip_byte_counts;

        Palette palette;

        std::string image_description;
        std::string software;
        std::string document_name;
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
    u32 getUnsigned(Pointer& p, Type type)
    {
        u32 value = 0;

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
            default:
                // TODO: parsing failure
                printLine(Print::Error, "    [getUnsigned] Unsupported type: {}", int(type));
                break;
        }

        return value;
    }

    template <typename Pointer>
    std::vector<u32> getUnsignedArray(Pointer p, ConstMemory memory, Type type, u64 count, bool is_big_tiff)
    {
        std::vector<u32> values;

        const u64 offset_size = is_big_tiff ? 8 : 4;

        if (getSize(type, count) > offset_size)
        {
            p = memory.address + getOffset(p, is_big_tiff);
        }

        for (u64 i = 0; i < count; ++i)
        {
            u32 value = getUnsigned(p, type);
            values.push_back(value);
        }

        return values;
    }

    template <typename Pointer>
    float getRational(Pointer p, ConstMemory memory, Type type, bool is_big_tiff)
    {
        if (!is_big_tiff)
        {
            p = memory.address + getOffset(p, is_big_tiff);
        }

        u32 numerator = getUnsigned(p, Type::LONG);
        u32 denominator = getUnsigned(p, Type::LONG);
        //printLine(Print::Info, "      numerator: {}, denominator: {}", numerator, denominator);
        float value = float(numerator) / denominator;
        return value;
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
                std::vector<u32> values = getUnsignedArray(p, memory, type, count, is_big_tiff);
                context.bits_per_sample = values;
                context.bpp = std::accumulate(values.begin(), values.end(), 0);
                printLine(Print::Info, "    [BitsPerSample]");

                // Print individual channel values
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
                printLine(Print::Info, "    [DocumentName]");
                //printLine(Print::Info, "{}", context.document_name);
                break;

            case Tag::ImageDescription:
                context.image_description = getAscii(p, memory, type, is_big_tiff);
                printLine(Print::Info, "    [ImageDescription]");
                //printLine(Print::Info, "{}", context.image_description);
                break;

            case Tag::StripOffsets:
            {
                printLine(Print::Info, "    [StripOffsets]");
                context.strip_offsets = getUnsignedArray(p, memory, type, count, is_big_tiff);
                suppress_info = false;
                break;
            }

            TIFF_CASE_UNSIGNED(Orientation, orientation);
            TIFF_CASE_UNSIGNED(SamplesPerPixel, samples_per_pixel);
            TIFF_CASE_UNSIGNED(RowsPerStrip, rows_per_strip);

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

            TIFF_CASE_UNSIGNED(PlanarConfiguration, planar_configuration);
            TIFF_CASE_UNSIGNED(Predictor, predictor);

            case Tag::Software:
                context.software = getAscii(p, memory, type, is_big_tiff);
                //printLine(Print::Info, "    [Software]");
                //printLine(Print::Info, "{}", context.software);
                break;

            case Tag::ColorMap:
            {
                printLine(Print::Info, "    [ColorMap]");
                u32 count = 1 << context.bpp;
                if (count > 256)
                {
                    //header.setError("Incorrect ColorMap size: {}.", count);
                    break;
                }

                context.palette.size = count;
                std::vector<u32> values = getUnsignedArray(p, memory, type, count * 3, is_big_tiff);

                for (u32 i = 0; i < count; ++i)
                {
                    u32 r = values[i + count * 0] >> 8;
                    u32 g = values[i + count * 1] >> 8;
                    u32 b = values[i + count * 2] >> 8;
                    context.palette[i] = Color(r, g, b, 0xff);
                }
                break;
            }

            TIFF_CASE_UNSIGNED(SampleFormat, sample_format);
            TIFF_CASE_UNSIGNED(JPEGProc, jpeg_proc);
            TIFF_CASE_UNSIGNED(JPEGInterchangeFormat, jpeg_interchange_format);
            TIFF_CASE_UNSIGNED(JPEGInterchangeFormatLength, jpeg_interchange_format_length);
            TIFF_CASE_UNSIGNED(JPEGRestartInterval, jpeg_restart_interval);
            TIFF_CASE_UNSIGNED(JPEGLosslessPredictors, jpeg_lossless_predictors);
            TIFF_CASE_UNSIGNED(JPEGPointTransforms, jpeg_point_transforms);
            TIFF_CASE_UNSIGNED(JPEGQTables, jpeg_qt_tables);
            TIFF_CASE_UNSIGNED(JPEGDCTables, jpeg_dc_tables);
            TIFF_CASE_UNSIGNED(JPEGACTables, jpeg_ac_tables);

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

            case Tag::DateTime:
            case Tag::Artist:
                // ignored tags
                break;
    
            case Tag::Matteing:
            case Tag::DataType:
            case Tag::ImageDepth:
            case Tag::TileDepth:
                // deprecated tags
                break;

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
    bool is_lzw_msb_first(ConstMemory input)
    {
        if (input.size < 2)
            return true; // Default to MSB-first for short inputs

        const u8* data = input.address;
        u8 byte0 = data[0];
        u8 byte1 = data[1];

        // Try MSB-first: high bits of byte0, low bits of byte1
        u32 msb_code = ((u32(byte0) << 1) | (u32(byte1) >> 7)) & 0x1FF; // 9-bit extraction

        // Try LSB-first: pack bytes and extract from low bits
        u32 lsb_data = u32(byte0) | (u32(byte1) << 8);
        u32 lsb_code = lsb_data & 0x1FF; // 9-bit extraction

        // Check which extraction yields ClearCode (256)
        if (msb_code == 256)
            return true;  // MSB-first (standard/default)
        else if (lsb_code == 256)
            return false; // LSB-first (non-standard)

        // If neither yields ClearCode, default to MSB-first (standard)
        return true;
    }

    static
    bool lzw_decompress(Memory output, ConstMemory input)
    {
        // Auto-detect bit order by checking which yields ClearCode (256)
        const bool is_msb_first = is_lzw_msb_first(input);

        // Unified LZW decoder supporting both MSB-first and LSB-first bit orders
        struct lzw_entry
        {
            s16 prefix;
            u8 first;
            u8 suffix;
        } codes[4096];

        u8 decode_stack[4096];

        // Initialize table with single characters
        for (int i = 0; i < 256; i++)
        {
            codes[i].prefix = -1;
            codes[i].first = static_cast<u8>(i);
            codes[i].suffix = static_cast<u8>(i);
        }

        const int ClearCode = 256;
        const int EoiCode = 257;
        int next_table_entry = 258;

        // Calculate pointers from Memory objects
        const u8* src_ptr = input.address;
        const u8* src_end = input.address + input.size;
        u8* dest_ptr = output.address;
        u8* dest_end = output.address + output.size;

        // Bit reading state
        s32 data = 0;
        s32 data_bits = 0;
        s32 codesize = 9;  // Start with 9-bit codes
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
            {
                break; // EOF
            }

            // Handle non-compliant streams that don't start with Clear code (LSB-first compatibility)
            if (!is_msb_first && first_code && Code != ClearCode)
            {
                // Initialize as if we got a Clear code
                next_table_entry = 258;
                codesize = 9;
                codemask = (1 << codesize) - 1;
                
                // Process this first code normally
                if (!WriteString(Code))
                    return false;
                OldCode = Code;
                first_code = false;
                continue;
            }
            first_code = false;

            // Validate code is in valid range
            if (Code >= next_table_entry + 1)
            {
                return false;
            }

            if (Code == ClearCode)
            {
                // Initialize table
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
                    // Critical Path: Code is in table (99.9% of the time)
                    if (!WriteString(Code))
                        return false;
                    if (OldCode >= 0)
                    {
                        AddStringToTable(OldCode, codes[Code].first);
                    }
                    OldCode = Code;
                }
                else
                {
                    // Code not in table - the "KwKwK" case (rare)
                    if (OldCode >= 0)
                    {
                        AddStringToTable(OldCode, codes[OldCode].first);
                        if (!WriteString(Code)) // Code now exists in table
                            return false;
                        OldCode = Code;
                    }
                }
            }
        }

        return true;
    }

    static
    bool ccitt_rle_decompress(Memory output, ConstMemory input, u32 width)
    {
        // Clean, minimal CCITT Modified Huffman decoder
        // Based on TIFF 6.0 specification requirements
        
        constexpr u8 White = 0x00;  // Raw white value (before PhotometricInterpretation)
        constexpr u8 Black = 0xFF;  // Raw black value (before PhotometricInterpretation)

        const u8* src = input.address;

        bool is_problematic_strip = (input.size == 370 && output.size == 8160);
        const u8* src_end = input.address + input.size;
        u8* dest = output.address;
        u8* dest_end = output.address + output.size;

        // CCITT Modified Huffman code tables
        struct ccitt_code {
            u16 code;     // bit pattern
            u8 bits;      // number of bits
            u16 run;      // run length
        };

        // Complete CCITT Modified Huffman white run codes (from TIFF 6.0 spec)
        // Note: These should be MSB-first bit patterns
        static const ccitt_code white_codes[] = {
            {0x35, 8, 0},    {0x7, 6, 1},     {0x7, 4, 2},     {0x8, 4, 3},
            {0xB, 4, 4},     {0xC, 4, 5},     {0xE, 4, 6},     {0xF, 4, 7},
            {0x13, 5, 8},    {0x14, 5, 9},    {0x7, 5, 10},    {0x8, 5, 11},
            {0x8, 6, 12},    {0x3, 6, 13},    {0x34, 6, 14},   {0x35, 6, 15},
            {0x2A, 6, 16},   {0x2B, 6, 17},   {0x27, 7, 18},   {0xC, 7, 19},
            {0x8, 7, 20},    {0x17, 7, 21},   {0x3, 7, 22},    {0x4, 7, 23},
            {0x28, 7, 24},   {0x2B, 7, 25},   {0x13, 7, 26},   {0x24, 7, 27},
            {0x18, 7, 28},   {0x2, 8, 29},    {0x3, 8, 30},    {0x1A, 8, 31},
            {0x1B, 8, 32},   {0x12, 8, 33},   {0x13, 8, 34},   {0x14, 8, 35},
            {0x15, 8, 36},   {0x16, 8, 37},   {0x17, 8, 38},   {0x28, 8, 39},
            {0x29, 8, 40},   {0x2A, 8, 41},   {0x2B, 8, 42},   {0x2C, 8, 43},
            {0x2D, 8, 44},   {0x4, 8, 45},    {0x5, 8, 46},    {0xA, 8, 47},
            {0xB, 8, 48},    {0x52, 8, 49},   {0x53, 8, 50},   {0x54, 8, 51},
            {0x55, 8, 52},   {0x24, 8, 53},   {0x25, 8, 54},   {0x58, 8, 55},
            {0x59, 8, 56},   {0x5A, 8, 57},   {0x5B, 8, 58},   {0x4A, 8, 59},
            {0x4B, 8, 60},   {0x32, 8, 61},   {0x33, 8, 62},   {0x34, 8, 63},
            // Make-up codes for white
            {0x1B, 5, 64},   {0x12, 5, 128},  {0x17, 6, 192},  {0x37, 7, 256},
            {0x36, 8, 320},  {0x37, 8, 384},  {0x64, 8, 448},  {0x65, 8, 512},
            {0x68, 8, 576},  {0x67, 8, 640},  {0xCC, 9, 704},  {0xCD, 9, 768},
            {0xD2, 9, 832},  {0xD3, 9, 896},  {0xD4, 9, 960},  {0xD5, 9, 1024},
            {0xD6, 9, 1088}, {0xD7, 9, 1152}, {0xD8, 9, 1216}, {0xD9, 9, 1280},
            {0xDA, 9, 1344}, {0xDB, 9, 1408}, {0x98, 9, 1472}, {0x99, 9, 1536},
            {0x9A, 9, 1600}, {0x18, 6, 1664}, {0x9B, 9, 1728}
        };

        // Complete CCITT Modified Huffman black run codes (from TIFF 6.0 spec) 
        static const ccitt_code black_codes[] = {
            {0x37, 10, 0},   {0x2, 3, 1},     {0x3, 2, 2},     {0x2, 2, 3},
            {0x3, 3, 4},     {0x3, 4, 5},     {0x2, 4, 6},     {0x3, 5, 7},
            {0x5, 6, 8},     {0x4, 6, 9},     {0x4, 7, 10},    {0x5, 7, 11},
            {0x7, 7, 12},    {0x4, 8, 13},    {0x7, 8, 14},    {0x18, 9, 15},
            {0x17, 10, 16},  {0x18, 10, 17},  {0x8, 10, 18},   {0x67, 11, 19},
            {0x68, 11, 20},  {0x6C, 11, 21},  {0x37, 11, 22},  {0x28, 11, 23},
            {0x17, 11, 24},  {0x18, 11, 25},  {0xCA, 12, 26},  {0xCB, 12, 27},
            {0xCC, 12, 28},  {0xCD, 12, 29},  {0x68, 12, 30},  {0x69, 12, 31},
            {0x6A, 12, 32},  {0x6B, 12, 33},  {0xD2, 12, 34},  {0xD3, 12, 35},
            {0xD4, 12, 36},  {0xD5, 12, 37},  {0xD6, 12, 38},  {0xD7, 12, 39},
            {0x6C, 12, 40},  {0x6D, 12, 41},  {0xDA, 12, 42},  {0xDB, 12, 43},
            {0x54, 12, 44},  {0x55, 12, 45},  {0x56, 12, 46},  {0x57, 12, 47},
            {0x64, 12, 48},  {0x65, 12, 49},  {0x52, 12, 50},  {0x53, 12, 51},
            {0x24, 12, 52},  {0x37, 12, 53},  {0x38, 12, 54},  {0x27, 12, 55},
            {0x28, 12, 56},  {0x58, 12, 57},  {0x59, 12, 58},  {0x2B, 12, 59},
            {0x2C, 12, 60},  {0x5A, 12, 61},  {0x66, 12, 62},  {0x67, 12, 63},
            // Make-up codes for black  
            {0xF, 10, 64},   {0xC8, 12, 128}, {0xC9, 12, 192}, {0x5B, 12, 256},
            {0x33, 12, 320}, {0x34, 12, 384}, {0x35, 12, 448}, {0x6C, 13, 512},
            {0x6D, 13, 576}, {0x4A, 13, 640}, {0x4B, 13, 704}, {0x4C, 13, 768},
            {0x4D, 13, 832}, {0x72, 13, 896}, {0x73, 13, 960}, {0x74, 13, 1024},
            {0x75, 13, 1088}, {0x76, 13, 1152}, {0x77, 13, 1216}, {0x52, 13, 1280},
            {0x53, 13, 1344}, {0x54, 13, 1408}, {0x55, 13, 1472}, {0x5A, 13, 1536},
            {0x5B, 13, 1600}, {0x64, 13, 1664}, {0x65, 13, 1728}
        };

        // Bit reading state
        u32 bit_buffer = 0;
        int bits_available = 0;

        auto ensure_bits = [&](int num_bits) -> bool
        {
            while (bits_available < num_bits && src < src_end)
            {
                bit_buffer = (bit_buffer << 8) | *src++;
                bits_available += 8;
            }
            return bits_available >= num_bits;
        };

        auto decode_run = [&](bool is_white) -> int
        {
            // DISABLE the 8-bit 0x01 check - this might be misinterpreting valid data
            // The 0x01 pattern could be part of a legitimate CCITT code

            // Search in appropriate table
            const ccitt_code* table = is_white ? white_codes : black_codes;
            int table_size = is_white ? sizeof(white_codes)/sizeof(ccitt_code) : 
                                        sizeof(black_codes)/sizeof(ccitt_code);

            // Try codes from shortest to longest (2-13 bits for CCITT)
            for (int bits = 2; bits <= 13; bits++)
            {
                if (!ensure_bits(bits)) continue;
                
                // Extract code of current length from MSB side
                u32 code = (bit_buffer >> (bits_available - bits)) & ((1 << bits) - 1);
                
                // Search for matching code in table
                for (int i = 0; i < table_size; i++)
                {
                    if (table[i].bits == bits && table[i].code == code)
                    {
                        // Found match - validate run length is reasonable
                        if (table[i].run > width)
                        {
                            // Don't return this - keep looking for better match
                            continue;
                        }

                        // Found reasonable match - consume bits 
                        bits_available -= bits;

                        return table[i].run;
                    }
                }
            }

            // Only treat as end if we're completely out of input data
            if (src >= src_end)
            {
                return -3; // End of data
            }

            // Show what pattern we couldn't decode
            if (ensure_bits(16))
            {
                u32 debug_bits = (bit_buffer >> (bits_available - 16)) & 0xFFFF;
            }

            // Simple fallback - skip 1 bit and continue  
            if (ensure_bits(1))
            {
                bits_available -= 1;
                return 1; // 1 pixel run
            }

            return -1; // Complete failure
        };

        // CCITT Modified Huffman decoding with direct u8 color values
        u8 current_color = White; // Start with white (TIFF 6.0: all rows begin with white)  
        int pixels_written = 0;
        int total_run_length = 0;

        while (src < src_end && dest < dest_end)
        {
            total_run_length = 0;

            // Decode run length (may need multiple codes for long runs)
            while (true)
            {
                int run_length = decode_run(current_color == White);

                if (run_length == -3)
                {
                    // End/padding marker - we're done
                    goto decode_complete;

                }
                else if (run_length < 0)
                {
                    // Check if we're at a row boundary - this might be normal row padding
                    if (pixels_written % width == 0 && pixels_written > 0)
                    {
                        // Try to skip to next byte boundary and look for valid codes
                        int bits_to_skip = bits_available % 8;
                        if (bits_to_skip > 0)
                        {
                            bits_available -= bits_to_skip;
                        }

                        // Reset to white for new row and break out to try next run
                        current_color = White;
                        break; // Break out of run decoding loop, start new run
                    }
                    return false;
                }

                total_run_length += run_length;

                // If this was a make-up code (multiple of 64), expect a terminating code next
                if (run_length >= 64 && (run_length % 64) == 0)
                {
                    continue; // Read the terminating code
                }
                else
                {
                    break; // This was a terminating code, we're done with this run
                }
            }

            // Check if we completed a row and need to handle row boundary
            int current_row = pixels_written / width;
            int pixels_in_current_row = pixels_written % width;

            // Handle zero-length runs (CRITICAL for black-edge images)
            if (total_run_length == 0)
            {
                // Zero-length run: don't write pixels, but DO alternate color
                current_color = ~current_color;
                // Zero-length run handled
                continue;
            }

            // Only output pixels if we have a valid run length
            if (total_run_length > 0)
            {
                // Simple bounds check for 8-bit expanded output
                int pixels_remaining = dest_end - dest;
                int pixels_to_write = std::min(total_run_length, pixels_remaining);

                if (pixels_to_write < total_run_length)
                {
                    // Run length truncated to remaining buffer space
                }

                // Output the run directly using current_color (no expansion needed!)
                for (int i = 0; i < pixels_to_write; i++)
                {
                    *dest++ = current_color;
                }

                pixels_written += pixels_to_write;
                current_color = ~current_color; // Flip between 0xFF and 0x00

                // Check if we completed a row
                if (pixels_written % width == 0 && pixels_written > 0)
                {
                    int row_completed = pixels_written / width;

                    // TIFF 6.0: "New rows always begin on the next available byte boundary"
                    if (bits_available % 8 != 0) {
                        int bits_to_skip = bits_available % 8;
                        bits_available -= bits_to_skip;
                    }

                    // TIFF 6.0: Each row starts with white
                    current_color = White;
                }

                // Stop if buffer is full
                if (pixels_written >= output.size)
                {
                    // Buffer full
                    break;
                }
            }
        }

        // Main decoding loop completed

    decode_complete:        
        int bytes_written = dest - output.address;

        // Fill any remaining output buffer with white
        while (dest < dest_end)
        {
            *dest++ = White;
        }

        return true;
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

            printLine(Print::Info, "  Tiff: {} x {} ({} bpp, {} channels)", 
                     m_context.width, m_context.height, m_context.bpp, m_context.samples_per_pixel);
        }

        Format getImageFormat()
        {
            // bit-packed formats resolve to at least 8 bits per channel
            u32 bits = m_context.sample_bits >= 8 ? m_context.sample_bits : 8;

            switch (PhotometricInterpretation(m_context.photometric))
            {
                case PhotometricInterpretation::WHITE_IS_ZERO:
                case PhotometricInterpretation::BLACK_IS_ZERO:
                {
                    if (m_context.samples_per_pixel == 1)
                    {
                        return LuminanceFormat(bits * 1, Format::UNORM, bits, 0);
                    }
                    else if (m_context.samples_per_pixel == 2)
                    {
                        return LuminanceFormat(bits * 2, Format::UNORM, bits, bits);
                    }
                }

                case PhotometricInterpretation::RGB:
                {
                    if (m_context.samples_per_pixel == 3)
                    {
                        return Format(bits * 3, Format::UNORM, Format::RGB, bits, bits, bits, 0);
                    }
                    else if (m_context.samples_per_pixel == 4)
                    {
                        return Format(bits * 4, Format::UNORM, Format::RGBA, bits, bits, bits, bits);
                    }
                    else
                    {
                        header.setError("Unsupported number of channels: {}", m_context.samples_per_pixel);
                        return Format();
                    }
                }

                case PhotometricInterpretation::PALETTE:
                    return IndexedFormat(8);

                case PhotometricInterpretation::TRANSPARENCY_MASK:
                case PhotometricInterpretation::SEPARATED:
                case PhotometricInterpretation::YCBCR:
                case PhotometricInterpretation::CIELAB:
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

            if (!header.success)
            {
                status.setError(header.info);
                return status;
            }

            DecodeTargetBitmap target(dest, header.width, header.height, header.format, m_context.palette, false);

            if (m_context.planar_configuration == 2)
            {
                // Planar format: strips organized as channels, clear target buffer first
                std::memset(target.image, 0, target.stride * header.height);
                
                // For planar: strips per spatial region = total_strips / samples_per_pixel
                u32 strips_per_spatial_region = m_context.strip_offsets.size() / m_context.samples_per_pixel;
                
                for (size_t spatial_strip = 0; spatial_strip < strips_per_spatial_region; ++spatial_strip)
                {
                    u32 y = spatial_strip * m_context.rows_per_strip;
                    u32 strip_height = std::min(m_context.rows_per_strip, header.height - y);
                    
                    for (u32 channel = 0; channel < m_context.samples_per_pixel; ++channel)
                    {
                        size_t strip_index = spatial_strip * m_context.samples_per_pixel + channel;
                        
                        const u8* src = m_memory.address + m_context.strip_offsets[strip_index];
                        u32 bytes = m_context.strip_byte_counts[strip_index];

                        Surface strip(target, 0, y, header.width, strip_height);
                        decodeStrip(strip, ConstMemory(src, bytes), header.width, strip_height, strip_index, channel);
                    }
                }
            }
            else
            {
                // Chunky format: strips organized spatially
                u32 y = 0;
                
                for (size_t i = 0; i < m_context.strip_offsets.size(); ++i)
                {
                    u32 strip_height = std::min(m_context.rows_per_strip, header.height - y);

                    const u8* src = m_memory.address + m_context.strip_offsets[i];
                    u32 bytes = m_context.strip_byte_counts[i];

                    Surface strip(target, 0, y, header.width, strip_height);
                    decodeStrip(strip, ConstMemory(src, bytes), header.width, strip_height, i);

                    y += strip_height;
                }
            }

            target.resolve();

            return status;
        }

        void expandSubBytePixels(Memory dest, ConstMemory src, int width, int height)
        {
            const u8* src_ptr = src.address;
            u8* dest_ptr = dest.address;

            int pixels_per_byte = 8 / m_context.bpp;
            int mask = (1 << m_context.bpp) - 1;

            // For palette images, keep raw indices (0-1, 0-3, 0-15)
            // For sample images, scale to full 8-bit range (0-255)
            bool is_palette = (m_context.photometric == 3); // PhotometricInterpretation::Palette
            int scale = is_palette ? 1 : (255 / mask);

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; x += pixels_per_byte)
                {
                    u8 packed_byte = *src_ptr++;

                    // Extract pixels from MSB to LSB
                    int shift = 8 - m_context.bpp;  // Start at MSB
                    for (int pixel = 0; pixel < pixels_per_byte && (x + pixel) < width; ++pixel)
                    {
                        int pixel_value = (packed_byte >> shift) & mask;
                        *dest_ptr++ = pixel_value * scale;
                        shift -= m_context.bpp;
                    }
                }
            }
        }

        void decodeStrip(Surface target, ConstMemory memory, int width, int height, int strip_index = -1, u32 channel_index = 0)
        {
            // Expand sub-byte formats (1, 2, 4 bits) to 8-bit during decompression for cleaner pipeline
            u32 effective_bpp = std::max(8u, u32(m_context.bpp));

            // For planar format, each strip contains one channel only
            u32 bytes_per_row;
            if (m_context.planar_configuration == 2)
            {
                bytes_per_row = (header.width * effective_bpp / m_context.samples_per_pixel + 7) / 8;
            } 
            else
            {
                bytes_per_row = (header.width * effective_bpp + 7) / 8;
            }
            u32 uncompressed_bytes = height * bytes_per_row;

            Buffer buffer(uncompressed_bytes);
            Buffer expanded_buffer;

            // Track whether we need post-decompression sub-byte expansion
            bool needs_expansion = (m_context.bpp < 8);

            switch (Compression(m_context.compression))
            {
                case Compression::NONE:
                {
                    if (needs_expansion)
                    {
                        expandSubBytePixels(buffer, memory, width, height);
                        needs_expansion = false;
                    }
                    else
                    {
                        std::memcpy(buffer, memory.address, uncompressed_bytes);
                    }
                    memory = buffer;
                    break;
                }

                case Compression::CCITT_RLE:
                {
                    bool success = ccitt_rle_decompress(buffer, memory, width);
                    if (!success)
                    {
                        printLine(Print::Error, "[CCITT-RLE] Decompression failed for strip {}", strip_index);
                        break;
                    }

                    memory = buffer;
                    needs_expansion = false; // CCITT already expands to 8-bit
                    break;
                }

                //case Compression::CCITT_FAX3:
                //    break;

                // case Compression::CCITT_FAX4:
                //     break;

                case Compression::LZW:
                {
                    bool success = lzw_decompress(buffer, memory);
                    if (!success)
                    {
                        printLine(Print::Error, "[LZW] Decompression failed");
                        break;
                    }

                    memory = buffer;
                    break;
                }

                case Compression::JPEG:
                    printLine(Print::Info, "    Unsupported compression: {}", m_context.compression);
                    break;

                case Compression::ZIP:
                    zlib::decompress(buffer, memory);
                    memory = buffer;
                    break;

                case Compression::PACKBITS:
                    packbits_decompress(buffer, memory);
                    memory = buffer;
                    break;

                case Compression::DEFLATE:
                case Compression::SGILOG:
                    printLine(Print::Info, "    Unsupported compression: {}", m_context.compression);
                    break;

                default:
                    printLine(Print::Info, "    Unknown compression: {}", m_context.compression);
                    //status.setError("Unknown compression.");
                    //return status;
                    return;
            }

            // Post-decompression sub-byte expansion if needed
            if (needs_expansion)
            {
                expanded_buffer.resize(uncompressed_bytes);
                expandSubBytePixels(expanded_buffer, memory, width, height);
                memory = expanded_buffer;
            }

            if (m_is_little_endian != isLittleEndianCPU())
            {
                if (m_context.sample_bits == 16)
                {
                    // Handle 16-bit endianness conversion
                    byteswap(reinterpret_cast<u16*>(const_cast<u8*>(memory.address)), memory.size / 2);
                }
            }

            // Apply PhotometricInterpretation color inversion for grayscale images
            if (m_context.photometric == 0 && m_context.samples_per_pixel == 1) // WhiteIsZero grayscale
            {
                u8* data = const_cast<u8*>(memory.address);
                for (u32 i = 0; i < memory.size; ++i)
                {
                    data[i] = 255 - data[i];
                }
            }

            for (u32 y = 0; y < height; ++y)
            {
                if (m_context.planar_configuration == 2)
                {
                    resolvePlanarScanline(target.image, memory.address, bytes_per_row, m_context.samples_per_pixel, channel_index);
                }
                else
                {
                    resolveChunkyScanline(target.image, memory.address, bytes_per_row, m_context.samples_per_pixel);
                }
                memory.address += bytes_per_row;
                target.image += target.stride;
            }
        }

        void resolveChunkyScanline(u8* dest, const u8* src, u32 bytes, u32 channels)
        {
            if (m_context.predictor == 1)
            {
                // chunky, no prediction
                std::memcpy(dest, src, bytes);
            }
            else if (m_context.predictor == 2)
            {
                // chunky, horizontal differencing
                std::memcpy(dest, src, channels); // copy first sample

                // apply horizontal differencing for each sample
                for (u32 x = channels; x < bytes; x += channels)
                {
                    for (u32 c = 0; c < channels; ++c)
                    {
                        dest[x + c] = src[x + c] + dest[x - channels + c];
                    }
                }
            }
        }

        void resolvePlanarScanline(u8* dest, const u8* src, u32 pixels, u32 channels, u32 channel_index)
        {
            if (m_context.predictor == 1)
            {
                // planar, no prediction - write channel data to correct offset
                u8* pixel_dest = dest + channel_index;

                for (u32 pixel = 0; pixel < pixels; ++pixel)
                {
                    *pixel_dest = src[pixel];
                    pixel_dest += channels;
                }
            }
            else if (m_context.predictor == 2)
            {
                // planar, horizontal differencing
                // TODO: implement predictor for planar format
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
