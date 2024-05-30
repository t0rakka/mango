/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <unistd.h>
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>

#if defined(MANGO_WINDOW_SYSTEM_XCB)

namespace
{
    using namespace mango;

    struct ScreenInfo
    {
        xcb_connection_t* screen;
        math::int32x2 resolution;
    };

    std::vector<ScreenInfo> getScreenInfo(xcb_connection_t* connection)
    {
        std::vector<ScreenInfo> screens;

        xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
        while (screen_iter.rem)
        {
            xcb_screen_t* screen = screen_iter.data;

            int width = screen_iter.data->width_in_pixels);
            int height = screen_iter.data->height_in_pixels);
            printf("screen %d: %d x %d\n", screen_iter.index, width, height);

            ScreenInfo info;

            info.screen = screen;
            info.resolution = math::int32x2(width, height);

            screens.push_back(info);

            xcb_screen_next(&screen_iter);
        }

        return screens;
    }

} // namespace

namespace mango
{

    // -----------------------------------------------------------------------
    // WindowHandle
    // -----------------------------------------------------------------------

    struct WindowHandle
    {
        NativeWindowHandle native;

        bool is_looping { false };

        WindowHandle(int width, int height);
        ~WindowHandle();
    };

    WindowHandle::WindowHandle(int width, int height)
    {
        const char* title = "XCB Window"; // MANGO TODO

        int screen_index;
        native.connection = xcb_connect(nullptr, &screen_index);
        if (!native.connection)
        {
            MANGO_EXCEPTION("[Window] xcb_connect() failed.");
        }

        const xcb_setup_t* setup = xcb_get_setup(native.connection);
        xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

        while (screen_index-- > 0)
        {
            xcb_screen_next(&screen_iterator);
        }

        xcb_screen_t* screen = screen_iterator.data;
        native.window = xcb_generate_id(native.connection);

        uint32_t value_list[] =
        {
            screen->white_pixel,
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        };

        xcb_create_window(
            native.connection,
            XCB_COPY_FROM_PARENT,
            native.window,
            screen->root,
            20, 20,
            width, height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
            value_list);

        xcb_change_property(
            native.connection,
            XCB_PROP_MODE_REPLACE,
            native.window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            std::strlen(title),
            title);

        xcb_map_window(native.connection, native.window);
        xcb_flush(native.connection);
    }

    WindowHandle::~WindowHandle()
    {
        xcb_destroy_window(native.connection, native.window);
        xcb_disconnect(native.connection);
    }

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    int Window::getScreenCount()
    {
        xcb_connection_t* connection = xcb_connect(NULL, NULL);
        if (xcb_connection_has_error(connection))
        {
            MANGO_EXCEPTION("[Window] xcb_connect() failed.");
        }

        std::vector<ScreenInfo> screens = getScreenInfo();
        int count = int(screens.size());

        xcb_disconnect(connection);

        return count;
    }

    int32x2 Window::getScreenSize(int index)
    {
        xcb_connection_t* connection = xcb_connect(NULL, NULL);
        if (xcb_connection_has_error(connection))
        {
            MANGO_EXCEPTION("[Window] xcb_connect() failed.");
        }

        std::vector<ScreenInfo> screens = getScreenInfo();
        int count = int(screens.size());

        xcb_disconnect(connection);

        index = std::max(index, 0);
        index = std::min(index, count - 1);

        return screens[index];
    }

    Window::Window(int width, int height, u32 flags)
    {
        m_handle = std::make_unique<WindowHandle>(width, height);
        MANGO_UNREFERENCED(flags); // MANGO TODO
    }

    Window::~Window()
    {
    }

    void Window::setWindowPosition(int x, int y)
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        const u32 values [] = { u32(x), u32(y) };

        xcb_configure_window(
            connection,
            window,
            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
            values);
        xcb_flush(connection);
    }

    void Window::setWindowSize(int width, int height)
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        const u32 values [] = { u32(width), u32(height) };

        xcb_configure_window(
            connection,
            window,
            XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
            values);
        xcb_flush(connection);
    }

    void Window::setTitle(const std::string& title)
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        xcb_change_property(
            connection,
            XCB_PROP_MODE_REPLACE,
            window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            title.length(),
            title.c_str());
        xcb_flush(connection);
    }

    void Window::setIcon(const Surface& surface)
    {
        // MANGO TODO
    }

    void Window::setVisible(bool enable)
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        if (enable)
        {
            xcb_map_window(connection, window);
            xcb_flush(connection);
        }
        else
        {
            xcb_unmap_window(connection, window);
            xcb_flush(connection);
        }
    }

    int32x2 Window::getWindowSize() const
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connection, window);
        xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(connection, cookie, NULL);

        int width = 0;
        int height = 0;

        if (reply)
        {
            width = reply->width;
            height = reply->height;
            free(reply);
        }

        return int32x2(width, height);
    }

    int32x2 Window::getCursorPosition() const
    {
        auto connection = m_handle->native.connection;
        auto window = m_handle->native.window;

        xcb_query_pointer_cookie_t cookie = xcb_query_pointer(connection, window);
        xcb_query_pointer_reply_t* reply = xcb_query_pointer_reply(connection, cookie, NULL);

        int x = 0;
        int y = 0;

        if (reply)
        {
            x = reply->root_x;
            y = reply->root_y;
            free(reply);
        }

        return int32x2(x, y);
    }

    bool Window::isKeyPressed(Keycode code) const
    {
        // MANGO TODO
        return false;
    }

    Window::operator NativeWindowHandle () const
    {
        return m_handle->native;
    }

    Window::operator WindowHandle* () const
    {
        return m_handle.get();
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


        m_handle->is_looping = true;

        for ( ; m_handle->is_looping; )
        {
            xcb_generic_event_t* event = xcb_poll_for_event(m_handle->native.connection);
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
                            m_handle->is_looping = false;
                            free(delete_reply);
                        }
#endif
                        break;
                    }

                    case XCB_KEY_PRESS:
                    {
                        // MANGO TODO
                        m_handle->is_looping = false;
                        break;
                    }

                    case XCB_BUTTON_PRESS:
                    {
                        xcb_button_press_event_t* button_press = (xcb_button_press_event_t *)event;
                        (void) button_press; // MANGO TODO
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
    }

    void Window::breakEventLoop()
    {
        m_handle->is_looping = false;
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

#endif // defined(MANGO_WINDOW_SYSTEM_XCB)
