/* libnih
 *
 * test_string.c - test suite for nih/string.c
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

#include <sys/types.h>
#include <sys/stat.h>

#include <pty.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>


void
test_sprintf (void)
{
	char *str1, *str2;

	TEST_FUNCTION ("nih_sprintf");

	/* Check that we can create a formatted string with no parent,
	 * it should be allocated with nih_alloc and be the right length.
	 */
	TEST_FEATURE ("with no parent");
	TEST_ALLOC_FAIL {
		str1 = nih_sprintf (NULL, "this %s a test %d", "is", 54321);

		if (test_alloc_failed) {
			TEST_EQ_P (str1, NULL);
			continue;
		}

		TEST_ALLOC_PARENT (str1, NULL);
		TEST_ALLOC_SIZE (str1, strlen (str1) + 1);
		TEST_EQ_STR (str1, "this is a test 54321");

		nih_free (str1);
	}


	/* Check that we can create a string with a parent. */
	TEST_FEATURE ("with a parent");
	str1 = nih_sprintf (NULL, "this %s a test %d", "is", 54321);

	TEST_ALLOC_FAIL {
		str2 = nih_sprintf (str1, "another %d test %s",
				    12345, "string");

		if (test_alloc_failed) {
			TEST_EQ_P (str2, NULL);
			continue;
		}

		TEST_ALLOC_PARENT (str2, str1);
		TEST_ALLOC_SIZE (str2, strlen (str2) + 1);
		TEST_EQ_STR (str2, "another 12345 test string");

		nih_free (str2);
	}

	nih_free (str1);
}


static char *
my_vsprintf (void *parent,
	     const char *format,
	     ...)
{
	char    *str;
	va_list  args;

	va_start (args, format);
	str = nih_vsprintf (parent, format, args);
	va_end (args);

	return str;
}

void
test_vsprintf (void)
{
	char *str1, *str2;

	TEST_FUNCTION ("nih_vsprintf");

	/* Check that we can create a formatted string for a va_list,
	 * first with no parent.
	 */
	TEST_FEATURE ("with no parent");
	TEST_ALLOC_FAIL {
		str1 = my_vsprintf (NULL, "this %s a test %d", "is", 54321);

		if (test_alloc_failed) {
			TEST_EQ_P (str1, NULL);
			continue;
		}

		TEST_ALLOC_PARENT (str1, NULL);
		TEST_ALLOC_SIZE (str1, strlen (str1) + 1);
		TEST_EQ_STR (str1, "this is a test 54321");

		nih_free (str1);
	}


	/* And then with a parent. */
	TEST_FEATURE ("with a parent");
	str1 = my_vsprintf (NULL, "this %s a test %d", "is", 54321);

	TEST_ALLOC_FAIL {
		str2 = my_vsprintf (str1, "another %d test %s",
				    12345, "string");

		if (test_alloc_failed) {
			TEST_EQ_P (str2, NULL);
			continue;
		}

		TEST_ALLOC_PARENT (str2, str1);
		TEST_ALLOC_SIZE (str2, strlen (str2) + 1);
		TEST_EQ_STR (str2, "another 12345 test string");

		nih_free (str2);
	}

	nih_free (str1);
}

void
test_strdup (void)
{
	char *str1, *str2;

	TEST_FUNCTION ("nih_strdup");

	/* Check that we can create a duplicate of another string,
	 * allocated with nih_alloc and no parent.
	 */
	TEST_FEATURE ("with no parent");
	TEST_ALLOC_FAIL {
		str1 = nih_strdup (NULL, "this is a test");

		if (test_alloc_failed) {
			TEST_EQ_P (str1, NULL);
			continue;
		}

		TEST_ALLOC_PARENT (str1, NULL);
		TEST_ALLOC_SIZE (str1, strlen (str1) + 1);
		TEST_EQ_STR (str1, "this is a test");

		nih_free (str1);
	}


	/* And check we can allocate with a parent. */
	TEST_FEATURE ("with a parent");
	str1 = nih_strdup (NULL, "this is a test");

	TEST_ALLOC_FAIL {
		str2 = nih_strdup (str1, "another test string");

		if (test_alloc_failed) {
			TEST_EQ_P (str2, NULL);
			continue;
		}

		TEST_ALLOC_PARENT (str2, str1);
		TEST_ALLOC_SIZE (str2, strlen (str2) + 1);
		TEST_EQ_STR (str2, "another test string");

		nih_free (str2);
	}

	nih_free (str1);
}

void
test_strndup (void)
{
	char *str1, *str2;

	TEST_FUNCTION ("nih_strndup");

	/* Check that we can create a duplicate of the first portion of
	 * another string, allocated with nih_alloc and no parent.  The
	 * new string should still include a NULL byte.
	 */
	TEST_FEATURE ("with no parent");
	TEST_ALLOC_FAIL {
		str1 = nih_strndup (NULL, "this is a test", 7);

		if (test_alloc_failed) {
			TEST_EQ_P (str1, NULL);
			continue;
		}

		TEST_ALLOC_PARENT (str1, NULL);
		TEST_ALLOC_SIZE (str1, 8);
		TEST_EQ_STR (str1, "this is");

		nih_free (str1);
	}


	/* Check that it works with a parent. */
	TEST_FEATURE ("with a parent");
	str1 = nih_strndup (NULL, "this is a test", 7);

	TEST_ALLOC_FAIL {
		str2 = nih_strndup (str1, "another test string", 12);

		if (test_alloc_failed) {
			TEST_EQ_P (str2, NULL);
			continue;
		}

		TEST_ALLOC_PARENT (str2, str1);
		TEST_ALLOC_SIZE (str2, 13);
		TEST_EQ_STR (str2, "another test");

		nih_free (str2);
	}

	nih_free (str1);


	/* Check that the right thing happens if the length we give is
	 * longer than the string, the returned size should be ample but
	 * with the complete string copied in.
	 */
	TEST_FEATURE ("with larger length than string");
	TEST_ALLOC_FAIL {
		str1 = nih_strndup (NULL, "small string", 20);

		if (test_alloc_failed) {
			TEST_EQ_P (str1, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (str1, 21);
		TEST_EQ_STR (str1, "small string");

		nih_free (str1);
	}
}


void
test_strcat (void)
{
	char *str, *ret;

	TEST_FUNCTION ("nih_strcat");

	/* Check that we can extend a string with another, resulting in the
	 * original string being modified and the new pointer stored in the
	 * argument and returned.
	 */
	TEST_FEATURE ("with string");
	TEST_ALLOC_FAIL {
		char *tmp;

		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "this is a test");
		}

		tmp = str;
		ret = nih_strcat (&str, NULL, " of strdup");

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, tmp);
			TEST_EQ_STR (str, "this is a test");

			nih_free (str);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 25);
		TEST_EQ_STR (str, "this is a test of strdup");

		nih_free (str);
	}


	/* Check that when no string is passed, this behaves as strdup.
	 */
	TEST_FEATURE ("with NULL");
	TEST_ALLOC_FAIL {
		str = NULL;
		ret = nih_strcat (&str, NULL, "test of strdup");

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ_STR (str, "test of strdup");

		nih_free (str);
	}
}

void
test_strncat (void)
{
	char *str, *ret;

	TEST_FUNCTION ("nih_strncat");

	/* Check that we can extend a string with the first number of bytes
	 * from another, resulting in the original string being modified and
	 * the new pointer stored in the argument and returned.
	 */
	TEST_FEATURE ("with larger string than length");
	TEST_ALLOC_FAIL {
		char *tmp;

		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "this is a test");
		}

		tmp = str;
		ret = nih_strncat (&str, NULL, " of strndup", 3);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, tmp);
			TEST_EQ_STR (str, "this is a test");

			nih_free (str);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 18);
		TEST_EQ_STR (str, "this is a test of");

		nih_free (str);
	}


	/* Check that if a longer length than the string is given, enough
	 * space is reserved but the string copy stops at the NULL.
	 */
	TEST_FEATURE ("with larger length than string");
	TEST_ALLOC_FAIL {
		char *tmp;

		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "this is a test");
		}

		tmp = str;
		ret = nih_strncat (&str, NULL, " of strndup", 21);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, tmp);
			TEST_EQ_STR (str, "this is a test");

			nih_free (str);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 36);
		TEST_EQ_STR (str, "this is a test of strndup");

		nih_free (str);
	}


	/* Check that when no string is passed, this behaves as strndup.
	 */
	TEST_FEATURE ("with NULL");
	TEST_ALLOC_FAIL {
		str = NULL;
		ret = nih_strncat (&str, NULL, "test of strndup", 12);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 13);
		TEST_EQ_STR (str, "test of strn");

		nih_free (str);
	}
}


void
test_strcat_sprintf (void)
{
	char *str, *ret;

	TEST_FUNCTION ("test_strcat_sprintf");

	/* Check that we can extend a string with a formatted string,
	 * resulting in the original string being modified and the new
	 * pointer stored in the argument and returned.
	 */
	TEST_FEATURE ("with original string");
	TEST_ALLOC_FAIL {
		char *tmp;

		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "this");
		}

		tmp = str;
		ret = nih_strcat_sprintf (&str, NULL,
					  " %s a test %d", "is", 54321);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, tmp);
			TEST_EQ_STR (str, "this");

			nih_free (str);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 21);
		TEST_EQ_STR (str, "this is a test 54321");

		nih_free (str);
	}


	/* Check that when no string is passed, this behaves as sprintf.
	 */
	TEST_FEATURE ("with NULL");
	TEST_ALLOC_FAIL {
		str = NULL;
		ret = nih_strcat_sprintf (&str, NULL,
					  "%s a test %d", "is", 54321);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ_STR (str, "is a test 54321");

		nih_free (str);
	}
}

static char *
my_strcat_vsprintf (char       **str,
		    const void  *parent,
		    const char  *format,
		    ...)
{
	char    *ret;
	va_list  args;

	va_start (args, format);
	ret = nih_strcat_vsprintf (str, parent, format, args);
	va_end (args);

	return ret;
}

void
test_strcat_vsprintf (void)
{
	char *str, *ret;

	TEST_FUNCTION ("test_strcat_vsprintf");

	/* Check that we can extend a string with a formatted string,
	 * resulting in the original string being modified and the new
	 * pointer stored in the argument and returned.
	 */
	TEST_FEATURE ("with original string");
	TEST_ALLOC_FAIL {
		char *tmp;

		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "this");
		}

		tmp = str;
		ret = my_strcat_vsprintf (&str, NULL,
					  " %s a test %d", "is", 54321);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, tmp);
			TEST_EQ_STR (str, "this");

			nih_free (str);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 21);
		TEST_EQ_STR (str, "this is a test 54321");

		nih_free (str);
	}


	/* Check that when no string is passed, this behaves as sprintf.
	 */
	TEST_FEATURE ("with NULL");
	TEST_ALLOC_FAIL {
		str = NULL;
		ret = my_strcat_vsprintf (&str, NULL,
					  "%s a test %d", "is", 54321);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_NE (ret, NULL);
		TEST_EQ_P (ret, str);

		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ_STR (str, "is a test 54321");

		nih_free (str);
	}
}


void
test_str_split (void)
{
	char **array;
	int    i;

	TEST_FUNCTION ("nih_str_split");

	/* Check that we can split a string into a NULL-terminated array
	 * at each matching character.  The array should be allocated with
	 * nih_alloc, and each element should also be with the array as
	 * their parent.
	 */
	TEST_FEATURE ("with no repeat");
	TEST_ALLOC_FAIL {
		array = nih_str_split (NULL, "this is  a\ttest", " \t", FALSE);

		if (test_alloc_failed) {
			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (array, sizeof (char *) * 6);
		for (i = 0; i < 5; i++)
			TEST_ALLOC_PARENT (array[i], array);

		TEST_EQ_STR (array[0], "this");
		TEST_EQ_STR (array[1], "is");
		TEST_EQ_STR (array[2], "");
		TEST_EQ_STR (array[3], "a");
		TEST_EQ_STR (array[4], "test");
		TEST_EQ_P (array[5], NULL);

		nih_free (array);
	}


	/* Check that we can split a string treating multiple consecutive
	 * matching characters as a single separator to be skipped.
	 */
	TEST_FEATURE ("with repeat");
	TEST_ALLOC_FAIL {
		array = nih_str_split (NULL, "this is  a\ttest", " \t", TRUE);

		if (test_alloc_failed) {
			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (array, sizeof (char *) * 5);
		for (i = 0; i < 4; i++)
			TEST_ALLOC_PARENT (array[i], array);

		TEST_EQ_STR (array[0], "this");
		TEST_EQ_STR (array[1], "is");
		TEST_EQ_STR (array[2], "a");
		TEST_EQ_STR (array[3], "test");
		TEST_EQ_P (array[4], NULL);

		nih_free (array);
	}


	/* Check that we can give an empty string, and end up with a
	 * one-element array that only contains a NULL pointer.
	 */
	TEST_FEATURE ("with empty string");
	TEST_ALLOC_FAIL {
		array = nih_str_split (NULL, "", " ", FALSE);

		if (test_alloc_failed) {
			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (array, sizeof (char *));
		TEST_EQ_P (array[0], NULL);

		nih_free (array);
	}
}

void
test_array_new (void)
{
	char **array;

	/* Check that we can allocate a NULL-terminated array of strings using
	 * nih_alloc().
	 */
	TEST_FUNCTION ("nih_str_array_new");
	TEST_ALLOC_FAIL {
		array = nih_str_array_new (NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (array, sizeof (char *));
		TEST_EQ_P (array[0], NULL);

		nih_free (array);
	}
}

void
test_array_add (void)
{
	char   **array, **ret;
	size_t   len;

	/* Check that we can append strings to a NULL-terminated array.
	 */
	TEST_FUNCTION ("nih_str_array_add");
	array = nih_str_array_new (NULL);
	len = 0;

	TEST_ALLOC_FAIL {
		ret = nih_str_array_add (&array, NULL, &len, "test");

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ (len, 1);
			TEST_EQ_STR (array[0], "test");
			TEST_EQ_P (array[1], NULL);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ (len, 1);
		TEST_ALLOC_PARENT (array[0], array);
		TEST_ALLOC_SIZE (array[0], 5);
		TEST_EQ_STR (array[0], "test");
		TEST_EQ_P (array[1], NULL);
	}

	nih_free (array);
}

void
test_array_addn (void)
{
	char   **array, **ret;
	size_t   len;

	/* Check that we can append strings to a NULL-terminated array.
	 */
	TEST_FUNCTION ("nih_str_array_addn");
	array = nih_str_array_new (NULL);
	len = 0;

	TEST_ALLOC_FAIL {
		ret = nih_str_array_addn (&array, NULL, &len, "testing", 4);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ (len, 1);
			TEST_EQ_STR (array[0], "test");
			TEST_EQ_P (array[1], NULL);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ (len, 1);
		TEST_ALLOC_PARENT (array[0], array);
		TEST_ALLOC_SIZE (array[0], 5);
		TEST_EQ_STR (array[0], "test");
		TEST_EQ_P (array[1], NULL);
	}

	nih_free (array);
}

void
test_array_addp (void)
{
	char   **array, **ret;
	char    *ptr1 = NULL;
	char    *ptr2 = NULL;
	size_t   len;

	TEST_FUNCTION ("nih_str_array_addn");


	/* Check that we can call the function with a NULL array pointer,
	 * and get one allocated automatically.
	 */
	TEST_FEATURE ("with no array given");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			ptr1 = nih_alloc (NULL, 1024);
			memset (ptr1, ' ', 1024);
		}

		array = NULL;
		len = 0;

		ret = nih_str_array_addp (&array, NULL, &len, ptr1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_EQ_P (array, NULL);

			TEST_EQ (len, 0);

			nih_free (ptr1);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ (len, 1);
		TEST_EQ_P (array[0], ptr1);
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_P (array[1], NULL);

		nih_free (array);
		nih_free (ptr1);
	}


	/* Check that we can append allocated blocks to a
	 * NULL-terminated array, and that the blocks are automatically
	 * reparented.
	 */
	TEST_FEATURE ("with length given");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			array = nih_str_array_new (NULL);
			len = 0;

			ptr1 = nih_alloc (NULL, 1024);
			memset (ptr1, ' ', 1024);
		}

		ret = nih_str_array_addp (&array, NULL, &len, ptr1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ (len, 0);
			TEST_EQ_P (array[0], NULL);

			nih_free (array);
			nih_free (ptr1);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ (len, 1);
		TEST_EQ_P (array[0], ptr1);
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_P (array[1], NULL);

		nih_free (array);
		nih_free (ptr1);
	}


	/* Check that we can omit the length, and have it calculated. */
	TEST_FEATURE ("with no length given");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			array = nih_str_array_new (NULL);
			len = 0;

			ptr1 = nih_alloc (NULL, 1024);
			memset (ptr1, ' ', 1024);

			assert (nih_str_array_addp (&array, NULL, NULL, ptr1));

			ptr2 = nih_alloc (NULL, 512);
			memset (ptr2, ' ', 512);
		}

		ret = nih_str_array_addp (&array, NULL, NULL, ptr2);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_P (array[0], ptr1);
			TEST_EQ_P (array[1], NULL);

			nih_free (array);
			nih_free (ptr1);
			nih_free (ptr2);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ_P (array[0], ptr1);
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_P (array[1], ptr2);
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_P (array[2], NULL);

		nih_free (array);
		nih_free (ptr1);
		nih_free (ptr2);
	}
}

void
test_array_copy (void)
{
	char   **array, **args;
	size_t   len;

	TEST_FUNCTION ("nih_str_array_copy");
	args = NIH_MUST (nih_str_array_new (NULL));
	NIH_MUST (nih_str_array_add (&args, NULL, NULL, "this"));
	NIH_MUST (nih_str_array_add (&args, NULL, NULL, "is"));
	NIH_MUST (nih_str_array_add (&args, NULL, NULL, "a"));
	NIH_MUST (nih_str_array_add (&args, NULL, NULL, "test"));


	/* Check that we can make a copy of an array, with each element
	 * a copy of the last.
	 */
	TEST_FEATURE ("with length given");
	TEST_ALLOC_FAIL {
		len = 0;
		array = nih_str_array_copy (NULL, &len, args);

		if (test_alloc_failed) {
			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_NE_P (array, NULL);

		TEST_EQ (len, 4);
		TEST_EQ_STR (array[0], "this");
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_STR (array[1], "is");
		TEST_ALLOC_PARENT (array[1], array);
		TEST_EQ_STR (array[2], "a");
		TEST_ALLOC_PARENT (array[2], array);
		TEST_EQ_STR (array[3], "test");
		TEST_ALLOC_PARENT (array[3], array);
		TEST_EQ_P (array[4], NULL);

		nih_free (array);
	}


	/* Check that we can omit the length, and have it calculated. */
	TEST_FEATURE ("with no length given");
	TEST_ALLOC_FAIL {
		array = nih_str_array_copy (NULL, NULL, args);

		if (test_alloc_failed) {
			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_NE_P (array, NULL);

		TEST_EQ_STR (array[0], "this");
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_STR (array[1], "is");
		TEST_ALLOC_PARENT (array[1], array);
		TEST_EQ_STR (array[2], "a");
		TEST_ALLOC_PARENT (array[2], array);
		TEST_EQ_STR (array[3], "test");
		TEST_ALLOC_PARENT (array[3], array);
		TEST_EQ_P (array[4], NULL);

		nih_free (array);
	}

	nih_free (args);


	/* Check that we can make a copy of an array with a single NULL
	 * element and have the same returned.
	 */
	TEST_FEATURE ("with zero-length array");
	args = NIH_MUST (nih_str_array_new (NULL));

	TEST_ALLOC_FAIL {
		len = 0;
		array = nih_str_array_copy (NULL, &len, args);

		if (test_alloc_failed) {
			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_NE_P (array, NULL);

		TEST_EQ (len, 0);
		TEST_EQ_P (array[0], NULL);

		nih_free (array);
	}

	nih_free (args);
}

void
test_array_append (void)
{
	char   **array, **args, **ret;
	size_t   len;

	TEST_FUNCTION ("nih_str_array_append");
	args = NIH_MUST (nih_str_array_new (NULL));
	NIH_MUST (nih_str_array_add (&args, NULL, NULL, "this"));
	NIH_MUST (nih_str_array_add (&args, NULL, NULL, "is"));
	NIH_MUST (nih_str_array_add (&args, NULL, NULL, "a"));
	NIH_MUST (nih_str_array_add (&args, NULL, NULL, "test"));


	/* Check that we can append one array onto the end of the other,
	 * and that the array is extended and each new element copied
	 * into the new array.
	 */
	TEST_FEATURE ("with length given");
	TEST_ALLOC_FAIL {
		len = 0;

		TEST_ALLOC_SAFE {
			array = nih_str_array_new (NULL);
			NIH_MUST (nih_str_array_add (&array, NULL, &len,
						     "foo"));
			NIH_MUST (nih_str_array_add (&array, NULL, &len,
						     "bar"));
		}

		ret = nih_str_array_append (&array, NULL, &len, args);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ (len, 2);
			TEST_EQ_STR (array[0], "foo");
			TEST_EQ_STR (array[1], "bar");
			TEST_EQ_P (array[2], NULL);

			nih_free (array);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ (len, 6);
		TEST_EQ_STR (array[0], "foo");
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_STR (array[1], "bar");
		TEST_ALLOC_PARENT (array[1], array);
		TEST_EQ_STR (array[2], "this");
		TEST_ALLOC_PARENT (array[2], array);
		TEST_EQ_STR (array[3], "is");
		TEST_ALLOC_PARENT (array[3], array);
		TEST_EQ_STR (array[4], "a");
		TEST_ALLOC_PARENT (array[4], array);
		TEST_EQ_STR (array[5], "test");
		TEST_ALLOC_PARENT (array[5], array);
		TEST_EQ_P (array[6], NULL);

		nih_free (array);
	}


	/* Check that we can omit the length, and have it calculated. */
	TEST_FEATURE ("with no length given");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			array = nih_str_array_new (NULL);
			NIH_MUST (nih_str_array_add (&array, NULL, NULL,
						     "foo"));
			NIH_MUST (nih_str_array_add (&array, NULL, NULL,
						     "bar"));
		}

		ret = nih_str_array_append (&array, NULL, NULL, args);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (array[0], "foo");
			TEST_EQ_STR (array[1], "bar");
			TEST_EQ_P (array[2], NULL);

			nih_free (array);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ_STR (array[0], "foo");
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_STR (array[1], "bar");
		TEST_ALLOC_PARENT (array[1], array);
		TEST_EQ_STR (array[2], "this");
		TEST_ALLOC_PARENT (array[2], array);
		TEST_EQ_STR (array[3], "is");
		TEST_ALLOC_PARENT (array[3], array);
		TEST_EQ_STR (array[4], "a");
		TEST_ALLOC_PARENT (array[4], array);
		TEST_EQ_STR (array[5], "test");
		TEST_ALLOC_PARENT (array[5], array);
		TEST_EQ_P (array[6], NULL);

		nih_free (array);
	}


	/* Check that we can pass a NULL array to get a copy of it, with
	 * the returned length containing the new length.
	 */
	TEST_FEATURE ("with NULL array and length");
	TEST_ALLOC_FAIL {
		len = 0;
		array = NULL;
		ret = nih_str_array_append (&array, NULL, &len, args);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ (len, 0);
			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ (len, 4);
		TEST_EQ_STR (array[0], "this");
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_STR (array[1], "is");
		TEST_ALLOC_PARENT (array[1], array);
		TEST_EQ_STR (array[2], "a");
		TEST_ALLOC_PARENT (array[2], array);
		TEST_EQ_STR (array[3], "test");
		TEST_ALLOC_PARENT (array[3], array);
		TEST_EQ_P (array[4], NULL);

		nih_free (array);
	}


	/* Check that we can pass a NULL array to get a copy of it, without
	 * passing the length in.
	 */
	TEST_FEATURE ("with NULL array and no length");
	TEST_ALLOC_FAIL {
		array = NULL;
		ret = nih_str_array_append (&array, NULL, NULL, args);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_P (array, NULL);
			continue;
		}

		TEST_NE_P (ret, NULL);

		TEST_EQ_STR (array[0], "this");
		TEST_ALLOC_PARENT (array[0], array);
		TEST_EQ_STR (array[1], "is");
		TEST_ALLOC_PARENT (array[1], array);
		TEST_EQ_STR (array[2], "a");
		TEST_ALLOC_PARENT (array[2], array);
		TEST_EQ_STR (array[3], "test");
		TEST_ALLOC_PARENT (array[3], array);
		TEST_EQ_P (array[4], NULL);

		nih_free (array);
	}


	nih_free (args);
}


void
test_str_wrap (void)
{
	char *str;

	TEST_FUNCTION ("nih_str_wrap");

	/* Check that a string smaller than the wrap length is returned
	 * unaltered.
	 */
	TEST_FEATURE ("with no wrapping");
	TEST_ALLOC_FAIL {
		str = nih_str_wrap (NULL, "this is a test", 80, 0, 0);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "this is a test");

		nih_free (str);
	}


	/* Check that a string with embedded new lines is returned with
	 * the line breaks preserved.
	 */
	TEST_FEATURE ("with embedded newlines");
	TEST_ALLOC_FAIL {
		str = nih_str_wrap (NULL, "this is\na test", 80, 0, 0);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "this is\na test");

		nih_free (str);
	}


	/* Check that a smaller string is indented if one is given. */
	TEST_FEATURE ("with no wrapping and indent");
	TEST_ALLOC_FAIL {
		str = nih_str_wrap (NULL, "this is a test", 80, 2, 0);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "  this is a test");

		nih_free (str);
	}


	/* Check that a string with embedded newlines gets an indent on
	 * each new line.
	 */
	TEST_FEATURE ("with embedded newlines and indent");
	TEST_ALLOC_FAIL {
		str = nih_str_wrap (NULL, "this is\na test", 80, 4, 2);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "    this is\n  a test");

		nih_free (str);
	}


	/* Check that a long string is split at the wrap point. */
	TEST_FEATURE ("with simple wrapping");
	TEST_ALLOC_FAIL {
		str = nih_str_wrap (NULL, "this is an example of a string "
				    "that will need wrapping to fit the line "
				    "length we set", 20, 0, 0);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("this is an example\n"
				   "of a string that\n"
				   "will need wrapping\n"
				   "to fit the line\n"
				   "length we set"));

		nih_free (str);
	}


	/* Check that a long string is split at the wrap point, and each
	 * new line indented, with the first line given a different indent.
	 */
	TEST_FEATURE ("with wrapping and indents");
	TEST_ALLOC_FAIL {
		str = nih_str_wrap (NULL, "this is an example of a string "
				    "that will need wrapping to fit the line "
				    "length we set", 20, 4, 2);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("    this is an\n"
				   "  example of a\n"
				   "  string that will\n"
				   "  need wrapping to\n"
				   "  fit the line\n"
				   "  length we set"));

		nih_free (str);
	}


	/* Check that a long string that would be split inside a long word
	 * is wrapepd before the word, and then split inside that word if it
	 * is still too long.
	 */
	TEST_FEATURE ("with split inside word");
	TEST_ALLOC_FAIL {
		str = nih_str_wrap (NULL, ("this string is supercalifragilis"
					   "ticexpialidocious even though the "
					   "sound of it is something quite "
					   "atrocious"), 30, 0, 0);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("this string is\n"
				   "supercalifragilisticexpialidoc\n"
				   "ious even though the sound of\n"
				   "it is something quite\n"
				   "atrocious"));

		nih_free (str);
	}


	/* Check that an indent is still applied if the split occurs inside
	 * a word.
	 */
	TEST_FEATURE ("with split inside word and indents");
	TEST_ALLOC_FAIL {
		str = nih_str_wrap (NULL, ("this string is supercalifragilis"
					   "ticexpialidocious even though the "
					   "sound of it is something quite "
					   "atrocious"), 30, 4, 2);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("    this string is\n"
				   "  supercalifragilisticexpialid\n"
				   "  ocious even though the sound\n"
				   "  of it is something quite\n"
				   "  atrocious"));

		nih_free (str);
	}
}

void
test_str_screen_width (void)
{
	struct winsize  winsize;
	int             pty, pts;
	size_t          len = 0;

	TEST_FUNCTION ("nih_str_screen_width");
	unsetenv ("COLUMNS");

	winsize.ws_row = 24;
	winsize.ws_col = 40;
	winsize.ws_xpixel = 0;
	winsize.ws_ypixel = 0;
	openpty (&pty, &pts, NULL, NULL, &winsize);

	/* Check that we can obtain the width of a screen, where one
	 * is available.  It should match the number of columns in the
	 * pty we run this within.
	 */
	TEST_FEATURE ("with screen width");
	TEST_DIVERT_STDOUT_FD (pts) {
		len = nih_str_screen_width ();
	}

	TEST_EQ (len, 40);


	/* Check that the COLUMNS environment variable overrides the width
	 * of the screen that we detect.
	 */
	TEST_FEATURE ("with COLUMNS variable");
	putenv ("COLUMNS=30");
	TEST_DIVERT_STDOUT_FD (pts) {
		len = nih_str_screen_width ();
	}

	TEST_EQ (len, 30);


	/* Check that we ignore a COLUMNS variable that's not an integer */
	TEST_FEATURE ("with illegal COLUMNS variable");
	putenv ("COLUMNS=30pt");
	TEST_DIVERT_STDOUT_FD (pts) {
		len = nih_str_screen_width ();
	}

	TEST_EQ (len, 40);

	unsetenv ("COLUMNS");
	close (pts);
	close (pty);


	/* Check that we fallback to assuming 80 columns if we don't have
	 * any luck with either the tty or COLUMNS variable.
	 */
	TEST_FEATURE ("with fallback to 80 columns");
	pts = open ("/dev/null", O_RDWR | O_NOCTTY);
	TEST_DIVERT_STDOUT_FD (pts) {
		len = nih_str_screen_width ();
	}

	TEST_EQ (len, 80);

	close (pts);
}

void
test_str_screen_wrap (void)
{
	char           *str = NULL;
	struct winsize  winsize;
	int             pty, pts;

	TEST_FUNCTION ("nih_str_screen_wrap");
	unsetenv ("COLUMNS");

	winsize.ws_row = 24;
	winsize.ws_col = 40;
	winsize.ws_xpixel = 0;
	winsize.ws_ypixel = 0;
	openpty (&pty, &pts, NULL, NULL, &winsize);

	/* Check that we correctly wrap text to the width of the screen
	 * when it is available.
	 */
	TEST_FEATURE ("with screen width");
	TEST_ALLOC_FAIL {
		TEST_DIVERT_STDOUT_FD (pts) {
			str = nih_str_screen_wrap (
				NULL, ("this is a string that "
				       "should need wrapping at "
				       "any different screen width "
				       "that we choose to set"),
				0, 0);
		}

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("this is a string that should need\n"
				   "wrapping at any different screen width\n"
				   "that we choose to set"));

		nih_free (str);
	}


	/* Check that we wrap at the number specified in the COLUMNS
	 * variable in preference to the width of the screen.
	 */
	TEST_FEATURE ("with COLUMNS variable");
	putenv ("COLUMNS=30");
	TEST_ALLOC_FAIL {
		TEST_DIVERT_STDOUT_FD (pts) {
			str = nih_str_screen_wrap (
				NULL, ("this is a string that "
				       "should need wrapping at "
				       "any different screen width "
				       "that we choose to set"),
				0, 0);
		}

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("this is a string that should\n"
				   "need wrapping at any\n"
				   "different screen width that\n"
				   "we choose to set"));

		nih_free (str);
	}

	unsetenv ("COLUMNS");
	close (pts);
	close (pty);


	/* Check that we fallback to assuming 80 columns if we don't have
	 * any luck with either the tty or COLUMNS variable.
	 */
	TEST_FEATURE ("with fallback to 80 columns");
	pts = open ("/dev/null", O_RDWR | O_NOCTTY);

	TEST_ALLOC_FAIL {
		TEST_DIVERT_STDOUT_FD (pts) {
			str = nih_str_screen_wrap (
				NULL, ("this is a string that "
				       "should need wrapping at "
				       "any different screen width "
				       "that we choose to set"),
				0, 0);
		}

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("this is a string that should need "
				   "wrapping at any different screen width "
				   "that\n"
				   "we choose to set"));

		nih_free (str);
	}

	close (pts);
}


int
main (int   argc,
      char *argv[])
{
	test_sprintf ();
	test_vsprintf ();
	test_strdup ();
	test_strndup ();
	test_strcat ();
	test_strncat ();
	test_strcat_sprintf ();
	test_strcat_vsprintf ();
	test_str_split ();
	test_array_new ();
	test_array_add ();
	test_array_addn ();
	test_array_addp ();
	test_array_copy ();
	test_array_append ();
	test_str_wrap ();
	test_str_screen_width ();
	test_str_screen_wrap ();

	return 0;
}
