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

    // Process-wide live WindowBackend list. The window that called enterEventLoop()
    // drains its own connection and then every other backend so secondary windows
    // receive input/close while sharing one thread / one pump.
    namespace window_peers
    {

        void registerBackend(WindowBackend* backend);
        void unregisterBackend(WindowBackend* backend);

        void forEach(void (*fn)(WindowBackend* backend, void* user), void* user);
        void forEachOther(WindowBackend* self, void (*fn)(WindowBackend* backend, void* user), void* user);

        void drainOtherBackends(WindowBackend* self);

        void setEventLoopOwner(Window* window);
        Window* eventLoopOwner();
        bool isEventLoopOwner(const Window* window);

    } // namespace window_peers

} // namespace mango
