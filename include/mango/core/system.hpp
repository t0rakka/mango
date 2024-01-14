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
#include <mango/core/stream.hpp>

namespace mango
{

    enum class Print
    {
        Error,
        Warning,
        Info,
        Verbose
    };

    struct TraceThread
    {
        std::string name;
        u32 tid;

        TraceThread(const std::string& name);
    };

    struct Trace
    {
        u32 tid;
        u64 time0;
        u64 time1;
        std::string_view category;
        std::string_view name;

        Trace(std::string_view category, std::string_view name);
        ~Trace();
    };

    struct Tracer
    {
        std::mutex mutex;
        fmt::memory_buffer buffer;
        Stream* output { nullptr };
        std::vector<TraceThread> threads;

        Tracer();
        ~Tracer();

        void start(Stream* stream);
        void stop();

        void append(const Trace& trace);
    };

    struct Context
    {
        Timer timer;
        Tracer tracer;

        mutable ThreadPool thread_pool;

        bool print_enable_error   = true;
        bool print_enable_warning = false;
        bool print_enable_info    = false;
        bool print_enable_verbose = true;

        Context();
        ~Context();
    };

    const Context& getSystemContext();

    std::string getPlatformInfo();
    std::string getSystemInfo();

    void startTrace(Stream* stream);
    void stopTrace();

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
