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

#include "loop.h"
#include "time.h"
#include "moment.h"
#include "atomic.h"

DECLARE_THREAD_LOCAL(io_loop_t*, loop, 0);

typedef struct io_handle_t {
	void(*processor)(struct io_handle_t* handle, DWORD transferred,
		OVERLAPPED* overlapped, io_loop_t* loop, DWORD error);
} io_handle_t;

/*
 * Internal API
 */

int io_loop_init(io_loop_t* loop)
{
	// Reset
	memset(loop, 0, sizeof(*loop));

	// Init task scheduler
	loop->current = &loop->main;
	loop->main.loop = loop;
	loop->prev = 0;

	// Init loop
	loop->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if (loop->iocp == 0)
	{
		// ToDo: translate from GetLastError()
		return ENOMEM;
	}

	mpscq_init(&loop->tasks);
	// mpscq_init(&loop->waiters);

	return 0;
}

int io_loop_cleanup(io_loop_t* loop)
{
	// ToDo: implement
	return 0;
}

int io_loop_wakeup(io_loop_t* loop)
{
	if (!PostQueuedCompletionStatus(loop->iocp, 0, 0, NULL))
	{
		// ToDo: translate from GetLastError()
		return ENOSYS;
	}

	return 0;
}

io_loop_t* io_loop_current()
{
	return get_thread_loop();
}

int io_loop_run(io_loop_t* loop)
{
	BOOL status;
	DWORD transfered;
	ULONG_PTR key;
	OVERLAPPED* overlapped;
	DWORD sys_error;
	BOOL failed;
	io_handle_t* handle;
	uint64_t timeout;
	uint64_t now;
	uint64_t nearest_event_time;
	int error;

	set_thread_loop(loop);

	// Execute entry if provided
	if (loop->entry)
	{
		io_loop_post(loop, loop->entry, loop->arg);
	}

	now = time_current();
	loop->last_activity_time = now;

	do
	{
		if (0 < moments_tick(&loop->sleeps, now))
		{
			now = time_current();
			loop->last_activity_time = now;
		}

		nearest_event_time = io_loop_nearest_event_time(loop);
		if (nearest_event_time == 0)
			timeout = -1;
		else
			timeout = (int)nearest_event_time;

		if (atomic_load64(&loop->shutdown))
		{
			break;
		}

		failed = 0;
		sys_error = 0;

		status = GetQueuedCompletionStatus(loop->iocp, &transfered, &key,
			&overlapped, (DWORD)timeout);

		now = time_current();

		if (status == FALSE)
		{
			sys_error = GetLastError();
			failed = 1;

			if (sys_error == ERROR_OPERATION_ABORTED ||
				sys_error == ERROR_CONNECTION_ABORTED)
			{
				/*
				* Handle was closed
				*/
				failed = 0;
				key = 0;
			}

			if (overlapped == NULL)
			{
				/*
				 * Completion port closed
				 */
				if (sys_error == ERROR_ABANDONED_WAIT_0)
				{
					failed = 1;
					break;
				}

				if (sys_error == WAIT_TIMEOUT)
				{
					failed = 0;
					key = 0;

					if (0 < moments_tick(&loop->idles, now))
					{
						now = time_current();
						loop->last_activity_time = now;
					}
				}
			}

			if (overlapped != NULL)
			{
				/*
				* Eof or Connection was closed
				*/
				if (transfered == 0)
				{
					failed = 0;
				}
			}
		}

		if (!failed)
		{
			if (key != 0)
			{
				handle = (io_handle_t*)key;
				handle->processor(handle, transfered, overlapped, loop, sys_error);
			}
			
			io_loop_process_tasks(loop);

			now = time_current();
			loop->last_activity_time = now;
		}

		moments_tick(&loop->timeouts, now);

		now = time_current();

	} while (!failed);

	if (0 != io_loop_cleanup(loop))
	{
		/* handle error */
	}

	error = 0;

	if (failed)
	{
		error = ENOSYS; // error_from_system(sys_error);
	}

	//if (loop->iocp != NULL)
	//{
	//	if (!CloseHandle(loop->iocp))
	//	{
	//		/* handle error when eopll not closed already */
	//	}
	//}

	return error;
}