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

    struct ImportGLTF : Scene
    {
        ImportGLTF(const filesystem::Path& path, const std::string& filename);
    };

} // namespace mango::import3d
