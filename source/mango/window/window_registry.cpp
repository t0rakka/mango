/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "window_registry.hpp"
#include "window_backend.hpp"

#include <array>
#include <mutex>

namespace mango
{

    namespace
    {

        struct BackendEntry
        {
            WindowSystem ws = WindowSystem::Default;
            WindowBackendFactory factory = nullptr;
            int priority = 0;
        };

        std::mutex g_registry_mutex;
        std::array<BackendEntry, 8> g_backends {};
        size_t g_backend_count = 0;

        int windowSystemPriority(WindowSystem ws)
        {
            switch (ws)
            {
                case WindowSystem::Wayland: return 300;
                case WindowSystem::Xlib:    return 200;
                case WindowSystem::Xcb:     return 100;
                case WindowSystem::Cocoa:   return 1000;
                case WindowSystem::Win32:   return 1000;
                default:                    return 0;
            }
        }

    } // namespace

    void registerWindowBackend(WindowSystem ws, WindowBackendFactory factory, int priority)
    {
        if (!factory)
        {
            return;
        }

        std::lock_guard<std::mutex> guard(g_registry_mutex);

        for (size_t i = 0; i < g_backend_count; ++i)
        {
            if (g_backends[i].ws == ws)
            {
                g_backends[i].factory = factory;
                g_backends[i].priority = priority;
                return;
            }
        }

        if (g_backend_count < g_backends.size())
        {
            g_backends[g_backend_count++] = BackendEntry { ws, factory, priority };
        }
    }

    bool isWindowBackendRegistered(WindowSystem ws)
    {
        std::lock_guard<std::mutex> guard(g_registry_mutex);

        for (size_t i = 0; i < g_backend_count; ++i)
        {
            if (g_backends[i].ws == ws)
            {
                return g_backends[i].factory != nullptr;
            }
        }

        return false;
    }

    bool isWindowBackendAvailable(WindowSystem ws)
    {
        return isWindowBackendRegistered(ws);
    }

    WindowSystem firstRegisteredWindowSystem()
    {
        std::lock_guard<std::mutex> guard(g_registry_mutex);

        WindowSystem best = WindowSystem::Default;
        int best_priority = -1;

        for (size_t i = 0; i < g_backend_count; ++i)
        {
            if (g_backends[i].factory && g_backends[i].priority > best_priority)
            {
                best_priority = g_backends[i].priority;
                best = g_backends[i].ws;
            }
        }

        return best;
    }

    std::unique_ptr<WindowBackend> createRegisteredWindowBackend(WindowSystem ws, Window* window,
        int width, int height, u32 flags, const char* title)
    {
        std::lock_guard<std::mutex> guard(g_registry_mutex);

        for (size_t i = 0; i < g_backend_count; ++i)
        {
            if (g_backends[i].ws == ws && g_backends[i].factory)
            {
                return g_backends[i].factory(window, width, height, flags, title);
            }
        }

        return nullptr;
    }

    BackendRegistrar::BackendRegistrar(WindowSystem ws, WindowBackendFactory factory)
    {
        registerWindowBackend(ws, factory, windowSystemPriority(ws));
    }

} // namespace mango
