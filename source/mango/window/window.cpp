/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/window/window.hpp>
#include <mango/core/timer.hpp>

namespace mango
{

    // ------------------------------------------------------------------------------
    // EventLoopState
    // ------------------------------------------------------------------------------

    void EventLoopState::reset(const EventLoopConfig& loopConfig)
    {
        config = loopConfig;
        running = true;
        needs_redraw = true;
        frame_in_flight = false;
        loop_start_time_us = Time::us();
        last_frame_time_us = 0;
        last_dt = 0.0;
        next_frame_deadline_us = 0;
    }

    void EventLoopState::invalidate()
    {
        needs_redraw = true;
    }

    bool EventLoopState::shouldScheduleFrame(u64 now_us) const
    {
        if (config.waitForFrame && frame_in_flight)
        {
            return false;
        }

        if (needs_redraw)
        {
            return true;
        }

        // A pending timed wake fires in either mode; this is what drives OnDemand
        // animation (and lets Continuous request a precise next frame).
        if (next_frame_deadline_us && now_us >= next_frame_deadline_us)
        {
            return true;
        }

        if (config.mode == FrameMode::OnDemand)
        {
            return false;
        }

        if (config.maxFrameRate > 0.0)
        {
            const u64 interval_us = u64(1'000'000.0 / config.maxFrameRate);
            if (now_us - last_frame_time_us < interval_us)
            {
                return false;
            }
        }

        return true;
    }

    u32 EventLoopState::computeWaitTimeoutMs(u64 now_us) const
    {
        // Async backpressure: a frame is in flight and will be cleared by
        // frameComplete() (possibly from another context that posts no event), so we
        // must keep polling to notice it promptly rather than blocking.
        if (config.waitForFrame && frame_in_flight)
        {
            return config.pollTimeoutMs;
        }

        // A redraw is already queued; resolve it on the next pass without sleeping.
        if (needs_redraw)
        {
            return 0;
        }

        // A pending timed wake bounds the sleep in either mode.
        if (next_frame_deadline_us)
        {
            if (now_us >= next_frame_deadline_us)
            {
                return 0;
            }

            // round up so we never wake a hair early and spin
            const u64 remaining_ms = (next_frame_deadline_us - now_us + 999) / 1000;
            return remaining_ms > 0x7fffffffull ? 0x7fffffffu : u32(remaining_ms);
        }

        // OnDemand with nothing scheduled: idle until an event wakes the loop.
        if (config.mode == FrameMode::OnDemand)
        {
            return WAIT_INFINITE;
        }

        // Continuous: keep the existing poll cadence; present()/vsync is the limiter.
        return config.pollTimeoutMs;
    }

    bool EventLoopState::consumeInvalidated()
    {
        const bool was_invalidated = needs_redraw;
        needs_redraw = false;
        return was_invalidated;
    }

    double EventLoopState::computeDt(u64 now_us)
    {
        double dt = 0.0;

        if (last_frame_time_us > 0)
        {
            dt = double(now_us - last_frame_time_us) / 1'000'000.0;
        }

        last_dt = dt;
        return dt;
    }

    // ------------------------------------------------------------------------------
    // Window
    // ------------------------------------------------------------------------------

    void Window::enterEventLoop(const EventLoopConfig& config)
    {
        m_event_loop.reset(config);
        syncDisplayRefreshRate();
        runEventLoop();
    }

    void Window::invalidate()
    {
        m_event_loop.invalidate();
        wakeEventLoop();
    }

    void Window::requestFrameAt(u64 time_us)
    {
        m_event_loop.next_frame_deadline_us = time_us;
        wakeEventLoop();
    }

    void Window::requestFrameIn(double seconds)
    {
        if (seconds <= 0.0)
        {
            // already due; fire on the next loop iteration
            requestFrameAt(Time::us());
            return;
        }

        requestFrameAt(Time::us() + u64(seconds * 1'000'000.0));
    }

    bool Window::isRunning() const
    {
        return m_event_loop.running;
    }

    void Window::requestQuit()
    {
        breakEventLoop();
    }

    void Window::breakEventLoop()
    {
        m_event_loop.running = false;
        wakeEventLoop();
    }

    const EventLoopConfig& Window::getEventLoopConfig() const
    {
        return m_event_loop.config;
    }

    void Window::setEventLoopConfig(const EventLoopConfig& config)
    {
        m_event_loop.config = config;
        syncDisplayRefreshRate();
    }

    void Window::setFrameMode(FrameMode mode)
    {
        m_event_loop.config.mode = mode;
    }

    void Window::setMaxFrameRate(double frameRate)
    {
        m_event_loop.config.maxFrameRate = frameRate;
        m_event_loop.config.trackDisplayRefreshRate = false;
    }

    void Window::syncDisplayRefreshRate()
    {
        if (!m_event_loop.config.trackDisplayRefreshRate)
        {
            return;
        }

        const double hz = getDisplayRefreshRate();
        if (hz > 0.0)
        {
            m_event_loop.config.maxFrameRate = hz * m_event_loop.config.displayRefreshHeadroom;
        }
    }

    void Window::frameComplete()
    {
        m_event_loop.frame_in_flight = false;
    }

    void Window::dispatchFrame()
    {
        if (!m_event_loop.running)
        {
            return;
        }

        const u64 now = Time::us();

        if (!m_event_loop.shouldScheduleFrame(now))
        {
            return;
        }

        // Classify why we're dispatching, mirroring shouldScheduleFrame's precedence:
        // an explicit redraw request wins, then a pending timed wake, otherwise the
        // continuous cadence. consumeInvalidated() also clears the needs_redraw flag.
        FrameTrigger trigger;
        if (m_event_loop.consumeInvalidated())
        {
            trigger = FrameTrigger::Invalidate;
        }
        else if (m_event_loop.next_frame_deadline_us && now >= m_event_loop.next_frame_deadline_us)
        {
            trigger = FrameTrigger::Timed;
        }
        else
        {
            trigger = FrameTrigger::Continuous;
        }

        // Only a frame that actually fired the deadline consumes it. If an invalidate
        // preempts a due deadline, the wake stays armed and fires on a later iteration
        // rather than being silently dropped; the client re-arms it (e.g. from onFrame)
        // to schedule the following frame.
        if (trigger == FrameTrigger::Timed)
        {
            m_event_loop.next_frame_deadline_us = 0;
        }

        FrameInfo info;
        info.time_us = now;
        info.time = double(now - m_event_loop.loop_start_time_us) / 1'000'000.0;
        info.dt = m_event_loop.computeDt(now);
        info.trigger = trigger;

        if (m_event_loop.config.waitForFrame)
        {
            m_event_loop.frame_in_flight = true;
        }

        m_event_loop.last_frame_time_us = now;

        onFrame(info);

        if (m_event_loop.config.waitForFrame)
        {
            m_event_loop.frame_in_flight = false;
        }
    }

    void Window::waitForNextIteration()
    {
        const u32 timeout = m_event_loop.config.pollTimeoutMs;
        if (timeout > 0)
        {
            Sleep::ms(timeout);
        }
    }

    void Window::onFrame(const FrameInfo& info)
    {
        MANGO_UNREFERENCED(info);
    }

    void Window::onResize(int width, int height)
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
        invalidate();
    }

} // namespace mango
