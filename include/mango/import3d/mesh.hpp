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
    // material
    // -----------------------------------------------------------------------

    using Texture = std::shared_ptr<image::Bitmap>;

    struct Material
    {
        enum class AlphaMode
        {
            ALPHA_OPAQUE,
            ALPHA_MASK,
            ALPHA_BLEND
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

        AlphaMode alphaMode { AlphaMode::ALPHA_OPAQUE };
        float alphaCutoff { 0.5f };
        bool twosided { false };
    };

    // -----------------------------------------------------------------------
    // TriangleMesh
    // -----------------------------------------------------------------------

    struct Vertex
    {
        enum : u32
        {
            POSITION  = 0x0001,
            NORMAL    = 0x0002,
            TANGENT   = 0x0004,
            TEXCOORD  = 0x0008,
            COLOR     = 0x0010,
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

    struct TriangleMesh
    {
        std::vector<Triangle> triangles;
        u32 material = 0;
        u32 flags = 0;

        void computeTangents();
    };

    // -----------------------------------------------------------------------
    // IndexedMesh
    // -----------------------------------------------------------------------

    struct IndexedPrimitive
    {
        u32 start = 0;
        u32 count = 0;
        u32 material = 0;

        IndexedPrimitive() = default;

        IndexedPrimitive(u32 start, u32 count)
            : start(start)
            , count(count)
        {
        }

        IndexedPrimitive(u32 start, u32 count, u32 material)
            : start(start)
            , count(count)
            , material(material)
        {
        }
    };

    struct IndexedMesh
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        std::vector<IndexedPrimitive> primitives;
        math::Box boundingBox;
        u32 flags = 0;

        IndexedMesh();
        IndexedMesh(const TriangleMesh& trimesh);
        IndexedMesh(const std::vector<TriangleMesh>& trimeshes);
    };

    // -----------------------------------------------------------------------
    // Mesh
    // -----------------------------------------------------------------------

    struct Primitive
    {
        enum Mode
        {
            TRIANGLE_LIST,
            TRIANGLE_STRIP,
            TRIANGLE_FAN,
        };

        Mode mode = TRIANGLE_LIST;
        u32 start = 0;
        u32 count = 0;
        u32 base = 0;
        u32 material = 0;
    };

    struct VertexAttribute
    {
        enum Type
        {
            NONE,
            INT8,
            INT16,
            INT32,
            UINT8,
            UINT16,
            UINT32,
            FLOAT16,
            FLOAT32,
        };

        Type type = NONE;
        u32 components = 0;
        u32 bytes = 0;
        u32 stride = 0;
        size_t offset = 0;

        VertexAttribute();
        VertexAttribute(Type type, u32 components);
        VertexAttribute(Type type, u32 components, u32 stride, size_t offset);

        operator bool () const;
    };

    struct VertexBuffer : Buffer
    {
        template <typename T>
        T* address(const VertexAttribute& attribute, size_t index = 0) const
        {
            return reinterpret_cast<T*>(data() + attribute.offset + index * attribute.stride);
        }
    };

    struct IndexBuffer : Buffer
    {
        enum Type
        {
            NONE,
            UINT8,
            UINT16,
            UINT32,
        };

        Type type = NONE;
    };

    struct Mesh
    {
        VertexBuffer vertices;

        VertexAttribute position;
        VertexAttribute normal;
        VertexAttribute tangent;
        VertexAttribute texcoord;
        VertexAttribute color;
        VertexAttribute joint;
        VertexAttribute weight;

        IndexBuffer indices;

        std::vector<Primitive> primitives;

        math::Box boundingBox;

        Mesh();
        Mesh(const IndexedMesh& mesh);
    };

    // -----------------------------------------------------------------------
    // scene
    // -----------------------------------------------------------------------

    struct Node
    {
        static constexpr u32 NONE = 0xffffffff;

        std::string name;
        std::vector<u32> children;
        matrix4x4 transform { 1.0f };
        u32 mesh = NONE;
    };

    struct Scene
    {
        std::vector<Material> materials;
        std::vector<std::unique_ptr<Mesh>> meshes;
        std::vector<Node> nodes;
        std::vector<u32> roots;
    };

    // -----------------------------------------------------------------------
    // utilities
    // -----------------------------------------------------------------------

#if 0

    void computeTangents(Mesh& mesh);
    Mesh convertMesh(const IndexedMesh& input);
    IndexedMesh convertMesh(const Mesh& input);

#endif

    Texture createTexture(const filesystem::Path& path, const std::string& filename);
    Texture createTexture(ConstMemory memory);

    // -----------------------------------------------------------------------
    // meshes
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

    std::unique_ptr<Mesh> createCube(float32x3 size);
    std::unique_ptr<Mesh> createIcosahedron(float radius);
    std::unique_ptr<Mesh> createDodecahedron(float radius);
    std::unique_ptr<Mesh> createTorus(TorusParameters params);
    std::unique_ptr<Mesh> createTorusknot(TorusknotParameters params);

} // namespace mango::import3d
