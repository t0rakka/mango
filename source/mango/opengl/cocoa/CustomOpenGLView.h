/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif

#import <Cocoa/Cocoa.h>
#include "../../window/cocoa/cocoa_window.hpp"

@interface CustomView : NSOpenGLView
{
@public
    mango::Window* mangoWindow;
    mango::WindowContext* mangoContext;
}

- (id)initWithFrame:(NSRect)frame window:(mango::Window*)window context:(mango::WindowContext*)context;
- (void)dispatchResize:(NSRect)frame;
- (void)trackContentView:(NSWindow*)window;

@end
