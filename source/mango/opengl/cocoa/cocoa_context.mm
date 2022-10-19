/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#ifndef MANGO_OPENGL_CONTEXT_NONE

#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#include "CustomOpenGLView.h"

// -----------------------------------------------------------------------
// CustomNSWindowDelegate
// -----------------------------------------------------------------------

@interface CustomNSWindowDelegate : NSObject {
    mango::OpenGLContext *context;
    id view;
    mango::WindowHandle *window_handle;
}

- (id)initWithCustomWindow:(mango::OpenGLContext *)theContext 
      andView:(id)theView
      andWindowHandle:(mango::WindowHandle *)theWindowHandle;
@end

// ...

@implementation CustomNSWindowDelegate

- (id)initWithCustomWindow:(mango::OpenGLContext *)theContext 
      andView:(id)theView
      andWindowHandle:(mango::WindowHandle *)theWindowHandle;
{
    if ((self = [super init]))
    {
        context = theContext;
        window_handle = theWindowHandle;
        view = theView;
    }

    return self;
}

- (BOOL)windowShouldClose:(id)sender
{
    (void)sender;
    context->breakEventLoop();
    return NO;
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    (void)notification;
    // NOTE: window gained focus
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    (void)notification;
    // NOTE: window lost focus
}

- (void)windowDidResize:(NSNotification *)notification
{
    //[handle->ctx update];
    NSRect frame = [[notification object] contentRectForFrameRect: [[notification object] frame]];
    [view dispatchResize:frame];
}

- (void)windowDidMove:(NSNotification *)notification
{
    (void)notification;
    //[handle->ctx update];
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
    (void)notification;
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
    (void)notification;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}

@end

namespace mango
{

    // -----------------------------------------------------------------------
    // OpenGLContextCocoa
    // -----------------------------------------------------------------------

    struct OpenGLContextCocoa : OpenGLContextHandle
    {
        id delegate;
        id view = nil;
        id ctx = nil;
        id win;
        u32 modifiers;

        WindowHandle* window;

        OpenGLContextCocoa(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* theShared)
            : window(*theContext)
        {
            [NSApplication sharedApplication];

            ProcessSerialNumber psn = { 0, kCurrentProcess };
            TransformProcessType(&psn, kProcessTransformToForegroundApplication);
            [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];

            [NSEvent setMouseCoalescingEnabled:NO];
            [NSApp finishLaunching];

            unsigned int styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

            NSRect frame = NSMakeRect(0, 0, width, height);

            win = [[CustomNSWindow alloc]
                initWithContentRect:frame
                styleMask:styleMask
                backing:NSBackingStoreBuffered
                defer:NO];
            if (!win)
            {
                debugPrint("NSWindow initWithContentRect failed.\n");
                return;
            }

            window->window = win;

            ((CustomNSWindow *)window->window).window = theContext;
            [ (NSWindow*) win center];
            delegate = [[CustomNSWindowDelegate alloc] initWithCustomWindow:theContext andView:view andWindowHandle:window];
            [win setDelegate:[delegate retain]];
            [win setAcceptsMouseMovedEvents:YES];
            [win setTitle:@"OpenGL"];
            [win setReleasedWhenClosed:NO];

            view = [[CustomView alloc] initWithFrame:[win frame] andCustomWindow:theContext];
            if (!view)
            {
                debugPrint("NSView initWithFrame failed.\n");
                // TODO: delete window
                return;
            }

            view = [view retain];
            [[win contentView] addSubview:view];
            [view trackContentView:win];

            // Configure the smallest allowed window size
            [win setMinSize:NSMakeSize(32, 32)];

            // Create menu
            [win createMenu];

            // configure attributes
            OpenGLContext::Config config;
            if (configPtr)
            {
                // override defaults
                config = *configPtr;
            }

            std::vector<NSOpenGLPixelFormatAttribute> attribs;

            if (!config.version || config.version >= 4)
            {
                attribs.push_back(NSOpenGLPFAOpenGLProfile);
                attribs.push_back(NSOpenGLProfileVersion4_1Core);
            }

            attribs.push_back(NSOpenGLPFAAccelerated);
            attribs.push_back(NSOpenGLPFADoubleBuffer);
            //attribs.push_back(NSOpenGLPFATripleBuffer);
            //attribs.push_back(NSOpenGLPFAFullScreen);
            //attribs.push_back(NSOpenGLPFAWindow);

            attribs.push_back(NSOpenGLPFAColorSize);
            attribs.push_back(config.red + config.green + config.blue);
            attribs.push_back(NSOpenGLPFAAlphaSize);
            attribs.push_back(config.alpha);
            attribs.push_back(NSOpenGLPFADepthSize);
            attribs.push_back(config.depth);
            attribs.push_back(NSOpenGLPFAStencilSize);
            attribs.push_back(config.stencil);

            if (config.samples > 1)
            {
                attribs.push_back(NSOpenGLPFASampleBuffers);
                attribs.push_back(1);
                attribs.push_back(NSOpenGLPFASamples);
                attribs.push_back(config.samples);
                attribs.push_back(NSOpenGLPFAMultisample);
            }

            // terminate attribs vector
            attribs.push_back(0);

            id pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs.data()];
            if (pixelFormat == nil)
            {
                debugPrint("NSOpenGLPixelFormat initWithAttributes failed.\n");
                // TODO: delete window
                return;
            }

            OpenGLContextCocoa* shared = theShared ? reinterpret_cast<OpenGLContextCocoa*>(theShared) : nullptr;

            ctx = [[NSOpenGLContext alloc]
                initWithFormat:pixelFormat
                shareContext: shared ? shared->ctx : nil];
            [pixelFormat release];

            if (!ctx)
            {
                debugPrint("Failed to create NSOpenGL Context.\n");
                // TODO: delete window
                return;
            }

            [view setOpenGLContext:ctx];
            [ctx makeCurrentContext];

            if ([win respondsToSelector:@selector(setRestorable:)])
            {
                [win setRestorable:NO];
            }

            modifiers = [NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

            [ctx update];
            [view dispatchResize:frame];
        }

        ~OpenGLContextCocoa()
        {
            [NSOpenGLContext clearCurrentContext];

            if (ctx)
            {
                [ctx release];
                ctx = nil;
            }

            if (win)
            {
                [win setDelegate:nil];
                [delegate release];
                [win setContentView:nil];
                [view release];
                [win close];
            }

            [NSApp stop:nil];
        }

        void makeCurrent() override
        {
            [ctx makeCurrentContext];
        }

        void swapBuffers() override
        {
            [ctx flushBuffer];
        }

        void swapInterval(int interval) override
        {
            GLint sync = interval;
            [ctx setValues:&sync forParameter:NSOpenGLContextParameterSwapInterval];
        }

        void toggleFullscreen() override
        {
            [view setHidden:YES];

            if ([view isInFullScreenMode])
            {
                [view exitFullScreenModeWithOptions:nil];
                [win makeFirstResponder:view];
                [win makeKeyAndOrderFront:view];
                [view trackContentView:win];
            }
            else
            {
                [view enterFullScreenMode:[win screen] withOptions:nil];
            }

            [view dispatchResize:[view frame]];
            [view setHidden:NO];
        }

        bool isFullscreen() const override
        {
            return [view isInFullScreenMode];
        }

        math::int32x2 getWindowSize() const override
        {
            NSRect rect = [view frame];
            rect = [[win contentView] convertRectToBacking:rect]; // NOTE: Retina conversion
            return math::int32x2(rect.size.width, rect.size.height);
        }
    };

    // -----------------------------------------------------------------------
    // OpenGLContext
    // -----------------------------------------------------------------------

    void OpenGLContext::initContext(int width, int height, u32 flags, const Config* configPtr, OpenGLContext* shared)
    {
        m_context = new OpenGLContextCocoa(this, width, height, flags, configPtr, shared);
    }

} // namespace mango

#endif // MANGO_OPENGL_CONTEXT_NONE
