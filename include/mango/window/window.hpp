/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <vector>
#include <mango/math/math.hpp>
#include <mango/image/surface.hpp>
#include <mango/filesystem/filesystem.hpp>

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_WIN32
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_WIN32)

namespace mango
{

    struct WindowHandle
    {
        HWND hwnd;
    };

} // namespace mango

#endif

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_COCOA
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_COCOA)

namespace mango
{

    struct WindowHandle
    {
        void* layer { nullptr }; // CAMetalLayer*, set when created with API_VULKAN
    };

} // namespace mango

#endif

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_XLIB
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_XLIB)

namespace mango
{

    namespace xlib
    {
        using Display = void*;
        using Window = unsigned long;
        using VisualID = unsigned long;
    }

    struct WindowHandle
    {
        xlib::Display display { nullptr };
        xlib::Window window { 0 };
        xlib::VisualID visualid { 0 };
    };

} // namespace mango

#endif

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_XCB
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_XCB)

    #include <xcb/xcb.h>

namespace mango
{

    struct WindowHandle
    {
        xcb_connection_t* connection { nullptr };
        xcb_window_t window { 0 };
        xcb_visualid_t visualid { 0 };
    };

} // namespace mango

#endif

// -----------------------------------------------------------------------
// MANGO_WINDOW_SYSTEM_WAYLAND
// -----------------------------------------------------------------------

#if defined(MANGO_WINDOW_SYSTEM_WAYLAND)

    #include <wayland-client.h>
    #include <wayland-client-protocol.h>

namespace mango
{

    struct WindowHandle
    {
        wl_display* display { nullptr };
        wl_surface* surface { nullptr };
    };

} // namespace mango

#endif

#if !defined(MANGO_WINDOW_SYSTEM_NONE)

namespace mango
{

    enum Keycode
    {
        KEYCODE_NONE,
        KEYCODE_ESC,
        KEYCODE_0,
        KEYCODE_1,
        KEYCODE_2,
        KEYCODE_3,
        KEYCODE_4,
        KEYCODE_5,
        KEYCODE_6,
        KEYCODE_7,
        KEYCODE_8,
        KEYCODE_9,
        KEYCODE_A,
        KEYCODE_B,
        KEYCODE_C,
        KEYCODE_D,
        KEYCODE_E,
        KEYCODE_F,
        KEYCODE_G,
        KEYCODE_H,
        KEYCODE_I,
        KEYCODE_J,
        KEYCODE_K,
        KEYCODE_L,
        KEYCODE_M,
        KEYCODE_N,
        KEYCODE_O,
        KEYCODE_P,
        KEYCODE_Q,
        KEYCODE_R,
        KEYCODE_S,
        KEYCODE_T,
        KEYCODE_U,
        KEYCODE_V,
        KEYCODE_W,
        KEYCODE_X,
        KEYCODE_Y,
        KEYCODE_Z,
        KEYCODE_F1,
        KEYCODE_F2,
        KEYCODE_F3,
        KEYCODE_F4,
        KEYCODE_F5,
        KEYCODE_F6,
        KEYCODE_F7,
        KEYCODE_F8,
        KEYCODE_F9,
        KEYCODE_F10,
        KEYCODE_F11,
        KEYCODE_F12,
        KEYCODE_BACKSPACE,
        KEYCODE_TAB,
        KEYCODE_RETURN,
        KEYCODE_LEFT_ALT,
        KEYCODE_RIGHT_ALT,
        KEYCODE_SPACE,
        KEYCODE_CAPS_LOCK,
        KEYCODE_PAGE_UP,
        KEYCODE_PAGE_DOWN,
        KEYCODE_INSERT,
        KEYCODE_DELETE,
        KEYCODE_HOME,
        KEYCODE_END,
        KEYCODE_LEFT,
        KEYCODE_RIGHT,
        KEYCODE_UP,
        KEYCODE_DOWN,
        KEYCODE_PRINT_SCREEN,
        KEYCODE_SCROLL_LOCK,
        KEYCODE_NUMPAD0,
        KEYCODE_NUMPAD1,
        KEYCODE_NUMPAD2,
        KEYCODE_NUMPAD3,
        KEYCODE_NUMPAD4,
        KEYCODE_NUMPAD5,
        KEYCODE_NUMPAD6,
        KEYCODE_NUMPAD7,
        KEYCODE_NUMPAD8,
        KEYCODE_NUMPAD9,
        KEYCODE_NUMLOCK,
        KEYCODE_DIVIDE,
        KEYCODE_MULTIPLY,
        KEYCODE_SUBTRACT,
        KEYCODE_ADDITION,
        KEYCODE_ENTER,
        KEYCODE_DECIMAL,
        KEYCODE_SHIFT,
        KEYCODE_LEFT_SHIFT,
        KEYCODE_RIGHT_SHIFT,
        KEYCODE_CONTROL,
        KEYCODE_LEFT_CONTROL,
        KEYCODE_RIGHT_CONTROL,
    };

    enum
    {
        KEYMASK_CONTROL = 0x0001,
        KEYMASK_SHIFT   = 0x0002,
        KEYMASK_SUPER   = 0x0004,
        KEYMASK_MENU    = 0x0008
    };

    enum MouseButton
    {
        MOUSEBUTTON_LEFT,
        MOUSEBUTTON_RIGHT,
        MOUSEBUTTON_MIDDLE,
        MOUSEBUTTON_X1,
        MOUSEBUTTON_X2,
        MOUSEBUTTON_WHEEL
    };

    // -----------------------------------------------------------------------
    // Event loop
    // -----------------------------------------------------------------------

    enum class FrameMode
    {
        OnDemand,   // onFrame only after invalidate()
        Continuous, // paced repeat for animations
    };

    enum class FrameTrigger
    {
        Invalidate,  // a redraw was requested via invalidate() (resize / expose / first frame)
        Timed,       // a requestFrameAt() / requestFrameIn() deadline elapsed
        Continuous,  // a paced repeat in FrameMode::Continuous
    };

    struct FrameInfo
    {
        // Seconds since the previous onFrame (0 on the first frame after enterEventLoop).
        double dt = 0.0;

        // Seconds since enterEventLoop() (monotonic, for animation phase).
        double time = 0.0;

        // Monotonic timestamp when this frame was scheduled (microseconds).
        u64 time_us = 0;

        // Why this frame was dispatched. Time-driven work (advancing an animation)
        // should key off FrameTrigger::Timed; FrameTrigger::Invalidate means "repaint
        // the current content" and must not advance the clock.
        FrameTrigger trigger = FrameTrigger::Invalidate;
    };

    struct EventLoopConfig
    {
        FrameMode mode = FrameMode::Continuous;

        // Maximum onFrame invocations per second (0 = no CPU-side cap).
        double maxFrameRate = 0.0;

        // Do not schedule another onFrame until the current frame is complete.
        // Sync rendering: completion is automatic when onFrame() returns.
        // Async rendering: call frameComplete() when present/GPU finishes (idempotent).
        bool waitForFrame = true;

        // Event poll sleep when idle (milliseconds).
        u32 pollTimeoutMs = 1;

        // Keep maxFrameRate matched to the window display refresh rate.
        bool trackDisplayRefreshRate = true;

        // CPU cap = getDisplayRefreshRate() * headroom when tracking is enabled.
        // Set above 1.0 so vsync/present stays the real limiter (nominal Hz is approximate).
        double displayRefreshHeadroom = 2.0;
    };

    struct EventLoopState
    {
        EventLoopConfig config;

        bool running = false;
        bool needs_redraw = false;
        bool frame_in_flight = false;

        u64 last_frame_time_us = 0;
        u64 loop_start_time_us = 0;
        double last_dt = 0.0;

        // One-shot timed wake (0 = none). When set, a frame is scheduled once the
        // monotonic clock reaches this deadline, in either FrameMode. Consumed on
        // dispatch; the client re-arms it (e.g. from onFrame) to keep an animation going.
        u64 next_frame_deadline_us = 0;

        // Sentinel for computeWaitTimeoutMs(): nothing is pending, so the platform
        // loop may block until an OS event arrives (subject to a platform safety cap).
        static constexpr u32 WAIT_INFINITE = 0xffffffffu;

        void reset(const EventLoopConfig& loopConfig);
        void invalidate();
        bool shouldScheduleFrame(u64 now_us) const;
        bool consumeInvalidated();
        double computeDt(u64 now_us);

        // Maximum time (ms) the platform loop may block before the next
        // dispatchFrame() opportunity. 0 = a frame is already due (don't block);
        // WAIT_INFINITE = idle, block until an event. Lets the loop sleep on the OS
        // event queue instead of busy-polling, so an idle window costs ~0% CPU.
        u32 computeWaitTimeoutMs(u64 now_us) const;
    };

    // -----------------------------------------------------------------------
    // Window
    // -----------------------------------------------------------------------

    class Window : public NonCopyable
    {
    protected:
        std::unique_ptr<struct WindowContext> m_window_context;
        EventLoopState m_event_loop;

        void waitForNextIteration();

        virtual void runEventLoop();

        // Unblock a loop that is sleeping on the OS event queue. Platform-specific:
        // posts a no-op event so a state change (invalidate / requestFrame /
        // breakEventLoop) is observed immediately, even from another thread. Lets the
        // idle wait block indefinitely instead of polling on a timeout.
        void wakeEventLoop();

    public:
        enum : u32
        {
            DISABLE_RESIZE  = 0x00000001,
            API_OPENGL      = 0x00010000,
            API_EGL         = 0x00020000,
            API_VULKAN      = 0x00040000,
        };

        Window(int width, int height, u32 flags = 0);
        virtual ~Window();

        static int getScreenCount();
        static math::int32x2 getScreenSize(int screen = 0);

        operator WindowHandle () const;
        operator struct WindowContext* () const;

        void setWindowPosition(int x, int y);
        void setWindowSize(int width, int height);
        void setTitle(const std::string& title);
        void setVisible(bool enable);

        virtual math::int32x2 getWindowSize() const;
        virtual math::int32x2 getCursorPosition() const;

        virtual double getDisplayRefreshRate() const;

        virtual void toggleFullscreen() = 0;
        virtual bool isFullscreen() const = 0;

        bool isKeyPressed(Keycode code) const;

        void enterEventLoop(const EventLoopConfig& config = {});
        virtual void breakEventLoop();
        void requestQuit();

        bool isRunning() const;
        void invalidate();
        void requestFrameAt(u64 time_us);  // schedule one frame at an absolute monotonic time (microseconds)
        void requestFrameIn(double seconds); // schedule one frame after a delay from now
        void dispatchFrame();
        void frameComplete();

        void syncDisplayRefreshRate();

        const EventLoopConfig& getEventLoopConfig() const;
        void setEventLoopConfig(const EventLoopConfig& config);
        void setFrameMode(FrameMode mode);
        void setMaxFrameRate(double frameRate);

        virtual void onFrame(const FrameInfo& info);
        virtual void onResize(int width, int height);
        virtual void onMinimize();
        virtual void onMaximize();
        virtual void onKeyPress(Keycode code, u32 mask);
        virtual void onKeyRelease(Keycode code);
        virtual void onMouseMove(int x, int y);
        virtual void onMouseClick(int x, int y, MouseButton button, int count);
        virtual void onDropFiles(const filesystem::FileIndex& index);
        virtual void onClose();
        virtual void onShow();
        virtual void onHide();
    };

} // namespace mango

#endif // !defined(MANGO_WINDOW_SYSTEM_NONE)
