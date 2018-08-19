# libio

libio is a cross platform high performance io library written in C. It
provides ability to write event driven servers and applications
with continuous code without callback hell.

Alpha version

## CLI

```bash
# build
> make

# build debug
> make build=Debug

# install
> make install

# uninstall
> make uninstall
```

## Samples

```
> make
> bin/file-server samples/todomvc/
# open browser and navigate to http://localhost:8080
```

## API Reference

```c
// All 'int' calls return 0 on success and errno on failure

typedef struct io_loop_t io_loop_t;
typedef void (*io_loop_fn)(io_loop_t* loop, void* arg);

int io_loop_start(io_loop_t** loop);
int io_loop_stop(io_loop_t* loop);
int io_loop_ref(io_loop_t* loop);
int io_loop_unref(io_loop_t* loop);
int io_loop_post(io_loop_t* loop, io_loop_fn entry, void* arg);
int io_loop_exec(io_loop_t* loop, io_loop_fn entry, void* arg);
int io_loop_sleep(uint64_t milliseconds);
int io_loop_idle(io_loop_t* loop, uint64_t milliseconds);


typedef struct io_event_t io_event_t;

int io_event_create(io_event_t** event);
int io_event_delete(io_event_t* event);
int io_event_notify(io_event_t* event);
int io_event_wait(io_event_t* event);


typedef struct io_stream_t io_stream_t;

int io_stream_create(io_stream_t** stream); // Creates a memory stream
int io_stream_close(io_stream_t* stream);
int io_stream_info(io_stream_t* stream, io_stream_info_t** info);
size_t io_stream_read(io_stream_t* stream, char* buffer, size_t length, int exact);
size_t io_stream_unread(io_stream_t* stream, const char* buffer, size_t length);
size_t io_stream_write(io_stream_t* stream, const char* buffer, size_t length);
int io_stream_pipe(io_stream_t* from, io_stream_t* to, size_t chunk_size, size_t* transferred);


typedef struct io_tcp_listener_t io_tcp_listener_t;

int io_tcp_listen(io_tcp_listener_t** listener, const char* ip, int port, int backlog);
int io_tcp_shutdown(io_tcp_listener_t* listener);
int io_tcp_accept(io_stream_t** stream, io_tcp_listener_t* listener);
int io_tcp_connect(io_stream_t** stream, const char* ip, int port, uint64_t timeout);


int io_file_create(const char* path);
int io_file_delete(const char* path);
int io_file_open(io_stream_t** stream, const char* path, io_file_options_t options);

// main entry
int io_run(io_loop_fn entry, void* arg);
```

## Platforms

Tested on
- Win64 (32 bit build)
- Ubuntu64 (64 bit build)

## ToDos

- Test on all platforms with different architectures

## Contacts

Email：[artak.khnkoyan@gmail.com](mailto:artak.khnkoyan@gmail.com)

## Licence

MIT