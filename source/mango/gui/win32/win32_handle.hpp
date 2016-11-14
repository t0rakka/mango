/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

namespace mango
{

    struct WindowHandle
    {
		HWND hwnd { NULL };
        HICON icon { NULL };

		WindowHandle(int width, int height);
		~WindowHandle();
    };

} // namespace mango
