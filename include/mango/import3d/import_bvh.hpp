/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <mango/import3d/mesh.hpp>

namespace mango::import3d
{

    // One joint from the BVH HIERARCHY section (engine space: +Z ahead).
    struct BvhJoint
    {
        std::string name;
        int parent = -1; // index into ImportBVH::joints / BvhClip::joints
        float32x3 offset { 0.0f, 0.0f, 0.0f }; // rest local translation (OFFSET)
    };

    // Biovision Hierarchy (.bvh) → Animation clip + skeleton for retargeting.
    // Channels use targetName (joint names from the file); node indices are left
    // empty until bindAnimation() against a rigged Scene.
    //
    // Coordinate space matches other importers (+X right, +Y up, +Z ahead).
    struct ImportBVH
    {
        ImportBVH(const filesystem::Path& path, const std::string& filename);
        explicit ImportBVH(ConstMemory memory, const std::string& name = "BVH");

        Animation animation;
        std::vector<BvhJoint> joints;      // hierarchy order (ROOT first, parents before children)
        std::vector<std::string> jointNames; // same order as joints[].name
    };

    struct BvhClip
    {
        Animation animation;
        std::vector<BvhJoint> joints;
    };

    // Load a .bvh as an external clip (animation only; no skeleton).
    Animation importAnimation(const std::string& filename);

    // Load a .bvh with hierarchy (needed to retarget onto a different bind pose).
    BvhClip importBvhClip(const std::string& filename);

} // namespace mango::import3d
