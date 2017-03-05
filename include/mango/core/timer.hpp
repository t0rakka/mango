/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <chrono>
#include "configure.hpp"

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

        uint64 ms() const
        {
            // time age in milliseconds
            return std::chrono::duration_cast<std::chrono::milliseconds>(now() - m_start).count();
        }

        uint64 us() const
        {
            // timer age in microseconds
            return std::chrono::duration_cast<std::chrono::microseconds>(now() - m_start).count();
        }

        uint64 ns() const
        {
            // timer age in nanoseconds
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now() - m_start).count();
        }

        Time now() const
        {
            // std::chrono compatible time_point
            return std::chrono::high_resolution_clock::now();
        }

    protected:
        Time m_start { now() };
    };

    struct LocalTime
    {
        uint16  year;	// [....]
        uint16  month;	// [1, 12]
        uint16  day;	// [1, 31]
        uint16  wday;   // [0, 6]
        uint16  hour;	// [0, 23]
        uint16  minute; // [0, 59]
        uint16  second; // [0, 60]
    };

    LocalTime getLocalTime();

} // namespace mango
