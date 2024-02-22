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

            if (numProperties > 0)
            {
                printLine(Print::Verbose, level * 2 + 2, "count: {} ({} bytes)", numProperties, propertyListLength);
            }

            p += propertyListLength;

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
