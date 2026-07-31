/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <algorithm>
#include <utility>
#include <unordered_map>
#include <mango/core/buffer.hpp>
#include <mango/math/math.hpp>
#include <mango/image/image.hpp>
#include <mango/filesystem/filesystem.hpp>

namespace mango::import3d
{

    // -----------------------------------------------------------------------
    // Canonical mesh / world / view space (importers + shape generators)
    //
    //   Handedness: right-handed (right × up = forward)
    //   Axes:       +X right, +Y up, +Z ahead (Unity-like labels)
    //   Winding:    clockwise when viewed from outside = front face
    //               OpenGL: GL_CW. Vulkan after a Y-up→Y-down last-mile
    //               (e.g. scale(1,-1,-1)): use VK_FRONT_FACE_COUNTER_CLOCKWISE
    //               — FB signed area flips with Y; mesh data stays CW.
    //   Normals:    outward
    //   Tangents:   .xyz = tangent, .w = bitangent sign for TBN
    //   Texcoords:  (0,0) = top-left; V increases downward
    //
    // -----------------------------------------------------------------------

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
            // glTF JOINTS_0 / WEIGHTS_0 (set 0, up to 4 influences).
            Joints    = 0x0020,
            Weights   = 0x0040,
        };

        float32x3 position { 0.0f, 0.0f, 0.0f };
        float32x3 normal   { 0.0f, 0.0f, 0.0f };
        float32x2 texcoord { 0.0f, 0.0f };
        float32x4 tangent  { 0.0f, 0.0f, 0.0f, 0.0f };
        float32x4 color    { 0.0f, 0.0f, 0.0f, 0.0f };

        // Indices into Skin::joints (not node indices). Unused slots = 0 with weight 0.
        u16 joint[4] { 0, 0, 0, 0 };
        float32x4 weight { 0.0f, 0.0f, 0.0f, 0.0f };
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
    // skinning (glTF-compatible)
    // -----------------------------------------------------------------------

    // Parallel to glTF skin: joints[i] is a node index; inverseBindMatrices[i]
    // transforms mesh bind-pose positions into that joint's local space.
    // Vertex::joint[] indexes into this joints[] array (JOINTS_0).
    struct Skin
    {
        std::string name;
        std::vector<u32> joints;
        std::vector<matrix4x4> inverseBindMatrices;
        std::optional<u32> skeleton; // optional skeleton root node
    };

    // -----------------------------------------------------------------------
    // animation (glTF-compatible; BVH can emit the same type later)
    // -----------------------------------------------------------------------

    enum class AnimationPath : u8
    {
        Translation, // float3
        Rotation,    // float4 quaternion xyzw
        Scale,       // float3
        Weights,     // morph target weights (float[N])
    };

    enum class AnimationInterpolation : u8
    {
        Linear,
        Step,
        CubicSpline, // values layout: [in-tangent, value, out-tangent] × keyframe
    };

    struct AnimationSampler
    {
        AnimationInterpolation interpolation { AnimationInterpolation::Linear };
        std::vector<float> times;   // seconds, monotonically increasing
        std::vector<float> values;  // tightly packed: keyCount * components
                                    // (or 3 * keyCount * components for CubicSpline)
        u32 components { 0 };       // 3 (T/S), 4 (R), or morph weight count
    };

    struct AnimationChannel
    {
        u32 sampler = 0;
        std::optional<u32> node;    // index into Scene::nodes (glTF)
        std::string targetName;     // node name — for BVH / retarget by name
        AnimationPath path { AnimationPath::Translation };
    };

    struct Animation
    {
        std::string name;
        float duration = 0.0f; // max sample time (seconds)
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
    };

    // -----------------------------------------------------------------------
    // scene
    // -----------------------------------------------------------------------

    struct Node
    {
        std::string name;
        std::vector<u32> children;

        // Bind-pose local transform. Prefer TRS for animation sampling; `transform`
        // is the composed matrix (scale * rotation * translation) in engine space.
        float32x3 translation { 0.0f, 0.0f, 0.0f };
        float32x4 rotation { 0.0f, 0.0f, 0.0f, 1.0f }; // quaternion xyzw
        float32x3 scale { 1.0f, 1.0f, 1.0f };
        matrix4x4 transform { 1.0f };
        bool hasTRS = false; // true when translation/rotation/scale are authoritative

        std::optional<u32> mesh;
        std::optional<u32> skin; // glTF: skin lives on the node that instances the mesh
    };

    struct Scene
    {
        std::vector<Material> materials;
        std::vector<std::unique_ptr<IndexedMesh>> meshes;
        std::vector<Skin> skins;
        std::vector<Animation> animations;
        std::vector<Node> nodes;
        std::vector<u32> roots;
    };

    // -----------------------------------------------------------------------
    // animation binding (external clips → rigged Scene)
    // -----------------------------------------------------------------------

    // Rename channel targets before bind: sourceName (e.g. BVH "LeftUpLeg") →
    // rig name (e.g. "thigh_l"). Unknown keys are left unchanged.
    void remapAnimationNames(Animation& animation,
        const std::unordered_map<std::string, std::string>& sourceToTarget);

    struct AnimationBindStats
    {
        u32 bound = 0;     // channels that resolved to a node
        u32 unbound = 0;   // channels with no matching node name
        std::vector<std::string> missing; // unique unbound target names
    };

    // Set channel.node by matching channel.targetName to scene.nodes[].name.
    // Clips stay usable unbound (name-only) for sharing across compatible rigs;
    // bind once per Model/Scene instance before playback.
    AnimationBindStats bindAnimation(Animation& animation, const Scene& scene,
        bool caseInsensitive = true);

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
