/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/opengl/opengl.hpp>
#include "../../window/cocoa/cocoa_window.h"

#import <Cocoa/Cocoa.h>

namespace mango
{

    struct OpenGLContextHandle
    {
        id delegate;
        id view;
        id ctx = nil;
        u32 modifiers;
    };

} // namespace mango

@interface CustomView : NSOpenGLView {
    mango::OpenGLContext *context;
}

- (void)dispatchResize:(NSRect)frame;
- (id)initWithFrame:(NSRect)frame andCustomWindow:(mango::OpenGLContext *)theContext;
- (void)trackContentView:(NSWindow *)window;

@end
