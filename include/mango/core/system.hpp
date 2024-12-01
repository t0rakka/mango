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

    // ----------------------------------------------------------------------------------
    // print
    // ----------------------------------------------------------------------------------

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
    // printLine
    // ----------------------------------------------------------------------------------

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
