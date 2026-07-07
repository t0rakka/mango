/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "cocoa_window.hpp"

#if defined(MANGO_ENABLE_COCOA)

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

#endif // defined(MANGO_ENABLE_COCOA)
