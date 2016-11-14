/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include "../core/configure.hpp"
#include "mapper.hpp"

namespace mango
{

    class Path : public FileIndex, public Mapper
    {
    public:
        Path(const std::string& pathname, const std::string& password = "");
        Path(const Path& path, const std::string& filename, const std::string& password = "");
        ~Path();

        void updateIndex();
        const std::string& pathname() const;
    };

} // namespace mango
