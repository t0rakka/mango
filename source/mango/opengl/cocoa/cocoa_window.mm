/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#include "CustomOpenGLView.h"

namespace mango
{

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    Window::Window(int width, int height)
    {
        // NOTE: Cocoa/OSX implementation only uses Window as interface and does NOT
        //       use the window constructor for anything else except creating the internal state (m_handle).
        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);

        m_handle = new WindowHandle();
    }

    Window::~Window()
    {
        delete m_handle;
    }

    void Window::setWindowSize(int width, int height)
    {
        [m_handle->window setContentSize:NSMakeSize(width, height)];
    }

    void Window::setTitle(const std::string& title)
    {
        [m_handle->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }

    void Window::setIcon(const Surface& icon)
    {
        const int width = icon.width;
        const int height = icon.height;

        Bitmap bitmap(width, height, FORMAT_R8G8B8A8);
        bitmap.blit(0, 0, icon);

        NSBitmapImageRep* bitmapImage = [[NSBitmapImageRep alloc]
                                         initWithBitmapDataPlanes:&bitmap.image
                                         pixelsWide:width
                                         pixelsHigh:height
                                         bitsPerSample:8
                                         samplesPerPixel:4
                                         hasAlpha:YES
                                         isPlanar:NO
                                         colorSpaceName:NSCalibratedRGBColorSpace
                                         bytesPerRow:width * 4
                                         bitsPerPixel:32];

        NSImage* iconImage = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
        [iconImage addRepresentation:bitmapImage];

        [[NSApplication sharedApplication] setApplicationIconImage:iconImage];
        [bitmapImage release];
        [iconImage release];
    }

    void Window::setVisible(bool enable)
    {
        if (enable)
        {
            [m_handle->window makeKeyAndOrderFront:nil];
        }
        else
        {
            [m_handle->window orderOut:nil];
        }
    }

    int2 Window::getWindowSize() const
    {
        NSRect rect = [[m_handle->window contentView] frame];
        rect = [[m_handle->window contentView] convertRectToBacking:rect];
        return int2(rect.size.width, rect.size.height);
    }

	int2 Window::getCursorPosition() const
	{
        NSRect rect = [[m_handle->window contentView] frame];
		NSPoint point = [m_handle->window mouseLocationOutsideOfEventStream];
		return int2(point.x, rect.size.height - point.y - 1);
	}

    bool Window::isKeyPressed(Keycode code) const
    {
        const int keyIndex = int(code);
        uint32 state = m_handle->keystate[keyIndex >> 5] & (1 << (keyIndex & 31));
        return state != 0;
    }
    
    void Window::enterEventLoop()
    {
#if 1
        m_handle->looping = true;
        
        while (m_handle->looping)
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
        }
#else
        [NSApp run];
#endif
    }
    
    void Window::breakEventLoop()
    {
#if 1
        m_handle->looping = false;
#else
        [NSApp stop:nil];
#endif
    }

    void Window::onIdle()
    {
    }

    void Window::onDraw()
    {
    }

    void Window::onResize(int width, int height)
    {
        MANGO_UNREFERENCED_PARAMETER(width);
        MANGO_UNREFERENCED_PARAMETER(height);
    }

    void Window::onMinimize()
    {
    }

    void Window::onMaximize()
    {
    }

    void Window::onKeyPress(Keycode code, uint32 mask)
    {
        MANGO_UNREFERENCED_PARAMETER(code);
        MANGO_UNREFERENCED_PARAMETER(mask);
    }

    void Window::onKeyRelease(Keycode code)
    {
        MANGO_UNREFERENCED_PARAMETER(code);
    }

    void Window::onMouseMove(int x, int y)
    {
        MANGO_UNREFERENCED_PARAMETER(x);
        MANGO_UNREFERENCED_PARAMETER(y);
    }

    void Window::onMouseClick(int x, int y, MouseButton button, int count)
    {
        MANGO_UNREFERENCED_PARAMETER(x);
        MANGO_UNREFERENCED_PARAMETER(y);
        MANGO_UNREFERENCED_PARAMETER(button);
        MANGO_UNREFERENCED_PARAMETER(count);
    }

    void Window::onDropFiles(const filesystem::FileIndex& index)
    {
        MANGO_UNREFERENCED_PARAMETER(index);
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
