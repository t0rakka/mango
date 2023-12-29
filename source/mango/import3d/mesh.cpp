/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/import3d/mesh.hpp>
#include "../../external/mikktspace/mikktspace.h"

namespace
{
    using namespace mango;
    using namespace mango::import3d;

    int callback_getNumFaces(const SMikkTSpaceContext* pContext)
    {
        Mesh& mesh = *reinterpret_cast<Mesh*>(pContext->m_pUserData);
        return int(mesh.triangles.size());
    }

    int callback_getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
    {
        MANGO_UNREFERENCED(pContext);
        MANGO_UNREFERENCED(iFace);
        return 3;
    }

    void callback_getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
    {
        Mesh& mesh = *reinterpret_cast<Mesh*>(pContext->m_pUserData);
        Vertex& vertex = mesh.triangles[iFace].vertex[iVert];
        fvPosOut[0] = vertex.position[0];
        fvPosOut[1] = vertex.position[1];
        fvPosOut[2] = vertex.position[2];
    }

    void callback_getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
    {
        Mesh& mesh = *reinterpret_cast<Mesh*>(pContext->m_pUserData);
        Vertex& vertex = mesh.triangles[iFace].vertex[iVert];
        fvNormOut[0] = vertex.normal[0];
        fvNormOut[1] = vertex.normal[1];
        fvNormOut[2] = vertex.normal[2];
    }

    void callback_getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
    {
        Mesh& mesh = *reinterpret_cast<Mesh*>(pContext->m_pUserData);
        Vertex& vertex = mesh.triangles[iFace].vertex[iVert];
        fvTexcOut[0] = vertex.texcoord[0];
        fvTexcOut[1] = vertex.texcoord[1];
    }

    void callback_setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
    {
        Mesh& mesh = *reinterpret_cast<Mesh*>(pContext->m_pUserData);
        Vertex& vertex = mesh.triangles[iFace].vertex[iVert];
        vertex.tangent = float32x4(fvTangent[0], fvTangent[1], fvTangent[2], fSign);
    }

    void callback_setTSpace(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fvBiTangent[], const float fMagS, const float fMagT,
                            const tbool bIsOrientationPreserving, const int iFace, const int iVert)
    {
        MANGO_UNREFERENCED(pContext);
        MANGO_UNREFERENCED(fvTangent);
        MANGO_UNREFERENCED(fvBiTangent);
        MANGO_UNREFERENCED(fMagS);
        MANGO_UNREFERENCED(fMagT);
        MANGO_UNREFERENCED(bIsOrientationPreserving);
        MANGO_UNREFERENCED(iFace);
        MANGO_UNREFERENCED(iVert);
    }

} // namespace

namespace mango::import3d
{

static
constexpr float pi2 = float(math::pi * 2.0);

void computeTangents(Mesh& mesh)
{
    SMikkTSpaceInterface mik_interface;

    mik_interface.m_getNumFaces = callback_getNumFaces;
    mik_interface.m_getNumVerticesOfFace = callback_getNumVerticesOfFace;
    mik_interface.m_getPosition = callback_getPosition;
    mik_interface.m_getNormal = callback_getNormal;
    mik_interface.m_getTexCoord = callback_getTexCoord;
    mik_interface.m_setTSpaceBasic = callback_setTSpaceBasic;
    mik_interface.m_setTSpace = callback_setTSpace;

    SMikkTSpaceContext mik_context;

    mik_context.m_pInterface = &mik_interface;
    mik_context.m_pUserData = &mesh;

    tbool status = genTangSpaceDefault(&mik_context);
    MANGO_UNREFERENCED(status);
}

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

    const float32x3 normal0( 1.0f, 0.0f, 0.0f);
    const float32x3 normal1(-1.0f, 0.0f, 0.0f);
    const float32x3 normal2( 0.0f, 1.0f, 0.0f);
    const float32x3 normal3( 0.0f,-1.0f, 0.0f);
    const float32x3 normal4( 0.0f, 0.0f, 1.0f);
    const float32x3 normal5( 0.0f, 0.0f,-1.0f);

    const float32x4 tangent0( 0.0f, 0.0f, 1.0f, 1.0f);
    const float32x4 tangent1( 0.0f, 0.0f,-1.0f, 1.0f);
    const float32x4 tangent2( 1.0f, 0.0f, 0.0f, 1.0f);
    const float32x4 tangent3(-1.0f, 0.0f, 0.0f, 1.0f);
    const float32x4 tangent4(-1.0f, 0.0f, 0.0f, 1.0f);
    const float32x4 tangent5( 1.0f, 0.0f, 0.0f, 1.0f);

    const float32x2 texcoord0(0.0f, 1.0f);
    const float32x2 texcoord1(0.0f, 0.0f);
    const float32x2 texcoord2(1.0f, 0.0f);
    const float32x2 texcoord3(1.0f, 1.0f);

    vertices =
    {
        // right (+x)
        { position1, normal0, tangent0, texcoord0 },
        { position3, normal0, tangent0, texcoord1 },
        { position7, normal0, tangent0, texcoord2 },
        { position5, normal0, tangent0, texcoord3 },

        // left (-x)
        { position4, normal1, tangent1, texcoord0 },
        { position6, normal1, tangent1, texcoord1 },
        { position2, normal1, tangent1, texcoord2 },
        { position0, normal1, tangent1, texcoord3 },

        // top (+y)
        { position2, normal2, tangent2, texcoord0 },
        { position6, normal2, tangent2, texcoord1 },
        { position7, normal2, tangent2, texcoord2 },
        { position3, normal2, tangent2, texcoord3 },

        // bottom (-y)
        { position4, normal3, tangent3, texcoord2 },
        { position0, normal3, tangent3, texcoord3 },
        { position1, normal3, tangent3, texcoord0 },
        { position5, normal3, tangent3, texcoord1 },

        // front (+z)
        { position5, normal4, tangent4, texcoord0 },
        { position7, normal4, tangent4, texcoord1 },
        { position6, normal4, tangent4, texcoord2 },
        { position4, normal4, tangent4, texcoord3 },

        // back (-z)
        { position0, normal5, tangent5, texcoord0 },
        { position2, normal5, tangent5, texcoord1 },
        { position3, normal5, tangent5, texcoord2 },
        { position1, normal5, tangent5, texcoord3 },
    };

    indices =
    {
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
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
