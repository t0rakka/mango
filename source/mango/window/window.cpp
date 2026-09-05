/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <cstdlib>
#include <string>
#include <mutex>
#include <mango/window/window.hpp>
#include <mango/window/event_loop.hpp>
#include <mango/core/timer.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/print.hpp>
#include <mango/core/string.hpp>
#include "window_backend.hpp"
#include "window_registry.hpp"
#include "window_peers.hpp"

namespace mango
{

    // ------------------------------------------------------------------------------
    // Window system selection helpers
    // ------------------------------------------------------------------------------

    std::string_view getString(WindowSystem ws)
    {
        switch (ws)
        {
            case WindowSystem::Win32:   return "Win32";
            case WindowSystem::Cocoa:   return "Cocoa";
            case WindowSystem::Xlib:    return "Xlib";
            case WindowSystem::Xcb:     return "Xcb";
            case WindowSystem::Wayland: return "Wayland";
            case WindowSystem::Default: return "Default";
        }
        return "UNDEFINED";
    }

    namespace
    {

        // Process-global active window system. g_window_system holds the current
        // selection (Default == not chosen yet); g_window_system_locked becomes
        // true once the value has been handed out, after which overrides are
        // refused (the resolved value has already driven instance/surface setup).
        // The mutex keeps the value, the lock flag and the one-time resolve/log
        // consistent across threads (the value and flag must move together).
        std::mutex g_window_system_mutex;
        WindowSystem g_window_system = WindowSystem::Default;
        bool g_window_system_locked = false;

#if defined(MANGO_PLATFORM_LINUX)

        bool isWindowSystemAvailable(WindowSystem ws)
        {
            return isWindowBackendRegistered(ws);
        }

        WindowSystem firstAvailableWindowSystem()
        {
            return firstRegisteredWindowSystem();
        }

        WindowSystem autoDetectWindowSystem()
        {
            // 1. Explicit override (forcing a backend, e.g. for testing):
            //    MANGO_WINDOW_SYSTEM = wayland | xlib | x11 | xcb
            if (const char* env = std::getenv("MANGO_WINDOW_SYSTEM"))
            {
                std::string value = toLower(env);

                WindowSystem requested = WindowSystem::Default;
                if (value == "wayland")
                    requested = WindowSystem::Wayland;
                else if (value == "xlib" || value == "x11")
                    requested = WindowSystem::Xlib;
                else if (value == "xcb")
                    requested = WindowSystem::Xcb;

                if (requested != WindowSystem::Default && isWindowSystemAvailable(requested))
                {
                    return requested;
                }

                printLine(Print::Warning,
                    "[Window] MANGO_WINDOW_SYSTEM=\"{}\" ignored (unknown or not compiled in).", env);
            }

            // 2. Auto-detect from the running session (preferred order Wayland > X).
            if (isWindowBackendRegistered(WindowSystem::Wayland))
            {
                if (const char* display = std::getenv("WAYLAND_DISPLAY"); display && *display)
                {
                    return WindowSystem::Wayland;
                }
            }

            if (isWindowBackendRegistered(WindowSystem::Xlib) || isWindowBackendRegistered(WindowSystem::Xcb))
            {
                if (const char* display = std::getenv("DISPLAY"); display && *display)
                {
                    if (isWindowBackendRegistered(WindowSystem::Xlib))
                    {
                        return WindowSystem::Xlib;
                    }
                    return WindowSystem::Xcb;
                }
            }

            // 3. Nothing detected: first available in preferred order.
            return firstAvailableWindowSystem();
        }

#endif // defined(MANGO_PLATFORM_LINUX)

    } // namespace

    // ------------------------------------------------------------------------------
    // EventLoopState
    // ------------------------------------------------------------------------------

    void EventLoopState::reset()
    {
        running = true;
        needs_redraw = true;
        frame_in_flight.store(false, std::memory_order_relaxed);
        frame_held.store(false, std::memory_order_relaxed);
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
        if (config.waitForFrame && frame_in_flight.load(std::memory_order_acquire))
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
        if (config.waitForFrame && frame_in_flight.load(std::memory_order_acquire))
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

        // Continuous with a CPU-side cap: sleep until the next frame slot.
        // When GLX swapInterval does not block (common under X11 compositors),
        // this is what keeps frame delivery regular — Wayland is paced by the
        // compositor instead and looks smooth without it.
        if (config.maxFrameRate > 0.0 && last_frame_time_us > 0)
        {
            const u64 interval_us = u64(1'000'000.0 / config.maxFrameRate);
            const u64 next_us = last_frame_time_us + interval_us;
            if (now_us >= next_us)
            {
                return 0;
            }

            const u64 remaining_ms = (next_us - now_us + 999) / 1000;
            return remaining_ms > 0x7fffffffull ? 0x7fffffffu : u32(remaining_ms);
        }

        // Continuous uncapped: short poll; present()/vsync is the limiter.
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

    Window::Window(int width, int height, u32 flags)
    {
        createBackend(width, height, flags, "Mango");
    }

    Window::~Window()
    {
        if (m_event_loop_runner)
        {
            m_event_loop_runner->detach(*this);
        }
    }

    void Window::setWindowSystem(WindowSystem ws)
    {
        std::lock_guard<std::mutex> guard(g_window_system_mutex);

        if (g_window_system_locked)
        {
            printLine(Print::Warning,
                "[Window] setWindowSystem() ignored: the window system is already in use ({}). "
                "Call it before creating any window or querying surface extensions.",
                getString(g_window_system));
            return;
        }

        g_window_system = resolveWindowSystem(ws);
        printLine(Print::Info, "WindowSystem: {}", getString(g_window_system));
    }

    WindowSystem Window::getWindowSystem()
    {
        std::lock_guard<std::mutex> guard(g_window_system_mutex);

        if (!g_window_system_locked)
        {
            if (g_window_system == WindowSystem::Default)
            {
                // No explicit override; resolve now (env + auto-detect) and log.
                g_window_system = resolveWindowSystem(WindowSystem::Default);
                printLine(Print::Info, "WindowSystem: {}", getString(g_window_system));
            }
            g_window_system_locked = true;
        }
        return g_window_system;
    }

    void Window::createBackend(int width, int height, u32 flags, const char* title)
    {
        m_backend = createWindowBackend(getWindowSystem(), this, width, height, flags, title);
        if (!m_backend)
        {
            MANGO_EXCEPTION("[Window] Creating window backend failed.");
        }
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

    math::int32x2 Window::getClientSize() const
    {
        return m_backend->getClientSize();
    }

    float Window::getContentScale() const
    {
        return m_backend->getContentScale();
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

    void Window::requestQuit()
    {
        if (m_event_loop_runner)
        {
            m_event_loop_runner->quit();
        }
    }

    void Window::handleCloseRequest()
    {
        onClose();

        if (m_event_loop_runner && m_event_loop.config.quitOnClose)
        {
            m_event_loop_runner->quit();
        }
        else
        {
            // Keep the native window out of the way until the app destroys this Window
            // from onClose (often deferred to the next frame).
            setVisible(false);
        }
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

    void Window::holdFrame()
    {
        if (m_event_loop.config.waitForFrame)
        {
            m_event_loop.frame_held.store(true, std::memory_order_release);
        }
    }

    void Window::frameComplete()
    {
        m_event_loop.frame_held.store(false, std::memory_order_relaxed);
        m_event_loop.frame_in_flight.store(false, std::memory_order_release);
        if (m_backend)
        {
            m_backend->wakeEventLoop();
        }
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
        //
        // In Continuous mode a pending invalidate is just "draw immediately" — still a
        // Continuous frame that advances time. Emitting Invalidate here froze dt at 0
        // and, with X11 expose spam, starved real Continuous frames (stutter / dead titles).
        const bool invalidated = m_event_loop.consumeInvalidated();

        FrameTrigger trigger;
        if (invalidated && m_event_loop.config.mode == FrameMode::OnDemand)
        {
            trigger = FrameTrigger::Invalidate;
        }
        else if (m_event_loop.next_frame_deadline_us && now >= m_event_loop.next_frame_deadline_us)
        {
            trigger = FrameTrigger::Timed;
        }
        else if (m_event_loop.config.mode == FrameMode::Continuous)
        {
            trigger = FrameTrigger::Continuous;
        }
        else
        {
            trigger = FrameTrigger::Invalidate;
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
        info.trigger = trigger;

        // Invalidate is a repaint of current content — do not advance the animation clock.
        if (trigger == FrameTrigger::Invalidate)
        {
            info.dt = 0.0;
        }
        else
        {
            info.dt = m_event_loop.computeDt(now);
            m_event_loop.last_frame_time_us = now;
        }

        if (m_event_loop.config.waitForFrame)
        {
            m_event_loop.frame_held.store(false, std::memory_order_relaxed);
            m_event_loop.frame_in_flight.store(true, std::memory_order_release);
        }

        onFrame(info);

        // Sync default: complete when onFrame returns unless holdFrame() opted into async.
        // Stay on the loop thread — clear in-flight without wakeEventLoop(). Waking on
        // every sync frame made X11 poll return immediately and fight frame pacing.
        if (m_event_loop.config.waitForFrame &&
            !m_event_loop.frame_held.load(std::memory_order_acquire))
        {
            m_event_loop.frame_in_flight.store(false, std::memory_order_release);
        }
    }

    void Window::onFrame(const FrameInfo& info)
    {
        MANGO_UNREFERENCED(info);
    }

    void Window::onEventLoopStarting()
    {
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
#if defined(MANGO_PLATFORM_WINDOWS)
        MANGO_UNREFERENCED(ws);
        return WindowSystem::Win32;
#elif defined(MANGO_PLATFORM_MACOS)
        MANGO_UNREFERENCED(ws);
        return WindowSystem::Cocoa;
#else
        // Linux: Default runs env-override + auto-detection. An explicit request is
        // honored when its backend is compiled in, otherwise we fall back to the
        // preferred order (Wayland > Xlib > Xcb).
        if (ws == WindowSystem::Default)
        {
            return autoDetectWindowSystem();
        }

        if (isWindowSystemAvailable(ws))
        {
            return ws;
        }

        return firstAvailableWindowSystem();
#endif
    }

    std::unique_ptr<WindowBackend> createWindowBackend(WindowSystem ws, Window* window,
        int width, int height, u32 flags, const char* title)
    {
#if defined(MANGO_PLATFORM_WINDOWS)
        MANGO_UNREFERENCED(ws);
        return createRegisteredWindowBackend(WindowSystem::Win32, window, width, height, flags, title);
#elif defined(MANGO_PLATFORM_MACOS)
        MANGO_UNREFERENCED(ws);
        return createRegisteredWindowBackend(WindowSystem::Cocoa, window, width, height, flags, title);
#else
        return createRegisteredWindowBackend(ws, window, width, height, flags, title);
#endif
    }

#if defined(MANGO_PLATFORM_LINUX)

    // Linux screen queries dispatch on the active WindowSystem so a Wayland
    // session never opens an X11 display (and vice versa). Works before any
    // Window exists — each backend connects ephemerally when needed.

    int Window::getScreenCount()
    {
        switch (getWindowSystem())
        {
#if defined(MANGO_HAS_WAYLAND_WINDOW)
            case WindowSystem::Wayland:
                return queryWaylandScreenCount();
#endif
#if defined(MANGO_HAS_XLIB_WINDOW)
            case WindowSystem::Xlib:
                return queryXlibScreenCount();
#endif
#if defined(MANGO_HAS_XCB_WINDOW)
            case WindowSystem::Xcb:
                return queryXcbScreenCount();
#endif
            default:
                return 0;
        }
    }

    math::int32x2 Window::getScreenSize(int screen)
    {
        switch (getWindowSystem())
        {
#if defined(MANGO_HAS_WAYLAND_WINDOW)
            case WindowSystem::Wayland:
                return queryWaylandScreenSize(screen);
#endif
#if defined(MANGO_HAS_XLIB_WINDOW)
            case WindowSystem::Xlib:
                return queryXlibScreenSize(screen);
#endif
#if defined(MANGO_HAS_XCB_WINDOW)
            case WindowSystem::Xcb:
                return queryXcbScreenSize(screen);
#endif
            default:
                return math::int32x2(0, 0);
        }
    }

#endif // defined(MANGO_PLATFORM_LINUX)

} // namespace mango
