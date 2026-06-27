/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/window/window.hpp>
#include <mango/core/timer.hpp>
#include <mango/core/exception.hpp>
#include "window_backend.hpp"

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
    // Window facade
    // ------------------------------------------------------------------------------

    Window::Window(int width, int height, u32 flags, WindowSystem ws)
    {
        createBackend(ws, width, height, flags, "Mango");
    }

    Window::~Window()
    {
        // m_backend destroyed by unique_ptr
    }

    void Window::createBackend(WindowSystem ws, int width, int height, u32 flags, const char* title)
    {
        m_window_system = resolveWindowSystem(ws);
        m_backend = createWindowBackend(m_window_system, this, width, height, flags, title);
        if (!m_backend)
        {
            MANGO_EXCEPTION("[Window] Creating window backend failed.");
        }
    }

    WindowSystem Window::getWindowSystem() const
    {
        return m_window_system;
    }

    void Window::setWindowPosition(int x, int y)
    {
        m_backend->setWindowPosition(x, y);
    }

    void Window::setWindowSize(int width, int height)
    {
        m_backend->setWindowSize(width, height);
    }

    void Window::setTitle(const std::string& title)
    {
        m_backend->setTitle(title);
    }

    void Window::setVisible(bool enable)
    {
        m_backend->setVisible(enable);
    }

    math::int32x2 Window::getWindowSize() const
    {
        return m_backend->getWindowSize();
    }

    math::int32x2 Window::getCursorPosition() const
    {
        return m_backend->getCursorPosition();
    }

    double Window::getDisplayRefreshRate() const
    {
        return m_backend->getDisplayRefreshRate();
    }

    void Window::toggleFullscreen()
    {
        m_backend->toggleFullscreen();
    }

    bool Window::isFullscreen() const
    {
        return m_backend->isFullscreen();
    }

    bool Window::isKeyPressed(Keycode code) const
    {
        return m_backend->isKeyPressed(code);
    }

    void Window::enterEventLoop(const EventLoopConfig& config)
    {
        m_event_loop.reset(config);
        syncDisplayRefreshRate();
        m_backend->runEventLoop();
    }

    void Window::invalidate()
    {
        m_event_loop.invalidate();
        m_backend->wakeEventLoop();
    }

    void Window::requestFrameAt(u64 time_us)
    {
        m_event_loop.next_frame_deadline_us = time_us;
        m_backend->wakeEventLoop();
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
        m_backend->wakeEventLoop();
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

    void Window::onMinimize()
    {
    }

    void Window::onMaximize()
    {
    }

    void Window::onKeyPress(Keycode code, u32 mask)
    {
        MANGO_UNREFERENCED(code);
        MANGO_UNREFERENCED(mask);
    }

    void Window::onKeyRelease(Keycode code)
    {
        MANGO_UNREFERENCED(code);
    }

    void Window::onMouseMove(int x, int y)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
    }

    void Window::onMouseClick(int x, int y, MouseButton button, int count)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
        MANGO_UNREFERENCED(button);
        MANGO_UNREFERENCED(count);
    }

    void Window::onDropFiles(const filesystem::FileIndex& index)
    {
        MANGO_UNREFERENCED(index);
    }

    void Window::onClose()
    {
    }

    void Window::onShow()
    {
    }

    void Window::onHide()
    {
    }

    // ------------------------------------------------------------------------------
    // Backend factory dispatch
    // ------------------------------------------------------------------------------

    WindowSystem resolveWindowSystem(WindowSystem ws)
    {
#if defined(MANGO_WINDOW_SYSTEM_WIN32)
        MANGO_UNREFERENCED(ws);
        return WindowSystem::Win32;
#elif defined(MANGO_WINDOW_SYSTEM_COCOA)
        MANGO_UNREFERENCED(ws);
        return WindowSystem::Cocoa;
#else
        // Linux: pick the requested backend, resolving Default to a sensible
        // available choice. Honor only backends actually compiled in.
        switch (ws)
        {
#if defined(MANGO_ENABLE_XLIB)
            case WindowSystem::Xlib:
                return WindowSystem::Xlib;
#endif
#if defined(MANGO_ENABLE_XCB)
            case WindowSystem::Xcb:
                return WindowSystem::Xcb;
#endif
#if defined(MANGO_ENABLE_WAYLAND)
            case WindowSystem::Wayland:
                return WindowSystem::Wayland;
#endif
            default:
                break;
        }

        // Default / unavailable request -> first compiled-in backend.
#if defined(MANGO_ENABLE_XLIB)
        return WindowSystem::Xlib;
#elif defined(MANGO_ENABLE_XCB)
        return WindowSystem::Xcb;
#elif defined(MANGO_ENABLE_WAYLAND)
        return WindowSystem::Wayland;
#else
        return WindowSystem::Default;
#endif
#endif
    }

    std::unique_ptr<WindowBackend> createWindowBackend(WindowSystem ws, Window* window,
        int width, int height, u32 flags, const char* title)
    {
#if defined(MANGO_WINDOW_SYSTEM_WIN32)
        MANGO_UNREFERENCED(ws);
        return createWin32Backend(window, width, height, flags, title);
#elif defined(MANGO_WINDOW_SYSTEM_COCOA)
        MANGO_UNREFERENCED(ws);
        return createCocoaBackend(window, width, height, flags, title);
#else
        switch (ws)
        {
#if defined(MANGO_ENABLE_XLIB)
            case WindowSystem::Xlib:
                return createXlibBackend(window, width, height, flags, title);
#endif
#if defined(MANGO_ENABLE_XCB)
            case WindowSystem::Xcb:
                return createXcbBackend(window, width, height, flags, title);
#endif
#if defined(MANGO_ENABLE_WAYLAND)
            case WindowSystem::Wayland:
                return createWaylandBackend(window, width, height, flags, title);
#endif
            default:
                break;
        }
        return nullptr;
#endif
    }

} // namespace mango
