/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <mango/window//window.hpp>

namespace mango::framebuffer
{

    // -------------------------------------------------------------------
    // Framebuffer
    // -------------------------------------------------------------------

    class Framebuffer : public Window
    {
    protected:
        struct FramebufferContext* m_context;

    public:
        Framebuffer(int width, int height);
        ~Framebuffer();

        Surface lock();
        void unlock();
        void present();
    };

} // namespace mango::framebuffer
