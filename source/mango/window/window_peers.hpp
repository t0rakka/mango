/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <vector>

namespace mango
{

    struct WindowBackend;
    class Window;
    class EventLoop;

    // Process-wide live WindowBackend list. The active EventLoop drains peer
    // backends so secondary windows receive input/close while sharing one thread.
    namespace window_peers
    {

        void registerBackend(WindowBackend* backend);
        void unregisterBackend(WindowBackend* backend);

        void forEach(void (*fn)(WindowBackend* backend, void* user), void* user);
        void forEachOther(WindowBackend* self, void (*fn)(WindowBackend* backend, void* user), void* user);

        void drainOtherBackends(WindowBackend* self);

        void setActiveEventLoop(EventLoop* loop);
        EventLoop* activeEventLoop();

        // True when window is the native event pump driver (first attach).
        bool isEventLoopPump(const Window* window);

    } // namespace window_peers

} // namespace mango
