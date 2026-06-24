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
        last_frame_time_us = Time::us();
        last_dt = 0.0;
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

        if (config.mode == FrameMode::OnDemand)
        {
            return needs_redraw;
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
        runEventLoop();
    }

    void Window::invalidate()
    {
        m_event_loop.invalidate();
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
    }

    const EventLoopConfig& Window::getEventLoopConfig() const
    {
        return m_event_loop.config;
    }

    void Window::setEventLoopConfig(const EventLoopConfig& config)
    {
        m_event_loop.config = config;
    }

    void Window::setFrameMode(FrameMode mode)
    {
        m_event_loop.config.mode = mode;
    }

    void Window::setMaxFrameRate(double frameRate)
    {
        m_event_loop.config.maxFrameRate = frameRate;
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

        FrameInfo info;
        info.time_us = now;
        info.dt = m_event_loop.computeDt(now);
        info.invalidated = m_event_loop.consumeInvalidated();

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
