/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/timer.hpp>
#include "cocoa_window.h"
#import <CoreVideo/CoreVideo.h>

// -----------------------------------------------------------------------
// CustomNSWindow
// -----------------------------------------------------------------------

@implementation CustomNSWindow

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (void)createMenu
{
    id menubar = [[NSMenu new] autorelease];
    id appMenuItem = [[NSMenuItem new] autorelease];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];
    id appMenu = [[NSMenu new] autorelease];
    id appName = [[NSProcessInfo processInfo] processName];
    id quitTitle = [@"Quit " stringByAppendingString:appName];
    id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];
}

@end

namespace mango
{
    // -----------------------------------------------------------------------
    // Window (static, screen queries)
    // -----------------------------------------------------------------------

    int Window::getScreenCount()
    {
        return 1;
    }

    math::int32x2 Window::getScreenSize(int screen)
    {
        MANGO_UNREFERENCED(screen);

        auto display = CGMainDisplayID();
        int w = int(CGDisplayPixelsWide(display));
        int h = int(CGDisplayPixelsHigh(display));
        return math::int32x2(w, h);
    }

    // -----------------------------------------------------------------------
    // WindowContext (CocoaBackend)
    // -----------------------------------------------------------------------

    std::unique_ptr<WindowBackend> createCocoaBackend(Window* window, int width, int height, u32 flags, const char* title)
    {
        auto backend = std::make_unique<WindowContext>();
        backend->owner = window;
        if (!backend->init(window, width, height, flags, title))
        {
            return nullptr;
        }
        return backend;
    }

    void WindowContext::setWindowPosition(int x, int y)
    {
        NSWindow* win = (__bridge NSWindow*)ns_window;
        [win setFrameTopLeftPoint:NSMakePoint(x, y)];
    }

    void WindowContext::setWindowSize(int width, int height)
    {
        NSWindow* win = (__bridge NSWindow*)ns_window;
        [win setContentSize:NSMakeSize(width, height)];
    }

    void WindowContext::setTitle(const std::string& title)
    {
        NSWindow* win = (__bridge NSWindow*)ns_window;
        [win setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void WindowContext::setVisible(bool enable)
    {
        NSWindow* win = (__bridge NSWindow*)ns_window;
        if (enable)
        {
            [win makeKeyAndOrderFront:nil];
        }
        else
        {
            [win orderOut:nil];
        }
    }

    math::int32x2 WindowContext::getClientSize() const
    {
        if (!content_view)
        {
            return math::int32x2(0, 0);
        }

        NSView* view = (__bridge NSView*)content_view;
        NSRect rect = [view bounds];
        return math::int32x2(int(rect.size.width), int(rect.size.height));
    }

    float WindowContext::getContentScale() const
    {
        if (!content_view)
        {
            return 1.0f;
        }

        NSView* view = (__bridge NSView*)content_view;
        NSWindow* win = [view window];
        if (win)
        {
            return float([win backingScaleFactor]);
        }

        NSScreen* screen = [NSScreen mainScreen];
        if (screen)
        {
            return float([screen backingScaleFactor]);
        }

        return WindowBackend::getContentScale();
    }

    math::int32x2 WindowContext::getWindowSize() const
    {
        if (content_view)
        {
            return getContentSize();
        }

        NSWindow* win = (__bridge NSWindow*)ns_window;
        NSRect rect = [[win contentView] frame];
        rect = [[win contentView] convertRectToBacking:rect];
        return math::int32x2(int(rect.size.width), int(rect.size.height));
    }

    math::int32x2 WindowContext::getCursorPosition() const
    {
        NSWindow* win = (__bridge NSWindow*)ns_window;
        NSRect rect = [[win contentView] frame];
        NSPoint point = [win mouseLocationOutsideOfEventStream];
        return math::int32x2(int(point.x), int(rect.size.height - point.y - 1));
    }

    bool WindowContext::isKeyPressed(Keycode code) const
    {
        switch (code)
        {
            case KEYCODE_SHIFT:
                return isKeyPressed(KEYCODE_LEFT_SHIFT) || isKeyPressed(KEYCODE_RIGHT_SHIFT);

            case KEYCODE_CONTROL:
                return isKeyPressed(KEYCODE_LEFT_CONTROL) || isKeyPressed(KEYCODE_RIGHT_CONTROL);

            default:
                break;
        }

        const int keyIndex = int(code);
        u32 state = keystate[keyIndex >> 5] & (1 << (keyIndex & 31));
        return state != 0;
    }

    double WindowContext::getDisplayRefreshRate() const
    {
        NSWindow* win = (__bridge NSWindow*)ns_window;
        if (!win)
        {
            return 0.0;
        }

        NSScreen* screen = [win screen];
        if (!screen)
        {
            screen = [NSScreen mainScreen];
        }

        if (!screen)
        {
            return 0.0;
        }

        if (@available(macOS 12.0, *))
        {
            return double([screen maximumFramesPerSecond]);
        }

        CGDirectDisplayID displayID = [[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];
        CVDisplayLinkRef link = nullptr;
        if (CVDisplayLinkCreateWithCGDisplay(displayID, &link) == kCVReturnSuccess)
        {
            const CVTime time = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(link);
            CVDisplayLinkRelease(link);
            if (time.timeScale > 0 && time.timeValue > 0)
            {
                return double(time.timeScale) / double(time.timeValue);
            }
        }

        return 0.0;
    }

    void WindowContext::wakeEventLoop()
    {
        // Post a no-op event so a loop blocked in nextEventMatchingMask returns at
        // once. postEvent:atStart: is safe to call from any thread, so a cross-thread
        // invalidate / requestFrame / breakEventLoop is observed immediately.
        @autoreleasepool
        {
            NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                              location:NSMakePoint(0, 0)
                              modifierFlags:0
                              timestamp:0.0
                              windowNumber:0
                              context:nil
                              subtype:0
                              data1:0
                              data2:0];
            [NSApp postEvent:event atStart:YES];
        }
    }

    void WindowContext::runEventLoop()
    {
        owner->syncDisplayRefreshRate();

        while (owner->isRunning())
        {
            // How long we may block before the next frame is due.
            const u32 timeout = owner->eventLoop().computeWaitTimeoutMs(Time::us());

            NSDate* deadline;
            if (timeout == 0)
            {
                deadline = [NSDate distantPast];        // a frame is due: don't block
            }
            else if (timeout == EventLoopState::WAIT_INFINITE)
            {
                // Nothing scheduled: sleep until an event arrives. wakeEventLoop()
                // posts one whenever the schedule changes, so this never stalls.
                deadline = [NSDate distantFuture];
            }
            else
            {
                deadline = [NSDate dateWithTimeIntervalSinceNow:double(timeout) / 1000.0];
            }

            // Block on the run loop until the first event arrives or the deadline
            // elapses (this is what idles at ~0% CPU instead of busy-polling), then
            // drain any remaining events without blocking.
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                              untilDate:deadline
                              inMode:NSDefaultRunLoopMode
                              dequeue:YES];

            while (event)
            {
                [NSApp sendEvent:event];
                event = [NSApp nextEventMatchingMask:NSEventMaskAny
                         untilDate:[NSDate distantPast]
                         inMode:NSDefaultRunLoopMode
                         dequeue:YES];
            }

            owner->dispatchFrame();
        }
    }

} // namespace mango

#include "../window_registry.hpp"
MANGO_REGISTER_WINDOW_BACKEND(Cocoa, createCocoaBackend);
