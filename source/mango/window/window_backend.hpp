/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

namespace mango
{

    // -----------------------------------------------------------------------
    // GraphicsSurfaceHooks
    // -----------------------------------------------------------------------

    // Optional hooks installed by OpenGL (EGL) while a graphics context is active.
    // Window backends call syncGraphicsSurface() on resize without depending on EGL.
    struct GraphicsSurfaceHooks
    {
        void* cookie = nullptr;
        void (*destroy)(void* cookie) = nullptr;
        void (*sync)(void* cookie) = nullptr;
        void (*present)(void* cookie) = nullptr;
    };

    // -----------------------------------------------------------------------
    // WindowBackend
    // -----------------------------------------------------------------------

    struct WindowBackend
    {
        Window* owner = nullptr;
        bool busy = false;
        GraphicsSurfaceHooks graphics_hooks;

        WindowBackend() = default;
        virtual ~WindowBackend();

        virtual void setWindowPosition(int x, int y) = 0;
        virtual void setWindowSize(int width, int height) = 0;
        virtual void setTitle(const std::string& title) = 0;
        virtual void setVisible(bool enable) = 0;

        virtual math::int32x2 getWindowSize() const = 0;
        // Client area in window coordinates (logical points on macOS Retina).
        // Differs from getWindowSize() on HiDPI Cocoa; same elsewhere.
        virtual math::int32x2 getClientSize() const = 0;
        virtual math::int32x2 getCursorPosition() const = 0;
        virtual double getDisplayRefreshRate() const = 0;

        virtual void toggleFullscreen() = 0;
        virtual bool isFullscreen() const = 0;

        virtual bool isKeyPressed(Keycode code) const = 0;

        virtual void runEventLoop() = 0;
        virtual void wakeEventLoop() = 0;

        void syncGraphicsSurface();
        void presentGraphicsSurface();
        void clearGraphicsHooks();

        // Called immediately before the graphics API presents a frame (EGL swap,
        // Vulkan vkQueuePresentKHR). Wayland uses this to refresh wl_surface state.
        virtual void beforePresent() {}

        // Graphics-neutral native handles for EGL bridge code in mango-opengl.
        virtual void* nativeDisplay() { return nullptr; }
        virtual void* nativeSurface() { return nullptr; }
        virtual void* createNativeWindowForGraphics(int width, int height, u32 flags);
    };

    WindowSystem resolveWindowSystem(WindowSystem ws);
    std::unique_ptr<WindowBackend> createWindowBackend(WindowSystem ws, Window* window,
        int width, int height, u32 flags, const char* title);

} // namespace mango
