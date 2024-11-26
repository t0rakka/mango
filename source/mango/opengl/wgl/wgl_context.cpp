/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/core/system.hpp>
#include <mango/opengl/opengl.hpp>

// -----------------------------------------------------------------------
// Extensions
// -----------------------------------------------------------------------

// extension function pointers
#define GLEXT_PROC(proc, name) proc name = NULL

#include <mango/opengl/func/wglext.hpp>
#include <mango/opengl/func/glext.hpp>

#undef GLEXT_PROC

// initialize extension function pointers
#define GLEXT_PROC(proc, name) name = reinterpret_cast<proc>(wglGetProcAddress(#name))

static
void init_wgl_extensions()
{
    #include <mango/opengl/func/wglext.hpp>
}

static
void init_glext_extensions()
{
    #include <mango/opengl/func/glext.hpp>
}

#undef GLEXT_PROC

namespace mango
{
    using namespace math;

    // -----------------------------------------------------------------------
    // OpenGLContextWGL
    // -----------------------------------------------------------------------

    struct OpenGLContextWGL : OpenGLContextHandle
    {
        HDC hdc { NULL };
        HGLRC hrc { NULL };
        RECT rect;
        bool fullscreen { false };

        HWND hwnd;

        OpenGLContextWGL(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* theShared)
            : hwnd(*theContext)
        {
            OpenGLContextWGL* shared = reinterpret_cast<OpenGLContextWGL*>(theShared);

            theContext->setWindowSize(width, height);

            hdc = ::GetDC(hwnd);

            // Configure attributes
            OpenGLContext::Config config;
            if (configPtr)
            {
                // Override defaults
                config = *configPtr;
            }

            u32 colorBits = config.red + config.green + config.blue + config.alpha;

            // Configure pixel format
            PIXELFORMATDESCRIPTOR pfd;
            pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = static_cast<BYTE>(colorBits);
            pfd.cRedBits = static_cast<BYTE>(config.red);
            pfd.cRedShift = 0;
            pfd.cGreenBits = static_cast<BYTE>(config.green);
            pfd.cGreenShift = 0;
            pfd.cBlueBits = static_cast<BYTE>(config.blue);
            pfd.cBlueShift = 0;
            pfd.cAlphaBits = static_cast<BYTE>(config.alpha);
            pfd.cAlphaShift = 0;
            pfd.cAccumBits = 0;
            pfd.cAccumRedBits = 0;
            pfd.cAccumGreenBits = 0;
            pfd.cAccumBlueBits = 0;
            pfd.cAccumAlphaBits = 0;
            pfd.cDepthBits = static_cast<BYTE>(config.depth);
            pfd.cStencilBits = static_cast<BYTE>(config.stencil);
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
            hrc = wglCreateContext(hdc);
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
                printLine(Print::Info, "[OpenGLContext] WGL_ARB_create_context : ENABLE");

                if (wglChoosePixelFormatARB && wglExtensions.find("WGL_ARB_pixel_format") != std::string::npos)
                {
                    printLine(Print::Info, "[OpenGLContext] WGL_ARB_pixel_format : ENABLE");

                    std::vector<int> formatAttribs;

                    formatAttribs.push_back(WGL_DRAW_TO_WINDOW_ARB);
                    formatAttribs.push_back(GL_TRUE);

                    formatAttribs.push_back(WGL_SUPPORT_OPENGL_ARB);
                    formatAttribs.push_back(GL_TRUE);

                    formatAttribs.push_back(WGL_DOUBLE_BUFFER_ARB);
                    formatAttribs.push_back(GL_TRUE);

                    if (wglExtensions.find("WGL_ARB_pixel_format_float") != std::string::npos)
                    {
                        if (config.hdr)
                        {
                            // HDR
                            printLine(Print::Info, "[OpenGLContext] WGL_PIXEL_TYPE_ARB : HDR");
                            formatAttribs.push_back(WGL_PIXEL_TYPE_ARB);
                            formatAttribs.push_back(WGL_TYPE_RGBA_FLOAT_ARB);
                        }
                        else
                        {
                            // SDR
                            printLine(Print::Info, "[OpenGLContext] WGL_PIXEL_TYPE_ARB : SDR");
                            formatAttribs.push_back(WGL_PIXEL_TYPE_ARB);
                            formatAttribs.push_back(WGL_TYPE_RGBA_ARB);
                        }
                    }
                    else
                    {
                        // SDR
                        printLine(Print::Info, "[OpenGLContext] WGL_PIXEL_TYPE_ARB : SDR");
                        formatAttribs.push_back(WGL_PIXEL_TYPE_ARB);
                        formatAttribs.push_back(WGL_TYPE_RGBA_ARB);
                    }

                    formatAttribs.push_back(WGL_RED_BITS_ARB);
                    formatAttribs.push_back(config.red);

                    formatAttribs.push_back(WGL_GREEN_BITS_ARB);
                    formatAttribs.push_back(config.green);

                    formatAttribs.push_back(WGL_BLUE_BITS_ARB);
                    formatAttribs.push_back(config.blue);

                    formatAttribs.push_back(WGL_ALPHA_BITS_ARB);
                    formatAttribs.push_back(config.alpha);

                    formatAttribs.push_back(WGL_COLOR_BITS_ARB);
                    formatAttribs.push_back(colorBits);

                    formatAttribs.push_back(WGL_DEPTH_BITS_ARB);
                    formatAttribs.push_back(config.depth);

                    formatAttribs.push_back(WGL_STENCIL_BITS_ARB);
                    formatAttribs.push_back(config.stencil);

                    if (wglExtensions.find("WGL_ARB_multisample") != std::string::npos)
                    {
                        if (config.samples > 1)
                        {
                            printLine(Print::Info, "[OpenGLContext] WGL_ARB_multisample : {}", config.samples);
                            formatAttribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
                            formatAttribs.push_back(GL_TRUE);

                            formatAttribs.push_back(WGL_SAMPLES_ARB);
                            formatAttribs.push_back(config.samples);
                        }
                    }

                    if (wglExtensions.find("WGL_ARB_framebuffer_sRGB") != std::string::npos)
                    {
                        if (config.srgb)
                        {
                            printLine(Print::Info, "[OpenGLContext] WGL_ARB_framebuffer_sRGB : ENABLE");
                            formatAttribs.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT);
                        }
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

                HGLRC old_hrc = hrc;
                HGLRC shared_hrc = shared ? shared->hrc : 0;

                shared = nullptr; // indicate we don't want to call wglShareLists()
                
                hrc = wglCreateContextAttribsARB(hdc, shared_hrc, contextAttribs);
                ::wglMakeCurrent(hdc, hrc);

                // Initialize extension function pointers (for the ARB extended context)
                init_wgl_extensions();

                // delete temporary context
                ::wglDeleteContext(old_hrc);
            }
            else
            {
                printLine(Print::Info, "[OpenGLContext] WGL_ARB_create_context : DISABLE");
            }

            if (shared)
            {
                ::wglShareLists(hrc, shared->hrc);
            }

            init_glext_extensions();
        }

        ~OpenGLContextWGL()
        {
            ::wglMakeCurrent(NULL, NULL);

            if (hrc)
            {
                wglDeleteContext(hrc);
            }

            if (hdc)
            {
                ::ReleaseDC(hwnd, hdc);
            }
        }

        void makeCurrent()
        {
            ::wglMakeCurrent(hdc, hrc);
        }

        void swapBuffers()
        {
            ::SwapBuffers(hdc);
        }

        void swapInterval(int interval)
        {
            if (wglSwapIntervalEXT)
            {
                wglSwapIntervalEXT(interval);
            }
        }

        void toggleFullscreen()
        {
            if (!fullscreen)
            {
                GetWindowRect(hwnd, &rect);

                DEVMODE dm;
                dm.dmSize = sizeof(DEVMODE);
                EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);

                ::SetWindowLongPtr(hwnd, GWL_STYLE, WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE);
                ::MoveWindow(hwnd, 0, 0, dm.dmPelsWidth, dm.dmPelsHeight, TRUE);
            }
            else
            {
                ::SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                ::SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
            }

            fullscreen = !fullscreen;
        }

        bool isFullscreen() const
        {
            return fullscreen;
        }

        int32x2 getWindowSize() const
        {
            RECT rect;
            ::GetClientRect(hwnd, &rect);
            return int32x2(rect.right - rect.left, rect.bottom - rect.top);
        }
    };

    OpenGLContextHandle* createOpenGLContextWGL(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        auto* context = new OpenGLContextWGL(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango
