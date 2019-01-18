/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/opengl/opengl.hpp>

#import <Cocoa/Cocoa.h>

namespace mango
{

    struct WindowHandle
    {
        // window state
        id     window;
        bool   looping;
        uint32 keystate[4] = { 0, 0, 0, 0 };

        // context state
        id delegate;
        id view;
        id ctx = nil;
        uint32 modifiers;
    };
    
} // namespace mango

@interface CustomNSWindow : NSWindow {
    mango::Window *window;
}

@property (assign) mango::Window *window;

- (void)createMenu;
@end
