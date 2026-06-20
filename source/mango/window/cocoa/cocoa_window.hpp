/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

namespace mango
{

    struct WindowContext : WindowHandle
    {
        void* ns_window { nullptr };
        void* ns_delegate { nullptr };
        void* content_view { nullptr };

        bool is_looping { false };
        bool fullscreen { false };
        bool is_opengl { false };
        bool is_vulkan { false };

        u32 keystate[4] = { 0, 0, 0, 0 };

        WindowContext() = default;
        ~WindowContext();

        bool init(Window* owner, int width, int height, u32 flags, const char* title);
        void attachContentView(Window* owner, void* view);
        void shutdown();

        void toggleFullscreen();
        math::int32x2 getContentSize() const;
        void updateMetalDrawableSize();
    };

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_COCOA)
