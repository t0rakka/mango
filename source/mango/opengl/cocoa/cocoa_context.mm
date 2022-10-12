/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#if !defined(__ppc__)

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
        id view;
        id ctx = nil;
        id window;
        u32 modifiers;

        OpenGLContextCocoa()
        {
        }

        ~OpenGLContextCocoa()
        {
            [NSOpenGLContext clearCurrentContext];

            if (ctx)
            {
                [ctx release];
                ctx = nil;
            }

            if (window)
            {
                [window setDelegate:nil];
                [delegate release];
                [window setContentView:nil];
                [view release];
                [window close];
            }

            [NSApp stop:nil];
        }
    };

    // -----------------------------------------------------------------------
    // OpenGLContext
    // -----------------------------------------------------------------------

    OpenGLContext::OpenGLContext(int width, int height, u32 flags, const Config* configPtr, OpenGLContext* shared)
        : Window(width, height, flags)
        , m_context(nullptr)
    {
        OpenGLContextCocoa* context = new OpenGLContextCocoa();
        m_context = context;

        [NSApplication sharedApplication];

        ProcessSerialNumber psn = { 0, kCurrentProcess };
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];

        [NSEvent setMouseCoalescingEnabled:NO];
        [NSApp finishLaunching];

        context->view = nil;

        unsigned int styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
            NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

        NSRect frame = NSMakeRect(0, 0, width, height);

        m_handle->window = [[CustomNSWindow alloc]
                      initWithContentRect:frame
                      styleMask:styleMask
                      backing:NSBackingStoreBuffered
                      defer:NO];
        if (!m_handle->window)
        {
            printf("NSWindow initWithContentRect failed.\n");
            return;
        }

        context->window = m_handle->window;

        ((CustomNSWindow *)m_handle->window).window = this;
        [ (NSWindow*) m_handle->window center];
        context->delegate = [[CustomNSWindowDelegate alloc] initWithCustomWindow:this andView:context->view andWindowHandle:m_handle];
        [m_handle->window setDelegate:[context->delegate retain]];
        [m_handle->window setAcceptsMouseMovedEvents:YES];
        [m_handle->window setTitle:@"OpenGL"];
        [m_handle->window setReleasedWhenClosed:NO];

        context->view = [[CustomView alloc] initWithFrame:[m_handle->window frame] andCustomWindow:this];
        if (!context->view)
        {
            printf("NSView initWithFrame failed.\n");
            // TODO: delete window
            return;
        }

        context->view = [context->view retain];
        [[m_handle->window contentView] addSubview:context->view];
        [context->view trackContentView:m_handle->window];

        // Configure the smallest allowed window size
        [m_handle->window setMinSize:NSMakeSize(32, 32)];

        // Create menu
        [m_handle->window createMenu];

        // configure attributes
        Config config;
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
            printf("NSOpenGLPixelFormat initWithAttributes failed.\n");
            // TODO: delete window
            return;
        }

        context->ctx = [[NSOpenGLContext alloc]
                  initWithFormat:pixelFormat
                  shareContext: shared ? reinterpret_cast<OpenGLContextCocoa*>(shared->m_context)->ctx : nil];
        [pixelFormat release];

        if (!context->ctx)
        {
            printf("Failed to create NSOpenGL Context.\n");
            // TODO: delete window
            return;
        }

        [context->view setOpenGLContext:context->ctx];
        [context->ctx makeCurrentContext];

        if ([m_handle->window respondsToSelector:@selector(setRestorable:)])
        {
            [m_handle->window setRestorable:NO];
        }

        context->modifiers = [NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

        // parse extension string
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        if (extensions)
        {
            parseExtensionString(m_extensions, reinterpret_cast<const char*>(extensions));
        }

        initExtensionMask();

        [context->ctx update];
        [context->view dispatchResize:frame];
        setVisible(true);
    }

    OpenGLContext::~OpenGLContext()
    {
        delete m_context;
    }

    void OpenGLContext::makeCurrent()
    {
        OpenGLContextCocoa* context = reinterpret_cast<OpenGLContextCocoa*>(m_context);
        [context->ctx makeCurrentContext];
    }

    void OpenGLContext::swapBuffers()
    {
        OpenGLContextCocoa* context = reinterpret_cast<OpenGLContextCocoa*>(m_context);
        [context->ctx flushBuffer];
    }

    void OpenGLContext::swapInterval(int interval)
    {
        OpenGLContextCocoa* context = reinterpret_cast<OpenGLContextCocoa*>(m_context);
        GLint sync = interval;
        [context->ctx setValues:&sync forParameter:NSOpenGLContextParameterSwapInterval];
    }

    void OpenGLContext::toggleFullscreen()
    {
        OpenGLContextCocoa* context = reinterpret_cast<OpenGLContextCocoa*>(m_context);

        CustomView* view = context->view;
        CustomNSWindow* window = m_handle->window;

        [view setHidden:YES];

        if ([view isInFullScreenMode])
        {
            [view exitFullScreenModeWithOptions:nil];
            [window makeFirstResponder:view];
            [window makeKeyAndOrderFront:view];
            [view trackContentView:window];
        }
        else
        {
            [view enterFullScreenMode:[window screen] withOptions:nil];
        }

        [view dispatchResize:[view frame]];
        [view setHidden:NO];
    }

    bool OpenGLContext::isFullscreen() const
    {
        OpenGLContextCocoa* context = reinterpret_cast<OpenGLContextCocoa*>(m_context);
        return [context->view isInFullScreenMode];
    }

    math::int32x2 OpenGLContext::getWindowSize() const
    {
        OpenGLContextCocoa* context = reinterpret_cast<OpenGLContextCocoa*>(m_context);

        CustomView* view = context->view;
        NSRect rect = [view frame];
        rect = [[m_handle->window contentView] convertRectToBacking:rect]; // NOTE: Retina conversion
        return math::int32x2(rect.size.width, rect.size.height);
    }

} // namespace mango

#endif // !defined(__ppc__)
