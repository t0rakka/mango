/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/core/timer.hpp>

#if defined(MANGO_WINDOW_SYSTEM_XCB)

#include <unistd.h>
#define explicit explicit_
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xkb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#undef explicit
#include "xcb_handle.hpp"

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
            printf("screen %d: %d x %d\n", screen_iter.index, width, height);

            ScreenInfo info;
            info.screen = screen;
            info.resolution = math::int32x2(width, height);
            screens.push_back(info);

            xcb_screen_next(&screen_iter);
        }

        return screens;
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
    // WindowHandle implementation
    // -----------------------------------------------------------------------

    WindowHandle::WindowHandle(int width, int height, u32 flags)
        : flags(flags)
        , key_symbols(nullptr)
    {
        int screen_index;
        native.connection = xcb_connect(nullptr, &screen_index);
        if (!native.connection)
        {
            MANGO_EXCEPTION("[Window] xcb_connect() failed.");
        }

        // Initialize XKB
        const xcb_query_extension_reply_t* xkb_reply = xcb_get_extension_data(native.connection, &xcb_xkb_id);
        if (!xkb_reply || !xkb_reply->present)
        {
            MANGO_EXCEPTION("[Window] XKB extension not available.");
        }

        // Initialize key symbols
        key_symbols = xcb_key_symbols_alloc(native.connection);
        if (!key_symbols)
        {
            MANGO_EXCEPTION("[Window] Failed to allocate key symbols.");
        }

        const xcb_setup_t* setup = xcb_get_setup(native.connection);
        xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

        while (screen_index-- > 0)
        {
            xcb_screen_next(&screen_iterator);
        }

        xcb_screen_t* screen = screen_iterator.data;
        native.window = xcb_generate_id(native.connection);

        // Create colormap
        colormap = xcb_generate_id(native.connection);
        xcb_create_colormap(native.connection, XCB_COLORMAP_ALLOC_NONE, colormap, screen->root, screen->root_visual);

        // Create window
        uint32_t value_list[] =
        {
            screen->white_pixel,
            XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY,
            colormap
        };

        xcb_create_window(
            native.connection,
            XCB_COPY_FROM_PARENT,
            native.window,
            screen->root,
            20, 20,
            width, height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP,
            value_list);

        // Intern atoms
        xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(native.connection, 1, 12, "WM_PROTOCOLS");
        xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(native.connection, 0, 16, "WM_DELETE_WINDOW");
        xcb_intern_atom_cookie_t state_cookie = xcb_intern_atom(native.connection, 0, 13, "_NET_WM_STATE");
        xcb_intern_atom_cookie_t fullscreen_cookie = xcb_intern_atom(native.connection, 0, 20, "_NET_WM_STATE_FULLSCREEN");
        xcb_intern_atom_cookie_t primary_cookie = xcb_intern_atom(native.connection, 0, 7, "PRIMARY");

        // XDnD atoms
        xcb_intern_atom_cookie_t xdnd_aware_cookie = xcb_intern_atom(native.connection, 0, 8, "XdndAware");
        xcb_intern_atom_cookie_t xdnd_enter_cookie = xcb_intern_atom(native.connection, 0, 9, "XdndEnter");
        xcb_intern_atom_cookie_t xdnd_position_cookie = xcb_intern_atom(native.connection, 0, 11, "XdndPosition");
        xcb_intern_atom_cookie_t xdnd_status_cookie = xcb_intern_atom(native.connection, 0, 10, "XdndStatus");
        xcb_intern_atom_cookie_t xdnd_typelist_cookie = xcb_intern_atom(native.connection, 0, 10, "XdndTypeList");
        xcb_intern_atom_cookie_t xdnd_actioncopy_cookie = xcb_intern_atom(native.connection, 0, 12, "XdndActionCopy");
        xcb_intern_atom_cookie_t xdnd_drop_cookie = xcb_intern_atom(native.connection, 0, 8, "XdndDrop");
        xcb_intern_atom_cookie_t xdnd_finished_cookie = xcb_intern_atom(native.connection, 0, 11, "XdndFinished");
        xcb_intern_atom_cookie_t xdnd_selection_cookie = xcb_intern_atom(native.connection, 0, 11, "XdndSelection");
        xcb_intern_atom_cookie_t xdnd_leave_cookie = xcb_intern_atom(native.connection, 0, 9, "XdndLeave");

        // Get atom replies
        xcb_intern_atom_reply_t* protocols_reply = xcb_intern_atom_reply(native.connection, protocols_cookie, nullptr);
        xcb_intern_atom_reply_t* delete_reply = xcb_intern_atom_reply(native.connection, delete_cookie, nullptr);
        xcb_intern_atom_reply_t* state_reply = xcb_intern_atom_reply(native.connection, state_cookie, nullptr);
        xcb_intern_atom_reply_t* fullscreen_reply = xcb_intern_atom_reply(native.connection, fullscreen_cookie, nullptr);
        xcb_intern_atom_reply_t* primary_reply = xcb_intern_atom_reply(native.connection, primary_cookie, nullptr);

        // XDnD atom replies
        xcb_intern_atom_reply_t* xdnd_aware_reply = xcb_intern_atom_reply(native.connection, xdnd_aware_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_enter_reply = xcb_intern_atom_reply(native.connection, xdnd_enter_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_position_reply = xcb_intern_atom_reply(native.connection, xdnd_position_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_status_reply = xcb_intern_atom_reply(native.connection, xdnd_status_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_typelist_reply = xcb_intern_atom_reply(native.connection, xdnd_typelist_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_actioncopy_reply = xcb_intern_atom_reply(native.connection, xdnd_actioncopy_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_drop_reply = xcb_intern_atom_reply(native.connection, xdnd_drop_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_finished_reply = xcb_intern_atom_reply(native.connection, xdnd_finished_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_selection_reply = xcb_intern_atom_reply(native.connection, xdnd_selection_cookie, nullptr);
        xcb_intern_atom_reply_t* xdnd_leave_reply = xcb_intern_atom_reply(native.connection, xdnd_leave_cookie, nullptr);

        // Store atoms
        atom_protocols = protocols_reply->atom;
        atom_delete = delete_reply->atom;
        atom_state = state_reply->atom;
        atom_fullscreen = fullscreen_reply->atom;
        atom_primary = primary_reply->atom;

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

        // Set window protocols
        xcb_change_property(native.connection, XCB_PROP_MODE_REPLACE, native.window, atom_protocols, XCB_ATOM_ATOM, 32, 1, &atom_delete);

        // Set XDnD version
        uint32_t xdnd_version = 5;
        xcb_change_property(native.connection, XCB_PROP_MODE_REPLACE, native.window, atom_xdnd_Aware, XCB_ATOM_ATOM, 32, 1, &xdnd_version);

        // Set window title
        const char* title = "XCB Window";
        xcb_change_property(native.connection, XCB_PROP_MODE_REPLACE, native.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);

        // Map window
        xcb_map_window(native.connection, native.window);
        xcb_flush(native.connection);

        // Initialize mouse time array
        for (int i = 0; i < 6; ++i)
        {
            mouse_time[i] = 0;
        }
    }

    WindowHandle::~WindowHandle()
    {
        if (native.connection)
        {
            if (native.window)
            {
                xcb_destroy_window(native.connection, native.window);
                native.window = 0;
            }

            if (colormap)
            {
                xcb_free_colormap(native.connection, colormap);
                colormap = 0;
            }

            if (icon_pixmap)
            {
                xcb_free_pixmap(native.connection, icon_pixmap);
                icon_pixmap = 0;
            }

            if (icon_mask)
            {
                xcb_free_pixmap(native.connection, icon_mask);
                icon_mask = 0;
            }

            if (key_symbols)
            {
                xcb_key_symbols_free(key_symbols);
                key_symbols = nullptr;
            }

            xcb_disconnect(native.connection);
            native.connection = nullptr;
        }
    }

    // -----------------------------------------------------------------------
    // Window implementation
    // -----------------------------------------------------------------------

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

    Window::Window(int width, int height, u32 flags)
    {
        m_handle = std::make_unique<WindowHandle>(width, height, flags);
        MANGO_UNREFERENCED(flags); // MANGO TODO
    }

    Window::~Window()
    {
    }

    void Window::setWindowPosition(int x, int y)
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        uint32_t values[] = { uint32_t(x), uint32_t(y) };
        xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
        xcb_flush(connection);
    }

    void Window::setWindowSize(int width, int height)
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        uint32_t values[] = { uint32_t(width), uint32_t(height) };
        xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        xcb_flush(connection);
    }

    void Window::setTitle(const std::string& title)
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
            title.length(), title.c_str());
        xcb_flush(connection);
    }

    void Window::setIcon(const Surface& surface)
    {
        /*
        // Clamp icon size to 256 x 256
        const int width = std::min(256, surface.width);
        const int height = std::min(256, surface.height);

        TemporaryBitmap temp(surface, width, height, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));

        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        // Create icon pixmap
        m_handle->icon_pixmap = xcb_generate_id(connection);
        xcb_create_pixmap(connection, 32, m_handle->icon_pixmap, window, width, height);

        // Create icon mask
        m_handle->icon_mask = xcb_generate_id(connection);
        xcb_create_pixmap(connection, 1, m_handle->icon_mask, window, width, height);

        // Create graphics context for icon
        xcb_gc_t gc = xcb_generate_id(connection);
        uint32_t gc_values[] = { 0 };
        xcb_create_gc(connection, gc, m_handle->icon_pixmap, XCB_GC_FOREGROUND, gc_values);

        // Put image data
        xcb_put_image(connection, XCB_IMAGE_FORMAT_Z_PIXMAP, m_handle->icon_pixmap, gc, width, height, 0, 0, 0, 32,
            reinterpret_cast<uint8_t*>(temp.image));

        // Create alpha mask
        size_t stride = (width + 7) / 8; // round to next multiple of 8
        Bitmap alphaMask(stride, height, Format(8, Format::UNORM, Format::A, 8));

        for (int y = 0; y < height; ++y)
        {
            u8* alpha = temp.image + y * temp.stride + 3;
            u8* dest = alphaMask.image + y * stride;

            for (int x = 0; x < width; ++x)
            {
                int s = alpha[x * 4] > 0 ? 1 : 0;
                dest[x / 8] |= (s << (x & 7));
            }
        }

        // Put mask data
        xcb_put_image(connection, XCB_IMAGE_FORMAT_XY_BITMAP, m_handle->icon_mask, gc, width, height, 0, 0, 0, 1,
            reinterpret_cast<uint8_t*>(alphaMask.image));

        // Free graphics context
        xcb_free_gc(connection, gc);

        // Set window hints
        xcb_icccm_wm_hints_t hints = { 0 };
        hints.flags = XCB_ICCCM_WM_HINT_ICON_PIXMAP | XCB_ICCCM_WM_HINT_ICON_MASK;
        hints.icon_pixmap = m_handle->icon_pixmap;
        hints.icon_mask = m_handle->icon_mask;
        xcb_icccm_set_wm_hints(connection, window, &hints);

        xcb_flush(connection);
        */
    }

    void Window::setVisible(bool enable)
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

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

    int32x2 Window::getWindowSize() const
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

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

    int32x2 Window::getCursorPosition() const
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

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

    bool Window::isKeyPressed(Keycode code) const
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
        xcb_get_input_focus_cookie_t focus_cookie = xcb_get_input_focus(m_handle->native.connection);
        xcb_get_input_focus_reply_t* focus_reply = xcb_get_input_focus_reply(m_handle->native.connection, focus_cookie, nullptr);

        if (focus_reply && focus_reply->focus == m_handle->native.window)
        {
            // Get keyboard state
            xcb_query_keymap_cookie_t keymap_cookie = xcb_query_keymap(m_handle->native.connection);
            xcb_query_keymap_reply_t* keymap_reply = xcb_query_keymap_reply(m_handle->native.connection, keymap_cookie, nullptr);

            if (keymap_reply)
            {
                xcb_keysym_t symbol = translateKeycodeToSymbol(code);
                xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(m_handle->key_symbols, symbol);
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

    Window::operator NativeWindowHandle () const
    {
        return m_handle->native;
    }

    Window::operator WindowHandle* () const
    {
        return m_handle.get();
    }

    void Window::enterEventLoop()
    {
        m_handle->is_looping = true;

        for (; m_handle->is_looping;)
        {
            xcb_generic_event_t* event = xcb_poll_for_event(m_handle->native.connection);
            if (event)
            {
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
                                if (time - m_handle->mouse_time[button] < 300)
                                {
                                    count = 2;
                                }
                                m_handle->mouse_time[button] = time;
                                break;
                            }

                            case 4:
                                count = 120;
                                break;

                            case 5:
                                count = -120;
                                break;
                        }

                        onMouseClick(button_press->event_x, button_press->event_y, button, count);
                        break;
                    }

                    case XCB_BUTTON_RELEASE:
                    {
                        xcb_button_release_event_t* button_release = (xcb_button_release_event_t*)event;
                        MouseButton button = translateButton(button_release->detail);
                        onMouseClick(button_release->event_x, button_release->event_y, button, 0);
                        break;
                    }

                    case XCB_MOTION_NOTIFY:
                    {
                        xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)event;
                        onMouseMove(motion->event_x, motion->event_y);
                        break;
                    }

                    case XCB_KEY_PRESS:
                    {
                        auto* key_press = reinterpret_cast<xcb_key_press_event_t*>(event);
                        xcb_keysym_t keysym = xcb_key_symbols_get_keysym(m_handle->key_symbols, key_press->detail, 0);
                        if (keysym != XKB_KEY_NoSymbol)
                        {
                            u32 mask = translateKeyMask(key_press->state);
                            onKeyPress(translateEventToKeycode(keysym), mask);
                        }
                        break;
                    }

                    case XCB_KEY_RELEASE:
                    {
                        auto* key_release = reinterpret_cast<xcb_key_release_event_t*>(event);
                        xcb_keysym_t keysym = xcb_key_symbols_get_keysym(m_handle->key_symbols, key_release->detail, 0);
                        if (keysym != XKB_KEY_NoSymbol)
                        {
                            bool is_repeat = false;

                            // Check for key repeat
                            xcb_generic_event_t* next_event = xcb_poll_for_event(m_handle->native.connection);
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
                                onKeyRelease(translateEventToKeycode(keysym));
                            }
                        }
                        break;
                    }

                    case XCB_CONFIGURE_NOTIFY:
                    {
                        xcb_configure_notify_event_t* configure = (xcb_configure_notify_event_t*)event;
                        if (configure->width != m_handle->size[0] || configure->height != m_handle->size[1])
                        {
                            m_handle->size[0] = configure->width;
                            m_handle->size[1] = configure->height;
                            if (!m_handle->busy)
                            {
                                onResize(configure->width, configure->height);
                            }
                        }
                        break;
                    }

                    case XCB_EXPOSE:
                    {
                        if (!m_handle->busy)
                        {
                            onDraw();
                        }
                        break;
                    }

                    case XCB_CLIENT_MESSAGE:
                    {
                        xcb_client_message_event_t* client_message = (xcb_client_message_event_t*)event;
                        if (client_message->type == m_handle->atom_protocols)
                        {
                            if (client_message->data.data32[0] == m_handle->atom_delete)
                            {
                                breakEventLoop();
                            }
                        }
                        else if (client_message->type == m_handle->atom_xdnd_Enter)
                        {
                            bool use_list = client_message->data.data32[1] & 1;
                            m_handle->xdnd_source = client_message->data.data32[0];
                            m_handle->xdnd_version = (client_message->data.data32[1] >> 24);
                            if (use_list)
                            {
                                // Fetch conversion targets
                                xcb_get_property_cookie_t cookie = xcb_get_property(m_handle->native.connection, 0,
                                    m_handle->xdnd_source, m_handle->atom_xdnd_TypeList, XCB_ATOM_ATOM, 0, 0x8000000L);
                                xcb_get_property_reply_t* reply = xcb_get_property_reply(m_handle->native.connection, cookie, nullptr);
                                if (reply)
                                {
                                    xcb_atom_t* atoms = (xcb_atom_t*)xcb_get_property_value(reply);
                                    int count = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
                                    // TODO: Implement target selection
                                    free(reply);
                                }
                            }
                            else
                            {
                                // Pick from list of three
                                m_handle->atom_xdnd_req = client_message->data.data32[2];
                            }
                        }
                        else if (client_message->type == m_handle->atom_xdnd_Position)
                        {
                            xcb_client_message_event_t reply = { 0 };
                            reply.response_type = XCB_CLIENT_MESSAGE;
                            reply.format = 32;
                            reply.window = client_message->data.data32[0];
                            reply.type = m_handle->atom_xdnd_Status;
                            reply.data.data32[0] = m_handle->native.window;
                            reply.data.data32[1] = (m_handle->atom_xdnd_req != 0);
                            reply.data.data32[2] = 0; // empty rectangle
                            reply.data.data32[3] = 0;
                            reply.data.data32[4] = m_handle->atom_xdnd_ActionCopy;

                            xcb_send_event(m_handle->native.connection, 0, client_message->data.data32[0],
                                XCB_EVENT_MASK_NO_EVENT, (char*)&reply);
                            xcb_flush(m_handle->native.connection);
                        }
                        else if (client_message->type == m_handle->atom_xdnd_Drop)
                        {
                            if (m_handle->atom_xdnd_req == 0)
                            {
                                // Respond to empty request
                                xcb_client_message_event_t reply = { 0 };
                                reply.response_type = XCB_CLIENT_MESSAGE;
                                reply.format = 32;
                                reply.window = client_message->data.data32[0];
                                reply.type = m_handle->atom_xdnd_Finished;
                                reply.data.data32[0] = m_handle->native.window;
                                reply.data.data32[1] = 0;
                                reply.data.data32[2] = 0; // failed

                                xcb_send_event(m_handle->native.connection, 0, client_message->data.data32[0],
                                    XCB_EVENT_MASK_NO_EVENT, (char*)&reply);
                            }
                            else
                            {
                                // Convert selection
                                xcb_convert_selection(m_handle->native.connection, m_handle->native.window,
                                    m_handle->atom_xdnd_Selection, m_handle->atom_xdnd_req,
                                    m_handle->atom_primary, client_message->data.data32[2]);
                            }
                        }
                        break;
                    }

                    case XCB_SELECTION_NOTIFY:
                    {
                        xcb_selection_notify_event_t* selection = (xcb_selection_notify_event_t*)event;
                        if (selection->target == m_handle->atom_xdnd_req)
                        {
                            // Read data
                            xcb_get_property_cookie_t cookie = xcb_get_property(m_handle->native.connection, 0,
                                m_handle->native.window, m_handle->atom_primary, XCB_ATOM_STRING, 0, 0x8000000L);
                            xcb_get_property_reply_t* reply = xcb_get_property_reply(m_handle->native.connection, cookie, nullptr);
                            if (reply)
                            {
                                // TODO: Process dropped files
                                free(reply);
                            }

                            // Send reply
                            xcb_client_message_event_t client_message = { 0 };
                            client_message.response_type = XCB_CLIENT_MESSAGE;
                            client_message.format = 32;
                            client_message.window = m_handle->xdnd_source;
                            client_message.type = m_handle->atom_xdnd_Finished;
                            client_message.data.data32[0] = m_handle->native.window;
                            client_message.data.data32[1] = 1;
                            client_message.data.data32[2] = m_handle->atom_xdnd_ActionCopy;

                            xcb_send_event(m_handle->native.connection, 0, m_handle->xdnd_source,
                                XCB_EVENT_MASK_NO_EVENT, (char*)&client_message);
                            xcb_flush(m_handle->native.connection);
                        }
                        break;
                    }
                }

                free(event);
            }
            else
            {
                if (!m_handle->busy)
                {
                    onIdle();
                }
                usleep(125);
            }
        }
    }

    void Window::breakEventLoop()
    {
        m_handle->is_looping = false;
    }

    void Window::onIdle()
    {
    }

    void Window::onDraw()
    {
    }

    void Window::onResize(int width, int height)
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
    }

    void Window::onMinimize()
    {
    }

    void Window::onMaximize()
    {
    }

    void Window::onKeyPress(Keycode code, u32 mask)
    {
        MANGO_UNREFERENCED(code);
        MANGO_UNREFERENCED(mask);
    }

    void Window::onKeyRelease(Keycode code)
    {
        MANGO_UNREFERENCED(code);
    }

    void Window::onMouseMove(int x, int y)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
    }

    void Window::onMouseClick(int x, int y, MouseButton button, int count)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
        MANGO_UNREFERENCED(button);
        MANGO_UNREFERENCED(count);
    }

    void Window::onDropFiles(const filesystem::FileIndex& index)
    {
        MANGO_UNREFERENCED(index);
    }

    void Window::onClose()
    {
    }

    void Window::onShow()
    {
    }

    void Window::onHide()
    {
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_XCB)
