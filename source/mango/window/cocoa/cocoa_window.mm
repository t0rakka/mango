/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/timer.hpp>
#include "cocoa_window.h"

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

#include <chrono>
#include <thread>

// -----------------------------------------------------------------------
// CustomNSWindow
// -----------------------------------------------------------------------

@implementation CustomNSWindow

@synthesize window;

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
    using namespace mango::image;

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    Window::Window(int width, int height, u32 flags)
    {
        // NOTE: Cocoa/macOS implementation only uses Window as interface and does NOT use the window
        //       constructor for anything else except creating the internal state (m_window_context).
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
        MANGO_UNREFERENCED(flags);

        m_window_context = std::make_unique<WindowContext>();
    }

    Window::~Window()
    {
    }

    int Window::getScreenCount()
    {
        // MANGO TODO: support more than default screen
        return 1;
    }

    math::int32x2 Window::getScreenSize(int screen)
    {
        // MANGO TODO: support more than default screen
        MANGO_UNREFERENCED(screen);

        // NOTE: both of these report incorrect resolution on Retina Display :D
#if 1
        auto display = CGMainDisplayID();
        int width = int(CGDisplayPixelsWide(display));
        int height = int(CGDisplayPixelsHigh(display));
        return math::int32x2(width, height);
#else
        NSRect rect = [[NSScreen mainScreen] frame];
        return math::int32x2(rect.size.width, rect.size.height);
#endif
    }

    void Window::setWindowPosition(int x, int y)
    {
        // NOTE: Retina conversion
        [m_window_context->window setFrameTopLeftPoint:NSMakePoint(x, y)];
    }

    void Window::setWindowSize(int width, int height)
    {
        [m_window_context->window setContentSize:NSMakeSize(width, height)];
    }

    void Window::setTitle(const std::string& title)
    {
        [m_window_context->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void Window::setVisible(bool enable)
    {
        if (enable)
        {
            [m_window_context->window makeKeyAndOrderFront:nil];
        }
        else
        {
            [m_window_context->window orderOut:nil];
        }
    }

    math::int32x2 Window::getWindowSize() const
    {
        // NOTE: Retina conversion
        NSRect rect = [[m_window_context->window contentView] frame];
        rect = [[m_window_context->window contentView] convertRectToBacking:rect];
        return math::int32x2(rect.size.width, rect.size.height);
    }

    math::int32x2 Window::getCursorPosition() const
    {
        NSRect rect = [[m_window_context->window contentView] frame];
        NSPoint point = [m_window_context->window mouseLocationOutsideOfEventStream];
        return math::int32x2(point.x, rect.size.height - point.y - 1);
    }

    bool Window::isKeyPressed(Keycode code) const
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
        u32 state = m_window_context->keystate[keyIndex >> 5] & (1 << (keyIndex & 31));
        bool pressed = state != 0;

        return pressed;
    }

    Window::operator WindowHandle () const
    {
        return nullptr;
    }

    Window::operator WindowContext* () const
    {
        return m_window_context.get();
    }

    void Window::enterEventLoop()
    {
        m_window_context->is_looping = true;

        while (m_window_context->is_looping)
        {
            NSEvent* event = [NSApp nextEventMatchingMask: NSEventMaskAny
                              untilDate: nil
                              inMode: NSDefaultRunLoopMode
                              dequeue: YES];

            if (event)
            {
                //if ([event type] == NSKeyUp && ([event modifierFlags] & NSCommandKeyMask))
                //    [[NSApp keyWindow] sendEvent:event];
                //else
                    [NSApp sendEvent:event];
            }
            else
            {
                onIdle();
            }

            // avoid saturating cpu
            Sleep::us(100);
        }
    }

    void Window::breakEventLoop()
    {
        m_window_context->is_looping = false;
    }

    void Window::onIdle()
    {
    }

    void Window::onDraw()
    {
    }

    void Window::onResize(int width, int height)
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
    }

    void Window::onMinimize()
    {
    }

    void Window::onMaximize()
    {
    }

    void Window::onKeyPress(Keycode code, u32 mask)
    {
        MANGO_UNREFERENCED(code);
        MANGO_UNREFERENCED(mask);
    }

    void Window::onKeyRelease(Keycode code)
    {
        MANGO_UNREFERENCED(code);
    }

    void Window::onMouseMove(int x, int y)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
    }

    void Window::onMouseClick(int x, int y, MouseButton button, int count)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
        MANGO_UNREFERENCED(button);
        MANGO_UNREFERENCED(count);
    }

    void Window::onDropFiles(const filesystem::FileIndex& index)
    {
        MANGO_UNREFERENCED(index);
    }

    void Window::onClose()
    {
    }

    void Window::onShow()
    {
    }

    void Window::onHide()
    {
    }

} // namespace mango

#endif // defined(MANGO_WINDOW_SYSTEM_COCOA)
