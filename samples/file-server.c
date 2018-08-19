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

#if defined(_WIN32)
#	define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "../include/io.h"

#if defined(__linux__)
#   define strcmp_nocase strcasecmp
#else
#   pragma warning(disable: 4996)
#   define strcmp_nocase _stricmp
#endif

#define HTTP404 \
  "HTTP/1.1 404 Not Found\r\n" \
  "Content-Type: text/plain\r\n" \
  "Content-Length: 0\r\n" \
  "\r\n" \
  "\n"

static char* folder;

void parse_filepath(const char* req, char* filepath)
{
    int i = 0;

    while (req[i++] != ' ');

    ++i;

    while (req[i] != ' ')
        *filepath++ = req[i++];

    *filepath = 0;
}

const char* mime_type(const char* name)
{
    size_t dot = strlen(name) - 1;

    while (dot >= 0 && name[dot] != '.')
        --dot;

    if (0 == strcmp_nocase(name + dot, ".html")) return "text/html";
    if (0 == strcmp_nocase(name + dot, ".htm")) return "text/html";
    if (0 == strcmp_nocase(name + dot, ".css")) return "text/css";
    if (0 == strcmp_nocase(name + dot, ".js")) return "text/javascript";
    if (0 == strcmp_nocase(name + dot, ".jpg")) return "image/jpg";
    if (0 == strcmp_nocase(name + dot, ".png")) return "image/png";
    if (0 == strcmp_nocase(name + dot, ".gif")) return "image/gif";
    if (0 == strcmp_nocase(name + dot, ".ico")) return "image/icon";

    return "application/octet-stream";
}

int send_headers(io_stream_t* stream, const char* path)
{
    io_path_info_t info;
    const char* mime = mime_type(path);
    char content_length[50];

    if (0 != io_path_info_get(path, &info) || info.size == 0)
    {
        io_stream_write(stream, HTTP404, sizeof(HTTP404) - 1);
        return EFAULT;
    }

    io_stream_write(stream, "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Type: ", 64);
    io_stream_write(stream, mime, strlen(mime));
    io_stream_write(stream, "\r\nContent-Length: ", 18);

    sprintf(content_length, "%d", (int)info.size);

    io_stream_write(stream, content_length, strlen(content_length));
    io_stream_write(stream, "\r\n\r\n", 4);

    return 0;
}

void on_connection(io_loop_t* loop, io_stream_t* stream)
{
    char buffer[1024 * 4];
    char filepath[1024 * 3];
    io_stream_t* file;
    size_t transferred;
    int error;
    io_stream_info_t* info;

    transferred = io_stream_read(stream, buffer, sizeof(buffer), 0);
    if (transferred == 0)
    {
        io_stream_write(stream, HTTP404, sizeof(HTTP404) - 1);
    }
    else
    {
        parse_filepath(buffer, filepath);
        if (0 == strlen(filepath))
        {
            strcpy(filepath, "index.html");
        }

        strcpy(buffer, folder);
        strcat(buffer, filepath);

        error = io_file_open(&file, buffer, 0);
        if (error)
        {
            io_stream_write(stream, HTTP404, sizeof(HTTP404) - 1);
        }
        else
        {
            send_headers(stream, buffer);
            io_stream_pipe(file, stream, 0, 0);
            io_stream_close(file);
        }
    }

    io_stream_close(stream);
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

    printf("server listening at http://localhost:8080\r\n");

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

void main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("usage: file-server folder\\\r\n");
        return;
    }

    folder = argv[1];

    int error = io_run(io_main, 0);
    if (error)
    {
        printf("%s\r\n", strerror(error));
    }
}