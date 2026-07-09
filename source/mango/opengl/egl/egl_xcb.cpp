/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "egl_surface.hpp"

#include "../../window/window_backend.hpp"
#include "../../window/xcb/xcb_window.hpp"

namespace mango::opengl::egl
{

    NativeWindowBinding createXcbNativeWindow(WindowBackend* backend, int width, int height, u32 flags)
    {
        NativeWindowBinding result;

        auto* xcb = static_cast<XcbBackend*>(backend);
        if (!xcb)
        {
            return result;
        }

        void* native = xcb->createNativeWindowForGraphics(width, height, flags);
        if (!native)
        {
            return result;
        }

        result.native_window = native;
        return result;
    }

} // namespace mango::opengl::egl
