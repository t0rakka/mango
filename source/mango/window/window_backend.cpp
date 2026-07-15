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

    float WindowBackend::getContentScale() const
    {
        const math::int32x2 layout = getClientSize();
        const math::int32x2 drawable = getWindowSize();

        if (layout.x <= 0 || layout.y <= 0)
        {
            return 1.0f;
        }

        const float sx = float(drawable.x) / float(layout.x);
        const float sy = float(drawable.y) / float(layout.y);
        return (sx + sy) * 0.5f;
    }

} // namespace mango
