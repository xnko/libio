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

#include "moment.h"
#include "task.h"

static int moment_compare(rbnode_t* node1, rbnode_t* node2)
{
    uint64_t time1 = ((moment_t*)node1)->time;
    uint64_t time2 = ((moment_t*)node2)->time;

    if (time1 < time2)
        return -1;

    if (time1 > time2)
        return 1;

    return 0;
}

static void moment_init(moment_t* moment)
{
    moment->node.parent = 0;
    moment->node.left = 0;
    moment->node.right = 0;
    moment->node.color = 0;

    moment->reached = 0;
    moment->removed = 0;
    moment->shutdown = 0;
}

void moments_add(moments_t* moments, moment_t* moment)
{
    moment_init(moment);
    rbtree_insert(&moments->root, &moment->node, moment_compare);
}

void moments_remove(moments_t* moments, moment_t* moment)
{
    rbtree_remove(&moments->root, &moment->node, moment_compare);
    moment->removed = 1;
}

void moments_shutdown(moments_t* moments)
{
    moment_t* temp;
    moment_t* moment = (moment_t*)rbtree_first(moments->root);
    while (moment)
    {
        temp = (moment_t*)rbtree_next(&moment->node);
        
        // Remove from moments and add to dues
        rbtree_remove(&moments->root, &moment->node, moment_compare);

        moment->shutdown = 1;
        task_resume(moment->task);

        moment = temp;
    }
}

uint64_t moments_tick(moments_t* moments, uint64_t now)
{
    uint64_t count = 0;
    moment_t dues;
    moment_t* tail = &dues;

    moment_t* temp;
    moment_t* moment = (moment_t*)rbtree_first(moments->root);
    
    dues.node.parent = 0;
    while (moment && moment->time <= now)
    {
        temp = (moment_t*)rbtree_next(&moment->node);
        
        // Remove from moments and add to dues
        rbtree_remove(&moments->root, &moment->node, moment_compare);
        tail->node.parent = &moment->node;
        moment->node.parent = 0;
        tail = moment;

        moment = temp;
    }

    // Resume dues
    moment = (moment_t*)dues.node.parent;
    while (moment)
    {
        temp = (moment_t*)moment->node.parent;

        moment->reached = 1;
        task_resume(moment->task);
        ++count;

        moment = temp;
    }

    return count;
}

uint64_t moments_nearest(moments_t* moments)
{
    moment_t* first = (moment_t*)rbtree_first(moments->root);

    if (first)
    {
        return first->time;
    }

    return 0;
}