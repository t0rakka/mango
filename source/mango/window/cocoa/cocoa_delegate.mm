/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
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
- (void)deferredSettle;

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

    mango::WindowContext* context = static_cast<mango::WindowContext*>(mangoWindow->backend());
    mango::cocoa::dispatchResize(mangoWindow, context, contentView, frame);
}

// Force one clean redraw on the *next* main-loop turn, after AppKit has finished
// laying out and committed the settled CAMetalLayer geometry. The per-event frame
// driven from windowDidResize during a live resize / fullscreen animation can race
// the final geometry commit on macOS: the frame is rendered and presented, but the
// last drawable lands a beat before the layer settles, so the compositor leaves the
// window gray. Because rendering is on-demand, nothing redraws afterwards and the
// gray sticks. Re-running the resize+present path deferred guarantees a final frame
// composited against the settled size.
- (void)deferredSettle
{
    mango::Window* window = mangoWindow;
    NSView* view = contentView;

    dispatch_async(dispatch_get_main_queue(), ^{
        NSWindow* nsWindow = [view window];
        if (!nsWindow)
        {
            return;
        }

        NSRect frame = [nsWindow contentRectForFrameRect:[nsWindow frame]];
        mango::WindowContext* context = static_cast<mango::WindowContext*>(window->backend());
        mango::cocoa::dispatchResize(window, context, view, frame);
    });
}

- (void)windowDidEndLiveResize:(NSNotification*)notification
{
    (void)notification;
    [self deferredSettle];
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification
{
    (void)notification;
    [self deferredSettle];
}

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
    (void)notification;
    [self deferredSettle];
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
