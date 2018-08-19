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

#include <errno.h>
#include "loop.h"
#include "atomic.h"
#include "memory.h"
#include "task.h"
#include "time.h"
#include "thread.h"
#include "threadpool.h"
#include "event.h"
#include "tcp.h"

typedef struct io_exec_t {
    io_loop_t* dest;
    io_loop_t* loop;
    task_t* task;
    io_loop_fn entry;
    void* arg;
} io_exec_t;

static void io_loop_exec_entry(io_loop_t* loop, void* arg)
{
    io_exec_t* exec = (io_exec_t*)arg;

    exec->entry(exec->dest, exec->arg);

    io_loop_post_task(exec->loop, exec->task);
}


task_t* io_loop_fetch_next_task(io_loop_t* loop)
{
    mpscq_node_t* node;
    task_t* task;
    
    node = mpscq_pop(&loop->tasks);
    task = container_of(node, task_t, node);

    return task;
}

void io_loop_process_tasks(io_loop_t* loop)
{
    task_t* task;
    int error;

    task = io_loop_fetch_next_task(loop);
    while (task != 0)
    {
        if (task->loop == loop)
        {
            // Wakeup
            task_resume(task);
        }
        else
        {
            // Post
            error = task_post(task, loop);
            if (error)
            {
                /* handle error */
            }
        }        
        
        task = io_loop_fetch_next_task(loop);
    }
}

IO_THREAD_TYPE io_thread_entry(void* arg)
{
    io_loop_t* loop = (io_loop_t*)arg;

    io_loop_run(loop);

    return 0;
}

/*
 * Public API
 */

int io_loop_start(io_loop_t** loop)
{
    int error = 0;

    *loop = 0;

    *loop = (io_loop_t*)io_malloc(sizeof (io_loop_t));
    if (*loop == 0)
    {
        return errno;
    }

    error = io_loop_init(*loop);
    if (error)
    {
        io_free(*loop);
        *loop = 0;
        return error;
    }

    // Keep reference
    io_loop_ref(*loop);

    error = io_thread_create(io_thread_entry, *loop);
    if (error)
    {
        io_loop_cleanup(*loop);
        io_free(*loop);

        *loop = 0;
    }

    return error;
}

int io_loop_stop(io_loop_t* loop)
{
    io_loop_t* current = io_loop_current();

    int shutdown = atomic_cas64(&loop->shutdown, 0, 1);
    if (shutdown && current != loop)
    {
        io_loop_wakeup(loop);
    }

    return 1;
}

int io_loop_ref(io_loop_t* loop)
{
    atomic_incr64(&loop->refs);

    return 0;
}

int io_loop_unref(io_loop_t* loop)
{
    uint64_t refs = atomic_decr64(&loop->refs);
    if (refs == 0)
    {
        // ToDo:
        io_loop_cleanup(loop);
        io_free(loop);
    }

    return 0;
}

int io_loop_post(io_loop_t* loop, io_loop_fn entry, void* arg)
{
    task_t* task;

    io_loop_t* current = io_loop_current();
    int error = task_create(&task, entry, arg);

    if (error)
    {
        return error;
    }

    // always async post 
    //if (current == loop)
    //{
    //    error = task_post(task, current);
    //}
    //else
    {
        mpscq_push(&loop->tasks, &task->node);
        error = io_loop_wakeup(loop);
    }

    if (error)
    {
        task_delete(task);
    }

    return error;
}

int io_loop_exec(io_loop_t* loop, io_loop_fn entry, void* arg)
{
    int error;
    io_exec_t exec;
    io_loop_t* current = io_loop_current();

    if (current == loop)
    {
        entry(current, arg);
        return 0;
    }

    exec.dest = loop;
    exec.loop = current;
    exec.task = current->current;
    exec.entry = entry;
    exec.arg = arg;

    error = io_loop_post(loop, io_loop_exec_entry, &exec);
    if (error)
    {
        return error;
    }

    task_suspend(current->current);
    return 0;
}

int io_loop_sleep(uint64_t milliseconds)
{
    moment_t moment;
    uint64_t now = time_current();
    io_loop_t* current = io_loop_current();

    memset(&moment, 0, sizeof (moment));

    moment.task = current->current;
    moment.time = now + milliseconds;

    moments_add(&current->sleeps, &moment);
    task_suspend(current->current);

    if (moment.removed)
    {
        return ECANCELED;
    }

    if (moment.shutdown)
    {
        return ECANCELED;
    }
    
    // Reached
    return 0;
}

int io_loop_idle(io_loop_t* loop, uint64_t milliseconds)
{
    moment_t moment;
    uint64_t now = time_current();
    io_loop_t* current = io_loop_current();

    memset(&moment, 0, sizeof (moment));

    moment.task = current->current;
    moment.time = now + milliseconds;

    moments_add(&current->idles, &moment);
    task_suspend(current->current);

    if (moment.removed)
    {
        return ECANCELED;
    }

    if (moment.shutdown)
    {
        return ECANCELED;
    }

    // Reached
    return 0;
}