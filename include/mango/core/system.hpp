/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <mango/core/configure.hpp>
#include <mango/core/thread.hpp>
#include <mango/core/timer.hpp>
#include <mango/core/string.hpp>

namespace mango
{

    enum class Print
    {
        Error,
        Warning,
        Info,
        Verbose
    };

    struct Context
    {
        mutable ThreadPool thread_pool;

        Timer timer;

        bool print_enable_error = true;
        bool print_enable_warning = false;
        bool print_enable_info = false;
        bool print_enable_verbose = true;

        Context();
        ~Context();
    };

    const Context& getSystemContext();

    std::string getPlatformInfo();
    std::string getSystemInfo();

    void printEnable(Print target, bool enable);
    bool isEnable(Print target);

    template <typename... T>
    void printLine(Print target, T... s)
    {
        if (isEnable(target))
        {
            fmt::print(std::forward<T>(s)...);
            fmt::print("\n");
        }
    }

    template <typename... T>
    void printLine(Print target, int indent, T... s)
    {
        if (isEnable(target))
        {
            fmt::print("{:{}}", "", indent);
            fmt::print(std::forward<T>(s)...);
            fmt::print("\n");
        }
    }

    template <typename... T>
    void printLine(T... s)
    {
        printLine(Print::Verbose, std::forward<T>(s)...);
    }

    template <typename... T>
    void printLine(int indent, T... s)
    {
        printLine(Print::Verbose, indent, std::forward<T>(s)...);
    }

} // namespace mango
