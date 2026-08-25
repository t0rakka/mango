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
    // CommandLine
    // ------------------------------------------------------------------------------

    class CommandLine
    {
    public:
        using Action = std::function<void()>;
        using ValueAction = std::function<void(std::string_view)>;
        using IntAction = std::function<void(int value)>;
        using FloatAction = std::function<void(float value)>;
        using Size2DAction = std::function<void(int width, int height)>;

        CommandLine() = default;

        CommandLine(int argc, const char** argv)
            : m_argv(argv, argv + argc)
        {
        }

        template <typename Iterator>
        CommandLine(Iterator first, Iterator last)
            : m_argv(first, last)
        {
        }

        explicit CommandLine(const std::vector<std::string>& strings)
        {
            m_argv.reserve(strings.size());
            for (const std::string& s : strings)
            {
                m_argv.push_back(s);
            }
        }

        CommandLine(const CommandLine&) = delete;
        CommandLine& operator=(const CommandLine&) = delete;
        CommandLine(CommandLine&&) noexcept = default;
        CommandLine& operator=(CommandLine&&) noexcept = default;

        size_t size() const { return m_argv.size(); }
        bool empty() const { return m_argv.empty(); }

        std::string_view operator [] (size_t index) const { return m_argv[index]; }

        // Boolean / switch flag: --name
        CommandLine& flag(std::string_view name, Action action) const;
        CommandLine& flag(std::string_view name, std::string_view help, Action action) const;

        // Option that consumes the next argument: --name value  (or --name=value)
        CommandLine& option(std::string_view name, ValueAction action) const;
        CommandLine& option(std::string_view name, std::string_view help, ValueAction action) const;

        CommandLine& optionInt(std::string_view name, IntAction action) const;
        CommandLine& optionInt(std::string_view name, std::string_view help, IntAction action) const;

        CommandLine& optionFloat(std::string_view name, FloatAction action) const;
        CommandLine& optionFloat(std::string_view name, std::string_view help, FloatAction action) const;

        // Option with WxH value: --name 4x4  (or --name=4x4, 4,4)
        CommandLine& option2D(std::string_view name, Size2DAction action) const;
        CommandLine& option2D(std::string_view name, std::string_view help, Size2DAction action) const;

        // Called for each non-option token (and for tokens after bare "--").
        // Only tokens starting with "--" are treated as options. A lone "-" is positional.
        // Other tokens starting with "-" are rejected (likely a mistyped option); pass
        // literal paths like "-file.png" after "--".
        // Tokens are always collected in positionals() as well.
        CommandLine& positional(ValueAction action) const;

        // Declare a required positional (order matters). Example:
        //   commands.requirePositional("filename");
        // parse() fails if fewer positionals were given; --help lists them.
        CommandLine& requirePositional(std::string_view name) const;

        // Optional usage suffix printed after argv[0] (commands[0]) by --help.
        // Example: commands.usage("[options]");
        CommandLine& usage(std::string_view text) const;

        // Walk m_argv[1..). Returns true on success.
        // --help / errors print unconditionally and return false.
        bool parse() const;

        void printHelp() const;

        const std::vector<std::string_view>& positionals() const;

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

        Handler* findHandler(std::string_view name) const;
        void registerHandler(Handler handler) const;
        void addPositional(std::string_view token) const;

        std::vector<std::string_view> m_argv;

        mutable std::vector<Handler> m_handlers;
        mutable std::vector<std::string> m_requiredPositionals;
        mutable ValueAction m_positional;
        mutable std::string m_usage;
        mutable std::string m_programName;
        mutable std::vector<std::string_view> m_positionals;
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

            mango::CommandLine commands(__argc, const_cast<const char**>(__argv));
            WindowsConsole console;
            return mangoMain(commands);
        }

    #else

        int main(int argc, const char** argv)
        {
            mango::CommandLine commands(argc, argv);
            return mangoMain(commands);
        }

    #endif

#endif // defined(MANGO_IMPLEMENT_MAIN)
