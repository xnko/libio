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
#include "stopwatch.h"
#include "platform.h"

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

#if PLATFORM_WINDOWS
uint64_t stopwatch_measure()
{
    LARGE_INTEGER counter;

    QueryPerformanceCounter(&counter);

    return counter.QuadPart;
}
#else
uint64_t stopwatch_measure()
{
    struct timespec ts;

#if PLATFORM_MACOSX

    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), HIGHRES_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;

#else // Not Windows, not MacOS

    clock_gettime(CLOCK_MONOTONIC, &ts);

#endif

    return (((uint64_t)ts.tv_sec) << 32) | ts.tv_nsec;
}
#endif

uint64_t stopwatch_nanoseconds(uint64_t start, uint64_t end)
{
#if PLATFORM_WINDOWS

    static LARGE_INTEGER frequency = { 0 }; // ticks per second
    uint64_t ticks;
    
    if (frequency.QuadPart == 0)
        QueryPerformanceFrequency(&frequency);

    if (start > end)
        ticks = start - end;
    else
        ticks = end - start;

	uint64_t sec = ticks / frequency.QuadPart;
    uint64_t rest = ticks % frequency.QuadPart;
	uint64_t nsec = (rest * BILLION) / frequency.QuadPart;

	return (sec * BILLION) + nsec;

#else

    uint64_t ssec = start >> 32;
    uint64_t snsec = start & 0xffffffff;
    uint64_t esec = end >> 32;
    uint64_t ensec = end & 0xffffffff;

    uint64_t sec, nsec;

    if (start > end)
    {
        if (snsec < ensec) {
            sec = ssec - esec - 1;
            nsec = snsec + BILLION - ensec;
        } else {
            sec = ssec - esec;
            nsec = snsec - ensec;
        }
    }
    else
    {
        if (ensec < snsec) {
            sec = esec - ssec - 1;
            nsec = ensec + BILLION - snsec;
        } else {
            sec = esec - ssec;
            nsec = ensec - snsec;
        }
    }
    
    return (sec * BILLION) + nsec;

#endif
}