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

#include <aio.h>
#include "memory.h"
#include "stream.h"
#include "time.h"
#include "moment.h"
#include "stopwatch.h"
#include "task.h"
#include "loop-linux.h"

typedef struct io_tcp_read_req_t {
    char* buffer;
    uint64_t length;
    uint64_t done;
    task_t* task;   
} io_tcp_read_req_t;

typedef struct io_tcp_write_req_t {
    const char* buffer;
    uint64_t length;
    uint64_t offset;
    task_t* task;
} io_tcp_write_req_t;

typedef struct io_file_read_req_t {
    struct aiocb aio;
    uint64_t done;
    task_t* task;
    io_loop_t* loop;
    int error;
} io_file_read_req_t;

typedef struct io_file_write_req_t {
    struct aiocb aio;
    uint64_t done;
    task_t* task;
    io_loop_t* loop;
    int error;
} io_file_write_req_t;

void io_stream_read_try(io_stream_t* stream)
{
    io_tcp_read_req_t* data = (io_tcp_read_req_t*)stream->platform.read_req;
    ssize_t n;

    n = read(stream->fd, data->buffer, data->length);
    if (n == -1)
    {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            stream->info.status.error = errno;
            stream->filters.head->on_status(stream->filters.head);
        }
    }
    else if (n == 0)
    {
        stream->info.status.eof = 1;
        stream->filters.head->on_status(stream->filters.head);
    }
    else
    {
        data->done = n;
    }
}

void io_stream_write_try(io_stream_t* stream)
{
    io_tcp_write_req_t* data = (io_tcp_write_req_t*)stream->platform.write_req;
    ssize_t n;

    n = write(stream->fd, data->buffer + data->offset, data->length - data->offset);
    if (n > 0)
    {
        data->offset += n;
    }
    else
    {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            stream->info.status.error = errno;
            stream->filters.head->on_status(stream->filters.head);
        }
    }
}

static size_t io_stream_tcp_on_read(io_filter_t* filter, char* buffer, size_t length)
{
    io_stream_t* stream = filter->stream;
    io_tcp_read_req_t read;
    moment_t timeout;
    uint64_t start, end, elapsed;
    uint64_t timeout_value = stream->info.read.timeout;

    if (length == 0)
    {
        return length;
    }

    if (stream->info.status.read_timeout ||
        stream->info.status.eof ||
        stream->info.status.error ||
        stream->info.status.closed ||
        stream->info.status.peer_closed ||
        stream->info.status.shutdown)
    {
        return 0;
    }

    if (atomic_load64(&stream->loop->shutdown))
    {
        stream->info.status.shutdown = 1;
        stream->filters.head->on_status(stream->filters.head);
        return 0;
    }

    read.buffer = buffer;
    read.length = length;
    read.done = 0;
    read.task = stream->loop->current;

    stream->platform.read_req = &read;

    if (stream->info.read.timeout > 0)
    {
        timeout.time = time_current() + stream->info.read.timeout;
        timeout.task = stream->loop->current;

        moments_add(&stream->loop->timeouts, &timeout);
    }
    else
    {
        timeout.time = 0;
    }

    start = stopwatch_measure();

    io_loop_read_add(stream->loop, stream->fd, &stream->platform.e);

    task_suspend(read.task);

    io_loop_read_del(stream->loop, stream->fd, &stream->platform.e);

    end = stopwatch_measure();
    elapsed = stopwatch_nanoseconds(start, end);

    if (timeout.time > 0)
    {
        moments_remove(&stream->loop->timeouts, &timeout);
    }

    stream->platform.read_req = 0;
    stream->info.read.bytes += read.done;
    stream->info.read.period += elapsed;

    if (timeout.time > 0 && timeout.reached)
    {
        stream->info.status.read_timeout = 1;
        stream->filters.head->on_status(stream->filters.head);

        return 0;
    }
    else
    {
        return read.done;
    }
}

static size_t io_stream_tcp_on_write(io_filter_t* filter, const char* buffer, size_t length)
{
    io_stream_t* stream = filter->stream;
    io_tcp_write_req_t write;
    moment_t timeout;
    uint64_t start, end, elapsed;

    if (length == 0)
    {
        return length;
    }

    if (stream->info.status.write_timeout ||
        stream->info.status.error ||
        stream->info.status.closed ||
        stream->info.status.peer_closed ||
        stream->info.status.shutdown)
    {
        return 0;
    }

    if (atomic_load64(&stream->loop->shutdown))
    {
        stream->info.status.shutdown = 1;
        stream->filters.head->on_status(stream->filters.head);
        return 0;
    }

    write.buffer = buffer;
    write.length = length;
    write.offset = 0;
    write.task = stream->loop->current;

    stream->platform.write_req = &write;

    if (stream->info.write.timeout > 0)
    {
        timeout.time = time_current() + stream->info.write.timeout;
        timeout.task = stream->loop->current;

        moments_add(&stream->loop->timeouts, &timeout);
    }
    else
    {
        timeout.time = 0;
    }

    start = stopwatch_measure();

    io_loop_write_add(stream->loop, stream->fd, &stream->platform.e);

    do
    {
        task_suspend(write.task);

        if (stream->info.status.write_timeout ||
            stream->info.status.error ||
            stream->info.status.closed ||
            stream->info.status.peer_closed ||
            stream->info.status.shutdown)
        {
            break;
        }

        if (timeout.time > 0 && timeout.reached)
        {
            break;
        }
    }
    while (write.offset < write.length);

    io_loop_write_del(stream->loop, stream->fd, &stream->platform.e);

    end = stopwatch_measure();
    elapsed = stopwatch_nanoseconds(start, end);

    if (timeout.time > 0)
    {
        moments_remove(&stream->loop->timeouts, &timeout);
    }

    stream->platform.write_req = 0;
    stream->info.write.bytes += write.offset;
    stream->info.write.period += elapsed;

    if (timeout.time > 0 && timeout.reached)
    {
        stream->info.status.write_timeout = 1;
        stream->filters.head->on_status(stream->filters.head);
    }

    return write.offset;
}

static void io_file_read_completion_handler(sigval_t sigval)
{
    io_file_read_req_t* read = (io_file_read_req_t*)sigval.sival_ptr;
    ssize_t result;
    
    read->error = aio_error(&read->aio);
    if (read->error == 0)
    {
        result = aio_return(&read->aio);
        if (result < 0)
        {
            read->error = errno;
            result = 0;
        }

        read->done = result;
    }

    io_loop_post_task(read->loop, read->task);
}

static void io_file_write_completion_handler(sigval_t sigval)
{
    io_file_write_req_t* write = (io_file_write_req_t*)sigval.sival_ptr;
    ssize_t result;

    write->error = aio_error(&write->aio);
    if (write->error == 0)
    {
        result = aio_return(&write->aio);
        if (result < 0)
        {
            write->error = errno;
            result = 0;
        }

        write->done = result;
    }

    io_loop_post_task(write->loop, write->task);
}

static size_t io_stream_file_on_read(io_filter_t* filter, char* buffer, size_t length)
{
    io_stream_t* stream = filter->stream;
    io_file_read_req_t read;
    moment_t timeout;
    uint64_t start, end, elapsed;
    int result;

    if (length == 0)
    {
        return length;
    }

    if (stream->info.status.read_timeout ||
        stream->info.status.eof ||
        stream->info.status.error ||
        stream->info.status.closed ||
        stream->info.status.peer_closed ||
        stream->info.status.shutdown)
    {
        return 0;
    }

    memset(&read, 0, sizeof(io_file_read_req_t));

    read.aio.aio_fildes = stream->fd;
    read.aio.aio_buf = buffer;
    read.aio.aio_nbytes = length;
    read.aio.aio_offset = stream->impl.file.read_offset;
    read.aio.aio_sigevent.sigev_notify = SIGEV_THREAD;
    read.aio.aio_sigevent.sigev_notify_function = io_file_read_completion_handler;
    read.aio.aio_sigevent.sigev_notify_attributes = 0;
    read.aio.aio_sigevent.sigev_value.sival_ptr = &read;
    read.done = 0;
    read.loop = stream->loop;
    read.task = stream->loop->current;

    result = aio_read(&read.aio);

    if (result == -1)
    {
        stream->info.status.error = errno;
        stream->filters.head->on_status(stream->filters.head);
        return 0;
    }

    if (stream->info.read.timeout > 0)
    {
        timeout.time = time_current() + stream->info.read.timeout;
        timeout.task = stream->loop->current;

        moments_add(&stream->loop->timeouts, &timeout);        
    }
    else
    {
        timeout.time = 0;
    }

    start = stopwatch_measure();

    task_suspend(read.task);

    end = stopwatch_measure();
    elapsed = stopwatch_nanoseconds(start, end);

    stream->info.read.bytes += read.done;
    stream->info.read.period += elapsed;

    if (timeout.time > 0)
    {
        moments_remove(&stream->loop->timeouts, &timeout);
    }

    if (timeout.time > 0 && timeout.reached)
    {
        stream->info.read.timeout = 1;
        stream->filters.head->on_status(stream->filters.head);

        result = aio_cancel(stream->fd, &read.aio);
        /* handle error */

        return 0;
    }
    else
    {
        stream->impl.file.read_offset += read.done;
        if (read.done == 0)
        {
            stream->info.status.eof = 1;
        }

        if (read.error)
        {
            stream->info.status.error = read.error;
        }

        if (read.done == 0 || read.error)
        {
            stream->filters.head->on_status(stream->filters.head);
        }

        return read.done;
    }
}

size_t io_stream_file_on_write(io_filter_t* filter, const char* buffer, size_t length)
{
    io_stream_t* stream = filter->stream;
    io_file_write_req_t write;
    moment_t timeout;
    uint64_t start, end, elapsed;
    uint64_t done = 0;
    int result;

    if (stream->info.write.timeout > 0)
    {
        timeout.time = time_current() + stream->info.write.timeout;
        timeout.task = stream->loop->current;

        moments_add(&stream->loop->timeouts, &timeout);
    }
    else
    {
        timeout.time = 0;
    }

    start = stopwatch_measure();

    do
    {
        memset(&write, 0, sizeof(io_file_write_req_t));

        write.aio.aio_fildes = stream->fd;
        write.aio.aio_buf = (char*)buffer + done;
        write.aio.aio_nbytes = length - done;
        write.aio.aio_offset = stream->impl.file.write_offset;
        write.aio.aio_sigevent.sigev_notify = SIGEV_THREAD;
        write.aio.aio_sigevent.sigev_notify_function = io_file_write_completion_handler;
        write.aio.aio_sigevent.sigev_notify_attributes = 0;
        write.aio.aio_sigevent.sigev_value.sival_ptr = &write;
        write.done = 0;
        write.loop = stream->loop;
        write.task = stream->loop->current;

        result = aio_write(&write.aio);

        if (result == -1)
        {
            stream->info.status.error = errno;
            stream->filters.head->on_status(stream->filters.head);
            break;
        }

        task_suspend(write.task);

        if (atomic_load64(&stream->loop->shutdown))
        {
            stream->info.status.shutdown = 1;
            stream->filters.head->on_status(stream->filters.head);
            break;
        }

        if (stream->info.status.write_timeout ||
            stream->info.status.error ||
            stream->info.status.closed ||
            stream->info.status.peer_closed ||
            stream->info.status.shutdown)
        {
            break;
        }

        if (timeout.time > 0 && timeout.reached)
        {
            break;
        }

        if (write.error != 0)
        {
            stream->info.status.error = write.error;
            stream->filters.head->on_status(stream->filters.head);
            break;
        }

        stream->impl.file.write_offset += write.done;
        done += write.done;

    }
    while (done < length);

    end = stopwatch_measure();
    elapsed = stopwatch_nanoseconds(start, end);

    stream->info.write.bytes += done;
    stream->info.write.period += elapsed;

    if (timeout.time > 0)
    {
        moments_remove(&stream->loop->timeouts, &timeout);
    }

    if (timeout.time > 0 && timeout.reached)
    {
        stream->info.status.write_timeout = 1;
        result = aio_cancel(stream->fd, &write.aio);
        /* handle error */
    }

    if (stream->info.status.write_timeout ||
        stream->info.status.error ||
        stream->info.status.closed ||
        stream->info.status.peer_closed ||
        stream->info.status.shutdown)
    {
        stream->filters.head->on_status(stream->filters.head);
        return 0;
    }

    return done;
}

static void io_stream_on_status(io_filter_t* filter)
{
}

static void io_stream_processor(io_stream_t* stream, int events)
{
    task_t* task = 0;

    if (events == -1)
    {
        stream->info.status.shutdown = 1;
        stream->filters.head->on_status(stream->filters.head);
    }
    else if (events & EPOLLERR)
    {
        stream->info.status.error = errno;
        stream->filters.head->on_status(stream->filters.head);
    }
    else if (events & EPOLLHUP)
    {
        stream->info.status.closed = 1;
        stream->filters.head->on_status(stream->filters.head);
    }
    else if (events & EPOLLRDHUP)
    {
        stream->info.status.peer_closed = 1;
        stream->filters.head->on_status(stream->filters.head);
    }
    else
    {
        if ((events & EPOLLIN) || (events & EPOLLPRI))
        {
            io_stream_read_try(stream);
            task = ((io_tcp_read_req_t*)stream->platform.read_req)->task;
        }
        else
        if (events & EPOLLOUT)
        {
            io_stream_write_try(stream);
            task = ((io_tcp_write_req_t*)stream->platform.write_req)->task;
        }
        else
        {
            stream->info.status.error = errno;
            stream->filters.head->on_status(stream->filters.head);
        }
    }

    if (task == 0 && stream->platform.read_req != 0)
    {
        task = ((io_tcp_read_req_t*)stream->platform.read_req)->task;
    }

    if (task == 0 && stream->platform.write_req != 0)
    {
        task = ((io_tcp_write_req_t*)stream->platform.write_req)->task;
    }

    if (task != 0)
    {
        task_resume(task);
    }
}

/*
 * Internal API
 */

int io_stream_attach(io_stream_t* stream)
{
    int error = 0;

    if (stream->loop != 0)
    {
        return 0;
    }

    stream->loop = io_loop_current();

    if (atomic_load64(&stream->loop->shutdown))
    {
        stream->info.status.shutdown = 1;
        stream->filters.head->on_status(stream->filters.head);
        return ECANCELED;
    }

    if (stream->info.type == IO_STREAM_TCP)
    {
        stream->platform.e.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;
        error = epoll_ctl(stream->loop->epoll, EPOLL_CTL_ADD, stream->fd,
                            &stream->platform.e);
    }
    
    if (error)
    {
        stream->loop = 0;
    }
    else
    {
        io_loop_ref(stream->loop);
    }

    return error;
}

void io_stream_init(io_stream_t* stream)
{
    stream->platform.processor = io_stream_processor;
    stream->platform.e.data.ptr = &stream->platform;

    io_filter_attach(&stream->operations, stream);

    stream->operations.on_status = io_stream_on_status;

    switch (stream->info.type) {
    case IO_STREAM_FILE:
        stream->operations.on_read = io_stream_file_on_read;
        stream->operations.on_write = io_stream_file_on_write;
        break;
    case IO_STREAM_TCP:
        stream->operations.on_read = io_stream_tcp_on_read;
        stream->operations.on_write = io_stream_tcp_on_write;
        break;
    case IO_STREAM_UDP:
        break;
    case IO_STREAM_TTY:
        break;
    case IO_STREAM_PIPE:
        break;
    default: // IO_STREAM_MEMORY
        break;
    }
}

/*
 * Public API
 */

int io_stream_delete(io_stream_t* stream)
{
    int error = 0;

    if (stream->info.type == IO_STREAM_MEMORY)
    {
        io_memory_chunk_t* chunk = LIST_HEAD((&stream->impl.memory));
        while (chunk) {
            io_free(chunk);
            chunk = LIST_HEAD((&stream->impl.memory));
        }
    }
    else if (stream->info.type == IO_STREAM_FILE)
    {
        stream->info.status.closed = 1;
        io_close(stream->fd);
    }
    else if (stream->info.type == IO_STREAM_TCP)
    {
        if (stream->loop != 0)
        {
            error = epoll_ctl(stream->loop->epoll, EPOLL_CTL_DEL, stream->fd,
                                &stream->platform.e);

            io_loop_unref(stream->loop);

            stream->loop = 0;
        }

        if (error == 0)
        {
            stream->info.status.closed = 1;
            io_close(stream->fd);
        }
    }

    stream->filters.head->on_status(stream->filters.head);

    if (stream->unread.length > 0) 
    {
        io_free(stream->unread.buffer);
        stream->unread.length = 0;
    }

    if (stream->loop != 0)
    {
        io_loop_unref(stream->loop);
        stream->loop = 0;
    }

    return errno;
}