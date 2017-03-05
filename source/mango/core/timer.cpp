/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <ctime>
#include <mango/core/timer.hpp>

namespace mango
{

#ifdef MANGO_PLATFORM_WINDOWS

    LocalTime::LocalTime()
    {
        SYSTEMTIME st;
        ::GetLocalTime(&st);

        year    = st.wYear;
        month   = st.wMonth;
        day     = st.wDay;
        wday    = st.wDayOfWeek;
        hour    = st.wHour;
        minute  = st.wMinute;
        second  = st.wSecond;
    }

#else

    LocalTime::LocalTime()
    {
	    std::time_t t = std::time(nullptr);
	    std::tm* s = std::localtime(&t);

        year   = s->tm_year + 1900;
        month  = s->tm_mon + 1;
        day    = s->tm_mday;
        wday   = s->tm_wday;
        hour   = s->tm_hour;
        minute = s->tm_min;
        second = s->tm_sec;
    }

#endif // MANGO_PLATFORM_UNIX

} // namespace mango
