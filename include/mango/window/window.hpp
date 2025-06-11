/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <mango/math/math.hpp>
#include <mango/image/surface.hpp>
#include <mango/filesystem/filesystem.hpp>

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_WIN32
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_WIN32)

namespace mango
{

    struct WindowHandle
    {
        HWND hwnd;
    };

} // namespace mango

#endif

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_COCOA
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

namespace mango
{

    using WindowHandle = void*;

} // namespace mango

#endif

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_XLIB
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

    #include <X11/Xlib.h>

    #ifdef Status
        #undef Status
        using Status = int;
    #endif

namespace mango
{

    struct WindowHandle
    {
        ::Display* display { nullptr };
        ::Window window { 0 };
        ::VisualID visualid { 0 };
    };

} // namespace mango

#endif

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_XCB
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_XCB)

    #include <xcb/xcb.h>

namespace mango
{

    struct WindowHandle
    {
        xcb_connection_t* connection { nullptr };
        xcb_window_t window { 0 };
        xcb_visualid_t visualid { 0 };
    };

} // namespace mango

#endif

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_WAYLAND
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

    #include <wayland-client.h>
    #include <wayland-client-protocol.h>

namespace mango
{

    struct WindowHandle
    {
        wl_display* display { nullptr };
        wl_surface* surface { nullptr };
    };

} // namespace mango

#endif

#if !defined(MANGO_WINDOW_SYSTEM_NONE)

namespace mango
{

    enum Keycode
    {
        KEYCODE_NONE,
        KEYCODE_ESC,
        KEYCODE_0,
        KEYCODE_1,
        KEYCODE_2,
        KEYCODE_3,
        KEYCODE_4,
        KEYCODE_5,
        KEYCODE_6,
        KEYCODE_7,
        KEYCODE_8,
        KEYCODE_9,
        KEYCODE_A,
        KEYCODE_B,
        KEYCODE_C,
        KEYCODE_D,
        KEYCODE_E,
        KEYCODE_F,
        KEYCODE_G,
        KEYCODE_H,
        KEYCODE_I,
        KEYCODE_J,
        KEYCODE_K,
        KEYCODE_L,
        KEYCODE_M,
        KEYCODE_N,
        KEYCODE_O,
        KEYCODE_P,
        KEYCODE_Q,
        KEYCODE_R,
        KEYCODE_S,
        KEYCODE_T,
        KEYCODE_U,
        KEYCODE_V,
        KEYCODE_W,
        KEYCODE_X,
        KEYCODE_Y,
        KEYCODE_Z,
        KEYCODE_F1,
        KEYCODE_F2,
        KEYCODE_F3,
        KEYCODE_F4,
        KEYCODE_F5,
        KEYCODE_F6,
        KEYCODE_F7,
        KEYCODE_F8,
        KEYCODE_F9,
        KEYCODE_F10,
        KEYCODE_F11,
        KEYCODE_F12,
        KEYCODE_BACKSPACE,
        KEYCODE_TAB,
        KEYCODE_RETURN,
        KEYCODE_LEFT_ALT,
        KEYCODE_RIGHT_ALT,
        KEYCODE_SPACE,
        KEYCODE_CAPS_LOCK,
        KEYCODE_PAGE_UP,
        KEYCODE_PAGE_DOWN,
        KEYCODE_INSERT,
        KEYCODE_DELETE,
        KEYCODE_HOME,
        KEYCODE_END,
        KEYCODE_LEFT,
        KEYCODE_RIGHT,
        KEYCODE_UP,
        KEYCODE_DOWN,
        KEYCODE_PRINT_SCREEN,
        KEYCODE_SCROLL_LOCK,
        KEYCODE_NUMPAD0,
        KEYCODE_NUMPAD1,
        KEYCODE_NUMPAD2,
        KEYCODE_NUMPAD3,
        KEYCODE_NUMPAD4,
        KEYCODE_NUMPAD5,
        KEYCODE_NUMPAD6,
        KEYCODE_NUMPAD7,
        KEYCODE_NUMPAD8,
        KEYCODE_NUMPAD9,
        KEYCODE_NUMLOCK,
        KEYCODE_DIVIDE,
        KEYCODE_MULTIPLY,
        KEYCODE_SUBTRACT,
        KEYCODE_ADDITION,
        KEYCODE_ENTER,
        KEYCODE_DECIMAL,
        KEYCODE_SHIFT,
        KEYCODE_LEFT_SHIFT,
        KEYCODE_RIGHT_SHIFT,
        KEYCODE_CONTROL,
        KEYCODE_LEFT_CONTROL,
        KEYCODE_RIGHT_CONTROL,
    };

    enum
    {
        KEYMASK_CONTROL = 0x0001,
        KEYMASK_SHIFT   = 0x0002,
        KEYMASK_SUPER   = 0x0004,
        KEYMASK_MENU    = 0x0008
    };

    enum MouseButton
    {
        MOUSEBUTTON_LEFT,
        MOUSEBUTTON_RIGHT,
        MOUSEBUTTON_MIDDLE,
        MOUSEBUTTON_X1,
        MOUSEBUTTON_X2,
        MOUSEBUTTON_WHEEL
    };

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    class Window : public NonCopyable
    {
    protected:
        std::unique_ptr<struct WindowContext> m_window_context;

    public:
        enum : u32
        {
            DISABLE_RESIZE  = 0x00000001,
            API_OPENGL      = 0x00010000,
            API_EGL         = 0x00020000,
        };

        Window(int width, int height, u32 flags = 0);
        virtual ~Window();

        static int getScreenCount();
        static math::int32x2 getScreenSize(int screen = 0);

        operator WindowHandle () const;
        operator struct WindowContext* () const;

        void setWindowPosition(int x, int y);
        void setWindowSize(int width, int height);
        void setTitle(const std::string& title);
        void setVisible(bool enable);

        virtual math::int32x2 getWindowSize() const;
        virtual math::int32x2 getCursorPosition() const;
        virtual bool isKeyPressed(Keycode code) const;

        void enterEventLoop();
        void breakEventLoop();

        virtual void onIdle();
        virtual void onDraw();
        virtual void onResize(int width, int height);
        virtual void onMinimize();
        virtual void onMaximize();
        virtual void onKeyPress(Keycode code, u32 mask);
        virtual void onKeyRelease(Keycode code);
        virtual void onMouseMove(int x, int y);
        virtual void onMouseClick(int x, int y, MouseButton button, int count);
        virtual void onDropFiles(const filesystem::FileIndex& index);
        virtual void onClose();
        virtual void onShow();
        virtual void onHide();
    };

} // namespace mango

#endif // !defined(MANGO_WINDOW_SYSTEM_NONE)
