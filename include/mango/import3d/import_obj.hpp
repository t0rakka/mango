/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/filesystem/filesystem.hpp>
#include <mango/import3d/mesh.hpp>

namespace mango::import3d
{

    struct Object
    {
        std::string name;
        u32 parent;
        matrix4x4 transform;
        std::vector<u32> meshes;
    };

    struct ImportOBJ
    {
        //std::vector<Material> materials;
        std::vector<Mesh> meshes;
        std::vector<Object> objects;

        ImportOBJ(const filesystem::Path& path, const std::string& filename);
    };

} // namespace mango::import3d
