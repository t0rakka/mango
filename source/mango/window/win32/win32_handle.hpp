/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

namespace mango
{

    struct WindowHandle
    {
        WNDCLASSEX wndclass { 0 };
		HWND hwnd { NULL };
        HICON icon { NULL };
		bool looping { false };

		WindowHandle(int width, int height, u32 flags);
		~WindowHandle();
    };

} // namespace mango
