/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/import3d/mesh.hpp>

namespace mango::import3d
{

    struct Import3DS : Scene
    {
        Import3DS(const filesystem::Path& path, const std::string& filename);
    };

} // namespace mango::import3d
