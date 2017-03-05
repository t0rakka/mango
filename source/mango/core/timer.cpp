/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <ctime>
#include <mango/core/timer.hpp>

namespace mango
{

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
