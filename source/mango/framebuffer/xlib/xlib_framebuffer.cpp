/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/framebuffer/framebuffer.hpp>
#include <mango/core/exception.hpp>
#include "../../window/xlib/xlib_handle.hpp"

namespace mango::framebuffer
{
    using namespace image;

    struct FramebufferContext
    {
        const WindowHandle& handle;
        int width;
        int height;
        Bitmap buffer;

        GC gc;
        XImage* image;

        FramebufferContext(int screen, int depth, int width, int height, const WindowHandle& handle)
            : handle(handle)
            , width(width)
            , height(height)
            , buffer(width, height, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8))
        {
            gc = DefaultGC(handle.display, screen);
            image = XCreateImage(handle.display, CopyFromParent, depth, ZPixmap, 0, NULL, width, height, 32, width * 4);
            if (!image)
            {
                MANGO_EXCEPTION("[Framebuffer] XCreateImage failed.");
            }
        }

        ~FramebufferContext()
        {
            image->data = NULL;
            XDestroyImage(image);
        }

        Surface lock()
        {
            return buffer;
        }

        void unlock()
        {
        }

        void present()
        {
            image->data = buffer.address<char>();
            XPutImage(handle.display, handle.window, gc, image, 0, 0, 0, 0, width, height);
            XFlush(handle.display);
        }
    };

	// -------------------------------------------------------------------
	// Framebuffer
	// -------------------------------------------------------------------

    Framebuffer::Framebuffer(int width, int height)
        : Window(width, height, Window::DISABLE_RESIZE)
    {
        ::Display* display = m_handle->display;
        int screen = DefaultScreen(display);
	    Visual* visual = DefaultVisual(display, screen);
        int depth = DefaultDepth(display, screen);

        if (!m_handle->createWindow(screen, depth, visual, width, height, "[FramebufferWindow]"))
        {
            MANGO_EXCEPTION("[Framebuffer] Window creation failed.");
        }

        m_context = new FramebufferContext(screen, depth, width, height, *m_handle);
    }

    Framebuffer::~Framebuffer()
    {
        delete m_context;
    }

    Surface Framebuffer::lock()
    {
        return m_context->lock();
    }

    void Framebuffer::unlock()
    {
        m_context->unlock();
    }

    void Framebuffer::present()
    {
        m_context->present();
    }

} // namespace mango::framebuffer
