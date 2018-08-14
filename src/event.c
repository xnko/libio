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

#include <memory.h>
#include "event.h"
#include "thread.h"
#include "task.h"
#include "mpscq.h"
#include "list.h"
#include "memory.h"

#if PLATFORM_WINDOWS
#   define  WIN32_LEAN_AND_MEAN 1
#   include <Windows.h>
#elif PLATFORM_LINUX
#   include <unistd.h>
#   include <sys/eventfd.h>
#   include "loop-linux.h"
#else
#   error Not Supported
#endif

#define IO_EVENT_COMMAND_NOTIFY 0
#define IO_EVENT_COMMAND_WAIT   1
//#define IO_EVENT_COMMAND_UNWAIT 2
#define IO_EVENT_COMMAND_DELETE 3

typedef struct io_event_t {
    LIST_OF(io_event_waiter_t);
} io_event_t;

typedef struct io_event_waiter_t {
    LIST_NODE_OF(io_event_waiter_t);
    io_loop_t* loop;
    task_t* task;
    int error;
} io_event_waiter_t;

typedef struct io_event_command_t {
    mpscq_node_t node;
    io_loop_t* loop;
    task_t* task;
    int type;
    io_event_t* event;
    io_event_waiter_t waiter;
} io_event_command_t;

typedef struct io_event_processor_t {
    mpscq_t commands;
#if PLATFORM_WINDOWS
    HANDLE event;
#elif PLATFORM_LINUX
    int event;
#else
#   error Not Supported
#endif
} io_event_processor_t;

static io_event_processor_t processor;

IO_THREAD_TYPE io_event_processor(void* arg)
{
    uint64_t counter;
    mpscq_node_t* node;
    io_event_command_t* command;
    io_event_waiter_t* waiter;

    while (1)
    {
#if PLATFORM_WINDOWS
        if (WAIT_OBJECT_0 != WaitForSingleObject(processor.event, INFINITE))
        {
            break;
        }
#elif PLATFORM_LINUX
        if (sizeof (counter) != read(processor.event, &counter, sizeof (counter)))
        {
            break;
        }
#else
#   error Not Supported
#endif

        // iterate through commands
        node = mpscq_pop(&processor.commands);
        command = container_of(node, io_event_command_t, node);
        while (command != 0)
        {
            switch (command->type) {
                case IO_EVENT_COMMAND_WAIT: {
                    LIST_PUSH_TAIL(command->event, (&command->waiter));
                    break;
                }

                case IO_EVENT_COMMAND_NOTIFY: {
                    waiter = LIST_HEAD(command->event);
                    while (waiter != 0)
                    {
                        LIST_POP_HEAD(command->event);

                        waiter->error = 0;
                        io_loop_post_task(waiter->loop, waiter->task);

                        waiter = LIST_HEAD(command->event);
                    }
                    break;
                }

                case IO_EVENT_COMMAND_DELETE: {
                    waiter = LIST_HEAD(command->event);
                    while (waiter != 0)
                    {
                        LIST_POP_HEAD(command->event);

                        waiter->error = ECANCELED;
                        io_loop_post_task(waiter->loop, waiter->task);

                        waiter = LIST_HEAD(command->event);
                    }
                    break;
                }
            }

            // next command
            node = mpscq_pop(&processor.commands);
            command = container_of(node, io_event_command_t, node);
        }
    }
}

int io_event_command(io_event_command_t* command)
{
    uint64_t one = 1;

    mpscq_push(&processor.commands, &command->node);
    command->loop = io_loop_current();
    command->task = command->loop->current;

#if PLATFORM_WINDOWS
    SetEvent(processor.event);
#elif PLATFORM_LINUX
    write(processor.event, &one, sizeof(one));
#else
#   error Not Supported
#endif

    task_suspend(command->task);

    return 0;
}

/*
 * Internal API
 */

int io_event_init()
{
    memset(&processor, 0, sizeof(processor));

    mpscq_init(&processor.commands);

#if PLATFORM_WINDOWS
    processor.event = CreateEventExA(
        0,          // security sttributes
        0,          // name
        0,          // flags
        SYNCHRONIZE // desired access
        );
#elif PLATFORM_LINUX
    processor.event = eventfd(
        0,  // initial
        0   // flags
        );
#else
#   error Not Supported
#endif

    io_thread_create(io_event_processor, 0);

    return 0;
}

int io_event_shutdown()
{
#if PLATFORM_WINDOWS
    CloseHandle(processor.event);
#elif PLATFORM_LINUX
    io_close(processor.event);
#else
#   error Not Supported
#endif
}

/*
int io_event_unwait(io_event_t* event)
{
    io_event_command_t command;
    
    command.type = IO_EVENT_COMMAND_UNWAIT;
    command.event = event;

    return io_event_command(&command);
}
*/

/*
 * Public API
 */

int io_event_create(io_event_t** event)
{
    *event = io_calloc(1, sizeof (io_event_t));
    if (*event == 0)
    {
        return errno;
    }

    return 0;
}

int io_event_delete(io_event_t* event)
{
    io_event_command_t command;
    
    command.type = IO_EVENT_COMMAND_DELETE;
    command.event = event;

    return io_event_command(&command);
}

int io_event_notify(io_event_t* event)
{
    io_event_command_t command;
    
    command.type = IO_EVENT_COMMAND_NOTIFY;
    command.event = event;

    return io_event_command(&command);
}

int io_event_wait(io_event_t* event)
{
    io_event_command_t command;
    
    command.type = IO_EVENT_COMMAND_WAIT;
    command.event = event;

    return io_event_command(&command);
}