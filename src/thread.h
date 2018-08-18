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

#ifndef IO_THREAD_H_INCLUDED
#define IO_THREAD_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

#if PLATFORM_WINDOWS

#   define  WIN32_LEAN_AND_MEAN 1
#   include <Windows.h>
#   define IO_THREAD_FN(fn) unsigned int (__stdcall*fn)
#	define IO_THREAD_TYPE unsigned int __stdcall

typedef CRITICAL_SECTION io_mutex_t;
typedef CONDITION_VARIABLE io_condition_t;

static FORCEINLINE void io_mutex_init(io_mutex_t* mutex)
{
    InitializeCriticalSection(mutex);
}

static FORCEINLINE void io_mutex_destroy(io_mutex_t* mutex)
{
    DeleteCriticalSection(mutex);
}

static FORCEINLINE void io_mutex_lock(io_mutex_t* mutex)
{
    EnterCriticalSection(mutex);
}

static FORCEINLINE void io_mutex_unlock(io_mutex_t* mutex)
{
    LeaveCriticalSection(mutex);
}

static FORCEINLINE void io_condition_init(io_condition_t* condition)
{
    InitializeConditionVariable(condition);
}

static FORCEINLINE void io_condition_destroy(io_condition_t* condition)
{
    // Empty
}

static FORCEINLINE void io_condition_signal(io_condition_t* condition)
{
    WakeConditionVariable(condition);
}

static FORCEINLINE void io_condition_wait(io_condition_t* condition, io_mutex_t* mutex)
{
    SleepConditionVariableCS(condition, mutex, INFINITE);
}

#else

#   include <pthread.h>
#   define IO_THREAD_FN(fn) void* (*fn)
#	define IO_THREAD_TYPE void*

typedef pthread_mutex_t io_mutex_t;
typedef pthread_cond_t io_condition_t;

static FORCEINLINE void io_mutex_init(io_mutex_t* mutex)
{
    pthread_mutex_init(mutex, 0);
}

static FORCEINLINE void io_mutex_destroy(io_mutex_t* mutex)
{
    pthread_mutex_destroy(mutex);
}

static FORCEINLINE void io_mutex_lock(io_mutex_t* mutex)
{
    pthread_mutex_lock(mutex);
}

static FORCEINLINE void io_mutex_unlock(io_mutex_t* mutex)
{
    pthread_mutex_unlock(mutex);
}

static FORCEINLINE void io_condition_init(io_condition_t* condition)
{
    pthread_cond_init(condition, 0);
}

static FORCEINLINE void io_condition_destroy(io_condition_t* condition)
{
    pthread_cond_destroy(condition);
}

static FORCEINLINE void io_condition_signal(io_condition_t* condition)
{
    pthread_cond_signal(condition);
}

static FORCEINLINE void io_condition_wait(io_condition_t* condition, io_mutex_t* mutex)
{
    pthread_cond_wait(condition, mutex);
}

#endif

typedef IO_THREAD_FN(thread_fn)(void* arg);

int io_thread_create(thread_fn entry, void* arg);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_THREAD_H_INCLUDED