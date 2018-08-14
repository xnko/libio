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

#include "rbtree.h"

#define RBNODE_BLACK    0
#define RBNODE_RED      1

#define RB_COLOR(n) (n ? n->color : RBNODE_BLACK)

static rbnode_t* rbnode_grandparent(rbnode_t* node)
{
    if (node->parent)
        return node->parent->parent;

    return 0;
}

static rbnode_t* rbnode_uncle(rbnode_t* node)
{
    rbnode_t* grandparent = rbnode_grandparent(node);

    if (!grandparent)
        return 0;

    if (node->parent == grandparent->left)
        return grandparent->right;

    return grandparent->left;
}

static rbnode_t* rbnode_sibling(rbnode_t* node)
{
    if (node == node->parent->left)
        return node->parent->right;

    return node->parent->left;
}

static void rbtree_rotate_left(rbnode_t** root, rbnode_t* node)
{
    rbnode_t* right = node->right;

    node->right = right->left;
    if (node->right)
        right->left->parent = node;

    right->parent = node->parent;
    if (right->parent)
    {
        if (node == right->parent->left)
            node->parent->left = right;
        else
            node->parent->right = right;
    }
    else
    {
        *root = right;
    }

    right->left = node;
    node->parent = right;
}

static void rbtree_rotate_right(rbnode_t** root, rbnode_t* node)
{
    rbnode_t* left = node->left;

    node->left = left->right;
    if (node->left)
        left->right->parent = node;

    left->parent = node->parent;
    if (left->parent)
    {
        if (node == left->parent->left)
            left->parent->left = left;
        else
            left->parent->right = left;
    }
    else
    {
        *root = left;
    }

    left->right = node;
    node->parent = left;
}

static void rbtree_insert_bal(rbnode_t** root, rbnode_t* node)
{
    rbnode_t* grandparent;
    rbnode_t* uncle;

    if (*root == node)
    {
        node->color = RBNODE_BLACK;
    }
    else if (RBNODE_RED == RB_COLOR(node->parent))
    {
        uncle = rbnode_uncle(node);
        grandparent = rbnode_grandparent(node);

        if (uncle && RBNODE_RED == uncle->color)
        {
            node->parent->color = RBNODE_BLACK;
            uncle->color = RBNODE_BLACK;
            grandparent->color = RBNODE_RED;

            rbtree_insert_bal(root, grandparent);
            return;
        }

        if (node == node->parent->right && node->parent == grandparent->left)
        {
            rbtree_rotate_left(root, node->parent);
            node = node->left;
        }
        else if (node == node->parent->left && node->parent == grandparent->right)
        {
            rbtree_rotate_right(root, node->parent);
            node = node->right;
        }

        grandparent = rbnode_grandparent(node);
        node->parent->color = RBNODE_BLACK;
        grandparent->color = RBNODE_RED;

        if (node == node->parent->left && node->parent == grandparent->left)
            rbtree_rotate_right(root, grandparent);
        else
            rbtree_rotate_left(root, grandparent);
    }
}

static void rbtree_remove_bal(rbnode_t** root, rbnode_t* node)
{
    rbnode_t* sibling;

    if (!node->parent)
        return;

    sibling = rbnode_sibling(node);

    if (RBNODE_RED == RB_COLOR(sibling))
    {
        node->parent->color = RBNODE_RED;
        sibling->color = RBNODE_BLACK;

        if (node == node->parent->left)
            rbtree_rotate_left(root, node->parent);
        else
            rbtree_rotate_right(root, node->parent);

        sibling = rbnode_sibling(node);
    }

    if (RBNODE_BLACK == RB_COLOR(node->parent) &&
        RBNODE_BLACK == RB_COLOR(sibling) &&
        RBNODE_BLACK == RB_COLOR(sibling->left) &&
        RBNODE_BLACK == RB_COLOR(sibling->right))
    {
        sibling->color = RBNODE_RED;
        rbtree_remove_bal(root, node->parent);
        return;
    }

    if (RBNODE_RED == RB_COLOR(node->parent) &&
        RBNODE_BLACK == RB_COLOR(sibling) &&
        RBNODE_BLACK == RB_COLOR(sibling->left) &&
        RBNODE_BLACK == RB_COLOR(sibling->right))
    {
        sibling->color = RBNODE_RED;
        node->parent->color = RBNODE_BLACK;
    }
    else
    { 
        if (node->parent->left == node &&
            RBNODE_BLACK == RB_COLOR(sibling->right) &&
            RBNODE_RED == RB_COLOR(sibling->left))
        {
            sibling->color = RBNODE_RED;
            sibling->left->color = RBNODE_BLACK;
            rbtree_rotate_right(root, sibling);
        }
        else if (node->parent->right == node &&
            RBNODE_BLACK == RB_COLOR(sibling->left) &&
            RBNODE_RED == RB_COLOR(sibling->right))
        {
            sibling->color = RBNODE_BLACK;
            rbtree_rotate_left(root, sibling);
        }

        sibling = rbnode_sibling(node);
        sibling->color = RB_COLOR(node->parent);
        node->parent->color = RBNODE_BLACK;

        if (node->parent->left == node)
        {
            sibling->right->color = RBNODE_BLACK;
            rbtree_rotate_left(root, node->parent);
        }
        else
        {
            sibling->left->color = RBNODE_BLACK;
            rbtree_rotate_right(root, node->parent);
        }
    }
}

static void rbtree_swap(rbnode_t* node1, rbnode_t* node2)
{
    rbnode_t* temp = node1->parent;
    int color;

    // swap parents

    node1->parent = node2->parent;
    node2->parent = temp;

    if (node1->parent)
    {
        if (node1->parent->left == node2)
            node1->parent->left = node1;
        else
            node1->parent->right = node1;
    }

    if (node2->parent)
    {
        if (node2->parent->left == node1)
            node2->parent->left = node2;
        else
            node2->parent->right = node2;
    }

    // swap lefts

    temp = node1->left;
    node1->left = node2->left;
    if (node1->left)
        node1->left->parent = node1;

    node2->left = temp;
    if (node2->left)
        node2->left->parent = node2;

    // swap rights

    temp = node1->right;
    node1->right = node2->right;
    if (node1->right)
        node1->right->parent = node1;

    node2->right = temp;
    if (node2->right)
        node2->right->parent = node2;

    // swap color

    color = node1->color;
    node1->color = node2->color;
    node2->color = color;
}

rbnode_t* rbtree_first(rbnode_t* root)
{
    rbnode_t* first = root;

    if (first)
    {
        while (first->left)
            first = first->left;
    }

    return first;
}

rbnode_t* rbtree_next(rbnode_t* node)
{
    rbnode_t* next = 0;

    if (node->right)
    {
        next = node->right;
    }
    else if (node->parent)
    {
        if (node != node->parent->right)
            next = node->parent->right;
    }

    if (next)
    {
        while (next->left)
            next = next->left;
    }

    return next;
}

void rbtree_insert(rbnode_t** root, rbnode_t* node, rbnode_compare_fn compare)
{
    rbnode_t* current;
    rbnode_t* parent;

    current = *root;

    while (current)
    {
        parent = current;

        if (compare(node, current) < 0)
            current = current->left;
        else
            current = current->right;
    }

    node->parent = 0;
    node->left = 0;
    node->right = 0;
    node->color = RBNODE_RED;

    if (!*root)
    {
        *root = node;
    }
    else
    {
        node->parent = parent;

        if (compare(node, parent) < 0)
            parent->left = node;
        else
            parent->right = node;
    }

    rbtree_insert_bal(root, node);
}

void rbtree_remove(rbnode_t** root, rbnode_t* node, rbnode_compare_fn compare)
{
    rbnode_t* child;
    rbnode_t* prev;
    rbnode_t phantom;

    phantom.left = 0;
    phantom.right = 0;
    phantom.parent = 0;
    phantom.color = RBNODE_BLACK;

    if (node->left && node->right)
    {
        prev = node->left;

        while (prev->right)
            prev = prev->right;

        rbtree_swap(node, prev);

        if (*root == prev)
            *root = node;
        else if (*root == node)
            *root = prev;
    }

    if (node->left) 
        child = node->left;
    else if (node->right)
        child = node->right;
    else
        child = &phantom;

    if (!node->parent)
        *root = child;
    else
    {
        if (node == node->parent->left)
            node->parent->left = child;
        else
            node->parent->right = child;
    }

    if (child != node->left)
        child->left = node->left;

    if (child != node->right)
        child->right = node->right;

    child->parent = node->parent;
    node->left = 0;
    node->right = 0;
    node->parent = 0;

    if (RBNODE_BLACK == RB_COLOR(node))
    {
        if (RBNODE_RED == RB_COLOR(child))
            child->color = RBNODE_BLACK;
        else
            rbtree_remove_bal(root, child);
    }

    if (&phantom == child)
    {
        if (child->parent)
        {
            if (child == child->parent->left)
                child->parent->left = 0;
            else
                child->parent->right = 0;
        }
        else if (*root == &phantom) 
            *root = 0;
    }
}

rbnode_t* rbtree_search(rbnode_t* root, rbnode_t* value, rbnode_compare_fn compare)
{
    rbnode_t* node = root;
    int comparision;

    while (node)
    {
        comparision = compare(value, node);

        if (comparision == 0)
            return node;

        if (comparision < 0)
            node = node->left;
        else
            node = node->right;
    }

    return 0;
}