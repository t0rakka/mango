/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#ifndef MANGO_OPENGL_CONTEXT_NONE

#include "CustomOpenGLView.h"
#include "../../window/cocoa/cocoa_input.hpp"

@implementation CustomView

- (id)initWithFrame:(NSRect)frame window:(mango::Window*)window context:(mango::WindowContext*)context
{
    if ((self = [super initWithFrame:frame]))
    {
        mangoWindow = window;
        mangoContext = context;
    }

    [self registerForDraggedTypes:[NSArray arrayWithObjects:NSPasteboardTypeFileURL, nil]];
    return self;
}

- (void)prepareOpenGL
{
    [super prepareOpenGL];
    [self setWantsBestResolutionOpenGLSurface:YES];
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)canBecomeKeyView
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)trackContentView:(NSWindow*)window
{
    mango::cocoa::trackContentView(self, window);
}

- (void)dispatchResize:(NSRect)frame
{
    mango::cocoa::dispatchResize(mangoWindow, mangoContext, self, frame);
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
    [super scrollWheel:event];
    mango::cocoa::viewScrollWheel(mangoWindow, self, event);
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;
    mangoWindow->onDraw();
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

- (void)concludeDragOperation:(id<NSDraggingInfo>)sender
{
    (void)sender;
    [self setNeedsDisplay:YES];
}

@end

#endif // MANGO_OPENGL_CONTEXT_NONE
