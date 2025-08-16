/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>
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
        Predictor = 269,
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
        ColorMap = 320,
        //ExtraSamples = 338, // SHORT
        //Copyright = 33432, // ASCII
    };

    enum class Compression
    {
        NONE = 1,
        CCITT_RLE = 2,
        CCITT_FAX3 = 3,
        CCITT_FAX4 = 4,
        LZW = 5,
        JPEG = 6,
        PACKBITS = 32773,
        DEFLATE = 32946,
        ZIP = 8
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

        u32 rows_per_strip = 0;
        std::vector<u32> strip_offsets;
        std::vector<u32> strip_byte_counts;

        std::string image_description;
        std::string software;
        std::string predictor;
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

            case Tag::Predictor:
                context.predictor = getAscii(p, memory, type);
                printLine(Print::Info, "    [Predictor]");
                //printLine(Print::Info, "{}", context.predictor);
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

            case Tag::Software:
                context.software = getAscii(p, memory, type);
                printLine(Print::Info, "    [Software]");
                //printLine(Print::Info, "{}", context.software);
                break;

            case Tag::ColorMap:
                printLine(Print::Info, "    [ColorMap]");
                // TODO: parse color map
                break;

            default:
                printLine(Print::Info, "    [UNKNOWN: {}]", int(tag));
                break;
        }

        printLine(Print::Info, "      type: {}, count: {}", int(type), count);
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
            header.format = LuminanceFormat(8, Format::UNORM, 8, 0);
            //header.format = Format(24, Format::UNORM, Format::RGB, 8, 8, 8); // TODO: set format based on samples per pixel and bits per sample
            header.compression = TextureCompression::NONE;

            // Determine format based on samples per pixel and bits per sample
            //setImageFormat();

            printLine(Print::Info, "  Image: {} x {} ({} bpp, {} channels)", 
                     m_context.width, m_context.height, m_context.bits_per_sample, m_context.samples_per_pixel);
        }

        /*
        void setImageFormat()
        {
            // Determine format based on samples per pixel and bits per sample
            switch (m_samples_per_pixel)
            {
                case 1: // Grayscale
                    if (m_bits_per_sample == 8)
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    else if (m_bits_per_sample == 16)
                        header.format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                    else
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;

                case 3: // RGB
                    if (m_bits_per_sample == 8)
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    else if (m_bits_per_sample == 16)
                        header.format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                    else
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;

                case 4: // RGBA
                    if (m_bits_per_sample == 8)
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    else if (m_bits_per_sample == 16)
                        header.format = Format(64, Format::UNORM, Format::RGBA, 16, 16, 16, 16);
                    else
                        header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;

                default:
                    header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
                    break;
            }
        }
        */

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

#if 1
            Bitmap bitmap(header.width, header.height, header.format);

            u32 y = 0;

            for (size_t i = 0; i < m_context.strip_offsets.size(); ++i)
            {
                u32 rows_to_read = std::min(m_context.rows_per_strip, header.height - y);

                u32 y0 = y;
                u32 y1 = y + rows_to_read;

                const u8* src = m_memory.address + m_context.strip_offsets[i];
                u32 src_size = m_context.strip_byte_counts[i];

                //for (u32 y = y0; y < y1; ++y)
                {
                    u8* dest = bitmap.address(0, y0);
                    std::memcpy(dest, src, src_size);
                    //src += src_size;
                }

                y += rows_to_read;
            }

            dest.blit(0, 0, bitmap);
#endif

            return status;
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
