/* libnih
 *
 * tree.c - generic binary tree implementation
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
 * @parent: parent object for new node.
 *
 * Allocates a new tree structure, usually used as the root of a new
 * binary tree.  You may prefer to allocate the NihTree structure statically
 * and use nih_tree_init() to initialise it instead.
 *
 * The structure is allocated using nih_alloc() so can be used as a context
 * to other allocations.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned tree node.  When all parents
 * of the returned tree node are freed, the returned tree node will also be
 * freed.
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

	nih_alloc_set_destructor (tree, nih_tree_destroy);

	return tree;
}

/**
 * nih_tree_entry_new:
 * @parent: parent object for new entry.
 *
 * Allocates a new tree entry structure, leaving the caller to set the
 * data of the entry.
 *
 * The structure is allocated using nih_alloc() so can be used as a context
 * to other allocations.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned tree entry.  When all parents
 * of the returned tree entry are freed, the returned tree entry will also be
 * freed.
 *
 * Returns: the new tree entry or NULL if the allocation failed.
 **/
NihTreeEntry *
nih_tree_entry_new (const void *parent)
{
	NihTreeEntry *tree;

	tree = nih_new (parent, NihTreeEntry);
	if (! tree)
		return NULL;

	nih_tree_init (&tree->node);

	nih_alloc_set_destructor (tree, nih_tree_destroy);

	tree->data = NULL;

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
 * nih_tree_destroy:
 * @node: node to be removed.
 *
 * Removes @node from its containing tree.
 *
 * Normally used or called from an nih_alloc() destructor so that the list
 * item is automatically removed from its containing list when freed.
 *
 * Returns: zero.
 **/
int
nih_tree_destroy (NihTree *node)
{
	nih_assert (node != NULL);

	nih_tree_unlink (node);

	return 0;
}


/**
 * VISIT:
 * @_node: node to check.
 *
 * Macro to expand to check whether a node is set, and whether the filter is
 * either unset or says not to filter this node.
 **/
#define VISIT(_node) ((_node) && ((! filter) || (! filter (data, (_node)))))

/**
 * nih_tree_next_full:
 * @tree: tree to iterate,
 * @node: node just visited,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Iterates the @tree in-order non-recursively; to obtain the first node,
 * @tree should be set to the root of the tree and @node should be NULL.
 * Then for subsequent nodes, @node should be the previous return value
 * from this function.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * Returns: next in-order node within @tree or NULL if no further nodes.
 **/
NihTree *
nih_tree_next_full (NihTree       *tree,
		    NihTree       *node,
		    NihTreeFilter  filter,
		    void          *data)
{
	NihTree *prev;

	nih_assert (tree != NULL);

	if (node) {
		prev = node;
		if (VISIT (node->right)) {
			node = node->right;
		} else {
			if (node == tree)
				return NULL;

			node = node->parent;
		}
	} else {
		prev = tree->parent;
		node = tree;
	}

	for (;;) {
		NihTree *tmp = node;

		if ((prev == node->parent) && VISIT (node->left)) {
			node = node->left;
		} else if (VISIT (node->right) && (prev == node->right)) {
			if (node == tree)
				return NULL;

			node = node->parent;
		} else if (VISIT (node)) {
			return node;
		} else {
			return NULL;
		}

		prev = tmp;
	}
}

/**
 * nih_tree_prev_full:
 * @tree: tree to iterate,
 * @node: node just visited,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Reverse-iterates the @tree in-order non-recursively; to obtain the last
 * node, @tree should be set to the root of the tree and @node should be NULL.
 * Then for subsequent nodes, @node should be the previous return value
 * from this function.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * Returns: previous in-order node within @tree or NULL if no further nodes.
 **/
NihTree *
nih_tree_prev_full (NihTree       *tree,
		    NihTree       *node,
		    NihTreeFilter  filter,
		    void          *data)
{
	NihTree *prev;

	nih_assert (tree != NULL);

	if (node) {
		prev = node;
		if (VISIT (node->left)) {
			node = node->left;
		} else {
			if (node == tree)
				return NULL;

			node = node->parent;
		}
	} else {
		prev = tree->parent;
		node = tree;
	}

	for (;;) {
		NihTree *tmp = node;

		if ((prev == node->parent) && VISIT (node->right)) {
			node = node->right;
		} else if (VISIT (node->left) && (prev == node->left)) {
			if (node == tree)
				return NULL;

			node = node->parent;
		} else if (VISIT (node)) {
			return node;
		} else {
			return NULL;
		}

		prev = tmp;
	}

	return NULL;
}


/**
 * nih_tree_next_pre_full:
 * @tree: tree to iterate,
 * @node: node just visited,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Iterates the @tree in-order non-recursively; to obtain the first node,
 * @tree should be set to the root of the tree and @node should be NULL.
 * Then for subsequent nodes, @node should be the previous return value
 * from this function.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * Returns: next in-order node within @tree or NULL if no further nodes.
 **/
NihTree *
nih_tree_next_pre_full (NihTree       *tree,
			NihTree       *node,
			NihTreeFilter  filter,
			void          *data)
{
	NihTree *prev;

	nih_assert (tree != NULL);

	if (node) {
		prev = node;
		if (VISIT (node->left)) {
			return node->left;
		} else if (VISIT (node->right)) {
			return node->right;
		} else {
			if (node == tree)
				return NULL;

			node = node->parent;
		}
	} else if (VISIT (tree)) {
		return tree;
	} else {
		return NULL;
	}

	for (;;) {
		NihTree *tmp = node;

		if ((prev != node->right) && VISIT (node->right)) {
			return node->right;
		} else {
			if (node == tree)
				return NULL;

			node = node->parent;
		}

		prev = tmp;
	}
}

/**
 * nih_tree_prev_pre_full:
 * @tree: tree to iterate,
 * @node: node just visited,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Reverse-iterates the @tree in-order non-recursively; to obtain the last
 * node, @tree should be set to the root of the tree and @node should be NULL.
 * Then for subsequent nodes, @node should be the previous return value
 * from this function.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * Returns: previous in-order node within @tree or NULL if no further nodes.
 **/
NihTree *
nih_tree_prev_pre_full (NihTree       *tree,
			NihTree       *node,
			NihTreeFilter  filter,
			void          *data)
{
	NihTree *prev;

	nih_assert (tree != NULL);

	if (node) {
		prev = node;
		if (node == tree)
			return NULL;

		node = node->parent;
	} else {
		prev = tree->parent;
		node = tree;
	}

	for (;;) {
		NihTree *tmp = node;

		if ((prev == node->parent) && VISIT (node->right)) {
			node = node->right;
		} else if ((prev != node->left) && VISIT (node->left)) {
			node = node->left;
		} else if (VISIT (node)) {
			return node;
		} else {
			return NULL;
		}

		prev = tmp;
	}
}


/**
 * nih_tree_next_post_full:
 * @tree: tree to iterate,
 * @node: node just visited,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Iterates the @tree in-order non-recursively; to obtain the first node,
 * @tree should be set to the root of the tree and @node should be NULL.
 * Then for subsequent nodes, @node should be the previous return value
 * from this function.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * Returns: next in-order node within @tree or NULL if no further nodes.
 **/
NihTree *
nih_tree_next_post_full (NihTree       *tree,
			 NihTree       *node,
			 NihTreeFilter  filter,
			 void          *data)
{
	NihTree *prev;

	nih_assert (tree != NULL);

	if (node) {
		prev = node;
		if (node == tree)
			return NULL;

		node = node->parent;
	} else {
		prev = tree->parent;
		node = tree;
	}

	for (;;) {
		NihTree *tmp = node;

		if ((prev == node->parent) && VISIT (node->left)) {
			node = node->left;
		} else if ((prev != node->right) && VISIT (node->right)) {
			node = node->right;
		} else if (VISIT (node)) {
			return node;
		} else {
			return NULL;
		}

		prev = tmp;
	}
}

/**
 * nih_tree_prev_post_full:
 * @tree: tree to iterate,
 * @node: node just visited,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Reverse-iterates the @tree in-order non-recursively; to obtain the last
 * node, @tree should be set to the root of the tree and @node should be NULL.
 * Then for subsequent nodes, @node should be the previous return value
 * from this function.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * Returns: previous in-order node within @tree or NULL if no further nodes.
 **/
NihTree *
nih_tree_prev_post_full (NihTree       *tree,
			 NihTree       *node,
			 NihTreeFilter  filter,
			 void          *data)
{
	NihTree *prev;

	nih_assert (tree != NULL);

	if (node) {
		prev = node;
		if (VISIT (node->right)) {
			return node->right;
		} else if (VISIT (node->left)) {
			return node->left;
		} else {
			if (node == tree)
				return NULL;

			node = node->parent;
		}
	} else if (VISIT (tree)) {
		return tree;
	} else {
		return NULL;
	}

	for (;;) {
		NihTree *tmp = node;

		if ((prev != node->left) && VISIT (node->left)) {
			return node->left;
		} else {
			if (node == tree)
				return NULL;

			node = node->parent;
		}

		prev = tmp;
	}
}
