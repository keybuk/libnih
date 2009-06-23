/* libnih
 *
 * test_option.c - test suite for nih/option.c
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
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/main.h>
#include <nih/option.h>
#include <nih/logging.h>


static int daemonise = 0;
static int recursive = 0;
static char *filename = NULL;
static char *wibble = NULL;
static char *option = NULL;

static int was_called = 0;
static NihOption *last_option = NULL;
static const char *last_arg = NULL;

static int
my_setter (NihOption *option, const char *arg)
{
	was_called++;

	last_option = malloc (sizeof (NihOption));
	memcpy (last_option, option, sizeof (NihOption));

	last_arg = arg;
	if (arg && (! strcmp (arg, "fail")))
		return -1;

	return 0;
}


static NihOptionGroup test_group1 = { "First test group" };
static NihOptionGroup test_group2 = { "Second test group" };
static NihOption options[] = {
	{ 'd', NULL, "become daemon",
	  &test_group1, NULL, &daemonise, NULL },
	{ 'f', "filename", "read this file",
	  &test_group1, "FILENAME", &filename, NULL },
	{ 'R', "recursive", "descend into sub-directories",
	  &test_group2, NULL, &recursive, NULL },
	{ 0, "wibble", "bored of inventing names",
	  &test_group2, NULL, &wibble, NULL },
	{ 'o', "option", "extended options",
	  &test_group2, "OPTION", &option, NULL },
	{ 's', "special", "something with special treatment",
	  &test_group2, "SPECIAL-LONG-ARGUMENT-NAME", NULL, my_setter },
	{ 'x', "execute", ("run something, give this a really long help "
			   "message so that it word wraps"),
	  &test_group1, NULL, NULL, my_setter },
	{ 'I', NULL, "add directory to include list",
	  &test_group1, "DIRECTORY", NULL, NULL },

	NIH_OPTION_LAST
};
static NihOption catch_options[] = {
	{ '-', "--", NULL, NULL, NULL, NULL, NULL },

	NIH_OPTION_LAST
};


void
test_parser (void)
{
	FILE *output;
	char *argv[16], **args = NULL;
	int   argc;

	TEST_FUNCTION ("nih_option_parser");
	output = tmpfile ();
	program_name = "test";


	/* Check that the option parser can be called with no arguments,
	 * which results in a single-element array being returned with
	 * NULL in the array.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc] = NULL;

		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		nih_free (args);
	}


	/* Check that all non-option arguments are passed through into the
	 * returned NULL-terminated array.
	 */
	TEST_FEATURE ("with all non-option arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc++] = "bar";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_STR (args[1], "bar");
		TEST_EQ_STR (args[2], "baz");
		TEST_EQ_P (args[3], NULL);

		nih_free (args);
	}


	/* Check that a dash on its own is not taken from the arguments.
	 */
	TEST_FEATURE ("with lone dash");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-";
		argv[argc] = NULL;

		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "-");
		TEST_EQ_P (args[1], NULL);

		nih_free (args);
	}


	/* Check that a single short option is taken from the arguments and
	 * the appropriate variable set.
	 */
	TEST_FEATURE ("with single short option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-d";
		argv[argc] = NULL;

		daemonise = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (daemonise);

		nih_free (args);
	}


	/* Check that all short options are taken from the arguments and
	 * all of the appropriate variables set.
	 */
	TEST_FEATURE ("with multiple short options");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-d";
		argv[argc++] = "-R";
		argv[argc] = NULL;

		daemonise = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (daemonise);
		TEST_TRUE (recursive);

		nih_free (args);
	}


	/* Check that multiple short options can be combined into a single
	 * argument, and that they're all handled.
	 */
	TEST_FEATURE ("with combined short options");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-dR";
		argv[argc] = NULL;

		daemonise = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (daemonise);
		TEST_TRUE (recursive);

		nih_free (args);
	}


	/* Check that short options and ordinary arguments can be intermixed,
	 * the arguments are returned in the array and the option values set.
	 */
	TEST_FEATURE ("with intermixed short options and arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc++] = "-d";
		argv[argc++] = "bar";
		argv[argc++] = "-R";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		daemonise = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_STR (args[1], "bar");
		TEST_EQ_STR (args[2], "baz");
		TEST_EQ_P (args[3], NULL);

		TEST_TRUE (daemonise);
		TEST_TRUE (recursive);

		nih_free (args);
	}


	/* Check that the first non-option argument can terminate the
	 * processing of other options when in command mode, and that the
	 * remaining options are returned in the array and the values NOT
	 * set.
	 */
	TEST_FEATURE ("with command-mode short options and arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc++] = "-d";
		argv[argc++] = "bar";
		argv[argc++] = "-R";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		daemonise = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, TRUE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_STR (args[1], "-d");
		TEST_EQ_STR (args[2], "bar");
		TEST_EQ_STR (args[3], "-R");
		TEST_EQ_STR (args[4], "baz");
		TEST_EQ_P (args[5], NULL);

		TEST_FALSE (daemonise);
		TEST_FALSE (recursive);

		nih_free (args);
	}


	/* Check that option processing can be terminated by a double-dash,
	 * and that following options are placed in the arguments and the
	 * values NOT set.
	 */
	TEST_FEATURE ("with short options and terminator");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc++] = "-d";
		argv[argc++] = "--";
		argv[argc++] = "bar";
		argv[argc++] = "-R";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		daemonise = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_STR (args[1], "bar");
		TEST_EQ_STR (args[2], "-R");
		TEST_EQ_STR (args[3], "baz");
		TEST_EQ_P (args[4], NULL);

		TEST_TRUE (daemonise);
		TEST_FALSE (recursive);

		nih_free (args);
	}


	/* Check that a short option can eat the next non-option argument
	 * as its own argument, which is stored in its value and not
	 * returned in the array.
	 */
	TEST_FEATURE ("with short argument option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-f";
		argv[argc++] = "foo";
		argv[argc] = NULL;

		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_EQ_STR (filename, "foo");

		nih_free (filename);
		nih_free (args);
	}


	/* Check that a short option with an argument can be specified
	 * multiple times, with only the last one being kept.
	 */
	TEST_FEATURE ("with repeated short argument option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-f";
		argv[argc++] = "foo";
		argv[argc++] = "-f";
		argv[argc++] = "bar";
		argv[argc] = NULL;

		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_EQ_STR (filename, "bar");

		nih_free (filename);
		nih_free (args);
	}


	/* Check that only the next non-option argument is eaten, and the
	 * rest of the arguments are returned in the array.
	 */
	TEST_FEATURE ("with short argument option and other arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-f";
		argv[argc++] = "foo";
		argv[argc++] = "bar";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "bar");
		TEST_EQ_STR (args[1], "baz");
		TEST_EQ_P (args[2], NULL);

		TEST_EQ_STR (filename, "foo");

		nih_free (filename);
		nih_free (args);
	}


	/* Stress test all the various ways of dealing with short options
	 * at once; in particular check that an option that takes an argument
	 * eats the first argument after the terminator.
	 */
	TEST_FEATURE ("with random mix of short options and arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "wibble";
		argv[argc++] = "-df";
		argv[argc++] = "--";
		argv[argc++] = "foo";
		argv[argc++] = "-R";
		argv[argc++] = "bar";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		daemonise = 0;
		recursive = 0;
		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "wibble");
		TEST_EQ_STR (args[1], "-R");
		TEST_EQ_STR (args[2], "bar");
		TEST_EQ_STR (args[3], "baz");
		TEST_EQ_P (args[4], NULL);

		TEST_TRUE (daemonise);
		TEST_FALSE (recursive);
		TEST_EQ_STR (filename, "foo");

		nih_free (filename);
		nih_free (args);
	}


	/* Check that the argument for a short option can immediately
	 * follow it, combined into one word.  Check that the characters
	 * of this word aren't treated as options.
	 */
	TEST_FEATURE ("with short option and embedded argument");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-fROOT";
		argv[argc] = NULL;

		filename = NULL;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_EQ_STR (filename, "ROOT");
		TEST_FALSE (recursive);

		nih_free (filename);
		nih_free (args);
	}


	/* Check that the short option may be inside a sequence of short
	 * options in one argument, and then only the next non-option argument
	 * is considered, not the remainder of the option argument.
	 */
	TEST_FEATURE ("with short option and non-embedded argument");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-dfR";
		argv[argc++] = "foo";
		argv[argc] = NULL;

		filename = NULL;
		daemonise = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (daemonise);
		TEST_TRUE (recursive);
		TEST_EQ_STR (filename, "foo");

		nih_free (filename);
		nih_free (args);
	}


	/* Check that multiple short options which accept arguments each
	 * take the next non-option argument, not themselves or the same
	 * argument.
	 */
	TEST_FEATURE ("with multiple short argument options");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-f";
		argv[argc++] = "-o";
		argv[argc++] = "foo";
		argv[argc++] = "bar";
		argv[argc] = NULL;

		filename = NULL;
		option = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_EQ_STR (filename, "foo");
		TEST_EQ_STR (option, "bar");

		nih_free (filename);
		nih_free (option);
		nih_free (args);
	}


	/* Check that a single long option is taken from the arguments and
	 * the appropriate variable set.
	 */
	TEST_FEATURE ("with single long option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--wibble";
		argv[argc] = NULL;

		wibble = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (wibble);

		nih_free (args);
	}


	/* Check that multiple long options are taken from the arguments
	 * and the appropriate variables set.
	 */
	TEST_FEATURE ("with multiple long options");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--wibble";
		argv[argc++] = "--recursive";
		argv[argc] = NULL;

		wibble = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (wibble);
		TEST_TRUE (recursive);

		nih_free (args);
	}


	/* Check that only the long options are taken from the arguments,
	 * and the non-option arguments are returned in the array.
	 */
	TEST_FEATURE ("with intermixed long options and arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc++] = "--wibble";
		argv[argc++] = "bar";
		argv[argc++] = "--recursive";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		wibble = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_STR (args[1], "bar");
		TEST_EQ_STR (args[2], "baz");
		TEST_EQ_P (args[3], NULL);

		TEST_TRUE (wibble);
		TEST_TRUE (recursive);

		nih_free (args);
	}


	/* Check that long options after the first non-option argument can
	 * be ignored when in command mode, and returned in the array with
	 * their value NOT being set.
	 */
	TEST_FEATURE ("with command-mode long options and arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc++] = "--wibble";
		argv[argc++] = "bar";
		argv[argc++] = "--recursive";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		wibble = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, TRUE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_STR (args[1], "--wibble");
		TEST_EQ_STR (args[2], "bar");
		TEST_EQ_STR (args[3], "--recursive");
		TEST_EQ_STR (args[4], "baz");
		TEST_EQ_P (args[5], NULL);

		TEST_FALSE (wibble);
		TEST_FALSE (recursive);

		nih_free (args);
	}


	/* Check that long options after the double-dash terminator are
	 * ignored and returned in the array without their value being set.
	 */
	TEST_FEATURE ("with long options and terminator");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc++] = "--wibble";
		argv[argc++] = "--";
		argv[argc++] = "bar";
		argv[argc++] = "--recursive";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		wibble = 0;
		recursive = 0;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_STR (args[1], "bar");
		TEST_EQ_STR (args[2], "--recursive");
		TEST_EQ_STR (args[3], "baz");
		TEST_EQ_P (args[4], NULL);

		TEST_TRUE (wibble);
		TEST_FALSE (recursive);

		nih_free (args);
	}


	/* Check that a long option may take an argument, which eats the
	 * next non-option argument and stores that in the value instead.
	 */
	TEST_FEATURE ("with long argument option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--filename";
		argv[argc++] = "foo";
		argv[argc] = NULL;

		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_EQ_STR (filename, "foo");

		nih_free (filename);
		nih_free (args);
	}


	/* Check that a long option with an argument may be repeated,
	 * with only the lat value being taken.
	 */
	TEST_FEATURE ("with repeated long argument option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--filename";
		argv[argc++] = "foo";
		argv[argc++] = "--filename";
		argv[argc++] = "bar";
		argv[argc] = NULL;

		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_EQ_STR (filename, "bar");

		nih_free (filename);
		nih_free (args);
	}


	/* Check that only the first non-option argument is eaten by a long
	 * option, and subsequent arguments are stilled returned in the
	 * array.
	 */
	TEST_FEATURE ("with long argument option and other arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--filename";
		argv[argc++] = "foo";
		argv[argc++] = "bar";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "bar");
		TEST_EQ_STR (args[1], "baz");
		TEST_EQ_P (args[2], NULL);

		TEST_EQ_STR (filename, "foo");

		nih_free (filename);
		nih_free (args);
	}


	/* Stress test all the various ways of dealing with long options
	 * at once; in particular check that an option that takes an argument
	 * eats the first argument after the terminator.
	 */
	TEST_FEATURE ("with random mix of long options and arguments");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "wibble";
		argv[argc++] = "--wibble";
		argv[argc++] = "--filename";
		argv[argc++] = "--";
		argv[argc++] = "foo";
		argv[argc++] = "--recursive";
		argv[argc++] = "bar";
		argv[argc++] = "baz";
		argv[argc] = NULL;

		wibble = 0;
		recursive = 0;
		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "wibble");
		TEST_EQ_STR (args[1], "--recursive");
		TEST_EQ_STR (args[2], "bar");
		TEST_EQ_STR (args[3], "baz");
		TEST_EQ_P (args[4], NULL);

		TEST_TRUE (wibble);
		TEST_FALSE (recursive);
		TEST_EQ_STR (filename, "foo");

		nih_free (filename);
		nih_free (args);
	}


	/* Check that the argument to a long option may be embedded into
	 * it, following an equals sign.
	 */
	TEST_FEATURE ("with long option and embedded argument");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--filename=ROOT";
		argv[argc] = NULL;

		filename = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_EQ_STR (filename, "ROOT");

		nih_free (filename);
		nih_free (args);
	}


	/* Check that multiple long options with arguments each eat the
	 * next non-option argument, not the same one.
	 */
	TEST_FEATURE ("with multiple long argument options");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--filename";
		argv[argc++] = "--option";
		argv[argc++] = "foo";
		argv[argc++] = "bar";
		argv[argc] = NULL;

		filename = NULL;
		option = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_EQ_STR (filename, "foo");
		TEST_EQ_STR (option, "bar");

		nih_free (filename);
		nih_free (option);
		nih_free (args);
	}


	/* Check that an invalid short option causes an error message to
	 * be output with a suggestion of help, and NULL to be returned.
	 */
	TEST_FEATURE ("with invalid short option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-z";
		argv[argc] = NULL;

		TEST_DIVERT_STDERR (output) {
			args = nih_option_parser (NULL, argc, argv,
						  options, FALSE);
		}
		rewind (output);

		TEST_EQ_P (args, NULL);

		TEST_FILE_EQ (output, "test: invalid option: -z\n");
		TEST_FILE_EQ (output,
			      "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that an invalid short option is ignored if there's a
	 * catch-all option in the list.
	 */
	TEST_FEATURE ("with invalid short option and catch-all");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-z";
		argv[argc] = NULL;
		args = nih_option_parser (NULL, argc, argv,
					  catch_options, FALSE);

		TEST_NE_P (args, NULL);

		nih_free (args);
	}


	/* Check that an invalid long option causes an error message to
	 * be output with a suggestion of help, and NULL to be returned.
	 */
	TEST_FEATURE ("with invalid long option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--zoiks";
		argv[argc] = NULL;

		TEST_DIVERT_STDERR (output) {
			args = nih_option_parser (NULL, argc, argv,
						  options, FALSE);
		}
		rewind (output);

		TEST_EQ_P (args, NULL);

		TEST_FILE_EQ (output, "test: invalid option: --zoiks\n");
		TEST_FILE_EQ (output,
			      "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that an invalid long option is ignored if there's a
	 * catch-all option in the list.
	 */
	TEST_FEATURE ("with invalid long option and catch-all");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--zoiks";
		argv[argc] = NULL;
		args = nih_option_parser (NULL, argc, argv,
					  catch_options, FALSE);

		TEST_NE_P (args, NULL);

		nih_free (args);
	}


	/* Check that an unexpected argument to a long option causes an
	 * error message to be output with a suggestion of help, and NULL
	 * to be returned.
	 */
	TEST_FEATURE ("with unexpected long option argument");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--wibble=woo";
		argv[argc] = NULL;

		TEST_DIVERT_STDERR (output) {
			args = nih_option_parser (NULL, argc, argv,
						  options, FALSE);
		}
		rewind (output);

		TEST_EQ_P (args, NULL);

		TEST_FILE_EQ (output,
			      "test: unexpected argument: --wibble=woo\n");
		TEST_FILE_EQ (output,
			      "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that an missing argument to a short option causes an error
	 * message to be output with a suggestion of help, and NULL to be
	 * returned.
	 */
	TEST_FEATURE ("with missing short option argument");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-f";
		argv[argc] = NULL;

		TEST_DIVERT_STDERR (output) {
			args = nih_option_parser (NULL, argc, argv,
						  options, FALSE);
		}
		rewind (output);

		TEST_EQ_P (args, NULL);

		TEST_FILE_EQ (output, "test: missing argument: -f\n");
		TEST_FILE_EQ (output, "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that an missing argument to a long option causes an error
	 * message to be output with a suggestion of help, and NULL to be
	 * returned.
	 */
	TEST_FEATURE ("with missing long option argument");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--filename";
		argv[argc] = NULL;

		TEST_DIVERT_STDERR (output) {
			args = nih_option_parser (NULL, argc, argv,
						  options, FALSE);
		}
		rewind (output);

		TEST_EQ_P (args, NULL);

		TEST_FILE_EQ (output, "test: missing argument: --filename\n");
		TEST_FILE_EQ (output, "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that a short option may result in a function call, and
	 * that the arguments to that call are correct.
	 */
	TEST_FEATURE ("with short setter option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-x";
		argv[argc++] = "foo";
		argv[argc] = NULL;

		was_called = 0;
		last_option = NULL;
		last_arg = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_P (args[1], NULL);

		TEST_TRUE (was_called);
		TEST_EQ (last_option->option, options[6].option);
		TEST_EQ_P (last_arg, NULL);

		nih_free (args);
		free (last_option);
	}


	/* Check that a short option that takens an argument can result in
	 * a function call, and that the argument is also passed to the
	 * function call.
	 */
	TEST_FEATURE ("with short setter argument option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-s";
		argv[argc++] = "foo";
		argv[argc] = NULL;

		was_called = 0;
		last_option = NULL;
		last_arg = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (was_called);
		TEST_EQ (last_option->option, options[5].option);
		TEST_EQ_STR (last_arg, "foo");

		nih_free (args);
		free (last_option);
	}


	/* Check that the setter function is stilled correctly if the
	 * argument to the short option is embedded within it.
	 */
	TEST_FEATURE ("with short setter embedded argument option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-sfoo";
		argv[argc] = NULL;

		was_called = 0;
		last_option = NULL;
		last_arg = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (was_called);
		TEST_EQ (last_option->option, options[5].option);
		TEST_EQ_STR (last_arg, "foo");

		nih_free (args);
		free (last_option);
	}


	/* Check that a long option may result in a function call, and
	 * that the arguments to that call are correct.
	 */
	TEST_FEATURE ("with long setter option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--execute";
		argv[argc++] = "foo";
		argv[argc] = NULL;

		was_called = 0;
		last_option = NULL;
		last_arg = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_STR (args[0], "foo");
		TEST_EQ_P (args[1], NULL);

		TEST_TRUE (was_called);
		TEST_EQ (last_option->option, options[6].option);
		TEST_EQ_P (last_arg, NULL);

		nih_free (args);
		free (last_option);
	}


	/* Check that a short option that takens an argument can result in
	 * a function call, and that the argument is also passed to the
	 * function call.
	 */
	TEST_FEATURE ("with long setter argument option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--special";
		argv[argc++] = "foo";
		argv[argc] = NULL;
		was_called = 0;
		last_option = NULL;
		last_arg = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (was_called);
		TEST_EQ (last_option->option, options[5].option);
		TEST_EQ_STR (last_arg, "foo");

		nih_free (args);
		free (last_option);
	}


	/* Check that the setter function is stilled correctly if the
	 * argument to the short option is embedded within it.
	 */
	TEST_FEATURE ("with long setter embedded argument option");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--special=foo";
		argv[argc] = NULL;

		was_called = 0;
		last_option = NULL;
		last_arg = NULL;
		args = nih_option_parser (NULL, argc, argv, options, FALSE);

		TEST_NE_P (args, NULL);
		TEST_EQ_P (args[0], NULL);

		TEST_TRUE (was_called);
		TEST_EQ (last_option->option, options[5].option);
		TEST_EQ_STR (last_arg, "foo");

		nih_free (args);
		free (last_option);
	}


	/* Check that an error code returned from a setter function for a
	 * short option results in NULL being returned by the parser, but
	 * no error message output (that's left up to the function).
	 */
	TEST_FEATURE ("with short setter embedded argument error");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-sfail";
		argv[argc] = NULL;

		was_called = 0;
		last_option = NULL;
		last_arg = NULL;

		TEST_DIVERT_STDERR (output) {
			args = nih_option_parser (NULL, argc, argv,
						  options, FALSE);
		}
		rewind (output);

		TEST_EQ_P (args, NULL);

		TEST_TRUE (was_called);
		TEST_EQ (last_option->option, options[5].option);
		TEST_EQ_STR (last_arg, "fail");

		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
		free (last_option);
	}


	/* Check that an error code returned from a setter function for a
	 * long option results in NULL being returned by the parser, but
	 * no error message output (that's left up to the function).
	 */
	TEST_FEATURE ("with long setter embedded argument error");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--special=fail";
		argv[argc] = NULL;

		was_called = 0;
		last_option = NULL;
		last_arg = NULL;

		TEST_DIVERT_STDERR (output) {
			args = nih_option_parser (NULL, argc, argv,
						  options, FALSE);
		}
		rewind (output);

		TEST_EQ_P (args, NULL);

		TEST_TRUE (was_called);
		TEST_EQ (last_option->option, options[5].option);
		TEST_EQ_STR (last_arg, "fail");

		TEST_FILE_END (output);

		free (last_option);
	}

	fclose (output);
}


void
test_count (void)
{
	NihOption opt;
	int       ret, value = 0;

	TEST_FUNCTION ("nih_option_count");

	/* Check that the count function treats the option value as an
	 * integer pointer, and increments it.
	 */
	TEST_FEATURE ("with zero value");
	opt.value = &value;
	ret = nih_option_count (&opt, NULL);

	TEST_EQ (ret, 0);
	TEST_EQ (value, 1);


	/* Check that calling again increments the value to two. */
	TEST_FEATURE ("with non-zero value");
	ret = nih_option_count (&opt, NULL);

	TEST_EQ (ret, 0);
	TEST_EQ (value, 2);
}

void
test_int (void)
{
	FILE      *output;
	NihOption  opt;
	int        ret, value = 0;

	TEST_FUNCTION ("nih_option_int");
	opt.value = &value;
	output = tmpfile ();
	program_name = "test";

	/* Check that the int function treats the option value as an
	 * integer pointer, and sets it.
	 */
	TEST_FEATURE ("with positive value");
	ret = nih_option_int (&opt, "42");

	TEST_EQ (ret, 0);
	TEST_EQ (value, 42);


	/* Check that a negative number can be parsed. */
	TEST_FEATURE ("with negative value");
	ret = nih_option_int (&opt, "-14");

	TEST_EQ (ret, 0);
	TEST_EQ (value, -14);


	/* Check that a zero value can be parsed. */
	TEST_FEATURE ("with zero value");
	ret = nih_option_int (&opt, "0");

	TEST_EQ (ret, 0);
	TEST_EQ (value, 0);


	/* Check that a non-numeric argument results in an error. */
	TEST_FEATURE ("with non-numeric argument");
	TEST_DIVERT_STDERR (output) {
		ret = nih_option_int (&opt, "foo");
	}
	rewind (output);

	TEST_LT (ret, 0);

	TEST_FILE_EQ (output, "test: illegal argument: foo\n");
	TEST_FILE_EQ (output, "Try `test --help' for more information.\n");
	TEST_FILE_END (output);

	TEST_FILE_RESET (output);


	/* Check that a partially non-numeric argument results in an error. */
	TEST_FEATURE ("with partially non-numeric argument");
	TEST_DIVERT_STDERR (output) {
		ret = nih_option_int (&opt, "15foo");
	}
	rewind (output);

	TEST_LT (ret, 0);

	TEST_FILE_EQ (output, "test: illegal argument: 15foo\n");
	TEST_FILE_EQ (output, "Try `test --help' for more information.\n");
	TEST_FILE_END (output);

	TEST_FILE_RESET (output);

	fclose (output);
}


static int logger_called = 0;

static int
my_logger (NihLogLevel  priority,
	   const char  *message)
{
	logger_called++;

	return 0;
}

void
test_quiet (void)
{
	char *argv[3], **args;
	int   argc;

	TEST_FUNCTION ("nih_option_quiet");
	program_name = "test";
	nih_log_set_logger (my_logger);

	/* Check that the --quiet option is automatically understood, and
	 * sets the log level such that only the error message is output.
	 */
	TEST_FEATURE ("with long option");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--quiet";
	argv[argc] = NULL;

	logger_called = 0;
	nih_log_set_priority (NIH_LOG_MESSAGE);

	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	nih_debug ("test message");
	nih_info ("test message");
	nih_message ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	TEST_NE_P (args, NULL);
	TEST_EQ_P (args[0], NULL);
	TEST_EQ (logger_called, 1);

	nih_free (args);


	/* Check that the -q option has the same effect. */
	TEST_FEATURE ("with short option");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-q";
	argv[argc] = NULL;

	logger_called = 0;
	nih_log_set_priority (NIH_LOG_MESSAGE);

	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	nih_debug ("test message");
	nih_info ("test message");
	nih_message ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	TEST_NE_P (args, NULL);
	TEST_EQ_P (args[0], NULL);
	TEST_EQ (logger_called, 1);

	nih_free (args);


	nih_log_set_priority (NIH_LOG_MESSAGE);
	nih_log_set_logger (nih_logger_printf);
}

void
test_verbose (void)
{
	char *argv[3], **args;
	int   argc;

	TEST_FUNCTION ("nih_option_verbose");
	program_name = "test";
	nih_log_set_logger (my_logger);

	/* Check that the --verbose option is automatically understood,
	 * and sets the log level such that messages of info, warn and
	 * error priority are output.
	 */
	TEST_FEATURE ("with long option");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--verbose";
	argv[argc] = NULL;

	logger_called = 0;
	nih_log_set_priority (NIH_LOG_MESSAGE);

	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	nih_debug ("test message");
	nih_info ("test message");
	nih_message ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	TEST_NE_P (args, NULL);
	TEST_EQ_P (args[0], NULL);
	TEST_EQ (logger_called, 4);

	nih_free (args);


	/* Check that the -v option has the same effect. */
	TEST_FEATURE ("with short option");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-v";
	argv[argc] = NULL;

	logger_called = 0;
	nih_log_set_priority (NIH_LOG_MESSAGE);

	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	nih_debug ("test message");
	nih_info ("test message");
	nih_message ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	TEST_NE_P (args, NULL);
	TEST_EQ_P (args[0], NULL);
	TEST_EQ (logger_called, 4);

	nih_free (args);


	nih_log_set_priority (NIH_LOG_MESSAGE);
	nih_log_set_logger (nih_logger_printf);
}

void
test_debug (void)
{
	char *argv[3], **args;
	int   argc;

	/* Check that the --debug option is automatically understood,
	 * and sets the log level such that messages of all priorities
	 * are output.
	 */
	TEST_FUNCTION ("nih_option_debug");
	program_name = "test";
	nih_log_set_logger (my_logger);

	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--debug";
	argv[argc] = NULL;

	logger_called = 0;
	nih_log_set_priority (NIH_LOG_MESSAGE);

	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	nih_debug ("test message");
	nih_info ("test message");
	nih_message ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	TEST_NE_P (args, NULL);
	TEST_EQ_P (args[0], NULL);
	TEST_EQ (logger_called, 5);

	nih_free (args);

	nih_log_set_priority (NIH_LOG_MESSAGE);
	nih_log_set_logger (nih_logger_printf);
}

void
test_version (void)
{
	FILE  *output;
	char  *argv[3];
	pid_t  pid;
	int    argc, status;

	/* Check that the --version option is caught, dealt with by outputting
	 * version information to standard output, and terminating the process
	 * with a zero exit code.
	 */
	TEST_FUNCTION ("nih_option_version");
	nih_main_init_full ("test", "wibble", "1.0",
			    "foo@bar.com", "Copyright Message");

	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--version";
		argv[argc] = NULL;

		output = tmpfile ();
		TEST_CHILD (pid) {
			TEST_DIVERT_STDOUT (output) {
				char **args;

				args = nih_option_parser (NULL, argc, argv,
							  options, FALSE);
				exit (1);
			}
		}

		waitpid (pid, &status, 0);
		rewind (output);

		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_FILE_EQ (output, "test (wibble 1.0)\n");
		TEST_FILE_EQ (output, "Copyright Message\n");
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ_N (output, "This is free software;");
		TEST_FILE_EQ_N (output, "warranty; not even for");
		TEST_FILE_END (output);

		fclose (output);
	}
}

void
test_help (void)
{
	FILE  *output;
	char  *argv[3];
	pid_t  pid;
	int    argc, status;

	/* Check that these functions set their appropriate string, this
	 * is only possible by checking the help output, so we call them
	 * and do the tests later.
	 */
	TEST_FUNCTION ("nih_option_set_usage_stem");
	nih_option_set_usage_stem ("[OPT]...");

	TEST_FUNCTION ("nih_option_set_usage");
	nih_option_set_usage ("CMD [ARG]...");

	TEST_FUNCTION ("nih_option_set_synopsis");
	nih_option_set_synopsis ("Frobnicates bars carefully, taking into "
				 "account things that are important when "
				 "doing that");

	TEST_FUNCTION ("nih_option_set_help");
	nih_option_set_help ("This is the help text for the bar frobnication "
			     "program.\n\n"
			     "It is also wrapped to the screen width, so it "
			     "can be as long as we like, and can also include "
			     "paragraph breaks and stuff.");

	TEST_FUNCTION ("nih_option_set_footer");
	nih_option_set_footer ("Go away!");


	/* Check that the --help option is caught, dealt with by outputting
	 * information about the options to standard output, and terminating
	 * the process with a zero exit code.
	 */
	TEST_FUNCTION ("nih_option_help");
	nih_main_init_full ("test", "wibble", "1.0",
			    "foo@bar.com", "Copyright Message");

	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--help";
		argv[argc] = NULL;

		output = tmpfile ();
		TEST_CHILD (pid) {
			unsetenv ("COLUMNS");

			TEST_DIVERT_STDOUT (output) {
				char **args;

				args = nih_option_parser (NULL, argc, argv,
							  options, FALSE);
				exit (1);
			}
		}

		waitpid (pid, &status, 0);
		rewind (output);

		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_FILE_EQ (output, "Usage: test [OPT]... CMD [ARG]...\n");
		TEST_FILE_EQ (output, ("Frobnicates bars carefully, taking "
				       "into account things that are "
				       "important when\n"));
		TEST_FILE_EQ (output, ("doing that\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "First test group options:\n");
		TEST_FILE_EQ (output, ("  -d                          "
				       "become daemon\n"));
		TEST_FILE_EQ (output, ("  -f, --filename=FILENAME     "
				       "read this file\n"));
		TEST_FILE_EQ (output, ("  -x, --execute               "
				       "run something, give this a really "
				       "long help\n"));
		TEST_FILE_EQ (output, ("                              "
				       "  message so that it word wraps\n"));
		TEST_FILE_EQ (output, ("  -I DIRECTORY                "
				       "add directory to include list\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Second test group options:\n");
		TEST_FILE_EQ (output, ("  -R, --recursive             "
				       "descend into sub-directories\n"));
		TEST_FILE_EQ (output, ("      --wibble                "
				       "bored of inventing names\n"));
		TEST_FILE_EQ (output, ("  -o, --option=OPTION         "
				       "extended options\n"));
		TEST_FILE_EQ (output, ("  -s, --special=SPECIAL-LONG-"
				       "ARGUMENT-NAME\n"));
		TEST_FILE_EQ (output, ("                              "
				       "something with special treatment\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Other options:\n");
		TEST_FILE_EQ (output, ("  -q, --quiet                 "
				       "reduce output to errors only\n"));
		TEST_FILE_EQ (output, ("  -v, --verbose               "
				       "increase output to include "
				       "informational messages\n"));
		TEST_FILE_EQ (output, ("      --help                  "
				       "display this help and exit\n"));
		TEST_FILE_EQ (output, ("      --version               "
				       "output version information and "
				       "exit\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, ("This is the help text for the bar "
				       "frobnication program.\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, ("It is also wrapped to the screen "
				       "width, so it can be as long as "
				       "we like, and\n"));
		TEST_FILE_EQ (output, ("can also include paragraph breaks "
				       "and stuff.\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Go away!\n");
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Report bugs to <foo@bar.com>\n");
		TEST_FILE_END (output);

		fclose (output);
	}
}


int
main (int   argc,
      char *argv[])
{
	test_parser ();
	test_count ();
	test_int ();
	test_quiet ();
	test_verbose ();
	test_debug ();
	test_version ();
	test_help ();

	return 0;
}
