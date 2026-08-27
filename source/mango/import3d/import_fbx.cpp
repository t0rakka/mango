/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <functional>
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
        std::vector<s32> indices;
        MappingInformationType mappingType { ByPolygonVertex };
        ReferenceInformationType referenceType { Direct };
    };

    struct MeshFBX
    {
        u64 id = 0;
        ArrayFBX<float32x3> positions;
        ArrayFBX<float32x3> normals;
        ArrayFBX<float32x2> texcoords;
        // LayerElementMaterial: indices into materials linked to the parent Model.
        // ByPolygon (typical): one entry per polygon. AllSame: a single entry.
        ArrayFBX<s32> materials;
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
        std::string type; // Mesh, LimbNode, Null, Root, ...
        float32x3 translation { 0.0f, 0.0f, 0.0f };
        float32x3 rotationDeg { 0.0f, 0.0f, 0.0f }; // Lcl Rotation, degrees XYZ
        float32x3 scaling { 1.0f, 1.0f, 1.0f };

        matrix4x4 localMatrix() const
        {
            constexpr float degToRad = 0.017453292519943295769f;
            // Row-vector convention (v * M): S * R * T  ≡  column-vector T * R * S.
            return matrix4x4::scale(scaling.x, scaling.y, scaling.z) *
                   matrix4x4::rotateXYZ(rotationDeg.x * degToRad,
                                        rotationDeg.y * degToRad,
                                        rotationDeg.z * degToRad) *
                   matrix4x4::translate(translation.x, translation.y, translation.z);
        }
    };

    struct ClusterFBX
    {
        u64 id = 0;
        std::string name;
        std::vector<s32> indexes;   // control-point indices
        std::vector<float> weights;
        matrix4x4 transform { 1.0f };     // mesh bind
        matrix4x4 transformLink { 1.0f }; // bone world bind
        bool hasTransform = false;
        bool hasTransformLink = false;
    };

    struct SkinDeformerFBX
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
            std::vector<s32>,
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

    std::string joinRelativePath(const std::string& dir, const std::string& file)
    {
        if (dir.empty())
            return file;
        std::string result = dir;
        if (result.back() != '/')
            result += '/';
        result += file;
        return result;
    }

    std::string tryResolveInDirectory(const filesystem::Path& dir, const std::string& basename, const std::string& stem)
    {
        // Exact / case-insensitive basename first (preserves declared extension when present).
        std::string found = findFileIgnoreCase(dir, basename);
        if (!found.empty())
            return found;

        // Stem match: RelativeFilename says .tga, disk has .png (very common).
        found = findFileByStemIgnoreCase(dir, stem);
        if (!found.empty())
            return found;

        return {};
    }

    std::string tryResolveUnder(const filesystem::Path& root, const std::string& relativeDir,
                                const std::string& basename, const std::string& stem)
    {
        if (relativeDir.empty())
            return {};

        try
        {
            // Same nesting model as glTF: Path(assetDir, "texture/") + basename.
            filesystem::Path dir(root, relativeDir);
            const std::string found = tryResolveInDirectory(dir, basename, stem);
            if (found.empty())
                return {};
            return joinRelativePath(relativeDir, found);
        }
        catch (...)
        {
            return {};
        }
    }

    // Resolve an FBX RelativeFilename / FileName against the mesh directory.
    // Returns a path relative to meshPath, or empty if no file exists on disk.
    std::string resolveImageFilename(const filesystem::Path& meshPath, const std::string& declared)
    {
        if (declared.empty())
            return {};

        const std::string normalized = normalizePathSeparators(declared);
        const std::string subdir = filesystem::getPath(normalized); // "texture/" or ""
        const std::string basename = filesystem::removePath(normalized);
        const std::string stem = filesystem::removeExtension(basename);

        // 0. Exact declared relative path (glTF-style), including subdirs.
        if (!findFileIgnoreCase(meshPath, normalized).empty())
            return normalized;

        // 1. Declared subdirectory next to the .fbx (Textures\Cerberus_A.tga), case-insensitive.
        if (!subdir.empty())
        {
            const std::string resolved = findSubdirectoryIgnoreCase(meshPath, subdir);
            if (!resolved.empty())
            {
                if (std::string found = tryResolveUnder(meshPath, resolved, basename, stem); !found.empty())
                    return found;
            }
        }

        // 2. Next to the .fbx.
        if (std::string found = tryResolveInDirectory(meshPath, basename, stem); !found.empty())
            return found;

        // 3. Conventional texture folders next to the .fbx.
        {
            const char* folders[] = { "textures/", "Textures/", "texture/", "Texture/" };
            for (const char* folder : folders)
            {
                if (std::string found = tryResolveUnder(meshPath, folder, basename, stem); !found.empty())
                    return found;
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
                    if (std::string found = tryResolveUnder(parentPath, folder, basename, stem); !found.empty())
                        return joinRelativePath("..", found);
                }
            }
            catch (...)
            {
            }
        }

        return {};
    }

    std::string extensionFromFilename(const std::string& filename)
    {
        const std::string ext = filesystem::getExtension(filename);
        return ext.empty() ? std::string(".png") : ext;
    }

    // Per-import image cache: FBX often has many Texture objects / materials
    // pointing at the same RelativeFilename (shared atlases, repeated slots).
    struct ImageSourceCache
    {
        std::unordered_map<std::string, u32> byKey;

        u32 loadFile(const filesystem::Path& meshPath, const std::string& declared,
                     const std::function<u32(const std::string&)>& addResolvedFile)
        {
            if (declared.empty())
                return ImageSample::none;

            const std::string key = "file:" + normalizePathSeparators(declared);
            auto it = byKey.find(key);
            if (it != byKey.end())
                return it->second;

            u32 index = ImageSample::none;
            const std::string resolved = resolveImageFilename(meshPath, declared);
            if (!resolved.empty())
                index = addResolvedFile(resolved);

            byKey.emplace(key, index);
            return index;
        }

        u32 loadEmbedded(u64 videoId, ConstMemory content, const std::string& extension, const std::string& debugName,
                         const std::function<u32(ConstMemory, const std::string&, const std::string&)>& addMemory)
        {
            const std::string key = "embed:" + std::to_string(videoId);
            auto it = byKey.find(key);
            if (it != byKey.end())
                return it->second;

            u32 index = ImageSample::none;
            if (content.size > 0)
                index = addMemory(content, extension, debugName);

            byKey.emplace(key, index);
            return index;
        }
    };

    // Probe Cerberus-style sidecars: stem ending in _A / Albedo → _N / _M / _R next to it.
    void loadPbrSidecars(const filesystem::Path& meshPath, const std::string& albedoDeclared,
                         ImageSourceCache& cache,
                         const std::function<u32(const std::string&)>& addResolvedFile,
                         u32& outNormal, u32& outMetallic, u32& outRoughness)
    {
        outNormal = ImageSample::none;
        outMetallic = ImageSample::none;
        outRoughness = ImageSample::none;

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

        auto loadVariant = [&] (std::initializer_list<const char*> variants) -> u32
        {
            for (const char* variant : variants)
            {
                u32 idx = cache.loadFile(meshPath, std::string("Textures/") + prefix + variant + ".png", addResolvedFile);
                if (idx != ImageSample::none)
                    return idx;
                idx = cache.loadFile(meshPath, prefix + variant + ".png", addResolvedFile);
                if (idx != ImageSample::none)
                    return idx;
            }
            return ImageSample::none;
        };

        outNormal = loadVariant({ "_N", "_Normal", "_NormalMap", "_Nor" });
        outMetallic = loadVariant({ "_M", "_Metallic", "_Metalness", "_Metal" });
        outRoughness = loadVariant({ "_R", "_Roughness", "_Rough" });
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

        // FBX GlobalSettings axis system (0=X, 1=Y, 2=Z). Defaults = Maya Y-up.
        s32 m_upAxis = 1;
        s32 m_upSign = 1;
        s32 m_frontAxis = 2;
        s32 m_frontSign = -1; // Maya: front toward viewer along −Z
        s32 m_coordAxis = 0;
        s32 m_coordSign = 1;
        s32 m_originalUpAxis = -1; // -1 = unknown / not authored
        bool m_axisFromFile = false;

        std::vector<MeshFBX> m_meshes;
        std::unordered_map<u64, MaterialFBX> m_materials;
        std::unordered_map<u64, TextureFBX> m_textures;
        std::unordered_map<u64, VideoFBX> m_videos;
        std::unordered_map<u64, ModelFBX> m_models;
        std::unordered_map<u64, ClusterFBX> m_clusters;
        std::unordered_map<u64, SkinDeformerFBX> m_skin_deformers;
        std::vector<ConnectionFBX> m_connections;

        // Object currently being filled by nested nodes (Properties70 / filenames).
        enum class Current : u8
        {
            None, Geometry, Model, Material, Texture, Video, GlobalSettings,
            Cluster, SkinDeformer
        };
        Current m_current { Current::None };
        u64 m_current_id { 0 };
        bool m_skip_uv_layer { false };

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

        static bool holdsInt(const Property& property)
        {
            return std::holds_alternative<u32>(property.value) ||
                   std::holds_alternative<u16>(property.value);
        }

        static s32 getInt(const Property& property)
        {
            if (std::holds_alternative<u32>(property.value))
                return s32(std::get<u32>(property.value));
            if (std::holds_alternative<u16>(property.value))
                return s32(std::get<u16>(property.value));
            return 0;
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

        void readObjectHeader(const std::vector<Property>& properties, u64& id, std::string& name, std::string& type)
        {
            readObjectHeader(properties, id, name);
            if (properties.size() > 2 && holdsString(properties[2]))
                type = std::string(getString(properties[2]));
        }

        void parseAxisProperty(const std::vector<Property>& properties)
        {
            if (properties.empty() || !holdsString(properties[0]) || properties.size() < 5)
                return;
            if (!holdsInt(properties[4]))
                return;

            const std::string_view key = getString(properties[0]);
            const s32 value = getInt(properties[4]);

            if (key == "UpAxis")
            {
                m_upAxis = value;
                m_axisFromFile = true;
            }
            else if (key == "UpAxisSign")
                m_upSign = value < 0 ? -1 : 1;
            else if (key == "FrontAxis")
                m_frontAxis = value;
            else if (key == "FrontAxisSign")
                m_frontSign = value < 0 ? -1 : 1;
            else if (key == "CoordAxis")
                m_coordAxis = value;
            else if (key == "CoordAxisSign")
                m_coordSign = value < 0 ? -1 : 1;
            else if (key == "OriginalUpAxis")
                m_originalUpAxis = value; // 0xFFFFFFFF → -1 (unknown)
        }

        // Map FBX vectors into engine space: +X right, +Y up, +Z ahead (Unity-like LH),
        // matching glTF importer output.
        float32x3 toOurs(const float32x3& p) const
        {
            // Tripo and some converters leave UpAxis=Y / FrontAxis=Z while the mesh
            // itself is still Z-up with +X forward (OriginalUpAxis == -1). Trusting
            // GlobalSettings then yields: red→ahead, green→left, blue→down.
            // Remap (-Y, Z, X) restores right / up / ahead.
            if (m_originalUpAxis < 0)
                return float32x3(-p.y, p.z, p.x);

            auto axis = [](s32 index, s32 sign) -> float32x3
            {
                const float s = float(sign);
                switch (index)
                {
                    case 0:  return float32x3(s, 0.0f, 0.0f);
                    case 1:  return float32x3(0.0f, s, 0.0f);
                    case 2:  return float32x3(0.0f, 0.0f, s);
                    default: return float32x3(0.0f, 0.0f, 0.0f);
                }
            };

            // FBX "Front" points toward the viewer (glTF +Z). Engine ahead = −front.
            const float32x3 right = axis(m_coordAxis, m_coordSign);
            const float32x3 up = axis(m_upAxis, m_upSign);
            const float32x3 front = axis(m_frontAxis, m_frontSign);

            return float32x3(dot(p, right), dot(p, up), -dot(p, front));
        }

        // Change-of-basis for matrices so skin IBMs / node locals match toOurs positions.
        // Row-vector form: M_ours = R^{-1} * M_fbx * R  where p_ours = p_fbx * R.
        matrix4x4 toOursMatrix(const matrix4x4& m) const
        {
            const float32x3 e0 = toOurs(float32x3(1.0f, 0.0f, 0.0f));
            const float32x3 e1 = toOurs(float32x3(0.0f, 1.0f, 0.0f));
            const float32x3 e2 = toOurs(float32x3(0.0f, 0.0f, 1.0f));

            // R such that [x,y,z] * R = toOurs(x,y,z)
            matrix4x4 R(1.0f);
            R(0, 0) = e0.x; R(0, 1) = e0.y; R(0, 2) = e0.z;
            R(1, 0) = e1.x; R(1, 1) = e1.y; R(1, 2) = e1.z;
            R(2, 0) = e2.x; R(2, 1) = e2.y; R(2, 2) = e2.z;

            return inverse(R) * m * R;
        }

        static matrix4x4 matrixFrom16(const std::vector<float>& v)
        {
            if (v.size() < 16)
                return matrix4x4(1.0f);
            // FBX stores 4x4 as 16 floats in row-major order.
            return matrix4x4(
                v[0],  v[1],  v[2],  v[3],
                v[4],  v[5],  v[6],  v[7],
                v[8],  v[9],  v[10], v[11],
                v[12], v[13], v[14], v[15]);
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

        void parseModelProperty(ModelFBX& model, const std::vector<Property>& properties)
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

            if (key == "Lcl Translation")
                readVec3(model.translation);
            else if (key == "Lcl Rotation")
                readVec3(model.rotationDeg);
            else if (key == "Lcl Scaling")
                readVec3(model.scaling);
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

            printLine(Print::Debug, level * 2, "[{}]", name);

            const u8* end = m_memory.address + endOffset;

            switch (level)
            {
                case 0:
                    if (name != "Objects" && name != "Connections" && name != "GlobalSettings")
                    {
                        return end;
                    }
                    break;

                case 1:
                    // Under Objects: geometry + material graph + deformers. Under Connections: C records.
                    // Under GlobalSettings: Properties70.
                    if (name != "Geometry" && name != "Model" && name != "Material" &&
                        name != "Texture" && name != "Video" && name != "Deformer" &&
                        name != "C" && name != "Properties70")
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
                        printLine(Print::Debug, level * 2 + 2, "u8: {}", value);
                        break;
                    }

                    case 'Y':
                    {
                        u16 value = p.read16();
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "u16: {}", value);
                        break;
                    }

                    case 'I':
                    {
                        u32 value = p.read32();
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "u32: {}", value);
                        break;
                    }

                    case 'L':
                    {
                        u64 value = p.read64();
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "u64: {}", value);
                        break;
                    }

                    case 'F':
                    {
                        float value = p.read32f();
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "f32: {}", value);
                        break;
                    }

                    case 'D':
                    {
                        double value = p.read64f();
                        property.value = float(value);
                        printLine(Print::Debug, level * 2 + 2, "f64: {}", value);
                        break;
                    }

                    case 'f':
                    {
                        auto value = read_property_array<float, float>(p);
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "f32[{}]", value.size());
                        break;
                    }

                    case 'd':
                    {
                        auto value = read_property_array<float, double>(p);
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "f64[{}]", value.size());
                        break;
                    }

                    case 'l':
                    {
                        auto value = read_property_array<u64, u64>(p);
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "u64[{}]", value.size());
                        break;
                    }

                    case 'i':
                    {
                        // Signed: PolygonVertexIndex marks polygon ends with ~index.
                        auto value = read_property_array<s32, s32>(p);
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "s32[{}]", value.size());
                        break;
                    }

                    case 'b':
                    {
                        auto value = read_property_array<u8, u8>(p);
                        property.value = value;
                        printLine(Print::Debug, level * 2 + 2, "u8[{}]", value.size());
                        break;
                    }

                    case 'S':
                    {
                        u32 length = p.read32();
                        std::string_view value(p.cast<const char>(), length);
                        property.value = value;
                        p += length;
                        printLine(Print::Debug, level * 2 + 2, "string: \"{}\"", value);
                        break;
                    }

                    case 'R':
                    {
                        u32 length = p.read32();
                        ConstMemory value(p, length);
                        property.value = value;
                        p += length;
                        printLine(Print::Debug, level * 2 + 2, "raw: {} bytes", length);
                        break;
                    }

                    default:
                        MANGO_EXCEPTION("[ImportFBX] Incorrect property type.");
                        break;
                }
            }

            p = next;

            // ---- top-level object open ----
            if (name == "GlobalSettings")
            {
                m_current = Current::GlobalSettings;
            }
            else if (name == "Geometry")
            {
                MeshFBX mesh;
                std::string unused;
                readObjectHeader(properties, mesh.id, unused);
                m_meshes.push_back(std::move(mesh));
                m_current = Current::Geometry;
                m_current_id = m_meshes.back().id;
                m_skip_uv_layer = false;
            }
            else if (name == "Model")
            {
                ModelFBX model;
                readObjectHeader(properties, model.id, model.name, model.type);
                m_models[model.id] = model;
                m_current = Current::Model;
                m_current_id = model.id;
            }
            else if (name == "Deformer")
            {
                std::string subtype;
                u64 id = 0;
                std::string dname;
                readObjectHeader(properties, id, dname, subtype);
                if (subtype == "Skin")
                {
                    SkinDeformerFBX skin;
                    skin.id = id;
                    skin.name = dname;
                    m_skin_deformers[id] = skin;
                    m_current = Current::SkinDeformer;
                    m_current_id = id;
                }
                else if (subtype == "Cluster")
                {
                    ClusterFBX cluster;
                    cluster.id = id;
                    cluster.name = dname;
                    m_clusters[id] = cluster;
                    m_current = Current::Cluster;
                    m_current_id = id;
                }
                else
                {
                    m_current = Current::None;
                    m_current_id = 0;
                }
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
            else if (name == "LayerElementNormal")
            {
                // Defaults for this layer; Mapping/Reference nodes override before data arrays.
                currentMappingType = ByPolygonVertex;
                currentReferenceType = Direct;
                if (!m_meshes.empty() && m_current == Current::Geometry)
                    m_meshes.back().normals = {};
            }
            else if (name == "LayerElementUV")
            {
                currentMappingType = ByPolygonVertex;
                currentReferenceType = Direct;
                // First UV set only (ignore lightmap / extra UV channels).
                m_skip_uv_layer = !m_meshes.empty()
                    && m_current == Current::Geometry
                    && !m_meshes.back().texcoords.values.empty();
                if (!m_skip_uv_layer && !m_meshes.empty() && m_current == Current::Geometry)
                    m_meshes.back().texcoords = {};
            }
            else if (name == "LayerElementMaterial")
            {
                currentMappingType = AllSame;
                currentReferenceType = Direct;
                if (!m_meshes.empty() && m_current == Current::Geometry)
                    m_meshes.back().materials = {};
            }
            else if (name == "P")
            {
                if (m_current == Current::Material)
                {
                    auto it = m_materials.find(m_current_id);
                    if (it != m_materials.end())
                        parseMaterialProperty(it->second, properties);
                }
                else if (m_current == Current::Model)
                {
                    auto it = m_models.find(m_current_id);
                    if (it != m_models.end())
                        parseModelProperty(it->second, properties);
                }
                else if (m_current == Current::GlobalSettings)
                {
                    parseAxisProperty(properties);
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

                        for (size_t i = 0; i + 2 < vec.size(); i += 3)
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
                    if (std::holds_alternative<std::vector<s32>>(properties[0].value))
                    {
                        mesh.positions.indices = std::get<std::vector<s32>>(properties[0].value);
                    }
                }
                else if (name == "Normals")
                {
                    if (std::holds_alternative<std::vector<float32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<float32>>(properties[0].value);

                        mesh.normals.mappingType = currentMappingType;
                        mesh.normals.referenceType = currentReferenceType;

                        for (size_t i = 0; i + 2 < vec.size(); i += 3)
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
                    // Optional 4th normal component / weight in some exporters — not tangent.w.
                    // Bitangent sign comes from mikktspace (or LayerElementTangent) later.
                }
                else if (name == "NormalsIndex" || name == "NormalIndex")
                {
                    if (std::holds_alternative<std::vector<s32>>(properties[0].value))
                    {
                        mesh.normals.indices = std::get<std::vector<s32>>(properties[0].value);
                        mesh.normals.referenceType = IndexToDirect;
                    }
                }
                else if (name == "UV")
                {
                    if (!m_skip_uv_layer &&
                        std::holds_alternative<std::vector<float32>>(properties[0].value))
                    {
                        auto vec = std::get<std::vector<float32>>(properties[0].value);

                        mesh.texcoords.mappingType = currentMappingType;
                        mesh.texcoords.referenceType = currentReferenceType;
                        mesh.texcoords.values.clear();

                        for (size_t i = 0; i + 1 < vec.size(); i += 2)
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
                    if (!m_skip_uv_layer &&
                        std::holds_alternative<std::vector<s32>>(properties[0].value))
                    {
                        mesh.texcoords.indices = std::get<std::vector<s32>>(properties[0].value);
                        mesh.texcoords.referenceType = IndexToDirect;
                    }
                }
                else if (name == "Materials")
                {
                    // Per-polygon (or AllSame) indices into materials connected to the parent Model.
                    if (std::holds_alternative<std::vector<s32>>(properties[0].value))
                    {
                        mesh.materials.values = std::get<std::vector<s32>>(properties[0].value);
                        mesh.materials.mappingType = currentMappingType;
                        mesh.materials.referenceType = currentReferenceType;
                    }
                }
                else if (name == "MaterialsIndex" || name == "MaterialIndex")
                {
                    if (std::holds_alternative<std::vector<s32>>(properties[0].value))
                    {
                        mesh.materials.indices = std::get<std::vector<s32>>(properties[0].value);
                        mesh.materials.referenceType = IndexToDirect;
                    }
                }
                else if (name == "Smoothing")
                {
                    // TODO
                }
            }

            if (m_current == Current::Cluster)
            {
                auto it = m_clusters.find(m_current_id);
                if (it != m_clusters.end())
                {
                    ClusterFBX& cluster = it->second;

                    if (name == "Indexes")
                    {
                        if (std::holds_alternative<std::vector<s32>>(properties[0].value))
                            cluster.indexes = std::get<std::vector<s32>>(properties[0].value);
                    }
                    else if (name == "Weights")
                    {
                        if (std::holds_alternative<std::vector<float32>>(properties[0].value))
                            cluster.weights = std::get<std::vector<float32>>(properties[0].value);
                    }
                    else if (name == "Transform")
                    {
                        if (std::holds_alternative<std::vector<float32>>(properties[0].value))
                        {
                            cluster.transform = matrixFrom16(std::get<std::vector<float32>>(properties[0].value));
                            cluster.hasTransform = true;
                        }
                    }
                    else if (name == "TransformLink")
                    {
                        if (std::holds_alternative<std::vector<float32>>(properties[0].value))
                        {
                            cluster.transformLink = matrixFrom16(std::get<std::vector<float32>>(properties[0].value));
                            cluster.hasTransformLink = true;
                        }
                    }
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

        // Skin deformer parented under a geometry (Skin → Geometry).
        u64 skinForGeometry(u64 geometryId) const
        {
            for (const ConnectionFBX& c : m_connections)
            {
                if (!c.isProperty && c.dst == geometryId && m_skin_deformers.count(c.src))
                    return c.src;
            }
            return 0;
        }

        // Clusters parented under a skin (Cluster → Skin).
        std::vector<u64> clustersForSkin(u64 skinId) const
        {
            std::vector<u64> result;
            for (const ConnectionFBX& c : m_connections)
            {
                if (!c.isProperty && c.dst == skinId && m_clusters.count(c.src))
                    result.push_back(c.src);
            }
            return result;
        }

        // Bone Model linked to a cluster (Bone → Cluster).
        u64 boneForCluster(u64 clusterId) const
        {
            for (const ConnectionFBX& c : m_connections)
            {
                if (!c.isProperty && c.dst == clusterId && m_models.count(c.src))
                    return c.src;
            }
            return 0;
        }
    };

} // namespace

namespace mango::import3d
{

    ImportFBX::ImportFBX(const filesystem::Path& path, const std::string& filename)
        : Scene(path)
    {
        // Keep the FBX mapping alive — Video Content embeds are ConstMemory into this file.
        auto file = std::make_shared<filesystem::File>(path, filename);
        ReaderFBX reader(*file);
        resources->files.push_back(std::move(file));

        std::unordered_map<std::string, u32> imageIndexByKey;

        auto addResolvedFile = [&](const std::string& filename) -> u32
        {
            if (filename.empty())
                return ImageSample::none;

            auto it = imageIndexByKey.find(filename);
            if (it != imageIndexByKey.end())
                return it->second;

            u32 idx = u32(images.size());
            images.push_back(ImageSource::fromFile(filename));
            imageIndexByKey.emplace(filename, idx);
            return idx;
        };

        auto addEmbeddedImage = [&](ConstMemory content, const std::string& extension, const std::string& debugName) -> u32
        {
            if (content.size == 0)
                return ImageSample::none;

            u32 idx = u32(images.size());
            images.push_back(ImageSource::fromMemory(content, extension, debugName));
            return idx;
        };

        const char* axisName[] = { "X", "Y", "Z" };
        const int up = reader.m_upAxis >= 0 && reader.m_upAxis <= 2 ? reader.m_upAxis : 1;
        const int front = reader.m_frontAxis >= 0 && reader.m_frontAxis <= 2 ? reader.m_frontAxis : 2;
        const int coord = reader.m_coordAxis >= 0 && reader.m_coordAxis <= 2 ? reader.m_coordAxis : 0;
        printLine(Print::Debug, "[FBX] axes: up={}{} front={}{} coord={}{}{}",
            reader.m_upSign < 0 ? "-" : "+",
            axisName[up],
            reader.m_frontSign < 0 ? "-" : "+",
            axisName[front],
            reader.m_coordSign < 0 ? "-" : "+",
            axisName[coord],
            reader.m_originalUpAxis < 0 ? "  (OriginalUpAxis unknown → Z-up remap)"
                : reader.m_axisFromFile ? "" : "  (defaults)");

        // ---- materials ----

        std::unordered_map<u64, u32> materialIdToIndex;
        ImageSourceCache imageCache;

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
                // Classic Phong defaults; overridden when PBR maps are present.
                material.metallicFactor = 0.0f;
                material.roughnessFactor = shininessToRoughness(src.shininess);

                std::string albedoDeclared;
                u32 slotMetallic = ImageSample::none;
                u32 slotRoughness = ImageSample::none;

                for (const auto& [slot, textureId] : reader.texturesForMaterial(id))
                {
                    auto texIt = reader.m_textures.find(textureId);
                    if (texIt == reader.m_textures.end())
                        continue;

                    const TextureFBX& fbxTex = texIt->second;
                    const VideoFBX* video = reader.findVideoForTexture(textureId);
                    const std::string declared = ReaderFBX::bestTexturePath(fbxTex, video);

                    u32 imageIndex = ImageSample::none;
                    if (video && video->content.size > 0)
                    {
                        const std::string extension = extensionFromFilename(
                            !video->relativeFilename.empty() ? video->relativeFilename : video->filename);
                        imageIndex = imageCache.loadEmbedded(video->id, video->content, extension, video->name, addEmbeddedImage);
                    }
                    if (imageIndex == ImageSample::none && !declared.empty())
                        imageIndex = imageCache.loadFile(path, declared, addResolvedFile);

                    const std::string slotLower = toLower(slot);
                    if (slotLower == "diffusecolor" || slotLower == "diffuse" ||
                        slotLower == "basecolor" || slotLower == "basecolour")
                    {
                        if (imageIndex != ImageSample::none)
                            material.baseColor = ImageSample::from(imageIndex, ImageSwizzle::rgba(), ImageColorSpace::sRGB);
                        albedoDeclared = declared;
                    }
                    else if (slotLower == "normalmap" || slotLower == "bump" ||
                             slotLower == "normal")
                    {
                        if (imageIndex != ImageSample::none)
                            material.normal = ImageSample::from(imageIndex, ImageSwizzle::rgb1(), ImageColorSpace::Linear);
                    }
                    else if (slotLower == "emissivecolor" || slotLower == "emissive")
                    {
                        if (imageIndex != ImageSample::none)
                            material.emissive = ImageSample::from(imageIndex, ImageSwizzle::rgb1(), ImageColorSpace::sRGB);
                    }
                    else if (slotLower == "shininessexponent" || slotLower == "shininess" ||
                             slotLower == "roughness" || slotLower == "specularroughness")
                    {
                        // Tripo / PBR-in-Phong: ShininessExponent often carries a roughness map.
                        slotRoughness = imageIndex;
                    }
                    else if (slotLower == "reflectionfactor" || slotLower == "reflection" ||
                             slotLower == "metallic" || slotLower == "metalness" ||
                             slotLower == "specularfactor")
                    {
                        // ReflectionFactor commonly carries a metallic map in PBR FBX exports.
                        slotMetallic = imageIndex;
                    }
                    else if (slotLower == "transparentcolor" || slotLower == "transparencyfactor")
                    {
                        // No dedicated opacity texture slot in mango Material; ignore for now.
                    }
                    else
                    {
                        // Unknown slot — if we have no albedo yet, treat as base color.
                        if (!material.baseColor && imageIndex != ImageSample::none)
                        {
                            material.baseColor = ImageSample::from(imageIndex, ImageSwizzle::rgba(), ImageColorSpace::sRGB);
                            albedoDeclared = declared;
                        }
                    }

                    printLine(Print::Verbose, "  [FBX] material '{}' slot '{}' -> '{}'",
                        material.name, slot, declared);
                }

                // Cerberus-style PBR sidecars next to the albedo map (_N / _M / _R).
                u32 sidecarNormal = ImageSample::none;
                u32 sidecarMetallic = ImageSample::none;
                u32 sidecarRoughness = ImageSample::none;
                loadPbrSidecars(path, albedoDeclared, imageCache, addResolvedFile,
                    sidecarNormal, sidecarMetallic, sidecarRoughness);

                if (!material.normal && sidecarNormal != ImageSample::none)
                    material.normal = ImageSample::from(sidecarNormal, ImageSwizzle::rgb1(), ImageColorSpace::Linear);

                // Prefer explicit FBX property slots; fill gaps from filename sidecars.
                const u32 metallicMap = slotMetallic != ImageSample::none ? slotMetallic : sidecarMetallic;
                const u32 roughnessMap = slotRoughness != ImageSample::none ? slotRoughness : sidecarRoughness;

                if (metallicMap != ImageSample::none)
                {
                    material.metallic = ImageSample::from(metallicMap, ImageSwizzle::r(), ImageColorSpace::Linear);
                    material.metallicFactor = 1.0f;
                }
                if (roughnessMap != ImageSample::none)
                {
                    material.roughness = ImageSample::from(roughnessMap, ImageSwizzle::r(), ImageColorSpace::Linear);
                    material.roughnessFactor = 1.0f;
                }

                // Concise material summary
                printLine(Print::Verbose, "[FBX] material '{}'", material.name);
                if (material.baseColor)
                    printLine(Print::Verbose, "  baseColor:  image[{}]  ('{}')", material.baseColor.image, albedoDeclared);
                else
                    printLine(Print::Verbose, "  baseColor:  none  ('{}')", albedoDeclared);
                if (material.normal)
                    printLine(Print::Verbose, "  normal:     image[{}]{}", material.normal.image,
                        sidecarNormal != ImageSample::none ? "  [sidecar]" : "");
                else
                    printLine(Print::Verbose, "  normal:     none");
                if (material.metallic || material.roughness)
                    printLine(Print::Verbose, "  metalRough: metal={} rough={}{}",
                        material.metallic ? material.metallic.image : u32(ImageSample::none),
                        material.roughness ? material.roughness.image : u32(ImageSample::none),
                        (slotMetallic != ImageSample::none || slotRoughness != ImageSample::none)
                            ? "  [ShininessExponent/ReflectionFactor]"
                        : (sidecarMetallic != ImageSample::none || sidecarRoughness != ImageSample::none)
                            ? "  [sidecar _M/_R]" : "");
                else
                    printLine(Print::Verbose, "  metalRough: none");
                printLine(Print::Verbose, "  metallicFactor: {}  roughnessFactor: {}",
                    material.metallicFactor, material.roughnessFactor);

                materialIdToIndex[id] = u32(materials.size());
                materials.push_back(std::move(material));
            }

            printLine(Print::Info, "[FBX] images: {}", images.size());
        }

        // ---- skeleton nodes (all FBX Models) ----

        std::unordered_map<u64, u32> modelIdToNode;
        nodes.reserve(reader.m_models.size() + 1);

        for (const auto& [id, model] : reader.m_models)
        {
            Node node;
            node.name = model.name;
            node.transform = reader.toOursMatrix(model.localMatrix());

            // Fill bind TRS for external clips (BVH) but keep the matrix as the
            // skinning source of truth. Decompose→compose does not round-trip for
            // FBX (Euler Lcl + basis change), and marking hasTRS would rebuild
            // locals that no longer match IBMs → exploded skin.
            {
                const matrix4x4& m = node.transform;
                float32x3 xaxis(m[0].x, m[0].y, m[0].z);
                float32x3 yaxis(m[1].x, m[1].y, m[1].z);
                float32x3 zaxis(m[2].x, m[2].y, m[2].z);
                node.translation = float32x3(m[3].x, m[3].y, m[3].z);
                node.scale = float32x3(math::length(xaxis), math::length(yaxis), math::length(zaxis));
                const float eps = 1.0e-8f;
                if (node.scale.x > eps) xaxis *= (1.0f / node.scale.x);
                else xaxis = float32x3(1.0f, 0.0f, 0.0f);
                if (node.scale.y > eps) yaxis *= (1.0f / node.scale.y);
                else yaxis = float32x3(0.0f, 1.0f, 0.0f);
                if (node.scale.z > eps) zaxis *= (1.0f / node.scale.z);
                else zaxis = float32x3(0.0f, 0.0f, 1.0f);
                if (math::dot(math::cross(xaxis, yaxis), zaxis) < 0.0f)
                {
                    node.scale.x = -node.scale.x;
                    xaxis = -xaxis;
                }
                const math::Quaternion q = math::normalize(math::Quaternion(math::Matrix3x3(xaxis, yaxis, zaxis)));
                node.rotation = float32x4(q.x, q.y, q.z, q.w);
                node.hasTRS = false;
            }

            modelIdToNode[id] = u32(nodes.size());
            nodes.push_back(std::move(node));
        }

        std::vector<bool> nodeHasParent(nodes.size(), false);
        for (const auto& c : reader.m_connections)
        {
            if (c.isProperty)
                continue;
            auto childIt = modelIdToNode.find(c.src);
            auto parentIt = modelIdToNode.find(c.dst);
            if (childIt == modelIdToNode.end() || parentIt == modelIdToNode.end())
                continue;
            nodes[parentIt->second].children.push_back(childIt->second);
            nodeHasParent[childIt->second] = true;
        }

        // ---- unified skin (glTF JOINTS_0 / WEIGHTS_0, max 4 influences) ----

        Skin skin;
        skin.name = "FBX.skin";
        std::unordered_map<u64, u16> boneIdToJoint;

        auto ensureJoint = [&](u64 boneId) -> u16
        {
            auto it = boneIdToJoint.find(boneId);
            if (it != boneIdToJoint.end())
                return it->second;
            auto nodeIt = modelIdToNode.find(boneId);
            if (nodeIt == modelIdToNode.end())
                return 0;
            const u16 jointIndex = u16(skin.joints.size());
            boneIdToJoint[boneId] = jointIndex;
            skin.joints.push_back(nodeIt->second);
            skin.inverseBindMatrices.push_back(matrix4x4(1.0f));
            return jointIndex;
        };

        // Prefer full LimbNode set so BVH / retarget has every bone; then cluster links.
        for (const auto& [id, model] : reader.m_models)
        {
            if (model.type == "LimbNode" || model.type == "Root")
                ensureJoint(id);
        }
        for (const auto& [clusterId, cluster] : reader.m_clusters)
        {
            MANGO_UNREFERENCED(cluster);
            const u64 boneId = reader.boneForCluster(clusterId);
            if (boneId && modelIdToNode.count(boneId))
                ensureJoint(boneId);
        }

        // Inverse bind: first cluster that links each bone wins.
        std::vector<bool> ibmSet(skin.joints.size(), false);
        for (const auto& [clusterId, cluster] : reader.m_clusters)
        {
            MANGO_UNREFERENCED(clusterId);
            if (!cluster.hasTransformLink)
                continue;
            const u64 boneId = reader.boneForCluster(clusterId);
            auto jt = boneIdToJoint.find(boneId);
            if (jt == boneIdToJoint.end())
                continue;
            const u16 jointIndex = jt->second;
            if (ibmSet[jointIndex])
                continue;

            matrix4x4 ibm = inverse(cluster.transformLink);
            if (cluster.hasTransform)
                ibm = ibm * cluster.transform;
            skin.inverseBindMatrices[jointIndex] = reader.toOursMatrix(ibm);
            ibmSet[jointIndex] = true;
        }

        // Skeleton root hint (glTF skin.skeleton).
        for (const auto& [id, model] : reader.m_models)
        {
            if (model.type == "Null" || model.type == "Root")
            {
                auto it = modelIdToNode.find(id);
                if (it != modelIdToNode.end())
                {
                    skin.skeleton = it->second;
                    break;
                }
            }
        }

        auto packInfluences = [](Vertex& vertex, std::vector<std::pair<u16, float>> list)
        {
            vertex.joint[0] = vertex.joint[1] = vertex.joint[2] = vertex.joint[3] = 0;
            vertex.weight = float32x4(0.0f, 0.0f, 0.0f, 0.0f);

            if (list.empty())
                return;

            std::sort(list.begin(), list.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });

            if (list.size() > 4)
                list.resize(4);

            float sum = 0.0f;
            for (const auto& iw : list)
                sum += iw.second;
            if (sum <= 0.0f)
                return;
            const float inv = 1.0f / sum;

            for (size_t i = 0; i < list.size(); ++i)
            {
                vertex.joint[i] = list[i].first;
                vertex.weight[i] = list[i].second * inv;
            }
        };

        const bool haveSkin = !skin.joints.empty() && !reader.m_clusters.empty();

        printLine(Print::Verbose, "[FBX] skeleton: {} nodes, skin joints: {}, clusters: {}",
            nodes.size(), skin.joints.size(), reader.m_clusters.size());

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
            bool hasNormalIndices = !current.normals.indices.empty();

            if (hasTexcoords)
            {
                trimesh.flags |= Vertex::Texcoord;
            }

            // Control-point influences for this geometry (FBX skin is per control point).
            const size_t positionCount = current.positions.values.size();
            std::vector<std::vector<std::pair<u16, float>>> cpInfluences(positionCount);
            bool geometrySkinned = false;

            if (haveSkin)
            {
                const u64 skinId = reader.skinForGeometry(current.id);
                if (skinId)
                {
                    for (u64 clusterId : reader.clustersForSkin(skinId))
                    {
                        const u64 boneId = reader.boneForCluster(clusterId);
                        auto jt = boneIdToJoint.find(boneId);
                        if (jt == boneIdToJoint.end())
                            continue;
                        auto cit = reader.m_clusters.find(clusterId);
                        if (cit == reader.m_clusters.end())
                            continue;
                        const ClusterFBX& cluster = cit->second;
                        const size_t n = std::min(cluster.indexes.size(), cluster.weights.size());
                        for (size_t i = 0; i < n; ++i)
                        {
                            const s32 cp = cluster.indexes[i];
                            const float w = cluster.weights[i];
                            if (cp < 0 || size_t(cp) >= positionCount || w <= 0.0f)
                                continue;
                            auto& list = cpInfluences[size_t(cp)];
                            bool merged = false;
                            for (auto& iw : list)
                            {
                                if (iw.first == jt->second)
                                {
                                    iw.second += w;
                                    merged = true;
                                    break;
                                }
                            }
                            if (!merged)
                                list.push_back({ jt->second, w });
                            geometrySkinned = true;
                        }
                    }
                }
            }

            if (geometrySkinned)
                trimesh.flags |= Vertex::Joints | Vertex::Weights;

            const char* mappingName = "ByPolygonVertex";
            switch (current.normals.mappingType)
            {
                case ByVertice:       mappingName = "ByVertice"; break;
                case ByEdge:          mappingName = "ByEdge"; break;
                case AllSame:         mappingName = "AllSame"; break;
                case ByPolygon:       mappingName = "ByPolygon"; break;
                case ByPolygonVertex: default: break;
            }

            const char* uvMappingName = "ByPolygonVertex";
            switch (current.texcoords.mappingType)
            {
                case ByVertice:       uvMappingName = "ByVertice"; break;
                case ByEdge:          uvMappingName = "ByEdge"; break;
                case AllSame:         uvMappingName = "AllSame"; break;
                case ByPolygon:       uvMappingName = "ByPolygon"; break;
                case ByPolygonVertex: default: break;
            }

            // Materials linked to this geometry's parent Model (slot order = LayerElementMaterial indices).
            std::vector<u64> modelMaterials;
            if (!reader.m_materials.empty())
            {
                const u64 modelId = reader.modelForGeometry(current.id);
                modelMaterials = modelId ? reader.materialsForModel(modelId) : std::vector<u64>{};
                if (modelMaterials.empty())
                    modelMaterials.push_back(reader.m_materials.begin()->first);
            }

            auto resolveMaterialSlot = [&](size_t polygonIndex) -> u32
            {
                if (current.materials.values.empty())
                    return 0;

                size_t valueIndex = 0;
                switch (current.materials.mappingType)
                {
                    case ByPolygon:
                        valueIndex = polygonIndex;
                        break;
                    case ByPolygonVertex:
                        valueIndex = polygonIndex;
                        break;
                    case AllSame:
                    default:
                        valueIndex = 0;
                        break;
                }

                s32 slot = 0;
                if (current.materials.referenceType == IndexToDirect && !current.materials.indices.empty())
                {
                    if (valueIndex >= current.materials.indices.size())
                        return 0;
                    const s32 idx = current.materials.indices[valueIndex];
                    if (idx < 0 || size_t(idx) >= current.materials.values.size())
                        return 0;
                    slot = current.materials.values[size_t(idx)];
                }
                else
                {
                    if (valueIndex >= current.materials.values.size())
                        valueIndex = 0;
                    slot = current.materials.values[valueIndex];
                }

                if (slot < 0)
                    slot = 0;
                return u32(slot);
            };

            auto materialIndexForSlot = [&](u32 slot) -> u32
            {
                if (modelMaterials.empty())
                    return 0;
                if (slot >= modelMaterials.size())
                    slot = 0;
                auto it = materialIdToIndex.find(modelMaterials[slot]);
                return it != materialIdToIndex.end() ? it->second : 0;
            };

            const size_t normalCount = current.normals.values.size();
            const size_t texcoordCount = current.texcoords.values.size();
            const size_t normalIndexCount = current.normals.indices.size();
            const size_t texcoordIndexCount = current.texcoords.indices.size();

            auto toOurs = [&](const float32x3& v) {
                return reader.toOurs(v);
            };

            auto resolveNormal = [&](size_t corner, s32 posIndex, size_t polygonIndex) -> float32x3
            {
                if (!hasNormals || normalCount == 0)
                    return float32x3(0.0f, 1.0f, 0.0f);

                size_t valueIndex = 0;

                switch (current.normals.mappingType)
                {
                    case ByVertice:
                        valueIndex = size_t(posIndex);
                        break;

                    case ByPolygon:
                        valueIndex = polygonIndex;
                        break;

                    case AllSame:
                        valueIndex = 0;
                        break;

                    case ByEdge:
                        valueIndex = corner;
                        break;

                    case ByPolygonVertex:
                    default:
                        valueIndex = corner;
                        break;
                }

                if (current.normals.referenceType == IndexToDirect || hasNormalIndices)
                {
                    if (valueIndex >= normalIndexCount)
                        return float32x3(0.0f, 1.0f, 0.0f);
                    const s32 idx = current.normals.indices[valueIndex];
                    if (idx < 0 || size_t(idx) >= normalCount)
                        return float32x3(0.0f, 1.0f, 0.0f);
                    return toOurs(current.normals.values[size_t(idx)]);
                }

                if (valueIndex >= normalCount)
                    return float32x3(0.0f, 1.0f, 0.0f);
                return toOurs(current.normals.values[valueIndex]);
            };

            auto resolveTexcoord = [&](size_t corner, s32 posIndex) -> float32x2
            {
                if (!hasTexcoords || texcoordCount == 0)
                    return float32x2(0.0f, 0.0f);

                size_t valueIndex = corner;
                if (current.texcoords.mappingType == ByVertice)
                    valueIndex = size_t(posIndex);

                if (current.texcoords.referenceType == IndexToDirect || hasTexcoordIndices)
                {
                    if (valueIndex >= texcoordIndexCount)
                        return float32x2(0.0f, 0.0f);
                    const s32 idx = current.texcoords.indices[valueIndex];
                    if (idx < 0 || size_t(idx) >= texcoordCount)
                        return float32x2(0.0f, 0.0f);
                    return current.texcoords.values[size_t(idx)];
                }

                if (valueIndex >= texcoordCount)
                    return float32x2(0.0f, 0.0f);
                return current.texcoords.values[valueIndex];
            };

            std::unordered_map<u32, Mesh> meshesByMaterial;

            s32 cornerPos[3] {};
            int corners = 0;
            size_t polygonIndex = 0;
            Triangle triangle;
            u32 polygonMaterial = materialIndexForSlot(resolveMaterialSlot(0));

            for (size_t i = 0; i < current.positions.indices.size(); ++i)
            {
                const s32 raw = current.positions.indices[i];
                const bool endOfPolygon = raw < 0;
                const s32 posIndex = endOfPolygon ? -(raw + 1) : raw;

                if (posIndex < 0 || size_t(posIndex) >= positionCount)
                {
                    corners = 0;
                    if (endOfPolygon)
                        ++polygonIndex;
                    continue;
                }

                if (corners == 0)
                    polygonMaterial = materialIndexForSlot(resolveMaterialSlot(polygonIndex));

                cornerPos[corners] = posIndex;
                triangle.vertex[corners].normal = resolveNormal(i, posIndex, polygonIndex);
                if (hasTexcoords)
                    triangle.vertex[corners].texcoord = resolveTexcoord(i, posIndex);
                if (geometrySkinned)
                    packInfluences(triangle.vertex[corners], cpInfluences[size_t(posIndex)]);

                ++corners;

                if (corners == 3)
                {
                    triangle.vertex[0].position = toOurs(current.positions.values[cornerPos[0]]);
                    triangle.vertex[1].position = toOurs(current.positions.values[cornerPos[1]]);
                    triangle.vertex[2].position = toOurs(current.positions.values[cornerPos[2]]);

                    if (!hasNormals)
                    {
                        const float32x3& p0 = triangle.vertex[0].position;
                        const float32x3& p1 = triangle.vertex[1].position;
                        const float32x3& p2 = triangle.vertex[2].position;
                        const float32x3 normal = normalize(cross(p0 - p2, p0 - p1));

                        triangle.vertex[0].normal = normal;
                        triangle.vertex[1].normal = normal;
                        triangle.vertex[2].normal = normal;
                    }

                    Mesh& bucket = meshesByMaterial[polygonMaterial];
                    if (bucket.triangles.empty())
                        bucket.flags = trimesh.flags;
                    bucket.triangles.push_back(triangle);

                    triangle.vertex[1] = triangle.vertex[2];
                    cornerPos[1] = cornerPos[2];
                    corners = 2;
                }

                if (endOfPolygon)
                {
                    corners = 0;
                    ++polygonIndex;
                }
            }

            size_t totalTriangles = 0;
            for (auto& [materialIndex, bucket] : meshesByMaterial)
            {
                totalTriangles += bucket.triangles.size();

                if (hasNormals && hasTexcoords &&
                    materialIndex < materials.size() && materials[materialIndex].normal)
                {
                    bucket.computeTangents();
                }

                mesh.append(bucket, materialIndex);
            }

            printLine(Print::Verbose, "  [FBX] primitives: {} ({} triangles)",
                meshesByMaterial.size(), totalTriangles);
        }

        meshes.push_back(std::move(ptr));

        if (haveSkin)
        {
            const u32 skinIndex = u32(skins.size());
            skins.push_back(std::move(skin));

            // Mesh stays in bind-pose space (identity node). Skin joints live elsewhere.
            Node meshNode;
            meshNode.name = "FBX.mesh";
            meshNode.transform = matrix4x4(1.0f);
            meshNode.mesh = 0;
            meshNode.skin = skinIndex;
            roots.push_back(u32(nodes.size()));
            nodes.push_back(std::move(meshNode));
            nodeHasParent.push_back(false);
        }
        else if (!nodes.empty())
        {
            // Static FBX: put mesh on first root model.
            for (size_t i = 0; i < nodes.size(); ++i)
            {
                if (!nodeHasParent[i])
                {
                    nodes[i].mesh = 0;
                    break;
                }
            }
        }
        else
        {
            Node node;
            node.name = "FBX.object";
            node.transform = matrix4x4(1.0f);
            node.mesh = 0;
            nodes.push_back(node);
            nodeHasParent.push_back(false);
        }

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (!nodeHasParent[i])
                roots.push_back(u32(i));
        }
        // Deduplicate roots (mesh node may already be listed).
        {
            std::vector<u32> unique;
            std::vector<bool> seen(nodes.size(), false);
            for (u32 r : roots)
            {
                if (r < nodes.size() && !seen[r])
                {
                    seen[r] = true;
                    unique.push_back(r);
                }
            }
            roots = std::move(unique);
        }
        if (roots.empty() && !nodes.empty())
            roots.push_back(0);
    }

} // namespace mango::import3d
