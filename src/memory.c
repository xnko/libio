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
#include <malloc.h>
#include "../include/io.h"
#include "memory.h"

/*
 * Implementation
 */

static void* io_default_malloc(size_t size)
{
    void* ptr = malloc(size);

    if (ptr == 0)
        errno = ENOMEM;

    return ptr;
}

static void* io_default_calloc(size_t count, size_t size)
{
    void* ptr = calloc(count, size);

    if (ptr == 0)
        errno = ENOMEM;

    return ptr;
}

static void* io_default_realloc(void* ptr, size_t size)
{
    void* new_ptr = realloc(ptr, size);

    if (new_ptr == 0)
        errno = ENOMEM;

    return new_ptr;
}

static void io_default_free(void* ptr)
{
    if (ptr != 0)
        free(ptr);
}

static void* (*io_malloc_fn)(size_t size) = io_default_malloc;
static void* (*io_calloc_fn)(size_t count, size_t size) = io_default_calloc;
static void* (*io_realloc_fn)(void* ptr, size_t size) = io_default_realloc;
static void  (*io_free_fn)(void* ptr) = io_default_free;

/*
 *  Internal API
 */

void* io_malloc(size_t size)
{
    return io_malloc_fn(size);
}

void* io_calloc(size_t count, size_t size)
{
    return io_calloc_fn(count, size);
}

void* io_realloc(void* ptr, size_t size)
{
    return io_realloc_fn(ptr, size);
}

void io_free(void* ptr)
{
    io_free_fn(ptr);
}

/*
 *  Public API
 */

void io_set_allocator(
    void* (*malloc_fn)(size_t size),
    void* (*calloc_fn)(size_t count, size_t size),
    void* (*realloc_fn)(void* ptr, size_t size),
    void  (*free_fn)(void* ptr))
{
    io_malloc_fn = malloc_fn;
    io_calloc_fn = calloc_fn;
    io_realloc_fn = realloc_fn;
    io_free_fn = free_fn;
}