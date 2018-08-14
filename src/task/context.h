/* Copyright (c) 2014, Artak Khnkoyan <artak.khnkoyan@gmail.com>
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

#ifndef TASK_CONTEXT_H_INCLUDED
#define TASK_CONTEXT_H_INCLUDED

/* Copyright (c) 2005-2006 Russ Cox, MIT; see COPYRIGHT */

#if defined(__sun__)
#   define __EXTENSIONS__ 1 /* SunOS */
#   if defined(__SunOS5_6__) || defined(__SunOS5_7__) || defined(__SunOS5_8__)
        /* NOT USING #define __MAKECONTEXT_V2_SOURCE 1 / * SunOS */
#   else
#       define __MAKECONTEXT_V2_SOURCE 1
#   endif
#endif

#define USE_UCONTEXT 1

#if defined(__OpenBSD__) || defined(__mips__)
#   undef   USE_UCONTEXT
#   define  USE_UCONTEXT 0
#endif

#if defined(__APPLE__)
#   include <AvailabilityMacros.h>
#   if defined(MAC_OS_X_VERSION_10_5)
#       undef   USE_UCONTEXT
#       define  USE_UCONTEXT 0
#   endif
#endif

#ifndef WIN32
#   include <errno.h>
#   include <stdlib.h>
#   include <unistd.h>
#   include <string.h>
#   include <assert.h>
#   include <time.h>
#   include <sys/time.h>
#   include <sys/types.h>
#   include <sys/wait.h>
#   include <sched.h>
#   include <signal.h>
#   if USE_UCONTEXT
#       include <ucontext.h>
#   endif
#   include <sys/utsname.h>
#   include <inttypes.h>
#endif

#if defined(__FreeBSD__) && __FreeBSD__ < 5
extern  int     getmcontext(mcontext_t*);
extern  void    setmcontext(const mcontext_t*);
#define setcontext(u)   setmcontext(&(u)->uc_mcontext)
#define getcontext(u)   getmcontext(&(u)->uc_mcontext)
extern  int     swapcontext(ucontext_t*, const ucontext_t*);
extern  void    makecontext(ucontext_t*, void(*)(), int, ...);
#endif

#if defined(__APPLE__)
#   define mcontext libthread_mcontext
#   define mcontext_t libthread_mcontext_t
#   define ucontext libthread_ucontext
#   define ucontext_t libthread_ucontext_t
#   if defined(__i386__)
#       include "386-ucontext.h"
#   elif defined(__x86_64__)
#       include "amd64-ucontext.h"
#   else
#       include "power-ucontext.h"
#   endif	
#endif

#if defined(__OpenBSD__)
#   define mcontext libthread_mcontext
#   define mcontext_t libthread_mcontext_t
#   define ucontext libthread_ucontext
#   define ucontext_t libthread_ucontext_t
#   if defined __i386__
#       include "386-ucontext.h"
#   else
#       include "power-ucontext.h"
#   endif
extern pid_t rfork_thread(int, void*, int(*)(void*), void*);
#endif

#if 0 &&  defined(__sun__)
#   define mcontext libthread_mcontext
#   define mcontext_t libthread_mcontext_t
#   define ucontext libthread_ucontext
#   define ucontext_t libthread_ucontext_t
#   include "sparc-ucontext.h"
#endif

#if defined(__arm__)
int getmcontext(mcontext_t*);
void setmcontext(const mcontext_t*);
#define	setcontext(u)	setmcontext(&(u)->uc_mcontext)
#define	getcontext(u)	getmcontext(&(u)->uc_mcontext)
#endif

#if defined(__mips__)
#include "mips-ucontext.h"
int getmcontext(mcontext_t*);
void setmcontext(const mcontext_t*);
#define	setcontext(u)	setmcontext(&(u)->uc_mcontext)
#define	getcontext(u)	getmcontext(&(u)->uc_mcontext)
#endif

#if defined(_WIN32)
#   include "win-ucontext.h"
#endif

#endif // TASK_CONTEXT_H_INCLUDED