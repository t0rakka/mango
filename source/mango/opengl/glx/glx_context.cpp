/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2022 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/system.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>
#include "../../window/xlib/xlib_handle.hpp"

namespace mango
{
    using namespace math;

    // -----------------------------------------------------------------------
    // OpenGLContextGLX
    // -----------------------------------------------------------------------

    static
    int contextErrorHandler(Display* display, XErrorEvent* event)
    {
        MANGO_UNREFERENCED(display);
        MANGO_UNREFERENCED(event);
        return 0;
    }

    struct OpenGLContextGLX : OpenGLContextHandle
    {
        GLXContext context { 0 };
        bool fullscreen { false };

        WindowHandle* window;

        OpenGLContextGLX(OpenGLContext* theContext, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* theShared)
            : window(*theContext)
        {
            if (theShared)
            {
                // TODO
                MANGO_EXCEPTION("[OpenGLContextGLX] Shared context is not implemented.");
            }

            // override defaults
            OpenGLContext::Config config;
            if (configPtr)
            {
                // Override defaults
                config = *configPtr;
            }

            // Configure attributes

            std::vector<int> visualAttribs;

            visualAttribs.push_back(GLX_X_RENDERABLE);
            visualAttribs.push_back(True);

            visualAttribs.push_back(GLX_DRAWABLE_TYPE);
            visualAttribs.push_back(GLX_WINDOW_BIT);

            visualAttribs.push_back(GLX_RENDER_TYPE);
            visualAttribs.push_back(GLX_RGBA_BIT);

            visualAttribs.push_back(GLX_X_VISUAL_TYPE);
            visualAttribs.push_back(GLX_TRUE_COLOR);

            visualAttribs.push_back(GLX_DOUBLEBUFFER);
            visualAttribs.push_back(True);

            visualAttribs.push_back(GLX_RED_SIZE);
            visualAttribs.push_back(config.red);

            visualAttribs.push_back(GLX_GREEN_SIZE);
            visualAttribs.push_back(config.green);

            visualAttribs.push_back(GLX_BLUE_SIZE);
            visualAttribs.push_back(config.blue);

            visualAttribs.push_back(GLX_ALPHA_SIZE);
            visualAttribs.push_back(config.alpha);

            visualAttribs.push_back(GLX_DEPTH_SIZE);
            visualAttribs.push_back(config.depth);

            visualAttribs.push_back(GLX_STENCIL_SIZE);
            visualAttribs.push_back(config.stencil);

            if (config.samples > 1)
            {
                visualAttribs.push_back(GLX_SAMPLE_BUFFERS);
                visualAttribs.push_back(1);

                visualAttribs.push_back(GLX_SAMPLES);
                visualAttribs.push_back(config.samples);
            }

            visualAttribs.push_back(None);

            int glx_major;
            int glx_minor;

            if (!glXQueryVersion(window->x11_display, &glx_major, &glx_minor))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextGLX] glXQueryVersion() failed.");
            }

            debugPrint("GLX version: %d.%d\n", glx_major, glx_minor);

            if ((glx_major == 1 && glx_minor < 3) || glx_major < 1)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextGLX] Invalid GLX version.");
            }

            int fbcount;
            GLXFBConfig* fbc = glXChooseFBConfig(window->x11_display, 
                DefaultScreen(window->x11_display), visualAttribs.data(), &fbcount);
            if (!fbc)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextGLX] glXChooseFBConfig() failed.");
            }

            // Pick the FB config/visual with the samples closest to attrib.samples
            int best_fbc = 0;
            int best_dist = 1024;

            for (int i = 0; i < fbcount; ++i)
            {
                XVisualInfo* vi = glXGetVisualFromFBConfig(window->x11_display, fbc[i]);
                if (vi)
                {
                    int sample_buffers;
                    glXGetFBConfigAttrib(window->x11_display, fbc[i], GLX_SAMPLE_BUFFERS, &sample_buffers);

                    int samples;
                    glXGetFBConfigAttrib(window->x11_display, fbc[i], GLX_SAMPLES, &samples);

#if 0
                    debugPrint("  Matching fbconfig %d, visual ID 0x%2x: SAMPLE_BUFFERS = %d, SAMPLES = %d\n",
                        i, (unsigned int)vi -> visualid, sample_buffers, samples);
#endif

                    if (!sample_buffers)
                    {
                        samples = 1;
                    }

                    int dist = std::abs(int(config.samples) - samples);
                    if (dist < best_dist)
                    {
                        best_dist = dist;
                        best_fbc = i;
                    }
                }

                XFree(vi);
            }

            GLXFBConfig bestFbc = fbc[best_fbc];
            XFree(fbc);

            XVisualInfo* vi = glXGetVisualFromFBConfig(window->x11_display, bestFbc);

            // create window
            if (!window->createWindow(vi->screen, vi->depth, vi->visual, width, height, "OpenGL"))
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextGLX] createWindow() failed.");
            }

            XFree(vi);

            // Get the default screen's GLX extension list
            const char* glxExts = glXQueryExtensionsString(window->x11_display, DefaultScreen(window->x11_display));

            // Create GLX extension set
            std::set<std::string> glxExtensions;
            if (glxExts)
            {
                OpenGLContext::parseExtensionString(glxExtensions, glxExts);
            }

            // NOTE: It is not necessary to create or make current to a context before calling glXGetProcAddressARB
            PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
                (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

            // Detect extension.
            bool isGLX_ARB_create_context = glxExtensions.find("GLX_ARB_create_context") != glxExtensions.end();

            // Install an X error handler so the application won't exit if GL 3.0
            // context allocation fails.
            //
            // Note this error handler is global.  All display connections in all threads
            // of a process use the same error handler, so be sure to guard against other
            // threads issuing X commands while this code is running.
            int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&contextErrorHandler);

            // Check for the GLX_ARB_create_context extension string and the function.
            if (isGLX_ARB_create_context && glXCreateContextAttribsARB)
            {
                int context_attribs[] =
                {
                    //GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                    GLX_CONTEXT_FLAGS_ARB,         GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                    None
                };

                context = glXCreateContextAttribsARB(window->x11_display, bestFbc, 0, True, context_attribs);

                // Sync to ensure any errors generated are processed.
                XSync(window->x11_display, False);

                if (context)
                {
                    //debugPrint("Created GL 3.0 context\n");
                }
                else
                {
                    //debugPrint("Failed to create GL 3.0 context ... using old-style GLX context\n");
                    context = glXCreateContextAttribsARB(window->x11_display, bestFbc, 0, True, NULL);
                }
            }
            else
            {
                //debugPrint("glXCreateContextAttribsARB() not found ... using old-style GLX context\n");
                context = glXCreateNewContext(window->x11_display, bestFbc, GLX_RGBA_TYPE, 0, True);
            }

            // Sync to ensure any errors generated are processed.
            XSync(window->x11_display, False);

            // Restore the original error handler
            XSetErrorHandler(oldHandler);

            if (!context)
            {
                shutdown();
                MANGO_EXCEPTION("[OpenGLContextGLX] OpenGL Context creation failed.");
            }

            // Verifying that context is a direct context
            if (!glXIsDirect(window->x11_display, context))
            {
                debugPrint("Indirect GLX rendering context obtained.\n");
            }
            else
            {
                debugPrint("Direct GLX rendering context obtained.\n");
            }

            // TODO: configuration selection API
            // TODO: context version selection: 4.3, 3.2, etc.
            // TODO: initialize GLX extensions using GLEXT headers
            glXMakeCurrent(window->x11_display, window->x11_window, context);

#if 0
            PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)glXGetProcAddress((const GLubyte*)"glGetStringi");

            if (glGetStringi == NULL)
            {
                // Fall-back to the pre-3.0 method for querying extensions.
                std::string extensionString = (const char*)glGetString(GL_EXTENSIONS);
                m_extensions = tokenizeString(extensionString, " ");
            }
            else
            {
                // Use the post-3.0 method for querying extensions
                GLint numExtensions = 0;
                glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

                for (int i = 0; i < numExtensions; ++i)
                {
                    std::string ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
                    m_extensions.push_back(ext);
                }
            }
#endif
        }

        ~OpenGLContextGLX()
        {
            shutdown();
        }

        void shutdown()
        {
            Display* display = window->x11_display;
            if (display)
            {
                glXMakeCurrent(display, 0, 0);

                if (context)
                {
                    glXDestroyContext(display, context);
                }
            }
        }

        void makeCurrent() override
        {
            glXMakeCurrent(window->x11_display, window->x11_window, context);
        }

        void swapBuffers() override
        {
            glXSwapBuffers(window->x11_display, window->x11_window);
        }

        void swapInterval(int interval) override
        {
            glXSwapIntervalEXT(window->x11_display, window->x11_window, interval);
        }

        void toggleFullscreen() override
        {
            // Disable rendering while switching fullscreen mode
            window->busy = true;
            glXMakeCurrent(window->x11_display, 0, 0);

            XEvent xevent;
            std::memset(&xevent, 0, sizeof(xevent));

            xevent.type = ClientMessage;
            xevent.xclient.window = window->x11_window;
            xevent.xclient.message_type = window->atom_state;
            xevent.xclient.format = 32;
            xevent.xclient.data.l[0] = 2; // NET_WM_STATE_TOGGLE
            xevent.xclient.data.l[1] = window->atom_fullscreen;
            xevent.xclient.data.l[2] = 0; // no second property to toggle
            xevent.xclient.data.l[3] = 1; // source indication: application
            xevent.xclient.data.l[4] = 0; // unused

            XMapWindow(window->x11_display, window->x11_window);

            // send the event to the root window
            if (!XSendEvent(window->x11_display, DefaultRootWindow(window->x11_display),
                False, SubstructureRedirectMask | SubstructureNotifyMask, &xevent))
            {
                // TODO: failed
            }

            XFlush(window->x11_display);

            // Enable rendering now that all the tricks are done
            window->busy = false;
            glXMakeCurrent(window->x11_display, window->x11_window, context);

            fullscreen = !fullscreen;
        }

        bool isFullscreen() const override
        {
            return fullscreen;
        }

        int32x2 getWindowSize() const override
        {
            XWindowAttributes attributes;
            XGetWindowAttributes(window->x11_display, window->x11_window, &attributes);
            return int32x2(attributes.width, attributes.height);
        }
    };

    OpenGLContextHandle* createOpenGLContextGLX(OpenGLContext* parent, int width, int height, u32 flags, const OpenGLContext::Config* configPtr, OpenGLContext* shared)
    {
        auto* context = new OpenGLContextGLX(parent, width, height, flags, configPtr, shared);
        return context;
    }

} // namespace mango
