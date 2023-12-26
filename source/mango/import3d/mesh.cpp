/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/import3d/mesh.hpp>

namespace mango::import3d
{

static
constexpr float pi2 = float(math::pi * 2.0);

Cube::Cube(float size)
{
    float s0 = size *-0.5f;
    float s1 = size * 0.5f;

    const float32x3 position0(s0, s0, s0);
    const float32x3 position1(s1, s0, s0);
    const float32x3 position2(s0, s1, s0);
    const float32x3 position3(s1, s1, s0);
    const float32x3 position4(s0, s0, s1);
    const float32x3 position5(s1, s0, s1);
    const float32x3 position6(s0, s1, s1);
    const float32x3 position7(s1, s1, s1);

    const float32x3 normal0( 0.0f, 0.0f,-1.0f);
    const float32x3 normal1( 0.0f, 0.0f, 1.0f);
    const float32x3 normal2(-1.0f, 0.0f, 0.0f);
    const float32x3 normal3( 1.0f, 0.0f, 0.0f);
    const float32x3 normal4( 0.0f,-1.0f, 0.0f);
    const float32x3 normal5( 0.0f, 1.0f, 0.0f);

    const float32x4 tangent(0.0f, 0.0f, 0.0f, 1.0f);

    const float32x2 texcoord0(0.0f, 0.0f);
    const float32x2 texcoord1(1.0f, 0.0f);
    const float32x2 texcoord2(0.0f, 1.0f);
    const float32x2 texcoord3(1.0f, 1.0f);

    vertices =
    {
        // bottom
        { position3, normal4, tangent, texcoord0 },
        { position2, normal4, tangent, texcoord1 },
        { position6, normal4, tangent, texcoord3 },
        { position7, normal4, tangent, texcoord2 },

        // top
        { position5, normal5, tangent, texcoord0 },
        { position4, normal5, tangent, texcoord1 },
        { position0, normal5, tangent, texcoord3 },
        { position1, normal5, tangent, texcoord2 },

        // front
        { position7, normal0, tangent, texcoord0 },
        { position6, normal0, tangent, texcoord1 },
        { position4, normal0, tangent, texcoord3 },
        { position5, normal0, tangent, texcoord2 },

        // back
        { position1, normal1, tangent, texcoord0 },
        { position0, normal1, tangent, texcoord1 },
        { position2, normal1, tangent, texcoord3 },
        { position3, normal1, tangent, texcoord2 },

        // right
        { position6, normal3, tangent, texcoord0 },
        { position2, normal3, tangent, texcoord1 },
        { position0, normal3, tangent, texcoord3 },
        { position4, normal3, tangent, texcoord2 },

        // left
        { position3, normal2, tangent, texcoord0 },
        { position7, normal2, tangent, texcoord1 },
        { position5, normal2, tangent, texcoord3 },
        { position1, normal2, tangent, texcoord2 },
    };

    indices =
    {
         0,  2,  1,  0,  3,  2,
         4,  6,  5,  4,  7,  6,
         8, 10,  9,  8, 11, 10,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 22, 21, 20, 23, 22,
    };
}

Torus::Torus(Parameters params)
{
    const float is = pi2 / params.innerSegments;
    const float js = pi2 / params.outerSegments;

    const float uscale = 4.0f / float(params.innerSegments);
    const float vscale = 1.0f / float(params.outerSegments);

    for (int i = 0; i < params.innerSegments + 1; ++i)
    {
        for (int j = 0; j < params.outerSegments + 1; ++j)
        {
            const float icos = std::cos(i * is);
            const float isin = std::sin(i * is);
            const float jcos = std::cos(j * js);
            const float jsin = std::sin(j * js);

            Vertex vertex;

            vertex.position = float32x3(
                icos * (params.innerRadius + jcos * params.outerRadius),
                isin * (params.innerRadius + jcos * params.outerRadius),
                jsin * params.outerRadius);
            vertex.normal = normalize(float32x3(icos * jcos, isin * jcos, jsin));
            vertex.texcoord = float32x2(i * uscale, j * vscale);

            vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < params.innerSegments; ++i)
    {
        int ci = (i + 0) * (params.outerSegments + 1);
        int ni = (i + 1) * (params.outerSegments + 1);

        for (int j = 0; j < params.outerSegments; ++j)
        {
            int a = ci + j + 0;
            int b = ni + j + 0;
            int c = ci + j + 1;
            int d = ni + j + 1;

            indices.push_back(c);
            indices.push_back(b);
            indices.push_back(a);

            indices.push_back(c);
            indices.push_back(d);
            indices.push_back(b);
        }
    }
}

// Torus knot generation
// written by Jari Komppa aka Sol / Trauma
// Based on:
// http://www.blackpawn.com/texts/pqtorus/default.html

Torusknot::Torusknot(Parameters params)
{
    params.thickness *= params.scale;

    // generate indices

    std::vector<int> stripIndices((params.steps + 1) * params.facets * 2);

    for (int j = 0; j < params.facets; j++)
    {
        for (int i = 0; i < params.steps + 1; i++)
        {
            stripIndices[i * 2 + 0 + j * (params.steps + 1) * 2] = (j + 1 + i * (params.facets + 1));
            stripIndices[i * 2 + 1 + j * (params.steps + 1) * 2] = (j + 0 + i * (params.facets + 1));
        }
    }

    // convert triangle strip into triangles

    for (size_t i = 2; i < stripIndices.size(); ++i)
    {
        int s = i & 1; // swap triangle winding-order
        indices.push_back(stripIndices[i]);
        indices.push_back(stripIndices[i - 1 - s]);
        indices.push_back(stripIndices[i - 2 + s]);
    }

    // generate vertices

    vertices.resize((params.steps + 1) * (params.facets + 1) + 1);

    float32x3 centerpoint;
    float Pp = params.p * 0 * pi2 / params.steps;
    float Qp = params.q * 0 * pi2 / params.steps;
    float r = (.5f * (2 + std::sin(Qp))) * params.scale;
    centerpoint.x = r * std::cos(Pp);
    centerpoint.y = r * std::cos(Qp);
    centerpoint.z = r * std::sin(Pp);

    for (int i = 0; i < params.steps; i++)
    {
        float32x3 nextpoint;
        Pp = params.p * (i + 1) * pi2 / params.steps;
        Qp = params.q * (i + 1) * pi2 / params.steps;
        r = (.5f * (2 + std::sin(Qp))) * params.scale;
        nextpoint.x = r * std::cos(Pp);
        nextpoint.y = r * std::cos(Qp);
        nextpoint.z = r * std::sin(Pp);

        float32x3 T = nextpoint - centerpoint;
        float32x3 N = nextpoint + centerpoint;
        float32x3 B = cross(T, N);
        N = cross(B, T);
        B = normalize(B);
        N = normalize(N);

        for (int j = 0; j < params.facets; j++)
        {
            float pointx = std::sin(j * pi2 / params.facets) * params.thickness * ((std::sin(params.clumpOffset + params.clumps * i * pi2 / params.steps) * params.clumpScale) + 1);
            float pointy = std::cos(j * pi2 / params.facets) * params.thickness * ((std::cos(params.clumpOffset + params.clumps * i * pi2 / params.steps) * params.clumpScale) + 1);

            const int offset = i * (params.facets + 1) + j;
            vertices[offset].position = N * pointx + B * pointy + centerpoint;
            vertices[offset].normal = normalize(vertices[offset].position - centerpoint);
            vertices[offset].texcoord = float32x2((float(j) / params.facets) * params.uscale, 
                                                  (float(i) / params.steps) * params.vscale);
        }

        // create duplicate vertex for sideways wrapping
        // otherwise identical to first vertex in the 'ring' except for the U coordinate
        vertices[i * (params.facets + 1) + params.facets].position = vertices[i * (params.facets + 1)].position;
        vertices[i * (params.facets + 1) + params.facets].normal = vertices[i * (params.facets + 1)].normal;
        vertices[i * (params.facets + 1) + params.facets].texcoord.x = params.uscale;
        vertices[i * (params.facets + 1) + params.facets].texcoord.y = vertices[i * (params.facets + 1)].texcoord.y;
        
        centerpoint = nextpoint;
    }

    // create duplicate ring of vertices for longways wrapping
    // otherwise identical to first 'ring' in the knot except for the V coordinate
    for (int j = 0; j < params.facets; j++)
    {
        vertices[params.steps * (params.facets + 1) + j].position = vertices[j].position;
        vertices[params.steps * (params.facets + 1) + j].normal = vertices[j].normal;

        vertices[params.steps * (params.facets + 1) + j].texcoord.x = vertices[j].texcoord.x;
        vertices[params.steps * (params.facets + 1) + j].texcoord.y = params.vscale;
    }

    // finally, there's one vertex that needs to be duplicated due to both U and V coordinate.
    vertices[params.steps * (params.facets + 1) + params.facets].position = vertices[0].position;
    vertices[params.steps * (params.facets + 1) + params.facets].normal = vertices[0].normal;
    vertices[params.steps * (params.facets + 1) + params.facets].texcoord = float32x2(params.uscale, params.vscale);
}

} // namespace mango::import3d
