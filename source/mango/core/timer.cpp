/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <ctime>
#include <mango/core/timer.hpp>

namespace mango
{

#ifdef MANGO_PLATFORM_WINDOWS

    // --------------------------------------------------------------------
    // Windows Timer
    // --------------------------------------------------------------------

    Timer::Timer()
    {
		LARGE_INTEGER freq;
		LARGE_INTEGER counter;
		::QueryPerformanceFrequency(&freq);
		::QueryPerformanceCounter(&counter);
		m_freq = static_cast<uint64>(freq.QuadPart);
		m_start = static_cast<uint64>(counter.QuadPart);
    }

    Timer::~Timer()
    {
    }

    void Timer::reset()
    {
		LARGE_INTEGER counter;
		::QueryPerformanceCounter(&counter);
		m_start = static_cast<uint64>(counter.QuadPart);
    }

    float Timer::time() const
    {
		LARGE_INTEGER counter;
		::QueryPerformanceCounter(&counter);
		uint64 delta = static_cast<uint64>(counter.QuadPart) - m_start;
		return static_cast<float>(delta) / static_cast<float>(m_freq);
    }

    LocalTime Timer::local() const
    {
        SYSTEMTIME st;
        ::GetLocalTime(&st);

        LocalTime c;

        c.year         = st.wYear;
        c.month        = st.wMonth;
        c.day          = st.wDay;
		c.wday  	   = st.wDayOfWeek;
        c.hour         = st.wHour;
        c.minute       = st.wMinute;
        c.second       = st.wSecond;

        return c;
    }

#endif

#ifdef MANGO_PLATFORM_UNIX

#if defined(MANGO_PLATFORM_LINUX)

    // --------------------------------------------------------------------
    // Linux Timer
    // --------------------------------------------------------------------

    Timer::Timer()
    {
        clock_gettime(CLOCK_REALTIME, &m_start);
    }

    Timer::~Timer()
    {
    }

    void Timer::reset()
    {
        clock_gettime(CLOCK_REALTIME, &m_start);
    }

    float Timer::time() const
    {
        timespec end;
        clock_gettime(CLOCK_REALTIME, &end);

        timespec delta;

        if ((end.tv_nsec - m_start.tv_nsec) < 0)
        {
            delta.tv_sec = end.tv_sec - m_start.tv_sec - 1;
            delta.tv_nsec = 1000000000 + end.tv_nsec - m_start.tv_nsec;
        }
        else
        {
            delta.tv_sec = end.tv_sec - m_start.tv_sec;
            delta.tv_nsec = end.tv_nsec - m_start.tv_nsec;
        }

        float s = delta.tv_sec + (delta.tv_nsec / 1000000000.0f);
        return s;
    }

#elif defined(__MACH__)

    // --------------------------------------------------------------------
    // Mach Kernel Timer
    // --------------------------------------------------------------------

    Timer::Timer()
    {
        host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &m_clock);

        mach_timespec_t ts;
        clock_get_time(m_clock, &ts);
        m_start.tv_sec = ts.tv_sec;
        m_start.tv_nsec = ts.tv_nsec;
    }

    Timer::~Timer()
    {
        mach_port_deallocate(mach_task_self(), m_clock);
    }

    void Timer::reset()
    {
        mach_timespec_t ts;
        clock_get_time(m_clock, &ts);
        m_start.tv_sec = ts.tv_sec;
        m_start.tv_nsec = ts.tv_nsec;
    }

    float Timer::time() const
    {
        timespec end;

        mach_timespec_t ts;
        clock_get_time(m_clock, &ts);
        end.tv_sec = ts.tv_sec;
        end.tv_nsec = ts.tv_nsec;

        timespec delta;

        if ((end.tv_nsec - m_start.tv_nsec) < 0)
        {
            delta.tv_sec = end.tv_sec - m_start.tv_sec - 1;
            delta.tv_nsec = 1000000000 + end.tv_nsec - m_start.tv_nsec;
        }
        else
        {
            delta.tv_sec = end.tv_sec - m_start.tv_sec;
            delta.tv_nsec = end.tv_nsec - m_start.tv_nsec;
        }

        float s = delta.tv_sec + (delta.tv_nsec / 1000000000.0f);
        return s;
    }

#else

    // --------------------------------------------------------------------
    // Generic Unix Timer
    // --------------------------------------------------------------------

    Timer::Timer()
    {
        ::gettimeofday(&m_start, NULL);
    }

    Timer::~Timer()
    {
    }

    void Timer::reset()
    {
        ::gettimeofday(&m_start, NULL);
    }

    float Timer::time() const
    {
        struct timeval current;

        ::gettimeofday(&current, NULL);

        return static_cast<float>(
            (current.tv_sec - m_start.tv_sec) * 1000.f +
            (current.tv_usec - m_start.tv_usec) / 1000.f) / 1000.f;
    }

#endif

#if 0

	// --------------------------------------------------------------------
	// chrono Timer
	// --------------------------------------------------------------------

	Timer::Timer()
	{
		m_start = std::chrono::high_resolution_clock::now();
	}

	Timer::~Timer()
	{
	}

	void Timer::reset()
	{
		m_start = std::chrono::high_resolution_clock::now();
	}

	float Timer::time() const
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count();
		double x = double(duration) / double(1000000.0);
		return float(x);
	}

	LocalTime Timer::local() const
	{
		SYSTEMTIME st;
		::GetLocalTime(&st);

		LocalTime c;

		c.year = st.wYear;
		c.month = st.wMonth;
		c.day = st.wDay;
		c.wday = st.wDayOfWeek;
		c.hour = st.wHour;
		c.minute = st.wMinute;
		c.second = st.wSecond;

		return c;
	}

#endif // 0

LocalTime Timer::local() const
{
	std::time_t t = std::time(NULL);
	std::tm* s = std::localtime(&t);

    LocalTime c;

    c.year   = s->tm_year + 1900;
    c.month  = s->tm_mon + 1;
    c.day    = s->tm_mday;
	c.wday   = s->tm_wday;
    c.hour   = s->tm_hour;
    c.minute = s->tm_min;
    c.second = s->tm_sec;

    return c;
}

#endif // MANGO_PLATFORM_UNIX

} // namespace mango
