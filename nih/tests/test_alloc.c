/* libnih
 *
 * test_alloc.c - test suite for nih/alloc.c
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
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

#include <stdlib.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>


void
test_alloc (void)
{
	void *ptr1, *ptr2;

	TEST_FUNCTION ("nih_alloc");

	/* Check allocation remembers the size, and is possible without
	 * a parent.
	 */
	TEST_FEATURE ("with no parent");
	ptr1 = nih_alloc (NULL, 8096);
	memset (ptr1, 'x', 8096);

	TEST_ALLOC_SIZE (ptr1, 8096);
	TEST_ALLOC_PARENT (ptr1, NULL);


	/* Check that allocation with a parent remembers the parent */
	TEST_FEATURE ("with a parent");
	ptr2 = nih_alloc (ptr1, 10);
	memset (ptr2, 'x', 10);

	TEST_ALLOC_SIZE (ptr2, 10);
	TEST_ALLOC_PARENT (ptr2, ptr1);


	nih_free (ptr1);
}


static int last_size = 0;

static void *
my_realloc (void *ptr, size_t size)
{
	last_size = size;
	return realloc (ptr, size);
}

static void *
null_realloc (void *ptr, size_t size)
{
	if (size > 100) {
		return NULL;
	} else {
		return realloc (ptr, size);
	}
}

void
test_alloc_using (void)
{
	void *ptr;

	TEST_FUNCTION ("nih_alloc_using");

	/* Check that the allocator function is passed the size, and that
	 * it's at least the size we asked for.
	 */
	TEST_FEATURE ("with realloc");
	ptr = nih_alloc_using (my_realloc, NULL, 8096);
	memset (ptr, 'x', 8096);

	TEST_GE (last_size, 8096);

	nih_free (ptr);


	/* Check that we get NULL if the allocator fails */
	TEST_FEATURE ("with failing realloc");
	ptr = nih_alloc_using (null_realloc, NULL, 8096);

	TEST_EQ_P (ptr, NULL);
}

void
test_realloc (void)
{
	void *ptr1, *ptr2, *ptr3;

	TEST_FUNCTION ("nih_realloc");

	/* Check that nih_realloc behaves like nih_alloc if the pointer is
	 * NULL (it should, in fact, just call it)
	 */
	TEST_FEATURE ("as nih_alloc");
	ptr1 = nih_realloc (NULL, NULL, 4096);
	memset (ptr1, 'x', 4096);

	TEST_ALLOC_SIZE (ptr1, 4096);
	TEST_ALLOC_PARENT (ptr1, NULL);

	nih_free (ptr1);


	/* Check that nih_realloc works if the block doesn't have a parent,
	 * the size should change and the parent should remain NULL.
	 */
	TEST_FEATURE ("with no parent");
	ptr1 = nih_alloc (NULL, 4096);
	memset (ptr1, 'x', 4096);

	ptr1 = nih_realloc (ptr1, NULL, 8096);
	memset (ptr1, 'x', 8096);

	TEST_ALLOC_SIZE (ptr1, 8096);
	TEST_ALLOC_PARENT (ptr1, NULL);


	/* Check that nih_realloc works if the block has a parent, the size
	 * should change but the parent should remain the same.
	 */
	TEST_FEATURE ("with a parent");
	ptr2 = nih_alloc (ptr1, 5);
	memset (ptr2, 'x', 5);

	ptr2 = nih_realloc (ptr2, ptr1, 10);
	memset (ptr2, 'x', 10);

	TEST_ALLOC_SIZE (ptr2, 10);
	TEST_ALLOC_PARENT (ptr2, ptr1);

	nih_free (ptr1);


	/* Check that nih_realloc works if the block being reallocated has
	 * children.  This is fiddly as they need their parent pointers
	 * adjusted.
	 */
	TEST_FEATURE ("with existant children");
	ptr1 = nih_alloc (NULL, 128);
	memset (ptr1, 'x', 128);

	ptr2 = nih_alloc (ptr1, 512);
	memset (ptr2, 'x', 512);

	ptr3 = nih_realloc (ptr1, NULL, 1024);
	memset (ptr3, 'x', 1024);

	TEST_ALLOC_PARENT (ptr2, ptr3);

	nih_free (ptr3);


	/* Check that nih_realloc returns NULL and doesn't alter the block
	 * if the allocator fails.
	 */
	TEST_FEATURE ("with failing realloc");
	ptr1 = nih_alloc_using (null_realloc, NULL, 10);
	memset (ptr1, 'x', 10);

	ptr2 = nih_realloc (ptr1, NULL, 200);

	TEST_EQ_P (ptr2, NULL);
	TEST_ALLOC_SIZE (ptr1, 10);

	nih_free (ptr1);
}

void
test_new (void)
{
	void *ptr1, *ptr2;

	TEST_FUNCTION ("nih_new");

	/* Check that nih_new works if we don't give it a parent, the block
	 * should be allocated with the size of the type given.
	 */
	TEST_FEATURE ("with no parent");
	ptr1 = nih_new (NULL, int);

	TEST_ALLOC_SIZE (ptr1, sizeof (int));
	TEST_ALLOC_PARENT (ptr1, NULL);


	/* Check that nih_new works if we do give a parent. */
	TEST_FEATURE ("with parent");
	ptr2 = nih_new (ptr1, char);

	TEST_ALLOC_SIZE (ptr2, sizeof (char));
	TEST_ALLOC_PARENT (ptr2, ptr1);

	nih_free (ptr1);
}


static int was_called;

static int
destructor_called (void *ptr)
{
	was_called++;

	return 2;
}

static int
child_destructor_called (void *ptr)
{
	was_called++;

	return 20;
}

void
test_free (void)
{
	void *ptr1, *ptr2;
	int   ret;

	TEST_FUNCTION ("nih_free");

	/* Check that nih_free works if the block has no parent.  Allocator
	 * should get called with zero as the size argument, the destructor
	 * should get called and nih_free should return that return value.
	 */
	TEST_FEATURE ("with no parent");
	ptr1 = nih_alloc_using (my_realloc, NULL, 10);
	nih_alloc_set_destructor (ptr1, destructor_called);
	last_size = -1;
	was_called = 0;
	ret = nih_free (ptr1);

	TEST_EQ (last_size, 0);
	TEST_TRUE (was_called);
	TEST_EQ (ret, 2);


	/* Check that the destructor on any children also gets called, which
	 * is as good a indication as any that the children are being freed.
	 */
	TEST_FEATURE ("with destructor on child");
	ptr1 = nih_alloc (NULL, 10);
	ptr2 = nih_alloc_using (my_realloc, ptr1, 10);
	nih_alloc_set_destructor (ptr2, child_destructor_called);
	last_size = -1;
	was_called = 0;
	ret = nih_free (ptr1);

	TEST_EQ (last_size, 0);
	TEST_TRUE (was_called);
	TEST_EQ (ret, 20);


	/* Check that both destructors on parent and children are called,
	 * and that the return value from nih_free is that of the parent's.
	 */
	TEST_FEATURE ("with child and destructors");
	ptr1 = nih_alloc (NULL, 10);
	ptr2 = nih_alloc (ptr1, 10);
	nih_alloc_set_destructor (ptr1, destructor_called);
	nih_alloc_set_destructor (ptr2, child_destructor_called);
	was_called = 0;
	ret = nih_free (ptr1);

	TEST_EQ (was_called, 2);
	TEST_EQ (ret, 2);
}

void
test_reparent (void)
{
	void *ptr1, *ptr2, *ptr3;

	TEST_FUNCTION ("nih_alloc_reparent");

	/* Check that a no-op works, a block without a parent being orphaned.
	 * Parent should remain NULL after, and this should be silent.
	 */
	TEST_FEATURE ("with orphan and no parent");
	ptr1 = nih_alloc (NULL, 100);
	memset (ptr1, 'x', 100);

	nih_alloc_reparent (ptr1, NULL);

	TEST_ALLOC_PARENT (ptr1, NULL);


	/* Check that we can assign a parent to an orphan. */
	TEST_FEATURE ("with orphan and new parent");
	ptr2 = nih_alloc (NULL, 50);
	memset (ptr2, 'x', 50);

	nih_alloc_reparent (ptr2, ptr1);

	TEST_ALLOC_PARENT (ptr2, ptr1);

	/* Free the block's new parent, this should also free the block */
	was_called = 0;
	nih_alloc_set_destructor (ptr2, destructor_called);
	nih_free (ptr1);

	TEST_TRUE (was_called);


	/* Check that we can orphan a block with a parent. */
	TEST_FEATURE ("with child and no parent");
	ptr1 = nih_alloc (NULL, 100);
	memset (ptr1, 'x', 100);

	ptr2 = nih_alloc (ptr1, 50);
	memset (ptr2, 'x', 50);

	nih_alloc_reparent (ptr2, NULL);

	TEST_ALLOC_PARENT (ptr2, NULL);

	/* Freeing the original parent should not free the block. */
	was_called = 0;
	nih_alloc_set_destructor (ptr2, destructor_called);
	nih_free (ptr1);

	TEST_FALSE (was_called);

	nih_free (ptr2);


	/* Check that we can reparent a block that already had a parent. */
	TEST_FEATURE ("with child and new parent");
	ptr1 = nih_alloc (NULL, 100);
	memset (ptr1, 'x', 100);

	ptr2 = nih_alloc (ptr1, 50);
	memset (ptr2, 'x', 50);

	ptr3 = nih_alloc (NULL, 75);
	memset (ptr3, 'x', 75);

	nih_alloc_reparent (ptr2, ptr3);

	TEST_ALLOC_PARENT (ptr2, ptr3);

	/* Freeing the original parent should not free the block. */
	was_called = 0;
	nih_alloc_set_destructor (ptr2, destructor_called);
	nih_free (ptr1);

	TEST_FALSE (was_called);

	/* Freeing the new parent should free the block. */
	was_called = 0;
	nih_alloc_set_destructor (ptr2, destructor_called);
	nih_free (ptr3);

	TEST_TRUE (was_called);
}

void
test_set_allocator (void)
{
	void *ptr;

	/* Check that the function alters the default allocator by allocating
	 * and freeing a block with it afterwards.
	 */
	TEST_FUNCTION ("nih_alloc_set_allocator");

	nih_alloc_set_allocator (my_realloc);

	last_size = 0;
	ptr = nih_alloc (NULL, 10);

	TEST_GE (last_size, 10);

	last_size = 0;
	nih_free (ptr);

	TEST_EQ (last_size, 0);

	nih_alloc_set_allocator (realloc);
}


int
main (int   argc,
      char *argv[])
{
	test_alloc ();
	test_alloc_using ();
	test_realloc ();
	test_new ();
	test_free ();
	test_reparent ();
	test_set_allocator ();

	return 0;
}
