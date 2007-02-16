/* libnih
 *
 * tree.c - generic binary tree implementation
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <nih/macros.h>
#include <nih/logging.h>
#include <nih/alloc.h>

#include "tree.h"


/**
 * nih_tree_init:
 * @tree: tree node to be initialised.
 *
 * Initialise an already allocated tree node, once done it can be used
 * as the start of a new binary tree or added to an existing tree.
 **/
void
nih_tree_init (NihTree *tree)
{
	nih_assert (tree != NULL);

	tree->parent = tree->left = tree->right = NULL;
}

/**
 * nih_tree_new:
 * @parent: parent of new node.
 *
 * Allocates a new tree structure, usually used as the root of a new
 * binary tree.  You may prefer to allocate the NihTree structure statically
 * and use nih_tree_init() to initialise it instead.
 *
 * The structure is allocated using nih_alloc() so can be used as a context
 * to other allocations.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the new tree node or NULL if the allocation failed.
 **/
NihTree *
nih_tree_new (const void *parent)
{
	NihTree *tree;

	tree = nih_new (parent, NihTree);
	if (! tree)
		return NULL;

	nih_tree_init (tree);

	return tree;
}




/**
 * nih_tree_add:
 * @tree: node in the destination tree,
 * @node: node to be added to the tree,
 * @where: where @node should be added.
 *
 * Adds @node to a new binary tree, either as a child of or, or replacing,
 * the existing node @tree.  The exact position is determined by @where,
 * which may be NIH_TREE_LEFT or NIH_TREE_RIGHT to indicate that @node
 * should be a child of @tree or NIH_TREE_HERE to indicate that @node
 * should replace @tree.
 *
 * If @node is already in another tree it is removed so there is no need
 * to call nih_tree_remove() before this function.  There is also no
 * requirement that the trees be different, so this can be used to reorder
 * a tree.
 *
 * Returns: node replaced by @node, normally NULL.
 **/
NihTree *
nih_tree_add (NihTree      *tree,
	      NihTree      *node,
	      NihTreeWhere  where)
{
	NihTree *replaced = NULL;

	nih_assert (tree != NULL);

	if (node)
		nih_tree_remove (node);

	if (where == NIH_TREE_LEFT) {
		replaced = tree->left;
		if (replaced)
			replaced->parent = NULL;

		tree->left = node;
		if (node)
			node->parent = tree;

	} else if (where == NIH_TREE_RIGHT) {
		replaced = tree->right;
		if (replaced)
			replaced->parent = NULL;

		tree->right = node;
		if (node)
			node->parent = tree;

	}

	return replaced;
}


/**
 * nih_tree_remove:
 * @node: node to be removed.
 *
 * Removes @node and its children from the containing tree.  Neither the
 * node nor children are freed, and the children are not unlinked from the
 * node.  Instead the node is returned so that it can be added to another
 * tree (through there's no need to call nih_tree_remove() first if you
 * wanted to do that) or used as the root of a new tree.
 *
 * Returns: @node as a root node.
 **/
NihTree *
nih_tree_remove (NihTree *node)
{
	nih_assert (node != NULL);

	if (node->parent) {
		if (node->parent->left == node) {
			node->parent->left = NULL;
		} else if (node->parent->right == node) {
			node->parent->right = NULL;
		}

		node->parent = NULL;
	}

	return node;
}

/**
 * nih_tree_unlink:
 * @node: node to be removed.
 *
 * Removes @node from its containing tree, as nih_tree_remove() does, but
 * also unlinks the node's children from itself so that they don't have
 * a dangling pointer.
 *
 * Returns: @node.
 **/
NihTree *
nih_tree_unlink (NihTree *node)
{
	nih_assert (node != NULL);

	nih_tree_remove (node);

	if (node->left)
		node->left->parent = NULL;

	if (node->right)
		node->right->parent = NULL;

	node->left = node->right = NULL;

	return node;
}

/**
 * nih_tree_destructor:
 * @node: node to be removed.
 *
 * Removes @node from its containing tree, intended to be used as an
 * nih_alloc() destructor so that the node is automatically removed if
 * it is freed.
 *
 * Returns: zero.
 **/
int
nih_tree_destructor (NihTree *node)
{
	nih_assert (node != NULL);

	nih_tree_unlink (node);

	return 0;
}

/**
 * nih_tree_free:
 * @node: node to be removed and freed.
 *
 * Removes @node from its containing tree and frees the memory allocated
 * for it.  @node must have been previously allocated using nih_alloc().
 *
 * Children of @node are not automatically freed unless they are allocated
 * as nih_alloc() children of @node.
 *
 * You must also take care of freeing the data attached to the node yourself
 * by either freeing it before calling this function or allocating it using
 * the node as the context.
 *
 * Returns: return value from destructor, or 0.
 **/
int
nih_tree_free (NihTree *node)
{
	nih_assert (node != NULL);

	nih_tree_unlink (node);
	return nih_free (node);
}


/**
 * nih_tree_next:
 * @tree: tree to iterate,
 * @node: node just visited.
 *
 * Iterates the @tree in-order non-recursively; to obtain the first node,
 * @tree should be set to the root of the tree and @node should be NULL.
 * Then for subsequent nodes, @node should be the previous return value
 * from this function.
 *
 * Returns: next in-order node within @tree or NULL if no further nodes.
 **/
NihTree *
nih_tree_next (NihTree *tree,
	       NihTree *node)
{
	NihTree *prev;

	nih_assert (tree != NULL);

	if (node) {
		prev = node;
		if (node->right) {
			node = node->right;
		} else {
			node = node->parent;
		}
	} else {
		prev = NULL;
		node = tree;
	}

	while (node) {
		NihTree *tmp = node;

		if (prev == node->parent) {
			if (node->left) {
				node = node->left;
			} else {
				return node;
			}
		} else if (prev == node->left) {
			return node;
		} else {
			node = node->parent;
		}

		prev = tmp;
	}

	return NULL;
}

/**
 * nih_tree_prev:
 * @tree: tree to iterate,
 * @node: node just visited.
 *
 * Reverse-iterates the @tree in-order non-recursively; to obtain the last
 * node, @tree should be set to the root of the tree and @node should be NULL.
 * Then for subsequent nodes, @node should be the previous return value
 * from this function.
 *
 * Returns: previous in-order node within @tree or NULL if no further nodes.
 **/
NihTree *
nih_tree_prev (NihTree *tree,
	       NihTree *node)
{
	NihTree *prev;

	nih_assert (tree != NULL);

	if (node) {
		prev = node;
		if (node->left) {
			node = node->left;
		} else {
			node = node->parent;
		}
	} else {
		prev = NULL;
		node = tree;
	}

	while (node) {
		NihTree *tmp = node;

		if (prev == node->parent) {
			if (node->right) {
				node = node->right;
			} else {
				return node;
			}
		} else if (prev == node->right) {
			return node;
		} else {
			node = node->parent;
		}

		prev = tmp;
	}

	return NULL;
}
