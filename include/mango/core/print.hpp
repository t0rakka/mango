/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/core/configure.hpp>
#include <mango/core/string.hpp>

namespace mango
{

    enum class Print
    {
        Error,
        Warning,
        Info,
        Debug,
        Verbose
    };

    void printEnable(Print target, bool enable);
    bool isEnable(Print target);

    // ----------------------------------------------------------------------------------
    // print()
    // ----------------------------------------------------------------------------------

    static inline
    void print(Print target, const std::string& text)
    {
        if (isEnable(target))
        {
            std::printf("%s", text.c_str());
        }
    }

    static inline
    void print(Print target, int indent, const std::string& text)
    {
        if (isEnable(target))
        {
            std::printf("%*s%s", indent, "",  text.c_str());
        }
    }

    template <typename... T>
    static inline
    void print(Print target, fmt::format_string<T...> fmt, T&&... args)
    {
        if (isEnable(target))
        {
            std::printf("%s", fmt::vformat(fmt.str, fmt::vargs<T...>{{args...}}).c_str());
        }
    }

    template <typename... T>
    static inline
    void print(Print target, int indent, fmt::format_string<T...> fmt, T&&... args)
    {
        if (isEnable(target))
        {
            std::printf("%*s%s", indent, "", 
                fmt::vformat(fmt.str, fmt::vargs<T...>{{args...}}).c_str());
        }
    }

    static inline
    void print(const std::string& text)
    {
        print(Print::Verbose, text);
    }

    static inline
    void print(int indent, const std::string& text)
    {
        print(Print::Verbose, indent, text);
    }

    template <typename... T>
    static inline
    void print(fmt::format_string<T...> fmt, T&&... args)
    {
        print(Print::Verbose, fmt, std::forward<T>(args)...);
    }

    template <typename... T>
    static inline
    void print(int indent, fmt::format_string<T...> fmt, T&&... args)
    {
        print(Print::Verbose, indent, fmt, std::forward<T>(args)...);
    }

    // ----------------------------------------------------------------------------------
    // printLine()
    // ----------------------------------------------------------------------------------

    static inline
    void printLine(Print target, const std::string& text)
    {
        if (isEnable(target))
        {
            std::printf("%s\n", text.c_str());
        }
    }

    static inline
    void printLine(Print target, int indent, const std::string& text)
    {
        if (isEnable(target))
        {
            std::printf("%*s%s\n", indent, "", text.c_str());
        }
    }

    template <typename... T>
    static inline
    void printLine(Print target, fmt::format_string<T...> fmt, T&&... args)
    {
        if (isEnable(target))
        {
            std::printf("%s\n", fmt::vformat(fmt.str, fmt::vargs<T...>{{args...}}).c_str());
        }
    }

    template <typename... T>
    static inline
    void printLine(Print target, int indent, fmt::format_string<T...> fmt, T&&... args)
    {
        if (isEnable(target))
        {
            std::printf("%*s%s\n", indent, "", 
                fmt::vformat(fmt.str, fmt::vargs<T...>{{args...}}).c_str());
        }
    }

    static inline
    void printLine(const std::string& text)
    {
        printLine(Print::Verbose, text);
    }

    static inline
    void printLine(int indent, const std::string& text)
    {
        printLine(Print::Verbose, indent, text);
    }

    template <typename... T>
    static inline
    void printLine(fmt::format_string<T...> fmt, T&&... args)
    {
        printLine(Print::Verbose, fmt, std::forward<T>(args)...);
    }

    template <typename... T>
    static inline
    void printLine(int indent, fmt::format_string<T...> fmt, T&&... args)
    {
        printLine(Print::Verbose, indent, fmt, std::forward<T>(args)...);
    }

} // namespace mango
