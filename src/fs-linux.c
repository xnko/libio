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

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "fs.h"
#include "threadpool.h"
#include "task.h"
#include "memory.h"
#include "loop-linux.h"

typedef struct io_file_req_t {
    const char* path;
    io_path_info_t* info;
    io_file_options_t options;
    int fd;
    int error;
} io_file_req_t;

void io_path_info_get_internal(io_work_t* work)
{
    io_file_req_t* req = (io_file_req_t*)work->arg;
    io_path_info_t* info = req->info;

    struct stat s;
    stat(req->path, &s);

    info->time_access = s.st_atime;
    info->time_create = s.st_ctime;
    info->time_modified = s.st_mtime;
    info->size = s.st_size;

    req->error = 0;

    io_loop_post_task(work->loop, work->task);
}

void io_file_open_internal(io_work_t* work)
{
    io_file_req_t* req = (io_file_req_t*)work->arg;
    int options = O_NONBLOCK;
    int mode = 0;

    if (req->options && IO_FILE_CREATE)
    {
        options |= (O_CREAT | O_WRONLY);
        mode = 0666;
    }
    else
    {
        options |= (O_RDWR);
    }

    if (req->options && IO_FILE_APPEND)
    {
        options |= O_APPEND;
    }

    if (req->options && IO_FILE_TRUNCATE)
    {
        options |= O_TRUNC;
    }

    req->fd = open(req->path, options, mode);
    if (req->fd < 0)
    {
        req->error = errno;
    }

    io_loop_post_task(work->loop, work->task);
}

void io_file_close_internal(io_work_t* work)
{
    io_file_req_t* req = (io_file_req_t*)work->arg;

    req->error = io_close(req->fd);

    io_loop_post_task(work->loop, work->task);
}

/*
 * Internal API
 */

int io_file_close(io_stream_t* stream)
{
    io_work_t work;
    io_file_req_t close;

    close.fd = stream->fd;
    close.error = 0;

    work.arg = &close;
    work.entry = io_file_close_internal;

    io_threadpool_post(&work);
    task_suspend(work.task);

    return close.error;
}

/*
 * Public API
 */

int io_path_info_get(const char* path, io_path_info_t* info)
{
    io_work_t work;
    io_file_req_t req;

    req.path = path;
    req.info = info;
    req.error = 0;

    work.arg = &req;
    work.entry = io_path_info_get_internal;

    io_threadpool_post(&work);
    task_suspend(work.task);

    return req.error;
}

int io_file_open(io_stream_t** stream, const char* path, io_file_options_t options)
{
    io_work_t work;
    io_file_req_t open;

    open.path = path;
    open.options = options;
    open.error = 0;

    work.arg = &open;
    work.entry = io_file_open_internal;

    io_threadpool_post(&work);
    task_suspend(work.task);

    if (open.error)
    {
        return open.error;
    }

    *stream = io_calloc(1, sizeof(io_stream_t));
    if (*stream == 0)
    {
        return ENOMEM;
    }

    (*stream)->fd = open.fd;
    (*stream)->info.type = IO_STREAM_FILE;

    io_stream_init(*stream);

    return 0;
}