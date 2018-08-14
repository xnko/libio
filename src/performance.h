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

#ifndef IO_PERFORMANCE_H_INCLUDED
#define IO_PERFORMANCE_H_INCLUDED

#include <stdint.h> // uint64_t
#include "atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct counter_t {
    atomic64_t count;
    atomic64_t nanoseconds;
} counter_t;

typedef struct performance_t {
    counter_t memory_alloc;
    counter_t memory_free;
    counter_t task_contextswitch;
    counter_t loop_crosspost;
    counter_t loop_idle;
    counter_t file_open;
    counter_t file_create;
    counter_t file_read;
    counter_t file_write;
    counter_t tcp_read;
    counter_t tcp_write;
    counter_t time_current;
    counter_t threadpool_post;
    counter_t event_watch;
    counter_t event_notify;
} performance_t;

static performance_t performance = {0};

static void counter_increment(counter_t* counter, uint64_t nanoseconds)
{
    atomic_incr64(&counter->count);
    atomic_add64(&counter->nanoseconds, nanoseconds);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_PERFORMANCE_H_INCLUDED