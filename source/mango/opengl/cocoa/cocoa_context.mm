/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#if !defined(__ppc__)

#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>

#include "CustomOpenGLView.h"

namespace
{
    using namespace mango;

    template <typename ContainerType>
    void parseExtensionString(ContainerType& container, const char* ext)
    {
        for (const char* s = ext; *s; ++s)
        {
            if (*s == ' ')
            {
                const std::ptrdiff_t length = s - ext;
                if (length > 0)
                {
                    container.emplace(ext, length);
                }
                ext = s + 1;
            }
        }
    }

} // namespace

// -----------------------------------------------------------------------
// CustomNSWindowDelegate
// -----------------------------------------------------------------------

@interface CustomNSWindowDelegate : NSObject {
    mango::OpenGLContext *context;
    mango::OpenGLContextHandle *context_handle;
    mango::WindowHandle *window_handle;
}

- (id)initWithCustomWindow:(mango::OpenGLContext *)theContext 
      andContextHandle:(mango::OpenGLContextHandle *)theContextHandle
      andWindowHandle:(mango::WindowHandle *)theWindowHandle;
@end

// ...

@implementation CustomNSWindowDelegate

- (id)initWithCustomWindow:(mango::OpenGLContext *)theContext 
      andContextHandle:(mango::OpenGLContextHandle *)theContextHandle
      andWindowHandle:(mango::WindowHandle *)theWindowHandle;
{
    if ((self = [super init]))
    {
        context = theContext;
        window_handle = theWindowHandle;
        context_handle = theContextHandle;
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
    [context_handle->view dispatchResize:frame];
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
    // OpenGLContext
    // -----------------------------------------------------------------------

    OpenGLContext::OpenGLContext(int width, int height, u32 flags, const Config* configPtr, OpenGLContext* shared)
	    : Window(width, height, flags)
        , m_context(nullptr)
    {
        [NSApplication sharedApplication];
        //xxx.autorelease_pool = [[NSAutoreleasePool alloc] init];

        ProcessSerialNumber psn = { 0, kCurrentProcess };
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];

        [NSEvent setMouseCoalescingEnabled:NO];
        [NSApp finishLaunching];

        // "createContext"

        m_context = new OpenGLContextHandle();
        m_context->view = nil;

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

        ((CustomNSWindow *)m_handle->window).window = this;
        [ (NSWindow*) m_handle->window center];
        m_context->delegate = [[CustomNSWindowDelegate alloc] initWithCustomWindow:this andContextHandle:m_context andWindowHandle:m_handle];
        [m_handle->window setDelegate:[m_context->delegate retain]];
        [m_handle->window setAcceptsMouseMovedEvents:YES];
        [m_handle->window setTitle:@"OpenGL"];
        [m_handle->window setReleasedWhenClosed:NO];

        m_context->view = [[CustomView alloc] initWithFrame:[m_handle->window frame] andCustomWindow:this];
        if (!m_context->view)
        {
            printf("NSView initWithFrame failed.\n");
            // TODO: delete window
            return;
        }

        m_context->view = [m_context->view retain];
#if 0
        [m_handle->window setContentView:m_view];
#else
        [[m_handle->window contentView] addSubview:m_context->view];
        [m_context->view trackContentView:m_handle->window];
#endif

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
#if 0
            attribs.push_back(NSOpenGLPFASupersample);
#else
            attribs.push_back(NSOpenGLPFAMultisample);
#endif
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

        m_context->ctx = [[NSOpenGLContext alloc]
                  initWithFormat:pixelFormat
                  shareContext: shared ? shared->m_context->ctx : nil];
        [pixelFormat release];

        if (!m_context->ctx)
        {
            printf("Failed to create NSOpenGL Context.\n");
            // TODO: delete window
            return;
        }

        [m_context->view setOpenGLContext:m_context->ctx];
        [m_context->ctx makeCurrentContext];

        if ([m_handle->window respondsToSelector:@selector(setRestorable:)])
        {
            [m_handle->window setRestorable:NO];
        }

        m_context->modifiers = [NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

        // parse extension string
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        if (extensions)
        {
            parseExtensionString(m_extensions, reinterpret_cast<const char*>(extensions));
        }

        initExtensionMask();

        [m_context->ctx update];
        [m_context->view dispatchResize:frame];
        setVisible(true);
    }

    OpenGLContext::~OpenGLContext()
    {
        [NSOpenGLContext clearCurrentContext];

        if (m_context->ctx)
        {
            [m_context->ctx release];
            m_context->ctx = nil;
        }

        if (m_handle->window)
        {
            [m_handle->window setDelegate:nil];
            [m_context->delegate release];
            [m_handle->window setContentView:nil];
            [m_context->view release];
            [m_handle->window close];
        }

        [NSApp stop:nil];

#if 0
        if (xxx.autorelease_pool)
        {
            [xxx.autorelease_pool drain];
            xxx.autorelease_pool = nil;
        }
#endif
        delete m_context;
    }

    void OpenGLContext::makeCurrent()
    {
        [m_context->ctx makeCurrentContext];
    }

    void OpenGLContext::swapBuffers()
    {
        [m_context->ctx flushBuffer];
    }

    void OpenGLContext::swapInterval(int interval)
    {
        GLint sync = interval;
        [m_context->ctx setValues:&sync forParameter:NSOpenGLContextParameterSwapInterval];
    }

    void OpenGLContext::toggleFullscreen()
    {
        CustomView* view = m_context->view;
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
        return [m_context->view isInFullScreenMode];
	}

    math::int32x2 OpenGLContext::getWindowSize() const
    {
        CustomView* view = m_context->view;
        NSRect rect = [view frame];
        rect = [[m_handle->window contentView] convertRectToBacking:rect]; // NOTE: Retina conversion
        return math::int32x2(rect.size.width, rect.size.height);
    }

} // namespace mango

#endif
