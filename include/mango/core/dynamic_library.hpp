/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
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

#if defined(MANGO_PLATFORM_WINDOWS)

        HMODULE m_handle;

#elif defined(MANGO_PLATFORM_UNIX)

        void* m_handle;

#endif

    public:
        DynamicLibrary(const std::string& filename);
        ~DynamicLibrary();

        void* address(const std::string& symbol) const;
    };

} // namespace mango
