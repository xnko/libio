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
#include "task.h"
#include "time.h"
#include "stopwatch.h"
#include "stream.h"

 /* read/write request */
typedef struct io_stream_req_t {
	task_t* task;
	uint32_t done;
} io_stream_req_t;

size_t io_stream_on_read(io_filter_t* filter, char* buffer, size_t length)
{
	io_stream_t* stream = filter->stream;
	io_stream_req_t read;
	moment_t timeout;
	uint64_t start, end, elapsed;
	WSABUF wsabuf;
	DWORD flags = 0;
	DWORD sys_error;
	BOOL completed = FALSE;

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

	read.done = 0;
	read.task = stream->loop->current;

	stream->platform.read_req = &read;

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

	switch (stream->info.type) {
		case IO_STREAM_FILE: {
			*(uint64_t*)&stream->platform.read.Offset =
				stream->impl.file.read_offset;

			completed = ReadFile(stream->fd, buffer, (DWORD)length,
				(LPDWORD)&read.done, &stream->platform.read);

			if (!completed)
			{
				sys_error = WSAGetLastError();
				if (sys_error == ERROR_SUCCESS)
				{
					completed = TRUE;
				}
				else if (sys_error != WSA_IO_PENDING)
				{
					completed = TRUE;
					stream->info.status.error = EFAULT; // api_error_translate(sys_error);
					stream->filters.head->on_status(stream->filters.head);
				}
			}

			break;
		}

		case IO_STREAM_TCP: {
			wsabuf.buf = buffer;
			wsabuf.len = (ULONG)length;
			completed = WSARecv((SOCKET)stream->fd, &wsabuf, 1,
				(LPDWORD)&read.done, &flags, &stream->platform.read, NULL);

			if (completed == SOCKET_ERROR)
			{
				sys_error = WSAGetLastError();
				if (sys_error == ERROR_SUCCESS)
				{
					// completed immediately
					completed = TRUE;
				}
				else if (sys_error == WSA_IO_PENDING)
				{
					// will complete asyncrounously
					completed = FALSE;
				}
				else
				{
					// error occured
					completed = TRUE;
					stream->info.status.error = EFAULT; // api_error_translate(sys_error);
					stream->filters.head->on_status(stream->filters.head);
				}
			}
			else
			{
				// completed immediately
				completed = TRUE;
			}

			break;
		}
	}

	if (!completed)
	{
		task_suspend(read.task);
	}

	end = stopwatch_measure();
	elapsed = stopwatch_nanoseconds(start, end);

	stream->info.read.bytes += read.done;
	stream->info.read.period += elapsed;

	if (timeout.time > 0)
	{
		moments_remove(&stream->loop->timeouts, &timeout);
	}

	if (stream->info.type == IO_STREAM_FILE)
	{
		stream->impl.file.read_offset += read.done;
	}

	if (timeout.time > 0 && timeout.reached)
	{
		stream->info.status.read_timeout = 1;
		stream->filters.head->on_status(stream->filters.head);

		return 0;
	}
	else
	{
		return (size_t)read.done;
	}
}

size_t io_stream_on_write(io_filter_t* filter, const char* buffer, size_t length)
{
	io_stream_t* stream = filter->stream;
	io_stream_req_t write;
	moment_t timeout;
	uint64_t start, end, elapsed;
	size_t offset = 0;
	WSABUF wsabuf;
	DWORD flags = 0;
	DWORD sys_error;
	BOOL completed = FALSE;

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

	write.done = 0;
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

	do
	{
		switch (stream->info.type) {
			case IO_STREAM_FILE: {
				*(uint64_t*)&stream->platform.write.Offset =
					stream->impl.file.write_offset;

				completed = WriteFile(stream->fd, buffer + offset,
					(DWORD)length - (DWORD)offset, (LPDWORD)&write.done,
					&stream->platform.write);

				if (!completed)
				{
					sys_error = WSAGetLastError();
					if (sys_error == ERROR_SUCCESS)
					{
						completed = TRUE;
					}
					else if (sys_error != WSA_IO_PENDING)
					{
						completed = TRUE;
						stream->info.status.error = EFAULT;// api_error_translate(sys_error);
						stream->filters.head->on_status(stream->filters.head);
					}
				}

				break;
			}

			case IO_STREAM_TCP: {
				wsabuf.buf = (char*)buffer;
				wsabuf.len = (DWORD)length;
				completed = WSASend((SOCKET)stream->fd, &wsabuf, 1,
					(LPDWORD)&write.done, 0, &stream->platform.write, NULL);

				if (completed == SOCKET_ERROR)
				{
					sys_error = WSAGetLastError();
					if (sys_error == ERROR_SUCCESS)
					{
						// completed immediately
						completed = TRUE;
					}
					else if (sys_error == WSA_IO_PENDING)
					{
						// will complete asyncrounously
						completed = FALSE;
					}
					else
					{
						// error occured
						completed = TRUE;
						stream->info.status.error = EFAULT; // api_error_translate(sys_error);
						stream->filters.head->on_status(stream->filters.head);
					}
				}
				else
				{
					// completed immediately
					completed = TRUE;
				}

				break;
			}
		}

		if (!completed)
		{
			task_suspend(write.task);
		}

		if (stream->info.type == IO_STREAM_FILE)
		{
			stream->impl.file.write_offset += write.done;
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

		offset += (size_t)write.done;
	}
	while (offset < length && write.done > 0);

	end = stopwatch_measure();
	elapsed = stopwatch_nanoseconds(start, end);

	stream->info.write.bytes += write.done;
	stream->info.write.period += elapsed;

	if (timeout.time > 0)
	{
		moments_remove(&stream->loop->timeouts, &timeout);
	}

	if (timeout.time > 0 && timeout.reached)
	{
		stream->info.status.write_timeout = 1;
		stream->filters.head->on_status(stream->filters.head);
	}

	return offset;
}

static void io_stream_on_status(io_filter_t* filter)
{
}

void io_stream_processor(void* e, DWORD transferred,
	OVERLAPPED* overlapped, io_loop_t* loop, DWORD error)
{
	io_stream_t* stream = container_of(e, io_stream_t, platform);

	io_stream_req_t* req;
	int is_read = 0;

	if (overlapped == &stream->platform.read)
	{
		req = (io_stream_req_t*)stream->platform.read_req;
		is_read = 1;
	}
	else
	{
		req = (io_stream_req_t*)stream->platform.write_req;
		is_read = 0;
	}

	/* we cannot remove handle from iocp on timeout, so handle it here and
	* just ignore when timeout happened on a stream
	*/
	if (req)
	{
		if (is_read)
		{
			if (stream->info.status.read_timeout)
			{
				return;
			}
		}
		else
		{
			if (stream->info.status.write_timeout)
			{
				return;
			}
		}

		req->done = transferred;
		if (transferred == 0)
		{
			if (error == ERROR_HANDLE_EOF)
				stream->info.status.eof = 1;
			else
				stream->info.status.error = ENOSYS; // api_error_translate(error);
		}

		task_resume(req->task);
	}
}

int io_stream_attach(io_stream_t* stream)
{
	HANDLE handle = 0;
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

	if (stream->info.type == IO_STREAM_FILE || stream->info.type == IO_STREAM_TCP)
	{
		handle = CreateIoCompletionPort((HANDLE)stream->fd, stream->loop->iocp,
			(UINT_PTR)&stream->platform, 0);

		if (handle == NULL)
			error = ENOSYS; // api_error_translate(GetLastError());
		else
		{
			SetFileCompletionNotificationModes((HANDLE)stream->fd,
				FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
		}
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

 /*
  * Internal API
  */

void io_stream_init(io_stream_t* stream)
{
	stream->platform.processor = io_stream_processor;

	io_filter_attach(&stream->operations, stream);

	stream->operations.on_status = io_stream_on_status;
	stream->operations.on_read = io_stream_on_read;
	stream->operations.on_write = io_stream_on_write;
}

/*
 * Public API
 */

int io_stream_delete(io_stream_t* stream)
{
	int error = 0;

	if (stream->info.status.closed)
	{
		return 0;
	}

	if (stream->info.type == IO_STREAM_FILE)
	{
		stream->info.status.closed = 1;
		CloseHandle((HANDLE)stream->fd);
		stream->fd = 0;
	}
	else if (stream->info.type == IO_STREAM_TCP)
	{
		stream->info.status.closed = 1;
		closesocket((SOCKET)stream->fd);
		stream->fd = 0;
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

	return error;
}