/* libnih
 *
 * test_tree.c - test suite for nih/tree.c
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

#include <nih/test.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/tree.h>


void
test_init (void)
{
	NihTree node;

	/* Check that nih_tree_init correctly initialises an empty tree
	 * node with all three pointers set to NULL.
	 */
	TEST_FUNCTION ("nih_tree_init");
	nih_tree_init (&node);

	TEST_EQ_P (node.parent, NULL);
	TEST_EQ_P (node.left, NULL);
	TEST_EQ_P (node.right, NULL);
}

void
test_new (void)
{
	NihTree *tree;

	/* Check that nih_tree_new allocates a new empty tree node with
	 * nih_alloc and that it is initialised with all three pointers
	 * set to NULL.  If allocation fails, we should get NULL returned.
	 */
	TEST_FUNCTION ("nih_tree_new");
	TEST_ALLOC_FAIL {
		tree = nih_tree_new (NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (tree, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (tree, sizeof (NihTree));
		TEST_EQ_P (tree->parent, NULL);
		TEST_EQ_P (tree->left, NULL);
		TEST_EQ_P (tree->right, NULL);

		nih_free (tree);
	}
}

void
test_entry_new (void)
{
	NihTreeEntry *tree;

	/* Check that nih_tree_entry_new allocates a new empty tree node with
	 * nih_alloc and that it is initialised with all three pointers
	 * set to NULL.  If allocation fails, we should get NULL returned.
	 */
	TEST_FUNCTION ("nih_tree_entry_new");
	TEST_ALLOC_FAIL {
		tree = nih_tree_entry_new (NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (tree, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (tree, sizeof (NihTreeEntry));
		TEST_EQ_P (tree->node.parent, NULL);
		TEST_EQ_P (tree->node.left, NULL);
		TEST_EQ_P (tree->node.right, NULL);
		TEST_EQ_P (tree->data, NULL);

		nih_free (tree);
	}
}


void
test_add (void)
{
	NihTree *tree, *node1, *node2, *node3, *node4, *ptr;


	TEST_FUNCTION ("nih_tree_add");
	tree = nih_tree_new (NULL);


	/* Check that we can add a node as a left-hand child of another node,
	 * where no child existed before.
	 */
	TEST_FEATURE ("as left-hand child");
	node1 = nih_tree_new (tree);

	ptr = nih_tree_add (tree, node1, NIH_TREE_LEFT);

	TEST_EQ_P (ptr, NULL);
	TEST_EQ_P (node1->parent, tree);
	TEST_EQ_P (tree->left, node1);


	/* Check that we can add a node as a right-child of another node,
	 * where no child existing before.
	 */
	TEST_FEATURE ("as right-hand child");
	node2 = nih_tree_new (tree);

	ptr = nih_tree_add (node1, node2, NIH_TREE_RIGHT);

	TEST_EQ_P (ptr, NULL);
	TEST_EQ_P (node2->parent, node1);
	TEST_EQ_P (node1->right, node2);


	/* Check that we can add a node as a left-child of another node,
	 * replacing the child in that slot already.  We should have the
	 * replaced child returned.
	 */
	TEST_FEATURE ("as replacement left-hand child");
	node3 = nih_tree_new (tree);

	ptr = nih_tree_add (tree, node3, NIH_TREE_LEFT);

	TEST_EQ_P (ptr, node1);
	TEST_EQ_P (ptr->parent, NULL);
	TEST_EQ_P (node3->parent, tree);
	TEST_EQ_P (tree->left, node3);


	/* Check that we can add a node as a right-child of another node,
	 * replacing the child in that slot already.  We should have the
	 * replaced child returned.
	 */
	TEST_FEATURE ("as replacement right-hand child");
	node4 = nih_tree_new (tree);

	ptr = nih_tree_add (node1, node4, NIH_TREE_RIGHT);

	TEST_EQ_P (ptr, node2);
	TEST_EQ_P (ptr->parent, NULL);
	TEST_EQ_P (node4->parent, node1);
	TEST_EQ_P (node1->right, node4);


	/* Check that we can swap a node within a tree from one child to
	 * another, getting the node that was replaced in return.
	 */
	TEST_FEATURE ("within same tree");
	nih_tree_add (tree, node1, NIH_TREE_RIGHT);
	nih_tree_add (node1, node2, NIH_TREE_LEFT);

	ptr = nih_tree_add (tree, node1, NIH_TREE_LEFT);

	TEST_EQ_P (ptr, node3);
	TEST_EQ_P (ptr->parent, NULL);
	TEST_EQ_P (node1->parent, tree);
	TEST_EQ_P (tree->left, node1);


	/* Check that we can perform a tree rotation with just two calls
	 * on the add function.
	 */
	TEST_FEATURE ("with tree rotation");
	nih_tree_add (tree, node3, NIH_TREE_RIGHT);

	ptr = nih_tree_add (tree, tree->left->right, NIH_TREE_LEFT);

	TEST_EQ_P (ptr, node1);
	TEST_EQ_P (ptr->parent, NULL);
	TEST_EQ_P (ptr->right, NULL);
	TEST_EQ_P (tree->left, node4);
	TEST_EQ_P (node4->parent, tree);

	ptr = nih_tree_add (ptr, tree, NIH_TREE_RIGHT);

	TEST_EQ_P (ptr, NULL);
	TEST_EQ_P (node1->parent, NULL);
	TEST_EQ_P (node1->left, node2);
	TEST_EQ_P (node2->parent, node1);
	TEST_EQ_P (node2->left, NULL);
	TEST_EQ_P (node2->right, NULL);
	TEST_EQ_P (node1->right, tree);
	TEST_EQ_P (tree->parent, node1);
	TEST_EQ_P (tree->left, node4);
	TEST_EQ_P (node4->parent, tree);
	TEST_EQ_P (tree->right, node3);
	TEST_EQ_P (node3->parent, tree);


	/* Check that a node may replace itself, with no damage; and that
	 * NULL should be returned since the replacement was a no-op.
	 */
	TEST_FEATURE ("as replacement for itself");

	ptr = nih_tree_add (node1, node2, NIH_TREE_LEFT);

	TEST_EQ_P (ptr, NULL);
	TEST_EQ_P (node2->parent, node1);
	TEST_EQ_P (node1->left, node2);


	/* Likewise check that moving a node within the tree to somewhere
	 * else in the tree, without replacing, just performs the move.
	 */
	TEST_FEATURE ("as move");

	ptr = nih_tree_add (node3, node4, NIH_TREE_LEFT);

	TEST_EQ_P (ptr, NULL);
	TEST_EQ_P (node3->left, node4);
	TEST_EQ_P (node4->parent, node3);
	TEST_EQ_P (tree->left, NULL);


	nih_free (tree);
}


void
test_remove (void)
{
	NihTree *tree, *node1, *node2, *ptr;

	TEST_FUNCTION ("nih_tree_remove");

	/* Check that we can remove a node from its containing tree, but that
	 * the node remains linked to its children.
	 */
	TEST_FEATURE ("with child node");
	tree = nih_tree_new (NULL);
	node1 = nih_tree_new (tree);
	node2 = nih_tree_new (tree);

	nih_tree_add (tree, node1, NIH_TREE_LEFT);
	nih_tree_add (node1, node2, NIH_TREE_RIGHT);

	ptr = nih_tree_remove (node1);

	TEST_EQ_P (ptr, node1);
	TEST_EQ_P (tree->left, NULL);
	TEST_EQ_P (node1->parent, NULL);
	TEST_EQ_P (node1->right, node2);
	TEST_EQ_P (node2->parent, node1);


	/* Check that an attempt to remove a root node has no effect. */
	TEST_FEATURE ("with root node");
	ptr = nih_tree_remove (node1);

	TEST_EQ_P (ptr, node1);
	TEST_EQ_P (node1->parent, NULL);
	TEST_EQ_P (node1->right, node2);
	TEST_EQ_P (node2->parent, node1);


	nih_free (tree);
}

void
test_unlink (void)
{
	NihTree *tree, *node1, *node2, *ptr;

	TEST_FUNCTION ("nih_tree_unlink");

	/* Check that we can unlink a node from its containing tree, and
	 * also have its children cast adrift.
	 */
	TEST_FEATURE ("with child node");
	tree = nih_tree_new (NULL);
	node1 = nih_tree_new (tree);
	node2 = nih_tree_new (tree);

	nih_tree_add (tree, node1, NIH_TREE_LEFT);
	nih_tree_add (node1, node2, NIH_TREE_RIGHT);

	ptr = nih_tree_unlink (node1);

	TEST_EQ_P (ptr, node1);
	TEST_EQ_P (tree->left, NULL);
	TEST_EQ_P (node1->parent, NULL);
	TEST_EQ_P (node1->left, NULL);
	TEST_EQ_P (node1->right, NULL);
	TEST_EQ_P (node2->parent, NULL);


	/* Check that an attempt to unlink a root node with children only
	 * unlinks the children.
	 */
	TEST_FEATURE ("with root node");
	nih_tree_add (tree, node1, NIH_TREE_LEFT);
	nih_tree_add (tree, node2, NIH_TREE_RIGHT);

	ptr = nih_tree_unlink (tree);

	TEST_EQ_P (ptr, tree);
	TEST_EQ_P (tree->parent, NULL);
	TEST_EQ_P (tree->left, NULL);
	TEST_EQ_P (tree->right, NULL);
	TEST_EQ_P (node1->parent, NULL);
	TEST_EQ_P (node2->parent, NULL);


	nih_free (tree);
}

void
test_destroy (void)
{
	NihTree *tree, *node1, *node2;
	int      ret;

	TEST_FUNCTION ("nih_tree_destroy");

	/* Check that we can unlink a node from its containing tree, and
	 * also have its children cast adrift.
	 */
	TEST_FEATURE ("with child node");
	tree = nih_tree_new (NULL);
	node1 = nih_tree_new (tree);
	node2 = nih_tree_new (tree);

	nih_tree_add (tree, node1, NIH_TREE_LEFT);
	nih_tree_add (node1, node2, NIH_TREE_RIGHT);

	ret = nih_tree_destroy (node1);

	TEST_EQ (ret, 0);

	TEST_EQ_P (tree->left, NULL);
	TEST_EQ_P (node1->parent, NULL);
	TEST_EQ_P (node1->left, NULL);
	TEST_EQ_P (node1->right, NULL);
	TEST_EQ_P (node2->parent, NULL);


	/* Check that an attempt to unlink a root node with children only
	 * unlinks the children.
	 */
	TEST_FEATURE ("with root node");
	nih_tree_add (tree, node1, NIH_TREE_LEFT);
	nih_tree_add (tree, node2, NIH_TREE_RIGHT);

	ret = nih_tree_destroy (tree);

	TEST_EQ (ret, 0);

	TEST_EQ_P (tree->parent, NULL);
	TEST_EQ_P (tree->left, NULL);
	TEST_EQ_P (tree->right, NULL);
	TEST_EQ_P (node1->parent, NULL);
	TEST_EQ_P (node2->parent, NULL);


	nih_free (tree);
}


/*
 * For the following tests, we use a specific tree as detailed below:
 *
 *                 a
 *               /   \
 *             /       \
 *           b           c
 *         /           /   \
 *       d           e       f
 *      /             \     / \
 *     g               h   i   k
 *   m'              p'
 *
 * We place each node in order, with node 'a' implicitly placed as the root.
 */

void
test_next (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_next");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can in-order iterate a reasonably complex tree,
	 * and that nih_tree_next returns the right pointer in each case
	 * until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['k' - 97];
	expect[1] = node['g' - 97];
	expect[2] = node['d' - 97];
	expect[3] = node['b' - 97];
	expect[4] = node['a' - 97];
	expect[5] = node['e' - 97];
	expect[6] = node['l' - 97];
	expect[7] = node['h' - 97];
	expect[8] = node['c' - 97];
	expect[9] = node['i' - 97];
	expect[10] = node['f' - 97];
	expect[11] = node['j' - 97];
	expect[12] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 12)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     13, i + 1);

		ptr = nih_tree_next (node['a' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['e' - 97];
	expect[1] = node['l' - 97];
	expect[2] = node['h' - 97];
	expect[3] = node['c' - 97];
	expect[4] = node['i' - 97];
	expect[5] = node['f' - 97];
	expect[6] = node['j' - 97];
	expect[7] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_next (node['c' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that we can iterate a tree with a single node. */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_next (node[0], NULL);
	TEST_EQ_P (ptr, node[0]);

	ptr = nih_tree_next (node[0], ptr);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}

void
test_foreach (void)
{
	NihTree *node[12], *expect[13];
	int      i;

	TEST_FUNCTION ("NIH_TREE_FOREACH");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can in-order iterate a reasonably complex tree,
	 * and that NIH_TREE_FOREACH sets the iterator to the right nodes
	 * in turn.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['k' - 97];
	expect[1] = node['g' - 97];
	expect[2] = node['d' - 97];
	expect[3] = node['b' - 97];
	expect[4] = node['a' - 97];
	expect[5] = node['e' - 97];
	expect[6] = node['l' - 97];
	expect[7] = node['h' - 97];
	expect[8] = node['c' - 97];
	expect[9] = node['i' - 97];
	expect[10] = node['f' - 97];
	expect[11] = node['j' - 97];
	expect[12] = NULL;

	i = 0;
	NIH_TREE_FOREACH (node['a' - 97], iter) {
		if (i > 11)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     12, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['e' - 97];
	expect[1] = node['l' - 97];
	expect[2] = node['h' - 97];
	expect[3] = node['c' - 97];
	expect[4] = node['i' - 97];
	expect[5] = node['f' - 97];
	expect[6] = node['j' - 97];
	expect[7] = NULL;

	i = 0;
	NIH_TREE_FOREACH (node['c' - 97], iter) {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}

void
test_prev (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_prev");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can reverse in-order iterate a reasonably complex
	 * tree, and that nih_tree_prev returns the right pointer in
	 * each case until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['j' - 97];
	expect[1] = node['f' - 97];
	expect[2] = node['i' - 97];
	expect[3] = node['c' - 97];
	expect[4] = node['h' - 97];
	expect[5] = node['l' - 97];
	expect[6] = node['e' - 97];
	expect[7] = node['a' - 97];
	expect[8] = node['b' - 97];
	expect[9] = node['d' - 97];
	expect[10] = node['g' - 97];
	expect[11] = node['k' - 97];
	expect[12] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 12)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     13, i + 1);

		ptr = nih_tree_prev (node['a' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['j' - 97];
	expect[1] = node['f' - 97];
	expect[2] = node['i' - 97];
	expect[3] = node['c' - 97];
	expect[4] = node['h' - 97];
	expect[5] = node['l' - 97];
	expect[6] = node['e' - 97];
	expect[7] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_prev (node['c' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that we can iterate a tree with a single node. */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_prev (node[0], NULL);
	TEST_EQ_P (ptr, node[0]);

	ptr = nih_tree_prev (node[0], ptr);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}


void
test_next_pre (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_next_pre");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can pre-order iterate a reasonably complex tree,
	 * and that nih_tree_next_pre returns the right pointer in each case
	 * until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['a' - 97];
	expect[1] = node['b' - 97];
	expect[2] = node['d' - 97];
	expect[3] = node['g' - 97];
	expect[4] = node['k' - 97];
	expect[5] = node['c' - 97];
	expect[6] = node['e' - 97];
	expect[7] = node['h' - 97];
	expect[8] = node['l' - 97];
	expect[9] = node['f' - 97];
	expect[10] = node['i' - 97];
	expect[11] = node['j' - 97];
	expect[12] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 12)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     13, i + 1);

		ptr = nih_tree_next_pre (node['a' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['c' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['h' - 97];
	expect[3] = node['l' - 97];
	expect[4] = node['f' - 97];
	expect[5] = node['i' - 97];
	expect[6] = node['j' - 97];
	expect[7] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_next_pre (node['c' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that we can iterate a tree with a single node. */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_next_pre (node[0], NULL);
	TEST_EQ_P (ptr, node[0]);

	ptr = nih_tree_next_pre (node[0], ptr);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}

void
test_foreach_pre (void)
{
	NihTree *node[12], *expect[13];
	int      i;

	TEST_FUNCTION ("NIH_TREE_FOREACH_PRE");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can in-order iterate a reasonably complex tree,
	 * and that NIH_TREE_FOREACH_PRE sets the iterator to the right nodes
	 * in turn.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['a' - 97];
	expect[1] = node['b' - 97];
	expect[2] = node['d' - 97];
	expect[3] = node['g' - 97];
	expect[4] = node['k' - 97];
	expect[5] = node['c' - 97];
	expect[6] = node['e' - 97];
	expect[7] = node['h' - 97];
	expect[8] = node['l' - 97];
	expect[9] = node['f' - 97];
	expect[10] = node['i' - 97];
	expect[11] = node['j' - 97];
	expect[12] = NULL;

	i = 0;
	NIH_TREE_FOREACH_PRE (node['a' - 97], iter) {
		if (i > 11)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     12, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['c' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['h' - 97];
	expect[3] = node['l' - 97];
	expect[4] = node['f' - 97];
	expect[5] = node['i' - 97];
	expect[6] = node['j' - 97];
	expect[7] = NULL;

	i = 0;
	NIH_TREE_FOREACH_PRE (node['c' - 97], iter) {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}

void
test_prev_pre (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_prev_pre");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can reverse pre-order iterate a reasonably complex
	 * tree, and that nih_tree_prev_pre returns the right pointer in each
	 * case until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['j' - 97];
	expect[1] = node['i' - 97];
	expect[2] = node['f' - 97];
	expect[3] = node['l' - 97];
	expect[4] = node['h' - 97];
	expect[5] = node['e' - 97];
	expect[6] = node['c' - 97];
	expect[7] = node['k' - 97];
	expect[8] = node['g' - 97];
	expect[9] = node['d' - 97];
	expect[10] = node['b' - 97];
	expect[11] = node['a' - 97];
	expect[12] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 12)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     13, i + 1);

		ptr = nih_tree_prev_pre (node['a' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['j' - 97];
	expect[1] = node['i' - 97];
	expect[2] = node['f' - 97];
	expect[3] = node['l' - 97];
	expect[4] = node['h' - 97];
	expect[5] = node['e' - 97];
	expect[6] = node['c' - 97];
	expect[7] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_prev_pre (node['c' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that we can iterate a tree with a single node. */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_prev_pre (node[0], NULL);
	TEST_EQ_P (ptr, node[0]);

	ptr = nih_tree_prev_pre (node[0], ptr);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}


void
test_next_post (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_next_post");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can post-order iterate a reasonably complex tree,
	 * and that nih_tree_next_post returns the right pointer in each case
	 * until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['k' - 97];
	expect[1] = node['g' - 97];
	expect[2] = node['d' - 97];
	expect[3] = node['b' - 97];
	expect[4] = node['l' - 97];
	expect[5] = node['h' - 97];
	expect[6] = node['e' - 97];
	expect[7] = node['i' - 97];
	expect[8] = node['j' - 97];
	expect[9] = node['f' - 97];
	expect[10] = node['c' - 97];
	expect[11] = node['a' - 97];
	expect[12] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 12)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     13, i + 1);

		ptr = nih_tree_next_post (node['a' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['l' - 97];
	expect[1] = node['h' - 97];
	expect[2] = node['e' - 97];
	expect[3] = node['i' - 97];
	expect[4] = node['j' - 97];
	expect[5] = node['f' - 97];
	expect[6] = node['c' - 97];
	expect[7] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_next_post (node['c' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that we can iterate a tree with a single node. */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_next_post (node[0], NULL);
	TEST_EQ_P (ptr, node[0]);

	ptr = nih_tree_next_post (node[0], ptr);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}

void
test_foreach_post (void)
{
	NihTree *node[12], *expect[13];
	int      i;

	TEST_FUNCTION ("NIH_TREE_FOREACH_POST");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can post-order iterate a reasonably complex tree,
	 * and that NIH_TREE_FOREACH_POST sets the iterator to the right nodes
	 * in turn.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['k' - 97];
	expect[1] = node['g' - 97];
	expect[2] = node['d' - 97];
	expect[3] = node['b' - 97];
	expect[4] = node['l' - 97];
	expect[5] = node['h' - 97];
	expect[6] = node['e' - 97];
	expect[7] = node['i' - 97];
	expect[8] = node['j' - 97];
	expect[9] = node['f' - 97];
	expect[10] = node['c' - 97];
	expect[11] = node['a' - 97];
	expect[12] = NULL;

	i = 0;
	NIH_TREE_FOREACH_POST (node['a' - 97], iter) {
		if (i > 11)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     12, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['l' - 97];
	expect[1] = node['h' - 97];
	expect[2] = node['e' - 97];
	expect[3] = node['i' - 97];
	expect[4] = node['j' - 97];
	expect[5] = node['f' - 97];
	expect[6] = node['c' - 97];
	expect[7] = NULL;

	i = 0;
	NIH_TREE_FOREACH_POST (node['c' - 97], iter) {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}

void
test_prev_post (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_prev_post");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can reverse post-order iterate a reasonably complex
	 * tree, and that nih_tree_prev_post returns the right pointer in each
	 * case until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['a' - 97];
	expect[1] = node['c' - 97];
	expect[2] = node['f' - 97];
	expect[3] = node['j' - 97];
	expect[4] = node['i' - 97];
	expect[5] = node['e' - 97];
	expect[6] = node['h' - 97];
	expect[7] = node['l' - 97];
	expect[8] = node['b' - 97];
	expect[9] = node['d' - 97];
	expect[10] = node['g' - 97];
	expect[11] = node['k' - 97];
	expect[12] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 12)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     13, i + 1);

		ptr = nih_tree_prev_post (node['a' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['c' - 97];
	expect[1] = node['f' - 97];
	expect[2] = node['j' - 97];
	expect[3] = node['i' - 97];
	expect[4] = node['e' - 97];
	expect[5] = node['h' - 97];
	expect[6] = node['l' - 97];
	expect[7] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 7)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_prev_post (node['c' - 97], ptr);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that we can iterate a tree with a single node. */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_prev_post (node[0], NULL);
	TEST_EQ_P (ptr, node[0]);

	ptr = nih_tree_prev_post (node[0], ptr);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}


/*
 * For the following tests, we use a specific tree as detailed below where
 * only those nodes marked with *s should be visited.
 *
 *                *a*
 *               /   \
 *             /       \
 *          *b*         *c*
 *         /           /   \
 *      *d*         *e*      f
 *      /             \     / \
 *     g              *h*  i   j
 *   k'              l'
 *
 * We place each node in order, with node 'a' implicitly placed as the root;
 * the filter function will return FALSE for 'j' and 'k' as well to test
 * that they're ignored since they are children.
 */

static int
my_filter (NihTree **nodes,
	   NihTree  *node)
{
	/* FALSE means that we DON'T ignore the node */
	if (! nodes)
		return TRUE;
	if (node == nodes['a' - 97])
		return FALSE;
	if (node == nodes['b' - 97])
		return FALSE;
	if (node == nodes['c' - 97])
		return FALSE;
	if (node == nodes['d' - 97])
		return FALSE;
	if (node == nodes['e' - 97])
		return FALSE;
	if (node == nodes['f' - 97])
		return TRUE;
	if (node == nodes['g' - 97])
		return TRUE;
	if (node == nodes['h' - 97])
		return FALSE;
	if (node == nodes['i' - 97])
		return TRUE;
	if (node == nodes['j' - 97])
		return FALSE;
	if (node == nodes['k' - 97])
		return FALSE;
	if (node == nodes['l' - 97])
		return TRUE;

	assert ("not reached");
	return TRUE;
}

void
test_next_full (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_next_full");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can in-order iterate a reasonably complex tree,
	 * and that nih_tree_next returns the right pointer in each case
	 * until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['d' - 97];
	expect[1] = node['b' - 97];
	expect[2] = node['a' - 97];
	expect[3] = node['e' - 97];
	expect[4] = node['h' - 97];
	expect[5] = node['c' - 97];
	expect[6] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_next_full (node['a' - 97], ptr,
					  (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['e' - 97];
	expect[1] = node['h' - 97];
	expect[2] = node['c' - 97];
	expect[3] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     4, i + 1);

		ptr = nih_tree_next_full (node['c' - 97], ptr,
					  (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that a tree with a single node to be ignored is not
	 * iterated.
	 */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_next_full (node[0], NULL,
				  (NihTreeFilter)my_filter, NULL);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}

void
test_foreach_full (void)
{
	NihTree *node[12], *expect[13];
	int      i;

	TEST_FUNCTION ("NIH_TREE_FOREACH_FULL");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can in-order iterate a reasonably complex tree,
	 * and that NIH_TREE_FOREACH sets the iterator to the right nodes
	 * in turn.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['d' - 97];
	expect[1] = node['b' - 97];
	expect[2] = node['a' - 97];
	expect[3] = node['e' - 97];
	expect[4] = node['h' - 97];
	expect[5] = node['c' - 97];
	expect[6] = NULL;

	i = 0;
	NIH_TREE_FOREACH_FULL (node['a' - 97], iter,
			       (NihTreeFilter)my_filter, &node) {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     6, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['e' - 97];
	expect[1] = node['h' - 97];
	expect[2] = node['c' - 97];
	expect[3] = NULL;

	i = 0;
	NIH_TREE_FOREACH_FULL (node['c' - 97], iter,
			       (NihTreeFilter)my_filter, &node) {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     3, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}

void
test_prev_full (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_prev_full");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can reverse in-order iterate a reasonably complex
	 * tree, and that nih_tree_prev returns the right pointer in
	 * each case until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['c' - 97];
	expect[1] = node['h' - 97];
	expect[2] = node['e' - 97];
	expect[3] = node['a' - 97];
	expect[4] = node['b' - 97];
	expect[5] = node['d' - 97];
	expect[6] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_prev_full (node['a' - 97], ptr,
					  (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['c' - 97];
	expect[1] = node['h' - 97];
	expect[2] = node['e' - 97];
	expect[3] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     4, i + 1);

		ptr = nih_tree_prev_full (node['c' - 97], ptr,
					  (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that a tree with a single node to be ignored is not
	 * iterated.
	 */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_prev_full (node[0], NULL,
				  (NihTreeFilter)my_filter, NULL);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}


void
test_next_pre_full (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_next_pre_full");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can pre-order iterate a reasonably complex tree,
	 * and that nih_tree_next_pre returns the right pointer in each case
	 * until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['a' - 97];
	expect[1] = node['b' - 97];
	expect[2] = node['d' - 97];
	expect[3] = node['c' - 97];
	expect[4] = node['e' - 97];
	expect[5] = node['h' - 97];
	expect[6] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_next_pre_full (node['a' - 97], ptr,
					      (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['c' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['h' - 97];
	expect[3] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     4, i + 1);

		ptr = nih_tree_next_pre_full (node['c' - 97], ptr,
					      (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that a tree with a single node to be ignored is not
	 * iterated.
	 */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_next_pre_full (node[0], NULL,
				      (NihTreeFilter)my_filter, NULL);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}

void
test_foreach_pre_full (void)
{
	NihTree *node[12], *expect[13];
	int      i;

	TEST_FUNCTION ("NIH_TREE_FOREACH_PRE_FULL");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can in-order iterate a reasonably complex tree,
	 * and that NIH_TREE_FOREACH_PRE sets the iterator to the right nodes
	 * in turn.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['a' - 97];
	expect[1] = node['b' - 97];
	expect[2] = node['d' - 97];
	expect[3] = node['c' - 97];
	expect[4] = node['e' - 97];
	expect[5] = node['h' - 97];
	expect[6] = NULL;

	i = 0;
	NIH_TREE_FOREACH_PRE_FULL (node['a' - 97], iter,
				   (NihTreeFilter)my_filter, &node) {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     6, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['c' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['h' - 97];
	expect[3] = NULL;

	i = 0;
	NIH_TREE_FOREACH_PRE_FULL (node['c' - 97], iter,
				   (NihTreeFilter)my_filter, &node) {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     3, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}

void
test_prev_pre_full (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_prev_pre_full");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can reverse pre-order iterate a reasonably complex
	 * tree, and that nih_tree_prev_pre returns the right pointer in each
	 * case until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['h' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['c' - 97];
	expect[3] = node['d' - 97];
	expect[4] = node['b' - 97];
	expect[5] = node['a' - 97];
	expect[6] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_prev_pre_full (node['a' - 97], ptr,
					      (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['h' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['c' - 97];
	expect[3] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     4, i + 1);

		ptr = nih_tree_prev_pre_full (node['c' - 97], ptr,
					      (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that a tree with a single node to be ignored is not
	 * iterated.
	 */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_prev_pre_full (node[0], NULL,
				      (NihTreeFilter)my_filter, NULL);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}


void
test_next_post_full (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_next_post_full");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can post-order iterate a reasonably complex tree,
	 * and that nih_tree_next_post returns the right pointer in each case
	 * until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['d' - 97];
	expect[1] = node['b' - 97];
	expect[2] = node['h' - 97];
	expect[3] = node['e' - 97];
	expect[4] = node['c' - 97];
	expect[5] = node['a' - 97];
	expect[6] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_next_post_full (node['a' - 97], ptr,
					       (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['h' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['c' - 97];
	expect[3] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     4, i + 1);

		ptr = nih_tree_next_post_full (node['c' - 97], ptr,
					       (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that a tree with a single node to be ignored is not
	 * iterated.
	 */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_next_post_full (node[0], NULL,
				       (NihTreeFilter)my_filter, NULL);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}

void
test_foreach_post_full (void)
{
	NihTree *node[12], *expect[13];
	int      i;

	TEST_FUNCTION ("NIH_TREE_FOREACH_POST_FULL");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can post-order iterate a reasonably complex tree,
	 * and that NIH_TREE_FOREACH_POST sets the iterator to the right nodes
	 * in turn.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['d' - 97];
	expect[1] = node['b' - 97];
	expect[2] = node['h' - 97];
	expect[3] = node['e' - 97];
	expect[4] = node['c' - 97];
	expect[5] = node['a' - 97];
	expect[6] = NULL;

	i = 0;
	NIH_TREE_FOREACH_POST_FULL (node['a' - 97], iter,
				    (NihTreeFilter)my_filter, &node) {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     6, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['h' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['c' - 97];
	expect[3] = NULL;

	i = 0;
	NIH_TREE_FOREACH_POST_FULL (node['c' - 97], iter,
				    (NihTreeFilter)my_filter, &node) {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     3, i + 1);

		if (iter != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], iter);

		i++;
	}


	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}

void
test_prev_post_full (void)
{
	NihTree *node[12], *expect[13], *ptr;
	int      i;

	TEST_FUNCTION ("nih_tree_prev_post_full");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node['a' - 97], node['b' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['a' - 97], node['c' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['b' - 97], node['d' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['e' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['c' - 97], node['f' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['d' - 97], node['g' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['e' - 97], node['h' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['f' - 97], node['i' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['f' - 97], node['j' - 97], NIH_TREE_RIGHT);
	nih_tree_add (node['g' - 97], node['k' - 97], NIH_TREE_LEFT);
	nih_tree_add (node['h' - 97], node['l' - 97], NIH_TREE_LEFT);


	/* Check that we can reverse post-order iterate a reasonably complex
	 * tree, and that nih_tree_prev_post returns the right pointer in each
	 * case until it finally returns NULL.
	 */
	TEST_FEATURE ("with full tree");
	expect[0] = node['a' - 97];
	expect[1] = node['c' - 97];
	expect[2] = node['e' - 97];
	expect[3] = node['h' - 97];
	expect[4] = node['b' - 97];
	expect[5] = node['d' - 97];
	expect[6] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 6)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     7, i + 1);

		ptr = nih_tree_prev_post_full (node['a' - 97], ptr,
					       (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	/* Check that we can limit the iteration to a partial tree rooted
	 * at the given tree node.
	 */
	TEST_FEATURE ("with partial tree");
	expect[0] = node['c' - 97];
	expect[1] = node['e' - 97];
	expect[2] = node['h' - 97];
	expect[3] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 3)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     4, i + 1);

		ptr = nih_tree_prev_post_full (node['c' - 97], ptr,
					       (NihTreeFilter)my_filter, &node);

		if (ptr != expect[i])
			TEST_FAILED ("wrong tree node for %d, expected %p got %p",
				     i, expect[i], ptr);

		i++;
	} while (ptr);


	for (i = 0; i < 12; i++)
		nih_free (node[i]);


	/* Check that a tree with a single node to be ignored is not
	 * iterated.
	 */
	TEST_FEATURE ("with single node");
	node[0] = nih_tree_new (NULL);

	ptr = nih_tree_prev_post_full (node[0], NULL,
				       (NihTreeFilter)my_filter, NULL);
	TEST_EQ_P (ptr, NULL);

	nih_free (node[0]);
}


int
main (int   argc,
      char *argv[])
{
	test_init ();
	test_new ();
	test_entry_new ();
	test_add ();
	test_remove ();
	test_unlink ();
	test_destroy ();
	test_next ();
	test_foreach ();
	test_prev ();
	test_next_pre ();
	test_foreach_pre ();
	test_prev_pre ();
	test_next_post ();
	test_foreach_post ();
	test_prev_post ();
	test_next_full ();
	test_foreach_full ();
	test_prev_full ();
	test_next_pre_full ();
	test_foreach_pre_full ();
	test_prev_pre_full ();
	test_next_post_full ();
	test_foreach_post_full ();
	test_prev_post_full ();

	return 0;
}
