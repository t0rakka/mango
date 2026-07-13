/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>
#include "../window_backend.hpp"

#if defined(MANGO_ENABLE_COCOA)

namespace mango
{

    struct WindowContext : WindowBackend
    {
        void* layer { nullptr };        // CAMetalLayer*, set when created with API_VULKAN
        void* ns_window { nullptr };
        void* ns_delegate { nullptr };
        void* content_view { nullptr };

        bool is_looping { false };
        bool fullscreen { false };
        bool is_opengl { false };
        bool is_vulkan { false };

        u32 keystate[4] = { 0, 0, 0, 0 };

        WindowContext() = default;
        ~WindowContext() override;

        bool init(Window* owner, int width, int height, u32 flags, const char* title);
        void attachContentView(Window* owner, void* view);
        void shutdown();

        math::int32x2 getContentSize() const;
        void updateMetalDrawableSize();

        // WindowBackend interface
        void setWindowPosition(int x, int y) override;
        void setWindowSize(int width, int height) override;
        void setTitle(const std::string& title) override;
        void setVisible(bool enable) override;
        math::int32x2 getWindowSize() const override;
        math::int32x2 getClientSize() const override;
        math::int32x2 getCursorPosition() const override;
        double getDisplayRefreshRate() const override;
        void toggleFullscreen() override;
        bool isFullscreen() const override;
        bool isKeyPressed(Keycode code) const override;
        void runEventLoop() override;
        void wakeEventLoop() override;
    };

} // namespace mango

#endif // defined(MANGO_ENABLE_COCOA)
