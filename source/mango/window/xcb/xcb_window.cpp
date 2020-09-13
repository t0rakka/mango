/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <unistd.h>
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include "xcb_handle.hpp"

namespace
{
    using namespace mango;

} // namespace

namespace mango
{

    // -----------------------------------------------------------------------
    // WindowHandle
    // -----------------------------------------------------------------------

    WindowHandle::WindowHandle(int width, int height)
    {
        const char* title = "XCB Window"; // TODO

        int screen_index;
        connection = xcb_connect(nullptr, &screen_index);
        if (!connection)
        {
            MANGO_EXCEPTION("[Window] xcb_connect() failed.");
        }

        const xcb_setup_t* setup = xcb_get_setup(connection);
        xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

        while (screen_index-- > 0)
        {
            xcb_screen_next(&screen_iterator);
        }

        xcb_screen_t* screen = screen_iterator.data;
        window = xcb_generate_id(connection);

        uint32_t value_list[] =
        {
            screen->white_pixel,
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        };

        xcb_create_window(
            connection,
            XCB_COPY_FROM_PARENT,
            window,
            screen->root,
            20, 20,
            width, height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
            value_list);

        xcb_change_property(
            connection,
            XCB_PROP_MODE_REPLACE,
            window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            std::strlen(title),
            title);

        xcb_map_window(connection, window);
        xcb_flush(connection);
    }

    WindowHandle::~WindowHandle()
    {
        xcb_destroy_window(connection, window);
        xcb_disconnect(connection);
    }

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    Window::Window(int width, int height, u32 flags)
    {
		m_handle = new WindowHandle(width, height);
        MANGO_UNREFERENCED(flags); // TODO
    }

    Window::~Window()
    {
		delete m_handle;
    }

    void Window::setWindowPosition(int x, int y)
    {
        // TODO
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
    }

    void Window::setWindowSize(int width, int height)
    {
        const uint32_t values[] = { uint32_t(width), uint32_t(height) };

        xcb_configure_window(
            m_handle->connection,
            m_handle->window,
            XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
            values);
        xcb_flush(m_handle->connection);
    }

    void Window::setTitle(const std::string& title)
    {
        xcb_change_property(
            m_handle->connection,
            XCB_PROP_MODE_REPLACE,
            m_handle->window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            title.length(),
            title.c_str());
        xcb_flush(m_handle->connection);
    }

    void Window::setIcon(const Surface& surface)
    {
#if 0
        // clamp icon size to 256 x 256
        const int width = std::min(256, surface.width);
        const int height = std::min(256, surface.height);

        Bitmap bitmap(width, height, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));
        bitmap.blit(0, 0, surface);

        int screen = DefaultScreen(m_handle->display);
        int depth = DefaultDepth(m_handle->display, screen);
        Visual* visual = DefaultVisual(m_handle->display, screen);

        XImage* icon = XCreateImage(m_handle->display, visual, depth, ZPixmap, 0,
            reinterpret_cast<char*>(bitmap.image), width, height, 32, 0);
        if (!icon)
        {
            MANGO_EXCEPTION("[Window] XCreateImage() failed.");
        }

        Pixmap icon_pixmap = XCreatePixmap(m_handle->display, RootWindow(m_handle->display, screen), width, height, depth);
        if (!icon_pixmap)
        {
            MANGO_EXCEPTION("[Window] XCreatePixmap() failed.");
        }

        XGCValues values;
        GC gc = XCreateGC(m_handle->display, icon_pixmap, 0, &values);
        if (!gc)
        {
            MANGO_EXCEPTION("[Window] XCreateGC() failed.");
        }

        XPutImage(m_handle->display, icon_pixmap, gc, icon, 0, 0, 0, 0, width, height);
        XFreeGC(m_handle->display, gc);

        // convert alpha channel to mask
        int stride = (width + 7) / 8; // round to next multiple of 8
        Bitmap alphaMask(stride, height, Format(8, Format::UNORM, Format::A, 8));

        for (int y = 0; y < height; ++y)
        {
            u8* alpha = bitmap.image + y * bitmap.stride + 3;
            u8* dest = alphaMask.image + y * stride;

            for (int x = 0; x < width; ++x)
            {
                int s = alpha[x * 4] > 0 ? 1 : 0;
                dest[x / 8] |= (s << (x & 7));
            }
        }

        Pixmap mask_pixmap = XCreatePixmapFromBitmapData(m_handle->display, m_handle->window,
            reinterpret_cast<char*>(alphaMask.image), width, height, 1, 0, 1);

        XWMHints* hints = XAllocWMHints();
        hints->flags       = IconPixmapHint | IconMaskHint;
        hints->icon_pixmap = icon_pixmap;
        hints->icon_mask   = mask_pixmap;
        XSetWMHints(m_handle->display, m_handle->window, hints);
        XFree(hints);

        XFlush(m_handle->display);

        if (m_handle->icon)
        {
            XDestroyImage(m_handle->icon);
            m_handle->icon = icon;
        }
#endif
    }

    void Window::setVisible(bool enable)
    {
#if 0
        if (enable)
        {
            XMapRaised(m_handle->display, m_handle->window);
            XFlush(m_handle->display);
        }
        else
        {
            XUnmapWindow(m_handle->display, m_handle->window);
            XFlush(m_handle->display);
        }
#endif
    }

    int32x2 Window::getWindowSize() const
    {
        // TODO
#if 0
        XWindowAttributes attributes;
        XGetWindowAttributes(m_handle->display, m_handle->window, &attributes);
        return int2(attributes.width, attributes.height);
#endif
        return int2(0, 0);
    }

	int32x2 Window::getCursorPosition() const
	{
        // TODO
#if 0
		::Window root;
		::Window child;
		int root_x, root_y;
		int child_x, child_y;
		unsigned int mask;

		XQueryPointer(m_handle->display, m_handle->window, &root, &child,
                      &root_x, &root_y, &child_x, &child_y, &mask);
		return int2(child_x, child_y);
#endif
        return int2(0, 0);
	}

    bool Window::isKeyPressed(Keycode code) const
    {
        // TODO
#if 0
        char keys[32];
        XQueryKeymap(m_handle->display, keys);

        bool pressed = false;

        KeySym symbol = translateKeycodeToSymbol(code);
        int keyidx = XKeysymToKeycode(m_handle->display, symbol);

        if (keyidx >=0 && keyidx < 255)
            pressed = (keys[keyidx / 8] & (1 << (keyidx % 8))) != 0;

        return pressed;
#endif
        return false;
    }

    void Window::enterEventLoop()
    {

        // https://www.x.org/releases/X11R7.6/doc/libxcb/tutorial/index.html
#if 0
        xcb_intern_atom_cookie_t  protocols_cookie = xcb_intern_atom( Parameters.Connection, 1, 12, "WM_PROTOCOLS" );
        xcb_intern_atom_reply_t  *protocols_reply  = xcb_intern_atom_reply( Parameters.Connection, protocols_cookie, 0 );
        xcb_intern_atom_cookie_t  delete_cookie    = xcb_intern_atom( Parameters.Connection, 0, 16, "WM_DELETE_WINDOW" );
        xcb_intern_atom_reply_t  *delete_reply     = xcb_intern_atom_reply( Parameters.Connection, delete_cookie, 0 );
        xcb_change_property( Parameters.Connection, XCB_PROP_MODE_REPLACE, Parameters.Handle, (*protocols_reply).atom, 4, 32, 1, &(*delete_reply).atom );
        free(protocols_reply);
#endif


        m_handle->looping = true;

        for ( ; m_handle->looping; )
        {
            xcb_generic_event_t* event = xcb_poll_for_event(m_handle->connection);
            if (event)
            {
                switch (event->response_type & 0x7f)
                {
                    // resize
                    case XCB_CONFIGURE_NOTIFY:
                    {
#if 0
                        xcb_configure_notify_event_t* configure_notify = (xcb_configure_notify_event_t*)event;
                        static uint16_t width = configure_notify->width;
                        static uint16_t height = configure_notify->height;

                        if ( ((configure_notify->width > 0) && (width != configure_notify->width)) ||
                             ((configure_notify->height > 0) && (height != configure_notify->height)) )
                        {
                            resize = true;
                            width = configure_notify->width;
                            height = configure_notify->height;
                        }
#endif
                        break;
                    }

                    // close
                    case XCB_CLIENT_MESSAGE:
                    {
#if 0
                        if ( (*(xcb_client_message_event_t*)event).data.data32[0] == (*delete_reply).atom )
                        {
                            m_handle->looping = false;
                            free(delete_reply);
                        }
#endif
                        break;
                    }

                    case XCB_KEY_PRESS:
                    {
                        // TODO
                        m_handle->looping = false;
                        break;
                    }

                    case XCB_BUTTON_PRESS:
                    {
                        xcb_button_press_event_t* button_press = (xcb_button_press_event_t *)event;
                        (void) button_press; // TODO
                        break;
                    }
                }

                free(event);
            }
            else
            {
                onIdle();
                usleep(125);
            }
        }


#if 0
        static Timer timer;

        for (int i = 0; i < 6; ++i)
        {
            m_handle->mouse_time[i] = 0;
        }

                switch (e.type)
                {
                    case ButtonPress:
                    {
                        MouseButton button = translateButton(e.xbutton.button);
                        int count = 1;

                        switch (e.xbutton.button)
                        {
                            case 0:
                            case 1:
                            case 2:
                            case 3:
                            {
                                // Simulate double click
                                float time = timer.time();
                                if (time - m_handle->mouse_time[button] < 300.0f/1000.0f)
                                {
                                    count = 2;
                                }
                                m_handle->mouse_time[button] = time;
                                break;
                            }

                            case 4:
                                count = 120;
                                break;

                            case 5:
                                count = -120;
                                break;
                        }

                        onMouseClick(e.xbutton.x, e.xbutton.y, button, count);
                        break;
                    }

                    case ButtonRelease:
                    {
                        MouseButton button = translateButton(e.xbutton.button);
                        onMouseClick(e.xbutton.x, e.xbutton.y, button, 0);
                        break;
                    }

                    case MotionNotify:
                        onMouseMove(e.xmotion.x, e.xmotion.y);
                        break;

                    case KeyPress:
                    {
                        u32 mask = translateKeyMask(e.xkey.state);
                        onKeyPress(translateEventToKeycode(&e), mask);
                        break;
                    }

                    case KeyRelease:
                    {
                        bool is_repeat = false;

                        if (XEventsQueued(m_handle->display, QueuedAfterReading))
                        {
                            XEvent next;
                            XPeekEvent(m_handle->display, &next);

                            if (next.type == KeyPress &&
                                next.xkey.time == e.xkey.time &&
                                next.xkey.keycode == e.xkey.keycode)
                            {
                                // delete repeated KeyPress event
                                XNextEvent(m_handle->display, &e);
                                is_repeat = true;
                            }
                        }

                        if (!is_repeat)
                            onKeyRelease(translateEventToKeycode(&e));
                        break;
                    }

                    case ConfigureNotify:
                    {
                        // Filter identical notifications
                        int width = e.xconfigure.width;
                        int height = e.xconfigure.height;
                        if (width != m_handle->size[0] || height != m_handle->size[1])
                        {
                            m_handle->size[0] = width;
                            m_handle->size[1] = height;
                            if (!m_handle->busy) {
                                onResize(width, height);
                            }
                        }
                        break;
                    }

                    case Expose:
                        if (!m_handle->busy) {
                            onDraw();
                        }
                        break;

                    case DestroyNotify:
                        break;

                    case ClientMessage:
                    {
                        unsigned int type = e.xclient.message_type;

                        if (type == m_handle->atom_xdnd_Enter)
                        {
                            bool use_list = e.xclient.data.l[1] & 1;
                            m_handle->xdnd_source = e.xclient.data.l[0];
                            m_handle->xdnd_version = (e.xclient.data.l[1] >> 24);
                            if (use_list) {
                                // fetch conversion targets
                                x11Prop p;
                                ReadProperty(&p, m_handle->display, m_handle->xdnd_source, m_handle->atom_xdnd_TypeList);
                                // pick one
                                m_handle->atom_xdnd_req = PickTarget(m_handle->display, (Atom*)p.data, p.count);
                                XFree(p.data);
                            } else {
                                // pick from list of three
                                m_handle->atom_xdnd_req = PickTargetFromAtoms(m_handle->display, 
                                    e.xclient.data.l[2], e.xclient.data.l[3], e.xclient.data.l[4]);
                            }
                        }
                        else if (type == m_handle->atom_xdnd_Position)
                        {
                            XClientMessageEvent m = { 0 };

                            m.type = ClientMessage;
                            m.display = e.xclient.display;
                            m.window = e.xclient.data.l[0];
                            m.message_type = m_handle->atom_xdnd_Status;
                            m.format=32;
                            m.data.l[0] = m_handle->window;
                            m.data.l[1] = (m_handle->atom_xdnd_req != None);
                            m.data.l[2] = 0; // specify an empty rectangle
                            m.data.l[3] = 0;
                            m.data.l[4] = m_handle->atom_xdnd_ActionCopy; // we only accept copying anyway

                            XSendEvent(m_handle->display, e.xclient.data.l[0], False, NoEventMask, (XEvent*)&m);
                            XFlush(m_handle->display);
                        }
                        else if (type == m_handle->atom_xdnd_Status)
                        {
                        }
                        else if (type == m_handle->atom_xdnd_TypeList)
                        {
                        }
                        else if (type == m_handle->atom_xdnd_ActionCopy)
                        {
                        }
                        else if (type == m_handle->atom_xdnd_Drop)
                        {
                            if (m_handle->atom_xdnd_req == None)
                            {
                                // respond to empty request
                                XClientMessageEvent m = { 0 };

                                m.type = ClientMessage;
                                m.display = e.xclient.display;
                                m.window = e.xclient.data.l[0];
                                m.message_type = m_handle->atom_xdnd_Finished;
                                m.format = 32;
                                m.data.l[0] = m_handle->window;
                                m.data.l[1] = 0;
                                m.data.l[2] = None; // failed
                                XSendEvent(m_handle->display, e.xclient.data.l[0],
                                    False, NoEventMask, (XEvent*)&m);
                            }
                            else
                            {
                                // convert
                                if (m_handle->xdnd_version >= 1) {
                                    XConvertSelection(m_handle->display, m_handle->atom_xdnd_Selection, m_handle->atom_xdnd_req, 
                                                      m_handle->atom_primary, m_handle->window, e.xclient.data.l[2]);
                                } else {
                                    XConvertSelection(m_handle->display, m_handle->atom_xdnd_Selection, m_handle->atom_xdnd_req, 
                                                      m_handle->atom_primary, m_handle->window, CurrentTime);
                                }
                            }
                        }
                        else if (type == m_handle->atom_xdnd_Finished)
                        {
                        }
                        else if (type == m_handle->atom_xdnd_Selection)
                        {
                        }
                        else if (type == m_handle->atom_xdnd_Leave)
                        {
                        }
                        else if (type == m_handle->atom_protocols)
                        {
                            if ((Atom)e.xclient.data.l[0] == m_handle->atom_delete)
                            {
                                breakEventLoop();
                                // NOTE: We should destroy the window here since it doesn't exist anymore.
                            }
                        }

                        break;
                    }

                    case SelectionNotify:
                    {
                        Atom target = e.xselection.target;
                        if (target == m_handle->atom_xdnd_req)
                        {
                            // read data
                            x11Prop p;
                            ReadProperty(&p, m_handle->display, m_handle->window, m_handle->atom_primary);

                            if (p.format == 8) {
                                dispatchOnDrop(this, p);
                            }

                            XFree(p.data);

                            // send reply
                            XClientMessageEvent m = { 0 };

                            m.type = ClientMessage;
                            m.display = m_handle->display;
                            m.window = m_handle->xdnd_source;
                            m.message_type = m_handle->atom_xdnd_Finished;
                            m.format = 32;
                            m.data.l[0] = m_handle->window;
                            m.data.l[1] = 1;
                            m.data.l[2] = m_handle->atom_xdnd_ActionCopy;
                            XSendEvent(m_handle->display, m_handle->xdnd_source, False, NoEventMask, (XEvent*)&m);
                            XSync(m_handle->display, False);
                        }

                        break;
                    }
#endif
    }

    void Window::breakEventLoop()
    {
        m_handle->looping = false;
    }

    void Window::onIdle()
    {
    }

    void Window::onDraw()
    {
    }

    void Window::onResize(int width, int height)
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);
    }

    void Window::onMinimize()
    {
    }

    void Window::onMaximize()
    {
    }

    void Window::onKeyPress(Keycode code, u32 mask)
    {
        MANGO_UNREFERENCED(code);
        MANGO_UNREFERENCED(mask);
    }

    void Window::onKeyRelease(Keycode code)
    {
        MANGO_UNREFERENCED(code);
    }

    void Window::onMouseMove(int x, int y)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
    }

    void Window::onMouseClick(int x, int y, MouseButton button, int count)
    {
        MANGO_UNREFERENCED(x);
        MANGO_UNREFERENCED(y);
        MANGO_UNREFERENCED(button);
        MANGO_UNREFERENCED(count);
    }

    void Window::onDropFiles(const filesystem::FileIndex& index)
    {
        MANGO_UNREFERENCED(index);
    }

    void Window::onClose()
    {
    }

    void Window::onShow()
    {
    }

    void Window::onHide()
    {
    }

} // namespace mango
