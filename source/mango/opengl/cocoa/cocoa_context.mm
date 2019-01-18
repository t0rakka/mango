/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
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
    mango::opengl::Context *context;
    mango::WindowHandle *handle;
}

- (id)initWithCustomWindow:(mango::opengl::Context *)theContext andHandle:(mango::WindowHandle *)theHandle;
@end

// ...

@implementation CustomNSWindowDelegate

- (id)initWithCustomWindow:(mango::opengl::Context *)theContext andHandle:(mango::WindowHandle *)theHandle
{
    if ((self = [super init]))
    {
        context = theContext;
        handle = theHandle;
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
    [handle->view dispatchResize:frame];
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

namespace mango {
namespace opengl {

    // -----------------------------------------------------------------------
    // Context
    // -----------------------------------------------------------------------

    Context::Context(int width, int height, const ContextAttribute* contextAttribute, Context* shared)
	    : Window(width, height)
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

        m_handle->view = nil;
        
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
        [m_handle->window center];
        m_handle->delegate = [[CustomNSWindowDelegate alloc] initWithCustomWindow:this andHandle:m_handle];
        [m_handle->window setDelegate:[m_handle->delegate retain]];
        [m_handle->window setAcceptsMouseMovedEvents:YES];
        [m_handle->window setTitle:@"OpenGL"];
        [m_handle->window setReleasedWhenClosed:NO];
        
        m_handle->view = [[CustomView alloc] initWithFrame:[m_handle->window frame]andCustomWindow:this];
        if (!m_handle->view)
        {
            printf("NSView initWithFrame failed.\n");
            // TODO: delete window
            return;
        }
        
        m_handle->view = [m_handle->view retain];
#if 0
        [m_handle->window setContentView:m_view];
#else
        [[m_handle->window contentView] addSubview:m_handle->view];
        [m_handle->view trackContentView:m_handle->window];
#endif

        // Configure the smallest allowed window size
        [m_handle->window setMinSize:NSMakeSize(32, 32)];

        // Create menu
        [m_handle->window createMenu];
        
        // configure attributes
        ContextAttribute attrib;
        if (contextAttribute)
        {
            // override defaults
            attrib = *contextAttribute;
        }

        std::vector<NSOpenGLPixelFormatAttribute> attribs;

#ifdef MANGO_CORE_PROFILE
        attribs.push_back(NSOpenGLPFAOpenGLProfile);
        attribs.push_back(NSOpenGLProfileVersion3_2Core);
#endif
        attribs.push_back(NSOpenGLPFAAccelerated);
        attribs.push_back(NSOpenGLPFADoubleBuffer);
        //attribs.push_back(NSOpenGLPFATripleBuffer);
        //attribs.push_back(NSOpenGLPFAFullScreen);
        //attribs.push_back(NSOpenGLPFAWindow);

        attribs.push_back(NSOpenGLPFAColorSize);
        attribs.push_back(attrib.red + attrib.green + attrib.blue);
        attribs.push_back(NSOpenGLPFAAlphaSize);
        attribs.push_back(attrib.alpha);
        attribs.push_back(NSOpenGLPFADepthSize);
        attribs.push_back(attrib.depth);
        attribs.push_back(NSOpenGLPFAStencilSize);
        attribs.push_back(attrib.stencil);

        if (attrib.samples > 1)
        {
            attribs.push_back(NSOpenGLPFASampleBuffers);
            attribs.push_back(1);
            attribs.push_back(NSOpenGLPFASamples);
            attribs.push_back(attrib.samples);
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
        
        m_handle->ctx = [[NSOpenGLContext alloc]
                  initWithFormat:pixelFormat
                  shareContext: shared ? shared->m_handle->ctx : nil];
        [pixelFormat release];

        if (!m_handle->ctx)
        {
            printf("Failed to create NSOpenGL Context.\n");
            // TODO: delete window
            return;
        }

        [m_handle->view setOpenGLContext:m_handle->ctx];
        [m_handle->ctx makeCurrentContext];

        if ([m_handle->window respondsToSelector:@selector(setRestorable:)])
        {
            [m_handle->window setRestorable:NO];
        }

        m_handle->modifiers = [NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

        // parse extension string
#ifdef MANGO_CORE_PROFILE
        GLint numExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        for (int i = 0; i < numExtensions; ++i)
        {
            const GLubyte* extension = glGetStringi(GL_EXTENSIONS, i);
            m_extensions.insert(reinterpret_cast<const char*>(extension));
        }
#else
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        if (extensions)
        {
            parseExtensionString(m_extensions, reinterpret_cast<const char*>(extensions));
        }
#endif

        initExtensionMask();

        [m_handle->ctx update];
        [m_handle->view dispatchResize:frame];
        setVisible(true);
    }

    Context::~Context()
    {
        [NSOpenGLContext clearCurrentContext];

        if (m_handle->ctx)
        {
            [m_handle->ctx release];
            m_handle->ctx = nil;
        }

        if (m_handle->window)
        {
            [m_handle->window setDelegate:nil];
            [m_handle->delegate release];
            [m_handle->window setContentView:nil];
            [m_handle->view release];
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
    }

    void Context::makeCurrent()
    {
        [m_handle->ctx makeCurrentContext];
    }

    void Context::swapBuffers()
    {
        [m_handle->ctx flushBuffer];
    }

    void Context::swapInterval(int interval)
    {
        GLint sync = interval;
        [m_handle->ctx setValues:&sync forParameter:NSOpenGLContextParameterSwapInterval];
    }

    void Context::toggleFullscreen()
    {
        CustomView* view = m_handle->view;
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

    bool Context::isFullscreen() const
	{
        return [m_handle->view isInFullScreenMode];
	}

} // namespace opengl
} // namespace mango
