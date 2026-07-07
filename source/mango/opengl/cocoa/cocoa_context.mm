/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/

#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>
#include "../../window/cocoa/cocoa_window.hpp"

#include "CustomOpenGLView.h"

namespace mango
{

    struct OpenGLContextCocoa : OpenGLContext
    {
        WindowContext* window_context;
        id ctx = nil;

        OpenGLContextCocoa(OpenGLWindow* theContext, int width, int height, u32 flags, const OpenGLWindow::Config* pConfig, OpenGLWindow* theShared)
            : window_context(static_cast<WindowContext*>(theContext->backend()))
        {
            NSWindow* win = (__bridge NSWindow*)window_context->ns_window;
            NSRect frame = [win frame];

            CustomView* view = [[CustomView alloc] initWithFrame:frame window:theContext context:window_context];
            if (!view)
            {
                printLine(Print::Error, "NSOpenGLView initWithFrame failed.");
                return;
            }

            window_context->attachContentView(theContext, (__bridge void*)view);
            [view release];

            OpenGLWindow::Config config;
            if (pConfig)
            {
                config = *pConfig;
            }

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

            attribs.push_back(0);

            id pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs.data()];
            if (pixelFormat == nil)
            {
                printLine(Print::Error, "NSOpenGLPixelFormat initWithAttributes failed.");
                return;
            }

            OpenGLContextCocoa* shared = theShared ? reinterpret_cast<OpenGLContextCocoa*>(theShared) : nullptr;

            ctx = [[NSOpenGLContext alloc]
                initWithFormat:pixelFormat
                shareContext:shared ? shared->ctx : nil];
            [pixelFormat release];

            if (!ctx)
            {
                printLine(Print::Error, "Failed to create NSOpenGL Context.");
                return;
            }

            view = (__bridge CustomView*)window_context->content_view;
            [view setOpenGLContext:ctx];
            [ctx makeCurrentContext];
            [ctx update];
            [view dispatchResize:frame];

            theContext->setTitle("OpenGL");
            MANGO_UNREFERENCED(width);
            MANGO_UNREFERENCED(height);
            MANGO_UNREFERENCED(flags);
        }

        ~OpenGLContextCocoa()
        {
            [NSOpenGLContext clearCurrentContext];

            if (ctx)
            {
                [ctx release];
                ctx = nil;
            }
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
            window_context->toggleFullscreen();
        }

        bool isFullscreen() const override
        {
            return window_context->isFullscreen();
        }

        math::int32x2 getWindowSize() const override
        {
            return window_context->getContentSize();
        }
    };

    OpenGLContext* createOpenGLContextCocoa(OpenGLWindow* parent, int width, int height, u32 flags, const OpenGLWindow::Config* configPtr, OpenGLWindow* shared)
    {
        return new OpenGLContextCocoa(parent, width, height, flags, configPtr, shared);
    }

} // namespace mango
