/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/import3d/import_fbx.hpp>

/*
    Autodesk FBX importer
*/

namespace mango::import3d
{

    ImportFBX::ImportFBX(const filesystem::Path& path, const std::string& filename)
    {
        filesystem::File file(path, filename);

        // TODO
        MANGO_UNREFERENCED(file);
    }

} // namespace mango::import3d
