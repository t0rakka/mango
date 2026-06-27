/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>
#include "../window_backend.hpp"

#if defined(MANGO_ENABLE_XCB)

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

    // Concrete XCB backend. Distinct class name (vs XlibBackend / WaylandBackend)
    // so all three Linux backends can be linked into one binary without ODR clash;
    // the native xcb headers (and the "#define explicit explicit_" hack) live only
    // in this and the GLX-XCB translation unit.
    struct XcbBackend : WindowBackend
    {
        // native handle (formerly in the public WindowHandle)
        xcb_connection_t*   connection { nullptr };
        xcb_window_t        window { 0 };
        xcb_visualid_t      visualid { 0 };

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
        bool fullscreen { false };

        xcb_key_symbols_t* key_symbols;

        XcbBackend();
        ~XcbBackend() override;

        operator xcb_window_t () const { return window; }

        bool init(int width, int height, u32 flags, const char* title);

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

#if defined(MANGO_ENABLE_VULKAN)
        VkSurfaceKHR createVulkanSurface(VkInstance instance) override;
        bool getPresentationSupport(VkPhysicalDevice physicalDevice, u32 queueFamilyIndex) override;
#endif

#if defined(MANGO_ENABLE_EGL)
        // Creates the XCB window and returns its XID packed into a void* for the
        // shared EGL TU. eglNativeDisplay() inherits the base nullptr, mapping to
        // EGL_DEFAULT_DISPLAY (the X11 platform), preserving the original behavior.
        void* eglNativeWindow(int width, int height, u32 flags) override;
#endif
    };

} // namespace mango

#endif // defined(MANGO_ENABLE_XCB)
