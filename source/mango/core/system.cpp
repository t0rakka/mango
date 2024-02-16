/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/system.hpp>
#include <mango/core/cpuinfo.hpp>
#include <mango/core/thread.hpp>
#include <mango/core/timer.hpp>
#include <mango/simd/simd.hpp>
#include <sstream>

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

        u64 flags = getCPUFlags();

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
        if (flags & INTEL_XOP) info << " XOP";
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
        if (flags & INTEL_3DNOW) info << " 3DNOW";
        if (flags & INTEL_3DNOW_EXT) info << " 3DNOW+";
        if (flags & INTEL_MMX) info << " MMX";
        if (flags & INTEL_MMX_PLUS) info << " MMX+";
        if (flags & INTEL_SSE) info << " SSE";
        if (flags & INTEL_SSE2) info << " SSE2";
        if (flags & INTEL_SSE3) info << " SSE3";
        if (flags & INTEL_SSSE3) info << " SSSE3";
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

    #if defined(MANGO_ENABLE_SSE3)
        info << " SSE3";
    #endif

    #if defined(MANGO_ENABLE_SSSE3)
        info << " SSSE3";
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

    #if defined(MANGO_ENABLE_XOP)
        info << " XOP";
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
    // Trace
    // ----------------------------------------------------------------------------

    static
    constexpr u32 trace_pool_offset = 0x10000;

    static
    void write(Stream& output, const std::vector<Trace::Data>& traces, bool& comma)
    {
        fmt::memory_buffer buffer;

        for (const auto& trace : traces)
        {
            u32 offset = 0;
            if (trace.category == "Task")
                offset = trace_pool_offset;

            fmt::format_to(std::back_inserter(buffer),
                "{}\n{{ \"cat\":\"{}\", \"pid\":1, \"tid\":{}, \"ts\":{}, \"dur\":{}, \"ph\":\"X\", \"name\":\"{}\" }}",
                    comma ? "," : "", trace.category, trace.tid + offset, trace.time0, trace.time1 - trace.time0, trace.name);

            comma = true;
        }

        output.write(buffer.data(), buffer.size());
    }

    TraceThread::TraceThread(const std::string& name)
        : tid(getThreadID())
        , name(name)
    {
        std::unique_lock<std::mutex> lock(g_context.tracer.mutex);
        g_context.tracer.threads.push_back(*this);
    }

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
        data.time1 = Time::us();
        g_context.tracer.append(*this);
    }

    Tracer::Tracer()
    {
    }

    Tracer::~Tracer()
    {
        stop();
    }

    void Tracer::start(Stream* stream)
    {
        std::unique_lock<std::mutex> lock(mutex);

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
        std::unique_lock<std::mutex> lock(mutex);

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

            fmt::format_to(std::back_inserter(buffer),
                "{}\n{{ \"name\":\"thread_name\", \"ph\":\"M\", \"pid\":1, \"tid\":{}, \"args\": {{\"name\":\"{}\" }} }}",
                    comma ? "," : "", th.tid + trace_pool_offset, th.name + " tasks:");
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
        std::unique_lock<std::mutex> lock(mutex);
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

    void startTrace(Stream* stream)
    {
        g_context.tracer.start(stream);
    }

    void stopTrace()
    {
        g_context.tracer.stop();
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
