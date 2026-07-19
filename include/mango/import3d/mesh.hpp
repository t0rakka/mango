/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <algorithm>
#include <utility>
#include <mango/core/buffer.hpp>
#include <mango/math/math.hpp>
#include <mango/image/image.hpp>
#include <mango/filesystem/filesystem.hpp>

namespace mango::import3d
{
    using float32x2 = math::Vector<float, 2>;
    using float32x3 = math::Vector<float, 3>;
    using float32x4 = math::Vector<float, 4>;
    using matrix4x4 = math::Matrix<float, 4, 4>;

    // -----------------------------------------------------------------------
    // texture
    // -----------------------------------------------------------------------

    using Texture = std::shared_ptr<image::Bitmap>;

    Texture createTexture(const filesystem::Path& path, const std::string& filename);
    Texture createTexture(ConstMemory memory);

    // -----------------------------------------------------------------------
    // material
    // -----------------------------------------------------------------------

    struct UvTransform
    {
        float32x2 scale { 1.0f, 1.0f };
        float32x2 offset { 0.0f, 0.0f };
        float rotation { 0.0f }; // radians, counter-clockwise
    };

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

        Texture metallicRoughnessTexture;
        Texture baseColorTexture;
        Texture emissiveTexture;
        Texture normalTexture;
        Texture occlusionTexture;

        UvTransform baseColorTransform;
        UvTransform metallicRoughnessTransform;
        UvTransform emissiveTransform;
        UvTransform normalTransform;
        UvTransform occlusionTransform;

        // KHR_materials_clearcoat (0 = disabled)
        float clearcoatFactor { 0.0f };
        float clearcoatRoughnessFactor { 0.0f };
        Texture clearcoatTexture;
        Texture clearcoatRoughnessTexture;
        Texture clearcoatNormalTexture;
        UvTransform clearcoatTransform;
        UvTransform clearcoatRoughnessTransform;
        UvTransform clearcoatNormalTransform;

        // KHR_materials_sheen (sheenColorFactor ~0 = disabled)
        float32x3 sheenColorFactor { 0.0f, 0.0f, 0.0f };
        float sheenRoughnessFactor { 0.0f };
        Texture sheenColorTexture;
        Texture sheenRoughnessTexture;

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
        };

        float32x3 position { 0.0f, 0.0f, 0.0f };
        float32x3 normal   { 0.0f, 0.0f, 0.0f };
        float32x2 texcoord { 0.0f, 0.0f };
        float32x4 tangent  { 0.0f, 0.0f, 0.0f, 0.0f };
        float32x4 color    { 0.0f, 0.0f, 0.0f, 0.0f };
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
        matrix4x4 transform { 1.0f };
        std::optional<u32> mesh;
    };

    struct Scene
    {
        std::vector<Material> materials;
        std::vector<std::unique_ptr<IndexedMesh>> meshes;
        std::vector<Node> nodes;
        std::vector<u32> roots;
    };

    // -----------------------------------------------------------------------
    // Canonical mesh space (all importers + shape generators)
    //
    //   Axes:      +X right, +Y up, +Z toward viewer (right-handed, glTF axes)
    //   Winding:   clockwise when viewed from outside
    //   Normals:   outward
    //   Texcoords: (0,0) = top-left of the image (glTF); V increases downward
    //
    // Importers bake this on load; apps must not apply a post-import basis fix.
    // -----------------------------------------------------------------------

    inline void reverseTriangleWinding(Triangle& triangle)
    {
        std::swap(triangle.vertex[1], triangle.vertex[2]);
    }

    // Outward geometric normal for a CW-outside triangle (p0, p1, p2).
    // RH cross of CW verts points inward; use the opposite product.
    inline float32x3 faceNormalOutwardCW(const float32x3& p0, const float32x3& p1, const float32x3& p2)
    {
        return normalize(cross(p0 - p2, p0 - p1));
    }

    inline void reverseTriangleListIndices(u32* indices, size_t count)
    {
        for (size_t i = 0; i + 2 < count; i += 3)
        {
            std::swap(indices[i + 1], indices[i + 2]);
        }
    }

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

    std::unique_ptr<IndexedMesh> createCube(float32x3 size);
    std::unique_ptr<IndexedMesh> createIcosahedron(float radius);
    std::unique_ptr<IndexedMesh> createDodecahedron(float radius);
    std::unique_ptr<IndexedMesh> createTorus(TorusParameters params);
    std::unique_ptr<IndexedMesh> createTorusknot(TorusknotParameters params);

} // namespace mango::import3d
