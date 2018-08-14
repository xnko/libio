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

#ifndef IO_POOL_H_INCLUDED
#define IO_POOL_H_INCLUDED

#include <stdint.h> // uint64_t
#include <stddef.h> // size_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct io_pool_t {
    uint64_t last_usage;
    uint64_t last_time;
    uint64_t interval;
    uint64_t current_usage;
    uint64_t used;
    uint64_t free;
    uint64_t block_size;
    void* free_list;
} io_pool_t;

void  io_pool_init(io_pool_t* pool);
void  io_pool_cleanup(io_pool_t* pool);
void* io_pool_alloc(io_pool_t* pool, size_t size);
void  io_pool_free(io_pool_t* pool, void* prt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_POOL_H_INCLUDED