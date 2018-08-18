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

#ifndef IO_STREAM_H_INCLUDED
#define IO_STREAM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <memory.h> // memcpy
#include "io.h"
#include "loop.h"
#include "list.h"

typedef struct io_memory_chunk_t {
    LIST_NODE_OF(io_memory_chunk_t);
} io_memory_chunk_t;

typedef struct io_stream_t {
    struct {
#if PLATFORM_WINDOWS
        void(*processor)(struct io_stream_t* stream, DWORD transferred,
                 OVERLAPPED* overlapped, io_loop_t* loop, DWORD error);
        OVERLAPPED read;
        OVERLAPPED write;
#elif PLATFORM_LINUX
        void(*processor)(struct io_stream_t* stream, int events);
        struct epoll_event e;
#else
#   error Not implemented
#endif
        void* read_req;
        void* write_req;
    } platform;
    int fd;
    io_stream_info_t info;
    io_loop_t* loop;
    io_filter_t operations;
    struct {
    io_filter_t* head;
    io_filter_t* tail;
    } filters;

    union {
        struct {
            uint64_t read_offset;
            uint64_t write_offset;
        } file;
        struct {
            LIST_OF(io_memory_chunk_t);
            uint64_t bucket_size;
            uint64_t length;
            uint64_t read_offset;
            uint64_t write_offset;
        } memory;
    } impl;

    struct {
        char* buffer;
        size_t offset;
        size_t length;
    } unread;
} io_stream_t;

static size_t io_stream_read_exact(io_stream_t* stream, char* buffer, size_t length)
{
    size_t offset = 0;
    size_t nread = 0;

    while (offset < length)
    {
        nread = io_stream_read(stream, buffer + offset, length - offset, 0);
        offset += nread;

        if (nread == 0)
            break;
    }

    return offset;
}

void io_stream_init(io_stream_t* stream);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_STREAM_H_INCLUDED