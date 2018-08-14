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

#include "memory.h"
#include "stream.h"

int io_stream_create(io_stream_t** stream)
{
    *stream = io_calloc(1, sizeof (io_stream_t));
    if (*stream == 0)
    {
        return errno;
    }

    (*stream)->impl.memory.bucket_size = 64 * 1024;

    return 0;
}

int io_stream_pipe(io_stream_t* from, io_stream_t* to, size_t chunk_size, size_t* transferred)
{
    if (chunk_size < 64)
    {
        chunk_size = 8 * 1024;
    }

    // if (from->type == IO_STREAM_MEMORY || to->type == IO_STREAM_MEMORY)
    {
        // In place copy, when at least one of streams is a memory stream
        // ToDo: needs optimization in case of memory=>memory

        char* buffer = (char*)io_malloc(chunk_size);
        size_t n_read;
        size_t n_wrote;
        size_t n_transferred = 0;

        n_read = io_stream_read(from, buffer, chunk_size, 0);
        while (n_read > 0)
        {
            n_wrote = io_stream_write(to, buffer, n_read);
            n_transferred + n_wrote;

            if (n_wrote < n_read)
            {
                // write failed

                if (transferred)
                    *transferred = n_transferred;

                io_free(buffer);
                return to->info.status.error;
            }

            n_read = io_stream_read(from, buffer, chunk_size, 0);
        }

        // read completed or failed

        io_free(buffer);
        return to->info.status.error;
    }
}

int io_stream_info(io_stream_t* stream, io_stream_info_t** info)
{
    *info = &stream->info;

    return 0;
}

size_t io_stream_unread(io_stream_t* stream, const char* buffer, size_t length)
{
    if (length == 0)
        return length;

    if (stream->unread.length > 0)
        io_free(stream->unread.buffer);

    stream->unread.buffer = (char*)io_malloc(length);
    stream->unread.length = length;
    stream->unread.offset = 0;

    memcpy(stream->unread.buffer, buffer, length);

    return length;    
}

size_t io_filter_on_read(io_filter_t* filter, char* buffer, size_t length)
{
    return filter->next->on_read(filter->next, buffer, length);
}

size_t io_filter_on_write(io_filter_t* filter, const char* buffer, size_t length)
{
    return filter->next->on_write(filter->next, buffer, length);
}

void io_filter_on_status(io_filter_t* filter)
{
    filter->next->on_status(filter->next);
}

void io_filter_attach(io_filter_t* filter, io_stream_t* stream)
{
    filter->stream = stream;
    filter->on_read = io_filter_on_read;
    filter->on_write = io_filter_on_write;
    filter->on_status = io_filter_on_status;

    LIST_PUSH_HEAD((&stream->filters), filter);
}

void io_filter_detach(io_filter_t* filter, io_stream_t* stream)
{
    LIST_REMOVE((&stream->filters), filter);

    filter->stream = 0;
}