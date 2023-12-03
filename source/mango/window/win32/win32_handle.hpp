/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

#if defined(MANGO_WINDOW_SYSTEM_WIN32)

namespace mango
{

    struct WindowHandle
    {
        WNDCLASSEX wndclass { 0 };
        HWND hwnd { NULL };
        HICON icon { NULL };
        bool is_looping { false };

        WindowHandle(int width, int height, u32 flags);
        ~WindowHandle();
    };

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_WIN32)
