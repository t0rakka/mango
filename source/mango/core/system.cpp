/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/system.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/thread.hpp>
#include <mango/core/timer.hpp>
#include <mango/simd/simd.hpp>
#include <sstream>

#if defined(WIN32)

    #include <stdio.h>
    #include <io.h>
    #include <fcntl.h>

    WindowsConsole::WindowsConsole()
    {
        if (AttachConsole(ATTACH_PARENT_PROCESS))
        {
            FILE* fp;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
            freopen_s(&fp, "CONIN$", "r", stdin);
            setvbuf(stdout, nullptr, _IONBF, 0);
            setvbuf(stderr, nullptr, _IONBF, 0);
            m_attached = true;
        }
    }

    WindowsConsole::~WindowsConsole()
    {
        if (m_attached)
        {
            HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
            if (hInput == INVALID_HANDLE_VALUE)
                return;

            // Prepare a KEY_EVENT_RECORD for ENTER key
            INPUT_RECORD ir[2] = {};

            ir[0].EventType = KEY_EVENT;
            ir[0].Event.KeyEvent.bKeyDown = TRUE;
            ir[0].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
            ir[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
            ir[0].Event.KeyEvent.uChar.AsciiChar = '\r';
            ir[0].Event.KeyEvent.dwControlKeyState = 0;

            ir[1] = ir[0];
            ir[1].Event.KeyEvent.bKeyDown = FALSE;

            DWORD written = 0;
            WriteConsoleInput(hInput, ir, 2, &written);

            FreeConsole();
        }
    }

#endif // WIN32

namespace
{
    using namespace mango;

    u32 getThreadID()
    {
        static std::atomic<u32> thread_counter { 0 };
        thread_local u32 id = ++thread_counter;
        return id;
    }

} // namespace

namespace mango
{

    // ----------------------------------------------------------------------------
    // getSystemContext()
    // ----------------------------------------------------------------------------

    Context::Context()
        : timer()
        , tracer()
        , thread_pool(ThreadPool::getHardwareConcurrency())
    {
        // get first ID to main thread
        TraceThread th("MainThread");
    }

    Context::~Context()
    {
    }

    static Context g_context;

    const Context& getSystemContext()
    {
        return g_context;
    }

    // ----------------------------------------------------------------------------
    // getPlatformInfo()
    // ----------------------------------------------------------------------------

    std::string getPlatformInfo()
    {
        std::stringstream info;

        info << "Platform: \"" << MANGO_PLATFORM_NAME << "\", ";

        info << "CPU: \"" << MANGO_CPU_NAME << "\", ";
#ifdef MANGO_CPU_64BIT
        info << "Bits: 64, ";
#else
        info << "Bits: 32, ";
#endif
#ifdef MANGO_LITTLE_ENDIAN
        info << "Endian: LITTLE ";
#else
        info << "Endian: BIG ";
#endif
        info << std::endl;

        return info.str();
    }

    // ----------------------------------------------------------------------------
    // getSystemInfo()
    // ----------------------------------------------------------------------------

    std::string getSystemInfo()
    {
        std::stringstream info;
        info << getPlatformInfo();

        u64 flags = cpu::getFlags();

        const u64 avx512_mask =
            INTEL_AVX512F |
            INTEL_AVX512PFI |
            INTEL_AVX512ERI |
            INTEL_AVX512CDI |
            INTEL_AVX512BW |
            INTEL_AVX512VL |
            INTEL_AVX512DQ |
            INTEL_AVX512IFMA |
            INTEL_AVX512VBMI |
            INTEL_AVX512FP16;

        info << "CPU Features:";
        if (!flags) info << " N/A";
        if (flags & INTEL_AES) info << " AES";
        if (flags & INTEL_CLMUL) info << " CLMUL";
        if (flags & INTEL_FMA3) info << " FMA3";
        if (flags & INTEL_MOVBE) info << " MOVBE";
        if (flags & INTEL_POPCNT) info << " POPCNT";
        if (flags & INTEL_F16C) info << " F16C";
        if (flags & INTEL_RDRAND) info << " RDRAND";
        if (flags & INTEL_CMOV) info << " CMOV";
        if (flags & INTEL_CMPXCHG16B) info << " CMPXCHG16B";
        if (flags & INTEL_FMA4) info << " FMA4";
        if (flags & INTEL_BMI1) info << " BMI1";
        if (flags & INTEL_BMI2) info << " BMI2";
        if (flags & INTEL_SHA) info << " SHA";
        if (flags & INTEL_LZCNT) info << " LZCNT";
        if (flags & ARM_CRC32) info << " CRC32";
        if (flags & ARM_AES) info << " AES";
        if (flags & ARM_SHA1) info << " SHA1";
        if (flags & ARM_SHA2) info << " SHA2";
        if (flags & ARM_PMULL) info << " PMULL";
        info << std::endl;

        info << "SIMD Features:";
        if (flags & INTEL_SSE) info << " SSE";
        if (flags & INTEL_SSE2) info << " SSE2";
        if (flags & INTEL_SSE4_1) info << " SSE4.1";
        if (flags & INTEL_SSE4_2) info << " SSE4.2";
        if (flags & INTEL_SSE4A) info << " SSE4A";
        if (flags & INTEL_AVX) info << " AVX";
        if (flags & INTEL_AVX2) info << " AVX2";
        if (flags & avx512_mask)
        {
            info << " AVX512:";
            if (flags & INTEL_AVX512F) info << "F|";
            if (flags & INTEL_AVX512PFI) info << "PFI|";
            if (flags & INTEL_AVX512ERI) info << "ERI|";
            if (flags & INTEL_AVX512CDI) info << "CDI|";
            if (flags & INTEL_AVX512BW) info << "BW|";
            if (flags & INTEL_AVX512VL) info << "VL|";
            if (flags & INTEL_AVX512DQ) info << "DQ|";
            if (flags & INTEL_AVX512IFMA) info << "IFMA|";
            if (flags & INTEL_AVX512VBMI) info << "VBMI|";
            if (flags & INTEL_AVX512FP16) info << "FP16|";
            info.seekp(-1, std::ios_base::cur);
        }
        if (flags & ARM_NEON) info << " NEON";
        info << std::endl;

        info << "Compiler Flags:";

#if defined(MANGO_ENABLE_SIMD)

    #if defined(MANGO_ENABLE_SSE)
        info << " SSE";
    #endif

    #if defined(MANGO_ENABLE_SSE2)
        info << " SSE2";
    #endif

    #if defined(MANGO_ENABLE_SSE4_1)
        info << " SSE4.1";
    #endif

    #if defined(MANGO_ENABLE_SSE4_2)
        info << " SSE4.2";
    #endif

    #if defined(MANGO_ENABLE_AVX)
        info << " AVX";
    #endif

    #if defined(MANGO_ENABLE_AVX2)
        info << " AVX2";
    #endif

    #if defined(MANGO_ENABLE_AVX512)
        info << " AVX512";
    #endif

    #if defined(MANGO_ENABLE_FMA4)
        info << " FMA4";
    #endif

    #if defined(MANGO_ENABLE_FMA3)
        info << " FMA3";
    #endif

    #if defined(__POPCNT__)
        info << " POPCNT";
    #endif

    #if defined(__F16C__)
        info << " F16C";
    #endif

    #if defined(__BMI__)
        info << " BMI";
    #endif

    #if defined(__BMI2__)
        info << " BMI2";
    #endif

    #if defined(__LZCNT__)
        info << " LZCNT";
    #endif

    #if defined(__AES__)
        info << " AES";
    #endif

    #if defined(__SHA__)
        info << " SHA";
    #endif

    #if defined(MANGO_ENABLE_NEON)
        info << " NEON";
    #endif

    #if defined(__ARM_FEATURE_CRC32)
        info << " CRC32";
    #endif

    #if defined(__ARM_FEATURE_CRYPTO)
        info << " CRYPTO";
    #endif

#else

        info << " N/A";

#endif

        info << std::endl;

        info << "Hardware threads: " << std::thread::hardware_concurrency() << std::endl;
        info << "Build: " << __DATE__ << "  " << __TIME__ << std::endl;

        return info.str();
    }

    // ----------------------------------------------------------------------------
    // print
    // ----------------------------------------------------------------------------

    void printEnable(Print target, bool enable)
    {
        switch (target)
        {
            case Print::Error:
                g_context.print_enable_error = enable;
                break;
            case Print::Warning:
                g_context.print_enable_warning = enable;
                break;
            case Print::Info:
                g_context.print_enable_info = enable;
                break;
            case Print::Debug:
                g_context.print_enable_debug = enable;
                break;
            case Print::Verbose:
                g_context.print_enable_verbose = enable;
                break;
        }
    }

    bool isEnable(Print target)
    {
        bool enable = false;

        switch (target)
        {
            case Print::Error:
                enable = g_context.print_enable_error;
                break;
            case Print::Warning:
                enable = g_context.print_enable_warning;
                break;
            case Print::Info:
                enable = g_context.print_enable_info;
                break;
            case Print::Debug:
                enable = g_context.print_enable_debug;
                break;
            case Print::Verbose:
                enable = g_context.print_enable_verbose;
                break;
        }

        return enable;
    }

    // ----------------------------------------------------------------------------
    // Tracer helper functions
    // ----------------------------------------------------------------------------

    static
    void write(Stream& output, const std::vector<Trace::Data>& traces, bool& comma)
    {
        fmt::memory_buffer buffer;

        for (const auto& trace : traces)
        {
            fmt::format_to(std::back_inserter(buffer),
                "{}\n{{ \"cat\":\"{}\", \"pid\":1, \"tid\":{}, \"ts\":{}, \"dur\":{}, \"ph\":\"X\", \"name\":\"{}\" }}",
                    comma ? "," : "", trace.category, trace.tid, trace.time0, trace.time1 - trace.time0, trace.name);
            comma = true;
        }

        output.write(buffer.data(), buffer.size());
    }

    void startTrace(Stream* stream)
    {
        g_context.tracer.start(stream);
    }

    void stopTrace()
    {
        g_context.tracer.stop();
    }

    // ----------------------------------------------------------------------------
    // TraceThread
    // ----------------------------------------------------------------------------

    TraceThread::TraceThread(const std::string& name)
        : tid(getThreadID())
        , name(name)
    {
        std::lock_guard<std::mutex> lock(g_context.tracer.mutex);
        g_context.tracer.threads.push_back(*this);
    }

    // ----------------------------------------------------------------------------
    // Trace
    // ----------------------------------------------------------------------------

    Trace::Trace(const std::string& category, const std::string& name)
    {
        data.tid = getThreadID();
        data.time0 = Time::us();
        data.category = category;
        data.name = name;
    }

    Trace::~Trace()
    {
        stop();
    }

    void Trace::stop()
    {
        if (!stopped)
        {
            data.time1 = Time::us();
            g_context.tracer.append(*this);
            stopped = true;
        }
    }

    // ----------------------------------------------------------------------------
    // Tracer
    // ----------------------------------------------------------------------------

    Tracer::Tracer()
    {
    }

    Tracer::~Tracer()
    {
        stop();
    }

    void Tracer::start(Stream* stream)
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (output)
        {
            // already running a trace
            return;
        }

        output = stream;

        traces.clear();

        comma = false;
        count = 0;

        // write header
        std::string s = fmt::format("{{\n\"traceEvents\": [");
        output->write(s.data(), s.length());
    }

    void Tracer::stop()
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (!output)
        {
            // not running a trace
            return;
        }

        writer.wait();

        fmt::memory_buffer buffer;

        for (const auto& th : threads)
        {
            fmt::format_to(std::back_inserter(buffer),
                "{}\n{{ \"name\":\"thread_name\", \"ph\":\"M\", \"pid\":1, \"tid\":{}, \"args\": {{\"name\":\"{}\" }} }}",
                    comma ? "," : "", th.tid, th.name);
            comma = true;
        }

        threads.clear();
        output->write(buffer.data(), buffer.size());

        write(*output, traces, comma);
        traces.clear();

        // write footer
        std::string s = fmt::format("\n]\n}}\n");
        output->write(s.data(), s.length());

        output = nullptr;
    }

    void Tracer::append(const Trace& trace)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (output)
        {
            traces.push_back(trace.data);

            if (traces.size() >= 1024)
            {
                writer.wait();
                std::swap(traces_out, traces);

                writer.enqueue([&]
                {
                    write(*output, traces_out, comma);
                    traces_out.clear();
                });
            }
        }
    }

    // ----------------------------------------------------------------------------
    // CommandLine
    // ----------------------------------------------------------------------------

    CommandLine::Handler* CommandLine::findHandler(std::string_view name) const
    {
        for (Handler& handler : m_handlers)
        {
            if (handler.name == name)
            {
                return &handler;
            }
        }

        return nullptr;
    }

    void CommandLine::registerHandler(Handler handler) const
    {
        if (handler.name.size() < 2 || handler.name[0] != '-' || handler.name[1] != '-')
        {
            MANGO_EXCEPTION("CommandLine: '{}' must start with '--'.", handler.name);
        }

        if (findHandler(handler.name))
        {
            MANGO_EXCEPTION("CommandLine: duplicate option '{}'.", handler.name);
        }

        if (handler.takesValue)
        {
            switch (handler.valueType)
            {
                case ValueType::String:
                    if (!handler.valueAction)
                    {
                        MANGO_EXCEPTION("CommandLine: option '{}' has no handler.", handler.name);
                    }
                    break;

                case ValueType::Int:
                    if (!handler.intAction)
                    {
                        MANGO_EXCEPTION("CommandLine: option '{}' has no handler.", handler.name);
                    }
                    break;

                case ValueType::Float:
                    if (!handler.floatAction)
                    {
                        MANGO_EXCEPTION("CommandLine: option '{}' has no handler.", handler.name);
                    }
                    break;

                case ValueType::Size2D:
                    if (!handler.size2DAction)
                    {
                        MANGO_EXCEPTION("CommandLine: option '{}' has no handler.", handler.name);
                    }
                    break;
            }
        }
        else if (!handler.action)
        {
            MANGO_EXCEPTION("CommandLine: flag '{}' has no handler.", handler.name);
        }

        m_handlers.push_back(std::move(handler));
    }

    void CommandLine::addPositional(std::string_view token) const
    {
        m_positionals.push_back(token);

        if (m_positional)
        {
            m_positional(token);
        }
    }

    CommandLine& CommandLine::flag(std::string_view name, Action action) const
    {
        return flag(name, {}, std::move(action));
    }

    CommandLine& CommandLine::flag(std::string_view name, std::string_view help, Action action) const
    {
        Handler handler;
        handler.name = std::string(name);
        handler.help = std::string(help);
        handler.takesValue = false;
        handler.action = std::move(action);
        registerHandler(std::move(handler));
        return const_cast<CommandLine&>(*this);
    }

    CommandLine& CommandLine::option(std::string_view name, ValueAction action) const
    {
        return option(name, {}, std::move(action));
    }

    CommandLine& CommandLine::option(std::string_view name, std::string_view help, ValueAction action) const
    {
        Handler handler;
        handler.name = std::string(name);
        handler.help = std::string(help);
        handler.takesValue = true;
        handler.valueType = ValueType::String;
        handler.valueAction = std::move(action);
        registerHandler(std::move(handler));
        return const_cast<CommandLine&>(*this);
    }

    CommandLine& CommandLine::optionInt(std::string_view name, IntAction action) const
    {
        return optionInt(name, {}, std::move(action));
    }

    CommandLine& CommandLine::optionInt(std::string_view name, std::string_view help, IntAction action) const
    {
        Handler handler;
        handler.name = std::string(name);
        handler.help = std::string(help);
        handler.takesValue = true;
        handler.valueType = ValueType::Int;
        handler.intAction = std::move(action);
        registerHandler(std::move(handler));
        return const_cast<CommandLine&>(*this);
    }

    CommandLine& CommandLine::optionFloat(std::string_view name, FloatAction action) const
    {
        return optionFloat(name, {}, std::move(action));
    }

    CommandLine& CommandLine::optionFloat(std::string_view name, std::string_view help, FloatAction action) const
    {
        Handler handler;
        handler.name = std::string(name);
        handler.help = std::string(help);
        handler.takesValue = true;
        handler.valueType = ValueType::Float;
        handler.floatAction = std::move(action);
        registerHandler(std::move(handler));
        return const_cast<CommandLine&>(*this);
    }

    CommandLine& CommandLine::option2D(std::string_view name, Size2DAction action) const
    {
        return option2D(name, {}, std::move(action));
    }

    CommandLine& CommandLine::option2D(std::string_view name, std::string_view help, Size2DAction action) const
    {
        Handler handler;
        handler.name = std::string(name);
        handler.help = std::string(help);
        handler.takesValue = true;
        handler.valueType = ValueType::Size2D;
        handler.size2DAction = std::move(action);
        registerHandler(std::move(handler));
        return const_cast<CommandLine&>(*this);
    }

    CommandLine& CommandLine::positional(ValueAction action) const
    {
        if (!action)
        {
            MANGO_EXCEPTION("CommandLine: positional handler must not be empty.");
        }

        m_positional = std::move(action);
        return const_cast<CommandLine&>(*this);
    }

    CommandLine& CommandLine::requirePositional(std::string_view name) const
    {
        m_requiredPositionals.emplace_back(name);
        return const_cast<CommandLine&>(*this);
    }

    CommandLine& CommandLine::usage(std::string_view text) const
    {
        m_usage = std::string(text);
        return const_cast<CommandLine&>(*this);
    }

    bool CommandLine::parse() const
    {
        m_positionals.clear();
        m_programName = m_argv.empty() ? std::string{} : std::string(m_argv[0]);

        for (size_t i = 1; i < m_argv.size(); ++i)
        {
            const std::string_view arg = m_argv[i];

            if (arg == "--")
            {
                for (++i; i < m_argv.size(); ++i)
                {
                    addPositional(m_argv[i]);
                }
                break;
            }

            if (arg == "--help")
            {
                printHelp();
                return false;
            }

            if (!arg.starts_with("--"))
            {
                if (arg.size() > 1 && arg[0] == '-')
                {
                    printLine("Unknown option: {}", arg);
                    printLine("Try '--help'.");
                    return false;
                }

                addPositional(arg);
                continue;
            }

            const size_t eq = arg.find('=');
            const bool hasInlineValue = (eq != std::string_view::npos);
            const std::string_view name = hasInlineValue ? arg.substr(0, eq) : arg;

            Handler* handler = findHandler(name);
            if (!handler)
            {
                printLine("Unknown option: {}", arg);
                printLine("Try '--help'.");
                return false;
            }

            if (handler->takesValue)
            {
                std::string_view value;

                if (hasInlineValue)
                {
                    value = arg.substr(eq + 1);
                }
                else
                {
                    if (i + 1 >= m_argv.size())
                    {
                        printLine("Missing value for option: {}", name);
                        printLine("Try '--help'.");
                        return false;
                    }

                    ++i;
                    value = m_argv[i];
                }

                switch (handler->valueType)
                {
                    case ValueType::Int:
                    {
                        int parsed = 0;

                        if (!tryParseInt(value, parsed))
                        {
                            printLine("Invalid value for option: {} (expected integer)", name);
                            printLine("Try '--help'.");
                            return false;
                        }

                        handler->intAction(parsed);
                        break;
                    }

                    case ValueType::Float:
                    {
                        float parsed = 0.0f;

                        if (!tryParseFloat(value, parsed))
                        {
                            printLine("Invalid value for option: {} (expected number)", name);
                            printLine("Try '--help'.");
                            return false;
                        }

                        handler->floatAction(parsed);
                        break;
                    }

                    case ValueType::Size2D:
                    {
                        int width = 0;
                        int height = 0;

                        if (!parseSize2D(value, width, height))
                        {
                            printLine("Invalid value for option: {} (expected WxH)", name);
                            printLine("Try '--help'.");
                            return false;
                        }

                        handler->size2DAction(width, height);
                        break;
                    }

                    case ValueType::String:
                        handler->valueAction(value);
                        break;
                }
            }
            else
            {
                if (hasInlineValue)
                {
                    printLine("Option '{}' does not take a value.", name);
                    return false;
                }

                handler->action();
            }
        }

        if (m_positionals.size() < m_requiredPositionals.size())
        {
            printLine("Missing required argument: {}",
                m_requiredPositionals[m_positionals.size()]);
            return false;
        }

        return true;
    }

    void CommandLine::printHelp() const
    {
        if (!m_programName.empty() || !m_usage.empty())
        {
            if (!m_programName.empty() && !m_usage.empty())
            {
                printLine("Usage: {} {}", m_programName, m_usage);
            }
            else if (!m_programName.empty())
            {
                printLine("Usage: {}", m_programName);
            }
            else
            {
                printLine("Usage: {}", m_usage);
            }
            printLine("");
        }

        if (!m_requiredPositionals.empty())
        {
            printLine("Arguments:");
            for (const std::string& name : m_requiredPositionals)
            {
                printLine("  <{}>  (required)", name);
            }
            printLine("");
        }

        printLine("Options:");

        auto optionValueSuffix = [](ValueType type) -> std::string_view
        {
            switch (type)
            {
                case ValueType::Int:
                    return " [=<integer>]";
                case ValueType::Float:
                    return " [=<number>]";
                case ValueType::Size2D:
                    return " [=<WxH>]";
                case ValueType::String:
                    break;
            }

            return " [=<value>]";
        };

        size_t nameWidth = sizeof("--help") - 1;
        for (const Handler& handler : m_handlers)
        {
            size_t width = handler.name.size();
            if (handler.takesValue)
            {
                width += optionValueSuffix(handler.valueType).size();
            }
            if (width > nameWidth)
            {
                nameWidth = width;
            }
        }

        auto printEntry = [&](std::string_view name, std::string_view help)
        {
            if (help.empty())
            {
                printLine("  {}", name);
            }
            else
            {
                printLine("  {:<{}}  {}", name, nameWidth, help);
            }
        };

        for (const Handler& handler : m_handlers)
        {
            std::string name = handler.name;
            if (handler.takesValue)
            {
                name += optionValueSuffix(handler.valueType);
            }
            printEntry(name, handler.help);
        }

        printEntry("--help", "Show this help and exit");
    }

    const std::vector<std::string_view>& CommandLine::positionals() const
    {
        return m_positionals;
    }

    // ----------------------------------------------------------------------------
    // Exception
    // ----------------------------------------------------------------------------

    Exception::Exception(const std::string message, const std::string func, const std::string file, u32 line)
        : m_message(message)
        , m_func(func)
        , m_file(file)
        , m_line(line)
    {
    }

    Exception::~Exception() noexcept
    {
    }

    const char* Exception::what() const noexcept
    {
        return m_message.c_str();
    }

    const char* Exception::func() const
    {
        return m_func.c_str();
    }

    const char* Exception::file() const
    {
        return m_file.c_str();
    }

    u32 Exception::line() const
    {
        return m_line;
    }

} // namespace mango
