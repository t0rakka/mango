/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/dynamic_library.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>

namespace mango
{

    DynamicLibrary::DynamicLibrary(const std::string& filename)
    {
        uint32 mode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        m_handle = ::LoadLibraryW(u16_fromBytes(filename).c_str());
        ::SetErrorMode(mode);
        if (!m_handle)
        {
            MANGO_EXCEPTION("LoadLibrary failed.");
        }
    }

    DynamicLibrary::~DynamicLibrary()
    {
        ::FreeLibrary(m_handle);
    }

    void* DynamicLibrary::address(const std::string& symbol) const
    {
        void* ptr = ::GetProcAddress(m_handle, symbol.c_str());
        return ptr;
    }

} // namespace mango
