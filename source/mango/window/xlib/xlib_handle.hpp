/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

namespace mango
{

    struct WindowHandle
    {
        NativeWindowHandle native;

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
        bool        busy { false };

        u32         flags;

        WindowHandle(int width, int height, u32 flags);
        ~WindowHandle();

        bool createXWindow(int screen, int depth, Visual* visual, int width, int height, const char* title);
    };

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_XLIB)
