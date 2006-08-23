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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <nih/alloc.h>


int
test_alloc (void)
{
	void *ptr1, *ptr2;
	int   ret = 0;

	printf ("Testing nih_alloc()\n");

	printf ("...with no parent\n");
	ptr1 = nih_alloc (NULL, 8096);
	memset (ptr1, 'x', 8096);

	/* Size should be correct */
	if (nih_alloc_size (ptr1) != 8096) {
		printf ("BAD: size of first block incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (ptr1) != NULL) {
		printf ("BAD: parent of first block incorrect.\n");
		ret = 1;
	}


	printf ("...with a parent\n");
	ptr2 = nih_alloc (ptr1, 10);
	memset (ptr2, 'x', 10);

	/* Size should be correct */
	if (nih_alloc_size (ptr2) != 10) {
		printf ("BAD: size of first block incorrect.\n");
		ret = 1;
	}

	/* Parent should be ptr1 */
	if (nih_alloc_parent (ptr2) != ptr1) {
		printf ("BAD: parent of second block incorrect.\n");
		ret = 1;
	}

	nih_free (ptr1);

	return ret;
}

static int last_size = 0;

static void *
my_realloc (void *ptr, size_t size)
{
	last_size = size;
	return realloc (ptr, size);
}

int
test_alloc_using (void)
{
	void *ptr;
	int   ret = 0;

	printf ("Testing nih_alloc_using()\n");
	ptr = nih_alloc_using (my_realloc, NULL, 8096);
	memset (ptr, 'x', 8096);

	/* Realloc function should have been passed the size */
	if (last_size < 8096) {
		printf ("BAD: realloc not called with correct size.\n");
		ret = 1;
	}

	nih_free (ptr);

	return ret;
}

int
test_realloc (void)
{
	void *ptr1, *ptr2;
	int   ret = 0;

	printf ("Testing nih_realloc()\n");

	printf ("...with no parent\n");
	ptr1 = nih_alloc (NULL, 4096);
	memset (ptr1, 'x', 4096);

	ptr1 = nih_realloc (ptr1, NULL, 8096);
	memset (ptr1, 'x', 8096);

	/* Size should be correct */
	if (nih_alloc_size (ptr1) != 8096) {
		printf ("BAD: size of first block incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (ptr1) != NULL) {
		printf ("BAD: parent of first block incorrect.\n");
		ret = 1;
	}


	printf ("...with a parent\n");
	ptr2 = nih_alloc (ptr1, 5);
	memset (ptr2, 'x', 5);

	ptr2 = nih_realloc (ptr2, ptr1, 10);
	memset (ptr2, 'x', 10);

	/* Size should be correct */
	if (nih_alloc_size (ptr2) != 10) {
		printf ("BAD: size of first block incorrect.\n");
		ret = 1;
	}

	/* Parent should be ptr1 */
	if (nih_alloc_parent (ptr2) != ptr1) {
		printf ("BAD: parent of second block incorrect.\n");
		ret = 1;
	}

	nih_free (ptr1);

	return ret;
}

int
test_new (void)
{
	void *ptr1, *ptr2;
	int   ret = 0;

	printf ("Testing nih_new()\n");

	printf ("...with no parent\n");
	ptr1 = nih_new (NULL, int);

	/* Size should be size of passed type */
	if (nih_alloc_size (ptr1) != sizeof (int)) {
		printf ("BAD: size of first block incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (ptr1) != NULL) {
		printf ("BAD: parent of first block incorrect.\n");
		ret = 1;
	}


	printf ("...with parent\n");
	ptr2 = nih_new (ptr1, char);

	/* Size should be size of passed type */
	if (nih_alloc_size (ptr2) != sizeof (char)) {
		printf ("BAD: size of second block incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (ptr2) != ptr1) {
		printf ("BAD: parent of second block incorrect.\n");
		ret = 1;
	}

	nih_free (ptr1);

	return ret;
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

int
test_free (void)
{
	void *ptr1, *ptr2;
	int   ret = 0, free_ret;

	printf ("Testing nih_free()\n");

	printf ("...with no parent\n");
	ptr1 = nih_alloc_using (my_realloc, NULL, 10);
	nih_alloc_set_destructor (ptr1, destructor_called);
	last_size = -1;
	was_called = 0;
	free_ret = nih_free (ptr1);

	/* Allocator function should have been called with zero size */
	if (last_size != 0) {
		printf ("BAD: allocator was not called.\n");
		ret = 1;
	}

	/* Destructor should have been called */
	if (! was_called) {
		printf ("BAD: destructor was not called.\n");
		ret = 1;
	}

	/* nih_free should return destructor return value */
	if (free_ret != 2) {
		printf ("BAD: return value of nih_free() not correct.\n");
		ret = 1;
	}


	printf ("...with destructor on child\n");
	ptr1 = nih_alloc (NULL, 10);
	ptr2 = nih_alloc_using (my_realloc, ptr1, 10);
	nih_alloc_set_destructor (ptr2, destructor_called);
	last_size = -1;
	was_called = 0;
	free_ret = nih_free (ptr1);

	/* Allocator of child should have been called with zero size */
	if (last_size != 0) {
		printf ("BAD: child allocator was not called.\n");
		ret = 1;
	}

	/* Destructor of child should have been called */
	if (! was_called) {
		printf ("BAD: child destructor was not called.\n");
		ret = 1;
	}

	/* nih_free should return destructor return value of child */
	if (free_ret != 2) {
		printf ("BAD: return value of nih_free() not correct.\n");
		ret = 1;
	}


	printf ("...with child and destructors\n");
	ptr1 = nih_alloc (NULL, 10); 
	ptr2 = nih_alloc (ptr1, 10);
	nih_alloc_set_destructor (ptr1, destructor_called);
	nih_alloc_set_destructor (ptr2, child_destructor_called);
	was_called = 0;
	free_ret = nih_free (ptr1);

	/* Both destructors should have been called */
	if (was_called != 2) {
		printf ("BAD: one or more destructors was not called.\n");
		ret = 1;
	}

	/* nih_free should return destructor return value of parent */
	if (free_ret != 2) {
		printf ("BAD: return value of nih_free() not correct.\n");
		ret = 1;
	}

	return ret;
}

int
test_set_allocator (void)
{
	void *ptr;
	int   ret = 0;

	printf ("Testing nih_alloc_set_allocator()\n");
	nih_alloc_set_allocator (my_realloc);

	last_size = 0;
	ptr = nih_alloc (NULL, 10);

	/* Allocator should have been called */
	if (last_size < 10) {
		printf ("BAD: allocator was not called.\n");
		ret = 1;
	}

	nih_free (ptr);

	nih_alloc_set_allocator (realloc);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_alloc ();
	ret |= test_alloc_using ();
	ret |= test_realloc ();
	ret |= test_new ();
	ret |= test_free ();
	ret |= test_set_allocator ();

	return ret;
}
