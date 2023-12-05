/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

namespace mango
{

    struct WindowHandle
    {
        NativeWindowHandle native;

        WindowHandle(int width, int height);
        ~WindowHandle();
    };

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_WAYLAND)
