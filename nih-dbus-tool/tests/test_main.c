/* nih-dbus-tool
 *
 * test_main.c - test suite for nih-dbus-tool/main.c
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

#include <stdio.h>

#include <nih/macros.h>
#include <nih/main.h>
#include <nih/option.h>


extern int mode_option (NihOption *option, const char *arg);

void
test_mode_option (void)
{
	NihOption opt;
	int       value;
	FILE *    output;
	int       ret = 0;

	TEST_FUNCTION ("mode_option");
	opt.value = &value;

	output = tmpfile ();


	/* Check that the mode_option function takes the argument as a
	 * string and sets the value to TRUE if it is "object".
	 */
	TEST_FEATURE ("with object");
	TEST_ALLOC_FAIL {
		value = -1;

		ret = mode_option (&opt, "object");

		TEST_EQ (ret, 0);
		TEST_EQ (value, TRUE);
	}


	/* Check that the mode_option function takes the argument as a
	 * string and sets the value to FALSE if it is "proxy".
	 */
	TEST_FEATURE ("with proxy");
	TEST_ALLOC_FAIL {
		value = -1;

		ret = mode_option (&opt, "proxy");

		TEST_EQ (ret, 0);
		TEST_EQ (value, FALSE);
	}


	/* Check that when the argument is an unknown string, an error
	 * message is output to standard error along with a suggestion of
	 * how to get help.
	 */
	TEST_FEATURE ("with unknown argument");
	TEST_ALLOC_FAIL {
		value = -1;

		TEST_DIVERT_STDERR (output) {
			ret = mode_option (&opt, "frodo");
		}
		rewind (output);

		TEST_LT (ret, 0);

		TEST_FILE_EQ (output, "test: illegal output mode: frodo\n");
		TEST_FILE_EQ (output, "Try `test --help' for more information.\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	fclose (output);
}


extern char *source_file_path (const void *parent, const char *output_path,
			       const char *filename)
	__attribute__ ((warn_unused_result, malloc));
extern char *header_file_path (const void *parent, const char *output_path,
			       const char *filename)
	__attribute__ ((warn_unused_result, malloc));

void
test_source_file_path (void)
{
	char *str;

	TEST_FUNCTION ("source_file_path");


	/* Check that when given an output path for the source file with
	 * the expected extension, the function returns the path unchanged.
	 */
	TEST_FEATURE ("with expected extension for output path");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, "/path/to/output.c", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "/path/to/output.c");

		nih_free (str);
	}


	/* Check that when given an output path for the source file with
	 * a different extension than expected, the function still returns
	 * the path unchanged.
	 */
	TEST_FEATURE ("with unusual extension for output path");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, "/path/to/output.cpp", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "/path/to/output.cpp");

		nih_free (str);
	}


	/* Check that when given an output path with no extension at all,
	 * the function still returns the path unchanged.
	 */
	TEST_FEATURE ("with no extension for output path");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, "/path/to/output", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "/path/to/output");

		nih_free (str);
	}


	/* Check that when given an output path for the header file, the
	 * function returns the path with the extension changed to be
	 * the expected extension for a source file.
	 */
	TEST_FEATURE ("with header extension for output path");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, "/path/to/output.h", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "/path/to/output.c");

		nih_free (str);
	}


	/* Check that when given an input filename with the usual extension,
	 * the directory is stripped and the extension is replaced with the
	 * expected extension for a source file.
	 */
	TEST_FEATURE ("with expected extension for input filename");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, NULL, "/path/to/input.xml");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.c");

		nih_free (str);
	}


	/* Check that when given an input filename with an unusual extension,
	 * it is still replaced with the expected extension for a source file.
	 */
	TEST_FEATURE ("with unusual extension for input filename");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, NULL, "/path/to/input.xp");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.c");

		nih_free (str);
	}


	/* Check that when given an input filename with no extension, the
	 * expected extension for a source file is appended.
	 */
	TEST_FEATURE ("with no extension for input filename");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, NULL, "/path/to/input");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.c");

		nih_free (str);
	}


	/* Check that when given an input filename with the usual extension
	 * for a source file, the extension is duplicated so as not to
	 * overwrite the input even though the directory is stripped.
	 */
	TEST_FEATURE ("with source extension for input filename");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, NULL, "/path/to/input.c");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.c.c");

		nih_free (str);
	}


	/* Check that when given an input filename with the usual extension
	 * for a header file, the epxected extension for a source file is
	 * appended rather than replaced to match the header file.
	 */
	TEST_FEATURE ("with header extension for input filename");
	TEST_ALLOC_FAIL {
		str = source_file_path (NULL, NULL, "/path/to/input.h");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.h.c");

		nih_free (str);
	}
}

void
test_header_file_path (void)
{
	char *str;

	TEST_FUNCTION ("header_file_path");


	/* Check that when given an output path for the source file with
	 * the expected extension, the function replaces the extension
	 * with the expected extension for a header file.
	 */
	TEST_FEATURE ("with expected extension for output path");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, "/path/to/output.c", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "/path/to/output.h");

		nih_free (str);
	}


	/* Check that when given an output path for the source file with
	 * a different extension than expected, the function still replaces
	 * the extension with the expected extension for a header file.
	 */
	TEST_FEATURE ("with unusual extension for output path");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, "/path/to/output.cpp", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "/path/to/output.h");

		nih_free (str);
	}


	/* Check that when given an output path with no extension at all,
	 * the function appends the expected extension for a header file.
	 */
	TEST_FEATURE ("with no extension for output path");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, "/path/to/output", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "/path/to/output.h");

		nih_free (str);
	}


	/* Check that when given an output path for the header file, the
	 * function returns the path unchanged.
	 */
	TEST_FEATURE ("with header extension for output path");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, "/path/to/output.h", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "/path/to/output.h");

		nih_free (str);
	}


	/* Check that when given an input filename with the usual extension,
	 * the directory is stripped and the extension is replaced with the
	 * expected extension for a header file.
	 */
	TEST_FEATURE ("with expected extension for input filename");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, NULL, "/path/to/input.xml");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.h");

		nih_free (str);
	}


	/* Check that when given an input filename with an unusual extension,
	 * it is still replaced with the expected extension for a source file.
	 */
	TEST_FEATURE ("with unusual extension for input filename");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, NULL, "/path/to/input.xp");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.h");

		nih_free (str);
	}


	/* Check that when given an input filename with no extension, the
	 * expected extension for a header file is appended.
	 */
	TEST_FEATURE ("with no extension for input filename");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, NULL, "/path/to/input");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.h");

		nih_free (str);
	}


	/* Check that when given an input filename with the usual extension
	 * for a source file, the extension is duplicated so as not to
	 * overwrite the input even though the directory is stripped.
	 */
	TEST_FEATURE ("with source extension for input filename");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, NULL, "/path/to/input.c");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.c.h");

		nih_free (str);
	}


	/* Check that when given an input filename with the usual extension
	 * for a header file, the expected extension for a header file is
	 * appended rather than replaced to match the source file.
	 */
	TEST_FEATURE ("with header extension for input filename");
	TEST_ALLOC_FAIL {
		str = header_file_path (NULL, NULL, "/path/to/input.h");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "input.h.h");

		nih_free (str);
	}
}


int
main (int   argc,
      char *argv[])
{
	program_name = "test";

	test_mode_option ();
	test_source_file_path ();
	test_header_file_path ();

	return 0;
}
