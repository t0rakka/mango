/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/timer.hpp>
#include "cocoa_window.h"

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

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
    using namespace mango::image;

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    Window::Window(int width, int height, u32 flags)
    {
        m_window_context = std::make_unique<WindowContext>();

        if (!m_window_context->init(this, width, height, flags, "Mango"))
        {
            MANGO_EXCEPTION("[Window] Creating window failed.");
        }
    }

    Window::~Window()
    {
    }

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

    Window::operator WindowHandle () const
    {
        return *m_window_context;
    }

    Window::operator WindowContext* () const
    {
        return m_window_context.get();
    }

    void Window::setWindowPosition(int x, int y)
    {
        NSWindow* win = (__bridge NSWindow*)m_window_context->ns_window;
        [win setFrameTopLeftPoint:NSMakePoint(x, y)];
    }

    void Window::setWindowSize(int width, int height)
    {
        NSWindow* win = (__bridge NSWindow*)m_window_context->ns_window;
        [win setContentSize:NSMakeSize(width, height)];
    }

    void Window::setTitle(const std::string& title)
    {
        NSWindow* win = (__bridge NSWindow*)m_window_context->ns_window;
        [win setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void Window::setVisible(bool enable)
    {
        NSWindow* win = (__bridge NSWindow*)m_window_context->ns_window;
        if (enable)
        {
            [win makeKeyAndOrderFront:nil];
        }
        else
        {
            [win orderOut:nil];
        }
    }

    math::int32x2 Window::getWindowSize() const
    {
        if (m_window_context->content_view)
        {
            return m_window_context->getContentSize();
        }

        NSWindow* win = (__bridge NSWindow*)m_window_context->ns_window;
        NSRect rect = [[win contentView] frame];
        rect = [[win contentView] convertRectToBacking:rect];
        return math::int32x2(int(rect.size.width), int(rect.size.height));
    }

    math::int32x2 Window::getCursorPosition() const
    {
        NSWindow* win = (__bridge NSWindow*)m_window_context->ns_window;
        NSRect rect = [[win contentView] frame];
        NSPoint point = [win mouseLocationOutsideOfEventStream];
        return math::int32x2(int(point.x), int(rect.size.height - point.y - 1));
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
        return state != 0;
    }

    void Window::enterEventLoop()
    {
        m_window_context->is_looping = true;

        while (m_window_context->is_looping)
        {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                              untilDate:nil
                              inMode:NSDefaultRunLoopMode
                              dequeue:YES];

            if (event)
            {
                [NSApp sendEvent:event];
            }
            else
            {
                onIdle();
            }

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
        MANGO_UNREFERENCED(width); MANGO_UNREFERENCED(height); 
    }

    void Window::onMinimize()
    {
    }
    void Window::onMaximize()
    {
    }

    void Window::onKeyPress(Keycode code, u32 mask)
    {
        MANGO_UNREFERENCED(code); MANGO_UNREFERENCED(mask);
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
