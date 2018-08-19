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

#include <sys/stat.h>
#include <errno.h>
#include "fs.h"
#include "threadpool.h"
#include "task.h"
#include "memory.h"

typedef struct io_file_req_t {
	const char* path;
	io_path_info_t* info;
	io_file_options_t options;
	HANDLE fd;
	int error;
} io_file_req_t;

void io_path_info_get_internal(io_work_t* work)
{
	io_file_req_t* req = (io_file_req_t*)work->arg;
	io_path_info_t* info = req->info;

	struct _stat64i32 s;
	_stat64i32(req->path, &s);

	info->time_access = s.st_atime;
	info->time_create = s.st_ctime;
	info->time_modified = s.st_mtime;
	info->size = s.st_size;

	req->error = 0;

	io_loop_post_task(work->loop, work->task);
}

void io_fs_open_internal(io_work_t* work)
{
	io_file_req_t* req = (io_file_req_t*)work->arg;
	int options = 0;

	if (req->options && IO_FILE_CREATE)
	{
		options |= (CREATE_ALWAYS);
	}
	else
	{
		options |= (OPEN_EXISTING);
	}

	if (req->options && IO_FILE_APPEND)
	{
		// by default
	}

	if (req->options && IO_FILE_TRUNCATE)
	{
		options |= TRUNCATE_EXISTING;
	}

	req->fd = CreateFileA(req->path, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
		options, FILE_FLAG_OVERLAPPED, NULL);
	
	if (req->fd == INVALID_HANDLE_VALUE)
	{
		req->error = EFAULT;
	}

	io_loop_post_task(work->loop, work->task);
}

void io_file_close_internal(io_work_t* work)
{
	io_file_req_t* req = (io_file_req_t*)work->arg;

	if (CloseHandle(req->fd))
	{
		req->error = 0;
	}
	else
	{
		req->error = EFAULT;
	}

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
	int error = 0;

	open.path = path;
	open.options = options;
	open.error = 0;

	work.arg = &open;
	work.entry = io_fs_open_internal;

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