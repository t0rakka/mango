/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <memory>
#include <string>

#include <mango/import3d/mesh.hpp>
#include <mango/import3d/import_3ds.hpp>
#include <mango/import3d/import_obj.hpp>
#include <mango/import3d/import_lwo.hpp>
#include <mango/import3d/import_fbx.hpp>
#include <mango/import3d/import_gltf.hpp>

namespace mango::import3d
{

    // Load a scene by filename extension:
    //   .gltf / .glb → ImportGLTF
    //   .fbx         → ImportFBX
    //   .obj         → ImportOBJ
    //   .3ds         → Import3DS
    //   .lwo         → ImportLWO
    // Throws on unsupported extension or importer failure.
    std::unique_ptr<Scene> importScene(const std::string& filename);

} // namespace mango::import3d
