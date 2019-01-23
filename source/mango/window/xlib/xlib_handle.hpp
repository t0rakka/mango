/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/gui/window.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

namespace mango
{

    struct WindowHandle
    {
		// window data
        ::Display* display { NULL };
        ::Window   window { 0 };
        ::Colormap colormap { 0 };
        ::XImage*  icon { NULL };

        // window close event atoms
        ::Atom     atom_protocols;
        ::Atom     atom_delete;

        // fullscreen toggle atoms
        ::Atom     atom_state;
        ::Atom     atom_fullscreen;

        // primary atom
        ::Atom     atom_primary;

        // xdnd atoms
        ::Atom     atom_xdnd_Aware;
        ::Atom     atom_xdnd_Enter;
        ::Atom     atom_xdnd_Position;
        ::Atom     atom_xdnd_Status;
        ::Atom     atom_xdnd_TypeList;
        ::Atom     atom_xdnd_ActionCopy;
        ::Atom     atom_xdnd_Drop;
        ::Atom     atom_xdnd_Finished;
        ::Atom     atom_xdnd_Selection;
        ::Atom     atom_xdnd_Leave;
        ::Atom     atom_xdnd_req;

        ::Window   xdnd_source;
        int        xdnd_version { 0 };
        int        size[2] = { 0, 0 };
        float      mouse_time[6];
        bool       looping { false };
        bool       busy { false };

        WindowHandle(int width, int height);
        ~WindowHandle();

        bool createWindow(XVisualInfo* vi, int width, int height, const char* title);
    };

} // namespace mango
