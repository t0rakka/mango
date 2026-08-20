/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/import3d/import.hpp>

namespace mango::import3d
{

    std::unique_ptr<Scene> importScene(const filesystem::Path& path, const std::string& filename)
    {
        // Resolve folder containing the scene file; assets are relative to this.
        const std::string dir = filesystem::getPath(filename);
        const std::string file = filesystem::removePath(filename);
        const filesystem::Path scenePath(path, dir);

        const std::string ext = toLower(filesystem::getExtension(filename));

        if (ext == ".gltf" || ext == ".glb")
            return std::make_unique<ImportGLTF>(scenePath, file);
        if (ext == ".fbx")
            return std::make_unique<ImportFBX>(scenePath, file);
        if (ext == ".obj")
            return std::make_unique<ImportOBJ>(scenePath, file);
        if (ext == ".3ds")
            return std::make_unique<Import3DS>(scenePath, file);
        if (ext == ".lwo")
            return std::make_unique<ImportLWO>(scenePath, file);

        MANGO_EXCEPTION("[import3d] Unsupported format '{}' (expected .gltf/.glb/.fbx/.obj/.3ds/.lwo).", ext);
    }

} // namespace mango::import3d
