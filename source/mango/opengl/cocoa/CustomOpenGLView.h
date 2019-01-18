/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "CustomWindow.h"

#import <Cocoa/Cocoa.h>

@interface CustomView : NSOpenGLView {
    mango::opengl::Context *context;
}

- (void)dispatchResize:(NSRect)frame;
- (id)initWithFrame:(NSRect)frame andCustomWindow:(mango::opengl::Context *)theContext;
- (void)trackContentView:(NSWindow *)window;
@end
