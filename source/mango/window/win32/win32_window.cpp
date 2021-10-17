/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <string>
#include <sstream>
#include <mango/core/string.hpp>
#include "win32_handle.hpp"

namespace
{
    using namespace mango;

    // -----------------------------------------------------------------------
    // enumToVirtual()
    // -----------------------------------------------------------------------

    int enumToVirtual(Keycode code)
    {
        int v = 0;

        switch (code)
        {
        case KEYCODE_ESC: v = VK_ESCAPE; break;
		case KEYCODE_0: v = '0'; break;
		case KEYCODE_1: v = '1'; break;
		case KEYCODE_2: v = '2'; break;
		case KEYCODE_3: v = '3'; break;
		case KEYCODE_4: v = '4'; break;
		case KEYCODE_5: v = '5'; break;
		case KEYCODE_6: v = '6'; break;
		case KEYCODE_7: v = '7'; break;
		case KEYCODE_8: v = '8'; break;
		case KEYCODE_9: v = '9'; break;
		case KEYCODE_A: v = 'A'; break;
		case KEYCODE_B: v = 'B'; break;
		case KEYCODE_C: v = 'C'; break;
		case KEYCODE_D: v = 'D'; break;
		case KEYCODE_E: v = 'E'; break;
		case KEYCODE_F: v = 'F'; break;
		case KEYCODE_G: v = 'G'; break;
		case KEYCODE_H: v = 'H'; break;
		case KEYCODE_I: v = 'I'; break;
		case KEYCODE_J: v = 'J'; break;
		case KEYCODE_K: v = 'K'; break;
		case KEYCODE_L: v = 'L'; break;
		case KEYCODE_M: v = 'M'; break;
		case KEYCODE_N: v = 'N'; break;
		case KEYCODE_O: v = 'O'; break;
		case KEYCODE_P: v = 'P'; break;
		case KEYCODE_Q: v = 'Q'; break;
		case KEYCODE_R: v = 'R'; break;
		case KEYCODE_S: v = 'S'; break;
		case KEYCODE_T: v = 'T'; break;
		case KEYCODE_U: v = 'U'; break;
		case KEYCODE_V: v = 'V'; break;
		case KEYCODE_W: v = 'W'; break;
		case KEYCODE_X: v = 'X'; break;
		case KEYCODE_Y: v = 'Y'; break;
		case KEYCODE_Z: v = 'Z'; break;
		case KEYCODE_F1: v = VK_F1; break;
		case KEYCODE_F2: v = VK_F2; break;
		case KEYCODE_F3: v = VK_F3; break;
		case KEYCODE_F4: v = VK_F4; break;
		case KEYCODE_F5: v = VK_F5; break;
		case KEYCODE_F6: v = VK_F6; break;
		case KEYCODE_F7: v = VK_F7; break;
		case KEYCODE_F8: v = VK_F8; break;
		case KEYCODE_F9: v = VK_F9; break;
		case KEYCODE_F10: v = VK_F10; break;
		case KEYCODE_F11: v = VK_F11; break;
		case KEYCODE_F12: v = VK_F12; break;
		case KEYCODE_NUMPAD0: v = VK_NUMPAD0; break;
		case KEYCODE_NUMPAD1: v = VK_NUMPAD1; break;
		case KEYCODE_NUMPAD2: v = VK_NUMPAD2; break;
		case KEYCODE_NUMPAD3: v = VK_NUMPAD3; break;
		case KEYCODE_NUMPAD4: v = VK_NUMPAD4; break;
		case KEYCODE_NUMPAD5: v = VK_NUMPAD5; break;
		case KEYCODE_NUMPAD6: v = VK_NUMPAD6; break;
		case KEYCODE_NUMPAD7: v = VK_NUMPAD7; break;
		case KEYCODE_NUMPAD8: v = VK_NUMPAD8; break;
		case KEYCODE_NUMPAD9: v = VK_NUMPAD9; break;
		case KEYCODE_NUMLOCK: v = VK_NUMLOCK; break;
        case KEYCODE_DIVIDE: v = VK_DIVIDE; break;
        case KEYCODE_MULTIPLY: v = VK_MULTIPLY; break;
        case KEYCODE_SUBTRACT: v = VK_SUBTRACT; break;
        case KEYCODE_ADDITION: v = VK_ADD; break;
        case KEYCODE_DECIMAL: v = VK_DECIMAL; break;
        case KEYCODE_BACKSPACE: v = VK_BACK; break;
        case KEYCODE_TAB: v = VK_TAB; break;
        case KEYCODE_RETURN: v = VK_RETURN; break;
        case KEYCODE_SPACE: v = VK_SPACE; break;
        case KEYCODE_PRINT_SCREEN: v = VK_PRINT; break;
        case KEYCODE_SCROLL_LOCK: v = VK_SCROLL; break;
        case KEYCODE_PAGE_UP: v = VK_PRIOR; break;
        case KEYCODE_PAGE_DOWN: v = VK_NEXT; break;
        case KEYCODE_INSERT: v = VK_INSERT; break;
        case KEYCODE_DELETE: v = VK_DELETE; break;
        case KEYCODE_HOME: v = VK_HOME; break;
        case KEYCODE_END: v = VK_END; break;
        case KEYCODE_LEFT: v = VK_LEFT; break;
        case KEYCODE_RIGHT: v = VK_RIGHT; break;
        case KEYCODE_UP: v = VK_UP; break;
        case KEYCODE_DOWN: v = VK_DOWN; break;
        }

        return v;
    }

	// -----------------------------------------------------------------------
	// virtualToEnum()
	// -----------------------------------------------------------------------

	Keycode virtualToEnum(WPARAM wparam, LPARAM lparam)
	{
		MANGO_UNREFERENCED(lparam);

		Keycode code = KEYCODE_NONE;

		switch (wparam)
		{
		case VK_ESCAPE:    code = KEYCODE_ESC; break;
		case '0':          code = KEYCODE_0; break;
		case '1':          code = KEYCODE_1; break;
		case '2':          code = KEYCODE_2; break;
		case '3':          code = KEYCODE_3; break;
		case '4':          code = KEYCODE_4; break;
		case '5':          code = KEYCODE_5; break;
		case '6':          code = KEYCODE_6; break;
		case '7':          code = KEYCODE_7; break;
		case '8':          code = KEYCODE_8; break;
		case '9':          code = KEYCODE_9; break;
		case 'A':          code = KEYCODE_A; break;
		case 'B':          code = KEYCODE_B; break;
		case 'C':          code = KEYCODE_C; break;
		case 'D':          code = KEYCODE_D; break;
		case 'E':          code = KEYCODE_E; break;
		case 'F':          code = KEYCODE_F; break;
		case 'G':          code = KEYCODE_G; break;
		case 'H':          code = KEYCODE_H; break;
		case 'I':          code = KEYCODE_I; break;
		case 'J':          code = KEYCODE_J; break;
		case 'K':          code = KEYCODE_K; break;
		case 'L':          code = KEYCODE_L; break;
		case 'M':          code = KEYCODE_M; break;
		case 'N':          code = KEYCODE_N; break;
		case 'O':          code = KEYCODE_O; break;
		case 'P':          code = KEYCODE_P; break;
		case 'Q':          code = KEYCODE_Q; break;
		case 'R':          code = KEYCODE_R; break;
		case 'S':          code = KEYCODE_S; break;
		case 'T':          code = KEYCODE_T; break;
		case 'U':          code = KEYCODE_U; break;
		case 'V':          code = KEYCODE_V; break;
		case 'W':          code = KEYCODE_W; break;
		case 'X':          code = KEYCODE_X; break;
		case 'Y':          code = KEYCODE_Y; break;
		case 'Z':          code = KEYCODE_Z; break;
		case VK_F1:        code = KEYCODE_F1; break;
		case VK_F2:        code = KEYCODE_F2; break;
		case VK_F3:        code = KEYCODE_F3; break;
		case VK_F4:        code = KEYCODE_F4; break;
		case VK_F5:        code = KEYCODE_F5; break;
		case VK_F6:        code = KEYCODE_F6; break;
		case VK_F7:        code = KEYCODE_F7; break;
		case VK_F8:        code = KEYCODE_F8; break;
		case VK_F9:        code = KEYCODE_F9; break;
		case VK_F10:       code = KEYCODE_F10; break;
		case VK_F11:       code = KEYCODE_F11; break;
		case VK_F12:       code = KEYCODE_F12; break;
		case VK_BACK:      code = KEYCODE_BACKSPACE; break;
		case VK_TAB:       code = KEYCODE_TAB; break;
		case VK_RETURN:    code = KEYCODE_RETURN; break;
			//KEYCODE_LEFT_ALT,
			//KEYCODE_RIGHT_ALT,
		case VK_SPACE:     code = KEYCODE_SPACE; break;
			//KEYCODE_CAPS_LOCK,
			//KEYCODE_LEFT_CTRL,
			//KEYCODE_RIGHT_CTRL,
			//KEYCODE_LEFT_SHIFT,
			//KEYCODE_RIGHT_SHIFT,
			//KEYCODE_LEFT_SUPER,
			//KEYCODE_RIGHT_SUPER,
			//KEYCODE_MENU,
			//KEYCODE_ALTGR,
		case VK_PRIOR:     code = KEYCODE_PAGE_UP; break;
		case VK_NEXT:      code = KEYCODE_PAGE_DOWN; break;
		case VK_INSERT:    code = KEYCODE_INSERT; break;
		case VK_DELETE:    code = KEYCODE_DELETE; break;
		case VK_HOME:      code = KEYCODE_HOME; break;
		case VK_END:       code = KEYCODE_END; break;
		case VK_LEFT:      code = KEYCODE_LEFT; break;
		case VK_RIGHT:     code = KEYCODE_RIGHT; break;
		case VK_UP:        code = KEYCODE_UP; break;
		case VK_DOWN:      code = KEYCODE_DOWN; break;
		case VK_PRINT:     code = KEYCODE_PRINT_SCREEN; break;
		case VK_SCROLL:    code = KEYCODE_SCROLL_LOCK; break;
			//KEYCODE_PAUSE_BREAK,
		case VK_NUMPAD0:   code = KEYCODE_NUMPAD0; break;
		case VK_NUMPAD1:   code = KEYCODE_NUMPAD1; break;
		case VK_NUMPAD2:   code = KEYCODE_NUMPAD2; break;
		case VK_NUMPAD3:   code = KEYCODE_NUMPAD3; break;
		case VK_NUMPAD4:   code = KEYCODE_NUMPAD4; break;
		case VK_NUMPAD5:   code = KEYCODE_NUMPAD5; break;
		case VK_NUMPAD6:   code = KEYCODE_NUMPAD6; break;
		case VK_NUMPAD7:   code = KEYCODE_NUMPAD7; break;
		case VK_NUMPAD8:   code = KEYCODE_NUMPAD8; break;
		case VK_NUMPAD9:   code = KEYCODE_NUMPAD9; break;
		case VK_NUMLOCK:   code = KEYCODE_NUMLOCK; break;
		case VK_DIVIDE:    code = KEYCODE_DIVIDE; break;
		case VK_MULTIPLY:  code = KEYCODE_MULTIPLY; break;
		case VK_SUBTRACT:  code = KEYCODE_SUBTRACT; break;
		case VK_ADD:       code = KEYCODE_ADDITION; break;
			// ENTER
		case VK_DECIMAL:   code = KEYCODE_DECIMAL; break;
		}

		return code;
	}

	u32 computeKeyMask()
	{
		u32 mask = 0;
		if (GetKeyState(VK_CONTROL) & 0x8000) mask |= KEYMASK_CTRL;
		if (GetKeyState(VK_SHIFT) & 0x8000) mask |= KEYMASK_SHIFT;
		if (GetKeyState(VK_LWIN) & 0x8000) mask |= KEYMASK_SUPER;
		if (GetKeyState(VK_RWIN) & 0x8000) mask |= KEYMASK_SUPER;
		if (GetKeyState(VK_LMENU) & 0x8000) mask |= KEYMASK_MENU;
		if (GetKeyState(VK_RMENU) & 0x8000) mask |= KEYMASK_MENU;
		return mask;
	}

	// -----------------------------------------------------------------------
	// utilities
	// -----------------------------------------------------------------------

	BOOL isDirectory(LPCWSTR name)
	{
		DWORD dwAttrib;

		if ((dwAttrib = GetFileAttributesW(name)) == 0xFFFFFFFF) return FALSE;
		return dwAttrib & FILE_ATTRIBUTE_DIRECTORY;
	}

	u64 getFileSize(LPCWSTR name)
	{
		LARGE_INTEGER nLargeInteger = { 0 };
		HANDLE hFile = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			BOOL bSuccess = GetFileSizeEx(hFile, &nLargeInteger);
			CloseHandle(hFile);
			(void)bSuccess;
		}

		return u64(nLargeInteger.QuadPart);
	}

	// -----------------------------------------------------------------------
	// WindowProc()
	// -----------------------------------------------------------------------

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT imsg, WPARAM wparam, LPARAM lparam)
	{
		LONG_PTR userdata = ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
		Window* window = reinterpret_cast<Window*>(userdata);
		if (!window)
		{
			return ::DefWindowProc(hwnd, imsg, wparam, lparam);
		}

		switch (imsg)
		{
		case WM_CREATE:
			break;

		case WM_DESTROY:
			return 0;

		case WM_CLOSE:
		{
			window->onClose();
			window->breakEventLoop();
			return 0;
		}

		case WM_SHOWWINDOW:
		{
			if (wparam)
				window->onShow();
			else
				window->onHide();
			return 0;
		}

		case WM_DISPLAYCHANGE:
		{
			::InvalidateRect(hwnd, NULL, FALSE);
			return 0;
		}

		case WM_PAINT:
		{
			if (!::IsIconic(hwnd))
			{
				if (::GetUpdateRect(hwnd, NULL, FALSE)) {
					window->onDraw();
				}
			}

			::ValidateRect(hwnd, NULL);
			return 0;
		}

		case WM_SIZE:
		{
			int width = LOWORD(lparam);
			int height = HIWORD(lparam);

			window->onResize(width, height);

			switch (wparam)
			{
			case SIZE_MINIMIZED:
				window->onMinimize();
				break;

			case SIZE_MAXIMIZED:
				window->onMaximize();
				break;

			case SIZE_RESTORED:
				break;
			}
			return 0;
		}

		case WM_SYSKEYUP:
			break;

		case WM_SYSKEYDOWN:
			break;

		case WM_KEYDOWN:
		{
			if (lparam & 0x40000000)
			{
				// ignore repeat
			}
			else
			{
				Keycode code = virtualToEnum(wparam, lparam);
				u32 mask = computeKeyMask();
				window->onKeyPress(code, mask);
			}
			return 0;
		}

		case WM_KEYUP:
		{
			Keycode code = virtualToEnum(wparam, lparam);
			window->onKeyRelease(code);
			return 0;
		}

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_XBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		{
			MouseButton button;
			int count;

			// Identify mouse button
			switch (imsg)
			{
			case WM_LBUTTONUP:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
				button = MOUSEBUTTON_LEFT;
				break;

			case WM_RBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONDBLCLK:
				button = MOUSEBUTTON_RIGHT;
				break;

			case WM_MBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONDBLCLK:
				button = MOUSEBUTTON_MIDDLE;
				break;

			case WM_XBUTTONUP:
			case WM_XBUTTONDOWN:
			case WM_XBUTTONDBLCLK:
				switch (GET_XBUTTON_WPARAM(wparam))
				{
				case XBUTTON1:
					button = MOUSEBUTTON_X1;
					break;

				case XBUTTON2:
					button = MOUSEBUTTON_X2;
					break;

				default:
					return 0;
				}
				break;

			case WM_MOUSEWHEEL:
				button = MOUSEBUTTON_WHEEL;
				break;

			default:
				return 0;
			}

			// Determine click count (0 = button released, 1 = button clicked, 2 = button double-clicked).
			// For the mouse wheel the count is signed and determines the direction (and amount) of the mouse wheel rotation.
			switch (imsg)
			{
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_XBUTTONUP:
				ReleaseCapture();
				count = 0;
				break;

			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_XBUTTONDOWN:
				SetCapture(hwnd);
				count = 1;
				break;

			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDBLCLK:
			case WM_XBUTTONDBLCLK:
				count = 2;
				break;

			case WM_MOUSEWHEEL:
				count = GET_WHEEL_DELTA_WPARAM(wparam);
				break;

			default:
				return 0;
			}

			short x = LOWORD(lparam);
			short y = HIWORD(lparam);
			window->onMouseClick(x, y, button, count);

			return 0;
		}

		case WM_MOUSEMOVE:
		{
			short x = LOWORD(lparam);
			short y = HIWORD(lparam);
			window->onMouseMove(x, y);
			return 0;
		}

		case WM_DROPFILES:
		{
			const int count = ::DragQueryFileW((HDROP)wparam, 0xffffffff, NULL, 0);

			filesystem::FileIndex dropped;

			for (int index = 0; index < count; ++index)
			{
				wchar_t filename[_MAX_PATH + _MAX_FNAME + 1];

				if (::DragQueryFileW((HDROP)wparam, index, filename, sizeof(filename)) > 0)
				{
					std::string s = mango::u16_toBytes(filename);
					if (isDirectory(filename))
					{
						dropped.emplace(s + "/", 0, filesystem::FileInfo::DIRECTORY);
					}
					else
					{
						u64 filesize = getFileSize(filename);
						dropped.emplace(s, filesize, 0);
					}
				}
			}

			window->onDropFiles(dropped);
			return 0;
		}

		case WM_ACTIVATE:
		{
			break;
		}

		case WM_SYSCOMMAND:
		{
			switch (wparam)
			{
			case SC_SCREENSAVE:
			case SC_MONITORPOWER:
				return 0;
			}
			break;
		}

		default:
			break;
		}

		return ::DefWindowProc(hwnd, imsg, wparam, lparam);
	}

} // namespace


namespace mango
{
    using namespace mango::math;
    using namespace mango::image;

	// -----------------------------------------------------------------------
	// WindowHandle
	// -----------------------------------------------------------------------

	WindowHandle::WindowHandle(int width, int height, u32 flags)
	{
		HINSTANCE hinstance = ::GetModuleHandle(NULL);

		// register window class
		wndclass.cbSize = sizeof(wndclass);
		wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.hInstance = hinstance;
		wndclass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = L"class::mango::window";
		wndclass.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hIconSm = ::LoadIcon(NULL, IDI_APPLICATION);

		::RegisterClassEx(&wndclass);

		// configuration
		DWORD mask = WS_OVERLAPPEDWINDOW;
        if (flags & Window::DISABLE_RESIZE)
        {
            mask &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }
		int x = 80;
		int y = 60;

		// adjust window rect
		RECT rect = { 0, 0, width - 1, height - 1 };
		::AdjustWindowRectEx(&rect, mask, FALSE, 0);
		width = rect.right - rect.left + 1;
		height = rect.bottom - rect.top + 1;

		// create window
		HWND parent = NULL;
		hwnd = ::CreateWindowExW(0, wndclass.lpszClassName, L"", 
			mask, x, y, width, height, parent, NULL, hinstance, NULL);

		::BringWindowToTop(hwnd);
		::SetForegroundWindow(hwnd);
		::SetFocus(hwnd);
		::DragAcceptFiles(hwnd, TRUE);
	}

	WindowHandle::~WindowHandle()
	{
		if (icon)
		{
			::DestroyIcon(icon);
		}

		if (hwnd)
		{
			::DestroyWindow(hwnd);
			::UnregisterClass(wndclass.lpszClassName, wndclass.hInstance);
		}
	}

	// -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    int Window::getScreenCount()
    {
        // TODO: support more than default screen
        return 1;
    }

    int32x2 Window::getScreenSize(int screen)
    {
        // TODO: support more than default screen
        MANGO_UNREFERENCED(screen);

        int width = int(GetSystemMetrics(SM_CXSCREEN));
        int height = int(GetSystemMetrics(SM_CYSCREEN));
        return int32x2(width, height);
    }

    Window::Window(int width, int height, u32 flags)
    {
        m_handle = new WindowHandle(width, height, flags);

		// register listener window
		LONG_PTR userdata = reinterpret_cast<LONG_PTR>(this);
		::SetWindowLongPtr(m_handle->hwnd, GWLP_USERDATA, userdata);
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
        RECT rect;

        rect.left = 0;
        rect.top = 0;
        rect.right = width - 1;
        rect.bottom = height - 1;

        ::AdjustWindowRect(&rect, (DWORD)GWL_STYLE, FALSE);

        width = rect.right - rect.left + 1;
        height = rect.bottom - rect.top + 1;

        ::SetWindowPos(m_handle->hwnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }

    void Window::setTitle(const std::string& title)
    {
        std::wstring name = mango::u16_fromBytes(title);
        ::SetWindowTextW(m_handle->hwnd, name.c_str());
    }

    void Window::setIcon(const Surface& icon)
    {
        Bitmap bitmap(icon, Format(32, Format::UNORM, Format::BGRA, 8, 8, 8, 8));

        HINSTANCE hinstance = ::GetModuleHandle(NULL);
        HICON hicon = ::CreateIcon(hinstance, icon.width, icon.height, 1, 32, NULL, bitmap.image);
        if (hicon)
        {
            if (m_handle->icon)
            {
                ::DestroyIcon(m_handle->icon);
                m_handle->icon = hicon;
            }
            SendMessage(m_handle->hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
            SendMessage(m_handle->hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);
            SendMessage(GetWindow(m_handle->hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hicon);
            SendMessage(GetWindow(m_handle->hwnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hicon);
        }
    }

    void Window::setVisible(bool enable)
    {
        int command = enable ? SW_SHOWNORMAL : SW_HIDE;
        ::ShowWindow(m_handle->hwnd, command);
    }

    int32x2 Window::getWindowSize() const
    {
        RECT rect;
        ::GetClientRect(m_handle->hwnd, &rect);
        return int32x2(rect.right - rect.left, rect.bottom - rect.top);
    }

	int32x2 Window::getCursorPosition() const
	{
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(m_handle->hwnd, &p);
		return int32x2(int(p.x), int(p.y));
	}

    bool Window::isKeyPressed(Keycode code) const
    {
		bool pressed = false;

		HWND active = GetActiveWindow();
		if (m_handle->hwnd == active)
		{
			int v = enumToVirtual(code);
			pressed = (GetAsyncKeyState(v) & 0x8000) != 0;
		}

		return pressed;
    }

    Window::operator HWND () const
    {
        return m_handle->hwnd;
    }

    void Window::enterEventLoop()
    {
        MSG msg;
        ::ZeroMemory(&msg, sizeof(msg));

		m_handle->looping = true;

		for (; m_handle->looping && msg.message != WM_QUIT;)
        {
            if (::PeekMessage(&msg, m_handle->hwnd, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
            else
            {
                onIdle();
            }
        }

		m_handle->looping = false;
    }

    void Window::breakEventLoop()
    {
        ::PostQuitMessage(0);
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
