/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
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

    debugPrintLine("BufferSize: %d", int(data.getBufferSize()));

    // --------------------------------------------------------------------------
    // parse
    // --------------------------------------------------------------------------

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

    for (auto& buffer : asset.buffers)
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
            [] (auto& arg) {},
            [&] (fastgltf::sources::URI& source)
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
			[&] (fastgltf::sources::ByteView& source)
            {
                ConstMemory memory = ConstMemory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());
                buffers.push_back(memory);

                // [ ] standard
                // [x] binary
                // [ ] embedded
                debugPrintLine("  ByteView: %d bytes", int(memory.size));
			},
            [&] (fastgltf::sources::BufferView& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  BufferView:");
            },
            [&] (fastgltf::sources::Vector& source)
            {
                ConstMemory memory = ConstMemory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());
                buffers.push_back(memory);

                // [ ] standard
                // [ ] binary
                // [x] embedded
                debugPrintLine("  vector: %d bytes", int(memory.size));
            },
            [&] (fastgltf::sources::CustomBuffer& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  CustomBuffer: %d", int(source.id));
            },
        }, buffer.data);
    }

    debugPrintLine("Buffers: %d", int(buffers.size()));

    // --------------------------------------------------------------------------
    // images
    // --------------------------------------------------------------------------

    std::vector<u32> m_images;

    for (auto& image : asset.images)
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
            [] (auto& arg) {},
            [&] (fastgltf::sources::URI& source)
            {
                assert(source.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(source.uri.isLocalPath()); // We're only capable of loading local files.

                const std::string path(source.uri.path().begin(), source.uri.path().end());

                m_images.push_back(0);

                // [x] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  URI: %s", path.c_str());
            },
            [&] (fastgltf::sources::Vector& source)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(source.bytes.data()), source.bytes.size());
                //std::cout << "  ImageFormat: " << getImageFormat(memory) << std::endl;

                m_images.push_back(u32(memory.size));

                // [ ] standard
                // [ ] binary
                // [x] embedded
                debugPrintLine("  vector: %d bytes", int(memory.size));
            },
			[&] (fastgltf::sources::ByteView& source)
            {
                // [ ] standard
                // [ ] binary
                // [ ] embedded
                debugPrintLine("  ByteView: %d bytes", int(source.bytes.size()));
			},
            [&] (fastgltf::sources::BufferView& source)
            {
                auto& bufferView = asset.bufferViews[source.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];
                auto span = std::get<fastgltf::sources::ByteView>(buffer.data).bytes;

                ConstMemory memory(reinterpret_cast<const u8*>(span.data() + bufferView.byteOffset), bufferView.byteLength);
                //std::cout << "  ImageFormat: " << getImageFormat(memory) << std::endl;

                m_images.push_back(u32(memory.size));

                // [ ] standard
                // [x] binary
                // [ ] embedded
                debugPrintLine("  BufferView: %d bytes", int(memory.size));
            },
        }, image.data);

    }

    // --------------------------------------------------------------------------
    // materials
    // --------------------------------------------------------------------------

    for (auto& material : asset.materials)
    {
        debugPrintLine("[Material]");
        debugPrintLine("  name %s", material.name.c_str());

        const fastgltf::PBRData& pbr = material.pbrData;
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
                u32 s = m_images[*texture.imageIndex];
                debugPrintLine("    baseColorTexture: %d", s);
            }
        }

        if (pbr.metallicRoughnessTexture.has_value())
        {
            fastgltf::Texture& texture = asset.textures[pbr.metallicRoughnessTexture->textureIndex];
            if (texture.imageIndex.has_value())
            {
                u32 s = m_images[*texture.imageIndex];
                debugPrintLine("    metallicRoughnessTexture: %d", s);
            }
        }

        if (material.normalTexture.has_value())
        {
                fastgltf::Texture& texture = asset.textures[material.normalTexture->textureIndex];
                if (texture.imageIndex.has_value())
                {
                    u32 s = m_images[*texture.imageIndex];
                    debugPrintLine("    normalTexture: %d", s);
                }
        }

        if (material.occlusionTexture.has_value())
        {
                fastgltf::Texture& texture = asset.textures[material.occlusionTexture->textureIndex];
                if (texture.imageIndex.has_value())
                {
                    u32 s = m_images[*texture.imageIndex];
                    debugPrintLine("    occlusionTexture: %d", s);
                }
        }

        if (material.emissiveTexture.has_value())
        {
                fastgltf::Texture& texture = asset.textures[material.emissiveTexture->textureIndex];
                if (texture.imageIndex.has_value())
                {
                    u32 s = m_images[*texture.imageIndex];
                    debugPrintLine("    emissiveTexture: %d", s);
                }
        }

        debugPrintLine("    emissiveFactor: %f %f %f",
            material.emissiveFactor[0],
            material.emissiveFactor[1],
            material.emissiveFactor[2]);

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

    for (auto& mesh : asset.meshes)
    {
        debugPrintLine("[Mesh]");
        debugPrintLine("  name: %s", mesh.name.c_str());

        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it)
        {
            debugPrintLine("  [primitive]");

            for (auto a = it->attributes.begin(); a != it->attributes.end(); ++a)
            {
                debugPrintLine("    [Attribute]");
                debugPrintLine("      name: %s", a->first.c_str());

                auto& accessor = asset.accessors[a->second];
                debugPrintLine("      components: %d", u32(fastgltf::getNumComponents(accessor.type)));
                //std::cout << "      type: " << fastgltf::getGLComponentType(accessor.componentType) << std::endl;

                auto& view = asset.bufferViews[accessor.bufferViewIndex.value()];

                auto offset = view.byteOffset + accessor.byteOffset;
                //u32 index = view.bufferIndex;
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
            }

            /*
            // position

            auto& positionAccessor = asset.accessors[it->attributes["POSITION"]];
            if (!positionAccessor.bufferViewIndex.has_value())
            {
                std::cout << "INFO: NO BufferViewIndex!" << std::endl;
                continue;
            }

            std::cout << "      components: " << u32(fastgltf::getNumComponents(positionAccessor.type)) << std::endl;
            std::cout << "      type: " << fastgltf::getGLComponentType(positionAccessor.componentType) << std::endl;

            auto& positionView = asset.bufferViews[positionAccessor.bufferViewIndex.value()];
            auto offset = positionView.byteOffset + positionAccessor.byteOffset;
            u32 index = positionView.bufferIndex;
            u32 count = positionAccessor.count;

            u32 stride;
            if (positionView.byteStride.has_value())
            {
                stride = u32(positionView.byteStride.value());
            }
            else
            {
                stride = u32(fastgltf::getElementByteSize(positionAccessor.type, positionAccessor.componentType));
            }

            std::cout << "      stride: " << stride << std::endl;
            std::cout << "      offset: " << offset << std::endl;
            std::cout << "      count: " << count << std::endl;

            for (int i = 0; i < 4; ++i)
            {
                float* p = reinterpret_cast<float*>(buf.data() + offset + i * stride);
                std::cout << p[0] << " " << p[1] << " " << p[2] << std::endl;
            }
            */

            // indices

            if (it->indicesAccessor.has_value())
            {
                auto& indices = asset.accessors[it->indicesAccessor.value()];
                if (indices.bufferViewIndex.has_value())
                {
                    //auto& indicesView = asset.bufferViews[indices.bufferViewIndex.value()];

                    //u32 firstIndex = u32(indices.byteOffset + indicesView.byteOffset) / fastgltf::getElementByteSize(indices.type, indices.componentType);
                    //u32 indexType = u32(fastgltf::getGLComponentType(indices.componentType));
                    //u32 offset = indicesView.byteOffset + indices.byteOffset;
                    u32 count = u32(indices.count);

                    u32 indexTypeSize = 0;
                    //u32 maxIndex = 0;

                    //u32 bufferIndex = indicesView.bufferIndex;
                    //const u8* data = buffers[bufferIndex].address;

                    switch (indices.componentType)
                    {
                        case fastgltf::ComponentType::UnsignedByte:
                        {
                            //const u8* source = reinterpret_cast<const u8*>(data + offset);
                            //maxIndex = computeMaxIndex(source, count);
                            indexTypeSize = 1;
                            break;
                        }

                        case fastgltf::ComponentType::UnsignedShort:
                        {
                            //const u16* source = reinterpret_cast<const u16*>(data + offset);
                            //maxIndex = computeMaxIndex(source, count);
                            indexTypeSize = 2;
                            break;
                        }

                        case fastgltf::ComponentType::UnsignedInt:
                        {
                            //const u32* source = reinterpret_cast<const u32*>(data + offset);
                            //maxIndex = computeMaxIndex(source, count);
                            indexTypeSize = 4;
                            break;
                        }

                        default:
                            break;
                    }

                    debugPrintLine("    [Indices]");
                    //debugPrintLine("      maxIndex: %d", maxIndex);
                    debugPrintLine("      indexTypeSize: %d", indexTypeSize);
                    debugPrintLine("      count: %d", count);
                }
            }
        }

        /*

        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it) {

            // Get the output primitive
            auto index = std::distance(mesh.primitives.begin(), it);
            auto& primitive = outMesh.primitives[index];
            primitive.primitiveType = fastgltf::to_underlying(it->type);
            primitive.vertexArray = vao;

            if (it->materialIndex.has_value())
            {
                primitive.materialUniformsIndex = it->materialIndex.value();
                auto& material = viewer->asset.materials[it->materialIndex.value()];
                if (material.pbrData.has_value() && material.pbrData->baseColorTexture.has_value())
                {
                    auto& texture = viewer->asset.textures[material.pbrData->baseColorTexture->textureIndex];
                    if (!texture.imageIndex.has_value())
                        return false;
                    primitive.albedoTexture = viewer->textures[texture.imageIndex.value()].texture;
                }
            }
        }

        */
    }

    u64 time1 = Time::ms();
    debugPrintLine("Time: %d ms", int(time1 - time0));
}

} // namespace mango::import3d
