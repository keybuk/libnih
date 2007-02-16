/* libnih
 *
 * test_tree.c - test suite for nih/tree.c
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
test_destructor (void)
{
	NihTree *tree, *node1, *node2;
	int      ret;

	TEST_FUNCTION ("nih_tree_destructor");

	/* Check that we can unlink a node from its containing tree, and
	 * also have its children cast adrift.
	 */
	TEST_FEATURE ("with child node");
	tree = nih_tree_new (NULL);
	node1 = nih_tree_new (tree);
	node2 = nih_tree_new (tree);

	nih_tree_add (tree, node1, NIH_TREE_LEFT);
	nih_tree_add (node1, node2, NIH_TREE_RIGHT);

	ret = nih_tree_destructor (node1);

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

	ret = nih_tree_destructor (tree);

	TEST_EQ (ret, 0);

	TEST_EQ_P (tree->parent, NULL);
	TEST_EQ_P (tree->left, NULL);
	TEST_EQ_P (tree->right, NULL);
	TEST_EQ_P (node1->parent, NULL);
	TEST_EQ_P (node2->parent, NULL);


	nih_free (tree);
}


static int destructor_called = 0;

static int
my_destructor (void *ptr)
{
	destructor_called++;

	return 0;
}

void
test_free (void)
{
	NihTree *tree, *node1, *node2;
	int      ret;

	/* Check that destructors are called on nih_tree_free and the return
	 * value of that destructor is returned; the node should be unlinked
	 * from the tree it was in, casting children adrift.
	 */
	TEST_FUNCTION ("nih_list_free");
	tree = nih_tree_new (NULL);
	node1 = nih_tree_new (tree);
	node2 = nih_tree_new (tree);

	nih_tree_add (tree, node1, NIH_TREE_LEFT);
	nih_tree_add (node1, node2, NIH_TREE_RIGHT);

	destructor_called = 0;
	nih_alloc_set_destructor (node1, my_destructor);

	ret = nih_tree_free (node1);

	TEST_EQ (ret, 0);
	TEST_TRUE (destructor_called);

	TEST_EQ_P (tree->left, NULL);
	TEST_EQ_P (node2->parent, NULL);

	nih_free (tree);
}


void
test_next (void)
{
	NihTree *node[13], *ptr;
	int      i;

	/* Check that we can iterate a reasonably complex tree in-order,
	 * and that nih_tree_next returns the right pointer in each case
	 * until it finally returns NULL.
	 */
	TEST_FUNCTION ("nih_tree_next");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node[4], node[3], NIH_TREE_LEFT);
	nih_tree_add (node[4], node[8], NIH_TREE_RIGHT);
	nih_tree_add (node[3], node[2], NIH_TREE_LEFT);
	nih_tree_add (node[8], node[5], NIH_TREE_LEFT);
	nih_tree_add (node[8], node[10], NIH_TREE_RIGHT);
	nih_tree_add (node[2], node[1], NIH_TREE_LEFT);
	nih_tree_add (node[5], node[7], NIH_TREE_RIGHT);
	nih_tree_add (node[10], node[11], NIH_TREE_RIGHT);
	nih_tree_add (node[1], node[0], NIH_TREE_LEFT);
	nih_tree_add (node[7], node[6], NIH_TREE_LEFT);
	nih_tree_add (node[10], node[9], NIH_TREE_LEFT);

	node[12] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 12)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     13, i + 1);

		ptr = nih_tree_next (node[0], ptr);

		if (ptr != node[i])
			TEST_FAILED ("wrong tree node, expected %p got %p",
				     node[i], ptr);

		i++;
	} while (ptr);

	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}

void
test_foreach (void)
{
	NihTree *node[12];
	int      i;

	/* Check that we can iterate a reasonably complex tree in-order,
	 * and that NIH_TREE_FOREACH sets the iterator to the right nodes
	 * in turn.
	 */
	TEST_FUNCTION ("NIH_TREE_FOREACH");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node[4], node[3], NIH_TREE_LEFT);
	nih_tree_add (node[4], node[8], NIH_TREE_RIGHT);
	nih_tree_add (node[3], node[2], NIH_TREE_LEFT);
	nih_tree_add (node[8], node[5], NIH_TREE_LEFT);
	nih_tree_add (node[8], node[10], NIH_TREE_RIGHT);
	nih_tree_add (node[2], node[1], NIH_TREE_LEFT);
	nih_tree_add (node[5], node[7], NIH_TREE_RIGHT);
	nih_tree_add (node[10], node[11], NIH_TREE_RIGHT);
	nih_tree_add (node[1], node[0], NIH_TREE_LEFT);
	nih_tree_add (node[7], node[6], NIH_TREE_LEFT);
	nih_tree_add (node[10], node[9], NIH_TREE_LEFT);

	i = 0;
	NIH_TREE_FOREACH (node[0], iter) {
		if (i > 11)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     12, i + 1);

		if (iter != node[i])
			TEST_FAILED ("wrong tree node, expected %p got %p",
				     node[i], iter);

		i++;
	}

	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}

void
test_prev (void)
{
	NihTree *node[13], *ptr;
	int      i;

	/* Check that we can reverse iterate a reasonably complex tree
	 * in-order, and that nih_tree_prev returns the right pointer in
	 * each case until it finally returns NULL.
	 */
	TEST_FUNCTION ("nih_tree_prev");
	for (i = 0; i < 12; i++)
		node[i] = nih_tree_new (NULL);

	nih_tree_add (node[7], node[8], NIH_TREE_LEFT);
	nih_tree_add (node[7], node[3], NIH_TREE_RIGHT);
	nih_tree_add (node[8], node[9], NIH_TREE_LEFT);
	nih_tree_add (node[3], node[6], NIH_TREE_LEFT);
	nih_tree_add (node[3], node[1], NIH_TREE_RIGHT);
	nih_tree_add (node[9], node[10], NIH_TREE_LEFT);
	nih_tree_add (node[6], node[4], NIH_TREE_RIGHT);
	nih_tree_add (node[1], node[0], NIH_TREE_RIGHT);
	nih_tree_add (node[10], node[11], NIH_TREE_LEFT);
	nih_tree_add (node[4], node[5], NIH_TREE_LEFT);
	nih_tree_add (node[1], node[2], NIH_TREE_LEFT);

	node[12] = NULL;

	i = 0;
	ptr = NULL;

	do {
		if (i > 12)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     13, i + 1);

		ptr = nih_tree_prev (node[0], ptr);

		if (ptr != node[i])
			TEST_FAILED ("wrong tree node, expected %p got %p",
				     node[i], ptr);

		i++;
	} while (ptr);

	for (i = 0; i < 12; i++)
		nih_free (node[i]);
}


int
main (int   argc,
      char *argv[])
{
	test_init ();
	test_new ();
	test_add ();
	test_remove ();
	test_unlink ();
	test_destructor ();
	test_free ();
	test_next ();
	test_foreach ();
	test_prev ();

	return 0;
}
