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

#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#   if defined(IO_BUILD_SHARED)
#       define IO_API __declspec(dllexport)
#   elif defined(IO_USE_SHARED)
#       define IO_API __declspec(dllimport)
#   else
#       define IO_API /* nothing */
#   endif
#elif __GNUC__ >= 4
#   define IO_API __attribute__((visibility("default")))
#else
#   define IO_API /* nothing */
#endif

#include <stdint.h> // uint64_t
#include <stddef.h> // size_t


// Typical call returns 0 on success and errno on failure

IO_API void io_set_allocator(
    void* (*malloc_fn)(size_t size),
    void* (*calloc_fn)(size_t count, size_t size),
    void* (*realloc_fn)(void* ptr, size_t size),
    void  (*free_fn)(void* ptr));


// Loop

typedef struct io_loop_t io_loop_t;
typedef void (*io_loop_fn)(io_loop_t* loop, void* arg);

IO_API int io_loop_start(io_loop_t** loop);
IO_API int io_loop_stop(io_loop_t* loop);
IO_API int io_loop_ref(io_loop_t* loop);
IO_API int io_loop_unref(io_loop_t* loop);
IO_API int io_loop_post(io_loop_t* loop, io_loop_fn entry, void* arg);
IO_API int io_loop_exec(io_loop_t* loop, io_loop_fn entry, void* arg);
IO_API int io_loop_sleep(uint64_t milliseconds);
IO_API int io_loop_idle(io_loop_t* loop, uint64_t milliseconds);


// Event

typedef struct io_event_t io_event_t;

IO_API int io_event_create(io_event_t** event);
IO_API int io_event_delete(io_event_t* event);
IO_API int io_event_notify(io_event_t* event);
IO_API int io_event_wait(io_event_t* event);


// Stream

typedef enum io_stream_type_t {
    IO_STREAM_MEMORY,
    IO_STREAM_FILE,
    IO_STREAM_TCP,
    IO_STREAM_UDP,
    IO_STREAM_TTY,
    IO_STREAM_PIPE
} io_stream_type_t;

typedef struct io_stream_status_t {
    unsigned eof : 1;
    unsigned closed : 1;
    unsigned peer_closed : 1;
    unsigned shutdown : 1;
    unsigned read_timeout : 1;
    unsigned write_timeout : 1;
    int error;
} io_stream_status_t;

typedef struct io_stream_info_t {
    io_stream_type_t type;
    io_stream_status_t status;
    struct {
        uint64_t timeout;       // milliseconds
        uint64_t bytes;         // total bytes
        uint64_t period;        // total nanoseconds
        uint64_t position;
    } read;
    struct {
        uint64_t timeout;       // milliseconds
        uint64_t bytes;         // total bytes
        uint64_t period;        // total nanoseconds
        uint64_t position;
    } write;
    void* data;
} io_stream_info_t;

typedef struct io_stream_t io_stream_t;

IO_API int io_stream_create(io_stream_t** stream); // Creates a memory stream
IO_API int io_stream_close(io_stream_t* stream);
IO_API int io_stream_info(io_stream_t* stream, io_stream_info_t** info);
IO_API size_t io_stream_read(io_stream_t* stream, char* buffer, size_t length, int exact);
IO_API size_t io_stream_unread(io_stream_t* stream, const char* buffer, size_t length);
IO_API size_t io_stream_write(io_stream_t* stream, const char* buffer, size_t length);
IO_API int io_stream_pipe(io_stream_t* from, io_stream_t* to, size_t chunk_size, size_t* transferred);

typedef struct io_filter_t {
    struct io_filter_t* next;
    struct io_filter_t* prev;
    io_stream_t* stream;
    size_t(*on_read)(struct io_filter_t* filter, char* buffer, size_t length);
    size_t(*on_write)(struct io_filter_t* filter, const char* buffer, size_t length);
    void(*on_status)(struct io_filter_t* filter);
} io_filter_t;

IO_API void io_filter_attach(io_filter_t* filter, io_stream_t* stream);
IO_API void io_filter_detach(io_filter_t* filter, io_stream_t* stream); 


// Tcp

typedef struct io_tcp_listener_t io_tcp_listener_t;

IO_API int io_tcp_listen(io_tcp_listener_t** listener, const char* ip, int port, int backlog);
IO_API int io_tcp_shutdown(io_tcp_listener_t* listener);
IO_API int io_tcp_accept(io_stream_t** stream, io_tcp_listener_t* listener);
IO_API int io_tcp_connect(io_stream_t** stream, const char* ip, int port, uint64_t timeout);


// Path info

typedef struct io_path_info_t {
    uint64_t size;
    uint64_t time_create;
    uint64_t time_access;
    uint64_t time_modified;
    int attributes;
    unsigned int is_file : 1;
} io_path_info_t;

IO_API int io_path_info_get(const char* path, io_path_info_t* info);
IO_API int io_path_info_set(const char* path, io_path_info_t* info);


// File

typedef enum io_file_options_t {
    IO_FILE_CREATE      = 1,
    IO_FILE_APPEND      = 2,
    IO_FILE_TRUNCATE    = 4
} io_file_options_t;

IO_API int io_file_create(const char* path);
IO_API int io_file_delete(const char* path);
IO_API int io_file_open(io_stream_t** stream, const char* path, io_file_options_t options);


// Directory

typedef struct io_directory_enum_t io_directory_enum_t;

IO_API int io_directory_create(const char* path);
IO_API int io_directory_delete(const char* path, int recursive);
IO_API int io_directory_listen(const char* path, const char* pattern, int events, uint64_t timeout);
IO_API int io_directory_enum_create(io_directory_enum_t** enm, const char* path);
IO_API int io_directory_enum_next(io_directory_enum_t* enm, const char** name, io_path_info_t* info);
IO_API int io_directory_enum_delete(io_directory_enum_t* enm);


IO_API int io_run(io_loop_fn entry, void* arg);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_H_INCLUDED