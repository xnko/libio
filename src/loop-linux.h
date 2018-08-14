/* Copyright (c) 2018, Artak Khnkoyan <artak.khnkoyan@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef IO_LOOP_LINUX_H_INCLUDED
#define IO_LOOP_LINUX_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include "loop.h"

#define IO_READ    1
#define IO_WRITE   2

static int io_loop_update(io_loop_t* loop, int fd, struct epoll_event* e, int events)
{
    int error = 0;
    e->events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    if ((events & IO_READ) == IO_READ)
        e->events |= (EPOLLIN | EPOLLPRI);

    if ((events & IO_WRITE) == IO_WRITE)
        e->events |= EPOLLOUT;

    if (epoll_ctl(loop->epoll, EPOLL_CTL_MOD, fd, e) == -1)
    {
        error = errno;
    }

    return error;
}

static int io_loop_read_add(io_loop_t* loop, int fd, struct epoll_event* e)
{
    if (e->events & EPOLLOUT)
        return io_loop_update(loop, fd, e, IO_READ | IO_WRITE);
    else
        return io_loop_update(loop, fd, e, IO_READ);
}

static int io_loop_read_del(io_loop_t* loop, int fd, struct epoll_event* e)
{
    if (e->events & EPOLLOUT)
        return io_loop_update(loop, fd, e, IO_WRITE);
    else
        return io_loop_update(loop, fd, e, 0);
}

static int io_loop_write_add(io_loop_t* loop, int fd, struct epoll_event* e)
{
    if (e->events & EPOLLIN)
        return io_loop_update(loop, fd, e, IO_READ | IO_WRITE);
    else
        return io_loop_update(loop, fd, e, IO_WRITE);
}

static int io_loop_write_del(io_loop_t* loop, int fd, struct epoll_event* e)
{
    if (e->events & EPOLLIN)
        return io_loop_update(loop, fd, e, IO_READ);
    else
        return io_loop_update(loop, fd, e, 0);
}

static int io_close(int fd)
{
    while (0 != close(fd))
    {
        if (errno != EINTR)
        {
            return errno;
        }
    }

    return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_LOOP_LINUX_H_INCLUDED