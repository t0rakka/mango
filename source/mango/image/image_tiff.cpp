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
    };

    enum class Tag : u16
    {
        //NewSubfileType = 254, // LONG
        //SubfileType = 255, // SHORT
        ImageWidth = 256,
        ImageLength = 257,
        BitsPerSample = 258,
        Compression = 259,
        PhotometricInterpretation = 262,
        //Threshholding = 263, // SHORT
        //CellWidth = 264, // SHORT
        //CellLength = 265, // SHORT
        //FillOrder = 266, // SHORT
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
        //FreeOffsets = 288, // LONG
        //FreeByteCounts = 289, // LONG
        //GrayResponseUnit = 290, // SHORT
        //GrayResponseCurve = 291, // SHORT
        ResolutionUnit = 296,
        Software = 305,
        //DateTime = 306, // ASCII
        //Artist = 315, // ASCII
        //HostComputer = 316, // ASCII
        Predictor = 317,
        ColorMap = 320,
        //ExtraSamples = 338, // SHORT
        //Copyright = 33432, // ASCII
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
        LINEAR_RAW = 34892
    };

    struct IFDContext
    {
        u32 width = 0;
        u32 height = 0;
        u16 bits_per_sample = 0;
        u16 samples_per_pixel = 3;
        u16 compression = 1;
        u16 photometric = 2;
        u16 resolution_unit = 0;
        float x_resolution = 0;
        float y_resolution = 0;
        u32 orientation = 0;
        u32 planar_configuration = 0;
        u32 predictor = 1;

        u32 rows_per_strip = 0;
        std::vector<u32> strip_offsets;
        std::vector<u32> strip_byte_counts;

        Palette palette;

        std::string image_description;
        std::string software;
        std::string document_name;
    };

    static inline
    u32 getSize(Type type, u32 count)
    {
        u32 size = 0;

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
                size = count * 8;
                break;
        }

        return size;
    }

    template <typename Pointer>
    u32 getOffset(Pointer p)
    {
        return p.read32();
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
                break;
        }

        return value;
    }

    template <typename Pointer>
    std::vector<u32> getUnsignedArray(Pointer p, ConstMemory memory, Type type, u32 count)
    {
        std::vector<u32> values;

        if (getSize(type, count) > 4)
        {
            p = memory.address + getOffset(p);
        }

        for (u32 i = 0; i < count; ++i)
        {
            u32 value = getUnsigned(p, type);
            values.push_back(value);
        }

        return values;
    }

    template <typename Pointer>
    float getRational(Pointer p, ConstMemory memory, Type type)
    {
        Pointer temp = memory.address + getOffset(p);
        u32 numerator = getUnsigned(temp, Type::LONG);
        u32 denominator = getUnsigned(temp, Type::LONG);
        //printLine(Print::Info, "      numerator: {}, denominator: {}", numerator, denominator);
        float value = float(numerator) / denominator;
        return value;
    }

    template <typename Pointer>
    std::string getAscii(Pointer p, ConstMemory memory, Type type)
    {
        const char* temp = reinterpret_cast<const char*>(memory.address + getOffset(p));
        std::string value = temp;
        return value;
    }

    template <typename Pointer>
    void parse_ifd(IFDContext& context, ConstMemory memory, Pointer p)
    {
        Tag tag = Tag(p.read16());
        Type type = Type(p.read16());
        u32 count = p.read32();

        switch (tag)
        {
            case Tag::ImageWidth:
                context.width = getUnsigned(p, type);
                printLine(Print::Info, "    [ImageWidth]");
                printLine(Print::Info, "      value: {}", context.width);
                break;

            case Tag::ImageLength:
                context.height = getUnsigned(p, type);
                printLine(Print::Info, "    [ImageLength]");
                printLine(Print::Info, "      value: {}", context.height);
                break;

            case Tag::BitsPerSample:
            {
                std::vector<u32> values = getUnsignedArray(p, memory, type, count);
                context.bits_per_sample = std::accumulate(values.begin(), values.end(), 0);
                printLine(Print::Info, "    [BitsPerSample]");
                printLine(Print::Info, "      value: {}", context.bits_per_sample);
                break;
            }

            case Tag::Compression:
                context.compression = getUnsigned(p, type);
                printLine(Print::Info, "    [Compression]");
                printLine(Print::Info, "      value: {}", context.compression);
                break;

            case Tag::PhotometricInterpretation:
                context.photometric = getUnsigned(p, type);
                printLine(Print::Info, "    [PhotometricInterpretation]");
                printLine(Print::Info, "      value: {}", context.photometric);
                // TODO: 0 - WhiteIsZero, 1 - BlackIsZero, 2 - RGB, ...
                break;

            case Tag::DocumentName:
                context.document_name = getAscii(p, memory, type);
                printLine(Print::Info, "    [DocumentName]");
                //printLine(Print::Info, "{}", context.document_name);
                break;

            case Tag::ImageDescription:
                context.image_description = getAscii(p, memory, type);
                printLine(Print::Info, "    [ImageDescription]");
                //printLine(Print::Info, "{}", context.image_description);
                break;

            case Tag::StripOffsets:
            {
                printLine(Print::Info, "    [StripOffsets]");
                context.strip_offsets = getUnsignedArray(p, memory, type, count);
                break;
            }

            case Tag::Orientation:
                context.orientation = getUnsigned(p, type);
                printLine(Print::Info, "    [Orientation]");
                printLine(Print::Info, "      value: {}", context.orientation);
                break;

            case Tag::SamplesPerPixel:
                context.samples_per_pixel = getUnsigned(p, type);
                printLine(Print::Info, "    [SamplesPerPixel]");
                printLine(Print::Info, "      value: {}", context.samples_per_pixel);
                break;

            case Tag::RowsPerStrip:
                context.rows_per_strip = getUnsigned(p, type);
                printLine(Print::Info, "    [RowsPerStrip]");
                printLine(Print::Info, "      value: {}", context.rows_per_strip);
                break;

            case Tag::StripByteCounts:
            {
                printLine(Print::Info, "    [StripByteCounts]");
                context.strip_byte_counts = getUnsignedArray(p, memory, type, count);
                break;
            }

            case Tag::ResolutionUnit:
                context.resolution_unit = getUnsigned(p, type);
                printLine(Print::Info, "    [ResolutionUnit]");
                printLine(Print::Info, "      value: {}", context.resolution_unit);
                break;

            case Tag::XResolution:
                context.x_resolution = getRational(p, memory, type);
                printLine(Print::Info, "    [XResolution]");
                printLine(Print::Info, "      value: {}", context.x_resolution);
                break;

            case Tag::YResolution:
                context.y_resolution = getRational(p, memory, type);
                printLine(Print::Info, "    [YResolution]");
                printLine(Print::Info, "      value: {}", context.y_resolution);
                break;

            case Tag::PlanarConfiguration:
                context.planar_configuration = getUnsigned(p, type);
                printLine(Print::Info, "    [PlanarConfiguration]");
                printLine(Print::Info, "      value: {}", context.planar_configuration);
                break;

            case Tag::Predictor:
                context.predictor = getUnsigned(p, type);
                printLine(Print::Info, "    [Predictor]");
                printLine(Print::Info, "      value: {}", context.predictor);
                break;

            case Tag::Software:
                context.software = getAscii(p, memory, type);
                printLine(Print::Info, "    [Software]");
                //printLine(Print::Info, "{}", context.software);
                break;

            case Tag::ColorMap:
            {
                printLine(Print::Info, "    [ColorMap]");
                u32 count = 1 << context.bits_per_sample;
                if (count > 256)
                {
                    //header.setError("Incorrect ColorMap size: {}.", count);
                    break;
                }

                context.palette.size = count;
                std::vector<u32> values = getUnsignedArray(p, memory, type, count * 3);

                for (u32 i = 0; i < count; ++i)
                {
                    u32 r = values[i + count * 0] >> 8;
                    u32 g = values[i + count * 1] >> 8;
                    u32 b = values[i + count * 2] >> 8;
                    context.palette[i] = Color(r, g, b, 0xff);
                }
                break;
            }

            default:
                printLine(Print::Info, "    [UNKNOWN: {}]", int(tag));
                break;
        }

        printLine(Print::Info, "      type: {}, count: {}", int(type), count);
    }

    static
    bool lzw_decompress(Memory output, ConstMemory input)
    {
        // Efficient LZW table structure (like the original!)
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
            // TIFF LZW transitions: 511->10bit, 1023->11bit, 2047->12bit
            // Simple bit trick: these are all (1<<bits)-1, so we can check next_table_entry directly
            if ((next_table_entry == 511 && codesize == 9) ||
                (next_table_entry == 1023 && codesize == 10) ||
                (next_table_entry == 2047 && codesize == 11))
            {
                codesize++;
                codemask = (1 << codesize) - 1;
            }

            // Fill bit buffer
            while (data_bits < codesize && src_ptr < src_end)
            {
                data = (data << 8) | s32(*src_ptr++);
                data_bits += 8;
            }

            if (data_bits < codesize)
                return -1; // EOF

            // Extract code (MSB first)
            int code = (data >> (data_bits - codesize)) & codemask;
            data_bits -= codesize;
            return code;
        };

        auto WriteString = [&](int code)
        {
            u8* sp = decode_stack;
            
            // Build string by following prefix chain
            while (code >= 0)
            {
                *sp++ = codes[code].suffix;
                if (codes[code].prefix < 0)
                    break;
                code = codes[code].prefix;
            }

            // Output string in reverse order
            if (dest_ptr + (sp - decode_stack) > dest_end)
                return false;

            while (sp > decode_stack)
                *dest_ptr++ = *--sp;

            return true;
        };

        auto AddStringToTable = [&](int oldcode, u8 first_char)
        {
            if (next_table_entry < 4096)
            {
                codes[next_table_entry].prefix = oldcode;
                codes[next_table_entry].first = codes[oldcode].first;
                codes[next_table_entry].suffix = first_char;
                next_table_entry++;
            }
        };

        int OldCode = -1;

        for (;;)
        {
            int Code = GetNextCode();
            if (Code < 0 || Code == EoiCode)
            {
                break; // EOF
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

                WriteString(Code);
                OldCode = Code;
            }
            else
            {
                if (Code < next_table_entry)
                {
                    // Critical Path: Code is in table (99.9% of the time)
                    WriteString(Code);
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
                        WriteString(Code); // Code now exists in table
                        OldCode = Code;
                    }
                }
            }
        }

        return true;
    }

    struct TIFFHeader
    {
        ByteOrder byte_order;
        u16 version;
        u32 first_ifd_offset;
        bool is_little_endian;

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

            if (version != 42)
            {
                header.setError("[ImageDecoder.TIFF] Invalid TIFF version ({}).", version);
                return false;
            }

            // Get first IFD offset
            if (is_little_endian)
            {
                first_ifd_offset = littleEndian::uload32(p + 4);
            }
            else
            {
                first_ifd_offset = bigEndian::uload32(p + 4);
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
            u16 ifd_count;
            if (m_is_little_endian)
            {
                ifd_count = littleEndian::uload16(p);
            }
            else
            {
                ifd_count = bigEndian::uload16(p);
            }
            p += 2;

            printLine(Print::Info, "  IFD entries: {}", ifd_count);

            // Parse each IFD entry
            for (int i = 0; i < ifd_count; ++i)
            {
                if (p + 12 > end)
                {
                    header.setError("[ImageDecoder.TIFF] IFD entry {} out of bounds.", i);
                    return;
                }

                if (m_is_little_endian)
                {
                    parse_ifd(m_context, m_memory, LittleEndianConstPointer(p));
                }
                else
                {
                    parse_ifd(m_context, m_memory, BigEndianConstPointer(p));
                }

                p += 12;
            }

            // Validate required fields
            if (m_context.width == 0 || m_context.height == 0)
            {
                header.setError("[ImageDecoder.TIFF] Missing required image dimensions.");
                return;
            }

            // Set header info
            header.width = m_context.width;
            header.height = m_context.height;
            header.depth = 0;
            header.levels = 0;
            header.faces = 0;
            header.format = getImageFormat();
            header.compression = TextureCompression::NONE;

            printLine(Print::Info, "  Tiff: {} x {} ({} bpp, {} channels)", 
                     m_context.width, m_context.height, m_context.bits_per_sample, m_context.samples_per_pixel);
        }

        Format getImageFormat()
        {
            // TODO: handle other bit depths than 8
            // TODO: better format selection logic

            switch (PhotometricInterpretation(m_context.photometric))
            {
                case PhotometricInterpretation::WHITE_IS_ZERO:
                    // TODO: handle inverse values
                    return LuminanceFormat(8, Format::UNORM, 8, 0);

                case PhotometricInterpretation::BLACK_IS_ZERO:
                {
                    if (m_context.samples_per_pixel == 1)
                    {
                        return LuminanceFormat(8, Format::UNORM, 8, 0);
                    }
                    else if (m_context.samples_per_pixel == 2)
                    {
                        return LuminanceFormat(16, Format::UNORM, 8, 8);
                    }
                }

                case PhotometricInterpretation::RGB:
                {
                    if (m_context.samples_per_pixel == 3)
                    {
                        return Format(24, Format::UNORM, Format::RGB, 8, 8, 8);
                    }
                    else if (m_context.samples_per_pixel == 4)
                    {
                        return Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
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

            // MANGO TODO: Implement actual TIFF decoding
            //status.setError("[ImageDecoder.TIFF] Decoding not yet implemented.");

            DecodeTargetBitmap target(dest, header.width, header.height, header.format, m_context.palette, false);

            u32 y = 0;

            for (size_t i = 0; i < m_context.strip_offsets.size(); ++i)
            {
                u32 strip_height = std::min(m_context.rows_per_strip, header.height - y);

                const u8* src = m_memory.address + m_context.strip_offsets[i];
                u32 bytes = m_context.strip_byte_counts[i];

                Surface strip(target, 0, y, header.width, strip_height);
                decodeStrip(strip, ConstMemory(src, bytes), header.width, strip_height);

                y += strip_height;
            }

            target.resolve();

            return status;
        }

        void decodeStrip(Surface target, ConstMemory memory, int width, int height)
        {
            //u32 bytes_per_row = (header.width * m_context.bits_per_sample * m_context.samples_per_pixel + 7) / 8;
            u32 bytes_per_row = (header.width * m_context.bits_per_sample + 7) / 8;
            u32 uncompressed_bytes = height * bytes_per_row;

            //printLine(Print::Info, "    Strip height: {}", height);
            //printLine(Print::Info, "    Bytes per row: {}", bytes_per_row);
            //printLine(Print::Info, "    Uncompressed bytes: {}", uncompressed_bytes);
            //printLine(Print::Info, "");

            // TODO: only allocate if needed
            Buffer buffer(uncompressed_bytes);

            switch (Compression(m_context.compression))
            {
                case Compression::NONE:
                    std::memcpy(buffer, memory.address, uncompressed_bytes);
                    memory = buffer;
                    break;

                //case Compression::CCITT_RLE:
                //    break;

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

                //case Compression::JPEG:
                //    break;

                case Compression::ZIP:
                    zlib::decompress(buffer, memory);
                    memory = buffer;
                    break;

                case Compression::PACKBITS:
                    packbits_decompress(buffer, memory);
                    memory = buffer;
                    break;

                //case Compression::DEFLATE:
                //    break;

                default:
                    printLine(Print::Info, "    Unknown compression: {}", m_context.compression);
                    //status.setError("Unknown compression.");
                    //return status;
                    return;
            }

            for (u32 y = 0; y < height; ++y)
            {
                resolveScanline(target.image, memory.address, bytes_per_row, m_context.samples_per_pixel);
                memory.address += bytes_per_row;
                target.image += target.stride;
            }
        }

        void resolveScanline(u8* dest, const u8* src, u32 bytes, u32 channels)
        {
            // PlanarConfiguration: 1 (chunky) or 2 (planar)
            // Predictor: 1 (no prediction) or 2 (horizontal differencing)

            //void byteswap(u16* data, size_t count);
            // TODO: byteswap(src, bytes / 2);

            if (m_context.planar_configuration == 1)
            {
                if (m_context.predictor == 1)
                {
                    // chunky, no prediction
                    std::memcpy(dest, src, bytes);
                }
                else if (m_context.predictor == 2)
                {
                    // chunky, horizontal differencing

                    // copy first sample
                    std::memcpy(dest, src, channels);

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
            else if (m_context.planar_configuration == 2)
            {
                if (m_context.predictor == 1)
                {
                    // planar, no prediction
                    // TODO: convert RRRRRR...GGGGGG....BBBBBB into RGBRGBRGB...
                }
                else if (m_context.predictor == 2)
                {
                    // planar, horizontal differencing
                    // TODO: convert RRRRRR...GGGGGG....BBBBBB into RGBRGBRGB...
                }
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
