/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/import3d/import.hpp>

namespace
{
    using namespace mango;
    using namespace mango::import3d;

    template <typename Importer>
    std::unique_ptr<Scene> makeImporter(const filesystem::Path& path, const std::string& dir,
                                        const std::string& file)
    {
        if (dir.empty())
            return std::make_unique<Importer>(path, file);

        filesystem::Path scenePath(path, dir);
        return std::make_unique<Importer>(scenePath, file);
    }
}

namespace mango::import3d
{

    std::unique_ptr<Scene> importScene(const filesystem::Path& path, const std::string& filename)
    {
        const std::string ext = toLower(filesystem::getExtension(filename));
        const std::string dir = filesystem::getPath(filename);
        const std::string file = filesystem::removePath(filename);

        if (ext == ".gltf" || ext == ".glb")
            return makeImporter<ImportGLTF>(path, dir, file);
        if (ext == ".fbx")
            return makeImporter<ImportFBX>(path, dir, file);
        if (ext == ".obj")
            return makeImporter<ImportOBJ>(path, dir, file);
        if (ext == ".3ds")
            return makeImporter<Import3DS>(path, dir, file);
        if (ext == ".lwo")
            return makeImporter<ImportLWO>(path, dir, file);

        MANGO_EXCEPTION("[import3d] Unsupported format '{}' (expected .gltf/.glb/.fbx/.obj/.3ds/.lwo).", ext);
    }

} // namespace mango::import3d
