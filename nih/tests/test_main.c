/* libnih
 *
 * test_main.c - test suite for nih/main.c
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

#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/list.h>
#include <nih/main.h>
#include <nih/timer.h>


void
test_init_gettext (void)
{
	/* Check that the macro to initialise gettext sets the text domain to
	 * the PACKAGE_NAME macro, and binds that to the LOCALEDIR macro.
	 */
	TEST_FUNCTION ("nih_main_init_gettext");
	nih_main_init_gettext();

	TEST_EQ_STR (textdomain (NULL), PACKAGE_NAME);
	TEST_EQ_STR (bindtextdomain (PACKAGE_NAME, NULL), LOCALEDIR);
}

void
test_init (void)
{
	TEST_FUNCTION ("nih_main_init_full");

	/* Check that we can initialise the program with all of the arguments
	 * and that they're all copied correctly into the globals
	 */
	TEST_FEATURE ("with all arguments");
	nih_main_init_full ("argv0", "package", "version", "bugreport",
			    "copyright");

	TEST_EQ_STR (program_name, "argv0");
	TEST_EQ_STR (package_name, "package");
	TEST_EQ_STR (package_version, "version");
	TEST_EQ_STR (package_bugreport, "bugreport");
	TEST_EQ_STR (package_copyright, "copyright");


	/* Check that we can pass NULL for both the bug report address and
	 * the copyright message.
	 */
	TEST_FEATURE ("with missing arguments");
	package_bugreport = package_copyright = NULL;
	nih_main_init_full ("argv0", "package", "version", NULL, NULL);

	TEST_EQ_P (package_bugreport, NULL);
	TEST_EQ_P (package_copyright, NULL);


	/* Check that the bug report address and copyright message are set
	 * to NULL if empty strings are passed instead.
	 */
	TEST_FEATURE ("with empty arguments");
	package_bugreport = package_copyright = NULL;
	nih_main_init_full ("argv0", "package", "version", "", "");

	TEST_EQ_P (package_bugreport, NULL);
	TEST_EQ_P (package_copyright, NULL);


	/* Check that the program name contains only the basename of a
	 * full path supplied.
	 */
	TEST_FEATURE ("with full program path");
	nih_main_init_full ("/usr/bin/argv0", "package", "version",
			    "bugreport", "copyright");

	TEST_EQ_STR (program_name, "argv0");


	/* Check that the nih_main_init macro passes all the arguments for
	 * us, except the program name, which we pass.
	 */
	TEST_FUNCTION ("nih_main_init");
	nih_main_init ("argv[0]");

	TEST_EQ_STR (program_name, "argv[0]");
	TEST_EQ_STR (package_name, PACKAGE_NAME);
	TEST_EQ_STR (package_version, PACKAGE_VERSION);
	TEST_EQ_STR (package_bugreport, PACKAGE_BUGREPORT);
	TEST_EQ_STR (package_copyright, PACKAGE_COPYRIGHT);
}

void
test_package_string (void)
{
	const char *str;

	TEST_FUNCTION ("nih_package_string");

	/* Check that the package string outputs just the program name and
	 * version if the program and package names match.  If the allocation
	 * fails, the program name should be returned.
	 */
	TEST_FEATURE ("with same program and package names");
	TEST_ALLOC_FAIL {
		nih_main_init_full ("test", "test", "1.0",
				    "bugreport", "copyright");
		str = nih_main_package_string ();

		if (test_alloc_failed) {
			TEST_EQ_STR (str, "test");
			continue;
		}

		TEST_EQ_STR (str, "test 1.0");
	}


	/* Check that the package string includes the package name if it
	 * differs from the program name.
	 */
	TEST_FEATURE ("with different program and package names");
	TEST_ALLOC_FAIL {
		nih_main_init_full ("test", "wibble", "1.0",
				    "bugreport", "copyright");
		str = nih_main_package_string ();

		if (test_alloc_failed) {
			TEST_EQ_STR (str, "test");
			continue;
		}

		TEST_EQ_STR (str, "test (wibble 1.0)");
	}


	/* Check that a repeated call returns the same pointer */
	TEST_FEATURE ("with repeated call");
	nih_main_init_full ("test", "wibble", "1.0", "bugreport", "copyright");

	str = nih_main_package_string ();
	TEST_EQ_P (nih_main_package_string (), str);
}

void
test_suggest_help (void)
{
	FILE *output;

	/* Check that the message to suggest help is placed on standard
	 * error, and formatted as we expect.
	 */
	TEST_FUNCTION ("nih_main_suggest_help");
	program_name = "test";

	output = tmpfile ();
	TEST_DIVERT_STDERR (output) {
		nih_main_suggest_help ();
	}
	rewind (output);

	TEST_FILE_EQ (output, "Try `test --help' for more information.\n");
	TEST_FILE_END (output);

	fclose (output);
}

void
test_version (void)
{
	FILE *output;

	/* Check that the version message is placed on standard output,
	 * includes the package string, copyright message and GPL notice.
	 */
	TEST_FUNCTION ("nih_main_version");
	nih_main_init_full ("test", "wibble", "1.0", NULL,
			    "Copyright Message");

	TEST_ALLOC_FAIL {

		unsetenv ("COLUMNS");
		output = tmpfile ();
		TEST_DIVERT_STDOUT (output) {
			nih_main_version ();
		}
		rewind (output);

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
test_daemonise (void)
{
	pid_t pid;
	char  result[2];
	int   status, fds[2];

	/* Check that nih_main_daemonise does all of the right things,
	 * our immediate child should exit with a zero status, and the
	 * child within that should be run with a working directory of /
	 */
	TEST_FUNCTION ("nih_main_daemonise");

	pipe (fds);
	TEST_CHILD (pid) {
		char buf[80];

		program_name = "test";
		if (nih_main_daemonise () < 0)
			exit (50);

		getcwd (buf, sizeof (buf));
		if (strcmp (buf, "/")) {
			write (fds[1], "wd", 2);
			exit (10);
		}

		write (fds[1], "ok", 2);
		exit (10);
	}

	waitpid (pid, &status, 0);

	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	if (read (fds[0], result, 2) != 2)
		TEST_FAILED ("expected return code from child");

	if (! memcmp (result, "wd", 2))
		TEST_FAILED ("wrong working directory for child");

	if (memcmp (result, "ok", 2))
		TEST_FAILED ("wrong return code from child, expected 'ok' got '%.2s'",
			     result);
}


static int callback_called = 0;
static void *last_data = NULL;

static void
my_callback (void            *data,
	     NihMainLoopFunc *func)
{
	callback_called++;
	last_data = data;
}

static void
my_timeout (void *data, NihTimer *timer)
{
	nih_main_term_signal (NULL, NULL);
	nih_main_loop_exit (42);
}

void
test_main_loop (void)
{
	NihMainLoopFunc *func;
	NihTimer        *timer;
	int              ret;

	/* Check that we can run through the main loop, and that the
	 * callback function will be run.  Also schedule an immediate
	 * timeout and make sure that's run too, that'll terminate the
	 * main loop with an exit value, make sure it's returned.
	 */
	TEST_FUNCTION ("nih_main_loop");
	callback_called = 0;
	last_data = NULL;
	func = nih_main_loop_add_func (NULL, my_callback, &func);
	timer = nih_timer_add_timeout (NULL, 1, my_timeout, NULL);
	ret = nih_main_loop ();

	TEST_EQ (ret, 42);
	TEST_TRUE (callback_called);
	TEST_EQ_P (last_data, &func);

	nih_list_free (&func->entry);
}

void
test_main_loop_add_func (void)
{
	NihMainLoopFunc *func;

	/* Check that we can add a callback function to the main loop,
	 * and that the structure returned is correctly populated and
	 * placed in a list.
	 */
	TEST_FUNCTION ("nih_main_loop_add_func");
	TEST_ALLOC_FAIL {
		func = nih_main_loop_add_func (NULL, my_callback, &func);

		if (test_alloc_failed) {
			TEST_EQ_P (func, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (func, sizeof (NihMainLoopFunc));
		TEST_LIST_NOT_EMPTY (&func->entry);
		TEST_EQ_P (func->callback, my_callback);
		TEST_EQ_P (func->data, &func);

		nih_list_free (&func->entry);
	}
}


int
main (int   argc,
      char *argv[])
{
	test_init_gettext ();
	test_init ();
	test_package_string ();
	test_suggest_help ();
	test_version ();
	test_daemonise ();
	test_main_loop ();
	test_main_loop_add_func ();

	return 0;
}
