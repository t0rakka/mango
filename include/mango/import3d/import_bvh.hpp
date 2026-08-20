/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/import3d/mesh.hpp>

namespace mango::import3d
{

    // Biovision Hierarchy (.bvh): skeleton from HIERARCHY, clip from MOTION.
    struct ImportBVH
    {
        ImportBVH(const filesystem::Path& path, const std::string& filename);
        explicit ImportBVH(ConstMemory memory, const std::string& name = "BVH");

        Skeleton skeleton;
        Animation clip;
    };

} // namespace mango::import3d
