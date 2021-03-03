/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2018 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include "xlib_handle.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace
{
    using namespace mango;
    using namespace mango::filesystem;

#define TR(a, b) case a: code = b; break

    Keycode translateEventToKeycode(XEvent* xe)
    {
        KeySym symbol = XLookupKeysym(reinterpret_cast<XKeyEvent*>(xe), 0);
        Keycode code;

        switch (symbol)
        {
            default: code = KEYCODE_NONE; break;
            TR(XK_Escape,         KEYCODE_ESC);
            TR(XK_0,              KEYCODE_0);
            TR(XK_1,              KEYCODE_1);
            TR(XK_2,              KEYCODE_2);
            TR(XK_3,              KEYCODE_3);
            TR(XK_4,              KEYCODE_4);
            TR(XK_5,              KEYCODE_5);
            TR(XK_6,              KEYCODE_6);
            TR(XK_7,              KEYCODE_7);
            TR(XK_8,              KEYCODE_8);
            TR(XK_9,              KEYCODE_9);
            TR(XK_a,              KEYCODE_A);
            TR(XK_b,              KEYCODE_B);
            TR(XK_c,              KEYCODE_C);
            TR(XK_d,              KEYCODE_D);
            TR(XK_e,              KEYCODE_E);
            TR(XK_f,              KEYCODE_F);
            TR(XK_g,              KEYCODE_G);
            TR(XK_h,              KEYCODE_H);
            TR(XK_i,              KEYCODE_I);
            TR(XK_j,              KEYCODE_J);
            TR(XK_k,              KEYCODE_K);
            TR(XK_l,              KEYCODE_L);
            TR(XK_m,              KEYCODE_M);
            TR(XK_n,              KEYCODE_N);
            TR(XK_o,              KEYCODE_O);
            TR(XK_p,              KEYCODE_P);
            TR(XK_q,              KEYCODE_Q);
            TR(XK_r,              KEYCODE_R);
            TR(XK_s,              KEYCODE_S);
            TR(XK_t,              KEYCODE_T);
            TR(XK_u,              KEYCODE_U);
            TR(XK_v,              KEYCODE_V);
            TR(XK_w,              KEYCODE_W);
            TR(XK_x,              KEYCODE_X);
            TR(XK_y,              KEYCODE_Y);
            TR(XK_z,              KEYCODE_Z);
            TR(XK_F1,             KEYCODE_F1);
            TR(XK_F2,             KEYCODE_F2);
            TR(XK_F3,             KEYCODE_F3);
            TR(XK_F4,             KEYCODE_F4);
            TR(XK_F5,             KEYCODE_F5);
            TR(XK_F6,             KEYCODE_F6);
            TR(XK_F7,             KEYCODE_F7);
            TR(XK_F8,             KEYCODE_F8);
            TR(XK_F9,             KEYCODE_F9);
            TR(XK_F10,            KEYCODE_F10);
            TR(XK_F11,            KEYCODE_F11);
            TR(XK_F12,            KEYCODE_F12);
            TR(XK_BackSpace,      KEYCODE_BACKSPACE);
            TR(XK_Tab,            KEYCODE_TAB);
            TR(XK_Return,         KEYCODE_RETURN);
            TR(XK_Alt_L,          KEYCODE_LEFT_ALT);
            TR(XK_Alt_R,          KEYCODE_RIGHT_ALT);
            TR(XK_space,          KEYCODE_SPACE);
            TR(XK_Caps_Lock,      KEYCODE_CAPS_LOCK);
            TR(XK_Page_Up,        KEYCODE_PAGE_UP);
            TR(XK_Page_Down,      KEYCODE_PAGE_DOWN);
            TR(XK_Insert,         KEYCODE_INSERT);
            TR(XK_Delete,         KEYCODE_DELETE);
            TR(XK_Home,           KEYCODE_HOME);
            TR(XK_End,            KEYCODE_END);
            TR(XK_Left,           KEYCODE_LEFT);
            TR(XK_Right,          KEYCODE_RIGHT);
            TR(XK_Up,             KEYCODE_UP);
            TR(XK_Down,           KEYCODE_DOWN);
            TR(XK_Print,          KEYCODE_PRINT_SCREEN);
            TR(XK_Scroll_Lock,    KEYCODE_SCROLL_LOCK);
            TR(XK_KP_0,           KEYCODE_NUMPAD0);
            TR(XK_KP_1,           KEYCODE_NUMPAD1);
            TR(XK_KP_2,           KEYCODE_NUMPAD2);
            TR(XK_KP_3,           KEYCODE_NUMPAD3);
            TR(XK_KP_4,           KEYCODE_NUMPAD4);
            TR(XK_KP_5,           KEYCODE_NUMPAD5);
            TR(XK_KP_6,           KEYCODE_NUMPAD6);
            TR(XK_KP_7,           KEYCODE_NUMPAD7);
            TR(XK_KP_8,           KEYCODE_NUMPAD8);
            TR(XK_KP_9,           KEYCODE_NUMPAD9);
            TR(XK_Num_Lock,       KEYCODE_NUMLOCK);
            TR(XK_KP_Divide,      KEYCODE_DIVIDE);
            TR(XK_KP_Multiply,    KEYCODE_MULTIPLY);
            TR(XK_KP_Subtract,    KEYCODE_SUBTRACT);
            TR(XK_KP_Add,         KEYCODE_ADDITION);
            TR(XK_KP_Enter,       KEYCODE_ENTER);
            TR(XK_KP_Decimal,     KEYCODE_DECIMAL);
        }

        return code;
    }

#undef TR
#define TR(a, b) case b: symbol = a; break

    KeySym translateKeycodeToSymbol(Keycode code)
    {
        KeySym symbol;

        switch (code)
        {
            default:
            TR(255,               KEYCODE_NONE);
            TR(XK_Escape,         KEYCODE_ESC);
            TR(XK_0,              KEYCODE_0);
            TR(XK_1,              KEYCODE_1);
            TR(XK_2,              KEYCODE_2);
            TR(XK_3,              KEYCODE_3);
            TR(XK_4,              KEYCODE_4);
            TR(XK_5,              KEYCODE_5);
            TR(XK_6,              KEYCODE_6);
            TR(XK_7,              KEYCODE_7);
            TR(XK_8,              KEYCODE_8);
            TR(XK_9,              KEYCODE_9);
            TR(XK_a,              KEYCODE_A);
            TR(XK_b,              KEYCODE_B);
            TR(XK_c,              KEYCODE_C);
            TR(XK_d,              KEYCODE_D);
            TR(XK_e,              KEYCODE_E);
            TR(XK_f,              KEYCODE_F);
            TR(XK_g,              KEYCODE_G);
            TR(XK_h,              KEYCODE_H);
            TR(XK_i,              KEYCODE_I);
            TR(XK_j,              KEYCODE_J);
            TR(XK_k,              KEYCODE_K);
            TR(XK_l,              KEYCODE_L);
            TR(XK_m,              KEYCODE_M);
            TR(XK_n,              KEYCODE_N);
            TR(XK_o,              KEYCODE_O);
            TR(XK_p,              KEYCODE_P);
            TR(XK_q,              KEYCODE_Q);
            TR(XK_r,              KEYCODE_R);
            TR(XK_s,              KEYCODE_S);
            TR(XK_t,              KEYCODE_T);
            TR(XK_u,              KEYCODE_U);
            TR(XK_v,              KEYCODE_V);
            TR(XK_w,              KEYCODE_W);
            TR(XK_x,              KEYCODE_X);
            TR(XK_y,              KEYCODE_Y);
            TR(XK_z,              KEYCODE_Z);
            TR(XK_F1,             KEYCODE_F1);
            TR(XK_F2,             KEYCODE_F2);
            TR(XK_F3,             KEYCODE_F3);
            TR(XK_F4,             KEYCODE_F4);
            TR(XK_F5,             KEYCODE_F5);
            TR(XK_F6,             KEYCODE_F6);
            TR(XK_F7,             KEYCODE_F7);
            TR(XK_F8,             KEYCODE_F8);
            TR(XK_F9,             KEYCODE_F9);
            TR(XK_F10,            KEYCODE_F10);
            TR(XK_F11,            KEYCODE_F11);
            TR(XK_F12,            KEYCODE_F12);
            TR(XK_BackSpace,      KEYCODE_BACKSPACE);
            TR(XK_Tab,            KEYCODE_TAB);
            TR(XK_Return,         KEYCODE_RETURN);
            TR(XK_Alt_L,          KEYCODE_LEFT_ALT);
            TR(XK_Alt_R,          KEYCODE_RIGHT_ALT);
            TR(XK_space,          KEYCODE_SPACE);
            TR(XK_Caps_Lock,      KEYCODE_CAPS_LOCK);
            TR(XK_Page_Up,        KEYCODE_PAGE_UP);
            TR(XK_Page_Down,      KEYCODE_PAGE_DOWN);
            TR(XK_Insert,         KEYCODE_INSERT);
            TR(XK_Delete,         KEYCODE_DELETE);
            TR(XK_Home,           KEYCODE_HOME);
            TR(XK_End,            KEYCODE_END);
            TR(XK_Left,           KEYCODE_LEFT);
            TR(XK_Right,          KEYCODE_RIGHT);
            TR(XK_Up,             KEYCODE_UP);
            TR(XK_Down,           KEYCODE_DOWN);
            TR(XK_Print,          KEYCODE_PRINT_SCREEN);
            TR(XK_Scroll_Lock,    KEYCODE_SCROLL_LOCK);
            TR(XK_KP_0,           KEYCODE_NUMPAD0);
            TR(XK_KP_1,           KEYCODE_NUMPAD1);
            TR(XK_KP_2,           KEYCODE_NUMPAD2);
            TR(XK_KP_3,           KEYCODE_NUMPAD3);
            TR(XK_KP_4,           KEYCODE_NUMPAD4);
            TR(XK_KP_5,           KEYCODE_NUMPAD5);
            TR(XK_KP_6,           KEYCODE_NUMPAD6);
            TR(XK_KP_7,           KEYCODE_NUMPAD7);
            TR(XK_KP_8,           KEYCODE_NUMPAD8);
            TR(XK_KP_9,           KEYCODE_NUMPAD9);
            TR(XK_Num_Lock,       KEYCODE_NUMLOCK);
            TR(XK_KP_Divide,      KEYCODE_DIVIDE);
            TR(XK_KP_Multiply,    KEYCODE_MULTIPLY);
            TR(XK_KP_Subtract,    KEYCODE_SUBTRACT);
            TR(XK_KP_Add,         KEYCODE_ADDITION);
            TR(XK_KP_Enter,       KEYCODE_ENTER);
            TR(XK_KP_Decimal,     KEYCODE_DECIMAL);
        }
        return symbol;
    }

#undef TR

    u32 translateKeyMask(int state)
    {
        u32 mask = 0;
        if (state & ControlMask) mask |= KEYMASK_CTRL;
        if (state & ShiftMask  ) mask |= KEYMASK_SHIFT;
        if (state & Mod4Mask   ) mask |= KEYMASK_SUPER;
        //if (state & xxxxxxxxxxx) mask |= KEYMASK_MENU;
        return mask;
    }

    MouseButton translateButton(int value)
    {
        MouseButton button = MOUSEBUTTON_MIDDLE;
        switch (value)
        {
            case 1: button = MOUSEBUTTON_LEFT; break;
            case 2: button = MOUSEBUTTON_MIDDLE; break;
            case 3: button = MOUSEBUTTON_RIGHT; break;
            case 4: button = MOUSEBUTTON_WHEEL; break; // wheel up
            case 5: button = MOUSEBUTTON_WHEEL; break; // wheel down
        }
        return button;
    }

    // -----------------------------------------------------------------------
    // Xdnd
    // -----------------------------------------------------------------------

    struct x11Prop
    {
        unsigned char* data;
        int format, count;
        Atom type;
    };

    void ReadProperty(x11Prop* p, Display* disp, ::Window w, Atom prop)
    {
        unsigned char* ret = NULL;
        Atom type;
        int fmt;
        unsigned long count;
        unsigned long bytes_left;
        int bytes_fetch = 0;

        do {
            if (ret != 0)
                XFree(ret);
            XGetWindowProperty(disp, w, prop, 0, bytes_fetch, False, AnyPropertyType, &type, &fmt, &count, &bytes_left, &ret);
            bytes_fetch += bytes_left;
        } while (bytes_left != 0);

        p->data = ret;
        p->format = fmt;
        p->count = count;
        p->type = type;
    }

    Atom PickTarget(::Display* disp, Atom list[], int list_count)
    {
        Atom request = None;
        char *name;
        int i;
        for (i = 0; i < list_count && request == None; i++) {
            name = XGetAtomName(disp, list[i]);
            if (strcmp("text/uri-list", name)==0)
                request = list[i];
            XFree(name);
        }
        return request;
    }

    Atom PickTargetFromAtoms(::Display* disp, Atom a0, Atom a1, Atom a2)
    {
        int count = 0;
        Atom atom[3];
        if (a0 != None) atom[count++] = a0;
        if (a1 != None) atom[count++] = a1;
        if (a2 != None) atom[count++] = a2;
        return PickTarget(disp, atom, count);
    }

    char* uri_to_local(char* uri)
    {
        if (memcmp(uri,"file:/", 6) == 0)
        {
            uri += 6; // local file
        }
        else if (strstr(uri, ":/") != nullptr)
        {
            return nullptr; // wrong scheme
        }

        bool local = uri[0] != '/' || (uri[0] != '\0' && uri[1] == '/');

        // is a hostname?
        if (!local && uri[0] == '/' && uri[2] != '/')
        {
            // transform network filename into local filename
            char* hostname_end = strchr(uri + 1, '/');
            if (hostname_end != nullptr)
            {
                char hostname[257];
                if (::gethostname(hostname, 255) == 0)
                {
                    hostname[256] = '\0';
                    if (memcmp(uri + 1, hostname, hostname_end - (uri + 1)) == 0)
                    {
                        uri = hostname_end + 1;
                        local = true;
                    }
                }
            }
        }

        char* file = nullptr;

        if (local)
        {
            file = uri;
            if (uri[1] == '/')
            {
                file++;
            }
            else
            {
                file--;
            }
        }

        return file;
    }

    inline char hex_to_char(char s)
    {
        char c;

        if (s >= '0' && s <= '9') c = s - '0';
        else if (s >= 'A' && s <= 'F') c = s - 'A' + 10;
        else if (s >= 'a' && s <= 'f') c = s - 'a' + 10;
        else c = 0;

        return c;
    }

    std::string uri_decode(const std::string& s)
    {
        const char* p = s.c_str();

        std::string result;

        for (auto length = s.length(); length > 0; )
        {
            const char c = p[0];
            if (c == '%' && length > 2)
            {
                char x = hex_to_char(p[1]) << 4;
                x |= hex_to_char(p[2]);
                if (x)
                {
                    result.push_back(x);
                    length -= 3;
                    p += 3;
                    continue;
                }
            }

            result.push_back(c);
            --length;
            ++p;
        }

        return result;
    }

    void dispatchOnDrop(mango::Window* window, x11Prop& p)
    {
        FileIndex dropped;

        bool expect_lf = false;
        char* start = nullptr;
        char* scan = (char*)p.data;
        int length = 0;

        while (p.count--)
        {
            if (!expect_lf)
            {
                if (*scan == 0x0d) 
                {
                    expect_lf = true;
                }

                if (start == nullptr)
                {
                    start = scan;
                    length = 0;
                }

                length++;
            }
            else
            {
                if (*scan == 0x0a && length > 0)
                {
                    std::vector<char> uri(length--);
                    memcpy(uri.data(), start, length);
                    uri[length] = '\0';

                    char* fn = uri_to_local(uri.data());
                    if (fn)
                    {
                        // decode HTML/URI encoded string to text
                        std::string filename = uri_decode(fn);

                        int fd = ::open(filename.c_str(), O_RDONLY);
                        if (fd >= 0)
                        {
                            struct stat s;
                            if (::fstat(fd, &s) == 0)
                            {
                                if ((s.st_mode & S_IFDIR) == 0)
                                {
                                    // file
                                    u64 filesize = u64(s.st_size);
                                    dropped.emplace(filename, filesize, 0);
                                }
                                else
                                {
                                    // folder
                                    dropped.emplace(filename + "/", 0, FileInfo::DIRECTORY);
                                }
                            }

                            ::close(fd);
                        }
                    }
                }

                expect_lf = false;
                start = nullptr;
            }

            scan++;
        }

        if (!dropped.empty())
        {
            window->onDropFiles(dropped);
        }
    }

    /*
    bool isXDND(WindowHandle& windowHandle)
    {
        if (!windowHandle.window)
            return false;

        Atom actual;
        int format;
        unsigned long count, remaining;
        unsigned char* data = 0;

        XGetWindowProperty(windowHandle.display,
                           windowHandle.window,
                           windowHandle.atom_xdnd_Aware,
                           0, 0x8000000L, False, XA_ATOM,
                           &actual, &format, &count, &remaining, &data);
        if (actual != XA_ATOM || format != 32 || count == 0 || !data)
        {
            if (data) XFree(data);
            return false;
        }

        if (data) XFree(data);

        return true;
    }
    */

} // namespace

namespace mango
{
    using namespace mango::math;
    using namespace mango::image;

    // -----------------------------------------------------------------------
    // WindowHandle
    // -----------------------------------------------------------------------

    WindowHandle::WindowHandle(int width, int height, u32 flags)
        : flags(flags)
    {
        MANGO_UNREFERENCED(width);
        MANGO_UNREFERENCED(height);

        display = XOpenDisplay(NULL);
        if (!display)
        {
            MANGO_EXCEPTION("[Window] XOpenDisplay() failed.");
        }
    }

    WindowHandle::~WindowHandle()
    {
        if (display)
        {
            if (window)
            {
                XDestroyWindow(display, window);
                window = 0;
            }

            if (colormap)
            {
                XFreeColormap(display, colormap);
                colormap = 0;
            }

            if (icon)
            {
                XDestroyImage(icon);
                icon = NULL;
            }

            XCloseDisplay(display);
            display = NULL;
        }
    }

    bool WindowHandle::createWindow(int screen, int depth, Visual* visual, int width, int height, const char* title)
    {
        if (!display)
            return false;

        ::Window root = RootWindow(display, screen);
        colormap = XCreateColormap(display, root, visual, AllocNone);

        XSetWindowAttributes wa;

        wa.colormap          = colormap;
        wa.background_pixmap = None ;
        wa.border_pixel      = 0;
	    wa.event_mask        = ExposureMask |
                               KeyPressMask |
                               KeyReleaseMask |
                               ButtonPressMask |
                               ButtonReleaseMask |
                               PointerMotionMask |
                               StructureNotifyMask;

        window = XCreateWindow(display, root,
            0, 0, width, height, 0, depth, InputOutput,
            visual, CWBorderPixel | CWColormap | CWEventMask, &wa);
        if (!window)
            return false;

        if (flags & Window::DISABLE_RESIZE)
        {
            XSizeHints hints;

            //hints.flags = PPosition | PMinSize | PMaxSize;
            hints.flags = PMinSize | PMaxSize;
            //hints.x = 0;
            //hints.y = 0;
            hints.min_width = width;
            hints.max_width = width;
            hints.min_height = height;
            hints.max_height = height;
  	        XSetWMNormalHints(display, window, &hints);
            XClearWindow(display, window);
            XMapRaised(display, window);
            XFlush(display);
        }

        // window close event atoms
        atom_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
        atom_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display, window, &atom_delete, 1);

        // fullscreen toggle atoms
        atom_state = XInternAtom(display, "_NET_WM_STATE", False);
        atom_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

        // primary atom
        atom_primary = XInternAtom(display, "PRIMARY", False);

        // xdnd atoms
        atom_xdnd_Aware = XInternAtom(display, "XdndAware", False);
        atom_xdnd_Enter = XInternAtom(display, "XdndEnter", False);
        atom_xdnd_Position = XInternAtom(display, "XdndPosition", False);
        atom_xdnd_Status = XInternAtom(display, "XdndStatus", False);
        atom_xdnd_TypeList = XInternAtom(display, "XdndTypeList", False);
        atom_xdnd_ActionCopy = XInternAtom(display, "XdndActionCopy", False);
        atom_xdnd_Drop = XInternAtom(display, "XdndDrop", False);
        atom_xdnd_Finished = XInternAtom(display, "XdndFinished", False);
        atom_xdnd_Selection = XInternAtom(display, "XdndSelection", False);
        atom_xdnd_Leave = XInternAtom(display, "XdndLeave", False);

        unsigned char xdnd_version = 5;
        XChangeProperty(display, window, atom_xdnd_Aware, XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)&xdnd_version, 1);

        XFlush(display);
        XStoreName(display, window, title);
        XMapWindow(display, window);

        return true;
    }

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    int Window::getScreenCount()
    {
        // TODO: support more than default screen
        return 1;
    }

    int32x2 Window::getScreenSize(int unused__screen)
    {
        // TODO: support more than default screen
        MANGO_UNREFERENCED(unused__screen);

        Display* display = XOpenDisplay(NULL);
        if (!display)
        {
            MANGO_EXCEPTION("[Window] XOpenDisplay() failed.");
        }

        Screen* screen = XDefaultScreenOfDisplay(display);

        int width = XWidthOfScreen(screen);
        int height = XHeightOfScreen(screen);
        return int32x2(width, height);
    }

    Window::Window(int width, int height, u32 flags)
    {
		m_handle = new WindowHandle(width, height, flags);
    }

    Window::~Window()
    {
		delete m_handle;
    }

    void Window::setWindowPosition(int x, int y)
    {
        XMoveWindow(m_handle->display, m_handle->window, x, y);
    }

    void Window::setWindowSize(int width, int height)
    {
        XResizeWindow(m_handle->display, m_handle->window, width, height);
    }

    void Window::setTitle(const std::string& title)
    {
        XStoreName(m_handle->display, m_handle->window, title.c_str());
        XMapWindow(m_handle->display, m_handle->window);
    }

    void Window::setIcon(const Surface& surface)
    {
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
        size_t stride = (width + 7) / 8; // round to next multiple of 8
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
    }

    void Window::setVisible(bool enable)
    {
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
    }

    int32x2 Window::getWindowSize() const
    {
        XWindowAttributes attributes;
        XGetWindowAttributes(m_handle->display, m_handle->window, &attributes);
        return int32x2(attributes.width, attributes.height);
    }

	int32x2 Window::getCursorPosition() const
	{
		::Window root;
		::Window child;
		int root_x, root_y;
		int child_x, child_y;
		unsigned int mask;

		XQueryPointer(m_handle->display, m_handle->window, &root, &child,
                      &root_x, &root_y, &child_x, &child_y, &mask);
		return int32x2(child_x, child_y);
	}

    bool Window::isKeyPressed(Keycode code) const
    {
        bool pressed = false;

        // get window with input focus
        ::Window focused;
        int temp;
        XGetInputFocus(m_handle->display, &focused, &temp);

        // only report keys for our window when it has input focus
        if (m_handle->window == focused)
        {
            char keys[32];
            XQueryKeymap(m_handle->display, keys);

            KeySym symbol = translateKeycodeToSymbol(code);
            int keyidx = XKeysymToKeycode(m_handle->display, symbol);

            if (keyidx >=0 && keyidx < 255)
            {
                pressed = (keys[keyidx / 8] & (1 << (keyidx % 8))) != 0;
            }
        }

        return pressed;
    }

    void Window::enterEventLoop()
    {
        m_handle->looping = true;

        static Timer timer;

        for (int i = 0; i < 6; ++i)
        {
            m_handle->mouse_time[i] = 0;
        }

        for (; m_handle->looping;)
        {
            for (; XPending(m_handle->display) > 0;)
            {
                XEvent e;
                XNextEvent(m_handle->display, &e);

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
                        {
                            onKeyRelease(translateEventToKeycode(&e));
                        }
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

#if 0
                        // debugging
                        char* name = XGetAtomName(m_handle->display, type);
                        printf("ClientMessage: %s\n", name);
                        XFree(name);
#endif

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

                    default:
                        break;
                } // switch
            }

            if (!m_handle->busy) {
                onIdle();
            }

            // Sleep 0.125 ms to avoid saturating CPU
            usleep(125);
        }
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

    void Window::onDropFiles(const FileIndex& index)
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
