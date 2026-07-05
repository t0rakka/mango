/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/configure.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/core/timer.hpp>

#if defined(MANGO_ENABLE_XCB)

#if defined(MANGO_ENABLE_VULKAN)
    #define VK_USE_PLATFORM_XCB_KHR
#endif

#include <unistd.h>
#include <poll.h>
#include <cstdint>

#define explicit explicit_
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xkb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/sync.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xcb/randr.h>
#undef explicit

#include "xcb_window.hpp"

namespace
{
    using namespace mango;
    using namespace mango::filesystem;

    struct ScreenInfo
    {
        xcb_screen_t* screen;
        math::int32x2 resolution;
    };

    std::vector<ScreenInfo> getScreenInfo(xcb_connection_t* connection)
    {
        std::vector<ScreenInfo> screens;

        xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
        while (screen_iter.rem)
        {
            xcb_screen_t* screen = screen_iter.data;

            int width = screen_iter.data->width_in_pixels;
            int height = screen_iter.data->height_in_pixels;

            ScreenInfo info;
            info.screen = screen;
            info.resolution = math::int32x2(width, height);
            screens.push_back(info);

            xcb_screen_next(&screen_iter);
        }

        return screens;
    }

    double queryXcbRefreshRate(xcb_connection_t* conn, xcb_window_t win)
    {
        xcb_randr_query_version_cookie_t version_cookie = xcb_randr_query_version(conn, 1, 6);
        xcb_randr_query_version_reply_t* version_reply = xcb_randr_query_version_reply(conn, version_cookie, nullptr);
        if (!version_reply)
        {
            return 0.0;
        }
        free(version_reply);

        xcb_randr_get_screen_resources_current_cookie_t resources_cookie =
            xcb_randr_get_screen_resources_current(conn, win);
        xcb_randr_get_screen_resources_current_reply_t* resources =
            xcb_randr_get_screen_resources_current_reply(conn, resources_cookie, nullptr);
        if (!resources)
        {
            return 0.0;
        }

        xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(conn, win);
        xcb_get_geometry_reply_t* geom = xcb_get_geometry_reply(conn, geom_cookie, nullptr);
        if (!geom)
        {
            free(resources);
            return 0.0;
        }

        xcb_translate_coordinates_cookie_t trans_cookie =
            xcb_translate_coordinates(conn, win, geom->root, geom->width / 2, geom->height / 2);
        xcb_translate_coordinates_reply_t* trans =
            xcb_translate_coordinates_reply(conn, trans_cookie, nullptr);

        double refresh = 0.0;
        xcb_randr_output_t chosen = XCB_NONE;

        if (trans)
        {
            const int root_x = trans->dst_x;
            const int root_y = trans->dst_y;

            xcb_randr_output_t* outputs = xcb_randr_get_screen_resources_current_outputs(resources);
            const int noutputs = xcb_randr_get_screen_resources_current_outputs_length(resources);

            for (int i = 0; i < noutputs; ++i)
            {
                xcb_randr_get_output_info_cookie_t output_cookie =
                    xcb_randr_get_output_info(conn, outputs[i], XCB_CURRENT_TIME);
                xcb_randr_get_output_info_reply_t* output =
                    xcb_randr_get_output_info_reply(conn, output_cookie, nullptr);
                if (!output || output->connection != XCB_RANDR_CONNECTION_CONNECTED)
                {
                    free(output);
                    continue;
                }

                xcb_randr_crtc_t* crtcs = xcb_randr_get_output_info_crtcs(output);
                const int ncrtcs = xcb_randr_get_output_info_crtcs_length(output);
                bool matched = false;

                for (int j = 0; j < ncrtcs; ++j)
                {
                    xcb_randr_get_crtc_info_cookie_t crtc_cookie =
                        xcb_randr_get_crtc_info(conn, crtcs[j], XCB_CURRENT_TIME);
                    xcb_randr_get_crtc_info_reply_t* crtc =
                        xcb_randr_get_crtc_info_reply(conn, crtc_cookie, nullptr);
                    if (!crtc || crtc->mode == XCB_NONE)
                    {
                        free(crtc);
                        continue;
                    }

                    if (root_x >= crtc->x && root_x < int(crtc->x + crtc->width) &&
                        root_y >= crtc->y && root_y < int(crtc->y + crtc->height))
                    {
                        chosen = outputs[i];
                        matched = true;
                    }

                    free(crtc);
                    if (matched)
                    {
                        break;
                    }
                }

                free(output);
                if (matched)
                {
                    break;
                }
            }
        }

        if (chosen == XCB_NONE)
        {
            xcb_randr_get_output_primary_cookie_t primary_cookie =
                xcb_randr_get_output_primary(conn, geom->root);
            xcb_randr_get_output_primary_reply_t* primary =
                xcb_randr_get_output_primary_reply(conn, primary_cookie, nullptr);
            if (primary)
            {
                chosen = primary->output;
                free(primary);
            }
        }

        if (chosen != XCB_NONE)
        {
            xcb_randr_get_output_info_cookie_t output_cookie =
                xcb_randr_get_output_info(conn, chosen, XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t* output =
                xcb_randr_get_output_info_reply(conn, output_cookie, nullptr);
            if (output && output->crtc != XCB_NONE)
            {
                xcb_randr_get_crtc_info_cookie_t crtc_cookie =
                    xcb_randr_get_crtc_info(conn, output->crtc, XCB_CURRENT_TIME);
                xcb_randr_get_crtc_info_reply_t* crtc =
                    xcb_randr_get_crtc_info_reply(conn, crtc_cookie, nullptr);
                if (crtc && crtc->mode != XCB_NONE)
                {
                    xcb_randr_mode_info_t* modes = xcb_randr_get_screen_resources_current_modes(resources);
                    const int nmodes = xcb_randr_get_screen_resources_current_modes_length(resources);
                    for (int i = 0; i < nmodes; ++i)
                    {
                        if (modes[i].id == crtc->mode)
                        {
                            const xcb_randr_mode_info_t& mode = modes[i];
                            if (mode.htotal > 0 && mode.vtotal > 0)
                            {
                                refresh = double(mode.dot_clock) / double(mode.htotal * mode.vtotal);
                            }
                            break;
                        }
                    }
                }
                free(crtc);
            }
            free(output);
        }

        free(trans);
        free(geom);
        free(resources);
        return refresh;
    }

#define TR(a, b) case a: code = b; break

    Keycode translateEventToKeycode(xcb_keysym_t symbol)
    {
        Keycode code;

        switch (symbol)
        {
            TR(XKB_KEY_Escape,         KEYCODE_ESC);
            TR(XKB_KEY_0,              KEYCODE_0);
            TR(XKB_KEY_1,              KEYCODE_1);
            TR(XKB_KEY_2,              KEYCODE_2);
            TR(XKB_KEY_3,              KEYCODE_3);
            TR(XKB_KEY_4,              KEYCODE_4);
            TR(XKB_KEY_5,              KEYCODE_5);
            TR(XKB_KEY_6,              KEYCODE_6);
            TR(XKB_KEY_7,              KEYCODE_7);
            TR(XKB_KEY_8,              KEYCODE_8);
            TR(XKB_KEY_9,              KEYCODE_9);
            TR(XKB_KEY_a,              KEYCODE_A);
            TR(XKB_KEY_b,              KEYCODE_B);
            TR(XKB_KEY_c,              KEYCODE_C);
            TR(XKB_KEY_d,              KEYCODE_D);
            TR(XKB_KEY_e,              KEYCODE_E);
            TR(XKB_KEY_f,              KEYCODE_F);
            TR(XKB_KEY_g,              KEYCODE_G);
            TR(XKB_KEY_h,              KEYCODE_H);
            TR(XKB_KEY_i,              KEYCODE_I);
            TR(XKB_KEY_j,              KEYCODE_J);
            TR(XKB_KEY_k,              KEYCODE_K);
            TR(XKB_KEY_l,              KEYCODE_L);
            TR(XKB_KEY_m,              KEYCODE_M);
            TR(XKB_KEY_n,              KEYCODE_N);
            TR(XKB_KEY_o,              KEYCODE_O);
            TR(XKB_KEY_p,              KEYCODE_P);
            TR(XKB_KEY_q,              KEYCODE_Q);
            TR(XKB_KEY_r,              KEYCODE_R);
            TR(XKB_KEY_s,              KEYCODE_S);
            TR(XKB_KEY_t,              KEYCODE_T);
            TR(XKB_KEY_u,              KEYCODE_U);
            TR(XKB_KEY_v,              KEYCODE_V);
            TR(XKB_KEY_w,              KEYCODE_W);
            TR(XKB_KEY_x,              KEYCODE_X);
            TR(XKB_KEY_y,              KEYCODE_Y);
            TR(XKB_KEY_z,              KEYCODE_Z);
            TR(XKB_KEY_F1,             KEYCODE_F1);
            TR(XKB_KEY_F2,             KEYCODE_F2);
            TR(XKB_KEY_F3,             KEYCODE_F3);
            TR(XKB_KEY_F4,             KEYCODE_F4);
            TR(XKB_KEY_F5,             KEYCODE_F5);
            TR(XKB_KEY_F6,             KEYCODE_F6);
            TR(XKB_KEY_F7,             KEYCODE_F7);
            TR(XKB_KEY_F8,             KEYCODE_F8);
            TR(XKB_KEY_F9,             KEYCODE_F9);
            TR(XKB_KEY_F10,            KEYCODE_F10);
            TR(XKB_KEY_F11,            KEYCODE_F11);
            TR(XKB_KEY_F12,            KEYCODE_F12);
            TR(XKB_KEY_BackSpace,      KEYCODE_BACKSPACE);
            TR(XKB_KEY_Tab,            KEYCODE_TAB);
            TR(XKB_KEY_Return,         KEYCODE_RETURN);
            TR(XKB_KEY_Alt_L,          KEYCODE_LEFT_ALT);
            TR(XKB_KEY_Alt_R,          KEYCODE_RIGHT_ALT);
            TR(XKB_KEY_space,          KEYCODE_SPACE);
            TR(XKB_KEY_Caps_Lock,      KEYCODE_CAPS_LOCK);
            TR(XKB_KEY_Page_Up,        KEYCODE_PAGE_UP);
            TR(XKB_KEY_Page_Down,      KEYCODE_PAGE_DOWN);
            TR(XKB_KEY_Insert,         KEYCODE_INSERT);
            TR(XKB_KEY_Delete,         KEYCODE_DELETE);
            TR(XKB_KEY_Home,           KEYCODE_HOME);
            TR(XKB_KEY_End,            KEYCODE_END);
            TR(XKB_KEY_Left,           KEYCODE_LEFT);
            TR(XKB_KEY_Right,          KEYCODE_RIGHT);
            TR(XKB_KEY_Up,             KEYCODE_UP);
            TR(XKB_KEY_Down,           KEYCODE_DOWN);
            TR(XKB_KEY_Print,          KEYCODE_PRINT_SCREEN);
            TR(XKB_KEY_Scroll_Lock,    KEYCODE_SCROLL_LOCK);
            TR(XKB_KEY_KP_0,           KEYCODE_NUMPAD0);
            TR(XKB_KEY_KP_1,           KEYCODE_NUMPAD1);
            TR(XKB_KEY_KP_2,           KEYCODE_NUMPAD2);
            TR(XKB_KEY_KP_3,           KEYCODE_NUMPAD3);
            TR(XKB_KEY_KP_4,           KEYCODE_NUMPAD4);
            TR(XKB_KEY_KP_5,           KEYCODE_NUMPAD5);
            TR(XKB_KEY_KP_6,           KEYCODE_NUMPAD6);
            TR(XKB_KEY_KP_7,           KEYCODE_NUMPAD7);
            TR(XKB_KEY_KP_8,           KEYCODE_NUMPAD8);
            TR(XKB_KEY_KP_9,           KEYCODE_NUMPAD9);
            TR(XKB_KEY_Num_Lock,       KEYCODE_NUMLOCK);
            TR(XKB_KEY_KP_Divide,      KEYCODE_DIVIDE);
            TR(XKB_KEY_KP_Multiply,    KEYCODE_MULTIPLY);
            TR(XKB_KEY_KP_Subtract,    KEYCODE_SUBTRACT);
            TR(XKB_KEY_KP_Add,         KEYCODE_ADDITION);
            TR(XKB_KEY_KP_Enter,       KEYCODE_ENTER);
            TR(XKB_KEY_KP_Decimal,     KEYCODE_DECIMAL);
            TR(XKB_KEY_Shift_L,        KEYCODE_LEFT_SHIFT);
            TR(XKB_KEY_Shift_R,        KEYCODE_RIGHT_SHIFT);
            TR(XKB_KEY_Control_L,      KEYCODE_LEFT_CONTROL);
            TR(XKB_KEY_Control_R,      KEYCODE_RIGHT_CONTROL);

            default:
                code = KEYCODE_NONE;
                break;
        }

        return code;
    }

#undef TR
#define TR(a, b) case b: symbol = a; break

    xcb_keysym_t translateKeycodeToSymbol(Keycode code)
    {
        xcb_keysym_t symbol;

        switch (code)
        {
            default:
            TR(255,               KEYCODE_NONE);
            TR(XKB_KEY_Escape,     KEYCODE_ESC);
            TR(XKB_KEY_0,          KEYCODE_0);
            TR(XKB_KEY_1,          KEYCODE_1);
            TR(XKB_KEY_2,          KEYCODE_2);
            TR(XKB_KEY_3,          KEYCODE_3);
            TR(XKB_KEY_4,          KEYCODE_4);
            TR(XKB_KEY_5,          KEYCODE_5);
            TR(XKB_KEY_6,          KEYCODE_6);
            TR(XKB_KEY_7,          KEYCODE_7);
            TR(XKB_KEY_8,          KEYCODE_8);
            TR(XKB_KEY_9,          KEYCODE_9);
            TR(XKB_KEY_a,          KEYCODE_A);
            TR(XKB_KEY_b,          KEYCODE_B);
            TR(XKB_KEY_c,          KEYCODE_C);
            TR(XKB_KEY_d,          KEYCODE_D);
            TR(XKB_KEY_e,          KEYCODE_E);
            TR(XKB_KEY_f,          KEYCODE_F);
            TR(XKB_KEY_g,          KEYCODE_G);
            TR(XKB_KEY_h,          KEYCODE_H);
            TR(XKB_KEY_i,          KEYCODE_I);
            TR(XKB_KEY_j,          KEYCODE_J);
            TR(XKB_KEY_k,          KEYCODE_K);
            TR(XKB_KEY_l,          KEYCODE_L);
            TR(XKB_KEY_m,          KEYCODE_M);
            TR(XKB_KEY_n,          KEYCODE_N);
            TR(XKB_KEY_o,          KEYCODE_O);
            TR(XKB_KEY_p,          KEYCODE_P);
            TR(XKB_KEY_q,          KEYCODE_Q);
            TR(XKB_KEY_r,          KEYCODE_R);
            TR(XKB_KEY_s,          KEYCODE_S);
            TR(XKB_KEY_t,          KEYCODE_T);
            TR(XKB_KEY_u,          KEYCODE_U);
            TR(XKB_KEY_v,          KEYCODE_V);
            TR(XKB_KEY_w,          KEYCODE_W);
            TR(XKB_KEY_x,          KEYCODE_X);
            TR(XKB_KEY_y,          KEYCODE_Y);
            TR(XKB_KEY_z,          KEYCODE_Z);
            TR(XKB_KEY_F1,         KEYCODE_F1);
            TR(XKB_KEY_F2,         KEYCODE_F2);
            TR(XKB_KEY_F3,         KEYCODE_F3);
            TR(XKB_KEY_F4,         KEYCODE_F4);
            TR(XKB_KEY_F5,         KEYCODE_F5);
            TR(XKB_KEY_F6,         KEYCODE_F6);
            TR(XKB_KEY_F7,         KEYCODE_F7);
            TR(XKB_KEY_F8,         KEYCODE_F8);
            TR(XKB_KEY_F9,         KEYCODE_F9);
            TR(XKB_KEY_F10,        KEYCODE_F10);
            TR(XKB_KEY_F11,        KEYCODE_F11);
            TR(XKB_KEY_F12,        KEYCODE_F12);
            TR(XKB_KEY_BackSpace,  KEYCODE_BACKSPACE);
            TR(XKB_KEY_Tab,        KEYCODE_TAB);
            TR(XKB_KEY_Return,     KEYCODE_RETURN);
            TR(XKB_KEY_Alt_L,      KEYCODE_LEFT_ALT);
            TR(XKB_KEY_Alt_R,      KEYCODE_RIGHT_ALT);
            TR(XKB_KEY_space,      KEYCODE_SPACE);
            TR(XKB_KEY_Caps_Lock,  KEYCODE_CAPS_LOCK);
            TR(XKB_KEY_Page_Up,    KEYCODE_PAGE_UP);
            TR(XKB_KEY_Page_Down,  KEYCODE_PAGE_DOWN);
            TR(XKB_KEY_Insert,     KEYCODE_INSERT);
            TR(XKB_KEY_Delete,     KEYCODE_DELETE);
            TR(XKB_KEY_Home,       KEYCODE_HOME);
            TR(XKB_KEY_End,        KEYCODE_END);
            TR(XKB_KEY_Left,       KEYCODE_LEFT);
            TR(XKB_KEY_Right,      KEYCODE_RIGHT);
            TR(XKB_KEY_Up,         KEYCODE_UP);
            TR(XKB_KEY_Down,       KEYCODE_DOWN);
            TR(XKB_KEY_Print,      KEYCODE_PRINT_SCREEN);
            TR(XKB_KEY_Scroll_Lock,KEYCODE_SCROLL_LOCK);
            TR(XKB_KEY_KP_0,       KEYCODE_NUMPAD0);
            TR(XKB_KEY_KP_1,       KEYCODE_NUMPAD1);
            TR(XKB_KEY_KP_2,       KEYCODE_NUMPAD2);
            TR(XKB_KEY_KP_3,       KEYCODE_NUMPAD3);
            TR(XKB_KEY_KP_4,       KEYCODE_NUMPAD4);
            TR(XKB_KEY_KP_5,       KEYCODE_NUMPAD5);
            TR(XKB_KEY_KP_6,       KEYCODE_NUMPAD6);
            TR(XKB_KEY_KP_7,       KEYCODE_NUMPAD7);
            TR(XKB_KEY_KP_8,       KEYCODE_NUMPAD8);
            TR(XKB_KEY_KP_9,       KEYCODE_NUMPAD9);
            TR(XKB_KEY_Num_Lock,   KEYCODE_NUMLOCK);
            TR(XKB_KEY_KP_Divide,  KEYCODE_DIVIDE);
            TR(XKB_KEY_KP_Multiply,KEYCODE_MULTIPLY);
            TR(XKB_KEY_KP_Subtract,KEYCODE_SUBTRACT);
            TR(XKB_KEY_KP_Add,     KEYCODE_ADDITION);
            TR(XKB_KEY_KP_Enter,   KEYCODE_ENTER);
            TR(XKB_KEY_KP_Decimal, KEYCODE_DECIMAL);
            TR(XKB_KEY_Shift_L,    KEYCODE_LEFT_SHIFT);
            TR(XKB_KEY_Shift_R,    KEYCODE_RIGHT_SHIFT);
            TR(XKB_KEY_Control_L,  KEYCODE_LEFT_CONTROL);
            TR(XKB_KEY_Control_R,  KEYCODE_RIGHT_CONTROL);
        }

        return symbol;
    }

#undef TR

    u32 translateKeyMask(uint16_t state)
    {
        u32 mask = 0;
        if (state & XCB_MOD_MASK_CONTROL) mask |= KEYMASK_CONTROL;
        if (state & XCB_MOD_MASK_SHIFT)   mask |= KEYMASK_SHIFT;
        if (state & XCB_MOD_MASK_4)       mask |= KEYMASK_SUPER;
        return mask;
    }

    MouseButton translateButton(uint8_t button)
    {
        MouseButton result = MOUSEBUTTON_MIDDLE;
        switch (button)
        {
            case 1: result = MOUSEBUTTON_LEFT; break;
            case 2: result = MOUSEBUTTON_MIDDLE; break;
            case 3: result = MOUSEBUTTON_RIGHT; break;
            case 4: result = MOUSEBUTTON_WHEEL; break; // wheel up
            case 5: result = MOUSEBUTTON_WHEEL; break; // wheel down
        }
        return result;
    }

} // namespace

namespace mango
{
    using namespace mango::math;
    using namespace mango::image;

    // -----------------------------------------------------------------------
    // XcbBackend
    // -----------------------------------------------------------------------

    XcbBackend::XcbBackend()
        : key_symbols(nullptr)
    {
        connection = xcb_connect(nullptr, nullptr);
        if (!connection)
        {
            MANGO_EXCEPTION("[XcbBackend] xcb_connect() failed.");
        }

        // Initialize XKB
        const xcb_query_extension_reply_t* xkb_reply = xcb_get_extension_data(connection, &xcb_xkb_id);
        if (!xkb_reply || !xkb_reply->present)
        {
            MANGO_EXCEPTION("[XcbBackend] XKB extension not available.");
        }

        // Initialize key symbols
        key_symbols = xcb_key_symbols_alloc(connection);
        if (!key_symbols)
        {
            MANGO_EXCEPTION("[XcbBackend] Failed to allocate key symbols.");
        }

        // Intern atoms
        xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
        xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
        xcb_intern_atom_cookie_t state_cookie = xcb_intern_atom(connection, 0, 13, "_NET_WM_STATE");
        xcb_intern_atom_cookie_t fullscreen_cookie = xcb_intern_atom(connection, 0, 24, "_NET_WM_STATE_FULLSCREEN");
        xcb_intern_atom_cookie_t primary_cookie = xcb_intern_atom(connection, 0, 7, "PRIMARY");
        xcb_intern_atom_cookie_t sync_request_cookie = xcb_intern_atom(connection, 0, 20, "_NET_WM_SYNC_REQUEST");
        xcb_intern_atom_cookie_t sync_counter_cookie = xcb_intern_atom(connection, 0, 28, "_NET_WM_SYNC_REQUEST_COUNTER");

        // XDnD atoms
        xcb_intern_atom_cookie_t xdnd_aware_cookie = xcb_intern_atom(connection, 0, 8, "XdndAware");
        xcb_intern_atom_cookie_t xdnd_enter_cookie = xcb_intern_atom(connection, 0, 9, "XdndEnter");
        xcb_intern_atom_cookie_t xdnd_position_cookie = xcb_intern_atom(connection, 0, 11, "XdndPosition");
        xcb_intern_atom_cookie_t xdnd_status_cookie = xcb_intern_atom(connection, 0, 10, "XdndStatus");
        xcb_intern_atom_cookie_t xdnd_typelist_cookie = xcb_intern_atom(connection, 0, 10, "XdndTypeList");
        xcb_intern_atom_cookie_t xdnd_actioncopy_cookie = xcb_intern_atom(connection, 0, 12, "XdndActionCopy");
        xcb_intern_atom_cookie_t xdnd_drop_cookie = xcb_intern_atom(connection, 0, 8, "XdndDrop");
        xcb_intern_atom_cookie_t xdnd_finished_cookie = xcb_intern_atom(connection, 0, 11, "XdndFinished");
        xcb_intern_atom_cookie_t xdnd_selection_cookie = xcb_intern_atom(connection, 0, 11, "XdndSelection");
        xcb_intern_atom_cookie_t xdnd_leave_cookie = xcb_intern_atom(connection, 0, 9, "XdndLeave");

        // Get atom replies
        xcb_intern_atom_reply_t* protocols_reply = xcb_intern_atom_reply(connection, protocols_cookie, nullptr);
        xcb_intern_atom_reply_t* delete_reply = xcb_intern_atom_reply(connection, delete_cookie, nullptr);
        xcb_intern_atom_reply_t* state_reply = xcb_intern_atom_reply(connection, state_cookie, nullptr);
        xcb_intern_atom_reply_t* fullscreen_reply = xcb_intern_atom_reply(connection, fullscreen_cookie, nullptr);
        xcb_intern_atom_reply_t* primary_reply = xcb_intern_atom_reply(connection, primary_cookie, nullptr);
        xcb_intern_atom_reply_t* sync_request_reply = xcb_intern_atom_reply(connection, sync_request_cookie, nullptr);
        xcb_intern_atom_reply_t* sync_counter_reply = xcb_intern_atom_reply(connection, sync_counter_cookie, nullptr);

        // XDnD atom replies
        xcb_intern_atom_reply_t* xdnd_aware_reply = xcb_intern_atom_reply(connection, xdnd_aware_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_enter_reply = xcb_intern_atom_reply(connection, xdnd_enter_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_position_reply = xcb_intern_atom_reply(connection, xdnd_position_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_status_reply = xcb_intern_atom_reply(connection, xdnd_status_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_typelist_reply = xcb_intern_atom_reply(connection, xdnd_typelist_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_actioncopy_reply = xcb_intern_atom_reply(connection, xdnd_actioncopy_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_drop_reply = xcb_intern_atom_reply(connection, xdnd_drop_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_finished_reply = xcb_intern_atom_reply(connection, xdnd_finished_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_selection_reply = xcb_intern_atom_reply(connection, xdnd_selection_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_leave_reply = xcb_intern_atom_reply(connection, xdnd_leave_cookie, nullptr);

        // Store atoms
        atom_protocols = protocols_reply->atom;
        atom_delete = delete_reply->atom;
        atom_state = state_reply->atom;
        atom_fullscreen = fullscreen_reply->atom;
        atom_primary = primary_reply->atom;
        atom_sync_request = sync_request_reply ? sync_request_reply->atom : 0;
        atom_sync_counter = sync_counter_reply ? sync_counter_reply->atom : 0;

        // Store XDnD atoms
        atom_xdnd_Aware = xdnd_aware_reply->atom;
        atom_xdnd_Enter = xdnd_enter_reply->atom;
        atom_xdnd_Position = xdnd_position_reply->atom;
        atom_xdnd_Status = xdnd_status_reply->atom;
        atom_xdnd_TypeList = xdnd_typelist_reply->atom;
        atom_xdnd_ActionCopy = xdnd_actioncopy_reply->atom;
        atom_xdnd_Drop = xdnd_drop_reply->atom;
        atom_xdnd_Finished = xdnd_finished_reply->atom;
        atom_xdnd_Selection = xdnd_selection_reply->atom;
        atom_xdnd_Leave = xdnd_leave_reply->atom;

        // Free atom replies
        free(protocols_reply);
        free(delete_reply);
        free(state_reply);
        free(fullscreen_reply);
        free(primary_reply);
        free(sync_request_reply);
        free(sync_counter_reply);
        free(xdnd_aware_reply);
        free(xdnd_enter_reply);
        free(xdnd_position_reply);
        free(xdnd_status_reply);
        free(xdnd_typelist_reply);
        free(xdnd_actioncopy_reply);
        free(xdnd_drop_reply);
        free(xdnd_finished_reply);
        free(xdnd_selection_reply);
        free(xdnd_leave_reply);

        // Initialize mouse time array
        for (int i = 0; i < 6; ++i)
        {
            mouse_time[i] = 0;
        }
    }

    XcbBackend::~XcbBackend()
    {
        if (connection)
        {
            if (window)
            {
                xcb_destroy_window(connection, window);
                window = 0;
            }

            if (colormap)
            {
                xcb_free_colormap(connection, colormap);
                colormap = 0;
            }

            if (icon_pixmap)
            {
                xcb_free_pixmap(connection, icon_pixmap);
                icon_pixmap = 0;
            }

            if (icon_mask)
            {
                xcb_free_pixmap(connection, icon_mask);
                icon_mask = 0;
            }

            if (key_symbols)
            {
                xcb_key_symbols_free(key_symbols);
                key_symbols = nullptr;
            }

            if (sync_counter)
            {
                xcb_sync_destroy_counter(connection, sync_counter);
                sync_counter = 0;
            }

            xcb_disconnect(connection);
            connection = nullptr;
        }
    }

    bool XcbBackend::init(int width, int height, u32 flags, const char* title)
    {
        const xcb_setup_t* setup = xcb_get_setup(connection);
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
        xcb_screen_t* screen = iter.data;
        if (!screen)
        {
            return false;
        }

        root = screen->root;
        window = xcb_generate_id(connection);

        uint32_t value_mask = XCB_CW_EVENT_MASK;
        uint32_t value_list [] =
        {
            XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY
        };

        visualid = screen->root_visual;

        xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, root,
                          0, 0, width, height, 0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          visualid,
                          value_mask, value_list);

        // Frame synchronization (_NET_WM_SYNC_REQUEST). Only enabled when the X server
        // has the SYNC extension and the atoms resolved. A 64-bit counter is created and
        // its id published in _NET_WM_SYNC_REQUEST_COUNTER; the compositor then sends a
        // sync request before each resize and waits for us to bump the counter once the
        // matching frame is drawn.
        const xcb_query_extension_reply_t* sync_ext = xcb_get_extension_data(connection, &xcb_sync_id);
        if (sync_ext && sync_ext->present && atom_sync_request && atom_sync_counter)
        {
            sync_counter = xcb_generate_id(connection);
            xcb_sync_int64_t initial = { 0, 0 };
            xcb_sync_create_counter(connection, sync_counter, initial);
            xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, atom_sync_counter,
                                XCB_ATOM_CARDINAL, 32, 1, &sync_counter);
            sync_value = initial;
            sync_supported = true;
        }

        // Set window protocols (WM_DELETE_WINDOW, and _NET_WM_SYNC_REQUEST when supported)
        xcb_atom_t protocols[2] = { atom_delete, atom_sync_request };
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, atom_protocols, XCB_ATOM_ATOM, 32,
                            sync_supported ? 2 : 1, protocols);

        // Set XDnD version
        uint32_t xdnd_version = 5;
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, atom_xdnd_Aware, XCB_ATOM_ATOM, 32, 1, &xdnd_version);

        // Set window title
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);

        // Set window manager hints for better resize behavior
        xcb_icccm_wm_hints_t hints = { 0 };
        hints.flags = XCB_ICCCM_WM_HINT_INPUT | XCB_ICCCM_WM_HINT_STATE | XCB_ICCCM_WM_HINT_WINDOW_GROUP;
        hints.input = 1;  // Window accepts input
        hints.initial_state = XCB_ICCCM_WM_STATE_NORMAL;
        hints.window_group = window;  // Set window group to itself
        xcb_icccm_set_wm_hints(connection, window, &hints);

        // Set window size hints
        xcb_size_hints_t size_hints = { 0 };
        if (flags & Window::DISABLE_RESIZE)
        {
            size_hints.flags = XCB_ICCCM_SIZE_HINT_P_MIN_SIZE | XCB_ICCCM_SIZE_HINT_P_MAX_SIZE;
            size_hints.min_width = width;
            size_hints.min_height = height;
            size_hints.max_width = width;
            size_hints.max_height = height;
        }
        else
        {
            size_hints.flags = XCB_ICCCM_SIZE_HINT_P_MIN_SIZE | XCB_ICCCM_SIZE_HINT_P_RESIZE_INC;
            size_hints.min_width = 1;
            size_hints.min_height = 1;
            size_hints.width_inc = 1;
            size_hints.height_inc = 1;
        }
        xcb_icccm_set_wm_size_hints(connection, window, XCB_ATOM_WM_NORMAL_HINTS, &size_hints);

        // Set window type hint
        xcb_intern_atom_cookie_t window_type_cookie = xcb_intern_atom(connection, 0, 16, "_NET_WM_WINDOW_TYPE");
        xcb_intern_atom_cookie_t window_type_normal_cookie = xcb_intern_atom(connection, 0, 13, "_NET_WM_WINDOW_TYPE_NORMAL");
        xcb_intern_atom_cookie_t wm_state_cookie = xcb_intern_atom(connection, 0, 13, "_NET_WM_STATE");
        xcb_intern_atom_cookie_t wm_state_normal_cookie = xcb_intern_atom(connection, 0, 13, "_NET_WM_STATE_NORMAL");

        xcb_intern_atom_reply_t* window_type_reply = xcb_intern_atom_reply(connection, window_type_cookie, nullptr);
        xcb_intern_atom_reply_t* window_type_normal_reply = xcb_intern_atom_reply(connection, window_type_normal_cookie, nullptr);
        xcb_intern_atom_reply_t* wm_state_reply = xcb_intern_atom_reply(connection, wm_state_cookie, nullptr);
        xcb_intern_atom_reply_t* wm_state_normal_reply = xcb_intern_atom_reply(connection, wm_state_normal_cookie, nullptr);

        if (window_type_reply && window_type_normal_reply)
        {
            xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, window_type_reply->atom, XCB_ATOM_ATOM, 32, 1, &window_type_normal_reply->atom);
        }

        if (wm_state_reply && wm_state_normal_reply)
        {
            xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, wm_state_reply->atom, XCB_ATOM_ATOM, 32, 1, &wm_state_normal_reply->atom);
        }

        free(window_type_reply);
        free(window_type_normal_reply);
        free(wm_state_reply);
        free(wm_state_normal_reply);

        // NOTE: the window is created un-mapped (not visible). This is required by Vulkan
        // so that nothing is shown while the application configures itself; the caller
        // makes the window visible with setVisible(true). OpenGLWindow does this
        // automatically after onContextReady() when enterEventLoop() is called.
        xcb_flush(connection);

        const xcb_query_extension_reply_t* randr = xcb_get_extension_data(connection, &xcb_randr_id);
        if (randr && randr->present)
        {
            xcb_randr_select_input(connection, window, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
        }

        return true;
    }

    void XcbBackend::toggleFullscreen()
    {
        xcb_client_message_event_t xevent = {0};

        xevent.response_type = XCB_CLIENT_MESSAGE;
        xevent.window = window;
        xevent.type = atom_state;
        xevent.format = 32;
        xevent.data.data32[0] = 2;  // NET_WM_STATE_TOGGLE
        xevent.data.data32[1] = atom_fullscreen;
        xevent.data.data32[2] = 0;  // No second property to toggle
        xevent.data.data32[3] = 1;  // Source indication: application
        xevent.data.data32[4] = 0;  // Unused field

        // Send the event to the root window
        xcb_send_event(connection, 0, root,
                       XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                       reinterpret_cast<const char*>(&xevent));
        xcb_flush(connection);

        // Get window geometry
        xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(connection, window);
        xcb_get_geometry_reply_t* geom = xcb_get_geometry_reply(connection, geom_cookie, nullptr);
        if (!geom)
        {
            return;
        }

        // Prepare the Expose event
        xcb_expose_event_t expose_event;
        std::memset(&expose_event, 0, sizeof(expose_event));
        expose_event.response_type = XCB_EXPOSE;
        expose_event.window = window;
        expose_event.x = 0;
        expose_event.y = 0;
        expose_event.width = geom->width;
        expose_event.height = geom->height;
        expose_event.count = 0;

        // Send the event
        xcb_send_event(connection, 0, window, XCB_EVENT_MASK_EXPOSURE,
                    reinterpret_cast<const char*>(&expose_event));

        // Flush the request
        xcb_flush(connection);

        free(geom);

        fullscreen = !fullscreen;
    }

    bool XcbBackend::isFullscreen() const
    {
        return fullscreen;
    }

    math::int32x2 XcbBackend::getWindowSize() const
    {
        xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connection, window);
        xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(connection, cookie, nullptr);

        int width = 0;
        int height = 0;

        if (reply)
        {
            width = reply->width;
            height = reply->height;
            free(reply);
        }

        return int32x2(width, height);
    }

    // -----------------------------------------------------------------------
    // XcbBackend factory
    // -----------------------------------------------------------------------

    std::unique_ptr<WindowBackend> createXcbBackend(Window* window, int width, int height, u32 flags, const char* title)
    {
        auto backend = std::make_unique<XcbBackend>();
        backend->owner = window;

        if (flags & Window::API_OPENGL)
        {
            // GLX and EGL must choose a visual before creating the window;
            // the GL context creation calls init() once the visual is known.
        }
        else
        {
            if (!backend->init(width, height, flags, title))
            {
                return nullptr;
            }
        }

        return backend;
    }

    // -----------------------------------------------------------------------
    // Window (static, screen queries)
    // -----------------------------------------------------------------------

#if !defined(MANGO_ENABLE_XLIB)

    // Provided by the Xlib backend when it is present; defined here only when the
    // build excludes Xlib, so the single Window::getScreen* definition is unique.

    int Window::getScreenCount()
    {
        xcb_connection_t* connection = xcb_connect(NULL, NULL);
        if (xcb_connection_has_error(connection))
        {
            MANGO_EXCEPTION("[Window] xcb_connect() failed.");
        }

        std::vector<ScreenInfo> screens = getScreenInfo(connection);
        int count = int(screens.size());

        xcb_disconnect(connection);

        return count;
    }

    int32x2 Window::getScreenSize(int index)
    {
        xcb_connection_t* connection = xcb_connect(NULL, NULL);
        if (xcb_connection_has_error(connection))
        {
            MANGO_EXCEPTION("[Window] xcb_connect() failed.");
        }

        std::vector<ScreenInfo> screens = getScreenInfo(connection);
        int count = int(screens.size());

        xcb_disconnect(connection);

        index = std::max(index, 0);
        index = std::min(index, count - 1);

        return screens[index].resolution;
    }

#endif // !defined(MANGO_ENABLE_XLIB)

    // -----------------------------------------------------------------------
    // XcbBackend (window operations + event loop)
    // -----------------------------------------------------------------------

    void XcbBackend::setWindowPosition(int x, int y)
    {
        uint32_t values[] = { uint32_t(x), uint32_t(y) };
        xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
        xcb_flush(connection);
    }

    void XcbBackend::setWindowSize(int width, int height)
    {
        uint32_t values[] = { uint32_t(width), uint32_t(height) };
        xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        xcb_flush(connection);
    }

    void XcbBackend::setTitle(const std::string& title)
    {
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title.c_str()), title.c_str());
        xcb_flush(connection);
    }

    void XcbBackend::setVisible(bool enable)
    {
        if (enable)
        {
            xcb_map_window(connection, window);
            uint32_t values[] = { XCB_STACK_MODE_ABOVE };
            xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_STACK_MODE, values);
            xcb_flush(connection);
        }
        else
        {
            xcb_unmap_window(connection, window);
            xcb_flush(connection);
        }
    }

    int32x2 XcbBackend::getCursorPosition() const
    {
        xcb_query_pointer_cookie_t cookie = xcb_query_pointer(connection, window);
        xcb_query_pointer_reply_t* reply = xcb_query_pointer_reply(connection, cookie, nullptr);

        int x = 0;
        int y = 0;

        if (reply)
        {
            x = reply->win_x;
            y = reply->win_y;
            free(reply);
        }

        return int32x2(x, y);
    }

    bool XcbBackend::isKeyPressed(Keycode code) const
    {
        switch (code)
        {
            case KEYCODE_SHIFT:
                return isKeyPressed(KEYCODE_LEFT_SHIFT) || isKeyPressed(KEYCODE_RIGHT_SHIFT);

            case KEYCODE_CONTROL:
                return isKeyPressed(KEYCODE_LEFT_CONTROL) || isKeyPressed(KEYCODE_RIGHT_CONTROL);

            default:
                break;
        }

        bool pressed = false;

        // Get window with input focus
        xcb_get_input_focus_cookie_t focus_cookie = xcb_get_input_focus(connection);
        xcb_get_input_focus_reply_t* focus_reply = xcb_get_input_focus_reply(connection, focus_cookie, nullptr);

        if (focus_reply && focus_reply->focus == window)
        {
            // Get keyboard state
            xcb_query_keymap_cookie_t keymap_cookie = xcb_query_keymap(connection);
            xcb_query_keymap_reply_t* keymap_reply = xcb_query_keymap_reply(connection, keymap_cookie, nullptr);

            if (keymap_reply)
            {
                xcb_keysym_t symbol = translateKeycodeToSymbol(code);
                xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(key_symbols, symbol);
                xcb_keycode_t keycode = keycodes ? keycodes[0] : 0;

                if (keycode >= 0 && keycode < 255)
                {
                    pressed = (keymap_reply->keys[keycode / 8] & (1 << (keycode % 8))) != 0;
                }

                free(keymap_reply);
            }
        }

        free(focus_reply);
        return pressed;
    }

    double XcbBackend::getDisplayRefreshRate() const
    {
        return queryXcbRefreshRate(connection, window);
    }

    void XcbBackend::wakeEventLoop()
    {
        // The loop blocks in poll() on the XCB connection fd. Same-thread state changes
        // (invalidate / requestFrame / breakEventLoop) are applied between iterations,
        // and the idle wait is capped, so a cross-thread change is noticed within the
        // cap without an explicit wake. A self-pipe could make this immediate later.
    }

    void XcbBackend::runEventLoop()
    {
        owner->syncDisplayRefreshRate();

        while (owner->isRunning())
        {
            bool hadEvents = false;

            for (;;)
            {
                xcb_generic_event_t* event = xcb_poll_for_event(connection);
                if (!event)
                {
                    break;
                }

                hadEvents = true;

                switch (event->response_type & 0x7f)
                {
                    case XCB_BUTTON_PRESS:
                    {
                        xcb_button_press_event_t* button_press = (xcb_button_press_event_t*)event;
                        MouseButton button = translateButton(button_press->detail);
                        int count = 1;

                        switch (button_press->detail)
                        {
                            case 0:
                            case 1:
                            case 2:
                            {
                                // Simulate double click
                                u32 time = mango::Time::ms();
                                if (time - mouse_time[button] < 300)
                                {
                                    count = 2;
                                }
                                mouse_time[button] = time;
                                break;
                            }

                            case 4:
                                count = 120;
                                break;

                            case 5:
                                count = -120;
                                break;
                        }

                        owner->onMouseClick(button_press->event_x, button_press->event_y, button, count);
                        break;
                    }

                    case XCB_BUTTON_RELEASE:
                    {
                        xcb_button_release_event_t* button_release = (xcb_button_release_event_t*)event;
                        MouseButton button = translateButton(button_release->detail);
                        owner->onMouseClick(button_release->event_x, button_release->event_y, button, 0);
                        break;
                    }

                    case XCB_MOTION_NOTIFY:
                    {
                        xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)event;
                        owner->onMouseMove(motion->event_x, motion->event_y);
                        break;
                    }

                    case XCB_KEY_PRESS:
                    {
                        auto* key_press = reinterpret_cast<xcb_key_press_event_t*>(event);
                        xcb_keysym_t keysym = xcb_key_symbols_get_keysym(key_symbols, key_press->detail, 0);
                        if (keysym != XKB_KEY_NoSymbol)
                        {
                            u32 mask = translateKeyMask(key_press->state);
                            owner->onKeyPress(translateEventToKeycode(keysym), mask);
                        }
                        break;
                    }

                    case XCB_KEY_RELEASE:
                    {
                        auto* key_release = reinterpret_cast<xcb_key_release_event_t*>(event);
                        xcb_keysym_t keysym = xcb_key_symbols_get_keysym(key_symbols, key_release->detail, 0);
                        if (keysym != XKB_KEY_NoSymbol)
                        {
                            bool is_repeat = false;

                            // Check for key repeat
                            xcb_generic_event_t* next_event = xcb_poll_for_event(connection);
                            if (next_event)
                            {
                                if ((next_event->response_type & 0x7f) == XCB_KEY_PRESS)
                                {
                                    xcb_key_press_event_t* next_key = (xcb_key_press_event_t*)next_event;
                                    if (next_key->time == key_release->time && next_key->detail == key_release->detail)
                                    {
                                        is_repeat = true;
                                    }
                                }
                                free(next_event);
                            }

                            if (!is_repeat)
                            {
                                owner->onKeyRelease(translateEventToKeycode(keysym));
                            }
                        }
                        break;
                    }

                    case XCB_CONFIGURE_NOTIFY:
                    {
                        // Coalesce: only record the new size here and flag a pending resize.
                        // The whole event queue is drained in this loop, so the latest size
                        // wins; onResize() + the frame are emitted once, after draining (see
                        // below). The frame is what bumps the _NET_WM_SYNC counter, so the
                        // compositor keeps the previous content until our resized frame lands.
                        xcb_configure_notify_event_t* configure = (xcb_configure_notify_event_t*)event;
                        if (configure->width != size[0] || configure->height != size[1])
                        {
                            size[0] = configure->width;
                            size[1] = configure->height;
                            resize_pending = true;
                        }

                        owner->syncDisplayRefreshRate();
                        break;
                    }

                    case XCB_EXPOSE:
                    {
                        if (!busy)
                        {
                            owner->invalidate();
                        }
                        break;
                    }

                    case XCB_CLIENT_MESSAGE:
                    {
                        xcb_client_message_event_t* client_message = (xcb_client_message_event_t*)event;
                        if (client_message->type == atom_protocols)
                        {
                            if (client_message->data.data32[0] == atom_delete)
                            {
                                owner->breakEventLoop();
                            }
                            else if (sync_supported && client_message->data.data32[0] == atom_sync_request)
                            {
                                // Store the value the compositor wants the counter set to once
                                // we have drawn the frame for the upcoming resize. data32[2] is
                                // the low word, data32[3] the high word.
                                sync_value.lo = client_message->data.data32[2];
                                sync_value.hi = int32_t(client_message->data.data32[3]);
                                sync_pending = true;
                            }
                        }
                        else if (client_message->type == atom_xdnd_Enter)
                        {
                            bool use_list = client_message->data.data32[1] & 1;
                            xdnd_source = client_message->data.data32[0];
                            xdnd_version = (client_message->data.data32[1] >> 24);
                            if (use_list)
                            {
                                // Fetch conversion targets
                                xcb_get_property_cookie_t cookie = xcb_get_property(connection, 0,
                                    xdnd_source, atom_xdnd_TypeList, XCB_ATOM_ATOM, 0, 0x8000000L);
                                xcb_get_property_reply_t* reply = xcb_get_property_reply(connection, cookie, nullptr);
                                if (reply)
                                {
                                    xcb_atom_t* atoms = (xcb_atom_t*)xcb_get_property_value(reply);
                                    int count = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
                                    // MANGO TODO: Implement target selection
                                    free(reply);
                                }
                            }
                            else
                            {
                                // Pick from list of three
                                atom_xdnd_req = client_message->data.data32[2];
                            }
                        }
                        else if (client_message->type == atom_xdnd_Position)
                        {
                            xcb_client_message_event_t reply = { 0 };
                            reply.response_type = XCB_CLIENT_MESSAGE;
                            reply.format = 32;
                            reply.window = client_message->data.data32[0];
                            reply.type = atom_xdnd_Status;
                            reply.data.data32[0] = window;
                            reply.data.data32[1] = (atom_xdnd_req != 0);
                            reply.data.data32[2] = 0; // empty rectangle
                            reply.data.data32[3] = 0;
                            reply.data.data32[4] = atom_xdnd_ActionCopy;

                            xcb_send_event(connection, 0, client_message->data.data32[0],
                                XCB_EVENT_MASK_NO_EVENT, (char*)&reply);
                            xcb_flush(connection);
                        }
                        else if (client_message->type == atom_xdnd_Drop)
                        {
                            if (atom_xdnd_req == 0)
                            {
                                // Respond to empty request
                                xcb_client_message_event_t reply = { 0 };
                                reply.response_type = XCB_CLIENT_MESSAGE;
                                reply.format = 32;
                                reply.window = client_message->data.data32[0];
                                reply.type = atom_xdnd_Finished;
                                reply.data.data32[0] = window;
                                reply.data.data32[1] = 0;
                                reply.data.data32[2] = 0; // failed

                                xcb_send_event(connection, 0, client_message->data.data32[0],
                                    XCB_EVENT_MASK_NO_EVENT, (char*)&reply);
                            }
                            else
                            {
                                // Convert selection
                                xcb_convert_selection(connection, window,
                                    atom_xdnd_Selection, atom_xdnd_req,
                                    atom_primary, client_message->data.data32[2]);
                            }
                        }
                        break;
                    }

                    case XCB_SELECTION_NOTIFY:
                    {
                        xcb_selection_notify_event_t* selection = (xcb_selection_notify_event_t*)event;
                        if (selection->target == atom_xdnd_req)
                        {
                            // Read data
                            xcb_get_property_cookie_t cookie = xcb_get_property(connection, 0,
                                window, atom_primary, XCB_ATOM_STRING, 0, 0x8000000L);
                            xcb_get_property_reply_t* reply = xcb_get_property_reply(connection, cookie, nullptr);
                            if (reply)
                            {
                                // MANGO TODO: Process dropped files
                                free(reply);
                            }

                            // Send reply
                            xcb_client_message_event_t client_message = { 0 };
                            client_message.response_type = XCB_CLIENT_MESSAGE;
                            client_message.format = 32;
                            client_message.window = xdnd_source;
                            client_message.type = atom_xdnd_Finished;
                            client_message.data.data32[0] = window;
                            client_message.data.data32[1] = 1;
                            client_message.data.data32[2] = atom_xdnd_ActionCopy;

                            xcb_send_event(connection, 0, xdnd_source,
                                XCB_EVENT_MASK_NO_EVENT, (char*)&client_message);
                            xcb_flush(connection);
                        }
                        break;
                    }
                }

                free(event);
            }

            // Emit a single resize for the coalesced final size before drawing, so the
            // frame dispatched below renders at the new extent.
            if (resize_pending && !busy)
            {
                resize_pending = false;
                owner->onResize(size[0], size[1]);
                owner->invalidate();
            }

            if (!busy)
            {
                owner->dispatchFrame();

                // The resized frame has now been submitted/presented; tell the compositor
                // it may show the new size. Doing this after dispatchFrame() is what keeps
                // the window border from flashing undefined content during a live resize.
                if (sync_pending && sync_supported)
                {
                    sync_pending = false;
                    xcb_sync_set_counter(connection, sync_counter, sync_value);
                    xcb_flush(connection);
                }
            }

            if (!hadEvents)
            {
                // Block on the XCB connection fd until an event arrives or the next
                // frame is due, instead of busy-polling. An idle (WAIT_INFINITE) wait
                // is capped so a cross-thread state change is observed within the cap;
                // a pending deadline (animation) is waited exactly so it fires on time.
                const u32 timeout = owner->eventLoop().computeWaitTimeoutMs(mango::Time::us());
                if (timeout != 0)
                {
                    const int wait_ms = (timeout == EventLoopState::WAIT_INFINITE) ? 100 : int(timeout);
                    xcb_flush(connection);
                    struct pollfd pfd = { xcb_get_file_descriptor(connection), POLLIN, 0 };
                    ::poll(&pfd, 1, wait_ms);
                }
            }
        }
    }

#if defined(MANGO_ENABLE_VULKAN)

    VkSurfaceKHR XcbBackend::createVulkanSurface(VkInstance instance)
    {
        VkXcbSurfaceCreateInfoKHR createInfo;

        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.connection = connection;
        createInfo.window = window;

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult result = vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            MANGO_EXCEPTION("[XcbBackend] vkCreateXcbSurfaceKHR() failed.");
        }

        return surface;
    }

    bool XcbBackend::getPresentationSupport(VkPhysicalDevice physicalDevice, u32 queueFamilyIndex)
    {
        VkBool32 support = vkGetPhysicalDeviceXcbPresentationSupportKHR(
            physicalDevice, queueFamilyIndex, connection, visualid);
        return support == VK_TRUE;
    }

#endif // defined(MANGO_ENABLE_VULKAN)

#if defined(MANGO_ENABLE_EGL)

    void* XcbBackend::eglNativeWindow(int width, int height, u32 flags)
    {
        if (!init(width, height, flags, "OpenGL|ES"))
        {
            return nullptr;
        }

        return reinterpret_cast<void*>(static_cast<std::uintptr_t>(window));
    }

#endif // defined(MANGO_ENABLE_EGL)

} // namespace mango

#endif // defined(MANGO_ENABLE_XCB)
