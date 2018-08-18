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

#ifndef IO_LOOP_H_INCLUDED
#define IO_LOOP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "io.h"
#include "mpscq.h"
#include "moment.h"
#include "platform.h"
#include "atomic.h"

#if PLATFORM_WINDOWS
#   define  WIN32_LEAN_AND_MEAN 1
#   include <Windows.h>
#else
#   include <sys/epoll.h>
#endif

#include "task/context.h"
typedef ucontext_t context_t;

typedef struct task_t {
    mpscq_node_t node;
    context_t   context;
    io_loop_t*  loop;
    struct task_t* parent;
    char*       stack;
    size_t      stack_size;
    io_loop_fn  entry;
    void*       arg;
    int         is_done;
    int         is_post;
    int         inherit_error_state;
} task_t;

typedef struct io_loop_t {
    atomic64_t refs;
    atomic64_t shutdown;
    uint64_t last_activity_time;

    // Task scheduler
    task_t* current;
    task_t* prev;
    task_t  main;
    void* value; // last yield value
    
    // Timeouts
    moments_t sleeps;
    moments_t idles;
    moments_t timeouts;

    // Entry
    io_loop_fn entry;
    void* arg;

    mpscq_t tasks;

    // Platform specific
#if PLATFORM_WINDOWS
    HANDLE iocp;
#elif PLATFORM_LINUX
    int epoll;
    struct {
        int fd;
        struct epoll_event event;
    } wakeup;

#else
#   error Not implemented
#endif

} io_loop_t;

static uint64_t io_loop_nearest_event_time(io_loop_t* loop)
{
    uint64_t nearest_time = 0;
    uint64_t nearest_sleep = moments_nearest(&loop->sleeps);
    uint64_t nearest_idle = moments_nearest(&loop->idles);
    uint64_t nearest_timeout = moments_nearest(&loop->timeouts);

    if (0 < nearest_sleep && nearest_sleep < nearest_time)
        nearest_time = nearest_sleep;

    if (0 < nearest_idle && nearest_idle < nearest_time)
        nearest_time = nearest_idle;

    if (0 < nearest_timeout && nearest_timeout < nearest_time)
        nearest_time = nearest_timeout;

    return nearest_time;
}

/*
 * Internal API
 */

int io_loop_init(io_loop_t* loop);
int io_loop_cleanup(io_loop_t* loop);
int io_loop_wakeup(io_loop_t* loop);
io_loop_t* io_loop_current();

static int io_loop_post_task(io_loop_t* loop, task_t* task)
{
    mpscq_push(&loop->tasks, &task->node);
    return io_loop_wakeup(loop);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_LOOP_H_INCLUDED