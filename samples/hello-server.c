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
#include <string.h>
#include <errno.h>
#include "../include/io.h"

#define RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/plain\r\n" \
  "Content-Length: 12\r\n" \
  "\r\n" \
  "hello world\n"

void on_connection(io_loop_t* loop, io_stream_t* stream)
{
    char buffer[1024 * 8];
    size_t tansferred;

    io_stream_info_t* info;

    io_stream_info(stream, &info);

    tansferred = io_stream_read(stream, buffer, sizeof(buffer), 0);

    if (info->status.closed ||
        info->status.peer_closed ||
        info->status.shutdown ||
        info->status.read_timeout ||
        info->status.write_timeout)
    {
        printf("fail status\r\n");
    }
    else if (info->status.error)
    {
        printf("%s\r\n", strerror(info->status.error));
    }
    else
    {
        tansferred = io_stream_write(stream, RESPONSE, sizeof(RESPONSE) - 1);
        if (info->status.error)
        {
            printf("%s\r\n", strerror(info->status.error));
        }
    }

    io_stream_delete(stream);
}

void io_main(io_loop_t* loop, void* arg)
{
    int error;
    io_tcp_listener_t* server;
    io_stream_t* stream;

    error = io_tcp_listen(&server, "127.0.0.1", 8080, 100);
    if (error)
    {
        printf("%s\r\n", strerror(error));
        return;
    }

    while (1)
    {
        error = io_tcp_accept(&stream, server);
        if (error)
        {
            printf("%s\r\n", strerror(error));
            break;
        }

        io_loop_post(loop, (io_loop_fn)on_connection, stream);
    }
}

void main()
{
    int error = io_run(io_main, 0);
    if (error)
    {
        printf("%s\r\n", strerror(error));
    }
}