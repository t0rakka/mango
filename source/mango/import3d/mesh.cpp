/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <map>
#include <mango/core/core.hpp>
#include <mango/import3d/mesh.hpp>
#include "../../external/mikktspace/mikktspace.h"

namespace
{
    using namespace mango;
    using namespace mango::import3d;

    int callback_getNumFaces(const SMikkTSpaceContext* pContext)
    {
        TriangleMesh& mesh = *reinterpret_cast<TriangleMesh*>(pContext->m_pUserData);
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
        TriangleMesh& mesh = *reinterpret_cast<TriangleMesh*>(pContext->m_pUserData);
        Vertex& vertex = mesh.triangles[iFace].vertex[iVert];
        fvPosOut[0] = vertex.position[0];
        fvPosOut[1] = vertex.position[1];
        fvPosOut[2] = vertex.position[2];
    }

    void callback_getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
    {
        TriangleMesh& mesh = *reinterpret_cast<TriangleMesh*>(pContext->m_pUserData);
        Vertex& vertex = mesh.triangles[iFace].vertex[iVert];
        fvNormOut[0] = vertex.normal[0];
        fvNormOut[1] = vertex.normal[1];
        fvNormOut[2] = vertex.normal[2];
    }

    void callback_getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
    {
        TriangleMesh& mesh = *reinterpret_cast<TriangleMesh*>(pContext->m_pUserData);
        Vertex& vertex = mesh.triangles[iFace].vertex[iVert];
        fvTexcOut[0] = vertex.texcoord[0];
        fvTexcOut[1] = vertex.texcoord[1];
    }

    void callback_setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
    {
        TriangleMesh& mesh = *reinterpret_cast<TriangleMesh*>(pContext->m_pUserData);
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

// --------------------------------------------------------------------
// std::unordered_map<Vertex, u32>
// --------------------------------------------------------------------

static inline
bool operator == (const Vertex& a, const Vertex& b)
{
    return std::memcmp(&a, &b, sizeof(Vertex)) == 0;
}

struct VertexHash
{
    std::size_t operator () (const Vertex& v) const
    {
        size_t h0 = std::hash<float>{}(v.position.x);
        size_t h1 = std::hash<float>{}(v.position.y);
        size_t h2 = std::hash<float>{}(v.position.z);
        size_t h = (h0 ^ h1) ^ (h2 >> 7);
        return h;
    }
};

// --------------------------------------------------------------------
// xxx
// --------------------------------------------------------------------

#if 0

// TODO: refactor this into generic strip/fan -> triangles index converter
// TODO: restart primitive

Mesh convertMesh(const IndexedMesh& input)
{
    Mesh output;

    for (const Primitive& primitive : input.primitives)
    {
        const u32 baseIndex = primitive.base;

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
                    triangle.vertex[0] = input.vertices[baseIndex + input.indices[i + 0]];
                    triangle.vertex[1] = input.vertices[baseIndex + input.indices[i + 1]];
                    triangle.vertex[2] = input.vertices[baseIndex + input.indices[i + 2]];

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

                Vertex v0 = input.vertices[baseIndex + input.indices[start + 0]];
                Vertex v1 = input.vertices[baseIndex + input.indices[start + 1]];

                for (size_t i = start + 2; i < end; ++i)
                {
                    triangle.vertex[(i + 0) & 1] = v0;
                    triangle.vertex[(i + 1) & 1] = v1;
                    triangle.vertex[2] = input.vertices[baseIndex + input.indices[i]];

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

                triangle.vertex[0] = input.vertices[baseIndex + input.indices[start + 0]];
                triangle.vertex[2] = input.vertices[baseIndex + input.indices[start + 1]];

                for (size_t i = start + 2; i < end; ++i)
                {
                    triangle.vertex[1] = triangle.vertex[2];
                    triangle.vertex[2] = input.vertices[baseIndex + input.indices[i]];

                    output.triangles.push_back(triangle);
                }

                break;
            }
        }
    }

    return output;
}

#endif // 0

void TriangleMesh::computeTangents()
{
    if (flags & Vertex::TANGENT)
    {
        // already have tangents
        return;
    }

    if (!(flags & Vertex::NORMAL) || !(flags & Vertex::TEXCOORD))
    {
        // normals and texcoords are required for tangents
        return;
    }

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
    mik_context.m_pUserData = this;

    tbool status = genTangSpaceDefault(&mik_context);
    MANGO_UNREFERENCED(status);

    // enable tangents
    flags |= Vertex::TANGENT;
}

struct IndexedMeshBuilder
{
    std::unordered_map<Vertex, u32, VertexHash> unique;

    void append(IndexedMesh& output, const TriangleMesh& input);
};

void IndexedMeshBuilder::append(IndexedMesh& output, const TriangleMesh& input)
{
    u32 startIndex = u32(output.indices.size());

    for (const Triangle& triangle : input.triangles)
    {
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

                // update bounding box
                output.boundingBox.extend(vertex.position);
            }

            output.indices.push_back(u32(index));
        }
    }

    u32 endIndex = u32(output.indices.size());

    output.primitives.emplace_back(startIndex, endIndex - startIndex, input.material);
    output.flags |= input.flags;
}

IndexedMesh::IndexedMesh()
{
}

IndexedMesh::IndexedMesh(const TriangleMesh& trimesh)
{
    IndexedMeshBuilder builder;
    builder.append(*this, trimesh);
}

IndexedMesh::IndexedMesh(const std::vector<TriangleMesh>& trimeshes)
{
    IndexedMeshBuilder builder;

    for (const auto& trimesh : trimeshes)
    {
        builder.append(*this, trimesh);
    }
}

Texture createTexture(const filesystem::Path& path, const std::string& filename)
{
    std::shared_ptr<image::Bitmap> texture;

    if (filename.empty())
    {
        return texture;
    }

    filesystem::File file(path, filename);

    printLine("createTexture: {}", filename);

    image::Format format(32, image::Format::UNORM, image::Format::RGBA, 8, 8, 8, 8);
    texture = std::make_shared<image::Bitmap>(file, filename, format);

    return texture;
}

Texture createTexture(ConstMemory memory)
{
    printLine("createTexture: {} KB", memory.size);

    image::Format format(32, image::Format::UNORM, image::Format::RGBA, 8, 8, 8, 8);
    std::shared_ptr<image::Bitmap> texture = std::make_shared<image::Bitmap>(memory, "", format);

    return texture;
}

static
u32 getAttributeSize(const VertexAttribute& attribute)
{
    u32 bytes = 0;

    switch (attribute.type)
    {
        case import3d::VertexAttribute::NONE:
            break;

        case import3d::VertexAttribute::INT8:
        case import3d::VertexAttribute::UINT8:
            bytes = 1;
            break;

        case import3d::VertexAttribute::INT16:
        case import3d::VertexAttribute::UINT16:
        case import3d::VertexAttribute::FLOAT16:
            bytes = 2;
            break;

        case import3d::VertexAttribute::INT32:
        case import3d::VertexAttribute::UINT32:
        case import3d::VertexAttribute::FLOAT32:
            bytes = 4;
            break;
    }

    return attribute.size * bytes;
}

VertexAttribute::VertexAttribute()
{
}

VertexAttribute::VertexAttribute(Type type, u32 size)
    : type(type)
    , size(size)
{
    bytes = getAttributeSize(*this);
}

VertexAttribute::VertexAttribute(Type type, u32 size, u32 stride, size_t offset)
    : type(type)
    , size(size)
    , stride(stride)
    , offset(offset)
{
    bytes = getAttributeSize(*this);
}

VertexAttribute::operator bool () const
{
    return bytes != 0;
}

class VertexAttributeBuilder
{
protected:
    std::vector<VertexAttribute> attributes;

public:
    void append(VertexAttribute::Type type, u32 size);
    size_t resolve(size_t numVertex, bool interleave = false);

    const VertexAttribute& operator [] (int index) const
    {
        return attributes[index];
    }
};

void VertexAttributeBuilder::append(VertexAttribute::Type type, u32 size)
{
    attributes.emplace_back(type, size);
}

size_t VertexAttributeBuilder::resolve(size_t numVertex, bool interleave)
{
    size_t bytesPerVertex = 0;

    for (auto& attribute : attributes)
    {
        if (interleave)
        {
            attribute.offset = bytesPerVertex;
        }
        else
        {
            attribute.stride = attribute.bytes;
            attribute.offset = numVertex * bytesPerVertex;
        }

        bytesPerVertex += attribute.bytes;
    }

    if (interleave)
    {
        for (auto& attribute : attributes)
        {
            attribute.stride = bytesPerVertex;
        }
    }

   return bytesPerVertex;
}

Mesh::Mesh()
{
}

Mesh::Mesh(const IndexedMesh& mesh)
{
    const size_t numVertex = mesh.vertices.size();

    VertexAttributeBuilder builder;

    if (mesh.flags & Vertex::POSITION)
    {
        builder.append(VertexAttribute::FLOAT32, 3);
    }

    if (mesh.flags & Vertex::NORMAL)
    {
        builder.append(VertexAttribute::FLOAT32, 3);
    }

    if (mesh.flags & Vertex::TANGENT)
    {
        builder.append(VertexAttribute::FLOAT32, 4);
    }

    if (mesh.flags & Vertex::TEXCOORD)
    {
        builder.append(VertexAttribute::FLOAT32, 2);
    }

    if (mesh.flags & Vertex::COLOR)
    {
        builder.append(VertexAttribute::FLOAT32, 3);
    }

    const size_t bytesPerVertex = builder.resolve(numVertex, false);

    vertices.resize(numVertex * bytesPerVertex);
    u8* data = vertices.data();

    size_t index = 0;

    if (mesh.flags & Vertex::POSITION)
    {
        position = builder[index++];

        for (size_t i = 0; i < numVertex; ++i)
        {
            const Vertex* vertex = &mesh.vertices[i];
            std::memcpy(data + position.offset + i * position.stride, vertex->position.data(), position.bytes);
        }
    }

    if (mesh.flags & Vertex::NORMAL)
    {
        normal = builder[index++];

        for (size_t i = 0; i < numVertex; ++i)
        {
            const Vertex* vertex = &mesh.vertices[i];
            std::memcpy(data + normal.offset + i * normal.stride, vertex->normal.data(), normal.bytes);
        }
    }

    if (mesh.flags & Vertex::TANGENT)
    {
        tangent = builder[index++];

        for (size_t i = 0; i < numVertex; ++i)
        {
            const Vertex* vertex = &mesh.vertices[i];
            std::memcpy(data + tangent.offset + i * tangent.stride, vertex->tangent.data(), tangent.bytes);
        }
    }

    if (mesh.flags & Vertex::TEXCOORD)
    {
        texcoord = builder[index++];

        for (size_t i = 0; i < numVertex; ++i)
        {
            const Vertex* vertex = &mesh.vertices[i];
            std::memcpy(data + texcoord.offset + i * texcoord.stride, vertex->texcoord.data(), texcoord.bytes);
        }
    }

    if (mesh.flags & Vertex::COLOR)
    {
        color = builder[index++];

        for (size_t i = 0; i < numVertex; ++i)
        {
            const Vertex* vertex = &mesh.vertices[i];
            std::memcpy(data + color.offset + i * color.stride, vertex->color.data(), color.bytes);
        }
    }

    indices.type = IndexBuffer::UINT32;
    indices.append(mesh.indices);

    for (auto p : mesh.primitives)
    {
        Primitive primitive;

        primitive.mode = Primitive::TRIANGLE_LIST;
        primitive.start = p.start;
        primitive.count = p.count;
        primitive.base = 0;
        primitive.material = p.material;

        primitives.push_back(primitive);
    }

    boundingBox = mesh.boundingBox;
}

std::unique_ptr<Mesh> createCube(float32x3 size)
{
    const float32x3 pos = size * 0.5f;
    const float32x3 neg = -pos;

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

    const u32 faces [] =
    {
        1, 3, 7, 5,
        4, 6, 2, 0,
        2, 6, 7, 3,
        4, 0, 1, 5,
        5, 7, 6, 4,
        0, 2, 3, 1,
    };

    const u16 indices [] =
    {
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    const size_t numVertex = 6 * 4;

    // create mesh

    std::unique_ptr<Mesh> ptr = std::make_unique<Mesh>();
    Mesh& mesh = *ptr;

    VertexAttributeBuilder builder;

    builder.append(VertexAttribute::FLOAT32, 3);
    builder.append(VertexAttribute::FLOAT32, 3);
    builder.append(VertexAttribute::FLOAT32, 4);
    builder.append(VertexAttribute::FLOAT32, 2);

    size_t bytesPerVertex = builder.resolve(numVertex, true);

    mesh.position = builder[0];
    mesh.normal   = builder[1];
    mesh.tangent  = builder[2];
    mesh.texcoord = builder[3];

    mesh.vertices.resize(numVertex * bytesPerVertex);

    u8* data = mesh.vertices.data();

    for (int i = 0; i < 6; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            const int k = faces[i * 4 + j];
            std::memcpy(data + mesh.position.offset, positions + k, mesh.position.bytes);
            std::memcpy(data + mesh.normal.offset,   normals + i,   mesh.normal.bytes);
            std::memcpy(data + mesh.tangent.offset,  tangents + i,  mesh.tangent.bytes);
            std::memcpy(data + mesh.texcoord.offset, texcoords + j, mesh.texcoord.bytes);
            data += bytesPerVertex;
        }
    }

    mesh.indices.type = IndexBuffer::UINT16;
    mesh.indices.append(indices, sizeof(indices));

    Primitive primitive;

    primitive.mode = Primitive::TRIANGLE_LIST;
    primitive.start = 0;
    primitive.count = 36;
    primitive.base = 0;
    primitive.material = 0;

    mesh.primitives.push_back(primitive);

    mesh.boundingBox.extend(pos);
    mesh.boundingBox.extend(neg);

    return ptr;
}

std::unique_ptr<Mesh> createIcosahedron(float radius)
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

    TriangleMesh trimesh;

    trimesh.flags = Vertex::POSITION | Vertex::NORMAL;

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

        trimesh.triangles.push_back(triangle);
    }

    std::unique_ptr<Mesh> ptr = std::make_unique<Mesh>(trimesh);
    return ptr;
}

std::unique_ptr<Mesh> createDodecahedron(float radius)
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

    TriangleMesh trimesh;

    trimesh.flags = Vertex::POSITION | Vertex::NORMAL;

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

        trimesh.triangles.push_back(triangle);

        triangle.vertex[0].position = p0;
        triangle.vertex[1].position = p2;
        triangle.vertex[2].position = p3;

        trimesh.triangles.push_back(triangle);

        triangle.vertex[0].position = p0;
        triangle.vertex[1].position = p3;
        triangle.vertex[2].position = p4;

        trimesh.triangles.push_back(triangle);
    }

    std::unique_ptr<Mesh> ptr = std::make_unique<Mesh>(trimesh);
    return ptr;
}

std::unique_ptr<Mesh> createTorus(TorusParameters params)
{
    const float is = pi2 / params.innerSegments;
    const float js = pi2 / params.outerSegments;

    const float uscale = 4.0f / float(params.innerSegments);
    const float vscale = 1.0f / float(params.outerSegments);

    const size_t numVertex = (params.innerSegments + 1) * (params.outerSegments + 1);

    std::vector<float32x3> positions(numVertex);
    std::vector<float32x3> normals(numVertex);
    std::vector<float32x4> tangents(numVertex);
    std::vector<float32x2> texcoords(numVertex);

    math::Box boundingBox;

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

            const size_t index = i * (params.outerSegments + 1) + j;

            positions[index] = position;
            normals  [index] = normalize(float32x3(jcos * icos, jcos * isin, jsin));
            tangents [index] = float32x4(tangent, 1.0f);
            texcoords[index] = float32x2(i * uscale, j * vscale);

            boundingBox.extend(position);
        }
    }

    std::vector<u32> indices;

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

    // create mesh

    std::unique_ptr<Mesh> ptr = std::make_unique<Mesh>();
    Mesh& mesh = *ptr;

    VertexAttributeBuilder builder;

    builder.append(VertexAttribute::FLOAT32, 3);
    builder.append(VertexAttribute::FLOAT32, 3);
    builder.append(VertexAttribute::FLOAT32, 4);
    builder.append(VertexAttribute::FLOAT32, 2);

    size_t bytesPerVertex = builder.resolve(numVertex);

    mesh.position = builder[0];
    mesh.normal   = builder[1];
    mesh.tangent  = builder[2];
    mesh.texcoord = builder[3];

    mesh.vertices.resize(numVertex * bytesPerVertex);

    u8* data = mesh.vertices.data();
    std::memcpy(data + mesh.position.offset, positions.data(), numVertex * mesh.position.bytes);
    std::memcpy(data + mesh.normal.offset,   normals.data(),   numVertex * mesh.normal.bytes);
    std::memcpy(data + mesh.tangent.offset,  tangents.data(),  numVertex * mesh.tangent.bytes);
    std::memcpy(data + mesh.texcoord.offset, texcoords.data(), numVertex * mesh.texcoord.bytes);

    mesh.indices.type = IndexBuffer::UINT32;
    mesh.indices.append(indices);

    Primitive primitive;

    primitive.mode = Primitive::TRIANGLE_LIST;
    primitive.start = 0;
    primitive.count = u32(indices.size());
    primitive.base = 0;
    primitive.material = 0;

    mesh.primitives.push_back(primitive);

    mesh.boundingBox = boundingBox;

    return ptr;
}

std::unique_ptr<Mesh> createTorusknot(TorusknotParameters params)
{
    // Torus knot generation
    // written by Jari Komppa aka Sol / Trauma
    // Tangent space generation added by t0rakka
    // Based on: http://www.blackpawn.com/texts/pqtorus/default.html

    params.scale *= 0.5f;
    params.thickness *= params.scale;

    const float uscale = params.uscale / params.facets;
    const float vscale = params.vscale / params.steps;

    const size_t numVertex = (params.steps + 1) * (params.facets + 1) + 1;

    std::vector<float32x3> positions(numVertex);
    std::vector<float32x3> normals(numVertex);
    std::vector<float32x4> tangents(numVertex);
    std::vector<float32x2> texcoords(numVertex);

    float Pp = params.p * 0 * pi2 / params.steps;
    float Qp = params.q * 0 * pi2 / params.steps;
    float r = (0.5f * (2.0f + std::sin(Qp))) * params.scale;

    float32x3 centerpoint;
    centerpoint.x = r * std::cos(Pp);
    centerpoint.y = r * std::cos(Qp);
    centerpoint.z = r * std::sin(Pp);

    for (int i = 0; i < params.steps; ++i)
    {
        Pp = params.p * (i + 1) * pi2 / params.steps;
        Qp = params.q * (i + 1) * pi2 / params.steps;
        r = (0.5f * (2.0f + std::sin(Qp))) * params.scale;

        float32x3 nextpoint;
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

            positions[offset] = centerpoint + normal;
            normals  [offset] = normalize(normal);
            tangents [offset] = float32x4(tangent, 1.0f);
            texcoords[offset] = float32x2(j * uscale, i * vscale);
        }

        // create duplicate vertex for sideways wrapping
        // otherwise identical to first vertex in the 'ring' except for the U coordinate

        const size_t s = i * (params.facets + 1);
        const size_t d = i * (params.facets + 1) + params.facets;

        positions[d] = positions[s];
        normals  [d] = normals  [s];
        tangents [d] = tangents [s];
        texcoords[d] = float32x2(params.uscale, texcoords[s].y);
        
        centerpoint = nextpoint;
    }

    // create duplicate ring of vertices for longways wrapping
    // otherwise identical to first 'ring' in the knot except for the V coordinate

    for (int j = 0; j < params.facets; ++j)
    {
        const size_t d = params.steps * (params.facets + 1) + j;

        positions[d] = positions[j];
        normals  [d] = normals  [j];
        tangents [d] = tangents [j];
        texcoords[d] = float32x2(texcoords[j].x, params.vscale);
    }

    // finally, there's one vertex that needs to be duplicated due to both U and V coordinate.

    const size_t d = params.steps * (params.facets + 1) + params.facets;
    positions[d] = positions[0];
    normals  [d] = normals  [0];
    tangents [d] = tangents [0];
    texcoords[d] = float32x2(params.uscale, params.vscale);

    // generate indices

    std::vector<u32> indices((params.steps + 1) * params.facets * 2);

    for (int j = 0; j < params.facets; j++)
    {
        for (int i = 0; i < params.steps + 1; i++)
        {
            indices[i * 2 + 0 + j * (params.steps + 1) * 2] = (j + 1 + i * (params.facets + 1));
            indices[i * 2 + 1 + j * (params.steps + 1) * 2] = (j + 0 + i * (params.facets + 1));
        }
    }

    // create mesh

    std::unique_ptr<Mesh> ptr = std::make_unique<Mesh>();
    Mesh& mesh = *ptr;

    VertexAttributeBuilder builder;

    builder.append(VertexAttribute::FLOAT32, 3);
    builder.append(VertexAttribute::FLOAT32, 3);
    builder.append(VertexAttribute::FLOAT32, 4);
    builder.append(VertexAttribute::FLOAT32, 2);

    size_t bytesPerVertex = builder.resolve(numVertex);

    mesh.position = builder[0];
    mesh.normal   = builder[1];
    mesh.tangent  = builder[2];
    mesh.texcoord = builder[3];

    mesh.vertices.resize(numVertex * bytesPerVertex);

    u8* data = mesh.vertices.data();
    std::memcpy(data + mesh.position.offset, positions.data(), numVertex * mesh.position.bytes);
    std::memcpy(data + mesh.normal.offset,   normals.data(),   numVertex * mesh.normal.bytes);
    std::memcpy(data + mesh.tangent.offset,  tangents.data(),  numVertex * mesh.tangent.bytes);
    std::memcpy(data + mesh.texcoord.offset, texcoords.data(), numVertex * mesh.texcoord.bytes);

    mesh.indices.type = IndexBuffer::UINT32;
    mesh.indices.append(indices);

    Primitive primitive;

    primitive.mode = Primitive::TRIANGLE_STRIP;
    primitive.start = 0;
    primitive.count = u32(indices.size());
    primitive.base = 0;
    primitive.material = 0;

    mesh.primitives.push_back(primitive);

    for (const float32x3& position : positions)
    {
        mesh.boundingBox.extend(position);
    }

    return ptr;
}

} // namespace mango::import3d
