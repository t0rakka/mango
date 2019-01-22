/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/framebuffer/framebuffer.hpp>
#include "../../gui/xlib/xlib_handle.hpp"

namespace mango {
namespace framebuffer {

	// -------------------------------------------------------------------
	// Framebuffer
	// -------------------------------------------------------------------

    Framebuffer::Framebuffer(int width, int height)
        : Window(width, height)
    {
        // TODO
        (void) width;
        (void) height;
    }

    Framebuffer::~Framebuffer()
    {
        // TODO
    }

    Surface Framebuffer::lock()
    {
        // TODO
        return Surface(0, 0, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8), 0, nullptr);
    }

    void Framebuffer::unlock()
    {
        // TODO
    }

    bool Framebuffer::locked() const
    {
        // TODO
        return false;
    }

    void Framebuffer::present()
    {
        // TODO
    }

} // namespace framebuffer
} // namespace mango
