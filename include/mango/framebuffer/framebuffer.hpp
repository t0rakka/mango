/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../window/window.hpp"

namespace mango {
namespace framebuffer {

    // -------------------------------------------------------------------
    // Framebuffer
    // -------------------------------------------------------------------

    class Framebuffer : public Window
    {
    protected:
        struct FramebufferHandle* m_handle;

    public:
        Framebuffer(int width, int height);
        ~Framebuffer();

        Surface lock();
        void unlock();
        bool locked() const;

        void present();
    };

} // namespace framebuffer
} // namespace mango
