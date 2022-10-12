/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/opengl/opengl.hpp>
#include "../../window/cocoa/cocoa_window.h"

#import <Cocoa/Cocoa.h>

@interface CustomView : NSOpenGLView {
    mango::OpenGLContext *context;
}

- (void)dispatchResize:(NSRect)frame;
- (id)initWithFrame:(NSRect)frame andCustomWindow:(mango::OpenGLContext *)theContext;
- (void)trackContentView:(NSWindow *)window;

@end
