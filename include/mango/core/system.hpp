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
        Debug,
        Verbose
    };

    struct TraceThread
    {
        u32 tid;
        std::string name;

        TraceThread(const std::string& name);
    };

    struct Trace
    {
        struct Data
        {
            u32 tid;
            u64 time0;
            u64 time1;
            std::string category;
            std::string name;
        } data;

        Trace(const std::string& category, const std::string& name);
        ~Trace();

        void stop();
    };

    struct Tracer
    {
        std::mutex mutex;
        Stream* output { nullptr };
        std::vector<TraceThread> threads;
        std::vector<Trace::Data> traces;
        std::vector<Trace::Data> traces_out;
        SerialQueue writer;
        bool comma;
        u32 count;

        Tracer();
        ~Tracer();

        void append(const Trace& trace);

        void start(Stream* stream);
        void stop();
    };

    struct Context
    {
        Timer timer;
        Tracer tracer;

        mutable ThreadPool thread_pool;

        bool print_enable_error   = true;
        bool print_enable_warning = false;
        bool print_enable_info    = false;
        bool print_enable_debug   = false;
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
