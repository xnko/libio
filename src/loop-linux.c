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

#include <fcntl.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <memory.h>
#include "platform.h"
#include "memory.h"
#include "mpscq.h"
#include "task.h"
#include "event.h"
#include "time.h"
#include "loop-linux.h"
#include "thread.h"

DECLARE_THREAD_LOCAL(io_loop_t*, loop, 0);

typedef struct io_handle_t {
    void(*processor)(struct io_handle_t* handle, int events);
} io_handle_t;

int io_loop_init(io_loop_t* loop)
{
    int error;

    // Reset
    memset(loop, 0, sizeof (*loop));

    // Init task scheduler
    loop->current = &loop->main;
    loop->main.loop = loop;
    loop->prev = 0;

    // Init loop
    loop->epoll = epoll_create1(EPOLL_CLOEXEC);
    if (loop->epoll == -1)
    {
        return errno;
    }

    // Init wakeup
    loop->wakeup.fd = eventfd(0, EFD_NONBLOCK);
    if (loop->wakeup.fd == -1)
    {
        error = errno;
        io_close(loop->epoll);
        return error;
    }

    loop->wakeup.event.data.ptr = &loop->wakeup;
    loop->wakeup.event.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLIN | EPOLLPRI;
    error = epoll_ctl(loop->epoll, EPOLL_CTL_ADD, loop->wakeup.fd, &loop->wakeup.event);
    error = io_loop_read_add(loop, loop->wakeup.fd, &loop->wakeup.event);
    if (error)
    {
        io_close(loop->wakeup.fd);
        io_close(loop->epoll);
        return error;
    }

    mpscq_init(&loop->tasks);
    // mpscq_init(&loop->waiters);

    return 0; 
}

int io_loop_cleanup(io_loop_t* loop)
{
    return 0;
}

int io_loop_wakeup(io_loop_t* loop)
{
    uint64_t one = 1;
    ssize_t wrote = write(loop->wakeup.fd, &one, sizeof (one));

    if (wrote != sizeof (uint64_t))
    {
        return errno;
    }

    return 0;
}

io_loop_t* io_loop_current()
{
    return get_thread_loop();
}

int io_loop_run(io_loop_t* loop)
{
#define IO_MAX_EVENTS 60

    struct epoll_event events[IO_MAX_EVENTS];
    task_t* task;
    io_handle_t* handle;
    eventfd_t value;
    uint64_t now;
    uint64_t nearest_event_time;
    int timeout;
    int error;
    int n, i;

    // Prepare
    memset(events, 0, sizeof(struct epoll_event) * IO_MAX_EVENTS);

    set_thread_loop(loop);

    // Execute entry if provided
    if (loop->entry)
    {
        io_loop_post(loop, loop->entry, loop->arg);
    }

    now = time_current();
    loop->last_activity_time = now;

    // Event loop
    do
    {
        if (0 < moments_tick(&loop->sleeps, now))
        {
            now = time_current();
            loop->last_activity_time = now;
        }

        nearest_event_time = io_loop_nearest_event_time(loop);
        if (nearest_event_time == 0)
            timeout = -1;
        else
            timeout = (int)nearest_event_time;

        if (atomic_load64(&loop->shutdown))
        {
            break;
        }

        n = epoll_wait(loop->epoll, events, IO_MAX_EVENTS, timeout);

        now = time_current();

        if (n == -1)
        {
            if (errno == EBADF || errno == EINVAL)
                break;

            // EINTR
            continue;
        }

        if (n > 0)
        {
            for (i = 0; i < n; ++i)
            {
                if (events[i].data.ptr == &loop->wakeup)
                {
                    // reset state
                    if (0 != eventfd_read(loop->wakeup.fd, &value))
                    {
                        error = errno;
                    }
                }
                else
                {
                    handle = (io_handle_t*)events[i].data.ptr;
                    handle->processor(handle, events[i].events);   
                }

                now = time_current();
                loop->last_activity_time = now;
            }

            io_loop_process_tasks(loop);

            now = time_current();
            loop->last_activity_time = now;
        }
        else // wait timeout
        {
            if (0 < moments_tick(&loop->idles, now))
            {
                now = time_current();
                loop->last_activity_time = now;
            }
        }

        moments_tick(&loop->timeouts, now);
    }
    while (1);

    // Unref
    io_loop_unref(loop);

    set_thread_loop(0);
    return 0;
}