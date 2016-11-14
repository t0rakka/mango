/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <chrono>
#include "configure.hpp"

#ifdef MANGO_PLATFORM_UNIX
#include <unistd.h>
#include <sys/time.h>
    #if defined(MANGO_PLATFORM_LINUX)
    #include <time.h>
    #elif defined(__MACH__)
    #include <mach/mach.h>
    #include <mach/clock.h>
    #endif
#endif

namespace mango
{

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

    // TODO: replace this header with std::chrono

    class Timer
    {
    private:

#ifdef MANGO_PLATFORM_WINDOWS
        uint64 m_freq;
        uint64 m_start;
#endif

#ifdef MANGO_PLATFORM_UNIX
    #if defined(MANGO_PLATFORM_LINUX)
        timespec m_start;
    #elif defined(__MACH__)
        clock_serv_t m_clock;
        struct timespec m_start;
    #else
		struct timeval m_start;
    #endif
#endif

#if 0
		std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
#endif

    public:
        Timer();
        ~Timer();

        void reset();
        float time() const;
        LocalTime local() const;
    };

} // namespace mango
