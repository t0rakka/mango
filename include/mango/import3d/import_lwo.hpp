/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/import3d/mesh.hpp>

namespace mango::import3d
{

    struct ImportLWO : Scene
    {
        ImportLWO(const filesystem::Path& path, const std::string& filename);
    };

} // namespace mango::import3d
