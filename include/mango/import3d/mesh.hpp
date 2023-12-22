/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <mango/math/math.hpp>

namespace mango::import3d
{

    using float32x2 = math::Vector<float, 2>;
    using float32x3 = math::Vector<float, 3>;
    using float32x4 = math::Vector<float, 4>;
    using matrix4x4 = math::Matrix<float, 4, 4>;

    struct Vertex
    {
        float32x3 position { 0.0f, 0.0f, 0.0f };
        float32x3 normal   { 0.0f, 0.0f, 0.0f };
        float32x2 texcoord { 0.0f, 0.0f };
    };

    struct Triangle
    {
        Vertex vertex[3];
    };

    struct Mesh
    {
        std::vector<Triangle> triangles;
    };

    struct IndexedMesh
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
    };

    struct Cube : IndexedMesh
    {
        Cube(float size);
    };

    struct Torus : IndexedMesh
    {
        struct Parameters
        {
            int innerSegments;
            int outerSegments;
            float innerRadius;
            float outerRadius;
        };

        Torus(Parameters params);
    };

    struct Torusknot : IndexedMesh
    {
        struct Parameters
        {
            int steps;           // Number of steps in the torus knot
            int facets;          // Number of facets
            float scale;         // Scale of the knot
            float thickness;     // Thickness of the knot
            float clumps;        // Number of clumps in the knot
            float clumpOffset;   // Offset of the clump (in 0..2pi)
            float clumpScale;    // Scale of a clump
            float uscale;        // U coordinate scale
            float vscale;        // V coordinate scale
            float p;             // P parameter of the knot
            float q;             // Q parameter of the knot
        };

        Torusknot(Parameters params);
    };

} // namespace mango::import3d
