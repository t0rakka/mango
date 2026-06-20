/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>
#include "cocoa_window.hpp"

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

namespace mango::cocoa
{

    Keycode translateKeyCode(u32 code);
    u32 translateKeyMask(unsigned long flags);

    void setKeyState(WindowContext* context, Keycode code, bool press);

    void dispatchResize(Window* window, WindowContext* context, NSView* view, const NSRect& frame);
    void trackContentView(NSView* view, NSWindow* window);

    void viewKeyDown(Window* window, WindowContext* context, NSEvent* event);
    void viewKeyUp(Window* window, WindowContext* context, NSEvent* event);
    void viewMouseMove(Window* window, NSView* view, NSEvent* event);
    void viewMouseClick(Window* window, NSView* view, NSEvent* event, MouseButton button, int clickCount);
    void viewScrollWheel(Window* window, NSView* view, NSEvent* event);
    void viewDropFiles(Window* window, void* draggingInfo);

    void toggleViewFullscreen(Window* window, WindowContext* context, NSWindow* nsWindow, NSView* view);

    void initApplication();
    void* createWindowDelegate(Window* window, void* view);

} // namespace mango::cocoa

#endif // defined(MANGO_WINDOW_SYSTEM_COCOA)
