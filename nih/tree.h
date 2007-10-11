/* libnih
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

#ifndef NIH_TREE_H
#define NIH_TREE_H

#include <nih/macros.h>


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
#define NIH_TREE_FOREACH(tree, iter) \
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
#define NIH_TREE_FOREACH_PRE(tree, iter) \
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
#define NIH_TREE_FOREACH_POST(tree, iter) \
	for (NihTree *iter = nih_tree_next_post ((tree), NULL); iter != NULL; \
	     iter = nih_tree_next_post ((tree), iter))


NIH_BEGIN_EXTERN

void          nih_tree_init      (NihTree *tree);
NihTree *     nih_tree_new       (const void *parent)
	__attribute__ ((warn_unused_result, malloc));
NihTreeEntry *nih_tree_entry_new (const void *parent)
	__attribute__ ((warn_unused_result, malloc));

NihTree *     nih_tree_add       (NihTree *tree, NihTree *node,
				  NihTreeWhere where);

NihTree *     nih_tree_remove    (NihTree *node);
NihTree *     nih_tree_unlink    (NihTree *node);
int           nih_tree_destroy   (NihTree *node);

NihTree *     nih_tree_next      (NihTree *tree, NihTree *node);
NihTree *     nih_tree_prev      (NihTree *tree, NihTree *node);

NihTree *     nih_tree_next_pre  (NihTree *tree, NihTree *node);
NihTree *     nih_tree_prev_pre  (NihTree *tree, NihTree *node);

NihTree *     nih_tree_next_post (NihTree *tree, NihTree *node);
NihTree *     nih_tree_prev_post (NihTree *tree, NihTree *node);

NIH_END_EXTERN

#endif /* NIH_TREE_H */
