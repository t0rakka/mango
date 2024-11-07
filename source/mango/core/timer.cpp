/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <ctime>
#include <mango/core/system.hpp>
#include <mango/core/timer.hpp>

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

    void Sleep::ms(s32 count)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(count));
    }

    void Sleep::us(s32 count)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(count));
    }

    void Sleep::ns(s32 count)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(count));
    }

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
