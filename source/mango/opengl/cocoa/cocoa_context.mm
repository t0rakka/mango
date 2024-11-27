/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#ifndef MANGO_OPENGL_CONTEXT_NONE

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

        OpenGLContextCocoa(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* pConfig, OpenGLContext* theShared)
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
                printLine(Print::Error, "NSWindow initWithContentRect failed.");
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
                printLine(Print::Error, "NSView initWithFrame failed.");
                [win release];
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
            if (pConfig)
            {
                // override defaults
                config = *pConfig;
            }

            // force color bits to defaults
            // - there is no reason to want float buffer on macOS OpenGL
            config.red   = 8;
            config.green = 8;
            config.blue  = 8;
            config.alpha = 8;

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
                printLine(Print::Error, "NSOpenGLPixelFormat initWithAttributes failed.");
                [win release];
                return;
            }

            OpenGLContextCocoa* shared = theShared ? reinterpret_cast<OpenGLContextCocoa*>(theShared) : nullptr;

            ctx = [[NSOpenGLContext alloc]
                initWithFormat:pixelFormat
                shareContext: shared ? shared->ctx : nil];
            [pixelFormat release];

            if (!ctx)
            {
                printLine(Print::Error, "Failed to create NSOpenGL Context.");
                [win release];
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
                [win release];
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

    OpenGLContextHandle* createOpenGLContextCocoa(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        auto* context = new OpenGLContextCocoa(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango

#endif // MANGO_OPENGL_CONTEXT_NONE
