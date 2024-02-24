/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <variant>
#include <mango/core/core.hpp>
#include <mango/import3d/import_fbx.hpp>

/*
    Autodesk FBX importer
*/

namespace
{
    using namespace mango;
    using namespace mango::import3d;

    struct MeshFBX
    {
        std::vector<float32x3> positions;
        std::vector<u32> indices;
    };

    struct Property
    {
        enum Type : u8
        {
            U8,
            U16,
            U32,
            U64,
            F32,
            VECTOR_U8,
            VECTOR_U32,
            VECTOR_U64,
            VECTOR_F32,
            STRING,
            MEMORY,
        };

        using Variant = std::variant<
            u8,
            u16,
            u32,
            u64,
            float32,
            std::vector<u8>,
            std::vector<u32>,
            std::vector<u64>,
            std::vector<float32>,
            std::string_view,
            ConstMemory>;

        Type type;
        Variant value;
    };

    struct ReaderFBX
    {
        ConstMemory m_memory;

        std::vector<MeshFBX> m_meshes;

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

        template <typename D, typename S>
        void read_values(std::vector<D>& output, const u8* p, u32 count)
        {
            while (count-- > 0)
            {
                S value;
                std::memcpy(&value, p, sizeof(S));
                p += sizeof(S);
#if !defined(MANGO_LITTLE_ENDIAN)
                value = byteswap(value);
#endif
                output.push_back(D(value));
            }
        }

        template <typename D, typename S>
        std::vector<D> read_property_array(LittleEndianConstPointer& p)
        {
            u32 length = p.read32();
            u32 encoding = p.read32();
            u32 compressed = p.read32();

            std::vector<D> output;

            if (encoding)
            {
                Buffer buffer(length * sizeof(S));
                CompressionStatus status = deflate_zlib::decompress(buffer, ConstMemory(p, compressed));

                read_values<D, S>(output, buffer, length);
                p += compressed;
            }
            else
            {
                read_values<D, S>(output, p, length);
                p += length * sizeof(S);
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

            const u8* end = m_memory.address + endOffset;

            switch (level)
            {
                case 0:
                    if (name != "Objects")
                    {
                        return end;
                    }
                    break;

                case 1:
                    if (name != "Geometry")
                    {
                        return end;
                    }
                    break;

                default:
                    break;
            }

            const u8* next = p + propertyListLength;

            std::vector<Property> properties(numProperties);

            for (Property& property : properties)
            {
                char type = *p++;

                switch (type)
                {
                    case 'C':
                    case 'B':
                    {
                        u8 value = *p++;
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "u8: {}", value);
                        break;
                    }

                    case 'Y':
                    {
                        u16 value = p.read16();
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "u16: {}", value);
                        break;
                    }

                    case 'I':
                    {
                        u32 value = p.read32();
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "u32: {}", value);
                        break;
                    }

                    case 'L':
                    {
                        u64 value = p.read64();
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "u64: {}", value);
                        break;
                    }

                    case 'F':
                    {
                        float value = p.read32f();
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "f32: {}", value);
                        break;
                    }

                    case 'D':
                    {
                        double value = p.read64f();
                        property.value = float(value);
                        printLine(Print::Verbose, level * 2 + 2, "f64: {}", value);
                        break;
                    }

                    case 'f':
                    {
                        auto value = read_property_array<float, float>(p);
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "f32[{}]", value.size());
                        break;
                    }

                    case 'd':
                    {
                        auto value = read_property_array<float, double>(p);
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "f64[{}]", value.size());
                        break;
                    }

                    case 'l':
                    {
                        auto value = read_property_array<u64, u64>(p);
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "u64[{}]", value.size());
                        break;
                    }

                    case 'i':
                    {
                        auto value = read_property_array<u32, u32>(p);
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "u32[{}]", value.size());
                        break;
                    }

                    case 'b':
                    {
                        auto value = read_property_array<u8, u8>(p);
                        property.value = value;
                        printLine(Print::Verbose, level * 2 + 2, "u8[{}]", value.size());
                        break;
                    }

                    case 'S':
                    {
                        u32 length = p.read32();
                        std::string_view value(p.cast<const char>(), length);
                        property.value = value;
                        p += length;
                        printLine(Print::Verbose, level * 2 + 2, "string: \"{}\"", value);
                        break;
                    }

                    case 'R':
                    {
                        u32 length = p.read32();
                        ConstMemory value(p, length);
                        property.value = value;
                        p += length;
                        printLine(Print::Verbose, level * 2 + 2, "raw: {} {} bytes", type, length);
                        break;
                    }

                    default:
                        MANGO_EXCEPTION("[ImportFBX] Incorrect property type.");
                        break;
                }

                // TODO: compute properties mask here
            }

            p = next;

            if (name == "Geometry")
            {
                //printLine(Print::Verbose, "-- starting new geometry");
                // TODO: check that it is "Mesh"

                MeshFBX mesh;
                m_meshes.push_back(mesh);
            }

            if (!m_meshes.empty())
            {
                MeshFBX& mesh = m_meshes.back();

                if (name == "Vertices")
                {
                    if (std::holds_alternative<std::vector<float32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<float32>>(properties[0].value);

                        for (size_t i = 0; i < vec.size() - 2; i += 3)
                        {
                            float x = vec[i + 0];
                            float y = vec[i + 1];
                            float z = vec[i + 2];
                            float32x3 position(x, y, z);
                            mesh.positions.push_back(position);
                        }

                        //printLine("-- positions: {}", mesh.positions.size());
                    }
                    // 51750 floats -> 17250 positions
                }
                else if (name == "PolygonVertexIndex")
                {
                    if (std::holds_alternative<std::vector<u32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<u32>>(properties[0].value);
                        mesh.indices = vec;
                    }
                    // 100629 indices -> 33542 triangles
                }
                else if (name == "Normals")
                {
                    // MappingInformationType = ByPolygonVertex
                    // ReferenceInformationType = Direct
                    // 301887 floats -> 100629 normals (33542 triangles)
                }
                else if (name == "UV")
                {
                    // MappingInformationType = ByPolygonVertex
                    // ReferenceInformationType = IndexToDirect
                    // 47874 floats -> 23936 texcoords
                }
                else if (name == "UVIndex")
                {
                    // MappingInformationType = ByPolygonVertex
                    // ReferenceInformationType = IndexToDirect
                    // 100629 indices -> 33542 triangles
                }
            }

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

        Material material;
        materials.push_back(material);

        std::unique_ptr<IndexedMesh> ptr = std::make_unique<IndexedMesh>();
        IndexedMesh& mesh = *ptr;

        for (const auto& current : reader.m_meshes)
        {
            Mesh trimesh;
            trimesh.flags = Vertex::POSITION | Vertex::NORMAL;

            // triangle indices
            s32 temp[3];
            int count = 0;

            for (size_t i = 0; i < current.indices.size(); ++i)
            {
                s32 index = current.indices[i];

                temp[count++] = index < 0 ? -(index + 1) : index;
                if (count == 3)
                {
                    float32x3 p0 = current.positions[temp[0]];
                    float32x3 p1 = current.positions[temp[1]];
                    float32x3 p2 = current.positions[temp[2]];

                    float32x3 normal = normalize(cross(p0 - p1, p0 - p2));

                    Triangle triangle;

                    triangle.vertex[0].position = p0;
                    triangle.vertex[1].position = p1;
                    triangle.vertex[2].position = p2;

                    triangle.vertex[0].normal = normal;
                    triangle.vertex[1].normal = normal;
                    triangle.vertex[2].normal = normal;

                    trimesh.triangles.push_back(triangle);

                    // next vertex
                    temp[1] = temp[2];
                    --count;
                }

                if (index < 0)
                {
                    // start a new polygon
                    count = 0;
                }
            }

            mesh.append(trimesh, 0);
        }

        meshes.push_back(std::move(ptr));

        Node node;

        node.name = "FBX.object";
        node.transform = matrix4x4(1.0f);
        node.mesh = 0;

        nodes.push_back(node);
        roots.push_back(0);
    }

} // namespace mango::import3d
