/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/window/window.hpp>

#import <Cocoa/Cocoa.h>

namespace mango
{
    struct WindowHandle
    {
        // window state
        id     window;
        bool   is_looping;
        u32 keystate[4] = { 0, 0, 0, 0 };
    };

} // namespace mango

// -----------------------------------------------------------------------
// CustomNSWindow
// -----------------------------------------------------------------------

@interface CustomNSWindow : NSWindow {
    mango::Window *window;
}

@property (assign) mango::Window *window;

- (void)createMenu;
@end
