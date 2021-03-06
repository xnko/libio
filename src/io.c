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
#include "event.h"
#include "threadpool.h"
#include "tcp.h"

#if PLATFORM_WINDOWS && IO_BUILD_SHARED

BOOL WINAPI DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}

#endif

int io_run(io_loop_fn entry, void* arg)
{
	int error;
	io_loop_t loop;

	error = io_tcp_init();
	if (error)
	{
		return error;
	}

    error = io_threadpool_init();
    if (error)
    {
        return error;
    }

	error = io_event_init();
	if (error)
	{
        io_threadpool_shutdown();
		return error;
	}

	error = io_loop_init(&loop);
	if (error)
	{
		io_event_shutdown();
        io_threadpool_shutdown();
		return error;
	}

	io_loop_ref(&loop);

	loop.entry = entry;
	loop.arg = arg;

	error = io_loop_run(&loop);
	io_event_shutdown();
    io_threadpool_shutdown();

	return error;
}