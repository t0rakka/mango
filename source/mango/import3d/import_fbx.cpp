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

    enum MappingInformationType
    {
        ByPolygonVertex,
        ByVertice,
        ByEdge,
        AllSame,
    };

    enum ReferenceInformationType
    {
        Direct,
        IndexToDirect,
    };

    template <typename T>
    struct ArrayFBX
    {
        std::vector<T> values;
        std::vector<u32> indices;
        MappingInformationType mappingType { ByPolygonVertex };
        ReferenceInformationType referenceType { Direct };
    };

    struct MeshFBX
    {
        ArrayFBX<float32x3> positions;
        ArrayFBX<float32x3> normals;
        ArrayFBX<float32x2> texcoords;
    };

    struct Property
    {
        /*
        enum Type : u8
        {
            U8,
            U16,
            U32,
            U64,
            F32,
            ARRAY_U8,
            ARRAY_U32,
            ARRAY_U64,
            ARRAY_F32,
            STRING,
            MEMORY,
        };
        */

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

        //Type type;
        Variant value;
    };

    struct ReaderFBX
    {
        ConstMemory m_memory;
        u32 m_version;

        MappingInformationType currentMappingType { ByPolygonVertex };
        ReferenceInformationType currentReferenceType { Direct };

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
            m_version = p.read32();
            printLine(Print::Verbose, "Version: {}", m_version);

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
            u64 endOffset;
            u64 numProperties;
            u64 propertyListLength;

            if (m_version < 7500)
            {
                endOffset = p.read32();
                numProperties = p.read32();
                propertyListLength = p.read32();
            }
            else
            {
                endOffset = p.read64();
                numProperties = p.read64();
                propertyListLength = p.read64();
            }

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
                        printLine(Print::Verbose, level * 2 + 2, "raw: {} bytes", length);
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
                MeshFBX mesh;
                m_meshes.push_back(mesh);
            }
            else if (name == "MappingInformationType")
            {
                if (std::holds_alternative<std::string_view>(properties[0].value))
                {
                    auto str = std::get<std::string_view>(properties[0].value);
                    if (str == "ByPolygonVertex")
                    {
                        currentMappingType = ByPolygonVertex;
                    }
                    else if (str == "ByVertice")
                    {
                        currentMappingType = ByVertice;
                    }
                    else if (str == "ByEdge")
                    {
                        currentMappingType = ByEdge;
                    }
                    else if (str == "AllSame")
                    {
                        currentMappingType = AllSame;
                    }
                }
            }
            else if (name == "ReferenceInformationType")
            {
                if (std::holds_alternative<std::string_view>(properties[0].value))
                {
                    auto str = std::get<std::string_view>(properties[0].value);
                    if (str == "Direct")
                    {
                        currentReferenceType = Direct;
                    }
                    else if (str == "IndexToDirect")
                    {
                        currentReferenceType = IndexToDirect;
                    }
                }
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
                            // TODO: coordinate system conversion
                            float x = vec[i + 0];
                            float y = vec[i + 1];
                            float z = vec[i + 2];
                            float32x3 position(x, y, z);
                            mesh.positions.values.push_back(position);
                        }
                    }
                }
                else if (name == "PolygonVertexIndex")
                {
                    if (std::holds_alternative<std::vector<u32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<u32>>(properties[0].value);
                        mesh.positions.indices = vec;
                    }
                }
                else if (name == "Normals")
                {
                    if (std::holds_alternative<std::vector<float32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<float32>>(properties[0].value);

                        mesh.normals.mappingType = currentMappingType;
                        mesh.normals.referenceType = currentReferenceType;

                        for (size_t i = 0; i < vec.size() - 2; i += 3)
                        {
                            // TODO: coordinate system conversion
                            float x = vec[i + 0];
                            float y = vec[i + 1];
                            float z = vec[i + 2];
                            float32x3 normal(x, y, z);
                            mesh.normals.values.push_back(normal);
                        }
                    }
                }
                else if (name == "NormalsW")
                {
                    // TODO
                }
                else if (name == "UV")
                {
                    if (std::holds_alternative<std::vector<float32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<float32>>(properties[0].value);

                        mesh.texcoords.mappingType = currentMappingType;
                        mesh.texcoords.referenceType = currentReferenceType;

                        for (size_t i = 0; i < vec.size() - 1; i += 2)
                        {
                            float x = vec[i + 0];
                            float y = vec[i + 1];
                            float32x2 texcoord(x, y);
                            mesh.texcoords.values.push_back(texcoord);
                        }
                    }
                }
                else if (name == "UVIndex")
                {
                    if (std::holds_alternative<std::vector<u32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<u32>>(properties[0].value);
                        mesh.texcoords.indices = vec;
                    }
                }
                else if (name == "Smoothing")
                {
                    // TODO
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

            bool hasNormals = !current.normals.values.empty();
            bool hasTexcoords = !current.texcoords.values.empty();
            bool hasTexcoordIndices = !current.texcoords.indices.empty();

#if 0
            printLine(Print::Verbose, "positions: {}, indices: {}, normals: {}",
                current.positions.values.size(),
                current.positions.indices.size(),
                current.normals.values.size());
#endif

            if (hasTexcoords)
            {
                trimesh.flags |= Vertex::TEXCOORD;
            }

            // triangle indices
            s32 tempIndex[3];
            int count = 0;

            Triangle triangle;

            for (size_t i = 0; i < current.positions.indices.size(); ++i)
            {
                s32 index = current.positions.indices[i];

                tempIndex[count] = index < 0 ? -(index + 1) : index;

                if (hasNormals && current.normals.mappingType == ByPolygonVertex)
                {
                    triangle.vertex[count].normal = current.normals.values[i];
                }

                if (hasTexcoords)
                {
                    s32 idx = s32(hasTexcoordIndices ? current.texcoords.indices[i] : i);
                    triangle.vertex[count].texcoord = current.texcoords.values[idx];
                }

                ++count;

                if (count == 3)
                {
                    float32x3 position0 = current.positions.values[tempIndex[0]];
                    float32x3 position1 = current.positions.values[tempIndex[1]];
                    float32x3 position2 = current.positions.values[tempIndex[2]];

                    triangle.vertex[0].position = position0;
                    triangle.vertex[1].position = position1;
                    triangle.vertex[2].position = position2;

                    if (current.normals.mappingType == ByVertice)
                    {
                        triangle.vertex[0].normal = current.normals.values[tempIndex[0]];
                        triangle.vertex[1].normal = current.normals.values[tempIndex[1]];
                        triangle.vertex[2].normal = current.normals.values[tempIndex[2]];
                    }

                    if (!hasNormals)
                    {
                        // TODO: smoothing groups (need file for testing)
                        float32x3 normal = normalize(cross(position0 - position1, position0 - position2));
                        triangle.vertex[0].normal = normal;
                        triangle.vertex[1].normal = normal;
                        triangle.vertex[2].normal = normal;
                    }

                    trimesh.triangles.push_back(triangle);

                    // next vertex
                    tempIndex[1] = tempIndex[2];
                    triangle.vertex[1].normal = triangle.vertex[2].normal;
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
