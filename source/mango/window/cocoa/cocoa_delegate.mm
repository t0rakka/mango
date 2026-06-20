/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "cocoa_input.hpp"
#include "cocoa_window.h"

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

// -----------------------------------------------------------------------
// CocoaWindowDelegate
// -----------------------------------------------------------------------

@interface CocoaWindowDelegate : NSObject
{
    mango::Window* mangoWindow;
    NSView* contentView;
}

- (id)initWithWindow:(mango::Window*)window view:(NSView*)view;

@end

@implementation CocoaWindowDelegate

- (id)initWithWindow:(mango::Window*)window view:(NSView*)view
{
    if ((self = [super init]))
    {
        mangoWindow = window;
        contentView = view;
    }

    return self;
}

- (BOOL)windowShouldClose:(id)sender
{
    (void)sender;
    mangoWindow->breakEventLoop();
    return NO;
}

- (void)windowDidResize:(NSNotification*)notification
{
    NSWindow* window = [notification object];
    NSRect frame = [window contentRectForFrameRect:[window frame]];

    mango::WindowContext* context = mangoWindow->operator mango::WindowContext*();
    mango::cocoa::dispatchResize(mangoWindow, context, contentView, frame);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication
{
    (void)theApplication;
    return YES;
}

@end

namespace mango::cocoa
{

    static
    id allocWindowDelegate(Window* window, NSView* view)
    {
        return [[CocoaWindowDelegate alloc] initWithWindow:window view:view];
    }

    void* createWindowDelegate(Window* window, void* view)
    {
        return (__bridge void*)allocWindowDelegate(window, (__bridge NSView*)view);
    }

    void initApplication()
    {
        [NSApplication sharedApplication];

        ProcessSerialNumber psn = { 0, kCurrentProcess };
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];

        [NSEvent setMouseCoalescingEnabled:NO];
        [NSApp finishLaunching];
    }

} // namespace mango::cocoa

#endif // defined(MANGO_WINDOW_SYSTEM_COCOA)
