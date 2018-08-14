## Introduction

A stackful task/coroutine library

- Cross platform
- Zero configuration
- Zero setup
- Blazing fast
- Memory efficient (with autogrow stacks)

## CLI

```bash
# build
> make

# build debug
> make build=Debug

# install
> make install

# uninstall
> make uninstall
```

## API Reference

Basically the whole include file :)

```c
// All calls return 0 on success and errno on failure

typedef struct task_t task_t;
typedef void (*task_fn)(task_t* task, void* arg);

// Create a new task with entry point and argument
int task_create(task_t** task, task_fn entry, void* arg);

// Delete the task
int task_delete(task_t* task);

// Yield a value to caller 
int task_yield(void* value);

// Run task as a generator
int task_next(task_t* task, void** yield);

// Post a task to current thread
int task_post(task_fn entry, void* arg);

// Suspend the current task, and resume main task
int task_suspend();

// Resume the supended task
int task_resume(task_t* task);
```

## Examples

```c
// Print fibonacci numbers

#include <stdio.h>
#include "task/task.h"

void fibonacci(task_t* task, void* arg)
{
    int series, first = 0, second = 1, next, c;

    series = *(int*)arg;

    for (c = 0; c < series; ++c)
    {
        if (c <= 1)
            next = c;
        else
        {
            next = first + second;
            first = second;
            second = next;
        }

        task_yield(&next);
    }
}

void main()
{
    task_t* task;
    int* next;
    int series = 30;

    task_create(&task, fibonacci, &series);
    
    while (!task_next(task, (void**)&next))
    {
        printf("%d\r\n", *next);
    }

    task_delete(task);
}
```

```c
// Print permutations

#include <stdio.h>
#include "task/task.h"

static char series[] = { 'a', 'b', 'c', 'd', 0 };

void swap(char* x, char* y)
{
    char temp;
    temp = *x;
    *x = *y;
    *y = temp;
}

void permute(task_t* task, void* arg)
{
    task_t* permuter;
    int index = arg ? *(int*)arg : 0;
    int next = index + 1;
    int i;

    if (series[next] == 0)
    {
        task_yield(0);
        return;
    }

    for (i = index; series[i]; ++i)
    {
        swap(series + index, series + i);

        task_create(&permuter, permute, &next);
        while (!task_next(permuter, 0))
        {
            task_yield(0);
        }
        task_delete(permuter);

        swap(series + index, series + i);
    }
}

void main()
{
    task_t* permuter;

    task_create(&permuter, permute, 0);
    while (!task_next(permuter, 0))
    {
        printf("%s\r\n", series);
    }
    task_delete(permuter);
}
```

```c
// Ping pong

#include <stdio.h>
#include "task/task.h"

void pong(task_t* task, void* arg)
{
    *((task_t**)arg) = task;

    int iteration = 1;

    while (1)
    {
        task_suspend();

        printf("pong %d\r\n", iteration++);
    }
}

void main()
{
    int iteration = 0;
    task_t* ponger;

    task_post(pong, &ponger);

    while (iteration++ < 30)
    {
        printf("ping %d\r\n", iteration);

        task_resume(ponger);
    }
}
```

## Platforms

Tested on
- Win64 (32 bit build)
- Ubuntu64 (64 bit build)

## ToDos

- Test on all platforms with different architectures
- Compare performance with other libraries

## Contacts

Email：[artak.khnkoyan@gmail.com](mailto:artak.khnkoyan@gmail.com)

## Licence

MIT