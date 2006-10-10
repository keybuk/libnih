/* libnih
 *
 * test_option.c - test suite for nih/option.c
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

#include <config.h>


#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
	last_option = option;
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


int
test_parser (void)
{
	FILE *output;
	char  text[81];
	char *argv[16], **args;
	int   oldstderr, ret = 0, argc;

	printf ("Testing nih_option_parser()\n");
	program_name = "test";

	output = tmpfile ();
	oldstderr = dup (STDERR_FILENO);


	printf ("...with no arguments\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc] = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with all non-option arguments\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "foo";
	argv[argc++] = "bar";
	argv[argc++] = "baz";
	argv[argc] = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be second argument */
	if (strcmp (args[1], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be third argument */
	if (strcmp (args[2], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[3] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with single short option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-d";
	argv[argc] = NULL;
	daemonise = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Daemonise variable should have been set */
	if (! daemonise) {
		printf ("BAD: daemonise value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with multiple short options\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-d";
	argv[argc++] = "-R";
	argv[argc] = NULL;
	daemonise = 0;
	recursive = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Daemonise variable should have been set */
	if (! daemonise) {
		printf ("BAD: daemonise value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should have been set */
	if (! recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with combined short options\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-dR";
	argv[argc] = NULL;
	daemonise = 0;
	recursive = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Daemonise variable should have been set */
	if (! daemonise) {
		printf ("BAD: daemonise value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should have been set */
	if (! recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with intermixed short options and arguments\n");
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

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be second argument */
	if (strcmp (args[1], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be third argument */
	if (strcmp (args[2], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[3] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Daemonise variable should have been set */
	if (! daemonise) {
		printf ("BAD: daemonise value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should have been set */
	if (! recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with command-mode short options and arguments\n");
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

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be first option */
	if (strcmp (args[1], "-d")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be second argument */
	if (strcmp (args[2], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Fourth array entry should be second option */
	if (strcmp (args[3], "-R")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Firth array entry should be third argument */
	if (strcmp (args[4], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[5] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Daemonise variable should not have been set */
	if (daemonise) {
		printf ("BAD: daemonise value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should have been set */
	if (recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with short options and terminator\n");
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

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be second argument */
	if (strcmp (args[1], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be the not-option */
	if (strcmp (args[2], "-R")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Fourth array entry should be third argument */
	if (strcmp (args[3], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[4] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Daemonise variable should have been set */
	if (! daemonise) {
		printf ("BAD: daemonise value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should NOT have been set */
	if (recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with short argument option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-f";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	filename = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);



	printf ("...with short argument option and other arguments\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-f";
	argv[argc++] = "foo";
	argv[argc++] = "bar";
	argv[argc++] = "baz";
	argv[argc] = NULL;
	filename = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* First array entry should be first argument */
	if (strcmp (args[0], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be second argument */
	if (strcmp (args[1], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[2] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);


	printf ("...with random mix of short options and arguments\n");
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

	/* First array entry should be previous to options */
	if (strcmp (args[0], "wibble")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be non-option */
	if (strcmp (args[1], "-R")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be first argument */
	if (strcmp (args[2], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Fourth array entry should be second argument */
	if (strcmp (args[3], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[4] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Daemonise variable should have been set */
	if (! daemonise) {
		printf ("BAD: daemonise value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should NOT have been set */
	if (recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);


	printf ("...with short option and embedded argument\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-fROOT";
	argv[argc] = NULL;
	filename = NULL;
	recursive = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to remainder of argument */
	if (strcmp (filename, "ROOT")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should not have been set */
	if (recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);


	printf ("...with short option and non-embedded argument\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-dfR";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	filename = NULL;
	daemonise = 0;
	recursive = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to first argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	/* Daemonise variable should have been set */
	if (! daemonise) {
		printf ("BAD: daemonise value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should have been set */
	if (! recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);


	printf ("...with multiple short argument options\n");
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

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	/* Option variable should be set to argument */
	if (strcmp (option, "bar")) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (option);
	nih_free (args);


	printf ("...with single long option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--wibble";
	argv[argc] = NULL;
	wibble = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Wibble variable should have been set */
	if (! wibble) {
		printf ("BAD: wibble value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with multiple long options\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--wibble";
	argv[argc++] = "--recursive";
	argv[argc] = NULL;
	wibble = 0;
	recursive = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Wibble variable should have been set */
	if (! wibble) {
		printf ("BAD: wibble value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should have been set */
	if (! recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with intermixed long options and arguments\n");
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

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be second argument */
	if (strcmp (args[1], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be third argument */
	if (strcmp (args[2], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[3] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Wibble variable should have been set */
	if (! wibble) {
		printf ("BAD: wibble value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should have been set */
	if (! recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with command-mode long options and arguments\n");
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

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be first option */
	if (strcmp (args[1], "--wibble")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be second argument */
	if (strcmp (args[2], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Fourth array entry should be third argument */
	if (strcmp (args[3], "--recursive")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be third argument */
	if (strcmp (args[4], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[5] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Wibble variable should not have been set */
	if (wibble) {
		printf ("BAD: wibble value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should not have been set */
	if (recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with long options and terminator\n");
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

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be second argument */
	if (strcmp (args[1], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be non-option */
	if (strcmp (args[2], "--recursive")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Fourth array entry should be third argument */
	if (strcmp (args[3], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[4] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Wibble variable should have been set */
	if (! wibble) {
		printf ("BAD: wibble value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should have NOT been set */
	if (recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with long argument option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--filename";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	filename = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);


	printf ("...with long argument option and other arguments\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--filename";
	argv[argc++] = "foo";
	argv[argc++] = "bar";
	argv[argc++] = "baz";
	argv[argc] = NULL;
	filename = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* First array entry should be first argument */
	if (strcmp (args[0], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be second argument */
	if (strcmp (args[1], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[2] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);


	printf ("...with random mix of long options and arguments\n");
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

	/* First array entry should be previous to options */
	if (strcmp (args[0], "wibble")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Second array entry should be non-option */
	if (strcmp (args[1], "--recursive")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Third array entry should be first argument */
	if (strcmp (args[2], "bar")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Fourth array entry should be second argument */
	if (strcmp (args[3], "baz")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[4] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Wibble variable should have been set */
	if (! wibble) {
		printf ("BAD: wibble value wasn't what we expected.\n");
		ret = 1;
	}

	/* Recursive variable should NOT have been set */
	if (recursive) {
		printf ("BAD: recursive value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);


	printf ("...with long option and embedded argument\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--filename=ROOT";
	argv[argc] = NULL;
	filename = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to remainder of argument */
	if (strcmp (filename, "ROOT")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (args);


	printf ("...with multiple long argument options\n");
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

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Filename variable should be set to argument */
	if (strcmp (filename, "foo")) {
		printf ("BAD: filename value wasn't what we expected.\n");
		ret = 1;
	}

	/* Option variable should be set to argument */
	if (strcmp (option, "bar")) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (filename);
	nih_free (option);
	nih_free (args);


	printf ("...with invalid short option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-z";
	argv[argc] = NULL;

	dup2 (fileno (output), STDERR_FILENO);
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be NULL */
	if (args != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Output should be message with program name and newline */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: invalid option: -z\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should include a suggestion of --help */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Try `test --help' for more information.\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	ftruncate (fileno (output), 0);


	printf ("...with invalid short option and catch-all\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-z";
	argv[argc] = NULL;
	args = nih_option_parser (NULL, argc, argv, catch_options, FALSE);

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with invalid long option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--zoiks";
	argv[argc] = NULL;

	dup2 (fileno (output), STDERR_FILENO);
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be NULL */
	if (args != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Output should be message with program name and newline */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: invalid option: --zoiks\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should include a suggestion of --help */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Try `test --help' for more information.\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	ftruncate (fileno (output), 0);


	printf ("...with invalid long option and catch-all\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--zoiks";
	argv[argc] = NULL;
	args = nih_option_parser (NULL, argc, argv, catch_options, FALSE);

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with unexpected long option argument\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--wibble=woo";
	argv[argc] = NULL;

	dup2 (fileno (output), STDERR_FILENO);
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be NULL */
	if (args != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Output should be message with program name and newline */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: unexpected argument: --wibble=woo\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should include a suggestion of --help */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Try `test --help' for more information.\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	ftruncate (fileno (output), 0);


	printf ("...with missing short option argument\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-f";
	argv[argc] = NULL;
	dup2 (fileno (output), STDERR_FILENO);
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be NULL */
	if (args != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Output should be message with program name and newline */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: missing argument: -f\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should include a suggestion of --help */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Try `test --help' for more information.\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	ftruncate (fileno (output), 0);


	printf ("...with missing long option argument\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--filename";
	argv[argc] = NULL;

	dup2 (fileno (output), STDERR_FILENO);
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be NULL */
	if (args != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Output should be message with program name and newline */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: missing argument: --filename\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should include a suggestion of --help */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Try `test --help' for more information.\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	ftruncate (fileno (output), 0);


	printf ("...with short setter option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-x";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	was_called = 0;
	last_option = NULL;
	last_arg = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[1] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Setter function should have been called */
	if (! was_called) {
		printf ("BAD: setter function was not called.\n");
		ret = 1;
	}

	/* Option passed should have been the execute one */
	if (last_option != &(options[6])) {
		printf ("BAD: setter's option wasn't what we expected.\n");
		ret = 1;
	}

	/* Argument passed to setter should have been NULL */
	if (last_arg != NULL) {
		printf ("BAD setter's argument wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with short setter argument option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-s";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	was_called = 0;
	last_option = NULL;
	last_arg = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Setter function should have been called */
	if (! was_called) {
		printf ("BAD: setter function was not called.\n");
		ret = 1;
	}

	/* Option passed should have been the special one */
	if (last_option != &(options[5])) {
		printf ("BAD: setter's option wasn't what we expected.\n");
		ret = 1;
	}

	/* Argument passed to setter should have been foo */
	if (strcmp (last_arg, "foo")) {
		printf ("BAD setter's argument wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with short setter embedded argument option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-sfoo";
	argv[argc] = NULL;
	was_called = 0;
	last_option = NULL;
	last_arg = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Setter function should have been called */
	if (! was_called) {
		printf ("BAD: setter function was not called.\n");
		ret = 1;
	}

	/* Option passed should have been the special one */
	if (last_option != &(options[5])) {
		printf ("BAD: setter's option wasn't what we expected.\n");
		ret = 1;
	}

	/* Argument passed to setter should have been foo */
	if (strcmp (last_arg, "foo")) {
		printf ("BAD setter's argument wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with long setter option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--execute";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	was_called = 0;
	last_option = NULL;
	last_arg = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* First array entry should be first argument */
	if (strcmp (args[0], "foo")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[1] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Setter function should have been called */
	if (! was_called) {
		printf ("BAD: setter function was not called.\n");
		ret = 1;
	}

	/* Option passed should have been the execute one */
	if (last_option != &(options[6])) {
		printf ("BAD: setter's option wasn't what we expected.\n");
		ret = 1;
	}

	/* Argument passed to setter should have been NULL */
	if (last_arg != NULL) {
		printf ("BAD setter's argument wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with long setter argument option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--special";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	was_called = 0;
	last_option = NULL;
	last_arg = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Setter function should have been called */
	if (! was_called) {
		printf ("BAD: setter function was not called.\n");
		ret = 1;
	}

	/* Option passed should have been the special one */
	if (last_option != &(options[5])) {
		printf ("BAD: setter's option wasn't what we expected.\n");
		ret = 1;
	}

	/* Argument passed to setter should have been foo */
	if (strcmp (last_arg, "foo")) {
		printf ("BAD setter's argument wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with long setter embedded argument option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--special=foo";
	argv[argc] = NULL;
	was_called = 0;
	last_option = NULL;
	last_arg = NULL;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Setter function should have been called */
	if (! was_called) {
		printf ("BAD: setter function was not called.\n");
		ret = 1;
	}

	/* Option passed should have been the special one */
	if (last_option != &(options[5])) {
		printf ("BAD: setter's option wasn't what we expected.\n");
		ret = 1;
	}

	/* Argument passed to setter should have been foo */
	if (strcmp (last_arg, "foo")) {
		printf ("BAD setter's argument wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with short setter embedded argument error\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-sfail";
	argv[argc] = NULL;
	was_called = 0;
	last_option = NULL;
	last_arg = NULL;

	dup2 (fileno (output), STDERR_FILENO);
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be NULL */
	if (args != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Setter function should have been called */
	if (! was_called) {
		printf ("BAD: setter function was not called.\n");
		ret = 1;
	}

	/* Option passed should have been the special one */
	if (last_option != &(options[5])) {
		printf ("BAD: setter's option wasn't what we expected.\n");
		ret = 1;
	}

	/* Argument passed to setter should have been fail */
	if (strcmp (last_arg, "fail")) {
		printf ("BAD setter's argument wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	ftruncate (fileno (output), 0);


	printf ("...with long setter embedded argument error\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--special=fail";
	argv[argc] = NULL;
	was_called = 0;
	last_option = NULL;
	last_arg = NULL;

	dup2 (fileno (output), STDERR_FILENO);
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be NULL */
	if (args != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Setter function should have been called */
	if (! was_called) {
		printf ("BAD: setter function was not called.\n");
		ret = 1;
	}

	/* Option passed should have been the special one */
	if (last_option != &(options[5])) {
		printf ("BAD: setter's option wasn't what we expected.\n");
		ret = 1;
	}

	/* Argument passed to setter should have been fail */
	if (strcmp (last_arg, "fail")) {
		printf ("BAD setter's argument wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	ftruncate (fileno (output), 0);


	fclose (output);
	close (oldstderr);

	return ret;
}


int
test_count (void)
{
	NihOption opt;
	int       ret = 0, value = 0, retval;

	printf ("Testing nih_option_count()\n");
	opt.value = &value;
	retval = nih_option_count (&opt, NULL);

	/* Return value should be zero */
	if (retval != 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Value should be incremented */
	if (value != 1) {
		printf ("BAD: value wasn't what we expected.\n");
		ret = 1;
	}


	retval = nih_option_count (&opt, NULL);

	/* Return value should be zero */
	if (retval != 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Value should be incremented */
	if (value != 2) {
		printf ("BAD: value wasn't what we expected.\n");
		ret = 1;
	}

	return ret;
}


static int logger_called = 0;

static int
my_logger (NihLogLevel  priority,
	   const char  *message)
{
	logger_called++;

	return 0;
}


int
test_quiet (void)
{
	char *argv[3], **args;
	int   ret = 0, argc;

	printf ("Testing nih_option_quiet()\n");
	program_name = "test";
	nih_log_set_logger (my_logger);

	printf ("...with long option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--quiet";
	argv[argc] = NULL;
	nih_log_set_priority (NIH_LOG_WARN);
	logger_called = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	nih_info ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should have been called only once */
	if (logger_called != 1) {
		printf ("BAD: priority wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with short option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-q";
	argv[argc] = NULL;
	nih_log_set_priority (NIH_LOG_WARN);
	logger_called = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	nih_info ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should have been called only once */
	if (logger_called != 1) {
		printf ("BAD: priority wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	nih_log_set_priority (NIH_LOG_INFO);
	nih_log_set_logger (nih_logger_printf);

	return ret;
}

int
test_verbose (void)
{
	char *argv[3], **args;
	int   ret = 0, argc;

	printf ("Testing nih_option_verbose()\n");
	program_name = "test";
	nih_log_set_logger (my_logger);

	printf ("...with long option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--verbose";
	argv[argc] = NULL;
	nih_log_set_priority (NIH_LOG_WARN);
	logger_called = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	nih_info ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should have been called three times */
	if (logger_called != 3) {
		printf ("BAD: priority wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	printf ("...with short option\n");
	argc = 0
		;
	argv[argc++] = "ignored";
	argv[argc++] = "-v";
	argv[argc] = NULL;
	nih_log_set_priority (NIH_LOG_WARN);
	logger_called = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	nih_info ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should have been called three times */
	if (logger_called != 3) {
		printf ("BAD: priority wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);


	nih_log_set_priority (NIH_LOG_INFO);
	nih_log_set_logger (nih_logger_printf);

	return ret;
}

int
test_debug (void)
{
	char *argv[3], **args;
	int   ret = 0, argc;

	printf ("Testing nih_option_debug()\n");
	program_name = "test";
	nih_log_set_logger (my_logger);

	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--debug";
	argv[argc] = NULL;
	nih_log_set_priority (NIH_LOG_WARN);
	logger_called = 0;
	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	nih_debug ("test message");
	nih_info ("test message");
	nih_warn ("test message");
	nih_error ("test message");

	/* Return value should not be NULL */
	if (args == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Return value should be a NULL array */
	if (args[0] != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should have been called four times */
	if (logger_called != 4) {
		printf ("BAD: priority wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (args);

	nih_log_set_priority (NIH_LOG_INFO);
	nih_log_set_logger (nih_logger_printf);

	return ret;
}


int
test_version (void)
{
	FILE  *output;
	char   text[81];
	char  *argv[3];
	pid_t  pid;
	int    ret = 0, argc, status;

	printf ("Testing nih_option_version()\n");
	program_name = "test";
	package_name = "wibble";
	package_version = "1.0";
	package_copyright = "Copyright Message";

	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--version";
	argv[argc] = NULL;

	output = tmpfile ();
	pid = fork ();
	if (pid == 0) {
		dup2 (fileno (output), STDOUT_FILENO);
		nih_option_parser (NULL, argc, argv, options, FALSE);
		exit (1);
	}

	waitpid (pid, &status, 0);
	rewind (output);

	/* Should have exited normally */
	if ((! WIFEXITED (status)) || (WEXITSTATUS (status) != 0)) {
		printf ("BAD: process did not exit normally.\n");
		ret = 1;
	}

	/* First line of output should be package string */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test (wibble 1.0)\n")) {
		printf ("BAD: package line wasn't what we expected.\n");
		ret = 1;
	}

	/* Second line of output should be copyright message */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Copyright Message\n")) {
		printf ("BAD: copyright line wasn't what we expected.\n");
		ret = 1;
	}

	/* Third line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Fourth line should be start of GPL preamble */
	fgets (text, sizeof (text), output);
	if (strncmp (text, "This is free software;", 22)) {
		printf ("BAD: first licence line wasn't what we expected.\n");
		ret = 1;
	}

	/* Fifth line should be GPL preamble */
	fgets (text, sizeof (text), output);
	if (strncmp (text, "warranty; not even for", 22)) {
		printf ("BAD: second licence line wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	fclose (output);

	return ret;
}

int
test_help (void)
{
	FILE  *output;
	char   text[81];
	char  *argv[3];
	pid_t  pid;
	int    ret = 0, argc, status;

	printf ("Testing nih_option_set_usage()\n");
	nih_option_set_usage ("CMD [ARG]...\n");


	printf ("Testing nih_option_help()\n");
	program_name = "test";
	package_bugreport = "foo@bar.com";

	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--help";
	argv[argc] = NULL;

	output = tmpfile ();
	pid = fork ();
	if (pid == 0) {
		dup2 (fileno (output), STDOUT_FILENO);
		nih_option_parser (NULL, argc, argv, options, FALSE);
		exit (1);
	}

	waitpid (pid, &status, 0);
	rewind (output);

	/* Should have exited normally */
	if ((! WIFEXITED (status)) || (WEXITSTATUS (status) != 0)) {
		printf ("BAD: process did not exit normally.\n");
		ret = 1;
	}

	/* First line of output should be usage string */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Usage: test [OPTION]... CMD [ARG]...\n")) {
		printf ("BAD: usage line wasn't what we expected.\n");
		ret = 1;
	}

	/* Second line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Start of first option group encountered */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "First test group options:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -d                          "
			   "become daemon\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -f, --filename=FILENAME     "
			   "read this file\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -x, --execute               "
			   "run something, give this a really long help\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("                              "
			   "  message so that it word wraps\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -I DIRECTORY                "
			   "add directory to include list\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Start of second option group encountered */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Second test group options:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

 	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -R, --recursive             "
			   "descend into sub-directories\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

 	fgets (text, sizeof (text), output);
	if (strcmp (text, ("      --wibble                "
			   "bored of inventing names\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

 	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -o, --option=OPTION         "
			   "extended options\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

 	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -s, --special=SPECIAL-LONG-ARGUMENT-NAME\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

 	fgets (text, sizeof (text), output);
	if (strcmp (text, ("                              "
			   "something with special treatment\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Start of default option group encountered */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Other options:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

 	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -q, --quiet                 "
			   "reduce output to errors only\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -v, --verbose               "
			   "increase output to include informational "
			   "messages\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("      --help                  "
			   "display this help and exit\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("      --version               "
			   "output version information and exit\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Last line should be bug report address */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Report bugs to <foo@bar.com>\n")) {
		printf ("BAD: bug report line wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	fclose (output);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_parser ();
	ret |= test_count ();
	ret |= test_quiet ();
	ret |= test_verbose ();
	ret |= test_debug ();
	ret |= test_version ();
	ret |= test_help ();

	return ret;
}
