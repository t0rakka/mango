/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include "configure.hpp"
#include "object.hpp"

namespace mango
{

    class DynamicLibrary : protected NonCopyable
    {
    protected:
        struct DynamicLibraryHandle* m_handle;

    public:
        DynamicLibrary(const std::string& filename);
        ~DynamicLibrary();

        void* address(const std::string& symbol) const;
    };

} // namespace mango
