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

#ifndef IO_ATOMIC_H_INCLUDED
#define IO_ATOMIC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

#if PLATFORM_APPLE
#   include <libkern/OSAtomic.h>
#endif

struct atomic32_t {
	uint32_t nonatomic;
};
typedef ALIGN(4) struct atomic32_t atomic32_t;

struct atomic64_t {
	uint64_t nonatomic;
};
typedef ALIGN(8) struct atomic64_t atomic64_t;

struct atomicptr_t {
	void* nonatomic;
};
typedef ALIGN(POINTER_SIZE) struct atomicptr_t atomicptr_t;

static FORCEINLINE uint32_t atomic_load32(atomic32_t* src);
static FORCEINLINE uint64_t atomic_load64(atomic64_t* src);
static FORCEINLINE void* atomic_loadptr(atomicptr_t* src);
static FORCEINLINE void atomic_store32(atomic32_t* dst, uint32_t val);
static FORCEINLINE void atomic_store64(atomic64_t* dst, uint64_t val);
static FORCEINLINE void atomic_storeptr(atomicptr_t* dst, void* val);
static FORCEINLINE uint32_t atomic_exchange_and_add32(atomic32_t* val, uint32_t add);
static FORCEINLINE uint32_t atomic_add32(atomic32_t* val, uint32_t add);
static FORCEINLINE uint32_t atomic_incr32(atomic32_t* val);
static FORCEINLINE uint32_t atomic_decr32(atomic32_t* val);
static FORCEINLINE uint64_t atomic_exchange_and_add64(atomic64_t* val, uint64_t add);
static FORCEINLINE uint64_t atomic_add64(atomic64_t* val, uint64_t add);
static FORCEINLINE uint64_t atomic_incr64(atomic64_t* val);
static FORCEINLINE uint64_t atomic_decr64(atomic64_t* val);
static FORCEINLINE bool atomic_cas32(atomic32_t* dst, uint32_t cmp, uint32_t set);
static FORCEINLINE bool atomic_cas64(atomic64_t* dst, uint64_t cmp, uint64_t set);
static FORCEINLINE bool atomic_cas_ptr(atomicptr_t* dst, void* cmp, void* set);

/*
 * Implementations
 */

static FORCEINLINE uint32_t atomic_load32(atomic32_t* val)
{
    return val->nonatomic;
}

static FORCEINLINE uint64_t atomic_load64(atomic64_t* val)
{
#if ARCHITECTURE_X86
    uint64_t result;
#   if COMPILER_MSVC || COMPILER_INTEL
    __asm
    {
        mov esi, val;
        mov ebx, eax;
        mov ecx, edx;
        lock cmpxchg8b [esi];
        mov dword ptr result, eax;
        mov dword ptr result[4], edx;
    }
#   else
    asm volatile(
        "movl %%ebx, %%eax\n"
        "movl %%ecx, %%edx\n"
        "lock; cmpxchg8b %1"
        : "=&A"(result)
        : "m"(val->nonatomic));
#   endif
    return result;
#else
    return val->nonatomic;
#endif
}

static FORCEINLINE void* atomic_loadptr(atomicptr_t* val)
{
    return val->nonatomic;
}

static FORCEINLINE void atomic_store32(atomic32_t* dst, uint32_t val)
{
    dst->nonatomic = val;
}

static FORCEINLINE void atomic_store64(atomic64_t* dst, uint64_t val)
{
#if ARCHITECTURE_X86
#   if COMPILER_MSVC || COMPILER_INTEL
    __asm
    {
        mov esi, dst;
        mov ebx, dword ptr val;
        mov ecx, dword ptr val[4];
    retry:
        cmpxchg8b [esi];
        jne retry;
    }
#   else
    uint64_t expected = dst->nonatomic;
#       if COMPILER_GCC
            __sync_bool_compare_and_swap(&dst->nonatomic, expected, val);
#       else
            asm volatile(
            "1:    cmpxchg8b %0\n"
            "      jne 1b"
            : "=m"(dst->nonatomic)
            : "b"((uint32_t)val), "c"((uint32_t)(val >> 32)), "A"(expected));
#       endif
#   endif
#else
    dst->nonatomic = val;
#endif
}

static FORCEINLINE void atomic_storeptr(atomicptr_t* dst, void* val)
{
    dst->nonatomic = val;
}

static FORCEINLINE uint32_t atomic_exchange_and_add32(atomic32_t* val, uint32_t add)
{
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
    return _InterlockedExchangeAdd((volatile long*)&val->nonatomic, add);
#elif COMPILER_GCC || COMPILER_CLANG
    return __sync_fetch_and_add(&val->nonatomic, add);
#else
#   error Not implemented
#endif
}


static FORCEINLINE uint32_t atomic_add32(atomic32_t* val, uint32_t add)
{
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
    uint32_t old = (uint32_t)_InterlockedExchangeAdd((volatile long*)&val->nonatomic, (long)add);
    return (old + add);
#elif PLATFORM_APPLE
    return OSAtomicAdd32(add, &val->nonatomic);
#elif COMPILER_GCC || COMPILER_CLANG
    return __sync_add_and_fetch(&val->nonatomic, add);
#else 
#   error Not implemented
#endif
}

static FORCEINLINE uint32_t atomic_incr32(atomic32_t* val) { return atomic_add32(val, 1); }
static FORCEINLINE uint32_t atomic_decr32(atomic32_t* val) { return atomic_add32(val, -1); }


static FORCEINLINE uint64_t atomic_exchange_and_add64(atomic64_t* val, uint64_t add)
{
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
#   if ARCHITECTURE_X86
    unsigned long long ref;
    do { ref = val->nonatomic; } while (_InterlockedCompareExchange64((volatile long long*)&val->nonatomic, ref + add, ref) != ref);
    return ref;
#   else /* X86_64 */
    return _InterlockedExchangeAdd64(&val->nonatomic, add);
#   endif
#elif PLATFORM_APPLE
	uint64_t ref;
	do { ref = (int64_t)val->nonatomic; } while(!OSAtomicCompareAndSwap64(ref, ref + add, (int64_t*)&val->nonatomic));
	return ref;
#elif COMPILER_GCC || COMPILER_CLANG
    return __sync_fetch_and_add(&val->nonatomic, add);
#else 
#   error Not implemented
#endif
}


static FORCEINLINE uint64_t atomic_add64(atomic64_t* val, uint64_t add)
{
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
#   if ARCHITECTURE_X86
    return atomic_exchange_and_add64(val, add) + add;
#   else
    return _InterlockedExchangeAdd64(&val->nonatomic, (long long)add) + add;
#   endif
#elif PLATFORM_APPLE
    return OSAtomicAdd64(add, &val->nonatomic);
#elif COMPILER_GCC || COMPILER_CLANG
    return __sync_add_and_fetch(&val->nonatomic, add);
#else 
#   error Not implemented
#endif
}

static FORCEINLINE uint64_t atomic_incr64(atomic64_t* val) { return atomic_add64(val, 1LL); }
static FORCEINLINE uint64_t atomic_decr64(atomic64_t* val) { return atomic_add64(val, -1LL); }

static FORCEINLINE bool atomic_cas32(atomic32_t* dst, uint32_t cmp, uint32_t set)
{
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
    return (_InterlockedCompareExchange((volatile long*)&dst->nonatomic, set, cmp) == cmp) ? true : false;
#elif PLATFORM_APPLE
    return OSAtomicCompareAndSwap32(cmp, set, &dst->nonatomic);
#elif COMPILER_GCC || COMPILER_CLANG
    return __sync_bool_compare_and_swap(&dst->nonatomic, cmp, set);
#else 
#   error Not implemented
#endif
}

static FORCEINLINE bool atomic_cas64(atomic64_t* dst, uint64_t cmp, uint64_t set)
{
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
    return (_InterlockedCompareExchange64((volatile long long*)&dst->nonatomic, set, cmp) == cmp) ? true : false;
#elif PLATFORM_APPLE
    return OSAtomicCompareAndSwap64(cmp, set, dst->nonatomic);
#elif COMPILER_GCC || COMPILER_CLANG
    return __sync_bool_compare_and_swap(&dst->nonatomic, cmp, set);
#else 
#   error Not implemented
#endif
}

static FORCEINLINE bool atomic_cas_ptr(atomicptr_t* dst, void* cmp, void* set)
{
#if POINTER_SIZE == 8
    return atomic_cas64((atomic64_t*)dst, (uint64_t)set, (uint64_t)cmp);
#elif POINTER_SIZE == 4
    return atomic_cas32((atomic32_t*)dst, (uint32_t)set, (uint32_t)cmp);
#else
#   error Unknown architecture (pointer size)
#endif
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_ATOMIC_H_INCLUDED