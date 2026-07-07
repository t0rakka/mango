/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2025 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/configure.hpp>
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/core/system.hpp>
#include <mango/core/timer.hpp>

#include "xlib_window.hpp"

#include <cstdint>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <X11/extensions/Xrandr.h>

namespace
{
    using namespace mango;
    using namespace mango::filesystem;

    double queryXlibRefreshRate(Display* dpy, ::Window win)
    {
        int event_base, error_base;
        if (!XRRQueryExtension(dpy, &event_base, &error_base))
        {
            return 0.0;
        }

        XRRScreenResources* resources = XRRGetScreenResourcesCurrent(dpy, win);
        if (!resources)
        {
            return 0.0;
        }

        double refresh = 0.0;

        ::Window root;
        int x, y;
        unsigned width, height, border, depth;
        if (XGetGeometry(dpy, win, &root, &x, &y, &width, &height, &border, &depth))
        {
            int root_x, root_y;
            ::Window child;
            if (XTranslateCoordinates(dpy, win, root, int(width / 2), int(height / 2), &root_x, &root_y, &child))
            {
                RROutput chosen = None;

                for (int i = 0; i < resources->noutput; ++i)
                {
                    XRROutputInfo* output = XRRGetOutputInfo(dpy, resources, resources->outputs[i]);
                    if (!output || output->connection != RR_Connected)
                    {
                        XRRFreeOutputInfo(output);
                        continue;
                    }

                    bool matched = false;

                    for (int j = 0; j < output->ncrtc; ++j)
                    {
                        XRRCrtcInfo* crtc = XRRGetCrtcInfo(dpy, resources, output->crtcs[j]);
                        if (!crtc || crtc->mode == None)
                        {
                            XRRFreeCrtcInfo(crtc);
                            continue;
                        }

                        if (root_x >= crtc->x && root_x < int(crtc->x + crtc->width) &&
                            root_y >= crtc->y && root_y < int(crtc->y + crtc->height))
                        {
                            chosen = resources->outputs[i];
                            matched = true;
                        }

                        XRRFreeCrtcInfo(crtc);
                        if (matched)
                        {
                            break;
                        }
                    }

                    XRRFreeOutputInfo(output);
                    if (matched)
                    {
                        break;
                    }
                }

                if (chosen == None)
                {
                    chosen = XRRGetOutputPrimary(dpy, root);
                }

                if (chosen != None)
                {
                    XRROutputInfo* output = XRRGetOutputInfo(dpy, resources, chosen);
                    if (output && output->crtc != None)
                    {
                        XRRCrtcInfo* crtc = XRRGetCrtcInfo(dpy, resources, output->crtc);
                        if (crtc && crtc->mode != None)
                        {
                            for (int i = 0; i < resources->nmode; ++i)
                            {
                                const XRRModeInfo& mode = resources->modes[i];
                                if (mode.id == crtc->mode)
                                {
                                    if (mode.hTotal > 0 && mode.vTotal > 0)
                                    {
                                        refresh = double(mode.dotClock) / double(mode.hTotal * mode.vTotal);
                                    }
                                    break;
                                }
                            }
                        }
                        XRRFreeCrtcInfo(crtc);
                    }
                    XRRFreeOutputInfo(output);
                }
            }
        }

        XRRFreeScreenResources(resources);
        return refresh;
    }

#define TR(a, b) case a: code = b; break

    Keycode translateEventToKeycode(XEvent* xe)
    {
        KeySym symbol = XLookupKeysym(reinterpret_cast<XKeyEvent*>(xe), 0);
        Keycode code;

        switch (symbol)
        {
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
            TR(XK_Shift_L,        KEYCODE_LEFT_SHIFT);
            TR(XK_Shift_R,        KEYCODE_RIGHT_SHIFT);
            TR(XK_Control_L,      KEYCODE_LEFT_CONTROL);
            TR(XK_Control_R,      KEYCODE_RIGHT_CONTROL);

            default:
                code = KEYCODE_NONE;
                break;
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
            TR(XK_Shift_L,        KEYCODE_LEFT_SHIFT);
            TR(XK_Shift_R,        KEYCODE_RIGHT_SHIFT);
            TR(XK_Control_L,      KEYCODE_LEFT_CONTROL);
            TR(XK_Control_R,      KEYCODE_RIGHT_CONTROL);
        }

        return symbol;
    }

#undef TR

    u32 translateKeyMask(int state)
    {
        u32 mask = 0;
        if (state & ControlMask) mask |= KEYMASK_CONTROL;
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

    inline
    char hex_to_char(char s)
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
                                    dropped.emplace(filename + "/", 0, FileInfo::Directory);
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

} // namespace

namespace mango
{
    using namespace mango::math;

    // -----------------------------------------------------------------------
    // XlibBackend
    // -----------------------------------------------------------------------

    XlibBackend::XlibBackend()
    {
        display = XOpenDisplay(NULL);
        if (!display)
        {
            MANGO_EXCEPTION("[XlibBackend] XOpenDisplay() failed.");
        }
    }

    XlibBackend::~XlibBackend()
    {
        Display* dpy = x11Display();
        if (dpy)
        {
            ::Window win = x11Window();
            if (win)
            {
                XDestroyWindow(dpy, win);
                window = 0;
            }

            if (x11_colormap)
            {
                XFreeColormap(dpy, x11_colormap);
                x11_colormap = 0;
            }

            if (x11_icon)
            {
                XDestroyImage(x11_icon);
                x11_icon = NULL;
            }

            if (sync_counter)
            {
                XSyncDestroyCounter(dpy, sync_counter);
                sync_counter = 0;
            }

            XCloseDisplay(dpy);
            display = nullptr;
        }
    }

    bool XlibBackend::init(int screen, int depth, Visual* visual, int width, int height, u32 flags, const char* title)
    {
        Display* dpy = x11Display();
        if (!dpy)
            return false;

        if (!visual)
        {
            XVisualInfo vinfo;
            if (!XMatchVisualInfo(dpy, screen, 24, TrueColor, &vinfo))
            {
                return false;
            }

            visual = vinfo.visual;
            depth = vinfo.depth;
        }

        visualid = XVisualIDFromVisual(visual);

        ::Window root = screen ? RootWindow(dpy, screen)
                               : DefaultRootWindow(dpy);

        x11_colormap = XCreateColormap(dpy, root, visual, AllocNone);
        x11_visual = visual;

        XSetWindowAttributes wa;

        wa.colormap          = x11_colormap;
        wa.background_pixmap = None ;
        wa.border_pixel      = 0;
        wa.event_mask        = ExposureMask |
                               KeyPressMask |
                               KeyReleaseMask |
                               ButtonPressMask |
                               ButtonReleaseMask |
                               PointerMotionMask |
                               StructureNotifyMask;

        window = XCreateWindow(dpy, root,
            0, 0, width, height,
            0,
            depth, InputOutput, visual,
            CWBorderPixel | CWColormap | CWEventMask, &wa);

        ::Window win = x11Window();
        if (!win)
        {
            //printLine("[WindowContext] XCreateWindow failed.");
            return false;
        }

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
            XSetWMNormalHints(dpy, win, &hints);
            XClearWindow(dpy, win);
            XFlush(dpy);
        }

        // window close event atoms
        atom_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
        atom_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

        // Frame synchronization (_NET_WM_SYNC_REQUEST). Only enabled when the X server
        // has the SYNC extension. A 64-bit counter is created and its id published in
        // _NET_WM_SYNC_REQUEST_COUNTER; the compositor then sends a sync request before
        // each resize and waits for us to bump the counter once the matching frame is
        // drawn, so the window border never shows undefined content during a live resize.
        atom_sync_request = XInternAtom(dpy, "_NET_WM_SYNC_REQUEST", False);
        atom_sync_counter = XInternAtom(dpy, "_NET_WM_SYNC_REQUEST_COUNTER", False);

        int sync_event_base, sync_error_base, sync_major = 3, sync_minor = 1;
        if (XSyncQueryExtension(dpy, &sync_event_base, &sync_error_base) &&
            XSyncInitialize(dpy, &sync_major, &sync_minor))
        {
            XSyncIntToValue(&sync_value, 0);
            sync_counter = XSyncCreateCounter(dpy, sync_value);
            long counter_id = long(sync_counter);
            XChangeProperty(dpy, win, atom_sync_counter, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char*)&counter_id, 1);
            sync_supported = true;
        }

        // Advertise WM_DELETE_WINDOW, plus _NET_WM_SYNC_REQUEST when supported.
        Atom protocols[2] = { atom_delete, atom_sync_request };
        XSetWMProtocols(dpy, win, protocols, sync_supported ? 2 : 1);

        // fullscreen toggle atoms
        atom_state = XInternAtom(dpy, "_NET_WM_STATE", False);
        atom_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

        // primary atom
        atom_primary = XInternAtom(dpy, "PRIMARY", False);

        // xdnd atoms
        atom_xdnd_Aware = XInternAtom(dpy, "XdndAware", False);
        atom_xdnd_Enter = XInternAtom(dpy, "XdndEnter", False);
        atom_xdnd_Position = XInternAtom(dpy, "XdndPosition", False);
        atom_xdnd_Status = XInternAtom(dpy, "XdndStatus", False);
        atom_xdnd_TypeList = XInternAtom(dpy, "XdndTypeList", False);
        atom_xdnd_ActionCopy = XInternAtom(dpy, "XdndActionCopy", False);
        atom_xdnd_Drop = XInternAtom(dpy, "XdndDrop", False);
        atom_xdnd_Finished = XInternAtom(dpy, "XdndFinished", False);
        atom_xdnd_Selection = XInternAtom(dpy, "XdndSelection", False);
        atom_xdnd_Leave = XInternAtom(dpy, "XdndLeave", False);

        unsigned char xdnd_version = 5;
        XChangeProperty(dpy, win, atom_xdnd_Aware, XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)&xdnd_version, 1);

        XFlush(dpy);
        XStoreName(dpy, win, title);

        // NOTE: the window is created un-mapped (not visible). This is required by Vulkan
        // so that nothing is shown while the application configures itself; the caller
        // makes the window visible with setVisible(true). OpenGLWindow does this
        // automatically after onContextReady() when enterEventLoop() is called.

        int randr_event_base, randr_error_base;
        if (XRRQueryExtension(dpy, &randr_event_base, &randr_error_base))
        {
            XRRSelectInput(dpy, win, RRScreenChangeNotifyMask);
        }

        return true;
    }

    void XlibBackend::toggleFullscreen()
    {
        Display* dpy = x11Display();
        ::Window win = x11Window();

        XEvent event;
        std::memset(&event, 0, sizeof(event));

        event.type = ClientMessage;
        event.xclient.window = win;
        event.xclient.message_type = atom_state;
        event.xclient.format = 32;
        event.xclient.data.l[0] = 2; // NET_WM_STATE_TOGGLE
        event.xclient.data.l[1] = atom_fullscreen;
        event.xclient.data.l[2] = 0; // no second property to toggle
        event.xclient.data.l[3] = 1; // source indication: application
        event.xclient.data.l[4] = 0; // unused

        XMapWindow(dpy, win);

        // send the event to the root window
        XSendEvent(dpy, DefaultRootWindow(dpy),
            False, SubstructureRedirectMask | SubstructureNotifyMask, &event);

        XFlush(dpy);

        // Get window dimensions
        XWindowAttributes attributes;
        XGetWindowAttributes(dpy, win, &attributes);

        XEvent expose_event;
        std::memset(&expose_event, 0, sizeof(event));

        expose_event.type = Expose;
        expose_event.xexpose.window = win;
        expose_event.xexpose.x = 0;
        expose_event.xexpose.y = 0;
        expose_event.xexpose.width = attributes.width;
        expose_event.xexpose.height = attributes.height;
        expose_event.xexpose.count = 0;

        // Send Expose event (generates Window::onDraw callback)
        XSendEvent(dpy, win, False, NoEventMask, &expose_event);

        fullscreen = !fullscreen;
    }

    bool XlibBackend::isFullscreen() const
    {
        return fullscreen;
    }

    math::int32x2 XlibBackend::getWindowSize() const
    {
        XWindowAttributes attributes;
        XGetWindowAttributes(x11Display(), x11Window(), &attributes);
        return int32x2(attributes.width, attributes.height);
    }

    // -----------------------------------------------------------------------
    // XlibBackend factory
    // -----------------------------------------------------------------------

    std::unique_ptr<WindowBackend> createXlibBackend(Window* window, int width, int height, u32 flags, const char* title)
    {
        auto backend = std::make_unique<XlibBackend>();
        backend->owner = window;

        if (flags & Window::API_OPENGL)
        {
            // GLX and EGL must choose a visual before creating the window;
            // the GL context creation calls init() once the visual is known.
        }
        else
        {
            int screen = DefaultScreen(backend->x11Display());
            int depth = DefaultDepth(backend->x11Display(), screen);
            if (!backend->init(screen, depth, nullptr, width, height, flags, title))
            {
                return nullptr;
            }
        }

        return backend;
    }

    // -----------------------------------------------------------------------
    // Window (static, screen queries)
    // -----------------------------------------------------------------------

    int Window::getScreenCount()
    {
        Display* display = XOpenDisplay(NULL);
        if (!display)
        {
            MANGO_EXCEPTION("[Window] XOpenDisplay() failed.");
        }

        int count = ScreenCount(display);

        XCloseDisplay(display);

        return count;
    }

    int32x2 Window::getScreenSize(int index)
    {
        Display* display = XOpenDisplay(NULL);
        if (!display)
        {
            MANGO_EXCEPTION("[Window] XOpenDisplay() failed.");
        }

        int count = ScreenCount(display);

        index = std::max(index, 0);
        index = std::min(index, count - 1);

        Screen* screen = ScreenOfDisplay(display, index);

        int width = XWidthOfScreen(screen);
        int height = XHeightOfScreen(screen);

        XCloseDisplay(display);

        return int32x2(width, height);
    }

    // -----------------------------------------------------------------------
    // XlibBackend (window operations + event loop)
    // -----------------------------------------------------------------------

    void XlibBackend::setWindowPosition(int x, int y)
    {
        XMoveWindow(x11Display(), x11Window(), x, y);
    }

    void XlibBackend::setWindowSize(int width, int height)
    {
        XResizeWindow(x11Display(), x11Window(), width, height);
    }

    void XlibBackend::setTitle(const std::string& title)
    {
        XStoreName(x11Display(), x11Window(), title.c_str());
        XFlush(x11Display());
    }

    void XlibBackend::setVisible(bool enable)
    {
        if (enable)
        {
            XMapRaised(x11Display(), x11Window());
            XFlush(x11Display());
        }
        else
        {
            XUnmapWindow(x11Display(), x11Window());
            XFlush(x11Display());
        }
    }

    int32x2 XlibBackend::getCursorPosition() const
    {
        ::Window root;
        ::Window child;
        int root_x, root_y;
        int child_x, child_y;
        unsigned int mask;

        XQueryPointer(x11Display(), x11Window(), &root, &child,
                      &root_x, &root_y, &child_x, &child_y, &mask);
        return int32x2(child_x, child_y);
    }

    bool XlibBackend::isKeyPressed(Keycode code) const
    {
        switch (code)
        {
            case KEYCODE_SHIFT:
                return isKeyPressed(KEYCODE_LEFT_SHIFT) || isKeyPressed(KEYCODE_RIGHT_SHIFT);

            case KEYCODE_CONTROL:
                return isKeyPressed(KEYCODE_LEFT_CONTROL) || isKeyPressed(KEYCODE_RIGHT_CONTROL);

            default:
                break;
        }

        bool pressed = false;

        // get window with input focus
        ::Window focused;
        int temp;
        XGetInputFocus(x11Display(), &focused, &temp);

        // only report keys for our window when it has input focus
        if (x11Window() == focused)
        {
            char keys[32];
            XQueryKeymap(x11Display(), keys);

            KeySym symbol = translateKeycodeToSymbol(code);
            int keyidx = XKeysymToKeycode(x11Display(), symbol);

            if (keyidx >=0 && keyidx < 255)
            {
                pressed = (keys[keyidx / 8] & (1 << (keyidx % 8))) != 0;
            }
        }

        return pressed;
    }

    double XlibBackend::getDisplayRefreshRate() const
    {
        return queryXlibRefreshRate(x11Display(), x11Window());
    }

    void XlibBackend::wakeEventLoop()
    {
        // The loop blocks in poll() on the X connection fd. Same-thread state changes
        // (invalidate / requestFrame / breakEventLoop) are applied between iterations,
        // and the idle wait is capped, so a cross-thread change is noticed within the
        // cap without an explicit wake. A self-pipe could make this immediate later.
    }

    void XlibBackend::runEventLoop()
    {
        for (int i = 0; i < 6; ++i)
        {
            mouse_time[i] = 0;
        }

        owner->syncDisplayRefreshRate();

        while (owner->isRunning())
        {
            while (XPending(x11Display()) > 0)
            {
                XEvent e;
                XNextEvent(x11Display(), &e);

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
                                u32 time = Time::ms();
                                if (time - mouse_time[button] < 300)
                                {
                                    count = 2;
                                }
                                mouse_time[button] = time;
                                break;
                            }

                            case 4:
                                count = 120;
                                break;

                            case 5:
                                count = -120;
                                break;
                        }

                        owner->onMouseClick(e.xbutton.x, e.xbutton.y, button, count);
                        break;
                    }

                    case ButtonRelease:
                    {
                        MouseButton button = translateButton(e.xbutton.button);
                        owner->onMouseClick(e.xbutton.x, e.xbutton.y, button, 0);
                        break;
                    }

                    case MotionNotify:
                        owner->onMouseMove(e.xmotion.x, e.xmotion.y);
                        break;

                    case KeyPress:
                    {
                        u32 mask = translateKeyMask(e.xkey.state);
                        owner->onKeyPress(translateEventToKeycode(&e), mask);
                        break;
                    }

                    case KeyRelease:
                    {
                        bool is_repeat = false;

                        if (XEventsQueued(x11Display(), QueuedAfterReading))
                        {
                            XEvent next;
                            XPeekEvent(x11Display(), &next);

                            if (next.type == KeyPress &&
                                next.xkey.time == e.xkey.time &&
                                next.xkey.keycode == e.xkey.keycode)
                            {
                                // delete repeated KeyPress event
                                XNextEvent(x11Display(), &e);
                                is_repeat = true;
                            }
                        }

                        if (!is_repeat)
                        {
                            owner->onKeyRelease(translateEventToKeycode(&e));
                        }
                        break;
                    }

                    case ConfigureNotify:
                    {
                        // Coalesce: record the new size and flag a pending resize. The whole
                        // queue is drained in this loop, so the latest size wins; onResize()
                        // and the frame are emitted once, after draining (see below). The frame
                        // is what bumps the _NET_WM_SYNC counter, so the compositor keeps the
                        // previous content until our resized frame lands.
                        int width = e.xconfigure.width;
                        int height = e.xconfigure.height;
                        if (width != size[0] || height != size[1])
                        {
                            size[0] = width;
                            size[1] = height;
                            resize_pending = true;
                        }

                        owner->syncDisplayRefreshRate();
                        break;
                    }

                    case Expose:
                        if (!busy)
                        {
                            owner->invalidate();
                        }
                        break;

                    case DestroyNotify:
                        break;

                    case ClientMessage:
                    {
                        unsigned int type = e.xclient.message_type;

#if 0
                        // debugging
                        char* name = XGetAtomName(x11Display(), type);
                        printf("ClientMessage: %s\n", name);
                        XFree(name);
#endif

                        if (type == atom_xdnd_Enter)
                        {
                            bool use_list = e.xclient.data.l[1] & 1;
                            xdnd_source = e.xclient.data.l[0];
                            xdnd_version = (e.xclient.data.l[1] >> 24);
                            if (use_list)
                            {
                                // fetch conversion targets
                                x11Prop p;
                                ReadProperty(&p, x11Display(), xdnd_source, atom_xdnd_TypeList);
                                // pick one
                                atom_xdnd_req = PickTarget(x11Display(), (Atom*)p.data, p.count);
                                XFree(p.data);
                            }
                            else
                            {
                                // pick from list of three
                                atom_xdnd_req = PickTargetFromAtoms(x11Display(),
                                    e.xclient.data.l[2], e.xclient.data.l[3], e.xclient.data.l[4]);
                            }
                        }
                        else if (type == atom_xdnd_Position)
                        {
                            XClientMessageEvent m = { 0 };

                            m.type = ClientMessage;
                            m.display = e.xclient.display;
                            m.window = e.xclient.data.l[0];
                            m.message_type = atom_xdnd_Status;
                            m.format=32;
                            m.data.l[0] = x11Window();
                            m.data.l[1] = (atom_xdnd_req != None);
                            m.data.l[2] = 0; // specify an empty rectangle
                            m.data.l[3] = 0;
                            m.data.l[4] = atom_xdnd_ActionCopy; // we only accept copying anyway

                            XSendEvent(x11Display(), e.xclient.data.l[0], False, NoEventMask, (XEvent*)&m);
                            XFlush(x11Display());
                        }
                        else if (type == atom_xdnd_Status)
                        {
                        }
                        else if (type == atom_xdnd_TypeList)
                        {
                        }
                        else if (type == atom_xdnd_ActionCopy)
                        {
                        }
                        else if (type == atom_xdnd_Drop)
                        {
                            if (atom_xdnd_req == None)
                            {
                                // respond to empty request
                                XClientMessageEvent m = { 0 };

                                m.type = ClientMessage;
                                m.display = e.xclient.display;
                                m.window = e.xclient.data.l[0];
                                m.message_type = atom_xdnd_Finished;
                                m.format = 32;
                                m.data.l[0] = x11Window();
                                m.data.l[1] = 0;
                                m.data.l[2] = None; // failed
                                XSendEvent(x11Display(), e.xclient.data.l[0],
                                    False, NoEventMask, (XEvent*)&m);
                            }
                            else
                            {
                                // convert
                                if (xdnd_version >= 1)
                                {
                                    XConvertSelection(x11Display(), atom_xdnd_Selection, atom_xdnd_req, 
                                                      atom_primary, x11Window(), e.xclient.data.l[2]);
                                }
                                else
                                {
                                    XConvertSelection(x11Display(), atom_xdnd_Selection, atom_xdnd_req, 
                                                      atom_primary, x11Window(), CurrentTime);
                                }
                            }
                        }
                        else if (type == atom_xdnd_Finished)
                        {
                        }
                        else if (type == atom_xdnd_Selection)
                        {
                        }
                        else if (type == atom_xdnd_Leave)
                        {
                        }
                        else if (type == atom_protocols)
                        {
                            if ((Atom)e.xclient.data.l[0] == atom_delete)
                            {
                                owner->breakEventLoop();
                                // NOTE: We should destroy the window here since it doesn't exist anymore.
                            }
                            else if (sync_supported && (Atom)e.xclient.data.l[0] == atom_sync_request)
                            {
                                // Store the value the compositor wants the counter set to once
                                // we have drawn the frame for the upcoming resize. l[2] is the
                                // low word, l[3] the high word.
                                XSyncIntsToValue(&sync_value, e.xclient.data.l[2], int(e.xclient.data.l[3]));
                                sync_pending = true;
                            }
                        }

                        break;
                    }

                    case SelectionNotify:
                    {
                        Atom target = e.xselection.target;
                        if (target == atom_xdnd_req)
                        {
                            // read data
                            x11Prop p;
                            ReadProperty(&p, x11Display(), x11Window(), atom_primary);

                            if (p.format == 8)
                            {
                                dispatchOnDrop(owner, p);
                            }

                            XFree(p.data);

                            // send reply
                            XClientMessageEvent m = { 0 };

                            m.type = ClientMessage;
                            m.display = x11Display();
                            m.window = xdnd_source;
                            m.message_type = atom_xdnd_Finished;
                            m.format = 32;
                            m.data.l[0] = x11Window();
                            m.data.l[1] = 1;
                            m.data.l[2] = atom_xdnd_ActionCopy;
                            XSendEvent(x11Display(), xdnd_source, False, NoEventMask, (XEvent*)&m);
                            XSync(x11Display(), False);
                        }

                        break;
                    }

                    default:
                        break;
                } // switch
            }

            // Emit a single resize for the coalesced final size before drawing, so the
            // frame dispatched below renders at the new extent.
            if (resize_pending && !busy)
            {
                resize_pending = false;
                owner->onResize(size[0], size[1]);
                owner->invalidate();
            }

            if (!busy)
            {
                owner->dispatchFrame();

                // The resized frame has now been submitted/presented; tell the compositor
                // it may show the new size. Doing this after dispatchFrame() is what keeps
                // the window border from flashing undefined content during a live resize.
                if (sync_pending && sync_supported)
                {
                    sync_pending = false;
                    XSyncSetCounter(x11Display(), sync_counter, sync_value);
                    XFlush(x11Display());
                }
            }

            // Block on the X connection fd until an event arrives or the next frame is
            // due, instead of busy-polling. An idle (WAIT_INFINITE) wait is capped so a
            // cross-thread state change is still observed within the cap; a pending
            // deadline (animation) is waited exactly, so a timed frame fires on time.
            if (XPending(x11Display()) == 0)
            {
                const u32 timeout = owner->eventLoop().computeWaitTimeoutMs(Time::us());
                if (timeout != 0)
                {
                    const int wait_ms = (timeout == EventLoopState::WAIT_INFINITE) ? 100 : int(timeout);
                    struct pollfd pfd = { ConnectionNumber(x11Display()), POLLIN, 0 };
                    ::poll(&pfd, 1, wait_ms);
                }
            }
        }
    }

    void* XlibBackend::createNativeWindowForGraphics(int width, int height, u32 flags)
    {
        if (!init(0, 0, nullptr, width, height, flags, "OpenGL|ES"))
        {
            return nullptr;
        }

        return reinterpret_cast<void*>(static_cast<std::uintptr_t>(window));
    }

} // namespace mango

#include "../window_registry.hpp"
MANGO_REGISTER_WINDOW_BACKEND(Xlib, createXlibBackend);
