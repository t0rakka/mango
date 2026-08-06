/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "window_peers.hpp"
#include <mango/window/event_loop.hpp>
#include "window_backend.hpp"

#include <algorithm>
#include <mutex>
#include <vector>

namespace mango
{
namespace window_peers
{

    namespace
    {
        std::mutex g_mutex;
        std::vector<WindowBackend*> g_backends;
        EventLoop* g_active_loop = nullptr;
    }

    void registerBackend(WindowBackend* backend)
    {
        if (!backend)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(g_mutex);
        if (std::find(g_backends.begin(), g_backends.end(), backend) == g_backends.end())
        {
            g_backends.push_back(backend);
        }
    }

    void unregisterBackend(WindowBackend* backend)
    {
        if (!backend)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(g_mutex);
        g_backends.erase(std::remove(g_backends.begin(), g_backends.end(), backend), g_backends.end());
    }

    void forEach(void (*fn)(WindowBackend* backend, void* user), void* user)
    {
        if (!fn)
        {
            return;
        }

        std::vector<WindowBackend*> snapshot;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            snapshot = g_backends;
        }

        for (WindowBackend* backend : snapshot)
        {
            fn(backend, user);
        }
    }

    void forEachOther(WindowBackend* self, void (*fn)(WindowBackend* backend, void* user), void* user)
    {
        if (!fn)
        {
            return;
        }

        std::vector<WindowBackend*> snapshot;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            snapshot = g_backends;
        }

        for (WindowBackend* backend : snapshot)
        {
            if (backend != self)
            {
                fn(backend, user);
            }
        }
    }

    void drainOtherBackends(WindowBackend* self)
    {
        forEachOther(self, [](WindowBackend* backend, void*)
        {
            backend->drainPendingEvents();
        }, nullptr);
    }

    void setActiveEventLoop(EventLoop* loop)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_active_loop = loop;
    }

    EventLoop* activeEventLoop()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_active_loop;
    }

} // namespace window_peers
} // namespace mango
