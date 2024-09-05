/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>
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
