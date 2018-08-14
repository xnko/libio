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

#ifndef IO_RBTREE_H_INCLUDED
#define IO_RBTREE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rbnode_t {
    struct rbnode_t* parent;
    struct rbnode_t* left;
    struct rbnode_t* right;
    int color;
} rbnode_t;

typedef int(*rbnode_compare_fn)(rbnode_t* node1, rbnode_t* node2);

rbnode_t* rbtree_first(rbnode_t* root);
rbnode_t* rbtree_next(rbnode_t* node);
void rbtree_insert(rbnode_t** root, rbnode_t* node, rbnode_compare_fn compare);
void rbtree_remove(rbnode_t** root, rbnode_t* node, rbnode_compare_fn compare);
rbnode_t* rbtree_search(rbnode_t* root, rbnode_t* value, rbnode_compare_fn compare);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IO_RBTREE_H_INCLUDED