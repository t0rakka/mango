/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstdarg>
#include <string>
#include <exception>
#include <mango/core/configure.hpp>
#include <mango/core/string.hpp>

namespace mango
{

    // ----------------------------------------------------------------------------
    // Status
    // ----------------------------------------------------------------------------

    struct Status
    {
        std::string info;
        bool success = true;

        operator bool () const
        {
            return success;
        }

        void setError(const char* s) noexcept
        {
            info = s;
            success = false;
        }

        void setError(std::string s) noexcept
        {
            info = std::move(s);
            success = false;
        }

        template <typename... T>
        void setError(fmt::format_string<T...> fmt, T&&... args) noexcept
        {
            info = fmt::format(fmt, std::forward<T>(args)...);
            success = false;
        }
    };

    // ----------------------------------------------------------------------------
    // Exception
    // ----------------------------------------------------------------------------

    class Exception : public std::exception
    {
    protected:
        std::string m_message;
        std::string m_func;
        std::string m_file;
        u32 m_line;

    public:
        Exception(const std::string message, const std::string func, const std::string file, u32 line);
        ~Exception() noexcept;

        const char* what() const noexcept;
        const char* func() const;
        const char* file() const;
        u32 line() const;
    };

    // ----------------------------------------------------------------------------
    // MANGO_EXCEPTION(...)
    // ----------------------------------------------------------------------------

#ifdef MANGO_PLATFORM_WINDOWS
    #define MANGO_EXCEPTION(...) \
        throw mango::Exception(fmt::format(__VA_ARGS__), __FUNCTION__, __FILE__, __LINE__)
#else
    #define MANGO_EXCEPTION(...) \
        throw mango::Exception(fmt::format(__VA_ARGS__), __func__, __FILE__, __LINE__)
#endif

} // namespace mango
