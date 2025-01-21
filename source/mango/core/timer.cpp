/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <ctime>
#include <mango/core/system.hpp>
#include <mango/core/timer.hpp>

#ifdef MANGO_PLATFORM_WINDOWS

namespace
{

    // https://blat-blatnik.github.io/computerBear/making-accurate-sleep-function/
    void preciseSleep(double seconds)
    {
        static double estimate = 5e-3;
        static double mean = 5e-3;
        static double m2 = 0;
        static int64_t count = 1;

        while (seconds > estimate)
        {
            auto start = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            auto end = std::chrono::high_resolution_clock::now();

            double observed = (end - start).count() / 1e9;
            seconds -= observed;

            ++count;
            double delta = observed - mean;
            mean += delta / count;
            m2   += delta * (observed - mean);
            double stddev = sqrt(m2 / (count - 1));
            estimate = mean + stddev;
        }

        // spin lock
        auto start = std::chrono::high_resolution_clock::now();
        while ((std::chrono::high_resolution_clock::now() - start).count() / 1e9 < seconds)
        {
        }
    }

} // namespace

#endif

namespace mango
{

    u64 Time::ms()
    {
        return getSystemContext().timer.ms();
    }

    u64 Time::us()
    {
        return getSystemContext().timer.us();
    }

    u64 Time::ns()
    {
        return getSystemContext().timer.ns();
    }

#ifdef MANGO_PLATFORM_WINDOWS

    void Sleep::ms(s32 count)
    {
        preciseSleep(count / 1000.0);
    }

    void Sleep::us(s32 count)
    {
        preciseSleep(count / 1000000.0);
    }

#else

    void Sleep::ms(s32 count)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void Sleep::us(s32 count)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(count));
    }

#endif

    LocalTime getLocalTime()
    {
        std::time_t t = std::time(nullptr);
        std::tm* s = std::localtime(&t);

        LocalTime time;

        time.year   = s->tm_year + 1900;
        time.month  = s->tm_mon + 1;
        time.day    = s->tm_mday;
        time.wday   = s->tm_wday;
        time.hour   = s->tm_hour;
        time.minute = s->tm_min;
        time.second = s->tm_sec;

        return time;
    }

} // namespace mango
