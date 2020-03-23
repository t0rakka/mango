/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <exception>
#include "configure.hpp"
#include "string.hpp"

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

        void setError(const std::string& error)
        {
            info = error;
            success = false;
        }

        void setError(const char* format, ...)
        {
            va_list args;
            va_start(args, format);
            info = makeString(format, args);
            success = false;
            va_end(args);
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
        int m_line;

    public:
        Exception(const std::string message, const std::string func, const std::string file, int line)
            : m_message(message)
            , m_func(func)
            , m_file(file)
            , m_line(line)
        {
        }

        ~Exception() noexcept
        {
        }

        const char* what() const noexcept
        {
            return m_message.c_str();
        }

        const char* func() const
        {
            return m_func.c_str();
        }

        const char* file() const
        {
            return m_file.c_str();
        }

        int line() const
        {
            return m_line;
        }
    };

	// ----------------------------------------------------------------------------
	// MANGO_EXCEPTION(...)
	// ----------------------------------------------------------------------------

#ifdef MANGO_PLATFORM_WINDOWS
    #define MANGO_EXCEPTION(...) \
        throw mango::Exception(mango::makeString(__VA_ARGS__), __FUNCTION__, __FILE__, __LINE__)
#else
    #define MANGO_EXCEPTION(...) \
        throw mango::Exception(mango::makeString(__VA_ARGS__), __func__, __FILE__, __LINE__)
#endif

} // namespace mango
