/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/import3d/import_gltf.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

/*
    Khronos GLTF 2.0 importer

    https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
*/

namespace
{
    using namespace mango;

    struct Attribute
    {
        const u8* data = nullptr;
        size_t count = 0;
        size_t stride = 0;
        size_t components;
        fastgltf::ComponentType type;

        operator bool () const
        {
            return data != nullptr;
        }
    };

} // namespace

namespace mango::import3d
{

ImportGLTF::ImportGLTF(const filesystem::Path& path, const std::string& filename)
{
    u64 time0 = Time::ms();

    // --------------------------------------------------------------------------
    // read
    // --------------------------------------------------------------------------

    filesystem::File file(path, filename);

    fastgltf::GltfDataBuffer data = *fastgltf::GltfDataBuffer::FromBytes(
        reinterpret_cast<const std::byte*>(file.data()), file.size());
        
    // --------------------------------------------------------------------------
    // parse
    // --------------------------------------------------------------------------

    printLine(Print::Verbose, "[ImportGLTF]");

    auto extensions = fastgltf::Extensions::None;

    extensions |= fastgltf::Extensions::KHR_mesh_quantization;
    extensions |= fastgltf::Extensions::KHR_texture_transform;
    extensions |= fastgltf::Extensions::KHR_texture_basisu;
    extensions |= fastgltf::Extensions::MSFT_texture_dds;
    extensions |= fastgltf::Extensions::KHR_mesh_quantization;
    extensions |= fastgltf::Extensions::EXT_meshopt_compression;
    extensions |= fastgltf::Extensions::KHR_lights_punctual;
    extensions |= fastgltf::Extensions::EXT_texture_webp;
    extensions |= fastgltf::Extensions::KHR_materials_specular;
    extensions |= fastgltf::Extensions::KHR_materials_ior;
    extensions |= fastgltf::Extensions::KHR_materials_iridescence;
    extensions |= fastgltf::Extensions::KHR_materials_volume;
    extensions |= fastgltf::Extensions::KHR_materials_transmission;
    extensions |= fastgltf::Extensions::KHR_materials_clearcoat;
    extensions |= fastgltf::Extensions::KHR_materials_emissive_strength;
    extensions |= fastgltf::Extensions::KHR_materials_sheen;
    extensions |= fastgltf::Extensions::KHR_materials_unlit;
    extensions |= fastgltf::Extensions::KHR_materials_anisotropy;

    fastgltf::Parser parser(extensions);

    auto type = fastgltf::determineGltfFileType(data);
    switch (type)
    {
        case fastgltf::GltfType::glTF:
            printLine(Print::Verbose, "  Type: gltf");
            break;
        case fastgltf::GltfType::GLB:
            printLine(Print::Verbose, "  Type: glb");
            break;
        default:
            printLine(Print::Error, "Failed to determine glTF container");
            return;
    }

    auto options = fastgltf::Options::None;

    options |= fastgltf::Options::DontRequireValidAssetMember;
    options |= fastgltf::Options::AllowDouble;
    //options |= fastgltf::Options::GenerateMeshIndices; // broken - don't use
    //options |= fastgltf::Options::LoadGLBBuffers;
    //options |= fastgltf::Options::LoadExternalBuffers;
    //options |= fastgltf::Options::LoadExternalImages;
    //options |= fastgltf::Options::LoadExternalImages;

    auto expected_asset = parser.loadGltf(data, "", options);
    if (expected_asset.error() != fastgltf::Error::None)
    {
        printLine(Print::Error, "  ERROR: {}", fastgltf::getErrorMessage(expected_asset.error()).data());
        return;
    }

    fastgltf::Asset asset = std::move(expected_asset.get());

    // --------------------------------------------------------------------------
    // buffers
    // --------------------------------------------------------------------------

    std::vector<std::unique_ptr<filesystem::File>> files;
    std::vector<ConstMemory> buffers;

    for (const auto& current : asset.buffers)
    {
        printLine(Print::Verbose, "[Buffer]");

        std::visit(fastgltf::visitor
        {
            [] (const auto& arg)
            {
                printLine(Print::Verbose, "  Unknown");
            },
            [&] (const fastgltf::sources::URI& source)
            {
                //std::string filename = path.parent_path() / source.uri.path();
                std::string filename = std::string(source.uri.path().begin(), source.uri.path().end());

                auto file = std::make_unique<filesystem::File>(path, filename);
                ConstMemory memory = *file;
                buffers.push_back(memory);
                files.emplace_back(std::move(file));

                // [x] standard
                // [ ] binary
                // [ ] embedded
                printLine(Print::Verbose, "  URI: \"{}\" {} bytes", filename, memory.size);
            },
            [&] (const fastgltf::sources::ByteView& source)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());
                buffers.push_back(memory);

                // [ ] standard
                // [x] binary
                // [ ] embedded
                printLine(Print::Verbose, "  ByteView: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::BufferView& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                printLine(Print::Verbose, "  BufferView:");
            },
            [&](const fastgltf::sources::Array& array)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(array.bytes.data()), array.bytes.size());
                buffers.push_back(memory);

                // [ ] standard
                // [ ] binary
                // [x] embedded
                printLine(Print::Verbose, "  Array: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::Vector& source)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());
                buffers.push_back(memory);

                // [ ] standard
                // [ ] binary
                // [x] embedded
                printLine(Print::Verbose, "  vector: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::CustomBuffer& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                printLine(Print::Verbose, "  CustomBuffer: {}", source.id);
            },
        }, current.data);
    }

    // --------------------------------------------------------------------------
    // images
    // --------------------------------------------------------------------------

    std::vector<Texture> textures;

    for (const auto& current : asset.images)
    {
        printLine(Print::Verbose, "[Image]");

        std::visit(fastgltf::visitor
        {
            [] (const auto& arg)
            {
            },
            [&] (const fastgltf::sources::URI& source)
            {
                const std::string filename(source.uri.path().begin(), source.uri.path().end());

                auto file = std::make_unique<filesystem::File>(path, filename);
                ConstMemory memory = *file;

                Texture texture = createTexture(path, filename);
                textures.push_back(texture);

                // [x] standard
                // [ ] binary
                // [ ] embedded
                printLine(Print::Verbose, "  URI: \"{}\" {} bytes", filename, memory.size);
            },
            [&] (const fastgltf::sources::Array& source)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());

                Texture texture = createTexture(memory);
                textures.push_back(texture);

                // [ ] standard
                // [ ] binary
                // [x] embedded
                printLine(Print::Verbose, "  Array: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::Vector& source)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());

                Texture texture = createTexture(memory);
                textures.push_back(texture);

                // [ ] standard
                // [ ] binary
                // [x] embedded
                printLine(Print::Verbose, "  vector: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::ByteView& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                printLine(Print::Verbose, "  ByteView: {} bytes (not supported)", source.bytes.size());
            },
            [&] (const fastgltf::sources::BufferView& source)
            {
                auto& bufferView = asset.bufferViews[source.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];

                ConstMemory memory;

                std::visit(fastgltf::visitor
                {
                    [] (auto& arg)
                    {
                    },
                    [&](fastgltf::sources::Array& vector)
                    {
                        memory.address = reinterpret_cast<const u8*>(vector.bytes.data() + bufferView.byteOffset);
                        memory.size = bufferView.byteLength;
                    }
                }, buffer.data);

                if (memory.address)
                {
                    Texture texture = createTexture(memory);
                    textures.push_back(texture);
                }

                // [ ] standard
                // [x] binary
                // [ ] embedded
                printLine(Print::Verbose, "  BufferView: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::CustomBuffer& source)
            {
                printLine(Print::Verbose, "  CustomBuffer: TODO");
            },
        }, current.data);

    } // images

    // --------------------------------------------------------------------------
    // materials
    // --------------------------------------------------------------------------

    for (const auto& current : asset.materials)
    {
        printLine(Print::Verbose, "[Material]");
        printLine(Print::Verbose, "  name: \"{}\"", current.name);

        Material material;

        const auto* emissiveFactor = current.emissiveFactor.data();

        const fastgltf::PBRData& pbr = current.pbrData;
        const auto* baseColorFactor = pbr.baseColorFactor.data();

        printLine(Print::Verbose, "  baseColorFactor: {} {} {} {}", baseColorFactor[0], baseColorFactor[1], baseColorFactor[2], baseColorFactor[3]);
        printLine(Print::Verbose, "  metallicFactor: {}", pbr.metallicFactor);
        printLine(Print::Verbose, "  roughnessFactor: {}", pbr.roughnessFactor);
        printLine(Print::Verbose, "  emissiveFactor: {} {} {}",
            current.emissiveFactor[0],
            current.emissiveFactor[1],
            current.emissiveFactor[2]);

        material.roughnessFactor = pbr.roughnessFactor;
        material.metallicFactor = pbr.metallicFactor;

        material.baseColorFactor[0] = pbr.baseColorFactor[0];
        material.baseColorFactor[1] = pbr.baseColorFactor[1];
        material.baseColorFactor[2] = pbr.baseColorFactor[2];
        material.baseColorFactor[3] = pbr.baseColorFactor[3];

        material.emissiveFactor[0] = emissiveFactor[0];
        material.emissiveFactor[1] = emissiveFactor[1];
        material.emissiveFactor[2] = emissiveFactor[2];

        if (pbr.baseColorTexture.has_value())
        {
            fastgltf::Texture& texture = asset.textures[pbr.baseColorTexture->textureIndex];
            if (texture.imageIndex.has_value())
            {
                Texture image = textures[*texture.imageIndex];
                material.baseColorTexture = image;
                printLine(Print::Verbose, "  baseColorTexture: ({} x {})", image->width, image->height);
            }
        }

        if (pbr.metallicRoughnessTexture.has_value())
        {
            fastgltf::Texture& texture = asset.textures[pbr.metallicRoughnessTexture->textureIndex];
            if (texture.imageIndex.has_value())
            {
                Texture image = textures[*texture.imageIndex];
                material.metallicRoughnessTexture = image;
                printLine(Print::Verbose, "  metallicRoughnessTexture: ({} x {})", image->width, image->height);
            }
        }

        if (current.normalTexture.has_value())
        {
            fastgltf::Texture& texture = asset.textures[current.normalTexture->textureIndex];
            if (texture.imageIndex.has_value())
            {
                Texture image = textures[*texture.imageIndex];
                material.normalTexture = image;
                printLine(Print::Verbose, "  normalTexture: ({} x {})", image->width, image->height);
            }
        }

        if (current.occlusionTexture.has_value())
        {
            fastgltf::Texture& texture = asset.textures[current.occlusionTexture->textureIndex];
            if (texture.imageIndex.has_value())
            {
                Texture image = textures[*texture.imageIndex];
                material.occlusionTexture = image;
                printLine(Print::Verbose, "  occlusionTexture: ({} x {})", image->width, image->height);
            }
        }

        if (current.emissiveTexture.has_value())
        {
            fastgltf::Texture& texture = asset.textures[current.emissiveTexture->textureIndex];
            if (texture.imageIndex.has_value())
            {
                Texture image = textures[*texture.imageIndex];
                material.emissiveTexture = image;
                printLine(Print::Verbose, "  emissiveTexture: ({} x {})", image->width, image->height);
            }
        }

        switch (current.alphaMode)
        {
            case fastgltf::AlphaMode::Opaque:
                material.alphaMode = Material::AlphaMode::Opaque;
                break;

            case fastgltf::AlphaMode::Mask:
                material.alphaMode = Material::AlphaMode::Mask;
                break;

            case fastgltf::AlphaMode::Blend:
                material.alphaMode = Material::AlphaMode::Blend;
                break;
        }

        material.alphaCutoff = current.alphaCutoff;
        material.twosided = current.doubleSided;

        materials.push_back(material);

        if (current.clearcoat)
        {
            printLine(Print::Verbose, "  Clearcoat: TODO");
        }

        if (current.iridescence)
        {
            printLine(Print::Verbose, "  Iridescence: TODO");
        }

        if (current.sheen)
        {
            printLine(Print::Verbose, "  Sheen: TODO");
        }

        if (current.specular)
        {
            printLine(Print::Verbose, "  Specular: TODO");
        }

        if (current.transmission)
        {
            printLine(Print::Verbose, "  Transmission: TODO");
        }

        if (current.volume)
        {
            printLine(Print::Verbose, "  Volume: TODO");
        }

        /*

        struct MaterialClearcoat
        {
            float clearcoatFactor;
            std::optional<TextureInfo> clearcoatTexture;
            float clearcoatRoughnessFactor;
            std::optional<TextureInfo> clearcoatRoughnessTexture;
            std::optional<TextureInfo> clearcoatNormalTexture;
        };

        struct MaterialIridescence
        {
            float iridescenceFactor;
            std::optional<TextureInfo> iridescenceTexture;
            float iridescenceIor;
            float iridescenceThicknessMinimum;
            float iridescenceThicknessMaximum;
            std::optional<TextureInfo> iridescenceThicknessTexture;
        };

        struct MaterialSheen
        {
            std::array<float, 3> sheenColorFactor;
            std::optional<TextureInfo> sheenColorTexture;
            float sheenRoughnessFactor;
            std::optional<TextureInfo> sheenRoughnessTexture;
        };

        struct MaterialSpecular
        {
            float specularFactor;
            std::optional<TextureInfo> specularTexture;
            std::array<float, 3> specularColorFactor;
            std::optional<TextureInfo> specularColorTexture;
        };

        struct MaterialTransmission
        {
            float transmissionFactor;
            std::optional<TextureInfo> transmissionTexture;
        };

        struct MaterialVolume
        {
            float thicknessFactor;
            std::optional<TextureInfo> thicknessTexture;
            float attenuationDistance;
            std::array<float, 3> attenuationColor;
        };

        struct Material
        {
            std::optional<float> emissiveStrength;
            std::optional<float> ior;
            bool unlit;
        };

        */

    } // materials

    if (materials.empty())
    {
        // Add default material
        Material material;
        materials.push_back(material);
    }

    // --------------------------------------------------------------------------
    // meshes
    // --------------------------------------------------------------------------

    for (const auto& current : asset.meshes)
    {
        printLine(Print::Verbose, "[Mesh]");
        printLine(Print::Verbose, "  name: \"{}\"", current.name);

        std::unique_ptr<IndexedMesh> ptr = std::make_unique<IndexedMesh>();
        IndexedMesh& mesh = *ptr;

        for (auto primitiveIterator = current.primitives.begin(); primitiveIterator != current.primitives.end(); ++primitiveIterator)
        {
            printLine(Print::Verbose, "  [primitive]");

            Attribute attributePosition;
            Attribute attributeNormal;
            Attribute attributeTangent;
            Attribute attributeTexcoord;
            Attribute attributeColor;

            for (auto attributeIterator = primitiveIterator->attributes.begin(); attributeIterator != primitiveIterator->attributes.end(); ++attributeIterator)
            {
                auto name = attributeIterator->name;

                Attribute* attribute = nullptr;
                const char* message = "";

                if (name == "POSITION")
                {
                    attribute = &attributePosition;
                    mesh.flags |= Vertex::Position;
                }
                else if (name == "NORMAL")
                {
                    attribute = &attributeNormal;
                    mesh.flags |= Vertex::Normal;
                }
                else if (name == "TANGENT")
                {
                    attribute = &attributeTangent;
                    mesh.flags |= Vertex::Tangent;
                }
                else if (name == "TEXCOORD_0")
                {
                    attribute = &attributeTexcoord;
                    mesh.flags |= Vertex::Texcoord;
                }
                else if (name == "COLOR_0")
                {
                    attribute = &attributeColor;
                    mesh.flags |= Vertex::Color;
                }
                else
                {
                    message = " : NOT SUPPORTED!";
                }

                printLine(Print::Verbose, "    [Attribute:\"{}\"{}]", name, message);

                auto& accessor = asset.accessors[attributeIterator->accessorIndex];
                auto& view = asset.bufferViews[accessor.bufferViewIndex.value()];

                auto offset = view.byteOffset + accessor.byteOffset;
                size_t index = view.bufferIndex;
                size_t count = accessor.count;

                size_t stride;
                if (view.byteStride.has_value())
                {
                    stride = view.byteStride.value();
                }
                else
                {
                    stride = fastgltf::getElementByteSize(accessor.type, accessor.componentType);
                }

                size_t components = fastgltf::getNumComponents(accessor.type);

                switch (accessor.componentType)
                {
                    case fastgltf::ComponentType::UnsignedByte:
                        printLine(Print::Verbose, "      type: u8 x {}", components);
                        break;
                    case fastgltf::ComponentType::UnsignedShort:
                        printLine(Print::Verbose, "      type: u16 x {}", components);
                        break;
                    case fastgltf::ComponentType::Float:
                        printLine(Print::Verbose, "      type: f32 x {}", components);
                        break;
                    default:
                        printLine(Print::Verbose, "      type: NOT SUPPORTED");
                        break;
                }

                printLine(Print::Verbose, "      stride: {}", stride);
                printLine(Print::Verbose, "      count: {}", count);

                if (attribute)
                {
                    attribute->data = buffers[index].address + offset;
                    attribute->count = count;
                    attribute->stride = stride;
                    attribute->components = components;
                    attribute->type = accessor.componentType;
                }

            } // attributeIterator

            // vertices

            std::vector<Vertex> vertices(attributePosition.count);

            if (attributePosition)
            {
                const u8* data = attributePosition.data;

                for (size_t i = 0; i < attributePosition.count; ++i)
                {
                    float x = uload32f(data + 0);
                    float y = uload32f(data + 4);
                    float z = uload32f(data + 8);
                    float32x3 position(x, y, -z);

                    data += attributePosition.stride;

                    vertices[i].position = position;

                    mesh.boundingBox.extend(position);
                }
            }
            else
            {
                // position attribute is required
                continue;
            }

            if (attributeNormal)
            {
                if (attributeNormal.count != attributePosition.count)
                {
                    // attribute counts must be identical
                    continue;
                }

                const u8* data = attributeNormal.data;

                for (size_t i = 0; i < attributeNormal.count; ++i)
                {
                    float x = uload32f(data + 0);
                    float y = uload32f(data + 4);
                    float z = uload32f(data + 8);
                    float32x3 normal(x, y, -z);

                    data += attributeNormal.stride;

                    vertices[i].normal = normal;
                }
            }

            if (attributeTangent)
            {
                if (attributeTangent.count != attributePosition.count)
                {
                    // attribute counts must be identical
                    continue;
                }

                const u8* data = attributeTangent.data;

                for (size_t i = 0; i < attributeTangent.count; ++i)
                {
                    float x = uload32f(data + 0);
                    float y = uload32f(data + 4);
                    float z = uload32f(data + 8);
                    float w = uload32f(data + 12);
                    float32x4 tangent(x, y, -z, w);

                    data += attributeTangent.stride;

                    vertices[i].tangent = tangent;
                }
            }

            if (attributeTexcoord)
            {
                if (attributeTexcoord.count != attributePosition.count)
                {
                    // attribute counts must be identical
                    continue;
                }

                const u8* data = attributeTexcoord.data;

                for (size_t i = 0; i < attributeTexcoord.count; ++i)
                {
                    // TODO: u8, u16
                    float32x2 texcoord = float32x2::uload(data);
                    data += attributeTexcoord.stride;

                    vertices[i].texcoord = texcoord;
                }
            }

            if (attributeColor)
            {
                if (attributeColor.count != attributePosition.count)
                {
                    // attribute counts must be identical
                    continue;
                }

                const u8* data = attributeColor.data;

                for (size_t i = 0; i < attributeColor.count; ++i)
                {
                    // TODO: 3 and 4 components
                    // TODO: u8, u16
                    float32x3 color = float32x3::uload(data);
                    data += attributeColor.stride;

                    vertices[i].color = float32x4(color, 1.0f);
                }
            }

            // indices

            std::vector<u32> indices;

            if (primitiveIterator->indicesAccessor.has_value())
            {
                auto& indicesAccessor = asset.accessors[primitiveIterator->indicesAccessor.value()];
                if (indicesAccessor.bufferViewIndex.has_value())
                {
                    auto& indicesView = asset.bufferViews[indicesAccessor.bufferViewIndex.value()];

                    size_t offset = indicesView.byteOffset + indicesAccessor.byteOffset;
                    size_t count = indicesAccessor.count;

                    size_t bufferIndex = indicesView.bufferIndex;
                    const u8* data = buffers[bufferIndex].address + offset;

                    printLine(Print::Verbose, "    [Indices]");
                    printLine(Print::Verbose, "      count: {}", count);

                    if (count < 3)
                    {
                        // not enough indices
                        continue;
                    }

                    indices.resize(count);

                    switch (indicesAccessor.componentType)
                    {
                        case fastgltf::ComponentType::UnsignedByte:
                        {
                            for (size_t i = 0; i < count; ++i)
                            {
                                u8 index = data[i];
                                indices[i] = index == 0xff ? 0xffffffff : index;
                            }
                            break;
                        }

                        case fastgltf::ComponentType::UnsignedShort:
                        {
                            for (size_t i = 0; i < count; ++i)
                            {
                                u16 index = uload16(data + i * 2);
                                indices[i] = index == 0xffff ? 0xffffffff : index;
                            }
                            break;
                        }

                        case fastgltf::ComponentType::UnsignedInt:
                        {
                            for (size_t i = 0; i < count; ++i)
                            {
                                indices[i] = uload32(data + i * 4);
                            }
                            break;
                        }

                        default:
                            break;
                    }
                }
            }

            bool needIndices = false;

            if (indices.empty())
            {
                needIndices = true;

                for (size_t i = 0; i < vertices.size(); ++i)
                {
                    u32 index = u32(i);
                    indices.push_back(index);
                }
            }

            const size_t materialIndex = primitiveIterator->materialIndex.has_value() ?
                primitiveIterator->materialIndex.value() : 0;

            const Material& material = materials[materialIndex];

            bool needTangent = false;

            if (material.normalTexture)
            {
                if (!attributeTangent && attributeNormal && attributeTexcoord)
                {
                    needTangent = true;
                }
            }

            if (needTangent || needIndices)
            {
                Mesh trimesh;

                trimesh.flags = mesh.flags;

                // TODO: support primitive restart (index: 0xffffffff)

                switch (primitiveIterator->type)
                {
                    case fastgltf::PrimitiveType::Triangles:
                    {
                        for (size_t i = 2; i < indices.size(); i += 3)
                        {
                            Triangle triangle;

                            triangle.vertex[0] = vertices[indices[i - 0]];
                            triangle.vertex[1] = vertices[indices[i - 1]];
                            triangle.vertex[2] = vertices[indices[i - 2]];

                            trimesh.triangles.push_back(triangle);
                        }
                        break;
                    }

                    case fastgltf::PrimitiveType::TriangleStrip:
                    {
                        Vertex v0 = vertices[indices[0]];
                        Vertex v1 = vertices[indices[1]];

                        for (size_t i = 2; i < indices.size(); ++i)
                        {
                            Triangle triangle;

                            triangle.vertex[(i + 1) & 1] = v0;
                            triangle.vertex[(i + 0) & 1] = v1;
                            triangle.vertex[2] = vertices[indices[i]];

                            trimesh.triangles.push_back(triangle);

                            v0 = v1;
                            v1 = triangle.vertex[2];
                        }
                        break;
                    }

                    case fastgltf::PrimitiveType::TriangleFan:
                    {
                        Triangle triangle;

                        triangle.vertex[0] = vertices[indices[0]];
                        triangle.vertex[1] = vertices[indices[1]];

                        for (size_t i = 2; i < indices.size(); ++i)
                        {
                            triangle.vertex[2] = triangle.vertex[1];
                            triangle.vertex[1] = vertices[indices[i]];

                            trimesh.triangles.push_back(triangle);
                        }
                        break;
                    }

                    default:
                        // unsupported primitive type
                        continue;
                }

                u64 time0 = Time::us();

                if (needTangent)
                {
                    trimesh.computeTangents();
                }

                u64 time1 = Time::us();

                mesh.append(trimesh, u32(materialIndex));

                u64 time2 = Time::us();
                u64 delta0 = time1 - time0;
                u64 delta1 = time2 - time1;

                printLine(Print::Verbose, "    Computing tangents: {}.{} ms", delta0 / 1000, delta0 % 1000);
                printLine(Print::Verbose, "    Mesh Indexing: {}.{} ms", delta1 / 1000, delta1 % 1000);
            }
            else
            {
                Primitive primitive;

                // re-generate indices because coordinate system conversion
                // reverses winding (front faces are now ccw, we want cw)
                std::vector<u32> temp;

                // TODO: support primitive restart (index: 0xffffffff)

                switch (primitiveIterator->type)
                {
                    case fastgltf::PrimitiveType::Triangles:
                    {
                        primitive.type = Primitive::Type::TriangleList;

                        for (size_t i = 0; i < indices.size(); i += 3)
                        {
                            temp.push_back(indices[i + 0]);
                            temp.push_back(indices[i + 2]);
                            temp.push_back(indices[i + 1]);
                        }
                        break;
                    }

                    case fastgltf::PrimitiveType::TriangleStrip:
                    {
                        primitive.type = Primitive::Type::TriangleList;

                        u32 index0 = indices[0];
                        u32 index1 = indices[1];

                        for (size_t i = 2; i < indices.size(); ++i)
                        {
                            if (i & 1)
                            {
                                temp.push_back(index0);
                                temp.push_back(index1);
                            }
                            else
                            {
                                temp.push_back(index1);
                                temp.push_back(index0);
                            }

                            u32 index2 = indices[i];
                            temp.push_back(index2);

                            index0 = index1;
                            index1 = index2;
                        }

                        break;
                    }

                    case fastgltf::PrimitiveType::TriangleFan:
                    {
                        primitive.type = Primitive::Type::TriangleFan;

                        temp.push_back(indices[0]);

                        const size_t size = indices.size();

                        for (size_t i = 1; i < size; ++i)
                        {
                            temp.push_back(indices[size - i]);
                        }
                        break;
                    }

                    default:
                        // unsupported primitive type
                        continue;
                }

                // use the re-generated indices
                std::swap(indices, temp);

                primitive.start = u32(mesh.indices.size());
                primitive.count = u32(indices.size());
                primitive.base = u32(mesh.vertices.size());
                primitive.material = u32(materialIndex);

                mesh.primitives.push_back(primitive);

                mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());
                mesh.indices.insert(mesh.indices.end(), indices.begin(), indices.end());
            }
        }

        meshes.push_back(std::move(ptr));
    }

    // --------------------------------------------------------------------------
    // nodes
    // --------------------------------------------------------------------------

    for (const auto& current : asset.nodes)
    {
        Node node;

        if (const auto* matrix = std::get_if<fastgltf::math::fmat4x4>(&current.transform))
        {
            const float* data = matrix->data();
            node.transform = matrix4x4(data);
        }
        else if (const auto* trs = std::get_if<fastgltf::TRS>(&current.transform))
        {
            const float* t = trs->translation.data();
            const float* r = trs->rotation.data();
            const float* s = trs->scale.data();

            matrix4x4 translation = matrix4x4::translate(t[0], t[1], t[2]);
            matrix4x4 rotation(math::Quaternion(r[0], r[1], r[2], r[3]));
            matrix4x4 scale = matrix4x4::scale(s[0], s[1], s[2]);

            node.transform = scale * rotation * translation;
        }

        // coordinate system conversion
        matrix4x4& m = node.transform;
        m[0] = float32x4( m[0][0], m[0][1],-m[0][2], m[0][3]);
        m[1] = float32x4( m[1][0], m[1][1],-m[1][2], m[1][3]);
        m[2] = float32x4(-m[2][0],-m[2][1], m[2][2],-m[2][3]);
        m[3] = float32x4( m[3][0], m[3][1],-m[3][2], m[3][3]);

        node.name = current.name;

        for (auto child : current.children)
        {
            node.children.push_back(u32(child));
        }

        if (current.meshIndex)
        {
            node.mesh = u32(current.meshIndex.value());
        }

        nodes.push_back(node);
    }

    // --------------------------------------------------------------------------
    // scenes
    // --------------------------------------------------------------------------

    for (const auto& current : asset.scenes)
    {
        printLine(Print::Verbose, "[Scene]");
        printLine(Print::Verbose, "  nodeIndices: {}", current.nodeIndices.size());

        for (auto nodeIndex : current.nodeIndices)
        {
            roots.push_back(u32(nodeIndex));
        }
    }

    // --------------------------------------------------------------------------
    // summary
    // --------------------------------------------------------------------------

    printLine(Print::Verbose, "[Summary]");
    printLine(Print::Verbose, "  Buffers:   {}", asset.buffers.size());
    printLine(Print::Verbose, "  Images:    {}", asset.images.size());
    printLine(Print::Verbose, "  Materials: {}", asset.materials.size());
    printLine(Print::Verbose, "  Meshes:    {}", asset.meshes.size());
    printLine(Print::Verbose, "  Nodes:     {}", asset.nodes.size());
    printLine(Print::Verbose, "  Scenes:    {}", asset.scenes.size());

    u64 time1 = Time::ms();
    printLine(Print::Verbose, "Time: {} ms", time1 - time0);
}

} // namespace mango::import3d
