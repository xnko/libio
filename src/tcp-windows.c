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

#include "tcp.h"
#include "io.h"
#include "task.h"
#include "memory.h"
#include "time.h"

#pragma comment(lib, "Ws2_32.lib")

static LPFN_ACCEPTEX lpfnAcceptEx;
static LPFN_DISCONNECTEX lpfnDisconnectEx;
static LPFN_CONNECTEX lpfnConnectEx;
static LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs;

typedef struct io_tcp_accept_t {
	task_t* task;
} io_tcp_accept_t;

typedef struct io_tcp_listener_t {
	void(*processor)(void* e, DWORD transferred, OVERLAPPED* overlapped, io_loop_t* loop, DWORD error);
	OVERLAPPED ovl;
	io_loop_t* loop;
	io_tcp_accept_t* accept;
	int af;
	SOCKET fd;
	int error;
	unsigned closed : 1;
	unsigned shutdown : 1;
} io_tcp_listener_t;

static int io_socket_non_block(SOCKET fd, int on)
{
	u_long v = on;
	int error;

	if (SOCKET_ERROR == ioctlsocket(fd, FIONBIO, &v))
	{
		error = EFAULT; // error_from_system(WSAGetLastError());

		return error;
	}

	return 0;
}

static int io_socket_send_buffer_size(SOCKET fd, int size)
{
	int error;

	if (SOCKET_ERROR != setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
		(const char*)&size, sizeof(size)))
	{
		return 0;
	}

	error = EFAULT; // error_from_system(WSAGetLastError());

	return error;
}

static int io_socket_recv_buffer_size(SOCKET fd, int size)
{
	int error;

	if (SOCKET_ERROR != setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
		(const char*)&size, sizeof(size)))
	{
		return 0;
	}

	error = EFAULT; // error_from_system(WSAGetLastError());

	return error;
}

static int io_tcp_nodelay(SOCKET fd, int enable)
{
	int error;

	int result = setsockopt(fd,
		IPPROTO_TCP,     /* set option at TCP level */
		TCP_NODELAY,     /* name of option */
		(char*)&enable,  /* the cast is historical cruft */
		sizeof(int));    /* length of option value */

	if (result == 0)
	{
		return 0;
	}

	error = EFAULT; // error_from_system(WSAGetLastError());

	return error;
}

static int io_tcp_keepalive(SOCKET fd, int enable, unsigned int delay)
{
	int error;

	if (SOCKET_ERROR == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
		(const char*)&enable, sizeof(enable)))
	{
		error = EFAULT; // error_from_system(WSAGetLastError());

		return error;
	}

	return 0;
}

void io_tcp_listener_processor(void* e, DWORD transferred,
	OVERLAPPED* overlapped, io_loop_t* loop,
	DWORD error)
{
	io_tcp_listener_t* listener = (io_tcp_listener_t*)e;

	task_resume(listener->accept->task);
}

void io_tcp_connect_processor(void* e, DWORD transferred,
	OVERLAPPED* overlapped, io_loop_t* loop,
	DWORD error)
{
	io_stream_t* stream = container_of(e, io_stream_t, platform);

	task_t* task = (task_t*)stream->platform.read_req;

	task_resume(task);
}

/*
 * Internal API
 */

int io_tcp_init()
{
	WORD version = 0x202;
	WSADATA wsadata;
	SOCKET socket;
	DWORD dwBytes;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
	GUID GuidConnectEx = WSAID_CONNECTEX;
	GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	int iResult;
	int sys_error = 0;

	sys_error = WSAStartup(version, &wsadata);
	if (sys_error != 0)
	{
		return EFAULT; // error_set_from_system(sys_error);
	}

	socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);

	if (socket == INVALID_SOCKET)
	{
		return EFAULT; // error_set_from_system(WSAGetLastError());
	}

	iResult = WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof(GuidAcceptEx),
		&lpfnAcceptEx, sizeof(lpfnAcceptEx),
		&dwBytes, NULL, NULL);
	if (iResult == SOCKET_ERROR)
	{
		sys_error = WSAGetLastError();
		closesocket(socket);
		return EFAULT; // error_set_from_system(sys_error);
	}

	iResult = WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidDisconnectEx, sizeof(GuidDisconnectEx),
		&lpfnDisconnectEx, sizeof(lpfnDisconnectEx),
		&dwBytes, 0, 0);
	if (iResult == SOCKET_ERROR)
	{
		sys_error = WSAGetLastError();
		closesocket(socket);
		return EFAULT; // error_set_from_system(sys_error);
	}

	iResult = WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidConnectEx, sizeof(GuidConnectEx),
		&lpfnConnectEx, sizeof(lpfnConnectEx),
		&dwBytes, 0, 0);
	if (iResult == SOCKET_ERROR)
	{
		sys_error = WSAGetLastError();
		closesocket(socket);
		return EFAULT; // error_set_from_system(sys_error);
	}

	iResult = WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockaddrs,
		sizeof(GuidGetAcceptExSockaddrs),
		&lpfnGetAcceptExSockaddrs,
		sizeof(lpfnGetAcceptExSockaddrs), &dwBytes, 0, 0);
	if (iResult == SOCKET_ERROR)
	{
		sys_error = WSAGetLastError();
		closesocket(socket);
		return EFAULT; // error_set_from_system(sys_error);
	}

	closesocket(socket);

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

	HANDLE handle;

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
		(*listener)->fd = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			NULL, 0, WSA_FLAG_OVERLAPPED);

		address = (struct sockaddr*)&addr_in;
		length = sizeof(struct sockaddr_in);
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = htons(port);

		inet_pton(AF_INET, ip, (void*)&addr_in.sin_addr);

		(*listener)->af = AF_INET;
	}
	else
	{
		// ipv6
		(*listener)->fd = WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
			NULL, 0, WSA_FLAG_OVERLAPPED);

		address = (struct sockaddr*)&addr_in6;
		length = sizeof(struct sockaddr_in6);
		addr_in6.sin6_family = AF_INET6;
		addr_in6.sin6_addr = in6addr_any;
		addr_in6.sin6_port = htons(port);

		inet_pton(AF_INET6, ip, (void*)&addr_in6.sin6_addr);

		(*listener)->af = AF_INET6;
	}

	error = io_socket_non_block((*listener)->fd, 1);
	error = io_socket_recv_buffer_size((*listener)->fd, 0);
	error = io_socket_send_buffer_size((*listener)->fd, 0);
	error = io_tcp_nodelay((*listener)->fd, 1);

	if (error)
	{
		closesocket((*listener)->fd);
		io_free(*listener);
		return EFAULT;
	}

	if (0 != bind((*listener)->fd, address, length))
	{
		closesocket((*listener)->fd);
		io_free(*listener);
		return EFAULT; // WSAGetLastError
	}

	if (0 != listen((*listener)->fd, backlog))
	{
		closesocket((*listener)->fd);
		io_free(*listener);
		return EFAULT; // WSAGetLastError
	}

	(*listener)->loop = loop;
	(*listener)->processor = io_tcp_listener_processor;

	handle = CreateIoCompletionPort((HANDLE)(*listener)->fd, loop->iocp, (ULONG_PTR)*listener, 0);
	if (handle == NULL)
	{
		closesocket((*listener)->fd);
		io_free(*listener);
		return EFAULT;
	}

	(*listener)->loop = loop;
	io_loop_ref(loop);

	SetFileCompletionNotificationModes((HANDLE)(*listener)->fd,
		FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

	return 0;
}

int io_tcp_shutdown(io_tcp_listener_t* listener)
{
	closesocket(listener->fd);
	listener->closed = 1;

	if (listener->loop != 0)
	{
		io_loop_unref(listener->loop);
		listener->loop = 0;
	}

	io_free(listener);

	return 0;
}

int io_tcp_accept(io_stream_t** stream, io_tcp_listener_t* listener)
{
	io_tcp_accept_t accept;
	CHAR buffer[2 * (sizeof(SOCKADDR_IN) + 16)];
	SOCKADDR *lpLocalSockaddr = NULL;
	SOCKADDR *lpRemoteSockaddr = NULL;
	int localSockaddrLen = 0;
	int remoteSockaddrLen = 0;
	DWORD dwBytes;
	DWORD sys_error;
	int error = 0;
	SOCKET fd;

	memset(&listener->ovl, 0, sizeof(OVERLAPPED));

	if (atomic_load64(&listener->loop->shutdown))
	{
		listener->error = ECANCELED;
		return ECANCELED;
	}

	if (listener->closed)
	{
		return ECANCELED;
	}

	if (listener->error != 0)
	{
		return listener->error;
	}

	accept.task = listener->loop->current;

	listener->accept = &accept;

	fd = WSASocketW(listener->af, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (!lpfnAcceptEx(listener->fd, fd, buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&dwBytes, (LPOVERLAPPED)&listener->ovl))
	{
		sys_error = WSAGetLastError();
		if (sys_error == WSA_IO_PENDING)
		{
			task_suspend(accept.task);
		}
		else if (sys_error != ERROR_SUCCESS)
		{
			closesocket(fd);
			return EFAULT;
		}
	}

	//lpfnGetAcceptExSockaddrs(buffer, 0,
	//	sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
	//	&lpLocalSockaddr, &localSockaddrLen,
	//	&lpRemoteSockaddr, &remoteSockaddrLen);

	//tcp->address.length = remoteSockaddrLen;
	//memcpy(&tcp->address.address, lpRemoteSockaddr, remoteSockaddrLen);

	error = io_socket_non_block(fd, 1);
	error = io_socket_recv_buffer_size(fd, 0);
	error = io_socket_send_buffer_size(fd, 0);
	error = io_tcp_nodelay(fd, 1);

	if (error)
	{
		closesocket(fd);
		return error;
	}

	*stream = io_calloc(1, sizeof(io_stream_t));
	if (*stream == 0)
	{
		closesocket(fd);
		return ENOMEM;
	}

	(*stream)->info.type = IO_STREAM_TCP;
	(*stream)->fd = (HANDLE)fd;
	io_stream_init(*stream);

	listener->accept = 0;

	return 0;
}

int io_tcp_connect(io_stream_t** stream, const char* ip, int port, uint64_t tmeout)
{
	struct sockaddr_in addr_in;
	struct sockaddr_in6 addr_in6;
	struct sockaddr* address;
	socklen_t length;

	HANDLE handle;
	DWORD dwSent = 0;
	DWORD sys_error = 0;
	BOOL result;
	BOOL completed = FALSE;
	int af;

	int error = 0;
	moment_t timeout;
	io_loop_t* loop = io_loop_current();

	if (atomic_load64(&loop->shutdown))
	{
		return ECANCELED;
	}

	*stream = io_calloc(1, sizeof(io_stream_t));
	if (*stream == 0)
	{
		return ENOMEM;
	}

	(*stream)->platform.processor = io_tcp_connect_processor;
	(*stream)->platform.read_req = loop->current;

	if (strchr(ip, ':') == 0)
	{
		// ipv4
		(*stream)->fd = (HANDLE)WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			NULL, 0, WSA_FLAG_OVERLAPPED);

		af = AF_INET;

		address = (struct sockaddr*)&addr_in;
		length = sizeof(struct sockaddr_in);
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = htons(port);

		inet_pton(AF_INET, ip, (void*)&addr_in.sin_addr);
	}
	else
	{
		// ipv6
		(*stream)->fd = (HANDLE)WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
			NULL, 0, WSA_FLAG_OVERLAPPED);

		af = AF_INET6;

		address = (struct sockaddr*)&addr_in6;
		length = sizeof(struct sockaddr_in6);
		addr_in6.sin6_family = AF_INET6;
		addr_in6.sin6_addr = in6addr_any;
		addr_in6.sin6_port = htons(port);

		inet_pton(AF_INET6, ip, (void*)&addr_in6.sin6_addr);
	}

	error = io_socket_non_block((SOCKET)(*stream)->fd, 1);
	error = io_socket_recv_buffer_size((SOCKET)(*stream)->fd, 0);
	error = io_socket_send_buffer_size((SOCKET)(*stream)->fd, 0);
	error = io_tcp_nodelay((SOCKET)(*stream)->fd, 1);

	if (error)
	{
		closesocket((SOCKET)(*stream)->fd);
		io_free(*stream);
		return error;
	}

	if (0 != bind((SOCKET)(*stream)->fd, address, length))
	{
		closesocket((SOCKET)(*stream)->fd);
		io_free(*stream);
		return EFAULT; // error_set_from_system(sys_error);
	}

	handle = CreateIoCompletionPort((HANDLE)(*stream)->fd, loop->iocp,
		(ULONG_PTR)&(*stream)->platform, 0);
	if (handle == NULL)
	{
		closesocket((SOCKET)(*stream)->fd);
		io_free(*stream);
		return EFAULT; // error_set_from_system(sys_error);
	}

	SetFileCompletionNotificationModes((HANDLE)(*stream)->fd,
		FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

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

	result = lpfnConnectEx((SOCKET)(*stream)->fd, address, length,
		NULL, 0,
		&dwSent, (LPOVERLAPPED)&(*stream)->platform.read);

	if (!result)
	{
		sys_error = WSAGetLastError();
		if (sys_error == ERROR_SUCCESS)
		{
			result = TRUE;
		}
		else if (sys_error != WSA_IO_PENDING)
		{
			// translate sys_error
		}
		else
		{
			result = TRUE;
			task_suspend(loop->current);
		}
	}

	if (timeout.time > 0)
	{
		moments_remove(&loop->timeouts, &timeout);
	}

	if (timeout.time > 0 && timeout.reached)
	{
		error = ETIMEDOUT;
	}

	if ((*stream)->info.status.closed ||
		(*stream)->info.status.shutdown)
	{
		error = EFAULT;
	}

	if (!error)
	{
		error = (*stream)->info.status.error;
	}


	if (error)
	{
		closesocket((SOCKET)(*stream)->fd);
		io_free(*stream);
		*stream = 0;

		return error;
	}
	
	(*stream)->info.type = IO_STREAM_TCP;
	(*stream)->loop = loop;
	io_stream_init(*stream);
	io_loop_ref(loop);

	return 0;
}