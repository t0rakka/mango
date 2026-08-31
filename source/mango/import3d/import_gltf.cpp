/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/import3d/import_gltf.hpp>
#include <mango/math/quaternion.hpp>

#include <algorithm>
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
        bool normalized = false;

        operator bool () const
        {
            return data != nullptr;
        }
    };

} // namespace

namespace mango::import3d
{

ImportGLTF::ImportGLTF(const filesystem::Path& path, const std::string& filename)
    : Scene(path)
{
    u64 time0 = Time::ms();

    // --------------------------------------------------------------------------
    // read
    // --------------------------------------------------------------------------

    resources->files.push_back(std::make_shared<filesystem::File>(path, filename));
    const filesystem::File& file = *resources->files.back();

    fastgltf::GltfDataBuffer dataBuffer = *fastgltf::GltfDataBuffer::FromBytes(
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

    auto type = fastgltf::determineGltfFileType(dataBuffer);
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

    auto expected_asset = parser.loadGltf(dataBuffer, "", options);
    if (expected_asset.error() != fastgltf::Error::None)
    {
        printLine(Print::Error, "  ERROR: {}", fastgltf::getErrorMessage(expected_asset.error()).data());
        return;
    }

    fastgltf::Asset asset = std::move(expected_asset.get());

    // --------------------------------------------------------------------------
    // buffers — zero-copy views; lifetime anchored via resources->files
    // --------------------------------------------------------------------------

    std::vector<ConstMemory> buffers;

    for (const auto& current : asset.buffers)
    {
        printLine(Print::Verbose, "[Buffer]");

        std::visit(fastgltf::visitor
        {
            [] (const auto& arg)
            {
                printLine(Print::Verbose, "  Unknown");
                MANGO_UNREFERENCED(arg);
            },
            [&] (const fastgltf::sources::URI& source)
            {
                std::string filename = std::string(source.uri.path().begin(), source.uri.path().end());

                auto file = std::make_shared<filesystem::File>(path, filename);
                ConstMemory memory = *file;
                buffers.push_back(memory);
                resources->files.push_back(std::move(file));

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
                MANGO_UNREFERENCED(source);
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
    // images (deferred decode — record sources only)
    // --------------------------------------------------------------------------

    auto extensionFromMime = [](fastgltf::MimeType mime) -> std::string
    {
        return mimeToExtension(fastgltf::getMimeTypeString(mime));
    };

    images.reserve(asset.images.size());

    for (const auto& current : asset.images)
    {
        printLine(Print::Verbose, "[Image]");

        ImageSource source;
        source.name = std::string(current.name);

        std::visit(fastgltf::visitor
        {
            [] (const auto& arg)
            {
                MANGO_UNREFERENCED(arg);
            },
            [&] (const fastgltf::sources::URI& uri)
            {
                const std::string filename(uri.uri.path().begin(), uri.uri.path().end());
                const std::string ext = extensionFromMime(uri.mimeType);
                source = ImageSource::fromFile(filename, ext, source.name);
                printLine(Print::Verbose, "  URI: \"{}\"", filename);
            },
            [&] (const fastgltf::sources::Array& arr)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(arr.bytes.data()), arr.bytes.size());
                source = ImageSource::fromMemory(memory, extensionFromMime(arr.mimeType), source.name);
                printLine(Print::Verbose, "  Array: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::Vector& vec)
            {
                ConstMemory memory(reinterpret_cast<const u8*>(vec.bytes.data()), vec.bytes.size());
                source = ImageSource::fromMemory(memory, extensionFromMime(vec.mimeType), source.name);
                printLine(Print::Verbose, "  vector: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::ByteView& view)
            {
                printLine(Print::Verbose, "  ByteView: {} bytes (not supported)", view.bytes.size());
                MANGO_UNREFERENCED(view);
            },
            [&] (const fastgltf::sources::BufferView& viewSrc)
            {
                auto& bufferView = asset.bufferViews[viewSrc.bufferViewIndex];

                ConstMemory memory;
                if (bufferView.bufferIndex < buffers.size())
                {
                    const ConstMemory& buf = buffers[bufferView.bufferIndex];
                    if (buf.address && bufferView.byteOffset + bufferView.byteLength <= buf.size)
                    {
                        memory.address = buf.address + bufferView.byteOffset;
                        memory.size = bufferView.byteLength;
                    }
                }

                // buffers[] views are file-backed for Scene lifetime
                if (memory.address)
                {
                    source = ImageSource::fromMemory(memory, extensionFromMime(viewSrc.mimeType), source.name);
                }

                printLine(Print::Verbose, "  BufferView: {} bytes", memory.size);
            },
            [&] (const fastgltf::sources::CustomBuffer& custom)
            {
                printLine(Print::Verbose, "  CustomBuffer: TODO");
                MANGO_UNREFERENCED(custom);
            },
        }, current.data);

        images.push_back(std::move(source));
    }

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

        material.name = std::string(current.name);
        material.roughnessFactor = pbr.roughnessFactor;
        material.metallicFactor = pbr.metallicFactor;
        material.baseColorFactor[0] = pbr.baseColorFactor[0];
        material.baseColorFactor[1] = pbr.baseColorFactor[1];
        material.baseColorFactor[2] = pbr.baseColorFactor[2];
        material.baseColorFactor[3] = pbr.baseColorFactor[3];
        material.emissiveFactor[0] = emissiveFactor[0];
        material.emissiveFactor[1] = emissiveFactor[1];
        material.emissiveFactor[2] = emissiveFactor[2];

        auto readUvTransform = [](const fastgltf::TextureInfo& info) -> UvTransform
        {
            UvTransform t;
            if (info.transform)
            {
                t.offset = float32x2(float(info.transform->uvOffset[0]), float(info.transform->uvOffset[1]));
                t.scale = float32x2(float(info.transform->uvScale[0]), float(info.transform->uvScale[1]));
                t.rotation = float(info.transform->rotation);
            }
            return t;
        };

        auto imageIndexOf = [&](const fastgltf::TextureInfo& info) -> u32
        {
            if (info.textureIndex >= asset.textures.size())
                return ImageSample::none;
            const fastgltf::Texture& texture = asset.textures[info.textureIndex];
            if (!texture.imageIndex.has_value())
                return ImageSample::none;
            const size_t idx = *texture.imageIndex;
            if (idx >= images.size() || images[idx].empty())
                return ImageSample::none;
            return u32(idx);
        };

        auto bindSample = [&](ImageSample& dst, const fastgltf::TextureInfo& info,
                              ImageSwizzle swizzle, ImageColorSpace colorSpace)
        {
            const u32 img = imageIndexOf(info);
            if (img == ImageSample::none)
                return;
            dst = ImageSample::from(img, swizzle, colorSpace);
            dst.texCoord = u32(info.texCoordIndex);
            dst.transform = readUvTransform(info);
        };

        if (pbr.baseColorTexture.has_value())
        {
            bindSample(material.baseColor, *pbr.baseColorTexture, ImageSwizzle::rgba(), ImageColorSpace::sRGB);
            if (material.baseColor)
                printLine(Print::Verbose, "  baseColor: image[{}]", material.baseColor.image);
        }

        if (pbr.metallicRoughnessTexture.has_value())
        {
            const fastgltf::TextureInfo& info = *pbr.metallicRoughnessTexture;
            bindSample(material.roughness, info, ImageSwizzle::g(), ImageColorSpace::Linear);
            bindSample(material.metallic, info, ImageSwizzle::b(), ImageColorSpace::Linear);
            if (material.roughness)
            {
                printLine(Print::Verbose, "  metallicRoughness: image[{}] (G=rough, B=metal)",
                    material.roughness.image);
            }
        }

        if (current.normalTexture.has_value())
        {
            bindSample(material.normal, *current.normalTexture, ImageSwizzle::rgb1(), ImageColorSpace::Linear);
            material.normal.scale = float(current.normalTexture->scale);
            if (material.normal)
            {
                printLine(Print::Verbose, "  normal: image[{}] scale {}",
                    material.normal.image, material.normal.scale);
            }
        }

        if (current.occlusionTexture.has_value())
        {
            bindSample(material.occlusion, *current.occlusionTexture, ImageSwizzle::r(), ImageColorSpace::Linear);
            material.occlusion.scale = float(current.occlusionTexture->strength);
            if (material.occlusion)
            {
                printLine(Print::Verbose, "  occlusion: image[{}] strength {}",
                    material.occlusion.image, material.occlusion.scale);
            }
        }

        if (current.emissiveTexture.has_value())
        {
            bindSample(material.emissive, *current.emissiveTexture, ImageSwizzle::rgb1(), ImageColorSpace::sRGB);
            if (material.emissive)
                printLine(Print::Verbose, "  emissive: image[{}]", material.emissive.image);
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

        if (current.clearcoat)
        {
            const fastgltf::MaterialClearcoat& coat = *current.clearcoat;
            material.clearcoatFactor = coat.clearcoatFactor;
            material.clearcoatRoughnessFactor = coat.clearcoatRoughnessFactor;

            if (coat.clearcoatTexture.has_value())
            {
                bindSample(material.clearcoat, *coat.clearcoatTexture, ImageSwizzle::r(), ImageColorSpace::Linear);
            }

            if (coat.clearcoatRoughnessTexture.has_value())
            {
                bindSample(material.clearcoatRoughness, *coat.clearcoatRoughnessTexture, ImageSwizzle::g(), ImageColorSpace::Linear);
            }

            if (coat.clearcoatNormalTexture.has_value())
            {
                bindSample(material.clearcoatNormal, *coat.clearcoatNormalTexture, ImageSwizzle::rgb1(), ImageColorSpace::Linear);
            }

            printLine(Print::Verbose, "  clearcoatFactor: {}", material.clearcoatFactor);
            printLine(Print::Verbose, "  clearcoatRoughnessFactor: {}", material.clearcoatRoughnessFactor);
        }

        if (current.sheen)
        {
            const fastgltf::MaterialSheen& sheen = *current.sheen;
            material.sheenColorFactor = float32x3(
                sheen.sheenColorFactor[0],
                sheen.sheenColorFactor[1],
                sheen.sheenColorFactor[2]);
            material.sheenRoughnessFactor = sheen.sheenRoughnessFactor;

            if (sheen.sheenColorTexture.has_value())
            {
                bindSample(material.sheenColor, *sheen.sheenColorTexture, ImageSwizzle::rgb1(), ImageColorSpace::sRGB);
            }

            if (sheen.sheenRoughnessTexture.has_value())
            {
                bindSample(material.sheenRoughness, *sheen.sheenRoughnessTexture, ImageSwizzle::a(), ImageColorSpace::Linear);
            }

            printLine(Print::Info, "  sheenColorFactor: {} {} {}  sheenRoughness: {} (shading TBD)",
                material.sheenColorFactor.x, material.sheenColorFactor.y, material.sheenColorFactor.z,
                material.sheenRoughnessFactor);
        }

        if (current.anisotropy)
        {
            const fastgltf::MaterialAnisotropy& aniso = *current.anisotropy;
            material.anisotropyStrength = aniso.anisotropyStrength;
            material.anisotropyRotation = aniso.anisotropyRotation;

            if (aniso.anisotropyTexture.has_value())
            {
                bindSample(material.anisotropy, *aniso.anisotropyTexture, ImageSwizzle::rgba(), ImageColorSpace::Linear);
            }

            printLine(Print::Verbose, "  anisotropyStrength: {}  anisotropyRotation: {}",
                material.anisotropyStrength, material.anisotropyRotation);
        }

        materials.push_back(material);

        if (current.iridescence)
            printLine(Print::Verbose, "  Iridescence: TODO");
        if (current.specular)
            printLine(Print::Verbose, "  Specular: TODO");
        if (current.transmission)
            printLine(Print::Verbose, "  Transmission: TODO");
        if (current.volume)
            printLine(Print::Verbose, "  Volume: TODO");
    }

    if (materials.empty())
    {
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
            Attribute attributeJoints;
            Attribute attributeWeights;

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
                else if (name == "JOINTS_0")
                {
                    attribute = &attributeJoints;
                    mesh.flags |= Vertex::Joints;
                }
                else if (name == "WEIGHTS_0")
                {
                    attribute = &attributeWeights;
                    mesh.flags |= Vertex::Weights;
                }
                else
                {
                    message = " : NOT SUPPORTED!";
                }

                printLine(Print::Verbose, "    [Attribute:\"{}\"{}]", name, message);

                auto& accessor = asset.accessors[attributeIterator->accessorIndex];
                if (!accessor.bufferViewIndex.has_value())
                    continue;

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
                    case fastgltf::ComponentType::UnsignedInt:
                        printLine(Print::Verbose, "      type: u32 x {}", components);
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
                    attribute->normalized = accessor.normalized;
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
                    // glTF: RH, +Z toward viewer, CCW outside.
                    // Reflect Z → our LH Unity-like (+X right, +Y up, +Z ahead).
                    // Handedness flip from Z-reflect already yields CW front faces.
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
                    // glTF UVs are already top-left / V-down (Vulkan-compatible).
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

            // glTF JOINTS_0 / WEIGHTS_0 (set 0 only — up to 4 influences).
            if (attributeJoints)
            {
                if (attributeJoints.count != attributePosition.count)
                    continue;

                const u8* data = attributeJoints.data;
                const size_t comps = std::min(attributeJoints.components, size_t(4));

                for (size_t i = 0; i < attributeJoints.count; ++i)
                {
                    switch (attributeJoints.type)
                    {
                        case fastgltf::ComponentType::UnsignedByte:
                            for (size_t c = 0; c < comps; ++c)
                                vertices[i].joint[c] = data[c];
                            break;

                        case fastgltf::ComponentType::UnsignedShort:
                            for (size_t c = 0; c < comps; ++c)
                                vertices[i].joint[c] = uload16(data + c * 2);
                            break;

                        default:
                            break;
                    }
                    data += attributeJoints.stride;
                }
            }

            if (attributeWeights)
            {
                if (attributeWeights.count != attributePosition.count)
                    continue;

                const u8* data = attributeWeights.data;
                const size_t comps = std::min(attributeWeights.components, size_t(4));

                for (size_t i = 0; i < attributeWeights.count; ++i)
                {
                    float w[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

                    switch (attributeWeights.type)
                    {
                        case fastgltf::ComponentType::Float:
                            for (size_t c = 0; c < comps; ++c)
                                w[c] = uload32f(data + c * 4);
                            break;

                        case fastgltf::ComponentType::UnsignedByte:
                            for (size_t c = 0; c < comps; ++c)
                            {
                                const float v = float(data[c]);
                                w[c] = attributeWeights.normalized ? v / 255.0f : v;
                            }
                            break;

                        case fastgltf::ComponentType::UnsignedShort:
                            for (size_t c = 0; c < comps; ++c)
                            {
                                const float v = float(uload16(data + c * 2));
                                w[c] = attributeWeights.normalized ? v / 65535.0f : v;
                            }
                            break;

                        default:
                            break;
                    }

                    // Spec says weights sum to 1; renormalize defensively.
                    float sum = w[0] + w[1] + w[2] + w[3];
                    if (sum > 0.0f)
                    {
                        const float inv = 1.0f / sum;
                        vertices[i].weight = float32x4(w[0] * inv, w[1] * inv, w[2] * inv, w[3] * inv);
                    }

                    data += attributeWeights.stride;
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

            if (material.normal)
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

                            // Z-reflect already yields CW front faces — keep file order.
                            triangle.vertex[0] = vertices[indices[i - 2]];
                            triangle.vertex[1] = vertices[indices[i - 1]];
                            triangle.vertex[2] = vertices[indices[i - 0]];

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

                            if (i & 1)
                            {
                                triangle.vertex[0] = v1;
                                triangle.vertex[1] = v0;
                            }
                            else
                            {
                                triangle.vertex[0] = v0;
                                triangle.vertex[1] = v1;
                            }

                            triangle.vertex[2] = vertices[indices[i]];
                            Vertex newest = triangle.vertex[2];

                            trimesh.triangles.push_back(triangle);

                            v0 = v1;
                            v1 = newest;
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
                            triangle.vertex[2] = vertices[indices[i]];
                            trimesh.triangles.push_back(triangle);
                            triangle.vertex[1] = triangle.vertex[2];
                        }
                        break;
                    }

                    default:
                        // unsupported primitive type
                        continue;
                }

                u64 b_time0 = Time::us();

                if (needTangent)
                {
                    trimesh.computeTangents();
                }

                u64 b_time1 = Time::us();

                mesh.append(trimesh, u32(materialIndex));

                u64 b_time2 = Time::us();
                u64 delta0 = b_time1 - b_time0;
                u64 delta1 = b_time2 - b_time1;

                printLine(Print::Verbose, "    Computing tangents: {}.{} ms", delta0 / 1000, delta0 % 1000);
                printLine(Print::Verbose, "    Mesh Indexing: {}.{} ms", delta1 / 1000, delta1 % 1000);
            }
            else
            {
                Primitive primitive;

                // TODO: support primitive restart (index: 0xffffffff)

                switch (primitiveIterator->type)
                {
                    case fastgltf::PrimitiveType::Triangles:
                        primitive.type = Primitive::Type::TriangleList;
                        break;

                    case fastgltf::PrimitiveType::TriangleStrip:
                        primitive.type = Primitive::Type::TriangleStrip;
                        break;

                    case fastgltf::PrimitiveType::TriangleFan:
                        primitive.type = Primitive::Type::TriangleFan;
                        break;

                    default:
                        // unsupported primitive type
                        continue;
                }

                // Z-reflect (det −1) already turns glTF CCW into CW — keep index order.

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
    // skins
    // --------------------------------------------------------------------------

    const matrix4x4 axisReflect = matrix4x4::scale(1.0f, 1.0f, -1.0f);

    for (const auto& current : asset.skins)
    {
        Skin skin;
        skin.name = current.name;

        for (std::size_t joint : current.joints)
        {
            skin.joints.push_back(u32(joint));
        }

        if (current.skeleton.has_value())
        {
            skin.skeleton = u32(current.skeleton.value());
        }

        skin.inverseBindMatrices.assign(skin.joints.size(), matrix4x4(1.0f));

        if (current.inverseBindMatrices.has_value())
        {
            const auto& accessor = asset.accessors[current.inverseBindMatrices.value()];
            if (accessor.bufferViewIndex.has_value())
            {
                const auto& view = asset.bufferViews[accessor.bufferViewIndex.value()];
                const size_t offset = view.byteOffset + accessor.byteOffset;
                const u8* data = buffers[view.bufferIndex].address + offset;

                size_t stride = view.byteStride.has_value()
                    ? view.byteStride.value()
                    : fastgltf::getElementByteSize(accessor.type, accessor.componentType);

                const size_t count = std::min(accessor.count, skin.joints.size());
                for (size_t i = 0; i < count; ++i)
                {
                    // glTF MAT4 is column-major; mango matrix4x4 ctor takes row-major floats.
                    // Reading 16 floats in memory order into mango's row layout matches how
                    // node matrices are imported (fmat4x4::data() → matrix4x4).
                    float m[16];
                    for (int k = 0; k < 16; ++k)
                    {
                        m[k] = uload32f(data + i * stride + size_t(k) * 4);
                    }

                    matrix4x4 ibm(m);
                    // Same S M S as node transforms so IBM space matches Z-reflected mesh.
                    skin.inverseBindMatrices[i] = axisReflect * ibm * axisReflect;
                }
            }
        }

        printLine(Print::Verbose, "[Skin]\n  \"{}\" joints={} ibms={}",
            skin.name, skin.joints.size(), skin.inverseBindMatrices.size());

        skins.push_back(std::move(skin));
    }

    // --------------------------------------------------------------------------
    // animations
    // --------------------------------------------------------------------------

    auto readFloatAccessor = [&](std::size_t accessorIndex, std::vector<float>& out, size_t* outComponents) -> bool
    {
        if (accessorIndex >= asset.accessors.size())
            return false;
        const auto& accessor = asset.accessors[accessorIndex];
        if (!accessor.bufferViewIndex.has_value())
            return false;
        if (accessor.componentType != fastgltf::ComponentType::Float)
            return false;

        const auto& view = asset.bufferViews[accessor.bufferViewIndex.value()];
        const size_t offset = view.byteOffset + accessor.byteOffset;
        const u8* data = buffers[view.bufferIndex].address + offset;
        const size_t components = fastgltf::getNumComponents(accessor.type);
        const size_t stride = view.byteStride.has_value()
            ? view.byteStride.value()
            : fastgltf::getElementByteSize(accessor.type, accessor.componentType);

        out.resize(accessor.count * components);
        for (size_t i = 0; i < accessor.count; ++i)
        {
            for (size_t c = 0; c < components; ++c)
            {
                out[i * components + c] = uload32f(data + i * stride + c * 4);
            }
        }
        if (outComponents)
            *outComponents = components;
        return true;
    };

    // Keys are converted into engine space (same Z-reflect as meshes / node locals).
    auto fixTranslation = [](float* v)
    {
        v[2] = -v[2];
    };

    auto fixRotation = [&](float* v)
    {
        // R' = S R S with S = diag(1,1,-1), then back to quaternion.
        matrix4x4 R(math::Quaternion(v[0], v[1], v[2], v[3]));
        matrix4x4 Rp = axisReflect * R * axisReflect;
        math::Quaternion q(Rp);

        // Keep hemisphere stable for interpolation.
        if (q.w < 0.0f)
        {
            q.x = -q.x;
            q.y = -q.y;
            q.z = -q.z;
            q.w = -q.w;
        }

        v[0] = q.x;
        v[1] = q.y;
        v[2] = q.z;
        v[3] = q.w;
    };

    for (const auto& current : asset.animations)
    {
        Animation animation;
        animation.name = current.name;

        animation.samplers.reserve(current.samplers.size());
        for (const auto& srcSampler : current.samplers)
        {
            AnimationSampler sampler;

            switch (srcSampler.interpolation)
            {
                case fastgltf::AnimationInterpolation::Step:
                    sampler.interpolation = AnimationInterpolation::Step;
                    break;
                case fastgltf::AnimationInterpolation::CubicSpline:
                    sampler.interpolation = AnimationInterpolation::CubicSpline;
                    break;
                case fastgltf::AnimationInterpolation::Linear:
                default:
                    sampler.interpolation = AnimationInterpolation::Linear;
                    break;
            }

            size_t timeComponents = 0;
            if (!readFloatAccessor(srcSampler.inputAccessor, sampler.times, &timeComponents))
            {
                printLine(Print::Verbose, "[Animation] \"{}\" sampler: bad input accessor", animation.name);
                animation.samplers.push_back(std::move(sampler));
                continue;
            }

            size_t valueComponents = 0;
            if (!readFloatAccessor(srcSampler.outputAccessor, sampler.values, &valueComponents))
            {
                printLine(Print::Verbose, "[Animation] \"{}\" sampler: bad output accessor", animation.name);
                animation.samplers.push_back(std::move(sampler));
                continue;
            }

            sampler.components = u32(valueComponents);

            if (!sampler.times.empty())
                animation.duration = std::max(animation.duration, sampler.times.back());

            animation.samplers.push_back(std::move(sampler));
        }

        animation.channels.reserve(current.channels.size());
        for (const auto& srcChannel : current.channels)
        {
            AnimationChannel channel;
            channel.sampler = u32(srcChannel.samplerIndex);

            switch (srcChannel.path)
            {
                case fastgltf::AnimationPath::Rotation:
                    channel.path = AnimationPath::Rotation;
                    break;
                case fastgltf::AnimationPath::Scale:
                    channel.path = AnimationPath::Scale;
                    break;
                case fastgltf::AnimationPath::Weights:
                    channel.path = AnimationPath::Weights;
                    break;
                case fastgltf::AnimationPath::Translation:
                default:
                    channel.path = AnimationPath::Translation;
                    break;
            }

            if (srcChannel.nodeIndex.has_value())
            {
                const u32 nodeIndex = u32(srcChannel.nodeIndex.value());
                channel.node = nodeIndex;
                if (nodeIndex < asset.nodes.size())
                {
                    channel.targetName = std::string(asset.nodes[nodeIndex].name);
                }
            }

            animation.channels.push_back(std::move(channel));
        }

        // Axis-fix each sampler once (samplers may be shared by multiple channels).
        std::vector<bool> samplerFixed(animation.samplers.size(), false);
        for (const AnimationChannel& channel : animation.channels)
        {
            if (channel.sampler >= animation.samplers.size() || samplerFixed[channel.sampler])
                continue;
            if (channel.path == AnimationPath::Weights || channel.path == AnimationPath::Scale)
            {
                samplerFixed[channel.sampler] = true;
                continue;
            }

            AnimationSampler& sampler = animation.samplers[channel.sampler];
            if (sampler.components < 3 || sampler.values.empty())
            {
                samplerFixed[channel.sampler] = true;
                continue;
            }

            const size_t elementCount =
                sampler.interpolation == AnimationInterpolation::CubicSpline ? 3 : 1;
            const size_t stride = size_t(sampler.components) * elementCount;
            for (size_t i = 0; i + stride <= sampler.values.size(); i += stride)
            {
                for (size_t e = 0; e < elementCount; ++e)
                {
                    float* v = sampler.values.data() + i + e * sampler.components;
                    if (channel.path == AnimationPath::Translation)
                        fixTranslation(v);
                    else if (channel.path == AnimationPath::Rotation && sampler.components >= 4)
                        fixRotation(v);
                }
            }
            samplerFixed[channel.sampler] = true;
        }

        printLine(Print::Verbose, "[Animation]\n  \"{}\" duration={:.3f}s samplers={} channels={}",
            animation.name, animation.duration, animation.samplers.size(), animation.channels.size());

        animations.push_back(std::move(animation));
    }

    // --------------------------------------------------------------------------
    // nodes
    // --------------------------------------------------------------------------

    for (const auto& current : asset.nodes)
    {
        Node node;
        node.name = current.name;

        if (const auto* matrix = std::get_if<fastgltf::math::fmat4x4>(&current.transform))
        {
            const float* data = matrix->data();
            node.transform = axisReflect * matrix4x4(data) * axisReflect;
        }
        else if (const auto* trs = std::get_if<fastgltf::TRS>(&current.transform))
        {
            const float* t = trs->translation.data();
            const float* r = trs->rotation.data();
            const float* s = trs->scale.data();

            float translation[3] = { t[0], t[1], t[2] };
            float rotation[4] = { r[0], r[1], r[2], r[3] };
            fixTranslation(translation);
            fixRotation(rotation);

            const matrix4x4 T = matrix4x4::translate(translation[0], translation[1], translation[2]);
            const matrix4x4 R = math::Quaternion(rotation[0], rotation[1], rotation[2], rotation[3]);
            const matrix4x4 S = matrix4x4::scale(s[0], s[1], s[2]);
            node.transform = S * R * T;
        }

        for (auto child : current.children)
        {
            node.children.push_back(u32(child));
        }

        if (current.meshIndex)
        {
            node.mesh = u32(current.meshIndex.value());
        }

        if (current.skinIndex)
        {
            node.skin = u32(current.skinIndex.value());
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
    printLine(Print::Verbose, "  Buffers:    {}", asset.buffers.size());
    printLine(Print::Verbose, "  Images:     {}", asset.images.size());
    printLine(Print::Verbose, "  Materials:  {}", asset.materials.size());
    printLine(Print::Verbose, "  Meshes:     {}", asset.meshes.size());
    printLine(Print::Verbose, "  Skins:      {}", asset.skins.size());
    printLine(Print::Verbose, "  Animations: {}", asset.animations.size());
    printLine(Print::Verbose, "  Nodes:      {}", asset.nodes.size());
    printLine(Print::Verbose, "  Scenes:     {}", asset.scenes.size());

    u64 time1 = Time::ms();
    printLine(Print::Verbose, "Time: {} ms", time1 - time0);
}

} // namespace mango::import3d
