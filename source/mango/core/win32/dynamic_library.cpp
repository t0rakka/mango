/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/dynamic_library.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>

namespace mango
{

    // ----------------------------------------------------------------------------
    // DynamicLibraryHandle
    // ----------------------------------------------------------------------------

    struct DynamicLibraryHandle
    {
        HMODULE handle;

        DynamicLibraryHandle(const std::string& filename)
        {
            u32 mode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
            handle = ::LoadLibraryW(u16_fromBytes(filename).c_str());
            ::SetErrorMode(mode);
            if (!handle)
            {
                MANGO_EXCEPTION("[DynamicLibrary] WIN32 LoadLibrary() failed.");
            }
        }

        ~DynamicLibraryHandle()
        {
            ::FreeLibrary(handle);
        }

        void* address(const std::string& symbol) const
        {
            auto ptr = ::GetProcAddress(handle, symbol.c_str());
            return reinterpret_cast<void*>(ptr);
        }
    };

    // ----------------------------------------------------------------------------
    // DynamicLibrary
    // ----------------------------------------------------------------------------

    DynamicLibrary::DynamicLibrary(const std::string& filename)
    {
        m_handle = std::make_unique<DynamicLibraryHandle>(filename);
    }

    DynamicLibrary::~DynamicLibrary()
    {
    }

    void* DynamicLibrary::address(const std::string& symbol) const
    {
        return m_handle->address(symbol);
    }

} // namespace mango
