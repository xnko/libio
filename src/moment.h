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

#ifndef IO_MOMENT_H_INCLUDED
#define IO_MOMENT_H_INCLUDED

#include <stdint.h> // uint64_t
#include "rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct moment_t {
    struct rbnode_t node;
    struct task_t* task;
    uint64_t time;
    unsigned reached : 1;
    unsigned removed : 1;
    unsigned shutdown : 1;
} moment_t;

typedef struct moments_t {
    struct rbnode_t* root;
} moments_t;

void moments_add(moments_t* moments, moment_t* moment);
void moments_remove(moments_t* moments, moment_t* moment);
void moments_shutdown(moments_t* moments);
uint64_t moments_tick(moments_t* moments, uint64_t now);
uint64_t moments_nearest(moments_t* moments);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_MOMENT_H_INCLUDED