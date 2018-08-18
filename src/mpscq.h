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

#ifndef IO_MPSCQ_H_INCLUDED
#define IO_MPSCQ_H_INCLUDED

#include "platform.h"
#if PLATFORM_WINDOWS
#   include <WinBase.h> // InterlockedExchangePointerAcquire
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Multiple producer single consumer lock-free queue.
 * Based on: http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
 */

typedef struct mpscq_node_t {
    struct mpscq_node_t* volatile next;
} mpscq_node_t;

typedef struct mpscq_t {
    mpscq_node_t* volatile  head;
    mpscq_node_t*           tail;
    mpscq_node_t            stub;
} mpscq_t;

static void mpscq_init(mpscq_t* queue)
{
    queue->head = &queue->stub;
    queue->tail = &queue->stub;
    queue->stub.next = 0;
}

static void mpscq_push(mpscq_t* queue, mpscq_node_t* node)
{
    mpscq_node_t* prev;

    node->next = 0;
#if PLATFORM_WINDOWS
    prev = InterlockedExchangePointerAcquire(&queue->head, node);
#else
    prev = __sync_lock_test_and_set(&queue->head, node);
#endif
    prev->next = node;
}

static mpscq_node_t* mpscq_pop(mpscq_t* queue)
{
    mpscq_node_t* tail = queue->tail;
    mpscq_node_t* next = tail->next;

    if (tail == &queue->stub)
    {
        if (0 == next)
            return 0;

        queue->tail = next;
        tail = next;
        next = next->next;
    }

    if (next)
    {
        queue->tail = next;
        return tail;
    }

    mpscq_node_t* head = queue->head;
    if (tail != head)
        return 0;

    mpscq_push(queue, &queue->stub);
    next = tail->next;
    if (next)
    {
        queue->tail = next;
        return tail;
    }

    return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_MPSCQ_H_INCLUDED