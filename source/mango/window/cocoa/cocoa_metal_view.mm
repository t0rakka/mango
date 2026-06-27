/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "cocoa_input.hpp"
#include "cocoa_window.h"

#if defined(MANGO_ENABLE_COCOA) && defined(MANGO_ENABLE_VULKAN)

#import <QuartzCore/QuartzCore.h>

// -----------------------------------------------------------------------
// MangoMetalView
// -----------------------------------------------------------------------

@implementation MangoMetalView

- (id)initWithFrame:(NSRect)frame window:(mango::Window*)window context:(mango::WindowContext*)context
{
    if ((self = [super initWithFrame:frame]))
    {
        mangoWindow = window;
        mangoContext = context;

        self.wantsLayer = YES;
        self.layer = [CAMetalLayer layer];
        self.layer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];

        [self registerForDraggedTypes:[NSArray arrayWithObjects:NSPasteboardTypeFileURL, nil]];
    }

    return self;
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    self.layer.contentsScale = self.window.backingScaleFactor;
    mangoContext->updateMetalDrawableSize();
}

- (void)dispatchResize:(NSRect)frame
{
    mango::cocoa::dispatchResize(mangoWindow, mangoContext, self, frame);
}

- (void)trackContentView:(NSWindow*)window
{
    mango::cocoa::trackContentView(self, window);
}

- (void)keyDown:(NSEvent*)event
{
    mango::cocoa::viewKeyDown(mangoWindow, mangoContext, event);
}

- (void)keyUp:(NSEvent*)event
{
    mango::cocoa::viewKeyUp(mangoWindow, mangoContext, event);
}

- (void)mouseMoved:(NSEvent*)event { mango::cocoa::viewMouseMove(mangoWindow, self, event); }
- (void)mouseDragged:(NSEvent*)event { mango::cocoa::viewMouseMove(mangoWindow, self, event); }
- (void)rightMouseDragged:(NSEvent*)event { mango::cocoa::viewMouseMove(mangoWindow, self, event); }
- (void)otherMouseDragged:(NSEvent*)event { mango::cocoa::viewMouseMove(mangoWindow, self, event); }

- (void)mouseDown:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MOUSEBUTTON_LEFT, int([event clickCount]));
}

- (void)mouseUp:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MOUSEBUTTON_LEFT, 0);
}

- (void)rightMouseDown:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MOUSEBUTTON_RIGHT, int([event clickCount]));
}

- (void)rightMouseUp:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MOUSEBUTTON_RIGHT, 0);
}

- (void)otherMouseDown:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MouseButton(event.buttonNumber), int([event clickCount]));
}

- (void)otherMouseUp:(NSEvent*)event
{
    mango::cocoa::viewMouseClick(mangoWindow, self, event, mango::MouseButton(event.buttonNumber), 0);
}

- (void)scrollWheel:(NSEvent*)event
{
    mango::cocoa::viewScrollWheel(mangoWindow, self, event);
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    if ((NSDragOperationGeneric & [sender draggingSourceOperationMask]) == NSDragOperationGeneric)
        return NSDragOperationGeneric;
    return NSDragOperationNone;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender
{
    (void)sender;
    return YES;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    mango::cocoa::viewDropFiles(mangoWindow, (__bridge void*)sender);
    return YES;
}

@end

#endif // defined(MANGO_ENABLE_COCOA) && defined(MANGO_ENABLE_VULKAN)
