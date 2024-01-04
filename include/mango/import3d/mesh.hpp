/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <mango/math/math.hpp>
#include <mango/image/image.hpp>

namespace mango::import3d
{

    using float32x2 = math::Vector<float, 2>;
    using float32x3 = math::Vector<float, 3>;
    using float32x4 = math::Vector<float, 4>;
    using matrix4x4 = math::Matrix<float, 4, 4>;
    using color8x4 = math::Vector<u8, 4>;

    // -----------------------------------------------------------------------
    // material
    // -----------------------------------------------------------------------

    using Texture = std::shared_ptr<image::Bitmap>;

    enum class AlphaMode
    {
        OPAQUE,
        MASK,
        BLEND
    };

    struct Material
    {
        float roughnessFactor { 1.0f };
        float metallicFactor { 1.0f };
        float32x4 baseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        float32x3 emissiveFactor { 0.0f, 0.0f, 0.0f };

        Texture metallicRoughnessTexture;
        Texture baseColorTexture;
        Texture emissiveTexture;
        Texture normalTexture;
        Texture occlusionTexture;

        AlphaMode alphaMode { AlphaMode::OPAQUE };
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
    };

    struct Triangle
    {
        Vertex vertex[3];
        //u32 material;
    };

    struct Mesh
    {
        std::vector<Triangle> triangles;
    };

    // -----------------------------------------------------------------------
    // indexed mesh
    // -----------------------------------------------------------------------

#if 0

    // TODO: convert everythin to use this layout:
    //
    // - requires supporting material index in Mesh
    // - add flags which vertex components are present
    // - use separate array for each vertex component
    // - support 16 and 32 bit indices

    struct Primitive
    {
        enum class Mode
        {
            TRIANGLE_LIST,
            TRIANGLE_STRIP,
            TRIANGLE_FAN,
        };

        std::vector<u32> indices;
        Mode mode { Mode::TRIANGLE_LIST };
        u32 material;
    };

    struct IndexedMesh
    {
        std::vector<Vertex> vertices;
        std::vector<Primitive> primitives;
    };

#else

    enum class PrimitiveMode
    {
        TRIANGLE_LIST,
        TRIANGLE_STRIP,
        TRIANGLE_FAN,
    };

    struct IndexedMesh
    {
        std::vector<Vertex> vertices;

        std::vector<u32> indices;
        PrimitiveMode mode { PrimitiveMode::TRIANGLE_LIST };
    };

#endif

    // -----------------------------------------------------------------------
    // scene
    // -----------------------------------------------------------------------

    struct Object
    {
        static constexpr u32 DefaultParent = 0xffffffff;

        std::string name;
        u32 parent { DefaultParent };
        matrix4x4 transform { 1.0f };
        std::vector<u32> meshes;
    };

    struct Scene
    {
        std::vector<Mesh> meshes;
        std::vector<Object> objects;
    };

    // -----------------------------------------------------------------------
    // utilities
    // -----------------------------------------------------------------------

    void computeTangents(Mesh& mesh);

    Mesh convertMesh(const IndexedMesh& input);
    IndexedMesh convertMesh(const Mesh& input);

    // -----------------------------------------------------------------------
    // shapes
    // -----------------------------------------------------------------------

    // TODO: more shapes (sphere, icosahedron, etc.)

    struct Cube : IndexedMesh
    {
        // TODO: float32x3 as size
        Cube(float size);
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
