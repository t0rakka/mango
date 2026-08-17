/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <functional>
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
        bool stopped = false;

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

    // ------------------------------------------------------------------------------
    // CommandLineParser
    // ------------------------------------------------------------------------------

    using CommandLine = std::vector<std::string_view>;

    class CommandLineParser
    {
    public:
        using Action = std::function<void()>;
        using ValueAction = std::function<void(std::string_view)>;
        using IntAction = std::function<void(int value)>;
        using FloatAction = std::function<void(float value)>;
        using Size2DAction = std::function<void(int width, int height)>;

        // Boolean / switch flag: --name
        CommandLineParser& flag(std::string_view name, Action action);
        CommandLineParser& flag(std::string_view name, std::string_view help, Action action);

        // Option that consumes the next argument: --name value  (or --name=value)
        CommandLineParser& option(std::string_view name, ValueAction action);
        CommandLineParser& option(std::string_view name, std::string_view help, ValueAction action);

        CommandLineParser& optionInt(std::string_view name, IntAction action);
        CommandLineParser& optionInt(std::string_view name, std::string_view help, IntAction action);

        CommandLineParser& optionFloat(std::string_view name, FloatAction action);
        CommandLineParser& optionFloat(std::string_view name, std::string_view help, FloatAction action);

        // Option with WxH value: --name 4x4  (or --name=4x4, 4,4)
        CommandLineParser& option2D(std::string_view name, Size2DAction action);
        CommandLineParser& option2D(std::string_view name, std::string_view help, Size2DAction action);

        // Called for each non-option token (and for tokens after bare "--").
        // Only tokens starting with "--" are treated as options. A lone "-" is positional.
        // Other tokens starting with "-" are rejected (likely a mistyped option); pass
        // literal paths like "-file.png" after "--".
        // Tokens are always collected in positionals() as well.
        CommandLineParser& positional(ValueAction action);

        // Declare a required positional (order matters). Example:
        //   parser.requirePositional("filename");
        // parse() fails if fewer positionals were given; --help lists them.
        CommandLineParser& requirePositional(std::string_view name);

        // Optional usage suffix printed after argv[0] (commands[0]) by --help.
        // Example: parser.usage("[options]");
        CommandLineParser& usage(std::string_view text);

        // Walk commands[1..). Returns true on success.
        // --help / errors print unconditionally and return false.
        bool parse(const CommandLine& commands);
        bool parse(int argc, const char** argv);

        void printHelp() const;

        const std::vector<std::string_view>& positionals() const { return m_positionals; }

    private:
        enum class ValueType
        {
            String,
            Int,
            Float,
            Size2D,
        };

        struct Handler
        {
            std::string name;
            std::string help;
            bool takesValue = false;
            ValueType valueType = ValueType::String;
            Action action;
            ValueAction valueAction;
            IntAction intAction;
            FloatAction floatAction;
            Size2DAction size2DAction;
        };

        Handler* findHandler(std::string_view name);
        void registerHandler(Handler handler);
        void addPositional(std::string_view token);

        std::vector<Handler> m_handlers;
        std::vector<std::string> m_requiredPositionals;
        ValueAction m_positional;
        std::string m_usage;
        std::string m_programName;

        std::vector<std::string_view> m_positionals;
    };

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
