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
        ImageWidth = 256,
        ImageLength = 257,
        BitsPerSample = 258,
        Compression = 259,
        PhotometricInterpretation = 262,
        //StripOffsets = 273,
        //StripByteCounts = 279,
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

        /*
        u16 m_photometric = 2;
        std::vector<u32> m_strip_offsets;
        std::vector<u32> m_strip_byte_counts;
        */
    };

    template <typename Pointer>
    u32 getOffset(Pointer p)
    {
        return p.read32();
    }

    template <typename Pointer>
    u32 getUnsigned(Pointer p, Type type)
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
                // TODO: use utility function to return vector of values
                //       and sum all the vector elements using std::accumulate
                if (count > 1)
                {
                    for (u32 i = 0; i < count; ++i)
                    {
                        Pointer temp = memory.address + getOffset(p);
                        context.bits_per_sample += getUnsigned(temp, type);
                    }
                }
                else
                {
                    context.bits_per_sample = getUnsigned(p, type);
                }
                printLine(Print::Info, "    [BitsPerSample]");
                printLine(Print::Info, "      value: {}", context.bits_per_sample);
                break;

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

            default:
                printLine(Print::Info, "    [Unknown: {}]", int(tag));
                break;
        }

        u32 offset = getOffset(p);
        printLine(Print::Info, "      type: {}, count: {}, offset: {}",
            int(type), count, offset);
    }

#if 0
    void parseIFDEntry(const IFDEntry& entry)
    {
        switch (entry.tag)
        {

            /*

            case 258: // BitsPerSample
                if (entry.type == 3 && entry.count == 1) // SHORT, single value
                {
                    m_bits_per_sample = u16(entry.value_offset & 0xFFFF);
                }
                else if (entry.count > 1)
                {
                    printLine(Print::Info, "    BitsPerSample: Multiple values at offset {}", entry.value_offset);
                    m_bits_per_sample = 0;

                    // Read the actual bits per sample array
                    const u8* bits_data = m_memory.address + entry.value_offset;
                    for (int i = 0; i < entry.count; i++)
                    {
                        // TODO: Handle big endian
                        u16 value = littleEndian::uload16(bits_data + i * 2);
                        printLine(Print::Info, "      value[{}] = {}", i, value);
                        m_bits_per_sample += u16(value);
                    }
                }
                else
                {
                    printLine(Print::Info, "    BitsPerSample: Unexpected type {} count {}", entry.type, entry.count);
                    m_bits_per_sample = 8; // Default fallback
                }
                printLine(Print::Info, "    BitsPerSample: {}", m_bits_per_sample);
                break;

            case 273: // StripOffsets
                printLine(Print::Info, "    StripOffsets: {} (count: {})", entry.value_offset, entry.count);
                // TODO: Handle multiple strip offsets
                if (entry.count == 1)
                {
                    m_strip_offsets.push_back(entry.value_offset);
                }
                break;

            case 277: // SamplesPerPixel
                m_samples_per_pixel = u16(entry.value_offset);
                printLine(Print::Info, "    SamplesPerPixel: {}", m_samples_per_pixel);
                break;

            case 279: // StripByteCounts
                printLine(Print::Info, "    StripByteCounts: {} (count: {})", entry.value_offset, entry.count);
                // TODO: Handle multiple strip byte counts
                if (entry.count == 1)
                {
                    m_strip_byte_counts.push_back(entry.value_offset);
                }
                break;
            */

            default:
                printLine(Print::Info, "    [Unknown: {}]", entry.tag);
                break;
        }

        printLine(Print::Info, "      type: {}, count: {}, value_offset: {}",
            int(entry.type), entry.count, entry.value_offset);
    }
#endif

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
