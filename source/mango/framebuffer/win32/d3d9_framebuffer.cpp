/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/framebuffer/framebuffer.hpp>
#include "../../window/win32/win32_handle.hpp"

#include <d3d9.h>

namespace mango {
namespace framebuffer {

	// -------------------------------------------------------------------
	// FramebufferContext
	// -------------------------------------------------------------------

    struct FramebufferContext
    {
        IDirect3D9* d3d;
        IDirect3DDevice9* device;
        IDirect3DSurface9* buffer;
        int width;
        int height;

        FramebufferContext(HWND hwnd, int width, int height)
            : d3d(nullptr)
            , device(nullptr)
            , buffer(nullptr)
            , width(width)
            , height(height)
        {
            D3DPRESENT_PARAMETERS pp;
            ::ZeroMemory(&pp, sizeof(D3DPRESENT_PARAMETERS));

            pp.BackBufferWidth = width;
            pp.BackBufferHeight = height;
            pp.BackBufferFormat = D3DFMT_A8R8G8B8;
            pp.hDeviceWindow = hwnd;
            pp.Windowed = TRUE;
            pp.BackBufferCount = 1;
            pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            pp.MultiSampleType = D3DMULTISAMPLE_NONE;
            pp.EnableAutoDepthStencil = TRUE;
            pp.AutoDepthStencilFormat = D3DFMT_D24S8;
            pp.FullScreen_RefreshRateInHz = 0;
            pp.PresentationInterval = 0;
            pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

            d3d = ::Direct3DCreate9(D3D9b_SDK_VERSION);
            d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device);
        }

        ~FramebufferContext()
        {
            unlock();
            if (device) device->Release();
            if (d3d) d3d->Release();
        }

        Surface lock()
        {
            device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &buffer);

            D3DLOCKED_RECT locked;
            buffer->LockRect(&locked, NULL, 0);

            Format format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8);
            return Surface(width, height, format, locked.Pitch, reinterpret_cast<uint8 *>(locked.pBits));
        }

        void unlock()
        {
            if (buffer)
            {
                buffer->UnlockRect();
                buffer->Release();
                buffer = nullptr;
            }
        }

        bool locked() const
        {
            return buffer != nullptr;
        }

        void present()
        {
            device->Present(NULL, NULL, NULL, NULL);
        }
    };

	// -------------------------------------------------------------------
	// Framebuffer
	// -------------------------------------------------------------------

    Framebuffer::Framebuffer(int width, int height)
        : Window(width, height, Window::DISABLE_RESIZE)
    {
        HWND hwnd = *this;
        m_context = new FramebufferContext(hwnd, width, height);
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

    bool Framebuffer::locked() const
    {
        return m_context->locked();
    }

    void Framebuffer::present()
    {
        m_context->present();
    }

} // namespace framebuffer
} // namespace mango
