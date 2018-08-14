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

#ifndef IO_LIST_H_INCLUDED
#define IO_LIST_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_OF(node_t)     \
    struct node_t* head;    \
    struct node_t* tail

#define LIST_NODE_OF(node_t)    \
    struct node_t* next;        \
    struct node_t* prev

#define LIST_PUSH_HEAD(list, node) {    \
    node->prev = 0;                     \
    node->next = 0;                     \
    if (list->head == 0) {              \
        list->head = node;              \
        list->tail = node;              \
    } else {                            \
        node->next = list->head;        \
        list->head->prev = node;        \
        list->head = node;              \
    }                                   \
}

#define LIST_PUSH_TAIL(list, node) {    \
    node->prev = 0;                     \
    node->next = 0;                     \
    if (list->tail == 0) {              \
        list->head = node;              \
        list->tail = node;              \
    } else {                            \
        node->prev = list->tail;        \
        list->tail->next = node;        \
        list->tail = node;              \
    }                                   \
}

#define LIST_HEAD(list) list->head
#define LIST_TAIL(list) list->tail

#define LIST_POP_HEAD(list) {           \
    void* node = list->head;            \
    if (list->head != 0) {              \
        list->head = list->head->next;  \
        if (list->tail == node)         \
            list->tail = 0;             \
        else                            \
            list->head->prev = 0;       \
    }                                   \
}

#define LIST_POP_TAIL(list) {           \
    void* node = list->tail;            \
    if (list->tail != 0) {              \
        node = list->tail;              \
        list->tail = list->tail->prev;  \
        if (list->head == node)         \
            list->head = 0;             \
        else                            \
            list->tail->next = 0;       \
    }                                   \
}

#define LIST_REMOVE(list, node) {       \
    if (node == list->head) {           \
        LIST_POP_HEAD(list);            \
    } else if (node == list->tail) {    \
        LIST_POP_TAIL(list);            \
    } else {                            \
        node->prev->next = node->next;  \
        node->next->prev = node->prev;  \
    }                                   \
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_LIST_H_INCLUDED