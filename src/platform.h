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

/*
 * Mostly taken from https://github.com/rampantpixels/foundation_lib
 */

#ifndef IO_PLATFORM_H_INCLUDED
#define IO_PLATFORM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  PLATFORM_ANDROID
 *  PLATFORM_IOS
 *  PLATFORM_IOS_SIMULATOR
 *  PLATFORM_MACOS
 *  PLATFORM_LINUX
 *  PLATFORM_LINUX_RASPBERRYPI
 *  PLATFORM_BSD
 *  PLATFORM_WINDOWS
 
 *  PLATFORM_APPLE - OSX, iOS
 *  PLATFORM_POSIX - Linux, BSD, OSX, iOS, Android
 *
 *  PLATFORM_FAMILY_MOBILE - iOS, Android
 *  PLATFORM_FAMILY_DESKTOP - Windows, OSX, Linux, BSD
 *
 *  PLATFORM_NAME
 *  PLATFORM_DESCRIPTION
 *
 *  ARCHITECTURE_ARM
 *  ARCHITECTURE_ARM6
 *  ARCHITECTURE_ARM7
 *  ARCHITECTURE_ARM8
 *  ARCHITECTURE_ARM_64
 *  ARCHITECTURE_ARM8_64
 *  ARCHITECTURE_X86
 *  ARCHITECTURE_X86_64
 *  ARCHITECTURE_PPC
 *  ARCHITECTURE_PPC_64
 *  ARCHITECTURE_IA64
 *  ARCHITECTURE_SSE2
 *  ARCHITECTURE_SSE3
 *  ARCHITECTURE_SSE4
 *  ARCHITECTURE_SSE4_FMA3
 *  ARCHITECTURE_NEON
 *  ARCHITECTURE_THUMB
 *  ARCHITECTURE_ENDIAN_LITTLE
 *  ARCHITECTURE_ENDIAN_BIG
 *
 *  COMPILER_CLANG
 *  COMPILER_INTEL
 *  COMPILER_GCC
 *  COMPILER_MSVC
 *
 *  COMPILER_NAME
 *  COMPILER_DESCRIPTION
 *
 *  TEXTIFY
 *
 *  RESTRICT
 *  THREADLOCAL
 *  DEPRECATED
 *  FORCEINLINE
 *  NOINLINE
 *  PURECALL
 *  CONSTCALL
 *  ALIGN
 *  ALIGNOF
 *
 *  LIKELY
 *  UNLIKELY
 *
 *  POINTER_SIZE
 *  WCHAR_SIZE
 *
 *  DECLARE_THREAD_LOCAL
 */

/* Platforms */
#define PLATFORM_ANDROID 0
#define PLATFORM_IOS 0
#define PLATFORM_IOS_SIMULATOR 0
#define PLATFORM_MACOSX 0
#define PLATFORM_LINUX 0
#define PLATFORM_LINUX_RASPBERRYPI 0
#define PLATFORM_BSD 0
#define PLATFORM_WINDOWS 0

/* Platform traits */
#define PLATFORM_APPLE 0
#define PLATFORM_POSIX 0

#define PLATFORM_FAMILY_MOBILE 0
#define PLATFORM_FAMILY_DESKTOP 0


/* Architectures */
#define ARCHITECTURE_ARM 0
#define ARCHITECTURE_ARM5 0
#define ARCHITECTURE_ARM6 0
#define ARCHITECTURE_ARM7 0
#define ARCHITECTURE_ARM8 0
#define ARCHITECTURE_ARM_64 0
#define ARCHITECTURE_ARM8_64 0
#define ARCHITECTURE_X86 0
#define ARCHITECTURE_X86_64 0
#define ARCHITECTURE_PPC 0
#define ARCHITECTURE_PPC_64 0
#define ARCHITECTURE_IA64 0
#define ARCHITECTURE_MIPS 0
#define ARCHITECTURE_MIPS_64 0

/* Architecture details */
#define ARCHITECTURE_SSE2 0
#define ARCHITECTURE_SSE3 0
#define ARCHITECTURE_SSE4 0
#define ARCHITECTURE_SSE4_FMA3 0
#define ARCHITECTURE_NEON 0
#define ARCHITECTURE_THUMB 0

#define ARCHITECTURE_ENDIAN_LITTLE 0
#define ARCHITECTURE_ENDIAN_BIG 0
	

/* Compilers */
#define COMPILER_CLANG  0 /* Clang/LLVM */
#define COMPILER_INTEL  0 /* Intel ICC/ICPC */
#define COMPILER_GCC    0 /* GNU GCC/G++ */
#define COMPILER_MSVC   0 /* Microsoft Visual Studio */


/* First, platforms and architectures */

/* Android */
#if defined(__ANDROID__)

#   undef   PLATFORM_ANDROID
#   define  PLATFORM_ANDROID 1

/* Compatibile platforms */
#   undef   PLATFORM_POSIX
#   define  PLATFORM_POSIX 1

#   define  PLATFORM_NAME "Android"

/* Architecture and detailed description */
#   if defined(__arm__)
#       undef   ARCHITECTURE_ARM
#       define  ARCHITECTURE_ARM 1
#       ifdef __ARM_ARCH_7A__
#           undef   ARCHITECTURE_ARM7
#           define  ARCHITECTURE_ARM7 1
#           define  PLATFORM_DESCRIPTION "Android ARMv7"
#       elif defined(__ARM_ARCH_5TE__)
#           undef   ARCHITECTURE_ARM5
#           define  ARCHITECTURE_ARM5 1
#           define  PLATFORM_DESCRIPTION "Android ARMv5"
#       else
#           error Unsupported ARM architecture
#       endif
#   elif defined( __aarch64__ )
#       undef   ARCHITECTURE_ARM
#       define  ARCHITECTURE_ARM 1
#       undef   ARCHITECTURE_ARM_64
#       define  ARCHITECTURE_ARM_64 1
/* Assume ARMv8 for now */
/* #    if defined( __ARM_ARCH ) && ( __ARM_ARCH == 8 ) */
#       undef   ARCHITECTURE_ARM8_64
#       define  ARCHITECTURE_ARM8_64 1
#       define  PLATFORM_DESCRIPTION "Android ARM64v8"
/* #    else
 * #      error Unrecognized AArch64 architecture
 * #    endif
 */
#   elif defined(__i386__)
#       undef   ARCHITECTURE_X86
#       define  ARCHITECTURE_X86 1
#       define  PLATFORM_DESCRIPTION "Android x86"
#   elif defined(__x86_64__)
#       undef   ARCHITECTURE_X86_64
#       define  ARCHITECTURE_X86_64 1
#       define  PLATFORM_DESCRIPTION "Android x86-64"
#   elif (defined(__mips__) && defined(__mips64))
#       undef   ARCHITECTURE_MIPS
#       define  ARCHITECTURE_MIPS 1
#       undef   ARCHITECTURE_MIPS_64
#       define  ARCHITECTURE_MIPS_64 1
#       define  PLATFORM_DESCRIPTION "Android MIPS64"
#       ifndef _MIPS_ISA
#           define  _MIPS_ISA 7 /*_MIPS_ISA_MIPS64*/
#       endif
#   elif defined(__mips__)
#       undef   ARCHITECTURE_MIPS
#       define  ARCHITECTURE_MIPS 1
#       define  PLATFORM_DESCRIPTION "Android MIPS"
#       include <asm/asm.h>
#       ifndef  _MIPS_ISA
#           define  _MIPS_ISA 6 /*_MIPS_ISA_MIPS32*/
#       endif
#       error Unknown architecture
#   endif

/* Traits */
#   if defined(__AARCH64EB__)
#       undef   ARCHITECTURE_ENDIAN_BIG
#       define  ARCHITECTURE_ENDIAN_BIG 1
#   else
#       undef   ARCHITECTURE_ENDIAN_LITTLE
#       define  ARCHITECTURE_ENDIAN_LITTLE 1
#   endif

#   undef   PLATFORM_FAMILY_MOBILE
#   define  PLATFORM_FAMILY_MOBILE 1

/* Workarounds for weird include dependencies in NDK headers */
#   if !defined(__LP64__)
#       define __LP64__ 0
#       define PLATFORM_ANDROID_LP64_WORKAROUND
#   endif

/* MacOS X and iOS */
#elif (defined(__APPLE__) && __APPLE__)

#   undef   PLATFORM_APPLE
#   define  PLATFORM_APPLE 1

#   undef   PLATFORM_POSIX
#   define  PLATFORM_POSIX 1

#   include <TargetConditionals.h>

#   if defined(__IPHONE__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR)

#       undef   PLATFORM_IOS
#       define  PLATFORM_IOS 1

#       define  PLATFORM_NAME "iOS"

#       if defined(__arm__)
#           undef   ARCHITECTURE_ARM
#           define  ARCHITECTURE_ARM 1
#           if defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7S__)
#               undef   ARCHITECTURE_ARM7
#               define  ARCHITECTURE_ARM7 1
#               define  PLATFORM_DESCRIPTION "iOS ARMv7"
#               ifndef __ARM_NEON__
#                   error Missing ARM NEON support
#               endif
#           elif defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6__)
#               undef   ARCHITECTURE_ARM6
#               define  ARCHITECTURE_ARM6 1
#               define  PLATFORM_DESCRIPTION "iOS ARMv6"
#           else
#               error Unrecognized ARM architecture
#           endif
#       elif defined(__arm64__)
#           undef   ARCHITECTURE_ARM
#           define  ARCHITECTURE_ARM 1
#           undef   ARCHITECTURE_ARM_64
#           define  ARCHITECTURE_ARM_64 1
#           if defined(__ARM64_ARCH_8__)
#               undef   ARCHITECTURE_ARM8_64
#               define  ARCHITECTURE_ARM8_64 1
#               define  PLATFORM_DESCRIPTION "iOS ARM64v8"
#           else
#               error Unrecognized ARM architecture
#           endif
#       elif defined(__i386__)
#           undef   PLATFORM_IOS_SIMULATOR
#           define  PLATFORM_IOS_SIMULATOR 1
#           undef   ARCHITECTURE_X86
#           define  ARCHITECTURE_X86 1
#           define  PLATFORM_DESCRIPTION "iOS x86 (simulator)"
#       elif defined(__x86_64__)
#           undef   PLATFORM_IOS_SIMULATOR
#           define  PLATFORM_IOS_SIMULATOR 1
#           undef   ARCHITECTURE_X86_64
#           define  ARCHITECTURE_X86_64 1
#           define  PLATFORM_DESCRIPTION "iOS x86_64 (simulator)"
#       else
#           error Unknown architecture
#       endif

#       undef   ARCHITECTURE_ENDIAN_LITTLE
#       define  ARCHITECTURE_ENDIAN_LITTLE 1

#       undef   PLATFORM_FAMILY_MOBILE
#       define  PLATFORM_FAMILY_MOBILE 1

#   elif defined(__MACH__)

#       undef   PLATFORM_MACOSX
#       define  PLATFORM_MACOSX 1

#       define  PLATFORM_NAME "MacOSX"

#       if defined(__x86_64__) || defined(__x86_64) || defined(__amd64)
#           undef   ARCHITECTURE_X86_64
#           define  ARCHITECTURE_X86_64 1
#           undef   ARCHITECTURE_ENDIAN_LITTLE
#           define  ARCHITECTURE_ENDIAN_LITTLE 1
#           define  PLATFORM_DESCRIPTION "MacOSX x86-64"
#       elif defined(__i386__) || defined(__intel__)
#           undef   ARCHITECTURE_X86
#           define  ARCHITECTURE_X86 1
#           undef   ARCHITECTURE_ENDIAN_LITTLE
#           define  ARCHITECTURE_ENDIAN_LITTLE 1
#           define  PLATFORM_DESCRIPTION "MacOSX x86"
#       elif defined(__powerpc64__) || defined(__POWERPC64__)
#           undef   ARCHITECTURE_PPC_64
#           define  ARCHITECTURE_PPC_64 1
#           undef   ARCHITECTURE_ENDIAN_BIG
#           define  ARCHITECTURE_ENDIAN_BIG 1
#           define  PLATFORM_DESCRIPTION "MacOSX PPC64"
#       elif defined(__powerpc__) || defined(__POWERPC__)
#           undef   ARCHITECTURE_PPC
#           define  ARCHITECTURE_PPC 1
#           undef   ARCHITECTURE_ENDIAN_BIG
#           define  ARCHITECTURE_ENDIAN_BIG 1
#           define  PLATFORM_DESCRIPTION "MacOSX PPC"
#       else
#           error Unknown architecture
#       endif

#       undef   PLATFORM_FAMILY_DESKTOP
#       define  PLATFORM_FAMILY_DESKTOP 1

#   else
#       error Unknown Apple Platform
#   endif

/* Linux */
#elif (defined(__linux__) || defined(__linux))

#   undef   PLATFORM_LINUX
#   define  PLATFORM_LINUX 1

#   undef   PLATFORM_POSIX
#   define  PLATFORM_POSIX 1

#   define  PLATFORM_NAME "Linux"

#   if defined(__x86_64__) || defined(__x86_64) || defined(__amd64)
#       undef   ARCHITECTURE_X86_64
#       define  ARCHITECTURE_X86_64 1
#       undef   ARCHITECTURE_ENDIAN_LITTLE
#       define  ARCHITECTURE_ENDIAN_LITTLE 1
#       define  PLATFORM_DESCRIPTION "Linux x86-64"
#   elif defined(__i386__) || defined(__intel__) || defined(_M_IX86)
#       undef   ARCHITECTURE_X86
#       define  ARCHITECTURE_X86 1
#       undef   ARCHITECTURE_ENDIAN_LITTLE
#       define  ARCHITECTURE_ENDIAN_LITTLE 1
#       define  PLATFORM_DESCRIPTION "Linux x86"
#   elif defined(__powerpc64__) || defined(__POWERPC64__)
#       undef   ARCHITECTURE_PPC_64
#       define  ARCHITECTURE_PPC_64 1
#       undef   ARCHITECTURE_ENDIAN_BIG
#       define  ARCHITECTURE_ENDIAN_BIG 1
#       define  PLATFORM_DESCRIPTION "Linux PPC64"
#   elif defined(__powerpc__) || defined(__POWERPC__)
#       undef   ARCHITECTURE_PPC
#       define  ARCHITECTURE_PPC 1
#       undef   ARCHITECTURE_ENDIAN_BIG
#       define  ARCHITECTURE_ENDIAN_BIG 1
#       define  PLATFORM_DESCRIPTION "Linux PPC"
#   elif defined(__arm__)
#       undef   ARCHITECTURE_ARM
#       define  ARCHITECTURE_ARM 1
#       undef   ARCHITECTURE_ENDIAN_LITTLE
#       define  ARCHITECTURE_ENDIAN_LITTLE 1
#       if defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7S__)
#           undef   ARCHITECTURE_ARM7
#           define  ARCHITECTURE_ARM7 1
#           define  PLATFORM_DESCRIPTION "Linux ARMv7"
#           ifndef __ARM_NEON__
#               error Missing ARM NEON support
#           endif
#       elif defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6ZK__)
#           undef   ARCHITECTURE_ARM6
#           define  ARCHITECTURE_ARM6 1
#           define  PLATFORM_DESCRIPTION "Linux ARMv6"
#       else
#           error Unrecognized ARM architecture
#       endif
#   elif defined( __arm64__ )
#       undef   ARCHITECTURE_ARM
#       define  ARCHITECTURE_ARM 1
#       undef   ARCHITECTURE_ARM_64
#       define  ARCHITECTURE_ARM_64 1
#       undef   ARCHITECTURE_ENDIAN_LITTLE
#       define  ARCHITECTURE_ENDIAN_LITTLE 1
#       if defined( __ARM64_ARCH_8__ )
#           undef   ARCHITECTURE_ARM8_64
#           define  ARCHITECTURE_ARM8_64 1
#           define  PLATFORM_DESCRIPTION "Linux ARM64v8"
#       else
#           error Unrecognized ARM architecture
#       endif
#   else
#       error Unknown architecture
#   endif

#   if defined(__raspberrypi__)
#       undef   PLATFORM_LINUX_RASPBERRYPI
#       define  PLATFORM_LINUX_RASPBERRYPI 1
#   endif

#   undef   PLATFORM_FAMILY_DESKTOP
#   define  PLATFORM_FAMILY_DESKTOP 1

#   ifndef _GNU_SOURCE
#       define _GNU_SOURCE
#   endif

/* BSD family */
#elif (defined(__BSD__) || defined(__FreeBSD__))

#   undef   PLATFORM_BSD
#   define  PLATFORM_BSD 1

#   undef   PLATFORM_POSIX
#   define  PLATFORM_POSIX 1

#   define  PLATFORM_NAME "BSD"

#   if defined(__x86_64__) || defined(__x86_64) || defined(__amd64)
#       undef   ARCHITECTURE_X86_64
#       define  ARCHITECTURE_X86_64 1
#       undef   ARCHITECTURE_ENDIAN_LITTLE
#       define  ARCHITECTURE_ENDIAN_LITTLE 1
#       define  PLATFORM_DESCRIPTION "BSD x86-64"
#   elif defined(__i386__) || defined(__intel__) || defined(_M_IX86)
#       undef   ARCHITECTURE_X86
#       define  ARCHITECTURE_X86 1
#       undef   ARCHITECTURE_ENDIAN_LITTLE
#       define  ARCHITECTURE_ENDIAN_LITTLE 1
#       define  PLATFORM_DESCRIPTION "BSD x86"
#   elif defined(__powerpc64__) || defined(__POWERPC64__)
#       undef   ARCHITECTURE_PPC_64
#       define  ARCHITECTURE_PPC_64 1
#       undef   ARCHITECTURE_ENDIAN_BIG
#       define  ARCHITECTURE_ENDIAN_BIG 1
#       define  PLATFORM_DESCRIPTION "BSD PPC64"
#   elif defined(__powerpc__) || defined(__POWERPC__)
#       undef   ARCHITECTURE_PPC
#       define  ARCHITECTURE_PPC 1
#       undef   ARCHITECTURE_ENDIAN_BIG
#       define  ARCHITECTURE_ENDIAN_BIG 1
#       define  PLATFORM_DESCRIPTION "BSD PPC"
#   else
#       error Unknown architecture
#   endif

#   undef   PLATFORM_FAMILY_DESKTOP
#   define  PLATFORM_FAMILY_DESKTOP 1

/* Windows */
#elif defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)

#   undef   PLATFORM_WINDOWS
#   define  PLATFORM_WINDOWS 1

#   define  PLATFORM_NAME "Windows"

#   if defined(__x86_64__) || defined(_M_AMD64) || defined(_AMD64_)
#       undef   ARCHITECTURE_X86_64
#       define  ARCHITECTURE_X86_64 1
#       define  PLATFORM_DESCRIPTION "Windows x86-64"
#   elif defined(__x86__) || defined(_M_IX86) || defined(_X86_)
#       undef   ARCHITECTURE_X86
#       define  ARCHITECTURE_X86 1
#       define  PLATFORM_DESCRIPTION "Windows x86"
#   elif defined(__ia64__) || defined(_M_IA64) || defined(_IA64_)
#       undef   ARCHITECTURE_IA64
#       define  ARCHITECTURE_IA64 1
#       define  PLATFORM_DESCRIPTION "Windows IA-64"
#   else
#       error Unknown architecture
#   endif

#   undef   ARCHITECTURE_ENDIAN_LITTLE
#   define  ARCHITECTURE_ENDIAN_LITTLE 1

#   undef   PLATFORM_FAMILY_DESKTOP
#   define  PLATFORM_FAMILY_DESKTOP 1

#   if !defined(_CRT_SECURE_NO_WARNINGS)
#       define  _CRT_SECURE_NO_WARNINGS 1
#   endif

#   include <winsock2.h>
#   include <mswsock.h>
#   include <ws2tcpip.h>

#else
#   error Unknown platform
#endif

#if ARCHITECTURE_ARM
#   if defined(__thumb__)
#       undef   ARCHITECTURE_THUMB
#       define  ARCHITECTURE_THUMB 1
#   endif
#endif


/* Utility macros */
#define TEXTIFY(x)  _TEXTIFY(x)
#define _TEXTIFY(x) #x


/* Architecture details */
#ifdef __SSE2__
#   undef   ARCHITECTURE_SSE2
#   define  ARCHITECTURE_SSE2 1
#endif

#ifdef __SSE3__
#   undef   ARCHITECTURE_SSE3
#   define  ARCHITECTURE_SSE3 1
#endif

#ifdef __SSE4_1__
#   undef   ARCHITECTURE_SSE4
#   define  ARCHITECTURE_SSE4 1
#endif

#ifdef __ARM_NEON__
#   undef   ARCHITECTURE_NEON
#   define  ARCHITECTURE_NEON 1
#endif


/* Compilers */

/* CLang/LLVM */
#if defined(__clang__)

#   undef   COMPILER_CLANG
#   define  COMPILER_CLANG 1

#   define  COMPILER_NAME "clang"
#   define  COMPILER_DESCRIPTION COMPILER_NAME " " __clang_version__

#   define  RESTRICT restrict
#   if PLATFORM_WINDOWS
#       define THREADLOCAL
#   else
#       define THREADLOCAL __thread
#   endif

#   define ATTRIBUTE(x) __attribute__((__##x##__))
#   define ATTRIBUTE2(x,y) __attribute__((__##x##__(y)))
#   define ATTRIBUTE3(x,y,z) __attribute__((__##x##__(y,z)))

#   define DEPRECATED ATTRIBUTE(deprecated)
#   ifndef FORCEINLINE
#       define FORCEINLINE inline ATTRIBUTE(always_inline)
#   endif
#   define NOINLINE ATTRIBUTE(noinline)
#   define PURECALL ATTRIBUTE(pure)
#   define CONSTCALL ATTRIBUTE(const)
#   define ALIGN(x) ATTRIBUTE2(aligned,x)
#   define ALIGNOF(x) __alignof__(x)
#   define SHARED_BUILD __attribute__((visibility("default")))
#   define SHARED_USE __attribute__((visibility("default")))

#   if PLATFORM_WINDOWS
#       ifndef __USE_MINGW_ANSI_STDIO
#           define __USE_MINGW_ANSI_STDIO 1
#       endif
#       ifndef _CRT_SECURE_NO_WARNINGS
#           define _CRT_SECURE_NO_WARNINGS 1
#       endif
#       ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#           define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 0
#       endif
#       ifndef _MSC_VER
#           define _MSC_VER 1300
#       endif
#       define USE_NO_MINGW_SETJMP_TWO_ARGS 1
#   endif

#   include <stdbool.h>
#   include <stdarg.h>
#   include <wchar.h>

#   ifndef LIKELY
#       define LIKELY(x) (x)
#       define UNLIKELY(x) (x)
#   endif

/* GCC */
#elif defined(__GNUC__)

#   undef   COMPILER_GCC
#   define  COMPILER_GCC 1

#   define COMPILER_NAME "gcc"
#   define COMPILER_DESCRIPTION COMPILER_NAME " v" TEXTIFY(__GNUC__) "." TEXTIFY(__GNUC_MINOR__)

#   define RESTRICT __restrict
#   define THREADLOCAL __thread

#   define ATTRIBUTE(x) __attribute__((__##x##__))
#   define ATTRIBUTE2(x,y) __attribute__((__##x##__(y)))
#   define ATTRIBUTE3(x,y,z) __attribute__((__##x##__(y,z)))

#   define DEPRECATED ATTRIBUTE(deprecated)
#   ifndef FORCEINLINE
#       define FORCEINLINE inline ATTRIBUTE(always_inline)
#   endif
#   define NOINLINE ATTRIBUTE(noinline)
#   define PURECALL ATTRIBUTE(pure)
#   define CONSTCALL ATTRIBUTE(const)
#   define ALIGN(x) ATTRIBUTE2(aligned,x)
#   define ALIGNOF(x) __alignof__(x)
#   define SHARED_BUILD __attribute__((visibility("default")))
#   define SHARED_USE __attribute__((visibility("default")))

#   if PLATFORM_WINDOWS
#       ifndef __USE_MINGW_ANSI_STDIO
#           define __USE_MINGW_ANSI_STDIO 1
#       endif
#       ifndef _CRT_SECURE_NO_WARNINGS
#           define _CRT_SECURE_NO_WARNINGS 1
#       endif
#       ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#           define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 0
#       endif
#       ifndef _MSC_VER
#           define _MSC_VER 1300
#       endif
#   endif

#   include <stdbool.h>
#   include <stdarg.h>
#   include <wchar.h>

#   ifndef LIKELY
#       define LIKELY(x) __builtin_expect(!!(x), 1)
#       define UNLIKELY(x) __builtin_expect(!!(x), 0)
#   endif

/* Intel ICC/ICPC */
#elif defined(__ICL) || defined(__ICC) || defined(__INTEL_COMPILER)

#   undef   COMPILER_INTEL
#   define  COMPILER_INTEL 1

#   define  COMPILER_NAME "intel"
#   if defined(__ICL)
#       define COMPILER_DESCRIPTION COMPILER_NAME " v" TEXTIFY(__ICL)
#   elif defined(__ICC)
#       define COMPILER_DESCRIPTION COMPILER_NAME " v" TEXTIFY(__ICC)
#  endif

#   define RESTRICT __restrict
#   define THREADLOCAL __declspec(thread)

#   define ATTRIBUTE(x)
#   define ATTRIBUTE2(x,y)
#   define ATTRIBUTE3(x,y,z)

#   define DEPRECATED
#   ifndef FORCEINLINE
#       define FORCEINLINE __forceinline
#   endif
#   define NOINLINE __declspec(noinline)
#   define PURECALL 
#   define CONSTCALL
#   define ALIGN(x) __declspec(align(x))
#   define ALIGNOF(x) __alignof(x)
#   define SHARED_BUILD __declspec(dllexport)
#   define SHARED_USE __declspec(dllimport)

#   if PLATFORM_WINDOWS
#       define va_copy(d,s) ((d)=(s))
#   endif

#   include <intrin.h>

#   define bool _Bool
#   define true 1
#   define false 0
#   define __bool_true_false_are_defined 1

#   ifndef LIKELY
#       define LIKELY(x) (x)
#       define UNLIKELY(x) (x)
#   endif

/* Microsoft Visual Studio */
#elif defined(_MSC_VER)

#   undef   COMPILER_MSVC
#  define   COMPILER_MSVC 1

#   define  COMPILER_NAME "msvc"
#   define  COMPILER_DESCRIPTION COMPILER_NAME " v" TEXTIFY(_MSC_VER)

#   define RESTRICT __restrict
#   define THREADLOCAL __declspec(thread)

#   define DEPRECATED __declspec(deprecated)
#   ifndef FORCEINLINE
#       define FORCEINLINE __forceinline
#   endif
#   define NOINLINE __declspec(noinline)
#   define PURECALL
#   define CONSTCALL
#   define ALIGN(x) __declspec(align(x))
#   define SHARED_BUILD __declspec(dllexport)
#   define SHARED_USE __declspec(dllimport)

#   ifndef LIKELY
#       define LIKELY(x) (x)
#       define UNLIKELY(x) (x)
#   endif

#   ifndef __cplusplus
typedef enum
{
	false = 0,
	true  = 1
} bool;
#   endif

#   if _MSC_VER < 1800
#       define va_copy(d,s) ((d)=(s))
#   endif

#   include <intrin.h>

#else
#   error Unknown compiler

#   define COMPILER_NAME "unknown"
#   define COMPILER_DESCRIPTION "unknown"

#   define RESTRICT
#   define THREADLOCAL

#   define DEPRECATED
#   define FORCEINLINE
#   define NOINLINE
#   define PURECALL
#   define CONSTCALL
#   define ALIGN
#   define ALIGNOF

#   define LIKELY(x) (x)
#   define UNLIKELY(x) (x)

#endif

/* Base data types */
#include <stdint.h>
#include <stddef.h>
#include <float.h>
#include <limits.h>

/* Pointer size */
#if ARCHITECTURE_ARM_64 || ARCHITECTURE_X86_64 || ARCHITECTURE_PPC_64 || ARCHITECTURE_IA64 || ARCHITECTURE_MIPS_64
#   define POINTER_SIZE 8
#else
#   define POINTER_SIZE 4
#endif

/* wchar_t size */
#if PLATFORM_LINUX_RASPBERRYPI
#   define WCHAR_SIZE 4
#else
#   if WCHAR_MAX > 0xffff
#       define WCHAR_SIZE 4
#   else
#       define WCHAR_SIZE 2
#   endif
#endif

/* Wrappers for platforms that not yet support thread-local storage declarations
 * For maximum portability use wrapper macro DECLARE_THREAD_LOCAL
 * Only works for types that can be safely cast through a uintptr_t (integers, pointers...)
 *   DECLARE_THREAD_LOCAL(int, profile_block, 0);
 *   set_thread_profile_block(1); // Assigns 1 to thread-local variable "profile_block"
 */
#if PLATFORM_APPLE || PLATFORM_ANDROID

/* Forward declarations of various system APIs */
#   if PLATFORM_ANDROID
typedef int _pthread_key_t;
#   else
typedef __darwin_pthread_key_t _pthread_key_t;
#   endif

extern int pthread_key_create(_pthread_key_t*, void (*)(void*));
extern int pthread_setspecific(_pthread_key_t, const void*);
extern void* pthread_getspecific(_pthread_key_t);

#define DECLARE_THREAD_LOCAL(type, name, init) \
static _pthread_key_t _##name##_key = 0; \
static FORCEINLINE _pthread_key_t get_##name##_key(void) { if (!_##name##_key) pthread_key_create(&_##name##_key, init); return _##name##_key; } \
static FORCEINLINE type get_thread_##name(void) { return (type)((uintptr_t)pthread_getspecific(get_##name##_key())); } \
static FORCEINLINE void set_thread_##name(type val) { pthread_setspecific(get_##name##_key(), (const void*)(uintptr_t)val); }

#elif PLATFORM_WINDOWS && COMPILER_CLANG

__declspec(dllimport) unsigned long __stdcall TlsAlloc();
__declspec(dllimport) void*__stdcall TlsGetValue(unsigned long);
__declspec(dllimport) int __stdcall TlsSetValue(unsigned long, void*);

#define DECLARE_THREAD_LOCAL(type, name, init) \
static unsigned long _##name##_key = 0; \
static FORCEINLINE unsigned long get_##name##_key(void) { if (!_##name##_key) { _##name##_key = TlsAlloc(); TlsSetValue(_##name##_key, init); } return _##name##_key; } \
static FORCEINLINE type get_thread_##name(void) { return (type)((uintptr_t)TlsGetValue(get_##name##_key())); } \
static FORCEINLINE void set_thread_##name(type val) { TlsSetValue(get_##name##_key(), (void*)((uintptr_t)val)); }

#else

#define DECLARE_THREAD_LOCAL(type, name, init) \
static THREADLOCAL type _thread_##name = init; \
static FORCEINLINE void set_thread_##name(type val) { _thread_##name = val; } \
static FORCEINLINE type get_thread_##name(void) { return _thread_##name; }

#endif


/* Format specifiers for 64bit and pointers */

#if defined( _MSC_VER )
#   define PRId64       "I64d"
#   define PRIi64       "I64i"
#   define PRIdPTR      "Id"
#   define PRIiPTR      "Ii"
#   define PRIo64       "I64o"
#   define PRIu64       "I64u"
#   define PRIx64       "I64x"
#   define PRIX64       "I64X"
#   define PRIoPTR      "Io"
#   define PRIuPTR      "Iu"
#   define PRIxPTR      "Ix"
#   define PRIXPTR      "IX"
#else
#   ifndef __STDC_FORMAT_MACROS
#       define __STDC_FORMAT_MACROS
#   endif
#   include <inttypes.h>
#endif

#if PLATFORM_WINDOWS
#   if POINTER_SIZE == 8
#       define PRIfixPTR  "016I64X"
#   else
#       define PRIfixPTR  "08IX"
#   endif
#else
#   if POINTER_SIZE == 8
#       define PRIfixPTR  "016llX"
#   else
#       define PRIfixPTR  "08X"
#   endif
#endif

#if PLATFORM_ANDROID && defined(PLATFORM_ANDROID_LP64_WORKAROUND)
#   undef   __LP64__
#   undef   PLATFORM_ANDROID_LP64_WORKAROUND
#endif

#if PLATFORM_ANDROID
#   undef   __ISO_C_VISIBLE
#   define  __ISO_C_VISIBLE 2011
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* IO_PLATFORM_H_INCLUDED */