/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/opengl/opengl.hpp>
#include "../../window/cocoa/cocoa_window.h"

#import <Cocoa/Cocoa.h>

namespace mango {
namespace opengl {

    struct ContextHandle
    {
        id delegate;
        id view;
        id ctx = nil;
        u32 modifiers;
    };

} // namespace opengl
} // namespace mango

@interface CustomView : NSOpenGLView {
    mango::opengl::Context *context;
}

- (void)dispatchResize:(NSRect)frame;
- (id)initWithFrame:(NSRect)frame andCustomWindow:(mango::opengl::Context *)theContext;
- (void)trackContentView:(NSWindow *)window;

@end
