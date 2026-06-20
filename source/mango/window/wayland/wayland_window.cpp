/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/core/timer.hpp>

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <algorithm>
#include <memory>
#include <vector>

#include "wayland_window.hpp"
#include <wayland-client-protocol.h>
#if defined(MANGO_ENABLE_EGL)
#include <wayland-egl.h>
#endif
#include <xkbcommon/xkbcommon-keysyms.h>
#include "xdg-shell-client-protocol.h"

namespace
{
    using namespace mango;

#define TR(a, b) case a: code = b; break

    Keycode translateKeysym(xkb_keysym_t symbol)
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
            TR(XKB_KEY_Left,           KEYCODE_LEFT);
            TR(XKB_KEY_Right,          KEYCODE_RIGHT);
            TR(XKB_KEY_Up,             KEYCODE_UP);
            TR(XKB_KEY_Down,           KEYCODE_DOWN);
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

    MouseButton translatePointerButton(uint32_t button)
    {
        switch (button)
        {
            case 0x110: return MOUSEBUTTON_LEFT;
            case 0x111: return MOUSEBUTTON_RIGHT;
            case 0x112: return MOUSEBUTTON_MIDDLE;
            case 0x113: return MOUSEBUTTON_X1;
            case 0x114: return MOUSEBUTTON_X2;
            default:    return MOUSEBUTTON_MIDDLE;
        }
    }

    u32 getKeyMask(struct xkb_state* state)
    {
        u32 mask = 0;

        if (!state)
        {
            return mask;
        }

        if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE))
        {
            mask |= KEYMASK_CONTROL;
        }

        if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE))
        {
            mask |= KEYMASK_SHIFT;
        }

        if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE))
        {
            mask |= KEYMASK_SUPER;
        }

        return mask;
    }

    void setKeyPressed(WindowContext* window, Keycode code, bool pressed)
    {
        if (!window || code == KEYCODE_NONE)
        {
            return;
        }

        const int idx = int(code);
        if (idx > 0 && idx < 256)
        {
            window->key_pressed[idx] = pressed;
        }
    }

    void clearKeyState(WindowContext* window)
    {
        if (!window)
        {
            return;
        }

        window->keyboard_focused = false;
        std::memset(window->key_pressed, 0, sizeof(window->key_pressed));

        if (window->xkb_state && window->xkb_keymap)
        {
            xkb_state_unref(window->xkb_state);
            window->xkb_state = xkb_state_new(window->xkb_keymap);
        }
    }

    void applyToplevelStates(WindowContext* window, struct wl_array* states)
    {
        if (!window || !window->owner)
        {
            return;
        }

        const bool was_maximized = window->maximized;
        const bool was_activated = window->activated;

        window->maximized = false;
        window->activated = false;
        window->fullscreen = false;

        if (states)
        {
            const uint32_t* state_data = static_cast<const uint32_t*>(states->data);
            const size_t count = states->size / sizeof(uint32_t);

            for (size_t i = 0; i < count; ++i)
            {
                switch (state_data[i])
                {
                    case XDG_TOPLEVEL_STATE_MAXIMIZED:
                        window->maximized = true;
                        break;

                    case XDG_TOPLEVEL_STATE_FULLSCREEN:
                        window->fullscreen = true;
                        break;

                    case XDG_TOPLEVEL_STATE_ACTIVATED:
                        window->activated = true;
                        break;

                    default:
                        break;
                }
            }
        }

        if (was_maximized != window->maximized)
        {
            window->owner->onMaximize();
        }

        if (!was_activated && window->activated)
        {
            window->owner->onShow();
        }
        else if (was_activated && !window->activated)
        {
            window->owner->onHide();
        }
    }

    void xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
    {
        WindowContext* window = static_cast<WindowContext*>(data);
        xdg_wm_base_pong(xdg_wm_base, serial);
        if (window->display)
        {
            wl_display_flush(window->display);
        }
    }

    static const struct xdg_wm_base_listener xdg_wm_base_listener =
    {
        .ping = xdg_wm_base_ping,
    };

    void xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
    {
        WindowContext* window = static_cast<WindowContext*>(data);
        window->syncSurfaceScale();
        window->syncEGLWindow();
        xdg_surface_ack_configure(xdg_surface, serial);
        window->configured = true;
    }

    static const struct xdg_surface_listener s_xdg_surface_listener =
    {
        .configure = xdg_surface_configure,
    };

    void xdg_toplevel_configure(void* data, struct xdg_toplevel* xdg_toplevel,
                                int32_t width, int32_t height, struct wl_array* states)
    {
        MANGO_UNREFERENCED(xdg_toplevel);

        WindowContext* window = static_cast<WindowContext*>(data);
        applyToplevelStates(window, states);

        const bool was_visible = window->size[0] > 0 && window->size[1] > 0;

        if (width <= 0 || height <= 0)
        {
            if (was_visible && window->owner)
            {
                window->owner->onMinimize();
            }

            window->size[0] = 0;
            window->size[1] = 0;
            window->pending_resize = true;
            window->requestRefresh();
            return;
        }

        if (!was_visible && window->owner)
        {
            window->owner->onShow();
        }

        if (window->size[0] != width || window->size[1] != height)
        {
            window->size[0] = width;
            window->size[1] = height;
            window->pending_resize = true;
            window->requestRefresh();
        }
    }

    void xdg_toplevel_close(void* data, struct xdg_toplevel* xdg_toplevel)
    {
        MANGO_UNREFERENCED(xdg_toplevel);
        WindowContext* window = static_cast<WindowContext*>(data);
        window->is_looping = false;
    }

    static const struct xdg_toplevel_listener s_xdg_toplevel_listener =
    {
        .configure = xdg_toplevel_configure,
        .close = xdg_toplevel_close,
    };

    void pointer_enter(void* data, struct wl_pointer* pointer, uint32_t serial,
                       struct wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
    {
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(serial);
        MANGO_UNREFERENCED(surface);

        WindowContext* window = static_cast<WindowContext*>(data);
        window->pointer_focused = true;
        window->cursor[0] = wl_fixed_to_int(surface_x);
        window->cursor[1] = wl_fixed_to_int(surface_y);
    }

    void pointer_leave(void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface)
    {
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(serial);
        MANGO_UNREFERENCED(surface);

        static_cast<WindowContext*>(data)->pointer_focused = false;
    }

    void pointer_motion(void* data, struct wl_pointer* pointer, uint32_t time,
                        wl_fixed_t surface_x, wl_fixed_t surface_y)
    {
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(time);

        WindowContext* window = static_cast<WindowContext*>(data);
        if (!window->owner)
        {
            return;
        }

        window->cursor[0] = wl_fixed_to_int(surface_x);
        window->cursor[1] = wl_fixed_to_int(surface_y);
        window->owner->onMouseMove(window->cursor[0], window->cursor[1]);
    }

    void pointer_button(void* data, struct wl_pointer* pointer, uint32_t serial,
                        uint32_t time, uint32_t button, uint32_t state)
    {
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(serial);
        MANGO_UNREFERENCED(time);

        WindowContext* window = static_cast<WindowContext*>(data);
        if (!window->owner)
        {
            return;
        }

        const MouseButton mouse_button = translatePointerButton(button);

        if (state == WL_POINTER_BUTTON_STATE_PRESSED)
        {
            int count = 1;

            if (mouse_button <= MOUSEBUTTON_MIDDLE)
            {
                const u32 now = Time::ms();
                const int idx = int(mouse_button);

                if (idx >= 0 && idx < 6 && now - window->mouse_time[idx] < 300)
                {
                    count = 2;
                }

                if (idx >= 0 && idx < 6)
                {
                    window->mouse_time[idx] = now;
                }
            }

            window->owner->onMouseClick(window->cursor[0], window->cursor[1], mouse_button, count);
        }
        else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
        {
            window->owner->onMouseClick(window->cursor[0], window->cursor[1], mouse_button, 0);
        }
    }

    void pointer_axis(void* data, struct wl_pointer* pointer, uint32_t time,
                      uint32_t axis, wl_fixed_t value)
    {
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(time);

        if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL)
        {
            return;
        }

        WindowContext* window = static_cast<WindowContext*>(data);
        if (!window->owner)
        {
            return;
        }

        const int delta = wl_fixed_to_int(value);
        if (delta == 0)
        {
            return;
        }

        const int count = delta > 0 ? -120 : 120;
        window->owner->onMouseClick(window->cursor[0], window->cursor[1], MOUSEBUTTON_WHEEL, count);
    }

    void pointer_frame(void* data, struct wl_pointer* pointer)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(pointer);
    }

    void pointer_axis_source(void* data, struct wl_pointer* pointer, uint32_t axis_source)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(axis_source);
    }

    void pointer_axis_stop(void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(time);
        MANGO_UNREFERENCED(axis);
    }

    void pointer_axis_discrete(void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(axis);
        MANGO_UNREFERENCED(discrete);
    }

    void pointer_axis_value120(void* data, struct wl_pointer* pointer, uint32_t axis, int32_t value120)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(axis);
        MANGO_UNREFERENCED(value120);
    }

    void pointer_axis_relative_direction(void* data, struct wl_pointer* pointer, uint32_t axis, uint32_t direction)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(pointer);
        MANGO_UNREFERENCED(axis);
        MANGO_UNREFERENCED(direction);
    }

    static const struct wl_pointer_listener pointer_listener =
    {
        .enter = pointer_enter,
        .leave = pointer_leave,
        .motion = pointer_motion,
        .button = pointer_button,
        .axis = pointer_axis,
        .frame = pointer_frame,
        .axis_source = pointer_axis_source,
        .axis_stop = pointer_axis_stop,
        .axis_discrete = pointer_axis_discrete,
        .axis_value120 = pointer_axis_value120,
        .axis_relative_direction = pointer_axis_relative_direction,
    };

    void keyboard_keymap(void* data, struct wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
    {
        MANGO_UNREFERENCED(keyboard);

        WindowContext* window = static_cast<WindowContext*>(data);
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
        {
            close(fd);
            return;
        }

        char* map_str = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (map_str == MAP_FAILED)
        {
            close(fd);
            return;
        }

        if (!window->xkb_context)
        {
            window->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        }

        if (window->xkb_keymap)
        {
            xkb_keymap_unref(window->xkb_keymap);
        }

        if (window->xkb_state)
        {
            xkb_state_unref(window->xkb_state);
        }

        window->xkb_keymap = xkb_keymap_new_from_string(
            window->xkb_context, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        window->xkb_state = window->xkb_keymap ? xkb_state_new(window->xkb_keymap) : nullptr;

        munmap(map_str, size);
        close(fd);
    }

    void keyboard_enter(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                        struct wl_surface* surface, struct wl_array* keys)
    {
        MANGO_UNREFERENCED(keyboard);
        MANGO_UNREFERENCED(serial);
        MANGO_UNREFERENCED(surface);

        WindowContext* window = static_cast<WindowContext*>(data);
        window->keyboard_focused = true;

        if (!window->xkb_state || !keys)
        {
            return;
        }

        const uint32_t* key_array = static_cast<const uint32_t*>(keys->data);
        const size_t count = keys->size / sizeof(uint32_t);

        for (size_t i = 0; i < count; ++i)
        {
            const uint32_t keycode = key_array[i] + 8;
            xkb_state_update_key(window->xkb_state, keycode, XKB_KEY_DOWN);

            const xkb_keysym_t sym = xkb_state_key_get_one_sym(window->xkb_state, keycode);
            setKeyPressed(window, translateKeysym(sym), true);
        }
    }

    void keyboard_leave(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface)
    {
        MANGO_UNREFERENCED(keyboard);
        MANGO_UNREFERENCED(serial);
        MANGO_UNREFERENCED(surface);

        clearKeyState(static_cast<WindowContext*>(data));
    }

    void keyboard_key(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                      uint32_t time, uint32_t key, uint32_t state)
    {
        MANGO_UNREFERENCED(keyboard);
        MANGO_UNREFERENCED(serial);
        MANGO_UNREFERENCED(time);

        WindowContext* window = static_cast<WindowContext*>(data);
        if (!window->xkb_state || !window->owner)
        {
            return;
        }

        const uint32_t keycode = key + 8;
        xkb_state_update_key(window->xkb_state, keycode,
            state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);

        const xkb_keysym_t sym = xkb_state_key_get_one_sym(window->xkb_state, keycode);
        const Keycode code = translateKeysym(sym);

        if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
        {
            setKeyPressed(window, code, true);
            window->owner->onKeyPress(code, getKeyMask(window->xkb_state));
        }
        else if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        {
            setKeyPressed(window, code, false);
            window->owner->onKeyRelease(code);
        }
    }

    void keyboard_modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                            uint32_t mods_depressed, uint32_t mods_latched,
                            uint32_t mods_locked, uint32_t group)
    {
        MANGO_UNREFERENCED(keyboard);
        MANGO_UNREFERENCED(serial);

        WindowContext* window = static_cast<WindowContext*>(data);
        if (window->xkb_state)
        {
            xkb_state_update_mask(window->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
        }
    }

    void keyboard_repeat_info(void* data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(keyboard);
        MANGO_UNREFERENCED(rate);
        MANGO_UNREFERENCED(delay);
    }

    static const struct wl_keyboard_listener keyboard_listener =
    {
        .keymap = keyboard_keymap,
        .enter = keyboard_enter,
        .leave = keyboard_leave,
        .key = keyboard_key,
        .modifiers = keyboard_modifiers,
        .repeat_info = keyboard_repeat_info,
    };

    void seat_capabilities(void* data, struct wl_seat* seat, uint32_t caps)
    {
        WindowContext* window = static_cast<WindowContext*>(data);

        if ((caps & WL_SEAT_CAPABILITY_POINTER) && !window->pointer)
        {
            window->pointer = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(window->pointer, &pointer_listener, window);
        }
        else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && window->pointer)
        {
            wl_pointer_destroy(window->pointer);
            window->pointer = nullptr;
            window->pointer_focused = false;
        }

        if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !window->keyboard)
        {
            window->keyboard = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(window->keyboard, &keyboard_listener, window);
        }
        else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && window->keyboard)
        {
            clearKeyState(window);
            wl_keyboard_destroy(window->keyboard);
            window->keyboard = nullptr;
        }
    }

    void seat_name(void* data, struct wl_seat* seat, const char* name)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(seat);
        MANGO_UNREFERENCED(name);
    }

    static const struct wl_seat_listener seat_listener =
    {
        .capabilities = seat_capabilities,
        .name = seat_name,
    };

    struct WaylandOutput
    {
        struct wl_output* output = nullptr;
        uint32_t registry_name = 0;
        int32_t width = 0;
        int32_t height = 0;
        int32_t scale = 1;
    };

    std::vector<std::unique_ptr<WaylandOutput>> g_outputs;

    int32_t getPrimaryBufferScale()
    {
        int32_t scale = 1;

        for (const auto& output : g_outputs)
        {
            scale = std::max(scale, output->scale);
        }

        return scale;
    }

    void output_geometry(void* data, struct wl_output* output, int32_t x, int32_t y,
                         int32_t physical_width, int32_t physical_height, int32_t subpixel,
                         const char* make, const char* model, int32_t transform)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(output);
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
        MANGO_UNREFERENCED(physical_width);
        MANGO_UNREFERENCED(physical_height);
        MANGO_UNREFERENCED(subpixel);
        MANGO_UNREFERENCED(make);
        MANGO_UNREFERENCED(model);
        MANGO_UNREFERENCED(transform);
    }

    void output_mode(void* data, struct wl_output* output, uint32_t flags,
                     int32_t width, int32_t height, int32_t refresh)
    {
        MANGO_UNREFERENCED(output);
        MANGO_UNREFERENCED(refresh);

        WaylandOutput* info = static_cast<WaylandOutput*>(data);
        if (flags & WL_OUTPUT_MODE_CURRENT)
        {
            info->width = width;
            info->height = height;
        }
    }

    void output_done(void* data, struct wl_output* output)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(output);
    }

    void output_scale(void* data, struct wl_output* output, int32_t factor)
    {
        MANGO_UNREFERENCED(output);

        WaylandOutput* info = static_cast<WaylandOutput*>(data);
        info->scale = factor;
    }

    void output_name(void* data, struct wl_output* output, const char* name)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(output);
        MANGO_UNREFERENCED(name);
    }

    void output_description(void* data, struct wl_output* output, const char* description)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(output);
        MANGO_UNREFERENCED(description);
    }

    static const struct wl_output_listener output_listener =
    {
        .geometry = output_geometry,
        .mode = output_mode,
        .done = output_done,
        .scale = output_scale,
        .name = output_name,
        .description = output_description,
    };

    void registry_global(void* data, struct wl_registry* registry,
                         uint32_t name, const char* interface, uint32_t version)
    {
        WindowContext* window = static_cast<WindowContext*>(data);

        if (std::strcmp(interface, "wl_compositor") == 0)
        {
            window->compositor = static_cast<struct wl_compositor*>(
                wl_registry_bind(registry, name, &wl_compositor_interface, 4));
        }
        else if (std::strcmp(interface, "xdg_wm_base") == 0)
        {
            const uint32_t bind_version = version < 4 ? version : 4;
            window->xdg_wm_base = static_cast<struct xdg_wm_base*>(
                wl_registry_bind(registry, name, &xdg_wm_base_interface, bind_version));
            xdg_wm_base_add_listener(window->xdg_wm_base, &xdg_wm_base_listener, window);
        }
        else if (std::strcmp(interface, "wl_seat") == 0)
        {
            window->seat = static_cast<struct wl_seat*>(
                wl_registry_bind(registry, name, &wl_seat_interface, 5));
            wl_seat_add_listener(window->seat, &seat_listener, window);
        }
        else if (std::strcmp(interface, "wl_output") == 0)
        {
            const uint32_t bind_version = version < 4 ? version : 4;
            auto output = std::make_unique<WaylandOutput>();
            output->registry_name = name;
            output->output = static_cast<struct wl_output*>(
                wl_registry_bind(registry, name, &wl_output_interface, bind_version));
            wl_output_add_listener(output->output, &output_listener, output.get());
            g_outputs.push_back(std::move(output));
        }
    }

    void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name)
    {
        MANGO_UNREFERENCED(data);
        MANGO_UNREFERENCED(registry);

        for (auto it = g_outputs.begin(); it != g_outputs.end(); ++it)
        {
            if ((*it)->registry_name == name)
            {
                if ((*it)->output)
                {
                    wl_output_destroy((*it)->output);
                }

                g_outputs.erase(it);
                break;
            }
        }
    }

    static const struct wl_registry_listener registry_listener =
    {
        .global = registry_global,
        .global_remove = registry_global_remove,
    };

} // namespace

namespace mango
{
    using namespace mango::math;

    // -----------------------------------------------------------------------
    // WindowContext
    // -----------------------------------------------------------------------

    WindowContext::WindowContext(int width, int height, u32 flags)
    {
        MANGO_UNREFERENCED(flags);

        size[0] = width;
        size[1] = height;

        display = wl_display_connect(nullptr);
        if (!display)
        {
            MANGO_EXCEPTION("[Window] Failed to connect to Wayland display.");
        }

        registry = wl_display_get_registry(display);
        if (!registry)
        {
            MANGO_EXCEPTION("[Window] Failed to get Wayland registry.");
        }

        wl_registry_add_listener(registry, &registry_listener, this);
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);

        if (!createWaylandWindow(width, height, "Mango Window"))
        {
            MANGO_EXCEPTION("[Window] Failed to create Wayland window.");
        }

        for (int i = 0; i < 6; ++i)
        {
            mouse_time[i] = 0;
        }
    }

    WindowContext::~WindowContext()
    {
        if (xdg_toplevel) xdg_toplevel_destroy(xdg_toplevel);
        if (xdg_surface) xdg_surface_destroy(xdg_surface);
        if (surface) wl_surface_destroy(surface);
        if (xdg_wm_base) xdg_wm_base_destroy(xdg_wm_base);
        if (pointer) wl_pointer_destroy(pointer);
        if (keyboard) wl_keyboard_destroy(keyboard);
#if defined(MANGO_ENABLE_EGL)
        if (egl_window) wl_egl_window_destroy(static_cast<struct wl_egl_window*>(egl_window));
#endif
        if (seat) wl_seat_destroy(seat);
        if (compositor) wl_compositor_destroy(compositor);
        if (registry) wl_registry_destroy(registry);
        if (display)
        {
            wl_display_flush(display);
            wl_display_disconnect(display);
        }
        if (xkb_state) xkb_state_unref(xkb_state);
        if (xkb_keymap) xkb_keymap_unref(xkb_keymap);
        if (xkb_context) xkb_context_unref(xkb_context);
    }

    void WindowContext::toggleFullscreen()
    {
        if (!xdg_toplevel)
        {
            return;
        }

        if (fullscreen)
        {
            xdg_toplevel_unset_fullscreen(xdg_toplevel);
            fullscreen = false;
        }
        else
        {
            xdg_toplevel_set_fullscreen(xdg_toplevel, nullptr);
            fullscreen = true;
        }

        requestRefresh();
    }

    void WindowContext::requestRefresh()
    {
        needs_redraw = true;
    }

    int32x2 WindowContext::getWindowSize() const
    {
        if (size[0] > 0 && size[1] > 0)
        {
            return int32x2(size[0], size[1]);
        }

#if defined(MANGO_ENABLE_EGL)
        if (egl_window && egl_synced_size[0] > 0 && egl_synced_size[1] > 0)
        {
            return int32x2(egl_synced_size[0], egl_synced_size[1]);
        }
#endif

        return int32x2(size[0], size[1]);
    }

    bool WindowContext::createWaylandWindow(int width, int height, const char* title)
    {
        if (!compositor || !xdg_wm_base)
        {
            return false;
        }

        surface = wl_compositor_create_surface(compositor);
        if (!surface)
        {
            return false;
        }

        xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
        if (!xdg_surface)
        {
            return false;
        }

        xdg_surface_add_listener(xdg_surface, &s_xdg_surface_listener, this);

        xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
        if (!xdg_toplevel)
        {
            return false;
        }

        xdg_toplevel_add_listener(xdg_toplevel, &s_xdg_toplevel_listener, this);
        xdg_toplevel_set_title(xdg_toplevel, title);
        xdg_toplevel_set_app_id(xdg_toplevel, "mango");

        if (width > 0 && height > 0)
        {
            size[0] = width;
            size[1] = height;
        }

        wl_surface_commit(surface);

        configured = false;
        while (!configured)
        {
            if (wl_display_dispatch(display) < 0)
            {
                return false;
            }
        }

        syncSurfaceScale();
        return true;
    }

    void WindowContext::syncSurfaceScale()
    {
        const int32_t scale = getPrimaryBufferScale();
        if (!surface || scale <= 0 || buffer_scale == scale)
        {
            return;
        }

        buffer_scale = scale;
        wl_surface_set_buffer_scale(surface, buffer_scale);
    }

    void WindowContext::syncEGLWindow()
    {
#if defined(MANGO_ENABLE_EGL)
        if (!egl_window || size[0] <= 0 || size[1] <= 0)
        {
            return;
        }

        syncSurfaceScale();

        if (egl_synced_size[0] == size[0] && egl_synced_size[1] == size[1])
        {
            return;
        }

        wl_egl_window_resize(static_cast<struct wl_egl_window*>(egl_window), size[0], size[1], 0, 0);

        egl_synced_size[0] = size[0];
        egl_synced_size[1] = size[1];
#endif
    }

    void WindowContext::dispatchPendingResize()
    {
        if (!owner || busy || !pending_resize)
        {
            return;
        }

        pending_resize = false;

        if (size[0] <= 0 || size[1] <= 0)
        {
            return;
        }

        syncSurfaceScale();
        syncEGLWindow();
        owner->onResize(size[0], size[1]);
    }

    void WindowContext::processEvents()
    {
        while (wl_display_prepare_read(display) != 0)
        {
            wl_display_dispatch_pending(display);
        }

        if (wl_display_read_events(display) < 0)
        {
            is_looping = false;
            return;
        }

        wl_display_dispatch_pending(display);
        wl_display_flush(display);
    }

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    Window::Window(int width, int height, u32 flags)
    {
        m_window_context = std::make_unique<WindowContext>(width, height, flags);
        m_window_context->owner = this;
    }

    Window::~Window()
    {
    }

    int Window::getScreenCount()
    {
        return int(g_outputs.size());
    }

    int32x2 Window::getScreenSize(int index)
    {
        if (index < 0 || index >= int(g_outputs.size()))
        {
            return int32x2(0, 0);
        }

        const WaylandOutput& output = *g_outputs[index];
        return int32x2(output.width, output.height);
    }

    Window::operator WindowHandle () const
    {
        return *m_window_context;
    }

    Window::operator WindowContext* () const
    {
        return m_window_context.get();
    }

    void Window::setWindowPosition(int x, int y)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
    }

    void Window::setWindowSize(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        m_window_context->size[0] = width;
        m_window_context->size[1] = height;
        m_window_context->pending_resize = true;
        m_window_context->syncEGLWindow();
        m_window_context->requestRefresh();
    }

    void Window::setTitle(const std::string& title)
    {
        if (m_window_context->xdg_toplevel && m_window_context->display)
        {
            xdg_toplevel_set_title(m_window_context->xdg_toplevel, title.c_str());
            wl_display_flush(m_window_context->display);
        }
    }

    void Window::setVisible(bool enable)
    {
        if (!m_window_context->surface || !m_window_context->display)
        {
            return;
        }

        if (enable)
        {
            wl_surface_commit(m_window_context->surface);
            wl_display_flush(m_window_context->display);
        }
    }

    int32x2 Window::getWindowSize() const
    {
        return m_window_context->getWindowSize();
    }

    int32x2 Window::getCursorPosition() const
    {
        return int32x2(m_window_context->cursor[0], m_window_context->cursor[1]);
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

        if (!m_window_context->keyboard_focused)
        {
            return false;
        }

        const int idx = int(code);
        if (idx <= 0 || idx >= 256)
        {
            return false;
        }

        return m_window_context->key_pressed[idx];
    }

    void Window::enterEventLoop()
    {
        m_window_context->is_looping = true;

        // Wayland compositors need at least one client buffer before the surface is shown.
        // The application is fully constructed by the time enterEventLoop() is called.
        m_window_context->processEvents();
        m_window_context->syncSurfaceScale();
        m_window_context->syncEGLWindow();

        if (!m_window_context->busy)
        {
            m_window_context->dispatchPendingResize();

            if (m_window_context->size[0] > 0 && m_window_context->size[1] > 0)
            {
                onDraw();
            }
        }

        for (; m_window_context->is_looping;)
        {
            m_window_context->processEvents();

            bool refreshed = false;

            if (m_window_context->pending_resize && !m_window_context->busy)
            {
                m_window_context->dispatchPendingResize();
                onDraw();
                refreshed = true;
                m_window_context->needs_redraw = false;
            }

            if (!refreshed && m_window_context->needs_redraw && !m_window_context->busy)
            {
                onDraw();
                m_window_context->needs_redraw = false;
            }

            if (!m_window_context->busy)
            {
                onIdle();
            }

            usleep(125);
        }
    }

    void Window::breakEventLoop()
    {
        m_window_context->is_looping = false;
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

    using namespace mango::filesystem;

    void Window::onDropFiles(const FileIndex& index)
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

#endif // defined(MANGO_WINDOW_SYSTEM_WAYLAND)
