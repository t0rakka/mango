/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <chrono>
#include <mango/core/configure.hpp>

namespace mango
{

    class Timer
    {
    public:
        using Time = std::chrono::time_point<std::chrono::high_resolution_clock>;

        void reset()
        {
            // reset timer
            m_start = now();
        }

        double time() const
        {
            // timer age in seconds
            std::chrono::duration<double> duration = now() - m_start;
            return duration.count();
        }

        u64 ms() const
        {
            // time age in milliseconds
            return u64(std::chrono::duration_cast<std::chrono::milliseconds>(now() - m_start).count());
        }

        u64 us() const
        {
            // timer age in microseconds
            return u64(std::chrono::duration_cast<std::chrono::microseconds>(now() - m_start).count());
        }

        u64 ns() const
        {
            // timer age in nanoseconds
            return u64(std::chrono::duration_cast<std::chrono::nanoseconds>(now() - m_start).count());
        }

        Time now() const
        {
            // std::chrono compatible time_point
            return std::chrono::high_resolution_clock::now();
        }

    protected:
        Time m_start { now() };
    };

    struct Time
    {
        static u64 ms();
        static u64 us();
        static u64 ns();
    };

    struct Sleep
    {
        static void ms(s32 count);
        static void us(s32 count);
        static void ns(s32 count);
    };

    struct LocalTime
    {
        int  year;   // [....]
        int  month;  // [1, 12]
        int  day;    // [1, 31]
        int  wday;   // [0, 6]
        int  hour;   // [0, 23]
        int  minute; // [0, 59]
        int  second; // [0, 60]
    };

    LocalTime getLocalTime();

} // namespace mango
