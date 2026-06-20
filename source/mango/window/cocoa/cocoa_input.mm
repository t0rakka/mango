/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "cocoa_input.hpp"

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

#import <Cocoa/Cocoa.h>
#include <mango/filesystem/filesystem.hpp>

namespace mango::cocoa
{

    Keycode translateKeyCode(u32 code)
    {
        static const Keycode table [] =
        {
            KEYCODE_A, KEYCODE_S, KEYCODE_D, KEYCODE_F, KEYCODE_H, KEYCODE_G, KEYCODE_Z, KEYCODE_X, KEYCODE_C, KEYCODE_V,
            KEYCODE_NONE, KEYCODE_B, KEYCODE_Q, KEYCODE_W, KEYCODE_E, KEYCODE_R, KEYCODE_Y, KEYCODE_T, KEYCODE_1, KEYCODE_2,
            KEYCODE_3, KEYCODE_4, KEYCODE_6, KEYCODE_5, KEYCODE_NONE, KEYCODE_9, KEYCODE_7, KEYCODE_NONE, KEYCODE_8, KEYCODE_0,
            KEYCODE_NONE, KEYCODE_O, KEYCODE_U, KEYCODE_NONE, KEYCODE_I, KEYCODE_P, KEYCODE_RETURN, KEYCODE_L, KEYCODE_J, KEYCODE_NONE,
            KEYCODE_K, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_N, KEYCODE_M, KEYCODE_NONE, KEYCODE_TAB, KEYCODE_SPACE,
            KEYCODE_NONE, KEYCODE_BACKSPACE, KEYCODE_NONE, KEYCODE_ESC, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_LEFT_SHIFT, KEYCODE_CAPS_LOCK, KEYCODE_LEFT_ALT, KEYCODE_LEFT_CONTROL,
            KEYCODE_RIGHT_SHIFT, KEYCODE_RIGHT_ALT, KEYCODE_RIGHT_CONTROL, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_DECIMAL, KEYCODE_NONE, KEYCODE_MULTIPLY, KEYCODE_NONE, KEYCODE_ADDITION,
            KEYCODE_NONE, KEYCODE_NUMLOCK, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_DIVIDE, KEYCODE_ENTER, KEYCODE_NONE, KEYCODE_SUBTRACT, KEYCODE_NONE,
            KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NUMPAD0, KEYCODE_NUMPAD1, KEYCODE_NUMPAD2, KEYCODE_NUMPAD3, KEYCODE_NUMPAD4, KEYCODE_NUMPAD5, KEYCODE_NUMPAD6, KEYCODE_NUMPAD7,
            KEYCODE_NONE, KEYCODE_NUMPAD8, KEYCODE_NUMPAD9, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_F5, KEYCODE_F6, KEYCODE_F7, KEYCODE_F3,
            KEYCODE_F8, KEYCODE_F9, KEYCODE_NONE, KEYCODE_F11, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_F10,
            KEYCODE_NONE, KEYCODE_F12, KEYCODE_NONE, KEYCODE_NONE, KEYCODE_INSERT, KEYCODE_HOME, KEYCODE_PAGE_UP, KEYCODE_DELETE, KEYCODE_F4, KEYCODE_END,
            KEYCODE_F2, KEYCODE_PAGE_DOWN, KEYCODE_F1, KEYCODE_LEFT, KEYCODE_RIGHT, KEYCODE_DOWN, KEYCODE_UP, KEYCODE_NONE,
        };

        if (code >= 128)
        {
            return KEYCODE_NONE;
        }

        return table[code];
    }

    u32 translateKeyMask(unsigned long flags)
    {
        u32 mask = 0;
        if (flags & NSEventModifierFlagControl) mask |= KEYMASK_CONTROL;
        if (flags & NSEventModifierFlagShift)   mask |= KEYMASK_SHIFT;
        if (flags & NSEventModifierFlagCommand) mask |= KEYMASK_SUPER;
        return mask;
    }

    void setKeyState(WindowContext* context, Keycode code, bool press)
    {
        const int keyIndex = int(code) >> 5;
        const int keyMask = 1 << (int(code) & 31);

        if (press)
        {
            context->keystate[keyIndex] |= keyMask;
        }
        else
        {
            context->keystate[keyIndex] &= ~keyMask;
        }
    }

    void dispatchResize(Window* window, WindowContext* context, NSView* view, const NSRect& frame)
    {
        NSRect backing = [view convertRectToBacking:frame];
        if (context->is_vulkan)
        {
            context->updateMetalDrawableSize();
        }
        window->onResize(int(backing.size.width), int(backing.size.height));
    }

    void trackContentView(NSView* view, NSWindow* window)
    {
        view.translatesAutoresizingMaskIntoConstraints = NO;

        NSArray* verticalConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|[subView]|"
                                                                               options:0
                                                                               metrics:nil
                                                                               views:@{ @"subView" : view }];

        NSArray* horizontalConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[subView]|"
                                                                                 options:0
                                                                                 metrics:nil
                                                                                 views:@{ @"subView" : view }];

        [[window contentView] addConstraints:verticalConstraints];
        [[window contentView] addConstraints:horizontalConstraints];
    }

    void viewKeyDown(Window* window, WindowContext* context, NSEvent* event)
    {
        if (event.isARepeat != YES)
        {
            u32 mask = translateKeyMask(event.modifierFlags);
            Keycode code = translateKeyCode(u32(event.keyCode));
            window->onKeyPress(code, mask);
            setKeyState(context, code, true);
        }
    }

    void viewKeyUp(Window* window, WindowContext* context, NSEvent* event)
    {
        Keycode code = translateKeyCode(u32(event.keyCode));
        window->onKeyRelease(code);
        setKeyState(context, code, false);
    }

    static
    void dispatchMouseMoveInternal(Window* window, NSView* view, NSEvent* event)
    {
        NSPoint pos = [view convertPoint:[event locationInWindow] fromView:nil];
        NSRect frame = [view frame];
        pos = [view convertPointToBacking:pos];

        int x = int(pos.x);
        int y = int(frame.size.height - pos.y);
        window->onMouseMove(x, y);
    }

    void viewMouseMove(Window* window, NSView* view, NSEvent* event)
    {
        dispatchMouseMoveInternal(window, view, event);
    }

    void viewMouseClick(Window* window, NSView* view, NSEvent* event, MouseButton button, int clickCount)
    {
        NSPoint pos = [view convertPoint:[event locationInWindow] fromView:nil];
        NSRect frame = [view frame];
        pos = [view convertPointToBacking:pos];

        int x = int(pos.x);
        int y = int(frame.size.height - pos.y);
        window->onMouseClick(x, y, button, clickCount);
    }

    void viewScrollWheel(Window* window, NSView* view, NSEvent* event)
    {
        viewMouseClick(window, view, event, MOUSEBUTTON_WHEEL, int(event.deltaY * 10.0));
    }

    void viewDropFiles(Window* window, void* draggingInfo)
    {
        id<NSDraggingInfo> sender = (__bridge id<NSDraggingInfo>)draggingInfo;
        NSPasteboard* pasteboard = [sender draggingPasteboard];

        NSArray* classes = [NSArray arrayWithObject:[NSURL class]];
        NSDictionary* options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                            forKey:NSPasteboardURLReadingFileURLsOnlyKey];
        NSArray* fileURLs = [pasteboard readObjectsForClasses:classes options:options];

        int numberOfFiles = int([fileURLs count]);
        if (numberOfFiles <= 0)
        {
            return;
        }

        filesystem::FileIndex dropped;

        for (int i = 0; i < numberOfFiles; ++i)
        {
            NSString* path = [[fileURLs objectAtIndex:i] path];

            BOOL isDirectory;
            [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory];

            std::string s = [path UTF8String];
            if (isDirectory)
            {
                dropped.emplace(s + "/", 0, filesystem::FileInfo::Directory);
            }
            else
            {
                u64 size = [[[NSFileManager defaultManager] attributesOfItemAtPath:path error:nil] fileSize];
                dropped.emplace(s, size, 0);
            }
        }

        window->onDropFiles(dropped);
    }

    void toggleViewFullscreen(Window* window, WindowContext* context, NSWindow* nsWindow, NSView* view)
    {
        [view setHidden:YES];

        if ([view isInFullScreenMode])
        {
            [view exitFullScreenModeWithOptions:nil];
            [nsWindow makeFirstResponder:view];
            [nsWindow makeKeyAndOrderFront:view];
            trackContentView(view, nsWindow);
            context->fullscreen = false;
        }
        else
        {
            [view enterFullScreenMode:[nsWindow screen] withOptions:nil];
            context->fullscreen = true;
        }

        dispatchResize(window, context, view, [view frame]);
        [view setHidden:NO];
    }

} // namespace mango::cocoa

#endif // defined(MANGO_WINDOW_SYSTEM_COCOA)
