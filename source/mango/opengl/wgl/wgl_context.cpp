/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>
#include "../../window/win32/win32_handle.hpp"

namespace
{

    using namespace mango;

    // -----------------------------------------------------------------------
	// parseExtensionString()
    // -----------------------------------------------------------------------

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
// Extensions
// -----------------------------------------------------------------------

// extension function pointers
#define GLEXT_PROC(proc, name) proc name = NULL

#include <mango/opengl/func/wglext.hpp>

#ifdef MANGO_OPENGL_CORE_PROFILE
    #include <mango/opengl/func/glcorearb.hpp>
#else
    #include <mango/opengl/func/glext.hpp>
#endif

#undef GLEXT_PROC

// initialize extension function pointers
#define GLEXT_PROC(proc, name) name = reinterpret_cast<proc>(wglGetProcAddress(#name))

static void init_wgl_extensions()
{
    #include <mango/opengl/func/wglext.hpp>
}

#if defined(MANGO_OPENGL_CORE_PROFILE)

static void init_glcorearb_extensions()
{
#include <mango/opengl/func/glcorearb.hpp>
}

#else

static void init_glext_extensions()
{
    #include <mango/opengl/func/glext.hpp>
}

#endif

#undef GLEXT_PROC

namespace mango {
namespace opengl {

    // -----------------------------------------------------------------------
    // ContextHandle
    // -----------------------------------------------------------------------

	struct ContextHandle
	{
		HDC hdc { NULL };
		HGLRC hrc { NULL };
        RECT rect;
		bool fullscreen { false };
    };

    // -----------------------------------------------------------------------
    // Context
    // -----------------------------------------------------------------------

    Context::Context(int width, int height, u32 flags, const ContextAttribute* contextAttribute, Context* shared)
		: Window(width, height, flags)
    {
		m_context = new ContextHandle();

		// TODO
		if (shared)
		{
			MANGO_EXCEPTION("[WGL Context] Shared context is not implemented.");
		}

		setWindowSize(width, height);

		HDC hdc = ::GetDC(m_handle->hwnd);

		// Configure attributes
		ContextAttribute attrib;
		if (contextAttribute)
		{
			// Override defaults
			attrib = *contextAttribute;
		}

		u32 colorBits = attrib.red + attrib.green + attrib.blue + attrib.alpha;

		// Configure pixel format
		PIXELFORMATDESCRIPTOR pfd;
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = 32;
		pfd.cColorBits = (BYTE)colorBits;
		pfd.cRedBits = (BYTE)attrib.red;
		pfd.cRedShift = 0;
		pfd.cGreenBits = (BYTE)attrib.green;
		pfd.cGreenShift = 0;
		pfd.cBlueBits = (BYTE)attrib.blue;
		pfd.cBlueShift = 0;
		pfd.cAlphaBits = (BYTE)attrib.alpha;
		pfd.cAlphaShift = 0;
		pfd.cAccumBits = 0;
		pfd.cAccumRedBits = 0;
		pfd.cAccumGreenBits = 0;
		pfd.cAccumBlueBits = 0;
		pfd.cAccumAlphaBits = 0;
		pfd.cDepthBits = (BYTE)attrib.depth;
		pfd.cStencilBits = (BYTE)attrib.stencil;
		pfd.cAuxBuffers = 0;
		pfd.iLayerType = PFD_MAIN_PLANE;
		pfd.bReserved = 0;
		pfd.dwLayerMask = 0;
		pfd.dwVisibleMask = 0;
		pfd.dwDamageMask = 0;

		// Select pixel format with the configuration
		GLuint format = ::ChoosePixelFormat(hdc, &pfd);
		::SetPixelFormat(hdc, format, &pfd);

		// Create temporary context
		HGLRC hrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hrc);

		// Initialize extension function pointers
		init_wgl_extensions();

		// WGL Extension string
		std::string wglExtensions;

		if (wglGetExtensionsStringARB)
		{
			wglExtensions = wglGetExtensionsStringARB(hdc);
		}

		// Create ARB extended context
		if (wglCreateContextAttribsARB && wglExtensions.find("WGL_ARB_create_context") != std::string::npos)
		{
			if (wglChoosePixelFormatARB && wglExtensions.find("WGL_ARB_pixel_format") != std::string::npos)
			{
				std::vector<int> formatAttribs;

				formatAttribs.push_back(WGL_DRAW_TO_WINDOW_ARB);
				formatAttribs.push_back(GL_TRUE);

				formatAttribs.push_back(WGL_SUPPORT_OPENGL_ARB);
				formatAttribs.push_back(GL_TRUE);

				formatAttribs.push_back(WGL_DOUBLE_BUFFER_ARB);
				formatAttribs.push_back(GL_TRUE);

				formatAttribs.push_back(WGL_PIXEL_TYPE_ARB);
				formatAttribs.push_back(WGL_TYPE_RGBA_ARB);

				formatAttribs.push_back(WGL_COLOR_BITS_ARB);
				formatAttribs.push_back(colorBits);

				formatAttribs.push_back(WGL_DEPTH_BITS_ARB);
				formatAttribs.push_back(attrib.depth);

				formatAttribs.push_back(WGL_STENCIL_BITS_ARB);
				formatAttribs.push_back(attrib.stencil);

				if (wglExtensions.find("WGL_ARB_multisample") != std::string::npos)
				{
					if (attrib.samples > 1)
					{
						formatAttribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
						formatAttribs.push_back(GL_TRUE);

						formatAttribs.push_back(WGL_SAMPLES_ARB);
						formatAttribs.push_back(attrib.samples);
					}
				}

				if (wglExtensions.find("WGL_ARB_pixel_format_float") != std::string::npos)
				{
					// TODO
				}

				if (wglExtensions.find("WGL_ARB_framebuffer_sRGB") != std::string::npos)
				{
					// TODO
					// WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB
				}

				formatAttribs.push_back(0);

				int pixelFormat;
				UINT numFormats;
				wglChoosePixelFormatARB(hdc, formatAttribs.data(), NULL, 1, &pixelFormat, &numFormats);
				::SetPixelFormat(hdc, format, &pfd);
			}
			else
			{
				int pixelFormat = ::ChoosePixelFormat(hdc, &pfd);
				::SetPixelFormat(hdc, format, &pfd);
				MANGO_UNREFERENCED(pixelFormat);
			}

			int contextAttribs[] =
			{
				//WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
				//WGL_CONTEXT_MINOR_VERSION_ARB, 2,
				//WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, // warning: do not use :)
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				0, 0
			};

			m_context->hdc = hdc;
			m_context->hrc = wglCreateContextAttribsARB(hdc, 0, contextAttribs);
			wglMakeCurrent(m_context->hdc, m_context->hrc);

			// Initialize extension function pointers (for the ARB extended context)
			init_wgl_extensions();

			// delete temporary context
			::wglDeleteContext(hrc);
		}
		else
		{
			// "WGL_ARB_create_context" not supported; activate the "temporary" WGL context
			m_context->hdc = hdc;
			m_context->hrc = hrc;
		}

		if (shared)
		{
			// TODO
		    //::wglShareLists(m_context->hrc, shared->m_context->hrc);
		}

#ifdef MANGO_OPENGL_CORE_PROFILE
		init_glcorearb_extensions();
#else
		init_glext_extensions();
#endif

		// parse extension string
		if (glGetString == nullptr)
		{
			MANGO_EXCEPTION("[WGL Context] Context creation failed, no GL functions.");
		}

		const GLubyte* extensions = glGetString(GL_EXTENSIONS);
		if (extensions)
		{
			parseExtensionString(m_extensions, reinterpret_cast<const char*>(extensions));
		}

		// initialize extension mask
		initExtensionMask();

		setVisible(true);
	}

    Context::~Context()
    {
		::wglMakeCurrent(NULL, NULL);

		if (m_context->hrc)
		{
			wglDeleteContext(m_context->hrc);
			m_context->hrc = NULL;
		}

		if (m_context->hdc)
		{
			::ReleaseDC(m_handle->hwnd, m_context->hdc);
			m_context->hdc = NULL;
		}

		delete m_context;
    }

    void Context::makeCurrent()
    {
        ::wglMakeCurrent(m_context->hdc, m_context->hrc);
    }

    void Context::swapBuffers()
    {
        ::SwapBuffers(m_context->hdc);
    }

    void Context::swapInterval(int interval)
    {
        if (wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(interval);
        }
    }

    void Context::toggleFullscreen()
    {
        if (!m_context->fullscreen)
        {
            GetWindowRect(m_handle->hwnd, &m_context->rect); // store

            DEVMODE dm;
            dm.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);

            ::SetWindowLongPtr(m_handle->hwnd, GWL_STYLE, WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE);
            ::MoveWindow(m_handle->hwnd, 0, 0, dm.dmPelsWidth, dm.dmPelsHeight, TRUE);
        }
        else
        {
            ::SetWindowLongPtr(m_handle->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
            ::SetWindowPos(m_handle->hwnd, HWND_TOP, m_context->rect.left, m_context->rect.top, m_context->rect.right - m_context->rect.left, m_context->rect.bottom - m_context->rect.top, 0);
        }

        m_context->fullscreen = !m_context->fullscreen;
    }

    bool Context::isFullscreen() const
	{
		return m_context->fullscreen;
	}

    int2 Context::getWindowSize() const
    {
		return Window::getWindowSize();
    }

} // namespace opengl
} // namespace mango
