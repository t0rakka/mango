/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <mango/core/configure.hpp>
#include <mango/core/thread.hpp>
#include <mango/core/timer.hpp>
#include <mango/core/string.hpp>
#include <mango/core/stream.hpp>
#include <mango/core/print.hpp>

namespace mango
{

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

    using CommandLine = std::vector<std::string_view>;

} // namespace mango

// ----------------------------------------------------------------------------------
// mangoMain()
// ----------------------------------------------------------------------------------

#if defined(WIN32)

    class WindowsConsole
    {
    private:
        bool m_attached = false;

    public:
        WindowsConsole();
        ~WindowsConsole();

        WindowsConsole(const WindowsConsole&) = delete;
        WindowsConsole& operator=(const WindowsConsole&) = delete;
    };

#endif // WIN32

#if defined(MANGO_IMPLEMENT_MAIN)

    // This will be called from platform specific main function below;
    // client must implement this function and return 0 on success
    int mangoMain(const mango::CommandLine& commands);

    #if defined(WIN32)

        int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
        {
            MANGO_UNREFERENCED(hInstance);
            MANGO_UNREFERENCED(hPrevInstance);
            MANGO_UNREFERENCED(lpCmdLine);
            MANGO_UNREFERENCED(nCmdShow);

            mango::CommandLine commands(__argv + 0, __argv + __argc);
            WindowsConsole console;
            return mangoMain(commands);
        }

    #else

        int main(int argc, const char** argv)
        {
            mango::CommandLine commands(argv + 0, argv + argc);
            return mangoMain(commands);
        }

    #endif

#endif // defined(MANGO_IMPLEMENT_MAIN)
