/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <algorithm>
#include <mango/window/event_loop.hpp>
#include <mango/core/exception.hpp>
#include "window_peers.hpp"
#include "window_backend.hpp"

namespace mango
{

    EventLoop::EventLoop() = default;

    EventLoop::~EventLoop()
    {
        if (m_running)
        {
            quit();
        }
        clear();
    }

    void EventLoop::attach(Window& window, const EventLoopConfig& config)
    {
        if (std::find(m_windows.begin(), m_windows.end(), &window) != m_windows.end())
        {
            return;
        }

        if (window.m_event_loop_runner && window.m_event_loop_runner != this)
        {
            window.m_event_loop_runner->detach(window);
        }

        window.m_event_loop.config = config;
        window.syncDisplayRefreshRate();

        m_windows.push_back(&window);
        window.m_event_loop_runner = this;

        if (!m_pump)
        {
            m_pump = &window;
        }

        // Windows attached after run() started still need a live EventLoopState and
        // the same onEventLoopStarting hook the initial set got (device ready / show).
        if (m_running)
        {
            window.m_event_loop.reset();
            window.onEventLoopStarting();
            window.backend()->wakeEventLoop();
        }
    }

    void EventLoop::clear()
    {
        while (!m_windows.empty())
        {
            detach(*m_windows.back());
        }

        m_pump = nullptr;
    }

    void EventLoop::run()
    {
        if (m_running)
        {
            return;
        }

        if (!m_pump)
        {
            MANGO_EXCEPTION("[EventLoop] No windows registered.");
        }

        m_running = true;

        for (Window* window : m_windows)
        {
            window->syncDisplayRefreshRate();
            window->m_event_loop.reset();
            window->onEventLoopStarting();
        }

        window_peers::setActiveEventLoop(this);
        m_pump->backend()->runEventLoop();
        window_peers::setActiveEventLoop(nullptr);

        m_running = false;

        for (Window* window : m_windows)
        {
            window->m_event_loop.running = false;
        }
    }

    void EventLoop::quit()
    {
        if (!m_running)
        {
            return;
        }

        m_running = false;

        for (Window* window : m_windows)
        {
            window->m_event_loop.running = false;
            window->backend()->wakeEventLoop();
        }
    }

    bool EventLoop::isRunning() const
    {
        return m_running;
    }

    void EventLoop::dispatchFrames()
    {
        if (!m_running)
        {
            return;
        }

        // Snapshot so attach/detach from onFrame (e.g. opening a tool window) is safe.
        const std::vector<Window*> snapshot = m_windows;
        for (Window* window : snapshot)
        {
            if (window->m_event_loop_runner == this)
            {
                window->dispatchFrame();
            }
        }
    }

    u32 EventLoop::computeWaitTimeoutMs(u64 now_us) const
    {
        if (m_windows.empty())
        {
            return EventLoopState::WAIT_INFINITE;
        }

        u32 timeout = EventLoopState::WAIT_INFINITE;

        for (Window* window : m_windows)
        {
            const u32 window_timeout = window->eventLoop().computeWaitTimeoutMs(now_us);

            if (window_timeout == 0)
            {
                return 0;
            }

            if (timeout == EventLoopState::WAIT_INFINITE)
            {
                timeout = window_timeout;
            }
            else if (window_timeout != EventLoopState::WAIT_INFINITE)
            {
                timeout = std::min(timeout, window_timeout);
            }
        }

        return timeout;
    }

    void EventLoop::detach(Window& window)
    {
        if (window.m_event_loop_runner == this)
        {
            window.m_event_loop_runner = nullptr;
        }

        m_windows.erase(std::remove(m_windows.begin(), m_windows.end(), &window), m_windows.end());

        if (m_pump == &window)
        {
            m_pump = m_windows.empty() ? nullptr : m_windows.front();
        }
    }

} // namespace mango

namespace mango::window_peers
{

    bool isEventLoopPump(const Window* window)
    {
        EventLoop* loop = activeEventLoop();
        if (!loop || !window)
        {
            return false;
        }

        return loop->m_pump == window;
    }

} // namespace mango::window_peers
