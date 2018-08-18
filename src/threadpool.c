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

#include <memory.h>
#include "io.h"
#include "atomic.h"
#include "threadpool.h"
#include "thread.h"

#if PLATFORM_WINDOWS
#   define  WIN32_LEAN_AND_MEAN 1
#   include <Windows.h>
#else
#   include <pthread.h>
#endif

#define NUM_THREADS 4

typedef struct io_threadpool_t {
    LIST_OF(io_work_t);
    atomic64_t shutdown;
    uint64_t idle_threads;
    io_condition_t condition;
    io_mutex_t mutex;
} io_threadpool_t;

static io_threadpool_t threadpool;

IO_THREAD_TYPE io_threadpool_worker(void* arg)
{
    io_work_t* work;

    while (1)
    {
        if (atomic_load64(&threadpool.shutdown))
        {
            break;
        }

        io_mutex_lock(&threadpool.mutex);

        while (threadpool.head == 0) {
            threadpool.idle_threads += 1;
            io_condition_wait(&threadpool.condition, &threadpool.mutex);
            threadpool.idle_threads -= 1;
        }

        work = LIST_HEAD((&threadpool));
        LIST_POP_HEAD((&threadpool));

        io_mutex_unlock(&threadpool.mutex);

        if (atomic_load64(&threadpool.shutdown))
            break;

        work->entry(work);
    }

	return 0;
}

int io_threadpool_init()
{
    int i;
    memset(&threadpool, 0, sizeof(threadpool));

    io_condition_init(&threadpool.condition);
    io_mutex_init(&threadpool.mutex);

    for (i = 0; i < NUM_THREADS; ++i)
    {
        io_thread_create(io_threadpool_worker, 0);
    }

    return 0;
}

int io_threadpool_shutdown()
{
    atomic_store64(&threadpool.shutdown, 1);

    // ToDo: Wait for threads

    io_condition_destroy(&threadpool.condition);
    io_mutex_destroy(&threadpool.mutex);

    return 1;
}

int io_threadpool_post(io_work_t* work)
{
    work->loop = io_loop_current();
    work->task = work->loop->current;

    io_mutex_lock(&threadpool.mutex);

    LIST_PUSH_TAIL((&threadpool), work)

    if (threadpool.idle_threads > 0)
        io_condition_signal(&threadpool.condition);

    io_mutex_unlock(&threadpool.mutex);

    return 1;
}
