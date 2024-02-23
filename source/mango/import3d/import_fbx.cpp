/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/import3d/import_fbx.hpp>

/*
    Autodesk FBX importer
*/

namespace
{
    using namespace mango;
    using namespace mango::import3d;

    struct ReaderFBX
    {
        ConstMemory m_memory;

        ReaderFBX(ConstMemory memory)
            : m_memory(memory)
        {
            LittleEndianConstPointer p = memory.address;

            const u8 magic [] =
            {
                0x4b, 0x61, 0x79, 0x64, 0x61, 0x72, 0x61, 0x20,
                0x46, 0x42, 0x58, 0x20, 0x42, 0x69, 0x6e, 0x61,
                0x72, 0x79, 0x20, 0x20, 0x00, 0x1a, 0x00,
            };

            if (std::memcmp(p, magic, 23))
            {
                MANGO_EXCEPTION("[ImportFBX] Incorrect header.");
            }

            p += 23;
            u32 version = p.read32();
            printLine(Print::Verbose, "Version: {}", version);

            const u8* end = memory.address + memory.size;

            while (p && p < end)
            {
                p = read_node(p, 0);
            }
        }

        ~ReaderFBX()
        {
        }

        template<typename T>
        void read_values(std::vector<T>& output, const u8* p, u32 count)
        {
            while (count-- > 0)
            {
                T value;
                std::memcpy(&value, p, sizeof(T));
                p += sizeof(T);
#if !defined(MANGO_LITTLE_ENDIAN)
                value = byteswap(value);
#endif
                output.push_back(value);
            }
        }

        template<typename T>
        std::vector<T> read_property_array(LittleEndianConstPointer& p, const char* type, int level)
        {
            u32 length = p.read32();
            u32 encoding = p.read32();
            u32 compressedLength = p.read32();

            std::vector<T> output;

            if (encoding)
            {
                Buffer buffer(length * sizeof(T));
                CompressionStatus status = deflate_zlib::decompress(buffer, ConstMemory(p, compressedLength));

                read_values(output, buffer, length);

                p += compressedLength;
                printLine(Print::Verbose, level * 2 + 2, "{}[{}] (compressed)", type, length);
            }
            else
            {
                read_values(output, p, length);

                p += length * sizeof(T);
                printLine(Print::Verbose, level * 2 + 2, "{}[{}]", type, length);
            }

            return output;
        }

        const u8* read_node(LittleEndianConstPointer p, int level)
        {
            u32 endOffset = p.read32();
            u32 numProperties = p.read32();
            u32 propertyListLength = p.read32();
            u8 nameLength = *p++;

            if (!endOffset)
            {
                return level ? p : nullptr;
            }

            std::string name(p.cast<const char>(), nameLength);
            p += nameLength;

            printLine(Print::Verbose, level * 2, "[{}]", name);

            /*
            if (numProperties > 0)
            {
                printLine(Print::Verbose, level * 2 + 2, "count: {} ({} bytes)", numProperties, propertyListLength);
            }
            */

            const u8* next = p + propertyListLength;

            for (u32 i = 0; i < numProperties; ++i)
            {
                char type = *p++;

                switch (type)
                {
                    case 'C':
                    case 'B':
                    {
                        u8 value = *p++;
                        printLine(Print::Verbose, level * 2 + 2, "u8: {}", value);
                        break;
                    }

                    case 'Y':
                    {
                        u16 value = p.read16();
                        printLine(Print::Verbose, level * 2 + 2, "u16: {}", value);
                        break;
                    }

                    case 'I':
                    {
                        u32 value = p.read32();
                        printLine(Print::Verbose, level * 2 + 2, "u32: {}", value);
                        break;
                    }

                    case 'L':
                    {
                        u64 value = p.read64();
                        printLine(Print::Verbose, level * 2 + 2, "u64: {}", value);
                        break;
                    }

                    case 'F':
                    {
                        float value = p.read32f();
                        printLine(Print::Verbose, level * 2 + 2, "f32: {}", value);
                        break;
                    }

                    case 'D':
                    {
                        double value = p.read64f();
                        printLine(Print::Verbose, level * 2 + 2, "f64: {}", value);
                        break;
                    }

                    case 'f':
                    {
                        auto values = read_property_array<float>(p, "f32", level);
                        MANGO_UNREFERENCED(values);
                        break;
                    }

                    case 'd':
                    {
                        auto values = read_property_array<double>(p, "f64", level);
                        MANGO_UNREFERENCED(values);
                        break;
                    }

                    case 'l':
                    {
                        auto values = read_property_array<u64>(p, "u64", level);
                        MANGO_UNREFERENCED(values);
                        break;
                    }

                    case 'i':
                    {
                        auto values = read_property_array<u32>(p, "u32", level);
                        MANGO_UNREFERENCED(values);
                        break;
                    }

                    case 'b':
                    {
                        auto values = read_property_array<u8>(p, "u8", level);
                        MANGO_UNREFERENCED(values);
                        break;
                    }

                    case 'S':
                    {
                        u32 length = p.read32();
                        std::string_view view(p.cast<const char>(), length);
                        p += length;
                        printLine(Print::Verbose, level * 2 + 2, "string: \"{}\"", view);
                        break;
                    }

                    case 'R':
                    {
                        u32 length = p.read32();
                        p += length;
                        printLine(Print::Verbose, level * 2 + 2, "raw: {} {} bytes", type, length);
                        break;
                    }

                    default:
                        printLine(Print::Verbose, level * 2 + 2, "unknown: xxxxxxxxxx");
                        break;
                }
            }

            p = next;

            const u8* end = m_memory.address + endOffset;

            while (p < end)
            {
                p = read_node(p, level + 1);
            }

            return p;
        }
    };

} // namespace

namespace mango::import3d
{

    ImportFBX::ImportFBX(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);
        ReaderFBX reader(file);
    }

} // namespace mango::import3d
