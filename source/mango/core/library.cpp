/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/library.hpp>
#include <mango/core/exception.hpp>

#if defined(MANGO_PLATFORM_UNIX)
    #include <dlfcn.h>
#endif

namespace mango
{

    // ----------------------------------------------------------------------------
    // DynamicLibraryHandle
    // ----------------------------------------------------------------------------

#if defined(MANGO_PLATFORM_WINDOWS)

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

#elif defined(MANGO_PLATFORM_UNIX)

    struct DynamicLibraryHandle
    {
        void* handle = nullptr;

        DynamicLibraryHandle(const std::string& filename)
        {
            handle = ::dlopen(filename.c_str(), RTLD_NOW);
            if (!handle)
            {
                MANGO_EXCEPTION("[DynamicLibrary] {}", ::dlerror());
            }
        }

        ~DynamicLibraryHandle()
        {
            if (handle)
            {
                ::dlclose(handle);
            }
        }

        void* address(const std::string& symbol) const
        {
            return ::dlsym(handle, symbol.c_str());
        }
    };

#else

    struct DynamicLibraryHandle
    {
        DynamicLibraryHandle(const std::string& filename)
        {
            MANGO_EXCEPTION("[DynamicLibrary] Not implemented on this platform.");
        }

        ~DynamicLibraryHandle()
        {
        }

        void* address(const std::string& symbol) const
        {
            return nullptr;
        }
    };

#endif


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
