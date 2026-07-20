/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cctype>
#include <cstring>
#include <unordered_map>
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
        ByPolygon,
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
        u64 id = 0;
        ArrayFBX<float32x3> positions;
        ArrayFBX<float32x3> normals;
        ArrayFBX<float32x2> texcoords;
        // LayerElementMaterial / AllSame: index into materials linked to the parent Model.
        u32 materialIndex = 0;
        bool hasMaterialIndex = false;
    };

    struct MaterialFBX
    {
        u64 id = 0;
        std::string name;
        float32x3 diffuse { 1.0f, 1.0f, 1.0f };
        float diffuseFactor { 1.0f };
        float32x3 emissive { 0.0f, 0.0f, 0.0f };
        float emissiveFactor { 1.0f };
        float shininess { 32.0f };
        float opacity { 1.0f };
    };

    struct TextureFBX
    {
        u64 id = 0;
        std::string name;
        std::string relativeFilename;
        std::string filename;
    };

    struct VideoFBX
    {
        u64 id = 0;
        std::string name;
        std::string relativeFilename;
        std::string filename;
        ConstMemory content;
    };

    struct ModelFBX
    {
        u64 id = 0;
        std::string name;
    };

    struct ConnectionFBX
    {
        bool isProperty = false; // OP vs OO
        u64 src = 0;
        u64 dst = 0;
        std::string property;
    };

    struct Property
    {
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

        Variant value;
    };

    std::string objectName(std::string_view full)
    {
        // FBX object names are "Name\0Class" (sometimes "Name\0\x01Class").
        size_t pos = full.find('\0');
        if (pos == std::string_view::npos)
            return std::string(full);
        return std::string(full.substr(0, pos));
    }

    std::string toLower(std::string s)
    {
        for (char& c : s)
            c = char(std::tolower(u8(c)));
        return s;
    }

    std::string normalizePathSeparators(std::string path)
    {
        for (char& c : path)
        {
            if (c == '\\')
                c = '/';
        }
        return path;
    }

    std::string parentPathname(std::string pathname)
    {
        while (!pathname.empty() && (pathname.back() == '/' || pathname.back() == '\\'))
            pathname.pop_back();
        return filesystem::getPath(pathname);
    }

    // Case-insensitive file lookup inside a Path directory (exact basename).
    std::string findFileIgnoreCase(const filesystem::Path& path, const std::string& filename)
    {
        if (filename.empty())
            return {};

        if (path.isFile(filename))
            return filename;

        const std::string wanted = toLower(filesystem::removePath(filename));
        for (const filesystem::FileInfo& info : path)
        {
            if (!info.isFile())
                continue;
            if (toLower(info.name) == wanted)
                return info.name;
        }
        return {};
    }

    // Case-insensitive stem match: "Cerberus_A.tga" finds "cerberus_A.png".
    // Artists routinely mismatch extension and casing when exporting from Windows.
    std::string findFileByStemIgnoreCase(const filesystem::Path& path, const std::string& stem)
    {
        if (stem.empty())
            return {};

        const std::string wanted = toLower(stem);
        for (const filesystem::FileInfo& info : path)
        {
            if (!info.isFile())
                continue;
            if (toLower(filesystem::removeExtension(info.name)) == wanted)
                return info.name;
        }
        return {};
    }

    // Resolve a subdirectory name under meshPath, ignoring case ("Textures" vs "textures").
    // Returns the on-disk directory name with trailing slash, or empty if not found.
    std::string findSubdirectoryIgnoreCase(const filesystem::Path& meshPath, const std::string& subdir)
    {
        if (subdir.empty())
            return {};

        std::string name = subdir;
        while (!name.empty() && (name.back() == '/' || name.back() == '\\'))
            name.pop_back();

        // Only the first path component (RelativeFilename is usually "Textures\file.ext").
        const size_t slash = name.find('/');
        if (slash != std::string::npos)
            name = name.substr(0, slash);
        if (name.empty() || name == "." || name == "..")
            return {};

        const std::string wanted = toLower(name);
        try
        {
            for (const filesystem::FileInfo& info : meshPath)
            {
                if (!info.isDirectory())
                    continue;
                std::string dirName = info.name;
                while (!dirName.empty() && dirName.back() == '/')
                    dirName.pop_back();
                if (toLower(dirName) == wanted)
                    return dirName + "/";
            }
        }
        catch (...)
        {
        }

        // Directory may exist but not appear in the index — try the declared name as-is.
        return name + "/";
    }

    Texture tryCreateTexture(const filesystem::Path& path, const std::string& filename)
    {
        Texture texture;
        if (filename.empty())
            return texture;

        try
        {
            texture = createTexture(path, filename);
        }
        catch (...)
        {
        }
        return texture;
    }

    Texture tryLoadInDirectory(const filesystem::Path& dir, const std::string& basename, const std::string& stem)
    {
        // Exact / case-insensitive basename first (preserves declared extension when present).
        std::string found = findFileIgnoreCase(dir, basename);
        if (!found.empty())
            return tryCreateTexture(dir, found);

        // Stem match: RelativeFilename says .tga, disk has .png (very common).
        found = findFileByStemIgnoreCase(dir, stem);
        if (!found.empty())
            return tryCreateTexture(dir, found);

        return {};
    }

    Texture tryLoadUnder(const filesystem::Path& root, const std::string& relativeDir,
                         const std::string& basename, const std::string& stem)
    {
        if (relativeDir.empty())
            return {};

        try
        {
            // Same nesting model as glTF: Path(assetDir, "texture/") + basename.
            filesystem::Path dir(root, relativeDir);
            return tryLoadInDirectory(dir, basename, stem);
        }
        catch (...)
        {
            return {};
        }
    }

    // Resolve an FBX RelativeFilename / FileName against the mesh directory.
    // Same Path nesting as ImportGLTF / createTexture(path, "textures/foo.png").
    Texture resolveTexturePath(const filesystem::Path& meshPath, const std::string& declared)
    {
        if (declared.empty())
            return {};

        const std::string normalized = normalizePathSeparators(declared);
        const std::string subdir = filesystem::getPath(normalized); // "texture/" or ""
        const std::string basename = filesystem::removePath(normalized);
        const std::string stem = filesystem::removeExtension(basename);

        // 0. Exact declared relative path (glTF-style), including subdirs.
        if (Texture t = tryCreateTexture(meshPath, normalized))
            return t;

        // 1. Declared subdirectory next to the .fbx (Textures\Cerberus_A.tga), case-insensitive.
        if (!subdir.empty())
        {
            const std::string resolved = findSubdirectoryIgnoreCase(meshPath, subdir);
            if (!resolved.empty())
            {
                if (Texture t = tryLoadUnder(meshPath, resolved, basename, stem))
                    return t;
            }
        }

        // 2. Next to the .fbx.
        if (Texture t = tryLoadInDirectory(meshPath, basename, stem))
            return t;

        // 3. Conventional texture folders next to the .fbx.
        {
            const char* folders[] = { "textures/", "Textures/", "texture/", "Texture/" };
            for (const char* folder : folders)
            {
                if (Texture t = tryLoadUnder(meshPath, folder, basename, stem))
                    return t;
            }
        }

        // 4. Parent-level textures/ (Cerberus-style: meshes/ + ../textures/).
        const std::string parent = parentPathname(meshPath.pathname());
        if (!parent.empty())
        {
            try
            {
                filesystem::Path parentPath(parent);
                const char* folders[] = { "textures/", "Textures/", "texture/", "Texture/" };
                for (const char* folder : folders)
                {
                    if (Texture t = tryLoadUnder(parentPath, folder, basename, stem))
                        return t;
                }
            }
            catch (...)
            {
            }
        }

        return {};
    }

    // Probe Cerberus-style sidecars: stem ending in _A / Albedo → _N / _M / _R next to it.
    void loadPbrSidecars(const filesystem::Path& meshPath, const std::string& albedoDeclared,
                         Texture& outNormal, Texture& outMetallic, Texture& outRoughness)
    {
        if (albedoDeclared.empty())
            return;

        const std::string normalized = normalizePathSeparators(albedoDeclared);
        const std::string basename = filesystem::removePath(normalized);
        std::string prefix = filesystem::removeExtension(basename);

        const char* suffixes[] = { "_A", "_Albedo", "_Diffuse", "_BaseColor", "_Basecolour" };
        for (const char* suffix : suffixes)
        {
            const size_t n = std::strlen(suffix);
            if (prefix.size() >= n && toLower(prefix.substr(prefix.size() - n)) == toLower(suffix))
            {
                prefix.erase(prefix.size() - n);
                break;
            }
        }

        auto loadVariant = [&] (std::initializer_list<const char*> variants) -> Texture
        {
            for (const char* variant : variants)
            {
                Texture t = resolveTexturePath(meshPath, std::string("Textures/") + prefix + variant + ".png");
                if (t)
                    return t;
                t = resolveTexturePath(meshPath, prefix + variant + ".png");
                if (t)
                    return t;
            }
            return {};
        };

        outNormal = loadVariant({ "_N", "_Normal", "_NormalMap", "_Nor" });
        outMetallic = loadVariant({ "_M", "_Metallic", "_Metalness", "_Metal" });
        outRoughness = loadVariant({ "_R", "_Roughness", "_Rough" });
    }

    // Pack separate metallic + roughness maps into glTF ORM layout (G=roughness, B=metallic).
    Texture packMetallicRoughness(const Texture& metallic, const Texture& roughness)
    {
        if (!metallic && !roughness)
            return {};

        const int width = metallic ? metallic->width : roughness->width;
        const int height = metallic ? metallic->height : roughness->height;

        image::Format format(32, image::Format::UNORM, image::Format::RGBA, 8, 8, 8, 8);
        auto packed = std::make_shared<image::Bitmap>(width, height, format);

        std::unique_ptr<image::TemporaryBitmap> metalView;
        std::unique_ptr<image::TemporaryBitmap> roughView;
        if (metallic)
            metalView = std::make_unique<image::TemporaryBitmap>(*metallic, width, height, format);
        if (roughness)
            roughView = std::make_unique<image::TemporaryBitmap>(*roughness, width, height, format);

        for (int y = 0; y < height; ++y)
        {
            u32* out = packed->address<u32>(0, y);
            const u32* mp = metalView ? metalView->address<u32>(0, y) : nullptr;
            const u32* rp = roughView ? roughView->address<u32>(0, y) : nullptr;

            for (int x = 0; x < width; ++x)
            {
                // Authors usually store grayscale in RGB — take R.
                const u8 metal = mp ? u8(mp[x] & 0xff) : u8(0);
                const u8 rough = rp ? u8(rp[x] & 0xff) : u8(255);
                // R unused, G roughness, B metallic, A opaque.
                out[x] = u32(0xff000000u) | (u32(metal) << 16) | (u32(rough) << 8);
            }
        }

        return packed;
    }

    float shininessToRoughness(float shininess)
    {
        // Blinn-Phong specular power → perceptual roughness (approx).
        shininess = std::max(shininess, 1.0f);
        return std::sqrt(2.0f / (shininess + 2.0f));
    }

    struct ReaderFBX
    {
        ConstMemory m_memory;
        u32 m_version;

        MappingInformationType currentMappingType { ByPolygonVertex };
        ReferenceInformationType currentReferenceType { Direct };

        std::vector<MeshFBX> m_meshes;
        std::unordered_map<u64, MaterialFBX> m_materials;
        std::unordered_map<u64, TextureFBX> m_textures;
        std::unordered_map<u64, VideoFBX> m_videos;
        std::unordered_map<u64, ModelFBX> m_models;
        std::vector<ConnectionFBX> m_connections;

        // Object currently being filled by nested nodes (Properties70 / filenames).
        enum class Current : u8 { None, Geometry, Model, Material, Texture, Video };
        Current m_current { Current::None };
        u64 m_current_id { 0 };

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
                MANGO_UNREFERENCED(status);

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

        static bool holdsString(const Property& property)
        {
            return std::holds_alternative<std::string_view>(property.value);
        }

        static std::string_view getString(const Property& property)
        {
            return std::get<std::string_view>(property.value);
        }

        static bool holdsFloat(const Property& property)
        {
            return std::holds_alternative<float32>(property.value);
        }

        static float getFloat(const Property& property)
        {
            return std::get<float32>(property.value);
        }

        void readObjectHeader(const std::vector<Property>& properties, u64& id, std::string& name)
        {
            if (!properties.empty() && std::holds_alternative<u64>(properties[0].value))
                id = std::get<u64>(properties[0].value);
            if (properties.size() > 1 && holdsString(properties[1]))
                name = objectName(getString(properties[1]));
        }

        void parseMaterialProperty(MaterialFBX& material, const std::vector<Property>& properties)
        {
            if (properties.empty() || !holdsString(properties[0]))
                return;

            const std::string_view key = getString(properties[0]);

            auto readVec3 = [&] (float32x3& out)
            {
                if (properties.size() >= 7 && holdsFloat(properties[4]) &&
                    holdsFloat(properties[5]) && holdsFloat(properties[6]))
                {
                    out = float32x3(getFloat(properties[4]), getFloat(properties[5]), getFloat(properties[6]));
                }
            };

            auto readScalar = [&] (float& out)
            {
                if (properties.size() >= 5 && holdsFloat(properties[4]))
                    out = getFloat(properties[4]);
            };

            if (key == "DiffuseColor" || key == "Diffuse")
                readVec3(material.diffuse);
            else if (key == "DiffuseFactor")
                readScalar(material.diffuseFactor);
            else if (key == "EmissiveColor" || key == "Emissive")
                readVec3(material.emissive);
            else if (key == "EmissiveFactor")
                readScalar(material.emissiveFactor);
            else if (key == "ShininessExponent" || key == "Shininess")
                readScalar(material.shininess);
            else if (key == "Opacity")
                readScalar(material.opacity);
        }

        void storeFilename(const std::vector<Property>& properties, std::string& relative, std::string& absolute, bool isRelative)
        {
            if (properties.empty() || !holdsString(properties[0]))
                return;
            std::string value(getString(properties[0]));
            if (isRelative)
                relative = std::move(value);
            else
                absolute = std::move(value);
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
                    if (name != "Objects" && name != "Connections")
                    {
                        return end;
                    }
                    break;

                case 1:
                    // Under Objects: geometry + material graph. Under Connections: C records.
                    if (name != "Geometry" && name != "Model" && name != "Material" &&
                        name != "Texture" && name != "Video" && name != "C")
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
            }

            p = next;

            // ---- top-level object open ----
            if (name == "Geometry")
            {
                MeshFBX mesh;
                std::string unused;
                readObjectHeader(properties, mesh.id, unused);
                m_meshes.push_back(std::move(mesh));
                m_current = Current::Geometry;
                m_current_id = m_meshes.back().id;
            }
            else if (name == "Model")
            {
                ModelFBX model;
                readObjectHeader(properties, model.id, model.name);
                m_models[model.id] = model;
                m_current = Current::Model;
                m_current_id = model.id;
            }
            else if (name == "Material")
            {
                MaterialFBX material;
                readObjectHeader(properties, material.id, material.name);
                m_materials[material.id] = material;
                m_current = Current::Material;
                m_current_id = material.id;
            }
            else if (name == "Texture")
            {
                TextureFBX texture;
                readObjectHeader(properties, texture.id, texture.name);
                m_textures[texture.id] = texture;
                m_current = Current::Texture;
                m_current_id = texture.id;
            }
            else if (name == "Video")
            {
                VideoFBX video;
                readObjectHeader(properties, video.id, video.name);
                m_videos[video.id] = video;
                m_current = Current::Video;
                m_current_id = video.id;
            }
            else if (name == "C" && properties.size() >= 3 && holdsString(properties[0]))
            {
                ConnectionFBX connection;
                const std::string_view kind = getString(properties[0]);
                connection.isProperty = (kind == "OP");
                if (std::holds_alternative<u64>(properties[1].value) &&
                    std::holds_alternative<u64>(properties[2].value))
                {
                    connection.src = std::get<u64>(properties[1].value);
                    connection.dst = std::get<u64>(properties[2].value);
                    if (connection.isProperty && properties.size() >= 4 && holdsString(properties[3]))
                        connection.property = std::string(getString(properties[3]));
                    m_connections.push_back(std::move(connection));
                }
            }
            else if (name == "P")
            {
                if (m_current == Current::Material)
                {
                    auto it = m_materials.find(m_current_id);
                    if (it != m_materials.end())
                        parseMaterialProperty(it->second, properties);
                }
            }
            else if (name == "RelativeFilename")
            {
                if (m_current == Current::Texture)
                {
                    auto it = m_textures.find(m_current_id);
                    if (it != m_textures.end())
                        storeFilename(properties, it->second.relativeFilename, it->second.filename, true);
                }
                else if (m_current == Current::Video)
                {
                    auto it = m_videos.find(m_current_id);
                    if (it != m_videos.end())
                        storeFilename(properties, it->second.relativeFilename, it->second.filename, true);
                }
            }
            else if (name == "FileName" || name == "Filename")
            {
                if (m_current == Current::Texture)
                {
                    auto it = m_textures.find(m_current_id);
                    if (it != m_textures.end())
                        storeFilename(properties, it->second.relativeFilename, it->second.filename, false);
                }
                else if (m_current == Current::Video)
                {
                    auto it = m_videos.find(m_current_id);
                    if (it != m_videos.end())
                        storeFilename(properties, it->second.relativeFilename, it->second.filename, false);
                }
            }
            else if (name == "Content")
            {
                if (m_current == Current::Video && !properties.empty() &&
                    std::holds_alternative<ConstMemory>(properties[0].value))
                {
                    auto it = m_videos.find(m_current_id);
                    if (it != m_videos.end())
                        it->second.content = std::get<ConstMemory>(properties[0].value);
                }
            }
            else if (name == "MappingInformationType")
            {
                if (holdsString(properties[0]))
                {
                    auto str = getString(properties[0]);
                    if (str == "ByPolygonVertex")
                        currentMappingType = ByPolygonVertex;
                    else if (str == "ByVertice" || str == "ByVertex")
                        currentMappingType = ByVertice;
                    else if (str == "ByEdge")
                        currentMappingType = ByEdge;
                    else if (str == "AllSame")
                        currentMappingType = AllSame;
                    else if (str == "ByPolygon")
                        currentMappingType = ByPolygon;
                }
            }
            else if (name == "ReferenceInformationType")
            {
                if (holdsString(properties[0]))
                {
                    auto str = getString(properties[0]);
                    if (str == "Direct")
                        currentReferenceType = Direct;
                    else if (str == "IndexToDirect")
                        currentReferenceType = IndexToDirect;
                }
            }

            if (!m_meshes.empty() && m_current == Current::Geometry)
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
                            // FBX / Maya: (0,0) = bottom-left. mango / glTF: (0,0) = top-left
                            // of the image (same as how Bitmap stores rows). Flip V so UVs and
                            // loaded textures co-align without a viewer-side fix.
                            float x = vec[i + 0];
                            float y = 1.0f - vec[i + 1]; // Maya bottom-left → Vulkan/glTF top-left
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
                else if (name == "Materials")
                {
                    // LayerElementMaterial indices into materials connected to the parent Model.
                    if (std::holds_alternative<std::vector<u32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<u32>>(properties[0].value);
                        if (!vec.empty())
                        {
                            mesh.materialIndex = vec[0];
                            mesh.hasMaterialIndex = true;
                        }
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

        // Prefer RelativeFilename; fall back to basename of absolute FileName.
        // Always normalize separators so Path nesting matches glTF ("texture/foo.png").
        static std::string bestTexturePath(const TextureFBX& texture, const VideoFBX* video)
        {
            if (video)
            {
                if (!video->relativeFilename.empty())
                    return normalizePathSeparators(video->relativeFilename);
                if (!video->filename.empty())
                    return filesystem::removePath(normalizePathSeparators(video->filename));
            }
            if (!texture.relativeFilename.empty())
                return normalizePathSeparators(texture.relativeFilename);
            if (!texture.filename.empty())
                return filesystem::removePath(normalizePathSeparators(texture.filename));
            return {};
        }

        const VideoFBX* findVideoForTexture(u64 textureId) const
        {
            for (const ConnectionFBX& c : m_connections)
            {
                if (!c.isProperty && c.dst == textureId)
                {
                    auto it = m_videos.find(c.src);
                    if (it != m_videos.end())
                        return &it->second;
                }
            }
            return nullptr;
        }

        // Textures linked to a material via OP (property slot name).
        std::vector<std::pair<std::string, u64>> texturesForMaterial(u64 materialId) const
        {
            std::vector<std::pair<std::string, u64>> result;
            for (const ConnectionFBX& c : m_connections)
            {
                if (c.isProperty && c.dst == materialId)
                {
                    if (m_textures.count(c.src))
                        result.emplace_back(c.property, c.src);
                }
            }
            return result;
        }

        // Materials connected to a model (OO child→parent).
        std::vector<u64> materialsForModel(u64 modelId) const
        {
            std::vector<u64> result;
            for (const ConnectionFBX& c : m_connections)
            {
                if (!c.isProperty && c.dst == modelId && m_materials.count(c.src))
                    result.push_back(c.src);
            }
            return result;
        }

        u64 modelForGeometry(u64 geometryId) const
        {
            for (const ConnectionFBX& c : m_connections)
            {
                if (!c.isProperty && c.src == geometryId && m_models.count(c.dst))
                    return c.dst;
            }
            return 0;
        }
    };

} // namespace

namespace mango::import3d
{

    ImportFBX::ImportFBX(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);
        ReaderFBX reader(file);

        // ---- materials ----
        std::unordered_map<u64, u32> materialIdToIndex;

        if (reader.m_materials.empty())
        {
            Material material;
            material.name = "default";
            material.metallicFactor = 0.0f;
            material.roughnessFactor = 0.5f;
            materials.push_back(material);
        }
        else
        {
            for (const auto& [id, src] : reader.m_materials)
            {
                Material material;
                material.name = src.name;
                material.baseColorFactor = float32x4(src.diffuse * src.diffuseFactor, src.opacity);
                material.emissiveFactor = src.emissive * src.emissiveFactor;
                // Classic Phong → dielectric unless a metallic map is found below.
                material.metallicFactor = 0.0f;
                material.roughnessFactor = shininessToRoughness(src.shininess);

                std::string albedoDeclared;

                for (const auto& [slot, textureId] : reader.texturesForMaterial(id))
                {
                    auto texIt = reader.m_textures.find(textureId);
                    if (texIt == reader.m_textures.end())
                        continue;

                    const TextureFBX& fbxTex = texIt->second;
                    const VideoFBX* video = reader.findVideoForTexture(textureId);
                    const std::string declared = ReaderFBX::bestTexturePath(fbxTex, video);

                    Texture loaded;
                    if (video && video->content.size > 0)
                    {
                        try
                        {
                            loaded = createTexture(video->content);
                        }
                        catch (...)
                        {
                        }
                    }
                    if (!loaded)
                        loaded = resolveTexturePath(path, declared);

                    const std::string slotLower = toLower(slot);
                    if (slotLower == "diffusecolor" || slotLower == "diffuse" ||
                        slotLower == "basecolor" || slotLower == "basecolour")
                    {
                        material.baseColorTexture = loaded;
                        albedoDeclared = declared;
                    }
                    else if (slotLower == "normalmap" || slotLower == "bump")
                    {
                        material.normalTexture = loaded;
                    }
                    else if (slotLower == "emissivecolor" || slotLower == "emissive")
                    {
                        material.emissiveTexture = loaded;
                    }
                    else if (slotLower == "transparentcolor" || slotLower == "transparencyfactor")
                    {
                        // No dedicated opacity texture slot in mango Material; ignore for now.
                    }
                    else
                    {
                        // Unknown slot — if we have no albedo yet, treat as base color.
                        if (!material.baseColorTexture && loaded)
                        {
                            material.baseColorTexture = loaded;
                            albedoDeclared = declared;
                        }
                    }

                    printLine(Print::Verbose, "  [FBX] material '{}' slot '{}' -> '{}'",
                        material.name, slot, declared);
                }

                // Cerberus-style PBR sidecars next to the albedo map (_N / _M / _R).
                Texture sidecarNormal;
                Texture sidecarMetallic;
                Texture sidecarRoughness;
                loadPbrSidecars(path, albedoDeclared, sidecarNormal, sidecarMetallic, sidecarRoughness);

                if (!material.normalTexture && sidecarNormal)
                    material.normalTexture = sidecarNormal;

                if (sidecarMetallic || sidecarRoughness)
                {
                    material.metallicRoughnessTexture =
                        packMetallicRoughness(sidecarMetallic, sidecarRoughness);
                    if (sidecarMetallic)
                        material.metallicFactor = 1.0f;
                    if (sidecarRoughness)
                        material.roughnessFactor = 1.0f;
                }

                // Concise material summary (printEnable(Print::Info, true) to see this).
                printLine(Print::Info, "[FBX] material '{}'", material.name);
                if (material.baseColorTexture)
                    printLine(Print::Info, "  baseColor:  {}x{}  ('{}')",
                        material.baseColorTexture->width, material.baseColorTexture->height, albedoDeclared);
                else
                    printLine(Print::Info, "  baseColor:  none  ('{}')", albedoDeclared);
                if (material.normalTexture)
                    printLine(Print::Info, "  normal:     {}x{}{}",
                        material.normalTexture->width, material.normalTexture->height,
                        sidecarNormal ? "  [sidecar]" : "");
                else
                    printLine(Print::Info, "  normal:     none");
                if (material.metallicRoughnessTexture)
                    printLine(Print::Info, "  metalRough: {}x{}{}",
                        material.metallicRoughnessTexture->width, material.metallicRoughnessTexture->height,
                        (sidecarMetallic || sidecarRoughness) ? "  [sidecar _M/_R packed]" : "");
                else
                    printLine(Print::Info, "  metalRough: none");
                printLine(Print::Info, "  metallicFactor: {}  roughnessFactor: {}",
                    material.metallicFactor, material.roughnessFactor);

                materialIdToIndex[id] = u32(materials.size());
                materials.push_back(std::move(material));
            }
        }

        // ---- meshes ----
        std::unique_ptr<IndexedMesh> ptr = std::make_unique<IndexedMesh>();
        IndexedMesh& mesh = *ptr;

        for (const auto& current : reader.m_meshes)
        {
            Mesh trimesh;
            trimesh.flags = Vertex::Position | Vertex::Normal;

            bool hasNormals = !current.normals.values.empty();
            bool hasTexcoords = !current.texcoords.values.empty();
            bool hasTexcoordIndices = !current.texcoords.indices.empty();

            if (hasTexcoords)
            {
                trimesh.flags |= Vertex::Texcoord;
            }

            // Resolve material index for this geometry.
            u32 materialIndex = 0;
            if (!reader.m_materials.empty())
            {
                const u64 modelId = reader.modelForGeometry(current.id);
                std::vector<u64> modelMaterials =
                    modelId ? reader.materialsForModel(modelId) : std::vector<u64>{};

                if (modelMaterials.empty())
                {
                    // Fall back: first material in the file.
                    modelMaterials.push_back(reader.m_materials.begin()->first);
                }

                u32 localIndex = current.hasMaterialIndex ? current.materialIndex : 0;
                if (localIndex >= modelMaterials.size())
                    localIndex = 0;

                const u64 materialId = modelMaterials[localIndex];
                auto it = materialIdToIndex.find(materialId);
                if (it != materialIdToIndex.end())
                    materialIndex = it->second;
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
                    // FBX geometry is typically RH Y-up (same family as glTF).
                    // 180° about Y → our +Z ahead; winding stays CCW until reverse below.
                    auto toOurs = [](const float32x3& v) {
                        return float32x3(-v.x, v.y, -v.z);
                    };

                    float32x3 position0 = toOurs(current.positions.values[tempIndex[0]]);
                    float32x3 position1 = toOurs(current.positions.values[tempIndex[1]]);
                    float32x3 position2 = toOurs(current.positions.values[tempIndex[2]]);

                    triangle.vertex[0].position = position0;
                    triangle.vertex[1].position = position1;
                    triangle.vertex[2].position = position2;

                    if (current.normals.mappingType == ByVertice)
                    {
                        triangle.vertex[0].normal = toOurs(current.normals.values[tempIndex[0]]);
                        triangle.vertex[1].normal = toOurs(current.normals.values[tempIndex[1]]);
                        triangle.vertex[2].normal = toOurs(current.normals.values[tempIndex[2]]);
                    }
                    else if (hasNormals && current.normals.mappingType == ByPolygonVertex)
                    {
                        for (int vi = 0; vi < 3; ++vi)
                            triangle.vertex[vi].normal = toOurs(triangle.vertex[vi].normal);
                    }

                    // Bake CW outside.
                    std::swap(triangle.vertex[1], triangle.vertex[2]);

                    if (!hasNormals)
                    {
                        // TODO: smoothing groups (need file for testing)
                        float32x3 p0 = triangle.vertex[0].position;
                        float32x3 p1 = triangle.vertex[1].position;
                        float32x3 p2 = triangle.vertex[2].position;
                        float32x3 normal = normalize(cross(p0 - p2, p0 - p1));

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

            mesh.append(trimesh, materialIndex);
        }

        meshes.push_back(std::move(ptr));

        Node node;

        node.name = reader.m_models.empty() ? "FBX.object" : reader.m_models.begin()->second.name;
        node.transform = matrix4x4(1.0f);
        node.mesh = 0;

        nodes.push_back(node);
        roots.push_back(0);
    }

} // namespace mango::import3d
