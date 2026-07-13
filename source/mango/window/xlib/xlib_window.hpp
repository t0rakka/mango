/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>
#include "../window_backend.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mango/math/math.hpp>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/sync.h>

namespace mango
{

    // Concrete Xlib backend. Distinct class name (vs XcbBackend / WaylandBackend)
    // so all three Linux backends can be linked into one binary without ODR clash;
    // the native Xlib headers live only in this and the GLX-Xlib translation unit.
    struct XlibBackend : WindowBackend
    {
        // native handle (formerly in the public WindowHandle)
        void*           display { nullptr };
        unsigned long   window { 0 };
        unsigned long   visualid { 0 };

        // window data
        ::Colormap  x11_colormap { 0 };
        ::Visual*   x11_visual { nullptr };
        ::XImage*   x11_icon { nullptr };

        // window close event atoms
        ::Atom      atom_protocols;
        ::Atom      atom_delete;

        // fullscreen toggle atoms
        ::Atom      atom_state;
        ::Atom      atom_fullscreen;

        // primary atom
        ::Atom      atom_primary;

        // xdnd atoms
        ::Atom      atom_xdnd_Aware;
        ::Atom      atom_xdnd_Enter;
        ::Atom      atom_xdnd_Position;
        ::Atom      atom_xdnd_Status;
        ::Atom      atom_xdnd_TypeList;
        ::Atom      atom_xdnd_ActionCopy;
        ::Atom      atom_xdnd_Drop;
        ::Atom      atom_xdnd_Finished;
        ::Atom      atom_xdnd_Selection;
        ::Atom      atom_xdnd_Leave;
        ::Atom      atom_xdnd_req;

        ::Window    xdnd_source;
        int         xdnd_version { 0 };
        int         size[2] = { 0, 0 };
        u64         mouse_time[6];
        bool        is_looping { false };
        bool        fullscreen  { false };

        // _NET_WM_SYNC_REQUEST: lets the compositor hold the resized frame until the
        // client has drawn it, eliminating the undefined/black border that otherwise
        // appears while the window grows faster than the swapchain catches up.
        ::Atom          atom_sync_request { 0 };   // _NET_WM_SYNC_REQUEST
        ::Atom          atom_sync_counter { 0 };   // _NET_WM_SYNC_REQUEST_COUNTER
        XSyncCounter    sync_counter { 0 };
        XSyncValue      sync_value;
        bool            sync_pending { false };
        bool            sync_supported { false };
        bool            resize_pending { false };

        XlibBackend();
        ~XlibBackend() override;

        ::Display* x11Display() const { return static_cast<::Display*>(display); }
        ::Window x11Window() const { return static_cast<::Window>(window); }
        ::VisualID x11VisualID() const { return static_cast<::VisualID>(visualid); }

        operator ::Window () const { return x11Window(); }

        bool init(int screen, int depth, Visual* visual, int width, int height, u32 flags, const char* title);

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

        void* createNativeWindowForGraphics(int width, int height, u32 flags) override;
    };

} // namespace mango
