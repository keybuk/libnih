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

#include <nih/alloc.h>


int
test_alloc_named (void)
{
	void *ptr1, *ptr2;
	int   ret = 0;

	printf ("Testing nih_alloc_named()\n");

	printf ("...with no parent\n");
	ptr1 = nih_alloc_named (NULL, 8096, "ptr1");
	memset (ptr1, 'x', 8096);

	/* Name should be correct */
	if (strcmp (nih_alloc_name (ptr1), "ptr1")) {
		printf ("BAD: name of first block incorrect.\n");
		ret = 1;
	}

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
	ptr2 = nih_alloc_named (ptr1, 10, "ptr2");
	memset (ptr2, 'x', 10);

	/* Name should be correct */
	if (strcmp (nih_alloc_name (ptr2), "ptr2")) {
		printf ("BAD: name of second block incorrect.\n");
		ret = 1;
	}

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

	/* Name should be automatically generated */
	if (! strstr (nih_alloc_name (ptr1), "test_alloc.c")) {
		printf ("BAD: name of first block incorrect.\n");
		ret = 1;
	}

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

	/* Name should be automatically generated */
	if (! strstr (nih_alloc_name (ptr2), "test_alloc.c")) {
		printf ("BAD: name of second block incorrect.\n");
		ret = 1;
	}

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

	return ret;
}

int
test_alloc (void)
{
	void *ptr1, *ptr2;
	int   ret = 0;

	printf ("Testing nih_alloc()\n");

	printf ("...with no parent\n");
	ptr1 = nih_alloc (NULL, 8096);

	/* Name should be automatically generated */
	if (! strstr (nih_alloc_name (ptr1), "test_alloc.c")) {
		printf ("BAD: name of first block incorrect.\n");
		ret = 1;
	}

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


	printf ("...with parent\n");
	ptr2 = nih_alloc (ptr1, 10);

	/* Name should be automatically generated */
	if (! strstr (nih_alloc_name (ptr2), "test_alloc.c")) {
		printf ("BAD: name of second block incorrect.\n");
		ret = 1;
	}

	/* Size should be correct */
	if (nih_alloc_size (ptr2) != 10) {
		printf ("BAD: size of second block incorrect.\n");
		ret = 1;
	}

	/* Parent should be none */
	if (nih_alloc_parent (ptr2) != ptr1) {
		printf ("BAD: parent of second block incorrect.\n");
		ret = 1;
	}

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

	printf ("...with single context\n");
	ptr1 = nih_alloc (NULL, 10);
	nih_alloc_set_destructor (ptr1, destructor_called);
	was_called = 0;
	free_ret = nih_free (ptr1);

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


	printf ("...with parent and child\n");
	ptr1 = nih_alloc (NULL, 10);
	ptr2 = nih_alloc (ptr1, 10);
	nih_alloc_set_destructor (ptr2, destructor_called);
	was_called = 0;
	free_ret = nih_free (ptr1);

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


	printf ("...with parent and child both with destructors\n");
	ptr1 = nih_alloc (NULL, 10);
	nih_alloc_set_destructor (ptr1, destructor_called);
	ptr2 = nih_alloc (ptr1, 10);
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
test_set_name (void)
{
	void *ptr;
	int   ret = 0;

	printf ("Testing nih_alloc_set_name()\n");
	ptr = nih_alloc_named (NULL, 10, "small");
	nih_alloc_set_name (ptr, "different name");

	/* Name should now be changed */
	if (strcmp (nih_alloc_name (ptr), "different name")) {
		printf ("BAD: name was not changed.\n");
		ret = 1;
	}

	return ret;
}


int main (int   argc,
	  char *argv[])
{
	int ret = 0;

	ret |= test_alloc_named ();
	ret |= test_new ();
	ret |= test_alloc ();
	ret |= test_free ();
	ret |= test_set_name ();

	return ret;
}
