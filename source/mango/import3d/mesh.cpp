/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/core.hpp>
#include <mango/import3d/mesh.hpp>
#include "../../external/mikktspace/mikktspace.h"

/*

This is a convenience API for reading various 3D object formats and providing the data in
unified layout for rendering, or processing and dumping into a file that custom engine can
read more efficiently. The intent is accessibility not performance.

*/

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

static inline
bool operator < (const Vertex& a, const Vertex& b)
{
    return std::memcmp(&a, &b, sizeof(Vertex)) < 0;
}

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

void loadTexture(Texture& texture, const filesystem::Path& path, const std::string& filename)
{
    /* NOTE:

    This is just a convenience function. What we really need is "TextureProvider" interface,
    which can return a memory mapped view of the texture file, or result of combining two textures
    into one when it is required. We might also want to use compressed textures and directly upload
    them into the GPU, or use compute decoder to decompress JPEG. DX12 also has DirectTexture API
    which could be supported when we don't always provide the data as Bitmap like we do here.

    A texture loading queue can cut the loading time into fraction but we want to keep this simple for now.

    */

    if (filename.empty())
    {
        return;
    }

    bool is_debug_enable = debugPrintIsEnable();
    debugPrintEnable(false);

    filesystem::File file(path, filename);

    image::Format format(32, image::Format::UNORM, image::Format::RGBA, 8, 8, 8, 8);
    texture = std::make_shared<image::Bitmap>(file, filename, format);

    debugPrintEnable(is_debug_enable);
}

void loadTexture(Texture& texture, ConstMemory memory)
{
    bool is_debug_enable = debugPrintIsEnable();
    debugPrintEnable(false);

    image::Format format(32, image::Format::UNORM, image::Format::RGBA, 8, 8, 8, 8);
    texture = std::make_shared<image::Bitmap>(memory, "", format);

    debugPrintEnable(is_debug_enable);
}

Mesh convertMesh(const IndexedMesh& input)
{
    Mesh output;

    for (const Primitive& primitive : input.primitives)
    {
        switch (primitive.mode)
        {
            case Primitive::Mode::TRIANGLE_LIST:
            {
                Triangle triangle;
                triangle.material = primitive.material;

                const size_t start = primitive.start;
                const size_t end = start + primitive.count;

                for (size_t i = start; i < end; i += 3)
                {
                    triangle.vertex[0] = input.vertices[input.indices[i + 0]];
                    triangle.vertex[1] = input.vertices[input.indices[i + 1]];
                    triangle.vertex[2] = input.vertices[input.indices[i + 2]];

                    output.triangles.push_back(triangle);
                }

                break;
            }

            case Primitive::Mode::TRIANGLE_STRIP:
            {
                Triangle triangle;
                triangle.material = primitive.material;

                const size_t start = primitive.start;
                const size_t end = start + primitive.count;

                Vertex v0 = input.vertices[input.indices[start + 0]];
                Vertex v1 = input.vertices[input.indices[start + 1]];

                for (size_t i = start + 2; i < end; ++i)
                {
                    triangle.vertex[(i + 0) & 1] = v0;
                    triangle.vertex[(i + 1) & 1] = v1;
                    triangle.vertex[2] = input.vertices[input.indices[i]];

                    v0 = v1;
                    v1 = triangle.vertex[2];

                    output.triangles.push_back(triangle);
                }

                break;
            }

            case Primitive::Mode::TRIANGLE_FAN:
            {
                Triangle triangle;
                triangle.material = primitive.material;

                const size_t start = primitive.start;
                const size_t end = start + primitive.count;

                triangle.vertex[0] = input.vertices[input.indices[start + 0]];
                triangle.vertex[2] = input.vertices[input.indices[start + 1]];

                for (size_t i = start + 2; i < end; ++i)
                {
                    triangle.vertex[1] = triangle.vertex[2];
                    triangle.vertex[2] = input.vertices[input.indices[i]];

                    output.triangles.push_back(triangle);
                }

                break;
            }
        }
    }

    return output;
}

IndexedMesh convertMesh(const Mesh& input)
{
    IndexedMesh output;

    std::vector<Triangle> triangles = input.triangles;
 
    std::sort(triangles.begin(), triangles.end(), [] (const Triangle& a, const Triangle& b)
    {
        // sort triangles by material
        return a.material < b.material;
    });

    std::map<Vertex, size_t> unique;

    Primitive primitive;

    primitive.mode = Primitive::Mode::TRIANGLE_LIST;
    primitive.start = 0;
    primitive.count = 0;
    primitive.material = 0;

    for (const Triangle& triangle : input.triangles)
    {
        if (primitive.material != triangle.material)
        {
            if (primitive.count > 0)
            {
                output.primitives.push_back(primitive);
            }

            // start a new primitive
            primitive.start += primitive.count;
            primitive.count = 0;
            primitive.material = triangle.material;
        }

        for (int i = 0; i < 3; ++i)
        {
            const Vertex& vertex = triangle.vertex[i];
            size_t index;

            auto it = unique.find(vertex);
            if (it != unique.end())
            {
                // vertex already exists; use it's index
                index = it->second;
            }
            else
            {
                index = output.vertices.size();
                unique[vertex] = index; // remember the index of this vertex
                output.vertices.push_back(vertex);
            }

            output.indices.push_back(u32(index));
            ++primitive.count;
        }
    }

    if (primitive.count > 0)
    {
        output.primitives.push_back(primitive);
    }

    return output;
}

Cube::Cube(float32x3 size)
{
    const float32x3 pos = size * 0.5f;
    const float32x3 neg = size * -0.5f;

    const float32x3 positions [] =
    {
        float32x3(neg.x, neg.y, neg.z),
        float32x3(pos.x, neg.y, neg.z),
        float32x3(neg.x, pos.y, neg.z),
        float32x3(pos.x, pos.y, neg.z),
        float32x3(neg.x, neg.y, pos.z),
        float32x3(pos.x, neg.y, pos.z),
        float32x3(neg.x, pos.y, pos.z),
        float32x3(pos.x, pos.y, pos.z),
    };

    const float32x3 normals [] =
    {
        float32x3( 1.0f, 0.0f, 0.0f),
        float32x3(-1.0f, 0.0f, 0.0f),
        float32x3( 0.0f, 1.0f, 0.0f),
        float32x3( 0.0f,-1.0f, 0.0f),
        float32x3( 0.0f, 0.0f, 1.0f),
        float32x3( 0.0f, 0.0f,-1.0f),
    };

    const float32x4 tangents [] =
    {
        float32x4( 0.0f, 0.0f, 1.0f, 1.0f),
        float32x4( 0.0f, 0.0f,-1.0f, 1.0f),
        float32x4( 1.0f, 0.0f, 0.0f, 1.0f),
        float32x4( 1.0f, 0.0f, 0.0f, 1.0f),
        float32x4(-1.0f, 0.0f, 0.0f, 1.0f),
        float32x4( 1.0f, 0.0f, 0.0f, 1.0f),
    };

    const float32x2 texcoords [] =
    {
        float32x2(0.0f, 1.0f),
        float32x2(0.0f, 0.0f),
        float32x2(1.0f, 0.0f),
        float32x2(1.0f, 1.0f),
    };

    const u32 points [] =
    {
        1, 3, 7, 5,
        4, 6, 2, 0,
        2, 6, 7, 3,
        4, 0, 1, 5,
        5, 7, 6, 4,
        0, 2, 3, 1,
    };

    for (int i = 0; i < 6; ++i)
    {
        Vertex vertex;

        vertex.normal = normals[i];
        vertex.tangent = tangents[i];

        for (int j = 0; j < 4; ++j)
        {
            vertex.position = positions[points[i * 4 + j]];
            vertex.texcoord = texcoords[j];
            vertices.push_back(vertex);
        }
    }

    indices =
    {
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    Primitive primitive;

    primitive.mode = Primitive::Mode::TRIANGLE_LIST;
    primitive.start = 0;
    primitive.count = 36;
    primitive.material = 0;

    primitives.push_back(primitive);
}

Icosahedron::Icosahedron(float radius)
{
    const float sqrt5 = std::sqrt(5.0f);
    const float phi = (1.0f + sqrt5) * 0.5f; // "golden ratio"

    const float ratio = std::sqrt(10.0f + (2.0f * sqrt5)) / (4.0f * phi);
    const float a = (radius / ratio) * 0.5;
    const float b = (radius / ratio) / (2.0f * phi);

    const float32x3 points [] =
    {
        float32x3( 0,  b, -a),
        float32x3( b,  a,  0),
        float32x3(-b,  a,  0),
        float32x3( 0,  b,  a),
        float32x3( 0, -b,  a),
        float32x3(-a,  0,  b),
        float32x3( 0, -b, -a),
        float32x3( a,  0, -b),
        float32x3( a,  0,  b),
        float32x3(-a,  0, -b),
        float32x3( b, -a,  0),
        float32x3(-b, -a,  0),
    };

    const u32 faces [] =
    {
        2, 1, 0, 1, 2, 3, 5, 4, 3, 4, 8, 3,
        7, 6, 0, 6, 9, 0, 11, 10, 4, 10, 11, 6,
        9, 5, 2, 5, 9, 11, 8, 7, 1, 7, 8, 10,
        2, 5, 3, 8, 1, 3, 9, 2, 0, 1, 7, 0, 
        11, 9, 6, 7, 10, 6, 5, 11, 4, 10, 8, 4,
    };

    Mesh mesh;

    for (int i = 0; i < 20; ++i)
    {
        float32x3 p0 = points[faces[i * 3 + 0]];
        float32x3 p1 = points[faces[i * 3 + 1]];
        float32x3 p2 = points[faces[i * 3 + 2]];

        float32x3 normal = normalize(cross(p0 - p1, p0 - p2));

        Triangle triangle;

        triangle.vertex[0].normal = normal;
        triangle.vertex[1].normal = normal;
        triangle.vertex[2].normal = normal;

        triangle.vertex[0].position = p0;
        triangle.vertex[1].position = p1;
        triangle.vertex[2].position = p2;

        mesh.triangles.push_back(triangle);
    }

    *reinterpret_cast<IndexedMesh*>(this) = convertMesh(mesh);
}

Dodecahedron::Dodecahedron(float radius)
{
    const float sqrt5 = std::sqrt(5.0f);
    const float phi = (1.0f + sqrt5) * 0.5f; // "golden ratio"

    const float a = phi;
    const float b = 1.0f / phi;

    const float scale = radius / sqrt(3.0f);

    const float32x3 points [] =
    {
        float32x3( 1,  1,  1),
        float32x3( 1,  1, -1),
        float32x3( 1, -1,  1),
        float32x3( 1, -1, -1),
        float32x3(-1,  1,  1),
        float32x3(-1,  1, -1),
        float32x3(-1, -1,  1),
        float32x3(-1, -1, -1),
        float32x3( 0,  b,  a),
        float32x3( 0,  b, -a),
        float32x3( 0, -b,  a),
        float32x3( 0, -b, -a),
        float32x3( b,  a,  0),
        float32x3( b, -a,  0),
        float32x3(-b,  a,  0),
        float32x3(-b, -a,  0),
        float32x3( a,  0,  b),
        float32x3( a,  0, -b),
        float32x3(-a,  0,  b),
        float32x3(-a,  0, -b),
    };

    const u32 faces [] =
    {
        8, 10, 2, 16, 0,
        12, 14, 4, 8, 0,
        16, 17, 1, 12, 0,
        17, 3, 11, 9, 1,
        9, 5, 14, 12, 1,
        10, 6, 15, 13, 2,
        13, 3, 17, 16, 2,
        13, 15, 7, 11, 3,
        18, 6, 10, 8, 4,
        14, 5, 19, 18, 4,
        9, 11, 7, 19, 5,
        6, 18, 19, 7, 15,
    };

    Mesh mesh;

    for (int i = 0; i < 12; ++i)
    {
        float32x3 p0 = points[faces[i * 5 + 0]] * scale;
        float32x3 p1 = points[faces[i * 5 + 1]] * scale;
        float32x3 p2 = points[faces[i * 5 + 2]] * scale;
        float32x3 p3 = points[faces[i * 5 + 3]] * scale;
        float32x3 p4 = points[faces[i * 5 + 4]] * scale;

        float32x3 normal = normalize(cross(p0 - p1, p0 - p2));

        Triangle triangle;

        triangle.vertex[0].normal = normal;
        triangle.vertex[1].normal = normal;
        triangle.vertex[2].normal = normal;

        triangle.vertex[0].position = p0;
        triangle.vertex[1].position = p1;
        triangle.vertex[2].position = p2;

        mesh.triangles.push_back(triangle);

        triangle.vertex[0].position = p0;
        triangle.vertex[1].position = p2;
        triangle.vertex[2].position = p3;

        mesh.triangles.push_back(triangle);

        triangle.vertex[0].position = p0;
        triangle.vertex[1].position = p3;
        triangle.vertex[2].position = p4;

        mesh.triangles.push_back(triangle);
    }

    *reinterpret_cast<IndexedMesh*>(this) = convertMesh(mesh);

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

            float32x3 position(
                icos * (params.innerRadius + jcos * params.outerRadius),
                isin * (params.innerRadius + jcos * params.outerRadius),
                jsin * params.outerRadius);
            float32x3 tangent = normalize(float32x3(-position.y, position.x, 0.0f));

            Vertex vertex;

            vertex.position = position;
            vertex.normal = normalize(float32x3(jcos * icos, jcos * isin, jsin));
            vertex.tangent = float32x4(tangent, 1.0f);;
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

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);

            indices.push_back(b);
            indices.push_back(d);
            indices.push_back(c);
        }
    }

    Primitive primitive;

    primitive.mode = Primitive::Mode::TRIANGLE_LIST;
    primitive.start = 0;
    primitive.count = u32(indices.size());
    primitive.material = 0;

    primitives.push_back(primitive);
}

// Torus knot generation
// written by Jari Komppa aka Sol / Trauma
// Based on:
// http://www.blackpawn.com/texts/pqtorus/default.html

Torusknot::Torusknot(Parameters params)
{
    params.scale *= 0.5f;
    params.thickness *= params.scale;

    const float uscale = params.uscale / params.facets;
    const float vscale = params.vscale / params.steps;

    // generate vertices
    vertices.resize((params.steps + 1) * (params.facets + 1) + 1);

    float32x3 centerpoint;
    float Pp = params.p * 0 * pi2 / params.steps;
    float Qp = params.q * 0 * pi2 / params.steps;
    float r = (0.5f * (2.0f + std::sin(Qp))) * params.scale;
    centerpoint.x = r * std::cos(Pp);
    centerpoint.y = r * std::cos(Qp);
    centerpoint.z = r * std::sin(Pp);

    for (int i = 0; i < params.steps; i++)
    {
        float32x3 nextpoint;
        Pp = params.p * (i + 1) * pi2 / params.steps;
        Qp = params.q * (i + 1) * pi2 / params.steps;
        r = (0.5f * (2.0f + std::sin(Qp))) * params.scale;
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

            float32x3 normal = N * pointx + B * pointy;
            float32x3 tangent = normalize(B * pointx - N * pointy);

            const int offset = i * (params.facets + 1) + j;

            vertices[offset].position = centerpoint + normal;
            vertices[offset].normal = normalize(normal);
            vertices[offset].tangent = float32x4(tangent, 1.0f);
            vertices[offset].texcoord = float32x2(j * uscale, i * vscale);
        }

        // create duplicate vertex for sideways wrapping
        // otherwise identical to first vertex in the 'ring' except for the U coordinate
        vertices[i * (params.facets + 1) + params.facets] = vertices[i * (params.facets + 1)];
        vertices[i * (params.facets + 1) + params.facets].texcoord.x = params.uscale;
        
        centerpoint = nextpoint;
    }

    // create duplicate ring of vertices for longways wrapping
    // otherwise identical to first 'ring' in the knot except for the V coordinate
    for (int j = 0; j < params.facets; j++)
    {
        vertices[params.steps * (params.facets + 1) + j] = vertices[j];
        vertices[params.steps * (params.facets + 1) + j].texcoord.y = params.vscale;
    }

    // finally, there's one vertex that needs to be duplicated due to both U and V coordinate.
    vertices[params.steps * (params.facets + 1) + params.facets] = vertices[0];
    vertices[params.steps * (params.facets + 1) + params.facets].texcoord = float32x2(params.uscale, params.vscale);

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
        indices.push_back(stripIndices[i - 2 + s]);
        indices.push_back(stripIndices[i - 1 - s]);
        indices.push_back(stripIndices[i]);
    }

    Primitive primitive;

    primitive.mode = Primitive::Mode::TRIANGLE_LIST;
    primitive.start = 0;
    primitive.count = u32(indices.size());
    primitive.material = 0;

    primitives.push_back(primitive);
}

} // namespace mango::import3d
