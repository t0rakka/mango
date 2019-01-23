/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>
#include <xcb/xcb.h>

namespace mango
{

    struct WindowHandle
    {
        xcb_connection_t* connection;
        xcb_window_t window;

        bool looping { false };

        WindowHandle(int width, int height);
        ~WindowHandle();
    };

} // namespace mango
