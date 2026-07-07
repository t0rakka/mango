/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "window_backend.hpp"

namespace mango
{

    WindowBackend::~WindowBackend()
    {
        clearGraphicsHooks();
    }

    void WindowBackend::syncGraphicsSurface()
    {
        if (graphics_hooks.sync)
        {
            graphics_hooks.sync(graphics_hooks.cookie);
        }
    }

    void WindowBackend::presentGraphicsSurface()
    {
        if (graphics_hooks.present)
        {
            graphics_hooks.present(graphics_hooks.cookie);
        }
    }

    void WindowBackend::clearGraphicsHooks()
    {
        if (graphics_hooks.destroy)
        {
            graphics_hooks.destroy(graphics_hooks.cookie);
        }

        graphics_hooks = {};
    }

    void* WindowBackend::createNativeWindowForGraphics(int width, int height, u32 flags)
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
        MANGO_UNREFERENCED(flags);
        return nullptr;
    }

} // namespace mango
