/* nih-dbus-tool
 *
 * test_indent.c - test suite for nih-dbus-tool/indent.c
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

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>

#include "indent.h"


void
test_indent (void)
{
	char *str;
	char *ret;

	TEST_FUNCTION ("indent");

	/* Check that we can indent a typical multi-line string, with each
	 * line being indented including the first and last, but that an
	 * indent isn't appended after the trailing newline.
	 */
	TEST_FEATURE ("with multi-line string");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n"
						 "of a multi-line\n"
						 "string.\n"));
		}

		ret = indent (&str, NULL, 1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, ("\tThis is a test\n"
				   "\tof a multi-line\n"
				   "\tstring.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that we can apply multiple levels of indent to a string
	 * at once, to save calling the function multiple times.
	 */
	TEST_FEATURE ("with multiple indent levels");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n"
						 "of a multi-line\n"
						 "string.\n"));
		}

		ret = indent (&str, NULL, 3);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, ("\t\t\tThis is a test\n"
				   "\t\t\tof a multi-line\n"
				   "\t\t\tstring.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that a missing final newline doesn't stop the last line
	 * being indented.
	 */
	TEST_FEATURE ("with missing final newline");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n"
						 "of a multi-line\n"
						 "string."));
		}

		ret = indent (&str, NULL, 1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, ("\tThis is a test\n"
				   "\tof a multi-line\n"
				   "\tstring."));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that a blank line does not have an indent added.
	 */
	TEST_FEATURE ("with blank line");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n\n"
						 "of a multi-line\n"
						 "string.\n"));
		}

		ret = indent (&str, NULL, 1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, ("\tThis is a test\n\n"
				   "\tof a multi-line\n"
				   "\tstring.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that consecutive blank lines are handled just as well
	 * as a single one, and that none of the lines get an indent
	 * added.
	 */
	TEST_FEATURE ("with consecutive blank lines");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n\n\n"
						 "of a multi-line\n\n"
						 "string.\n"));
		}

		ret = indent (&str, NULL, 1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, ("\tThis is a test\n\n\n"
				   "\tof a multi-line\n\n"
				   "\tstring.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that the initial line may be blank, in which case it
	 * too does not get an indent added.
	 */
	TEST_FEATURE ("with initial blank line");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("\n"
						 "This is a test\n"
						 "of a multi-line\n"
						 "string.\n"));
		}

		ret = indent (&str, NULL, 1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, ("\n"
				   "\tThis is a test\n"
				   "\tof a multi-line\n"
				   "\tstring.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that a single-line string is indented with no
	 * indent after the trailing new line.
	 */
	TEST_FEATURE ("with single-line string");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "This is a test\n");
		}

		ret = indent (&str, NULL, 1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, "\tThis is a test\n");
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that a simple string is indented. */
	TEST_FEATURE ("with simple string");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "This is a test");
		}

		ret = indent (&str, NULL, 1);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, "\tThis is a test");
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}
}


void
test_comment (void)
{
	char *str;
	char *ret;

	TEST_FUNCTION ("comment");

	/* Check that a multi-line string can be commented out, with each
	 * line including the first and last being prefixed with comment
	 * characters; but that a comment character doesn't appear after
	 * the trailing newline.
	 */
	TEST_FEATURE ("with multi-line string");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n"
						 "of a multi-line\n"
						 "string.\n"));
		}

		ret = comment (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, (" * This is a test\n"
				   " * of a multi-line\n"
				   " * string.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that a missing final newline doesn't stop the last line
	 * being commented.
	 */
	TEST_FEATURE ("with missing final newline");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n"
						 "of a multi-line\n"
						 "string."));
		}

		ret = comment (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, (" * This is a test\n"
				   " * of a multi-line\n"
				   " * string."));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that a blank line still has comment markers added,
	 * but that there is no trailing space before the newline.
	 */
	TEST_FEATURE ("with blank line");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n\n"
						 "of a multi-line\n"
						 "string.\n"));
		}

		ret = comment (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, (" * This is a test\n"
				   " *\n"
				   " * of a multi-line\n"
				   " * string.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that consecutive blank lines are handled just as well
	 * as a single one, and that all of the lines get comment markers
	 * added with no trailing spaces.
	 */
	TEST_FEATURE ("with consecutive blank lines");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("This is a test\n\n\n"
						 "of a multi-line\n\n"
						 "string.\n"));
		}

		ret = comment (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, (" * This is a test\n"
				   " *\n"
				   " *\n"
				   " * of a multi-line\n"
				   " *\n"
				   " * string.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that the initial line may be blank, in which case it
	 * too still gets a comment marker added with no trailing space.
	 */
	TEST_FEATURE ("with initial blank line");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, ("\n"
						 "This is a test\n"
						 "of a multi-line\n"
						 "string.\n"));
		}

		ret = comment (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, (" *\n"
				   " * This is a test\n"
				   " * of a multi-line\n"
				   " * string.\n"));
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that a single-line string is commented with no
	 * marker after the trailing new line.
	 */
	TEST_FEATURE ("with single-line string");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "This is a test\n");
		}

		ret = comment (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, " * This is a test\n");
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}


	/* Check that a simple string is commented out. */
	TEST_FEATURE ("with simple string");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "This is a test");
		}

		ret = comment (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);
			TEST_NE_P (str, NULL);

			nih_free (str);
			continue;
		}

		TEST_EQ_STR (ret, " * This is a test");
		TEST_ALLOC_SIZE (ret, strlen (ret) + 1);

		nih_free (ret);
	}
}


int
main (int   argc,
      char *argv[])
{
	test_indent ();
	test_comment ();

	return 0;
}
