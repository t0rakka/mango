/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/import3d/import_gltf.hpp>

#include <fastgltf/parser.hpp>
#include <fastgltf/types.hpp>

namespace mango::import3d
{

ImportGLTF::ImportGLTF(const filesystem::Path& path, const std::string& filename)
{
    u64 time0 = Time::ms();

    filesystem::File file(path, filename);

    Buffer filebuffer(file);
    filebuffer.append(nullptr, fastgltf::getGltfBufferPadding());

    fastgltf::GltfDataBuffer data;
    data.fromByteView(filebuffer.data(), filebuffer.size() - fastgltf::getGltfBufferPadding(), filebuffer.size());

    //debugPrintLine("BufferSize: %d", int(data.getBufferSize()));

    // --------------------------------------------------------------------------
    // parse
    // --------------------------------------------------------------------------

    debugPrintLine("[ImportGLTF]");

    auto gltfOptions = 
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble;
    //gltfOptions |= fastgltf::Options::LoadGLBBuffers;
    //gltfOptions |= fastgltf::Options::LoadExternalBuffers;
    //gltfOptions |= fastgltf::Options::LoadExternalImages;

    fastgltf::Parser parser(fastgltf::Extensions::KHR_mesh_quantization);

    auto type = fastgltf::determineGltfFileType(&data);
    if (type == fastgltf::GltfType::glTF)
    {
        debugPrintLine("  Type: gltf");
    }
    else if (type == fastgltf::GltfType::GLB)
    {
        debugPrintLine("  Type: glb");
    }
    else
    {
        debugPrintLine("Failed to determine glTF container");
        return;
    }

    auto expected = parser.loadGltf(&data, "", gltfOptions);
    fastgltf::Asset asset = std::move(expected.get());

    // --------------------------------------------------------------------------
    // buffers
    // --------------------------------------------------------------------------

    std::vector<std::unique_ptr<filesystem::File>> files;
    std::vector<ConstMemory> buffers;

    for (const auto& cbuffer : asset.buffers)
    {
        debugPrintLine("[Buffer]");

        /*
        std::cout << "  ByteLength: " << buffer.byteLength << std::endl;
        std::cout << "  BufferView:   " << std::holds_alternative<fastgltf::sources::BufferView>(buffer.data) << std::endl;
        std::cout << "  URI:          " << std::holds_alternative<fastgltf::sources::URI>(buffer.data) << std::endl;
        std::cout << "  Vector:       " << std::holds_alternative<fastgltf::sources::Vector>(buffer.data) << std::endl;
        std::cout << "  CustomBuffer: " << std::holds_alternative<fastgltf::sources::CustomBuffer>(buffer.data) << std::endl;
        std::cout << "  ByteView:     " << std::holds_alternative<fastgltf::sources::ByteView>(buffer.data) << std::endl;
        */

        std::visit(fastgltf::visitor
        {
            [] (const auto& arg)
            {
            },
            [&] (const fastgltf::sources::URI& source)
            {
                //std::string filename = path.parent_path() / source.uri.path();
                std::string filename = std::string(source.uri.path().begin(), source.uri.path().end());

                auto file = std::make_unique<filesystem::File>(path, filename);
                buffers.push_back(*file);
                files.emplace_back(std::move(file));

                // [x] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  URI: %s", filename.c_str());
            },
			[&] (const fastgltf::sources::ByteView& source)
            {
                ConstMemory memory = ConstMemory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());
                buffers.push_back(memory);

                // [ ] standard
                // [x] binary
                // [ ] embedded
                debugPrintLine("  ByteView: %d bytes", int(memory.size));
			},
            [&] (const fastgltf::sources::BufferView& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  BufferView:");
            },
            [&] (const fastgltf::sources::Vector& source)
            {
                ConstMemory memory = ConstMemory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());
                buffers.push_back(memory);

                // [ ] standard
                // [ ] binary
                // [x] embedded
                debugPrintLine("  vector: %d bytes", int(memory.size));
            },
            [&] (const fastgltf::sources::CustomBuffer& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  CustomBuffer: %d", int(source.id));
            },
        }, cbuffer.data);
    }

    // --------------------------------------------------------------------------
    // images
    // --------------------------------------------------------------------------

    std::vector<Texture> m_images;

    for (const auto& cimage : asset.images)
    {
        debugPrintLine("[Image]");

        /*
        std::cout << "  BufferView:   " << std::holds_alternative<fastgltf::sources::BufferView>(image.data) << std::endl;
        std::cout << "  URI:          " << std::holds_alternative<fastgltf::sources::URI>(image.data) << std::endl;
        std::cout << "  Vector:       " << std::holds_alternative<fastgltf::sources::Vector>(image.data) << std::endl;
        std::cout << "  CustomBuffer: " << std::holds_alternative<fastgltf::sources::CustomBuffer>(image.data) << std::endl;
        std::cout << "  ByteView:     " << std::holds_alternative<fastgltf::sources::ByteView>(image.data) << std::endl;
        */

        std::visit(fastgltf::visitor
        {
            [] (const auto& arg)
            {
            },
            [&] (const fastgltf::sources::URI& source)
            {
                //assert(source.fileByteOffset == 0); // We don't support offsets with stbi.
                //assert(source.uri.isLocalPath()); // We're only capable of loading local files.

                const std::string filename(source.uri.path().begin(), source.uri.path().end());

                auto file = std::make_unique<filesystem::File>(path, filename);
                ConstMemory memory = *file;

                Texture texture;
                loadTexture(texture, path, filename);
                m_images.push_back(texture);

                // [x] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  URI: %s : %d bytes", filename.c_str(), u32(memory.size));
            },
            [&] (const fastgltf::sources::Vector& source)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());
                //std::cout << "  ImageFormat: " << getImageFormat(memory) << std::endl;

                Texture texture;
                loadTexture(texture, memory);
                m_images.push_back(texture);

                // [ ] standard
                // [ ] binary
                // [x] embedded
                debugPrintLine("  vector: %d bytes", u32(memory.size));
            },
			[&] (const fastgltf::sources::ByteView& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  ByteView: %d bytes (not supported)", u32(source.bytes.size()));
			},
            [&] (const fastgltf::sources::BufferView& source)
            {
                auto& bufferView = asset.bufferViews[source.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];
                auto span = std::get<fastgltf::sources::ByteView>(buffer.data).bytes;

                ConstMemory memory(reinterpret_cast<const u8*>(span.data() + bufferView.byteOffset), bufferView.byteLength);
                //std::cout << "  ImageFormat: " << getImageFormat(memory) << std::endl;

                Texture texture;
                loadTexture(texture, memory);
                m_images.push_back(texture);

                // [ ] standard
                // [x] binary
                // [ ] embedded
                debugPrintLine("  BufferView: %d bytes", u32(memory.size));
            },
        }, cimage.data);

    }

    // --------------------------------------------------------------------------
    // materials
    // --------------------------------------------------------------------------

    for (const auto& cmaterial : asset.materials)
    {
        debugPrintLine("[Material]");
        debugPrintLine("  name %s", cmaterial.name.c_str());

        const fastgltf::PBRData& pbr = cmaterial.pbrData;
        const auto* baseColor = pbr.baseColorFactor.data();

        debugPrintLine("  [PBR]");
        debugPrintLine("    baseColorFactor: %f %f %f %f", baseColor[0], baseColor[1], baseColor[2], baseColor[3]);
        debugPrintLine("    metallicFactor: %f", pbr.metallicFactor);
        debugPrintLine("    roughnessFactor: %f", pbr.roughnessFactor);

        if (pbr.baseColorTexture.has_value())
        {
            fastgltf::Texture& texture = asset.textures[pbr.baseColorTexture->textureIndex];
            if (texture.imageIndex.has_value())
            {
                Texture image = m_images[*texture.imageIndex];
                debugPrintLine("    baseColorTexture: (%d x %d)", image->width, image->height);
            }
        }

        if (pbr.metallicRoughnessTexture.has_value())
        {
            fastgltf::Texture& texture = asset.textures[pbr.metallicRoughnessTexture->textureIndex];
            if (texture.imageIndex.has_value())
            {
                Texture image = m_images[*texture.imageIndex];
                debugPrintLine("    metallicRoughnessTexture: (%d x %d)", image->width, image->height);
            }
        }

        if (cmaterial.normalTexture.has_value())
        {
                fastgltf::Texture& texture = asset.textures[cmaterial.normalTexture->textureIndex];
                if (texture.imageIndex.has_value())
                {
                    Texture image = m_images[*texture.imageIndex];
                    debugPrintLine("    normalTexture: (%d x %d)", image->width, image->height);
                }
        }

        if (cmaterial.occlusionTexture.has_value())
        {
                fastgltf::Texture& texture = asset.textures[cmaterial.occlusionTexture->textureIndex];
                if (texture.imageIndex.has_value())
                {
                    Texture image = m_images[*texture.imageIndex];
                    debugPrintLine("    occlusionTexture: (%d x %d)", image->width, image->height);
                }
        }

        if (cmaterial.emissiveTexture.has_value())
        {
                fastgltf::Texture& texture = asset.textures[cmaterial.emissiveTexture->textureIndex];
                if (texture.imageIndex.has_value())
                {
                    Texture image = m_images[*texture.imageIndex];
                    debugPrintLine("    emissiveTexture: (%d x %d)", image->width, image->height);
                }
        }

        debugPrintLine("    emissiveFactor: %f %f %f",
            cmaterial.emissiveFactor[0],
            cmaterial.emissiveFactor[1],
            cmaterial.emissiveFactor[2]);

#if 0

        struct MaterialSpecular
        {
            float specularFactor;
            std::optional<TextureInfo> specularTexture;
            std::array<float, 3> specularColorFactor;
            std::optional<TextureInfo> specularColorTexture;
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

        struct MaterialVolume
        {
            float thicknessFactor;
            std::optional<TextureInfo> thicknessTexture;
            float attenuationDistance;
            std::array<float, 3> attenuationColor;
        };

        struct MaterialTransmission
        {
            float transmissionFactor;
            std::optional<TextureInfo> transmissionTexture;
        };

        struct MaterialClearcoat
        {
            float clearcoatFactor;
            std::optional<TextureInfo> clearcoatTexture;
            float clearcoatRoughnessFactor;
            std::optional<TextureInfo> clearcoatRoughnessTexture;
            std::optional<TextureInfo> clearcoatNormalTexture;
        };

        struct MaterialSheen
        {
            std::array<float, 3> sheenColorFactor;
            std::optional<TextureInfo> sheenColorTexture;
            float sheenRoughnessFactor;
            std::optional<TextureInfo> sheenRoughnessTexture;
        };

        struct Material
        {
            AlphaMode alphaMode;
            float alphaCutoff;

            bool doubleSided;

            std::unique_ptr<MaterialClearcoat> clearcoat;
            std::unique_ptr<MaterialIridescence> iridescence;
            std::unique_ptr<MaterialSheen> sheen;
            std::unique_ptr<MaterialSpecular> specular;
            std::unique_ptr<MaterialTransmission> transmission;
            std::unique_ptr<MaterialVolume> volume;

            std::optional<float> emissiveStrength;
            std::optional<float> ior;

            bool unlit;
        };

#endif
    }

    // --------------------------------------------------------------------------
    // meshes
    // --------------------------------------------------------------------------

    for (const auto& cmesh : asset.meshes)
    {
        debugPrintLine("[Mesh]");
        debugPrintLine("  name: %s", cmesh.name.c_str());

        IndexedMesh mesh;

        for (auto primitiveIterator = cmesh.primitives.begin(); primitiveIterator != cmesh.primitives.end(); ++primitiveIterator)
        {
            debugPrintLine("  [primitive]");

            struct Attribute
            {
                const u8* data = nullptr;
                size_t count = 0;
                size_t stride = 0;
            };

            Attribute attributePosition;
            Attribute attributeNormal;
            Attribute attributeTangent;
            Attribute attributeTexcoord;

            for (auto attributeIterator = primitiveIterator->attributes.begin(); attributeIterator != primitiveIterator->attributes.end(); ++attributeIterator)
            {
                auto name = attributeIterator->first;

                Attribute* attribute = nullptr;
                const char* message = "";

                if (name == "POSITION")
                {
                    attribute = &attributePosition;
                }
                else if (name == "NORMAL")
                {
                    attribute = &attributeNormal;
                }
                else if (name == "TANGENT")
                {
                    attribute = &attributeTangent;
                }
                else if (name == "TEXCOORD_0")
                {
                    attribute = &attributeTexcoord;
                }
                else
                {
                    message = " : NOT SUPPORTED!";
                }

                debugPrintLine("    [Attribute]");
                debugPrintLine("      name: \"%s\"%s", name.c_str(), message);

                auto& accessor = asset.accessors[attributeIterator->second];
                debugPrintLine("      components: %d", u32(fastgltf::getNumComponents(accessor.type)));

                auto& view = asset.bufferViews[accessor.bufferViewIndex.value()];

                auto offset = view.byteOffset + accessor.byteOffset;
                u32 index = view.bufferIndex;
                u32 count = accessor.count;

                u32 stride;
                if (view.byteStride.has_value())
                {
                    stride = u32(view.byteStride.value());
                }
                else
                {
                    stride = u32(fastgltf::getElementByteSize(accessor.type, accessor.componentType));
                }

                debugPrintLine("      stride: %d", stride);
                debugPrintLine("      offset: %d", offset);
                debugPrintLine("      count: %d", count);

                if (attribute)
                {
                    attribute->data = buffers[index].address + offset;
                    attribute->count = count;
                    attribute->stride = stride;
                }
            }

            // TODO: support different attribute types currently we "assume" everything is float

            std::vector<Vertex> vertices;

            if (attributePosition.data)
            {
                vertices.resize(attributePosition.count);

                const u8* data = attributePosition.data;

                for (size_t i = 0; i < attributePosition.count; ++i)
                {
                    std::memcpy(vertices[i].position.data(), data, sizeof(float32x3));
                    data += attributePosition.stride;
                }
            }
            else
            {
                // position attribute is required
                continue;
            }

            if (attributeNormal.data)
            {
                if (attributeNormal.count != attributePosition.count)
                {
                    // attribute counts must be identical
                    continue;
                }

                const u8* data = attributeNormal.data;

                for (size_t i = 0; i < attributeNormal.count; ++i)
                {
                    std::memcpy(vertices[i].normal.data(), data, sizeof(float32x3));
                    data += attributeNormal.stride;
                }
            }

            if (attributeTangent.data)
            {
                if (attributeTangent.count != attributePosition.count)
                {
                    // attribute counts must be identical
                    continue;
                }

                const u8* data = attributeTangent.data;

                for (size_t i = 0; i < attributeTangent.count; ++i)
                {
                    std::memcpy(vertices[i].normal.data(), data, sizeof(float32x4));
                    data += attributeTangent.stride;
                }
            }

            if (attributeTexcoord.data)
            {
                if (attributeTexcoord.count != attributePosition.count)
                {
                    // attribute counts must be identical
                    continue;
                }

                const u8* data = attributeTexcoord.data;

                for (size_t i = 0; i < attributeTexcoord.count; ++i)
                {
                    std::memcpy(vertices[i].normal.data(), data, sizeof(float32x2));
                    data += attributeTexcoord.stride;
                }
            }

            for (Vertex& vertex : vertices)
            {
                if (attributeNormal.data)
                {
                    // TODO: do this inline
                    vertex.normal = normalize(vertex.normal);
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

                    //u32 firstIndex = u32(indices.byteOffset + indicesView.byteOffset) / fastgltf::getElementByteSize(indices.type, indices.componentType);
                    //u32 indexType = u32(fastgltf::getGLComponentType(indices.componentType));
                    u32 offset = indicesView.byteOffset + indicesAccessor.byteOffset;
                    size_t count = indicesAccessor.count;

                    u32 indexTypeSize = 0;

                    u32 bufferIndex = indicesView.bufferIndex;
                    const u8* data = buffers[bufferIndex].address + offset;

                    indices.resize(count);

                    switch (indicesAccessor.componentType)
                    {
                        case fastgltf::ComponentType::UnsignedByte:
                        {
                            const u8* source = reinterpret_cast<const u8*>(data);
                            for (size_t i = 0; i < count; ++i)
                            {
                                indices[i] = source[i];
                            }
                            indexTypeSize = 1;
                            break;
                        }

                        case fastgltf::ComponentType::UnsignedShort:
                        {
                            const u16* source = reinterpret_cast<const u16*>(data);
                            for (size_t i = 0; i < count; ++i)
                            {
                                indices[i] = source[i];
                            }
                            indexTypeSize = 2;
                            break;
                        }

                        case fastgltf::ComponentType::UnsignedInt:
                        {
                            const u32* source = reinterpret_cast<const u32*>(data);
                            for (size_t i = 0; i < count; ++i)
                            {
                                indices[i] = source[i];
                            }
                            indexTypeSize = 4;
                            break;
                        }

                        default:
                            break;
                    }

                    debugPrintLine("    [Indices]");
                    debugPrintLine("      indexTypeSize: %d", indexTypeSize);
                    debugPrintLine("      count: %d", count);
                }
            }

            mesh.vertices = vertices;
            mesh.indices = indices;

            Primitive primitive;

            primitive.mode = Primitive::Mode::TRIANGLE_LIST; // TODO: correct type
            primitive.start = 0;
            primitive.count = u32(indices.size());
            primitive.material = 0;

            mesh.primitives.push_back(primitive);

        } // primitive

        meshes.push_back(mesh);
        debugPrintLine("Mesh. vertices: %d, indices: %d", (int)mesh.vertices.size(), (int)mesh.indices.size());

        /*

        for (auto it = cmesh.primitives.begin(); it != cmesh.primitives.end(); ++it) {

            // Get the output primitive
            auto index = std::distance(cmesh.primitives.begin(), it);
            auto& primitive = outMesh.primitives[index];

            primitive.primitiveType = fastgltf::to_underlying(it->type);
        }

        */
    }

    debugPrintLine("[Summary]");
    debugPrintLine("  Buffers:   %d", int(asset.buffers.size()));
    debugPrintLine("  Images:    %d", int(asset.images.size()));
    debugPrintLine("  Materials: %d", int(asset.materials.size()));
    debugPrintLine("  Meshes:    %d", int(asset.meshes.size()));

    u64 time1 = Time::ms();
    debugPrintLine("Time: %d ms", int(time1 - time0));
}

} // namespace mango::import3d
