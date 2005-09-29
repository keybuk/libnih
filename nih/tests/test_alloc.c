/* libnih
 *
 * test_alloc.c - test suite for nih/alloc.c
 *
 * Copyright © 2005 Scott James Remnant <scott@netsplit.com>.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>

#include <nih/alloc.h>


static int was_called;

int
destructor_called (void *ptr)
{
	was_called = 1;

	return 2;
}

int
test_alloc (void)
{
	void *ptr1, *ptr2;
	int   ret = 0, free_ret;

	printf ("Testing nih_alloc_size() with no parent\n");
	ptr1 = nih_alloc_size (NULL, 8096);
	memset (ptr1, 'x', 8096);

	/* Name should still be retrievable */
	if (nih_alloc_name (ptr1)[0] == 'x') {
		printf ("BAD: name of first block incorrect.\n");
		ret = 1;
	}


	printf ("Testing nih_alloc_size() with a parent\n");
	ptr2 = nih_alloc_size (ptr1, 10);
	nih_alloc_set_destructor (ptr2, destructor_called);
	memset (ptr2, 'x', 10);

	/* Name should still be retrievable */
	if (nih_alloc_name (ptr2)[0] == 'x') {
		printf ("BAD: name of second block incorrect.\n");
		ret = 1;
	}


	printf ("Testing nih_free() on parent\n");
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


	printf ("Testing nih_alloc_size() reuses small pointers\n");
	nih_alloc_return_unused (0);
	ptr1 = nih_alloc_size (NULL, 10);
	nih_free (ptr1);

	ptr2 = nih_alloc_size (NULL, 10);

	/* Pointer should be the same as the last one */
	if (ptr2 != ptr1) {
		printf ("BAD: small pointers differed.\n");
		ret = 1;
	}


	printf ("Testing nih_alloc_size() reuses large pointers\n");
	nih_alloc_return_unused (1);
	ptr1 = nih_alloc_size (NULL, 8096);
	nih_free (ptr1);

	ptr2 = nih_alloc_size (NULL, 8096);

	/* Pointer should be the same as the last one */
	if (ptr2 != ptr1) {
		printf ("BAD: large pointers differed.\n");
		ret = 1;
	}


	printf ("Testing nih_alloc_size() picks best-fit large pointer\n");
	nih_alloc_return_unused (1);
	ptr1 = nih_alloc_size (NULL, 8096);
	nih_free (ptr1);

	ptr2 = nih_alloc_size (NULL, 4096);
	nih_free (ptr2);

	ptr1 = nih_alloc_size (NULL, 6000);
	nih_free (ptr1);

	ptr1 = nih_alloc_size (NULL, 3000);

	/* Pointer should be the smaller one */
	if (ptr1 != ptr2) {
		printf ("BAD: pointer wasn't smallest available.\n");
		ret = 1;
	}


	printf ("Testing nih_alloc_size() ignores too-small pointers\n");
	nih_alloc_return_unused (1);
	ptr1 = nih_alloc_size (NULL, 1000);
	nih_free (ptr1);

	ptr2 = nih_alloc_size (NULL, 1001);

	/* Pointer should not be the too-small one */
	if (ptr2 == ptr1) {
		printf ("BAD: pointer was too small.\n");
		ret = 1;
	}


	return ret;
}

int
test_alloc_set_name (void)
{
	void *ptr;
	int   ret = 0;

	printf ("Testing nih_alloc_set_name()\n");
	ptr = nih_alloc_new (NULL, 10, "small");
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

	ret |= test_alloc ();
	ret |= test_alloc_set_name ();

	return ret;
}
