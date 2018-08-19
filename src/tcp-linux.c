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

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include "tcp.h"
#include "moment.h"
#include "time.h"
#include "task.h"
#include "memory.h"
#include "loop-linux.h"

typedef struct io_tcp_accept_t {
    io_stream_t* tcp;
    task_t* task;
    int error;
} io_tcp_accept_t;

typedef struct io_tcp_listener_t {
    void(*processor)(struct io_tcp_listener_t* listener, int events);
    struct epoll_event e;
    io_loop_t* loop;
    io_tcp_accept_t* accept;
    int af;
    int fd;
    int error;
    unsigned closed : 1;
    unsigned shutdown : 1;
} io_tcp_listener_t;

static int io_socket_non_block(int fd, int on)
{
    int result;

    do
    {
        result = ioctl(fd, FIONBIO, &on);
    }
    while (result == -1 && errno == EINTR);

    if (result)
    {
        return errno;
    }

    return 0;
}

static int io_socket_send_buffer_size(int fd, int size)
{
    if (0 == setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)))
    {
        return 0;
    }

    return errno;
}

static int io_socket_recv_buffer_size(int fd, int size)
{
    if (0 == setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)))
    {
        return 0;
    }

    return errno;
}

static int io_tcp_nodelay(int fd, int enable)
{
    return setsockopt(fd,
                    IPPROTO_TCP,     /* set option at TCP level */
                    TCP_NODELAY,     /* name of option */
                    (char*)&enable,  /* the cast is historical cruft */
                    sizeof(int));    /* length of option value */
}

static int io_tcp_keepalive(int fd, int enable, unsigned int delay)
{
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)))
    {
        return errno;
    }

    if (enable && setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &delay, sizeof(delay)))
    {
        return errno;
    }

    return 0;
}

static void io_tcp_listener_accept_try(io_tcp_listener_t* listener)
{
    struct sockaddr address;
    socklen_t length = sizeof(address);
    io_stream_t* tcp = listener->accept->tcp;
    int error = 0;

    tcp->fd = accept(listener->fd, &address, &length);
    if (tcp->fd == -1)
    {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            /* We have processed all incoming connections. */
        }
        else
        {
            error = errno;
        }
    }

    if (!error)
    {
        error = io_socket_non_block(tcp->fd, 1);
    }

    if (!error)
    {
        error = io_socket_recv_buffer_size(tcp->fd, 0);
    }

    if (!error)
    {
        error = io_socket_send_buffer_size(tcp->fd, 0);
    }

    if (!error)
    {
        error = io_tcp_nodelay(tcp->fd, 1);
    }

    listener->accept->error = error;
}

static void io_tcp_listener_processor(io_tcp_listener_t* listener, int events)
{
    int error;
    int wakeup = 1;
    task_t* task = 0;

    if (events == -1)
    {
        listener->accept->error = ECANCELED;
    }
    else if (events & EPOLLERR)
    {
        listener->accept->error = errno;
    }
    else if (events & EPOLLHUP)
    {
        listener->accept->error = ECANCELED;
    }
    else
    {
        if ((events & EPOLLIN) || (events & EPOLLPRI))
        {
            io_tcp_listener_accept_try(listener);
            wakeup = listener->accept->error == 0;
        }
        else
        {
            listener->accept->error = errno;
        }
    }

    if (wakeup)
    {
        task_resume(listener->accept->task);
    }
}

static void io_tcp_connect_processor(io_stream_t* stream, int events)
{
    if (events == -1)
    {
        stream->info.status.shutdown = 1;
    }
    else if (events & EPOLLERR)
    {
        stream->info.status.error = errno;
    }
    else if (events & EPOLLHUP)
    {
        stream->info.status.closed = 1;
    }
    else if ((events & EPOLLOUT) != EPOLLOUT)
    {
        stream->info.status.error = errno;
    }

    task_resume((task_t*)stream->platform.read_req);
}

/*
 * Internal API
 */

int io_tcp_init()
{
    return 0;
}

/*
 * Public API
 */

int io_tcp_listen(io_tcp_listener_t** listener, const char* ip, int port, int backlog)
{
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
    struct sockaddr* address;
    socklen_t length;

    int error = 0;
    io_loop_t* loop = io_loop_current();

    if (atomic_load64(&loop->shutdown))
    {
        return ECANCELED;
    }

    *listener = io_calloc(1, sizeof(io_tcp_listener_t));
    if (*listener == 0)
    {
        return ENOMEM;
    }

    if (strchr(ip, ':') == 0)
    {
        // ipv4
        (*listener)->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

        address = (struct sockaddr*)&addr_in;
        length = sizeof(struct sockaddr_in);
        addr_in.sin_family = AF_INET;
        addr_in.sin_addr.s_addr = inet_addr(ip);
        addr_in.sin_port = htons(port);

        (*listener)->af = AF_INET;
    }
    else
    {
        // ipv6
        (*listener)->fd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

        address = (struct sockaddr*)&addr_in6;
        length = sizeof(struct sockaddr_in6);
        addr_in6.sin6_family = AF_INET6;
        addr_in6.sin6_addr = in6addr_any;
        addr_in6.sin6_port = htons(port);

        inet_pton(AF_INET6, ip, (void*)&addr_in6.sin6_addr.__in6_u);

        (*listener)->af = AF_INET6;
    }

    error = io_socket_non_block((*listener)->fd, 1);
    error = io_socket_recv_buffer_size((*listener)->fd, 0);
    error = io_socket_send_buffer_size((*listener)->fd, 0);
    error = io_tcp_nodelay((*listener)->fd, 1);

    if (0 != bind((*listener)->fd, address, length))
    {
        io_free(*listener);
        return errno;
    }

    if (0 != listen((*listener)->fd, backlog))
    {
        io_free(*listener);
        return errno;
    }

    (*listener)->loop = loop;
    (*listener)->processor = io_tcp_listener_processor;
    (*listener)->e.data.ptr = (*listener);
    (*listener)->e.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    error = epoll_ctl(loop->epoll, EPOLL_CTL_ADD, (*listener)->fd, &(*listener)->e);

    if (!error)
    {
        io_loop_ref(loop);
    }
    else
    {
        io_free(*listener);
    }

    return error;
}

int io_tcp_shutdown(io_tcp_listener_t* listener)
{
    int error = epoll_ctl(listener->loop->epoll, EPOLL_CTL_DEL, listener->fd, &listener->e);

    io_close(listener->fd);
    listener->closed = 1;

    if (listener->loop != 0)
    {
        io_loop_unref(listener->loop);
        listener->loop = 0;
    }

    return error;
}

int io_tcp_accept(io_stream_t** tcp, io_tcp_listener_t* listener)
{
    io_tcp_accept_t accept;
    io_stream_t* stream;

    if (listener->error)
    {
        return listener->error;
    }

    if (listener->closed)
    {
        return ECANCELED;
    }

    if (listener->shutdown)
    {
        return ECANCELED;
    }

    if (atomic_load64(&listener->loop->shutdown))
    {
        listener->shutdown = 1;
        return ECANCELED;
    }

    stream = io_calloc(1, sizeof(*stream));
    if (stream == 0)
    {
        return ENOMEM;
    }

    stream->info.type = IO_STREAM_TCP;
    accept.task = listener->loop->current;
    accept.tcp = stream;
    accept.error = 0;

    listener->accept = &accept;

    io_loop_read_add(listener->loop, listener->fd, &listener->e);

    task_suspend(accept.task);

    io_loop_read_del(listener->loop, listener->fd, &listener->e);

    listener->accept = 0;

    if (accept.error)
    {
        io_free(stream);
        return accept.error;
    }

    *tcp = stream;
    io_stream_init(stream);

    return 0;
}

int io_tcp_connect(io_stream_t** tcp, const char* ip, int port, uint64_t tmeout)
{
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
    struct sockaddr* address;
    socklen_t length;

    io_stream_t* stream;
    int error = 0;
    moment_t timeout;
    io_loop_t* loop = io_loop_current();

    if (atomic_load64(&loop->shutdown))
    {
        return ECANCELED;
    }

    stream = io_calloc(1, sizeof(*stream));
    if (stream == 0)
    {
        return ENOMEM;
    }

    stream->info.type = IO_STREAM_TCP;
    stream->loop = loop;
    stream->platform.processor = io_tcp_connect_processor;
    stream->platform.read_req = loop->current;
    stream->platform.e.data.ptr = &stream->platform;

    if (strchr(ip, ':') == 0)
    {
        // ipv4
        stream->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

        address = (struct sockaddr*)&addr_in;
        length = sizeof(struct sockaddr_in);
        addr_in.sin_family = AF_INET;
        addr_in.sin_addr.s_addr = inet_addr(ip);
        addr_in.sin_port = htons(port);
    }
    else
    {
        // ipv6
        stream->fd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

        address = (struct sockaddr*)&addr_in6;
        length = sizeof(struct sockaddr_in6);
        addr_in6.sin6_family = AF_INET6;
        addr_in6.sin6_addr = in6addr_any;
        addr_in6.sin6_port = htons(port);

        inet_pton(AF_INET6, ip, (void*)&addr_in6.sin6_addr.__in6_u);
    }

    error = io_socket_non_block(stream->fd, 1);
    error = io_socket_recv_buffer_size(stream->fd, 0);
    error = io_socket_send_buffer_size(stream->fd, 0);
    error = io_tcp_nodelay(stream->fd, 1);

    if (0 != connect(stream->fd, address, length))
    {
        if (errno != EINPROGRESS)
        {
            error = errno;
        }
        else
        {
            // wait needed
            stream->platform.e.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLOUT;
            error = epoll_ctl(loop->epoll, EPOLL_CTL_ADD, stream->fd, &stream->platform.e);
            if (0 == error)
            {
                error = io_loop_write_add(loop, stream->fd, &stream->platform.e);
                if (0 != error)
                {
                    epoll_ctl(loop->epoll, EPOLL_CTL_DEL, stream->fd, &stream->platform.e);
                }
                else
                {
                    if (tmeout > 0)
                    {
                        timeout.time = time_current() + tmeout;
                        timeout.task = loop->current;

                        moments_add(&loop->timeouts, &timeout);
                    }
                    else
                    {
                        timeout.time = 0;
                    }

                    task_suspend(loop->current);

                    if (timeout.time > 0)
                    {
                        moments_remove(&loop->timeouts, &timeout);
                    }

                    if (timeout.time > 0 && timeout.reached)
                    {
                        error = ETIMEDOUT;
                    }

                    if (stream->info.status.closed ||
                        stream->info.status.shutdown ||
                        stream->info.status.error)
                    {
                        epoll_ctl(loop->epoll, EPOLL_CTL_DEL, stream->fd, &stream->platform.e);
                    }
                    else
                    {
                        io_loop_write_del(loop, stream->fd, &stream->platform.e);
                    }
                }
            }
        }
    }
    else
    {
        // completed immediately
    }

    if (!error && stream->info.status.error)
    {
        error = stream->info.status.error;
    }

    if (!error && stream->info.status.closed)
    {
        error = ECANCELED;
    }

    if (!error && stream->info.status.shutdown)
    {
        error = ECANCELED;
    }

    if (error)
    {
        io_close(stream->fd);
        io_free(stream);

        return error;
    }

    io_stream_init(stream);
    io_loop_ref(loop);

    *tcp = stream;

    return 0;
}