/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <memory>
#include <algorithm>
#include <utility>
#include <unordered_map>
#include <mango/core/buffer.hpp>
#include <mango/math/math.hpp>
#include <mango/image/image.hpp>
#include <mango/filesystem/filesystem.hpp>
#include <mango/import3d/animation.hpp>

namespace mango::import3d
{

    // -----------------------------------------------------------------------
    // Canonical mesh / world / view space (importers + shape generators)
    //
    //   Handedness: right-handed (right × up = forward)
    //   Axes:       +X right, +Y up, +Z ahead (Unity-like labels)
    //   Winding:    clockwise when viewed from outside = front face
    //               OpenGL: GL_CW. Vulkan after a Y-up→Y-down last-mile
    //               (e.g. scale(1,-1,-1)): use VK_FRONT_FACE_COUNTER_CLOCKWISE
    //               — FB signed area flips with Y; mesh data stays CW.
    //   Normals:    outward
    //   Tangents:   .xyz = tangent, .w = bitangent sign for TBN
    //   Texcoords:  (0,0) = top-left; V increases downward
    //
    // -----------------------------------------------------------------------

    using float32x2 = math::Vector<float, 2>;
    using float32x3 = math::Vector<float, 3>;
    using float32x4 = math::Vector<float, 4>;
    using matrix4x4 = math::Matrix<float, 4, 4>;

    // -----------------------------------------------------------------------
    // images (deferred decode — engine loads / packs / uploads)
    //
    // Importers record *how* to get pixel data. They do not decode bitmaps.
    //
    // External file:
    //   filename relative to SceneResources::path; open File when decoding.
    //   Decode jobs may capture resources->path (refcounted Mapper).
    //
    // Embedded (GLB buffer view, FBX Video, …):
    //   ConstMemory is a non-owning view into SceneResources::files.
    //   Capture std::shared_ptr<SceneResources> (or individual File) on decode jobs.
    //
    // Empty: filename.empty() && !memory.address (ConstMemory{nullptr,0}).
    // Format id is a mango extension (".jpg", ".png", …), not a MIME type.
    // -----------------------------------------------------------------------

    // glTF/etc. speak MIME; mango image decode speaks extensions.
    // Pass-through if `mime` already looks like an extension (leading '.').
    inline std::string mimeToExtension(std::string_view mime)
    {
        if (mime.empty())
            return {};

        if (mime.front() == '.')
            return std::string(mime);

        // Common glTF / FBX image MIME types
        if (mime == "image/jpeg" || mime == "image/jpg")
            return ".jpg";
        if (mime == "image/png")
            return ".png";
        if (mime == "image/webp")
            return ".webp";
        if (mime == "image/bmp" || mime == "image/x-ms-bmp")
            return ".bmp";
        if (mime == "image/gif")
            return ".gif";
        if (mime == "image/tiff" || mime == "image/tif")
            return ".tiff";
        if (mime == "image/ktx" || mime == "image/ktx2")
            return ".ktx2";
        if (mime == "image/vnd-ms.dds" || mime == "image/dds")
            return ".dds";
        if (mime == "image/x-exr" || mime == "image/exr")
            return ".exr";
        if (mime == "image/x-hdr" || mime == "image/vnd.radiance")
            return ".hdr";

        return {};
    }

    enum class ImageColorSpace : u8
    {
        Auto,   // engine picks from usage (albedo/emissive → sRGB, data → linear)
        sRGB,
        Linear,
    };

    enum class ImageChannel : u8
    {
        Red = 0,
        Green = 1,
        Blue = 2,
        Alpha = 3,
        Zero = 4, // constant 0
        One = 5,  // constant 1
    };

    // Map source RGBA → sample RGBA. Semantics then read what they need
    // (baseColor: rgba, metallic/roughness/occlusion/opacity: .r, normal/emissive: .rgb).
    struct ImageSwizzle
    {
        ImageChannel x { ImageChannel::Red };
        ImageChannel y { ImageChannel::Green };
        ImageChannel z { ImageChannel::Blue };
        ImageChannel w { ImageChannel::Alpha };

        static ImageSwizzle rgba() { return {}; }
        static ImageSwizzle rgb1()
        {
            return { ImageChannel::Red, ImageChannel::Green, ImageChannel::Blue, ImageChannel::One };
        }
        // Grayscale file → replicate R (also fine if decoder already expanded L→RGB).
        static ImageSwizzle rrr1()
        {
            return { ImageChannel::Red, ImageChannel::Red, ImageChannel::Red, ImageChannel::One };
        }
        static ImageSwizzle r()
        {
            return { ImageChannel::Red, ImageChannel::Zero, ImageChannel::Zero, ImageChannel::One };
        }
        static ImageSwizzle g()
        {
            return { ImageChannel::Green, ImageChannel::Zero, ImageChannel::Zero, ImageChannel::One };
        }
        static ImageSwizzle b()
        {
            return { ImageChannel::Blue, ImageChannel::Zero, ImageChannel::Zero, ImageChannel::One };
        }
        static ImageSwizzle a()
        {
            return { ImageChannel::Alpha, ImageChannel::Zero, ImageChannel::Zero, ImageChannel::One };
        }
    };

    struct ImageSource
    {
        std::string filename;   // relative to SceneResources::path; empty if embed-only
        ConstMemory memory {};  // non-owning embed view; {nullptr,0} if file-only / empty
        std::string extension;  // mango format id: ".jpg", ".png", …
        std::string name;       // optional debug label

        bool empty() const
        {
            return filename.empty() && !memory.address;
        }

        bool isFile() const
        {
            return !filename.empty();
        }

        bool isMemory() const
        {
            return memory.address != nullptr && memory.size > 0;
        }

        // extension: mango id; if empty, derived from filename.
        // Prefer mimeToExtension() at call sites that only have a MIME string.
        static ImageSource fromFile(std::string file, std::string extension = {}, std::string debugName = {})
        {
            ImageSource src;
            src.filename = std::move(file);
            src.extension = extension.empty()
                ? std::string(filesystem::getExtension(src.filename))
                : std::move(extension);
            src.name = std::move(debugName);
            return src;
        }

        // Non-owning: mem must outlive decode (typically Scene / Import* buffers).
        static ImageSource fromMemory(ConstMemory mem, std::string extension, std::string debugName = {})
        {
            ImageSource src;
            src.memory = mem;
            src.extension = std::move(extension);
            src.name = std::move(debugName);
            return src;
        }
    };

    struct UvTransform
    {
        float32x2 scale { 1.0f, 1.0f };
        float32x2 offset { 0.0f, 0.0f };
        float rotation { 0.0f }; // radians, counter-clockwise
    };

    // One semantic binding: which image, which channels, UV, color space.
    struct ImageSample
    {
        static constexpr u32 none = ~0u;

        u32 image = none; // index into Scene::images
        u32 texCoord = 0;
        ImageSwizzle swizzle {};
        ImageColorSpace colorSpace { ImageColorSpace::Auto };
        float scale { 1.0f }; // normal scale / occlusion strength
        UvTransform transform {};

        bool enabled() const { return image != none; }
        explicit operator bool() const { return enabled(); }

        static ImageSample from(u32 imageIndex, ImageSwizzle sw = ImageSwizzle::rgba(),
                                ImageColorSpace cs = ImageColorSpace::Auto)
        {
            ImageSample s;
            s.image = imageIndex;
            s.swizzle = sw;
            s.colorSpace = cs;
            return s;
        }
    };

    // Optional decode helpers for tools / tests (importers do not use these).
    using Texture = std::shared_ptr<image::Bitmap>;
    Texture createTexture(const filesystem::Path& path, const std::string& filename);
    Texture createTexture(ConstMemory memory);

    // -----------------------------------------------------------------------
    // material
    // -----------------------------------------------------------------------

    struct Material
    {
        enum class AlphaMode
        {
            Opaque,
            Mask,
            Blend
        };

        std::string name;

        float roughnessFactor { 1.0f };
        float metallicFactor { 1.0f };
        float32x4 baseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        float32x3 emissiveFactor { 0.0f, 0.0f, 0.0f };

        // Semantic samples — engine packs into GPU textures (e.g. glTF ORM).
        ImageSample baseColor;   // rgba, typically sRGB
        ImageSample metallic;    // .r after swizzle (glTF MR: Blue)
        ImageSample roughness;   // .r after swizzle (glTF MR: Green)
        ImageSample emissive;    // rgb, typically sRGB
        ImageSample normal;      // rgb, linear
        ImageSample occlusion;   // .r (glTF often Red of ORM / dedicated map)
        ImageSample opacity;     // .r — engine may pack into baseColor.a

        // KHR_materials_clearcoat (0 = disabled)
        float clearcoatFactor { 0.0f };
        float clearcoatRoughnessFactor { 0.0f };
        ImageSample clearcoat;
        ImageSample clearcoatRoughness;
        ImageSample clearcoatNormal;

        // KHR_materials_sheen (sheenColorFactor ~0 = disabled)
        float32x3 sheenColorFactor { 0.0f, 0.0f, 0.0f };
        float sheenRoughnessFactor { 0.0f };
        ImageSample sheenColor;
        ImageSample sheenRoughness;

        // KHR_materials_anisotropy (anisotropyStrength ~0 = disabled)
        float anisotropyStrength { 0.0f };
        float anisotropyRotation { 0.0f }; // radians (glTF); Exocortex OBJ anisor is [0,1]
        ImageSample anisotropy;

        AlphaMode alphaMode { AlphaMode::Opaque };
        float alphaCutoff { 0.5f };
        bool twosided { false };
    };

    // -----------------------------------------------------------------------
    // Mesh
    // -----------------------------------------------------------------------

    struct Vertex
    {
        enum : u32
        {
            Position  = 0x0001,
            Normal    = 0x0002,
            Texcoord  = 0x0004,
            Tangent   = 0x0008,
            Color     = 0x0010,
            // glTF JOINTS_0 / WEIGHTS_0 (set 0, up to 4 influences).
            Joints    = 0x0020,
            Weights   = 0x0040,
        };

        float32x3 position { 0.0f, 0.0f, 0.0f };
        float32x3 normal   { 0.0f, 0.0f, 0.0f };
        float32x2 texcoord { 0.0f, 0.0f };
        float32x4 tangent  { 0.0f, 0.0f, 0.0f, 0.0f };
        float32x4 color    { 0.0f, 0.0f, 0.0f, 0.0f };

        // Indices into Skin::joints (not node indices). Unused slots = 0 with weight 0.
        u16 joint[4] { 0, 0, 0, 0 };
        float32x4 weight { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct Triangle
    {
        Vertex vertex[3];
    };

    struct Mesh
    {
        std::vector<Triangle> triangles;
        u32 flags = 0;

        void computeTangents();
    };

    // -----------------------------------------------------------------------
    // IndexedMesh
    // -----------------------------------------------------------------------

    struct Primitive
    {
        enum class Type
        {
            TriangleList,
            TriangleStrip,
            TriangleFan,
        };

        Type type = Type::TriangleList;
        u32 start = 0;
        u32 count = 0;
        u32 base = 0;
        u32 material = 0;
    };

    struct IndexedMesh
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        std::vector<Primitive> primitives;
        math::Box boundingBox;
        u32 flags = 0;

        IndexedMesh();
        IndexedMesh(const Mesh& mesh, u32 material);

        void append(const Mesh& mesh, u32 material);
    };

    // -----------------------------------------------------------------------
    // scene
    // -----------------------------------------------------------------------

    struct Node
    {
        std::string name;
        std::vector<u32> children;

        matrix4x4 transform { 1.0f }; // bind-pose local transform (authoritative)

        std::optional<u32> mesh;
        std::optional<u32> skin; // glTF: skin lives on the node that instances the mesh
    };

    struct SceneResources
    {
        filesystem::Path path { "./" };
        std::vector<std::shared_ptr<filesystem::File>> files;
    };

    struct Scene
    {
        Scene()
            : resources(std::make_shared<SceneResources>())
        {
        }

        explicit Scene(const filesystem::Path& scenePath)
            : resources(std::make_shared<SceneResources>())
        {
            resources->path = scenePath;
        }

        std::shared_ptr<SceneResources> resources;

        const filesystem::Path& path() const
        {
            return resources->path;
        }

        std::vector<ImageSource> images;
        std::vector<Material> materials;
        std::vector<std::unique_ptr<IndexedMesh>> meshes;
        std::vector<Skin> skins;
        std::vector<Animation> animations;
        std::vector<Node> nodes;
        std::vector<u32> roots;
    };

    // -----------------------------------------------------------------------
    // shapes
    // -----------------------------------------------------------------------

    struct TorusParameters
    {
        int innerSegments = 128;
        int outerSegments = 32;
        float innerRadius = 1.0f;
        float outerRadius = 0.2f;
    };

    struct TorusknotParameters
    {
        int steps = 256;            // Number of steps in the torus knot
        int facets = 16;            // Number of facets
        float scale = 1.0f;         // Scale of the knot
        float thickness = 0.124f;   // Thickness of the knot
        float clumps = 12.0f;       // Number of clumps in the knot
        float clumpOffset = 20.0f;  // Offset of the clump (in 0..2pi)
        float clumpScale = 0.4f;    // Scale of a clump
        float uscale = 4.0f;        // U coordinate scale
        float vscale = 128.0f;      // V coordinate scale
        float p = 2.0f;             // P parameter of the knot
        float q = 5.0f;             // Q parameter of the knot
    };

    struct QuadParameters
    {
        float32x3 origin { 0.0f, 0.0f, 0.0f };    // plane origin (center of quad)
        float32x3 direction { 0.0f, 1.0f, 0.0f }; // plane direction (normal)
        float size = 1.0f;                        // full edge length (area = size²)
        u32 subdivision = 1;                      // N×N cells (N >= 1)
    };

    std::unique_ptr<IndexedMesh> createCube(float32x3 size);
    std::unique_ptr<IndexedMesh> createIcosahedron(float radius);
    std::unique_ptr<IndexedMesh> createDodecahedron(float radius);
    std::unique_ptr<IndexedMesh> createTorus(TorusParameters params);
    std::unique_ptr<IndexedMesh> createTorusknot(TorusknotParameters params);
    std::unique_ptr<IndexedMesh> createSphere(float radius, int subdivisions);
    std::unique_ptr<IndexedMesh> createQuad(QuadParameters params);

} // namespace mango::import3d
