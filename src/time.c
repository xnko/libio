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

#include <time.h>
#include "time.h"
#include "platform.h"

#if PLATFORM_LINUX
#   include <sys/time.h>
#endif

#if PLATFORM_MACOSX
#   include <mach/clock.h>
#   include <mach/mach.h>
#endif

#define THOUSAND    1000UL
#define MILLION     (1000 * THOUSAND)
#define BILLION     (1000 * MILLION)

// 1 second = BILLION nanoseconds

/*
 * API
 */
uint64_t time_current()
{
#if PLATFORM_WINDOWS

    FILETIME ft;
    LARGE_INTEGER li;
    uint64_t ret;

    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    ret = li.QuadPart;
    ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
    ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

    return ret;

#else

    struct timeval tv;       
    if (gettimeofday(&tv, 0) != 0)
    {
        return 0;
    }

    return (uint64_t)((tv.tv_sec * 1000ul) + (tv.tv_usec / 1000ul));

#endif
}