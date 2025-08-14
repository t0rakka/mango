/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/pointer.hpp>
#include <mango/core/system.hpp>
#include <mango/core/buffer.hpp>
#include <mango/image/image.hpp>
#include <mango/math/math.hpp>

namespace
{
    using namespace mango;
    using namespace mango::image;
    using namespace mango::math;

    // ------------------------------------------------------------
    // ImageDecoder
    // ------------------------------------------------------------

    // Specification:
    // https://www.adobe.io/content/dam/udp/en/open/standards/tiff/TIFF6.pdf

    enum class ByteOrder
    {
        TIFF_LITTLE_ENDIAN = 0x4949,  // "II"
        TIFF_BIG_ENDIAN = 0x4D4D      // "MM"
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

    struct IFDEntry
    {
        u16 tag;
        u16 type;
        u32 count;
        u32 value_offset;

        void parse(BigEndianConstPointer& p)
        {
            tag = p.read16();
            type = p.read16();
            count = p.read32();
            value_offset = p.read32();
        }

        void parse(LittleEndianConstPointer& p)
        {
            tag = p.read16();
            type = p.read16();
            count = p.read32();
            value_offset = p.read32();
        }
    };

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
            printLine(Print::Info, "  Byte Order:    {}", is_little_endian ? "Little Endian" : "Big Endian");
            printLine(Print::Info, "  Version:       {}", version);
            printLine(Print::Info, "  First IFD:     {}", first_ifd_offset);

            return true;
        }
    };

    struct Interface : ImageDecodeInterface
    {
        ConstMemory m_memory;
        TIFFHeader m_header;
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
            // MANGO TODO: Parse IFD entries to extract image dimensions, format, etc.
            // For now, just set some basic values as placeholders
            
            header.width = 0;
            header.height = 0;
            header.depth = 0;
            header.levels = 0;
            header.faces = 0;
            header.format = Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8);
            header.compression = TextureCompression::NONE;

            printLine(Print::Info, "  [IFD parsing not yet implemented]");
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
            status.setError("[ImageDecoder.TIFF] Decoding not yet implemented.");

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
