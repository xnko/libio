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

#ifndef IO_THREADPOOL_H_INCLUDED
#define IO_THREADPOOL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#include "loop.h"

typedef struct io_work_t io_work_t;

typedef void (*io_work_fn)(struct io_work_t* work);

typedef struct io_work_t {
    LIST_NODE_OF(io_work_t);
    io_work_fn entry;
    io_loop_t* loop;
    task_t* task;
    void* arg;
} io_work_t;

int io_threadpool_init();
int io_threadpool_shutdown();
int io_threadpool_post(io_work_t* work);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_THREADPOOL_H_INCLUDED