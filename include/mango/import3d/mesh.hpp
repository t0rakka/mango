/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>
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
    // mesh
    // -----------------------------------------------------------------------

    struct Vertex
    {
        float32x3 position { 0.0f, 0.0f, 0.0f };
        float32x3 normal   { 0.0f, 0.0f, 0.0f };
        float32x4 tangent  { 0.0f, 0.0f, 0.0f, 1.0f };
        float32x2 texcoord { 0.0f, 0.0f };
        float32x3 color    { 1.0f, 1.0f, 1.0f };

        /* TODO: these flags will be used to generate attribute arrays for IndexedMesh
        enum : u32
        {
            POSITION  = 0x00000001,
            NORMAL    = 0x00000002,
            TANGENT   = 0x00000004,
            TEXCOORD  = 0x00000008,
            COLOR     = 0x00000010,
            // TODO: JOINTS, WEIGHTS
        };
        */
    };

    struct Triangle
    {
        Vertex vertex[3];
        u32 material = 0;
    };

    struct Mesh
    {
        std::vector<Triangle> triangles;
    };

    // -----------------------------------------------------------------------
    // indexed mesh
    // -----------------------------------------------------------------------

    // TODO: vertices will eventually use nr. of attributes stored as separate arrays
    // TODO: the attribute arrays will support different types
    // TODO: primitives will have bounding volume (box + sphere)
    // TODO: indices can be 16 or 32 bit

    struct Primitive
    {
        enum class Mode
        {
            TRIANGLE_LIST,
            TRIANGLE_STRIP,
            TRIANGLE_FAN,
        };

        Mode mode = Mode::TRIANGLE_LIST;
        u32 start;
        u32 count;
        u32 base;
        u32 material = 0;
    };

    struct IndexedMesh
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        std::vector<Primitive> primitives;
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
        std::vector<IndexedMesh> meshes;
        std::vector<Node> nodes;
        std::vector<u32> roots;
    };

    // -----------------------------------------------------------------------
    // utilities
    // -----------------------------------------------------------------------

    void computeTangents(Mesh& mesh);
    Mesh convertMesh(const IndexedMesh& input);
    IndexedMesh convertMesh(const Mesh& input);

    Texture createTexture(const filesystem::Path& path, const std::string& filename);
    Texture createTexture(ConstMemory memory);

    // -----------------------------------------------------------------------
    // shapes
    // -----------------------------------------------------------------------

    struct Cube : IndexedMesh
    {
        Cube(float32x3 size);
    };

    struct Icosahedron : IndexedMesh
    {
        Icosahedron(float radius);
    };

    struct Dodecahedron : IndexedMesh
    {
        Dodecahedron(float radius);
    };

    struct Torus : IndexedMesh
    {
        struct Parameters
        {
            int innerSegments = 128;
            int outerSegments = 32;
            float innerRadius = 1.0f;
            float outerRadius = 0.2f;
        };

        Torus(Parameters params);
    };

    struct Torusknot : IndexedMesh
    {
        struct Parameters
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

        Torusknot(Parameters params);
    };

} // namespace mango::import3d
