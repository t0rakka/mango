/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>
#include <mango/window/window.hpp>

namespace mango
{

    namespace window_peers
    {
        bool isEventLoopPump(const Window* window);
    }

    // -----------------------------------------------------------------------
    // EventLoop
    // -----------------------------------------------------------------------

    class EventLoop : private NonCopyable
    {
    public:
        EventLoop();
        ~EventLoop();

        void attach(Window& window, const EventLoopConfig& config = {});
        void detach(Window& window);
        void clear();

        // Blocks until quit() is called. The first attached window drives the native pump.
        void run();

        void quit();
        bool isRunning() const;

        // Used by platform backends while run() is active.
        void dispatchFrames();
        u32 computeWaitTimeoutMs(u64 now_us) const;

    private:
        friend class Window;
        friend bool window_peers::isEventLoopPump(const Window* window);

        std::vector<Window*> m_windows;
        Window* m_pump = nullptr;
        bool m_running = false;
    };

} // namespace mango
