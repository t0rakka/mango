/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <string>
#include <optional>
#include <mango/core/configure.hpp>
#include <mango/math/math.hpp>

namespace mango::import3d
{

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
        std::vector<math::Matrix<float, 4, 4>> inverseBindMatrices;
        std::optional<u32> skeleton; // optional skeleton root node
    };

    // -----------------------------------------------------------------------
    // skeleton (bone hierarchy from source file)
    // -----------------------------------------------------------------------

    struct Joint
    {
        std::string name;
        int parent = -1; // index into Skeleton::joints, -1 for root
        math::Vector<float, 3> offset { 0.0f, 0.0f, 0.0f }; // bind local translation
    };

    struct Skeleton
    {
        std::vector<Joint> joints; // ROOT first, parents before children
    };

    // -----------------------------------------------------------------------
    // animation clip
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
        std::optional<u32> node;       // Scene::nodes (glTF, same file)
        std::optional<u32> joint;      // Skeleton::joints (BVH, same file)
        std::string targetName;        // target name from source file
        AnimationPath path { AnimationPath::Translation };
    };

    struct Animation
    {
        std::string name;
        float duration = 0.0f; // max sample time (seconds)
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
    };

} // namespace mango::import3d
