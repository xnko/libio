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

#ifndef IO_TASK_H_INCLUDED
#define IO_TASK_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "loop.h"

int task_create(task_t** task, io_loop_fn entry, void* arg);
int task_delete(task_t* task);
int task_yield(task_t* current, void* value); // may return is_cancelled
int task_exec(task_t* task, io_loop_t* loop, void** yield);
int task_post(task_t* task, io_loop_t* loop);
int task_next(task_t* task, io_loop_t* loop, void** yield);
int task_suspend(task_t* current);
int task_resume(task_t* task);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_TASK_H_INCLUDED