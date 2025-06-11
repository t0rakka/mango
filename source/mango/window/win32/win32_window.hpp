/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

#if defined(MANGO_WINDOW_SYSTEM_WIN32)

namespace mango
{

    struct WindowContext
    {
        WNDCLASSEX wndclass{ 0 };
        HWND hwnd { NULL };
        HICON icon{  NULL };
        bool is_looping { false };

        bool fullscreen { false };
        RECT rect;

        WindowContext(int width, int height, u32 flags);
        ~WindowContext();

        void toggleFullscreen();
    };

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_WIN32)
