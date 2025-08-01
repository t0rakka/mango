/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

#if defined(MANGO_WINDOW_SYSTEM_XCB)

#include <unistd.h>
#include <mango/math/math.hpp>

#define explicit explicit_
#include <sys/types.h>
#include <sys/stat.h>
#include <xcb/xproto.h>
#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#undef explicit

namespace mango
{

    struct WindowContext : WindowHandle
    {
        // window data
        xcb_window_t root { 0 };
        xcb_colormap_t colormap { 0 };
        xcb_pixmap_t icon_pixmap { 0 };
        xcb_pixmap_t icon_mask { 0 };

        // window close event atoms
        xcb_atom_t atom_protocols;
        xcb_atom_t atom_delete;

        // fullscreen toggle atoms
        xcb_atom_t atom_state;
        xcb_atom_t atom_fullscreen;

        // primary atom
        xcb_atom_t atom_primary;

        // xdnd atoms
        xcb_atom_t atom_xdnd_Aware;
        xcb_atom_t atom_xdnd_Enter;
        xcb_atom_t atom_xdnd_Position;
        xcb_atom_t atom_xdnd_Status;
        xcb_atom_t atom_xdnd_TypeList;
        xcb_atom_t atom_xdnd_ActionCopy;
        xcb_atom_t atom_xdnd_Drop;
        xcb_atom_t atom_xdnd_Finished;
        xcb_atom_t atom_xdnd_Selection;
        xcb_atom_t atom_xdnd_Leave;
        xcb_atom_t atom_xdnd_req;

        xcb_window_t xdnd_source;
        int xdnd_version { 0 };
        int size[2] = { 0, 0 };
        u64 mouse_time[6];
        bool is_looping { false };
        bool busy { false };
        bool fullscreen { false };

        xcb_key_symbols_t* key_symbols;

        WindowContext();
        ~WindowContext();

        operator xcb_window_t () const { return window; }

        bool init(int width, int height, u32 flags, const char* title);
        void toggleFullscreen();
        math::int32x2 getWindowSize() const;
    };

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_XCB)
