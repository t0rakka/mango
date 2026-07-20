/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/import3d/import3d.hpp>

namespace mango::import3d
{

    std::unique_ptr<Scene> importScene(const std::string& filename)
    {
        std::string dir = filesystem::getPath(filename);
        if (dir.empty())
        {
            dir = "./";
        }

        const std::string file = filesystem::removePath(filename);
        const std::string ext = toLower(filesystem::getExtension(filename));
        const filesystem::Path path(dir);

        // Path is the asset directory; basename is the file. Texture / sidecar
        // URIs resolve relative to path (same contract as each Import* ctor).

        if (ext == ".gltf" || ext == ".glb")
        {
            return std::make_unique<ImportGLTF>(path, file);
        }
        if (ext == ".fbx")
        {
            return std::make_unique<ImportFBX>(path, file);
        }
        if (ext == ".obj")
        {
            return std::make_unique<ImportOBJ>(path, file);
        }
        if (ext == ".3ds")
        {
            return std::make_unique<Import3DS>(path, file);
        }
        if (ext == ".lwo")
        {
            return std::make_unique<ImportLWO>(path, file);
        }

        MANGO_EXCEPTION("[import3d] Unsupported format '{}' (expected .gltf/.glb/.fbx/.obj/.3ds/.lwo).", ext);
    }

} // namespace mango::import3d
