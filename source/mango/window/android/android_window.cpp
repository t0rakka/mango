/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/window/window.hpp>

#if defined(MANGO_WINDOW_SYSTEM_ANDROID)

namespace mango
{

    struct WindowContext
    {
        ANativeWindow* window;

        WindowContext(int width, int height);
        ~WindowContext();
    };

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_ANDROID)
