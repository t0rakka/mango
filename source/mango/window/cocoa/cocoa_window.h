/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "cocoa_window.hpp"

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

#if defined(MANGO_ENABLE_OPENGL)
    #ifndef GL_SILENCE_DEPRECATION
    #define GL_SILENCE_DEPRECATION
    #endif
#endif

#import <Cocoa/Cocoa.h>

// -----------------------------------------------------------------------
// CustomNSWindow
// -----------------------------------------------------------------------

@interface CustomNSWindow : NSWindow
{
@public
    mango::Window* mangoWindow;
}

- (void)createMenu;

@end

#if defined(MANGO_ENABLE_VULKAN)

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

#endif // defined(MANGO_ENABLE_VULKAN)

#endif // defined(MANGO_WINDOW_SYSTEM_COCOA)
