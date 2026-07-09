/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>
#include "../window_backend.hpp"

namespace mango
{

    struct WindowContext : WindowBackend
    {
        HWND hwnd { nullptr };

        WNDCLASSEX wndclass{ 0 };
        bool is_looping { false };
        bool fullscreen { false };
        bool is_opengl { false };
        RECT rect;

        WindowContext(int width, int height, u32 flags);
        ~WindowContext() override;

        // WindowBackend interface
        void setWindowPosition(int x, int y) override;
        void setWindowSize(int width, int height) override;
        void setTitle(const std::string& title) override;
        void setVisible(bool enable) override;
        math::int32x2 getWindowSize() const override;
        math::int32x2 getCursorPosition() const override;
        double getDisplayRefreshRate() const override;
        void toggleFullscreen() override;
        bool isFullscreen() const override;
        bool isKeyPressed(Keycode code) const override;
        void runEventLoop() override;
        void wakeEventLoop() override;
    };

} // namespace mango
