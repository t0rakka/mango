/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <memory>
#include <mango/window/window.hpp>

namespace mango
{

    struct WindowBackend;

    using WindowBackendFactory = std::unique_ptr<WindowBackend> (*)(Window* window, int width, int height, u32 flags, const char* title);

    void registerWindowBackend(WindowSystem ws, WindowBackendFactory factory, int priority);
    bool isWindowBackendRegistered(WindowSystem ws);
    WindowSystem firstRegisteredWindowSystem();
    std::unique_ptr<WindowBackend> createRegisteredWindowBackend(WindowSystem ws, Window* window,
        int width, int height, u32 flags, const char* title);

    struct BackendRegistrar
    {
        BackendRegistrar(WindowSystem ws, WindowBackendFactory factory);
    };

} // namespace mango

#define MANGO_REGISTER_WINDOW_BACKEND(System, Factory) \
    namespace { \
        const ::mango::BackendRegistrar g_mango_window_backend_##Factory( \
            ::mango::WindowSystem::System, &::mango::Factory); \
    }
