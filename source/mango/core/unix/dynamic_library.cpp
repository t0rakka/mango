/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/dynamic_library.hpp>
#include <mango/core/exception.hpp>
#include <dlfcn.h>

namespace mango
{

    DynamicLibrary::DynamicLibrary(const std::string& filename)
    {
        m_handle = ::dlopen(filename.c_str(), RTLD_NOW);
        if (!m_handle)
        {
            MANGO_EXCEPTION("[DynamicLibrary] %s", ::dlerror());
        }
    }

    DynamicLibrary::~DynamicLibrary()
    {
        ::dlclose(m_handle);
    }

    void* DynamicLibrary::address(const std::string& symbol) const
    {
        return ::dlsym(m_handle, symbol.c_str());
    }

} // namespace mango
