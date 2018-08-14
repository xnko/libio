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

#include "../memory.h"
#include "../task.h"

#if PLATFORM_WINDOWS == 0
#   include <pthread.h>
#endif

#include <malloc.h>

static uintptr_t task_default_stack_size = 0;

static uintptr_t task_get_default_stack_size()
{
    if (task_default_stack_size == 0)
    {
        // ToDo: retrieve current thread stack size
        task_default_stack_size = 1024 * 1024;
    }

    return task_default_stack_size;
}

static void task_entry_point(struct task_t* task)
{
    task->entry(task->loop, task->arg);
    task->is_done = 1;
    task->loop->prev = task;
    setcontext(&task->parent->context);
}

#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
#pragma optimize("", off)
#endif
void task_swapcontext(struct task_t* current, struct task_t* other)
{
    io_loop_t* loop = current->loop;
	
    /*
     * save error state
     */

    int error = errno;
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
    DWORD win_error = GetLastError();
#endif

    loop->prev = current;
    loop->current = other;
    swapcontext(&current->context, &other->context);
    loop->current = current;

    if (loop->prev != 0 &&
        loop->prev->is_post &&
        loop->prev->is_done)
    {
        /*
         * if prev was posted and done then delete it
         */

        task_delete(loop->prev);
        loop->prev = 0;
    }

    if (loop->current->inherit_error_state == 0)
    {
        /*
         * if current task does not inherit error state, then
         * restore its last error state
         */

        errno = error;
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
        SetLastError(win_error);
#endif
    }
}
#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)
#pragma optimize("", on)
#endif


#if PLATFORM_WINDOWS

static DWORD page_size = 0;

static DWORD task_get_page_size()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    return si.dwPageSize;
}

static void* task_create_stack(size_t size)
{
    if (page_size == 0)
    {
        page_size = task_get_page_size();
    }

    void* vp = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    if (!vp)
    {
        errno = ENOMEM;
        return 0;
    }

    // needs at least 2 pages to fully construct the coroutine and switch to it
    size_t init_commit_size = page_size + page_size;
    char* pPtr = ((char*)vp) + size;
    pPtr -= init_commit_size;
    if (!VirtualAlloc(pPtr, init_commit_size, MEM_COMMIT, PAGE_READWRITE))
        goto cleanup;

    // create guard page so the OS can catch page faults and grow our stack
    pPtr -= page_size;
    if (VirtualAlloc(pPtr, page_size, MEM_COMMIT, PAGE_READWRITE|PAGE_GUARD))
        return vp;

cleanup:

    VirtualFree(vp, 0, MEM_RELEASE);
    
    errno = ENOMEM;
    return 0;
}

static void task_delete_stack(void* stack, size_t size)
{
    VirtualFree(stack, 0, MEM_RELEASE);
}

#else

#include <sys/mman.h>

static int page_size = 0;

static void* task_create_stack(size_t size)
{
    if (page_size == 0)
    {
        page_size = getpagesize();
    }

    void* vp = mmap(
        0,                      // void *addr,
        size,                   // size_t length,
        PROT_READ | PROT_WRITE, // int prot,
        MAP_ANONYMOUS |         // int flags,
        MAP_NORESERVE |
        MAP_STACK |
        // MAP_UNINITIALIZED |
        MAP_PRIVATE,  
        -1,                     // int fd,
        0                       // off_t offset
    );
    if (!vp)
    {
        return 0;
    }
}

static void task_delete_stack(void* stack, size_t size)
{
    munmap(stack, size);
}

#endif

/*
 * Internal API
 */

int task_create(struct task_t** task, io_loop_fn entry, void* arg)
{
    uintptr_t stack_size = task_get_default_stack_size();

    *task = (task_t*)io_calloc(1, sizeof(**task));
    if (*task == 0)
    {
        errno = ENOMEM;
        return errno;
    }

    (*task)->stack = task_create_stack(stack_size);
    if ((*task)->stack == 0)
    {
        io_free(*task);

        errno = ENOMEM;
        return errno;
    }

    (*task)->stack_size = stack_size;
    (*task)->entry = entry;
    (*task)->arg = arg;
    (*task)->is_done = 0;
    (*task)->is_post = 0;
    (*task)->parent = 0;
    (*task)->inherit_error_state = 0;

#if PLATFORM_WINDOWS && (COMPILER_MSVC || COMPILER_INTEL)

    (*task)->context.uc.ContextFlags = CONTEXT_ALL;

    getcontext(&(*task)->context);

    (*task)->context.stack = (*task)->stack;
    (*task)->context.stack_size = (*task)->stack_size;

    makecontext(&(*task)->context, (void(*)())task_entry_point, 1, *task);

#else

    getcontext(&(*task)->context);

    (*task)->context.uc_stack.ss_sp = (*task)->stack;
    (*task)->context.uc_stack.ss_size = (*task)->stack_size;
    (*task)->context.uc_stack.ss_flags = 0;
    (*task)->context.uc_link = 0;

    makecontext(&(*task)->context, (void(*)())task_entry_point, 1, *task);

#endif

    return 0;
}

int task_delete(struct task_t* task)
{
    if (task->loop != 0 && task->loop->current == task)
    {
        /* Cannot delete self task */
        return EDEADLOCK;
    }

    task_delete_stack(task->stack, task->stack_size);
    io_free(task);

    return 0;
}

int task_yield(task_t* current, void* value)
{
    if (current == &current->loop->main)
    {
        /* Cannot yield main task */
        return EDEADLOCK;
    }
    
    current->loop->value = value;
    task_swapcontext(current, current->parent);

    return 0;
}

int task_exec(task_t* task, io_loop_t* loop, void** yield)
{
    int error_state;

    if (task->is_done)
    {
        /* Cannot exec done task */
        return EALREADY;
    }

    task->loop = loop;
    
    /*
     * save/restore current tasks inherit_error_state value
     */
    error_state = loop->current->inherit_error_state;

    task->parent = loop->current;
    task->is_post = 0;
	
    /*
     * inherit error state set by callee task
     */
    loop->current->inherit_error_state = 1;
    task_swapcontext(loop->current, task);
    loop->current->inherit_error_state = error_state;

    if (yield)
        *yield = loop->value;

    return 0;
}

int task_next(task_t* task, io_loop_t* loop, void** yield)
{
    int error = task_exec(task, loop, yield);
    if (error)
    {
        return error;
    }

    if (task->is_done)
    {
        /* Done */
        return EALREADY;
    }

    return 0;
}

int task_post(task_t* task, io_loop_t* loop)
{
    task->loop = loop;
    task->parent = &loop->main;
    task->is_post = 1;
	
    task_swapcontext(loop->current, task);

    return 0;
}

int task_suspend(task_t* current)
{
    io_loop_t* loop = current->loop;

    if (current == &loop->main)
    {
        // Cannot yeld main task
        return EDEADLOCK;
    }

    /*
     * save/restore current tasks inherit_error_state value
     */
    int error_state = current->inherit_error_state;

    current->inherit_error_state = 1;
    task_swapcontext(current, &loop->main);
    current->inherit_error_state = error_state;

    return 0;
}

int task_resume(task_t* task)
{
    task_swapcontext(task->loop->current, task);

    return 0;
}