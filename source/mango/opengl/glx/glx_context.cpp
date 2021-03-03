/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/opengl/opengl.hpp>
#include "../../window/xlib/xlib_handle.hpp"

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
                    container.insert(std::string(ext, length));
                }
                ext = s + 1;
            }
        }
    }

    int contextErrorHandler(Display* display, XErrorEvent* event)
    {
        (void) display;
        (void) event;
        return 0;
    }

} // namespace

namespace mango
{
    using namespace math;

    // -----------------------------------------------------------------------
    // OpenGLContextHandle
    // -----------------------------------------------------------------------

    struct OpenGLContextHandle
    {
		GLXContext context { 0 };
        bool fullscreen { false };
    };

    static void deleteContext(WindowHandle* window_handle, OpenGLContextHandle* context_handle)
    {
        if (window_handle->display)
        {
            glXMakeCurrent(window_handle->display, 0, 0);

            if (context_handle->context)
            {
                glXDestroyContext(window_handle->display, context_handle->context);
                context_handle->context = 0;
            }
        }

        delete context_handle;
    }

    // -----------------------------------------------------------------------
    // OpenGLContext
    // -----------------------------------------------------------------------

    OpenGLContext::OpenGLContext(int width, int height, u32 flags, const Config* configPtr, OpenGLContext* shared)
	    : Window(width, height, flags)
    {
        m_context = new OpenGLContextHandle();

        // TODO
        if (shared)
        {
            MANGO_EXCEPTION("[GLX OpenGLContext] Shared context is not implemented yet.");
        }

        // override defaults
        Config config;
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

        if (!glXQueryVersion(m_handle->display, &glx_major, &glx_minor))
        {
            deleteContext(m_handle, m_context);
            m_context = NULL;
            MANGO_EXCEPTION("[GLX Context] glXQueryVersion() failed.");
        }

        printf("GLX version: %d.%d\n", glx_major, glx_minor);

        if ((glx_major == 1 && glx_minor < 3) || glx_major < 1)
        {
            deleteContext(m_handle, m_context);
            m_context = NULL;
            MANGO_EXCEPTION("[GLX Context] Invalid GLX version.");
        }

        int fbcount;
        GLXFBConfig* fbc = glXChooseFBConfig(m_handle->display, DefaultScreen(m_handle->display), visualAttribs.data(), &fbcount);
        if (!fbc)
        {
            deleteContext(m_handle, m_context);
            m_context = NULL;
            MANGO_EXCEPTION("[GLX Context] glXChooseFBConfig() failed.");
        }

        //printf("Found %d matching FB configs.\n", fbcount);

        // Pick the FB config/visual with the samples closest to attrib.samples
        int best_fbc = 0;
        int best_dist = 1024;

        for (int i = 0; i < fbcount; ++i)
        {
            XVisualInfo* vi = glXGetVisualFromFBConfig(m_handle->display, fbc[i]);
            if (vi)
            {
                int sample_buffers;
                glXGetFBConfigAttrib(m_handle->display, fbc[i], GLX_SAMPLE_BUFFERS, &sample_buffers);

                int samples;
                glXGetFBConfigAttrib(m_handle->display, fbc[i], GLX_SAMPLES, &samples);

#if 0
                printf("  Matching fbconfig %d, visual ID 0x%2x: SAMPLE_BUFFERS = %d, SAMPLES = %d\n",
                    i, (unsigned int)vi -> visualid, sample_buffers, samples);
#endif

                if (!sample_buffers)
                {
                    samples = 1;
                }

                int dist = config.samples - samples;
                if (dist < 0)
                {
                    dist = -dist;
                }

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

        XVisualInfo* vi = glXGetVisualFromFBConfig(m_handle->display, bestFbc);

        // create window
        if (!m_handle->createWindow(vi->screen, vi->depth, vi->visual, width, height, "OpenGL"))
        {
            deleteContext(m_handle, m_context);
            m_context = NULL;
            MANGO_EXCEPTION("[GLX Context] createWindow() failed.");
        }

        XFree(vi);

        // Get the default screen's GLX extension list
        const char* glxExts = glXQueryExtensionsString(m_handle->display, DefaultScreen(m_handle->display));

        // Create GLX extension set
        std::set<std::string> glxExtensions;
        if (glxExts)
        {
            parseExtensionString(glxExtensions, glxExts);
        }

        // NOTE: It is not necessary to create or make current to a context before calling glXGetProcAddressARB
        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
            (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

        // Detect extension.
        bool isGLX_ARB_create_context = glxExtensions.find("GLX_ARB_create_context") != glxExtensions.end();

        m_context->context = 0;

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

            m_context->context = glXCreateContextAttribsARB(m_handle->display, bestFbc, 0, True, context_attribs);

            // Sync to ensure any errors generated are processed.
            XSync(m_handle->display, False);

            if (m_context->context)
            {
                //printf("Created GL 3.0 context\n");
            }
            else
            {
                //printf("Failed to create GL 3.0 context ... using old-style GLX context\n");
                m_context->context = glXCreateContextAttribsARB(m_handle->display, bestFbc, 0, True, NULL);
            }
        }
        else
        {
            //printf("glXCreateContextAttribsARB() not found ... using old-style GLX context\n");
            m_context->context = glXCreateNewContext(m_handle->display, bestFbc, GLX_RGBA_TYPE, 0, True);
        }

        // Sync to ensure any errors generated are processed.
        XSync(m_handle->display, False);

        // Restore the original error handler
        XSetErrorHandler(oldHandler);

        if (!m_context->context)
        {
            deleteContext(m_handle, m_context);
            m_context = nullptr;
            MANGO_EXCEPTION("[GLX Context] OpenGL Context creation failed.");
        }

        // Verifying that context is a direct context
        if (!glXIsDirect(m_handle->display, m_context->context))
        {
            printf("Indirect GLX rendering context obtained.\n");
        }
        else
        {
            printf("Direct GLX rendering context obtained.\n");
        }

        // TODO: configuration selection API
        // TODO: context version selection: 4.3, 3.2, etc.
        // TODO: initialize GLX extensions using GLEXT headers
        glXMakeCurrent(m_handle->display, m_handle->window, m_context->context);

        const GLubyte* s0 = glGetString(GL_VENDOR);
        const GLubyte* s1 = glGetString(GL_RENDERER);
        const GLubyte* s2 = glGetString(GL_VERSION);

        printf("Vendor:   \"%s\"\n", reinterpret_cast<const char *>(s0));
        printf("Renderer: \"%s\"\n", reinterpret_cast<const char *>(s1));
        printf("Version:  \"%s\"\n", reinterpret_cast<const char *>(s2));

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

        // parse extension string
        const GLubyte* extensions = glGetString(GL_EXTENSIONS);
        if (extensions)
        {
            parseExtensionString(m_extensions, reinterpret_cast<const char*>(extensions));
        }

        // initialize extension mask
        initExtensionMask();
    }

    OpenGLContext::~OpenGLContext()
    {
        deleteContext(m_handle, m_context);
    }

    void OpenGLContext::makeCurrent()
    {
        glXMakeCurrent(m_handle->display, m_handle->window, m_context->context);
    }

    void OpenGLContext::swapBuffers()
    {
        glXSwapBuffers(m_handle->display, m_handle->window);
    }

    void OpenGLContext::swapInterval(int interval)
    {
        glXSwapIntervalEXT(m_handle->display, m_handle->window, interval);

    }

    void OpenGLContext::toggleFullscreen()
    {
        // Disable rendering while switching fullscreen mode
        m_handle->busy = true;
        glXMakeCurrent(m_handle->display, 0, 0);

        XEvent xevent;
        std::memset(&xevent, 0, sizeof(xevent));

        xevent.type = ClientMessage;
        xevent.xclient.window = m_handle->window;
        xevent.xclient.message_type = m_handle->atom_state;
        xevent.xclient.format = 32;
        xevent.xclient.data.l[0] = 2; // NET_WM_STATE_TOGGLE
        xevent.xclient.data.l[1] = m_handle->atom_fullscreen;
        xevent.xclient.data.l[2] = 0; // no second property to toggle
        xevent.xclient.data.l[3] = 1; // source indication: application
        xevent.xclient.data.l[4] = 0; // unused

        XMapWindow(m_handle->display, m_handle->window);

        // send the event to the root window
        if (!XSendEvent(m_handle->display, DefaultRootWindow(m_handle->display), False,
            SubstructureRedirectMask | SubstructureNotifyMask, &xevent))
        {
            // TODO: failed
        }

        XFlush(m_handle->display);

        // Enable rendering now that all the tricks are done
        m_handle->busy = false;
        glXMakeCurrent(m_handle->display, m_handle->window, m_context->context);

        m_context->fullscreen = !m_context->fullscreen;
    }

    bool OpenGLContext::isFullscreen() const
	{
		return m_context->fullscreen;
	}

    int32x2 OpenGLContext::getWindowSize() const
    {
		return Window::getWindowSize();
    }

} // namespace mango
