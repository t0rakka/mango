/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window/window.hpp>

namespace mango
{
    struct WindowContext;
}

@class NSWindow;

@interface MangoMetalView : NSView
{
@public
    mango::Window* mangoWindow;
    mango::WindowContext* mangoContext;
}

- (id)initWithFrame:(NSRect)frame window:(mango::Window*)window context:(mango::WindowContext*)context;
- (void)dispatchResize:(NSRect)frame;
- (void)trackContentView:(NSWindow*)window;

@end
