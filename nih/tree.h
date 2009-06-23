/* libnih
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

#ifndef NIH_TREE_H
#define NIH_TREE_H

#include <nih/macros.h>

/**
 * Provides a generic binary tree implementation.  No assumption is
 * made about the structure of the tree, or its rules.  Instead when
 * you add a node to a tree, you must specify the parent node and whether
 * to add the new node to its left or right.
 *
 * Tree nodes may be created in one of two ways.  The most common is to
 * embed the NihTree structure as the first member of your own structure,
 * and initialise it with nih_tree_init() after allocating the structure.
 * Alternatively you may create NihTreeEntry structures with
 * nih_tree_entry_new() and point at your own data from them.
 *
 * If you need no data for the tree root, you may use NihTree itself and
 * allocate it with nih_tree_new().
 *
 * Nodes may be added to the tree with nih_tree_add(), passing the parent
 * node, the new node and whether to add to the left or right.
 *
 * To remove a node from the tree, and its children, use nih_tree_remove();
 * the node removed becomes the root of a new tree.
 *
 * Nodes may be moved between trees, or relocated within a tree, by simply
 * calling nih_tree_add() - there's no need to call nih_tree_remove() first.
 *
 * A node may also be removed from a tree and from its children using
 * nih_tree_unlink(); the node removed, and each of its children, become
 * the roots of new trees.
 *
 * Tree-iteration may be performed non-recursively in a pre-order, in-order
 * or post-order fashion; forwards or backwards.  The functions
 * nih_tree_next_full(), nih_tree_prev_full(), nih_tree_next_pre_full(),
 * nih_tree_prev_pre_full(), nih_tree_next_post_full() and
 * nih_tree_prev_post_full() all return the next or previous node, allowing
 * for filtering.  If you do not need to filter macros are provided that
 * pass NULL, named without the _full extension.
 *
 * These are almost always used in a for loop, so macros are provided that
 * expand to a for loop for each of the different orders;
 * NIH_TREE_FOREACH_FULL(), NIH_TREE_FOREACH_PRE_FULL() and
 * NIH_TREE_FOREACH_POST_FULL().  Versions which pass NULL for the filter
 * are provided without the _FULL extension.
 **/


/**
 * NihTreeWhere:
 *
 * These constants define a position for one node, relative to another;
 * usually for when adding a node to an existing tree.
 **/
typedef enum {
	NIH_TREE_LEFT  = -1,
	NIH_TREE_RIGHT =  1,
} NihTreeWhere;


/**
 * NihTree:
 * @parent: parent node in the tree,
 * @left: left child node,
 * @right: right child node.
 *
 * This structure can be used both to refer to a binary tree and can be
 * placed in your own structures to use them as tree nodes.
 *
 * A node without any parent (root node) has @parent set to NULL, nodes
 * without any children (leaf nodes) have @left and @right set to NULL.
 *
 * NihTree is most useful for implementing pure binary trees, where the
 * properties of that structure (such as simple location or traversal) are
 * desired.
 *
 * General trees (where each node may have zero or more nodes, beyond two)
 * can be implemented using binary trees as described by Knuth (fundamentally,
 * head right for siblings, left for children) or as lists of children in
 * each node (such as used by nih_alloc); pick whichever suits your data
 * best.
 **/
typedef struct nih_tree {
	struct nih_tree *parent, *left, *right;
} NihTree;

/**
 * NihTreeEntry:
 * @node: tree node,
 * @data: data pointer,
 * @str: string pointer,
 * @int_data: integer value.
 *
 * This structure can be used as a generic NihTree node that contains
 * a pointer to generic data, a string or contains an integer value.
 *
 * You should take care of setting the data yourself.
 **/
typedef struct nih_tree_entry {
	NihTree node;
	union {
		void *data;
		char *str;
		int   int_data;
	};
} NihTreeEntry;


/**
 * NihTreeFilter:
 * @data: data pointer,
 * @node: node to be visited.
 *
 * A tree filter is a function that is called when iterating a tree to
 * determine whether a particular node and its children should be ignored.
 *
 * Returns: TRUE if the node should be ignored, FALSE otherwise.
 **/
typedef int (*NihTreeFilter) (void *data, NihTree *node);


/**
 * NIH_TREE_FOREACH_FULL:
 * @tree: root of the tree to iterate,
 * @iter: name of iterator variable,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Expands to a for statement that in-order iterates over each node in @tree,
 * setting @iter to each node for the block within the loop.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * You should not make changes to the structure of the tree while iterating,
 * since the order will be relatively unpredictable.
 **/
#define NIH_TREE_FOREACH_FULL(tree, iter, filter, data)			\
	for (NihTree *iter = nih_tree_next_full ((tree), NULL, (filter), (data)); \
	     iter != NULL;						\
	     iter = nih_tree_next_full ((tree), iter, (filter), (data)))

/**
 * NIH_TREE_FOREACH_PRE_FULL:
 * @tree: root of the tree to iterate,
 * @iter: name of iterator variable,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Expands to a for statement that pre-order iterates over each node in @tree,
 * setting @iter to each node for the block within the loop.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * You should not make changes to the structure of the tree while iterating,
 * since the order will be relatively unpredictable.
 **/
#define NIH_TREE_FOREACH_PRE_FULL(tree, iter, filter, data)		\
	for (NihTree *iter = nih_tree_next_pre_full ((tree), NULL, (filter), (data)); \
	     iter != NULL;						\
	     iter = nih_tree_next_pre_full ((tree), iter, (filter), (data)))

/**
 * NIH_TREE_FOREACH_POST_FULL:
 * @tree: root of the tree to iterate,
 * @iter: name of iterator variable,
 * @filter: filter function to test each node,
 * @data: data pointer to pass to @filter.
 *
 * Expands to a for statement that post-order iterates over each node in @tree,
 * setting @iter to each node for the block within the loop.
 *
 * If @filter is given, it will be called for each node visited and must
 * return FALSE otherwise the node and its children will be ignored.
 *
 * You should not make changes to the structure of the tree while iterating,
 * since the order will be relatively unpredictable.
 **/
#define NIH_TREE_FOREACH_POST_FULL(tree, iter, filter, data)		\
	for (NihTree *iter = nih_tree_next_post_full ((tree), NULL, (filter), (data)); \
	     iter != NULL;						\
	     iter = nih_tree_next_post_full ((tree), iter, (filter), (data)))


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
#define nih_tree_next(tree, node)			\
	nih_tree_next_full ((tree), (node), NULL, NULL)

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
#define nih_tree_prev(tree, node)			\
	nih_tree_prev_full ((tree), (node), NULL, NULL)

/**
 * nih_tree_next_pre:
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
#define nih_tree_next_pre(tree, node)				\
	nih_tree_next_pre_full ((tree), (node), NULL, NULL)

/**
 * nih_tree_prev_pre:
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
#define nih_tree_prev_pre(tree, node)				\
	nih_tree_prev_pre_full ((tree), (node), NULL, NULL)

/**
 * nih_tree_next_post:
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
#define nih_tree_next_post(tree, node)				\
	nih_tree_next_post_full ((tree), (node), NULL, NULL)

/**
 * nih_tree_prev_post:
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
#define nih_tree_prev_post(tree, node)				\
	nih_tree_prev_post_full ((tree), (node), NULL, NULL)


/**
 * NIH_TREE_FOREACH:
 * @tree: root of the tree to iterate,
 * @iter: name of iterator variable.
 *
 * Expands to a for statement that in-order iterates over each node in @tree,
 * setting @iter to each node for the block within the loop.
 *
 * You should not make changes to the structure of the tree while iterating,
 * since the order will be relatively unpredictable.
 **/
#define NIH_TREE_FOREACH(tree, iter)					\
	for (NihTree *iter = nih_tree_next ((tree), NULL); iter != NULL; \
	     iter = nih_tree_next ((tree), iter))

/**
 * NIH_TREE_FOREACH_PRE:
 * @tree: root of the tree to iterate,
 * @iter: name of iterator variable.
 *
 * Expands to a for statement that pre-order iterates over each node in @tree,
 * setting @iter to each node for the block within the loop.
 *
 * You should not make changes to the structure of the tree while iterating,
 * since the order will be relatively unpredictable.
 **/
#define NIH_TREE_FOREACH_PRE(tree, iter)				\
	for (NihTree *iter = nih_tree_next_pre ((tree), NULL); iter != NULL; \
	     iter = nih_tree_next_pre ((tree), iter))

/**
 * NIH_TREE_FOREACH_POST:
 * @tree: root of the tree to iterate,
 * @iter: name of iterator variable.
 *
 * Expands to a for statement that post-order iterates over each node in @tree,
 * setting @iter to each node for the block within the loop.
 *
 * You should not make changes to the structure of the tree while iterating,
 * since the order will be relatively unpredictable.
 **/
#define NIH_TREE_FOREACH_POST(tree, iter)				\
	for (NihTree *iter = nih_tree_next_post ((tree), NULL); iter != NULL; \
	     iter = nih_tree_next_post ((tree), iter))


NIH_BEGIN_EXTERN

void          nih_tree_init           (NihTree *tree);
NihTree *     nih_tree_new            (const void *parent)
	__attribute__ ((warn_unused_result, malloc));
NihTreeEntry *nih_tree_entry_new      (const void *parent)
	__attribute__ ((warn_unused_result, malloc));

NihTree *     nih_tree_add            (NihTree *tree, NihTree *node,
				       NihTreeWhere where);

NihTree *     nih_tree_remove         (NihTree *node);
NihTree *     nih_tree_unlink         (NihTree *node);
int           nih_tree_destroy        (NihTree *node);

NihTree *     nih_tree_next_full      (NihTree *tree, NihTree *node,
				       NihTreeFilter filter, void *data);
NihTree *     nih_tree_prev_full      (NihTree *tree, NihTree *node,
				       NihTreeFilter filter, void *data);

NihTree *     nih_tree_next_pre_full  (NihTree *tree, NihTree *node,
				       NihTreeFilter filter, void *data);
NihTree *     nih_tree_prev_pre_full  (NihTree *tree, NihTree *node,
				       NihTreeFilter filter, void *data);

NihTree *     nih_tree_next_post_full (NihTree *tree, NihTree *node,
				       NihTreeFilter filter, void *data);
NihTree *     nih_tree_prev_post_full (NihTree *tree, NihTree *node,
				       NihTreeFilter filter, void *data);

NIH_END_EXTERN

#endif /* NIH_TREE_H */
