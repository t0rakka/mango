/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace mango
{

    // Non-blocking self-pipe so a loop blocked in poll() wakes on cross-thread
    // invalidate / requestFrame / frameComplete / quit.
    struct LinuxEventWake
    {
        int fds[2] = { -1, -1 };

        void create()
        {
            if (fds[0] >= 0)
            {
                return;
            }

#if defined(__linux__)
            if (::pipe2(fds, O_CLOEXEC | O_NONBLOCK) == 0)
            {
                return;
            }
#endif
            if (::pipe(fds) != 0)
            {
                fds[0] = fds[1] = -1;
                return;
            }

#if defined(F_SETFD)
            ::fcntl(fds[0], F_SETFD, FD_CLOEXEC);
            ::fcntl(fds[1], F_SETFD, FD_CLOEXEC);
#endif
#if defined(O_NONBLOCK)
            ::fcntl(fds[0], F_SETFL, O_NONBLOCK);
            ::fcntl(fds[1], F_SETFL, O_NONBLOCK);
#endif
        }

        void destroy()
        {
            if (fds[0] >= 0)
            {
                ::close(fds[0]);
                fds[0] = -1;
            }
            if (fds[1] >= 0)
            {
                ::close(fds[1]);
                fds[1] = -1;
            }
        }

        void signal()
        {
            if (fds[1] < 0)
            {
                return;
            }

            const char byte = 1;
            for (;;)
            {
                const ssize_t n = ::write(fds[1], &byte, 1);
                if (n > 0 || (n < 0 && errno != EINTR))
                {
                    break;
                }
            }
        }

        void drain()
        {
            if (fds[0] < 0)
            {
                return;
            }

            char buf[64];
            for (;;)
            {
                const ssize_t n = ::read(fds[0], buf, sizeof(buf));
                if (n <= 0)
                {
                    break;
                }
            }
        }

        int readFd() const
        {
            return fds[0];
        }
    };

} // namespace mango
