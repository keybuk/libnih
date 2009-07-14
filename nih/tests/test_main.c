/* libnih
 *
 * test_main.c - test suite for nih/main.c
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
#include <sys/wait.h>

#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/list.h>
#include <nih/main.h>
#include <nih/timer.h>
#include <nih/error.h>


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
	 * and that they're all copied correctly into the globals.  When the
	 * program and package names are the same, the package string should
	 * only contain one copy.
	 */
	TEST_FEATURE ("with same program and package names");
	TEST_ALLOC_FAIL {
		nih_main_init_full ("test", "test", "1.0",
				    "bugreport", "copyright");

		TEST_EQ_STR (program_name, "test");
		TEST_EQ_STR (package_name, "test");
		TEST_EQ_STR (package_version, "1.0");
		TEST_EQ_STR (package_bugreport, "bugreport");
		TEST_EQ_STR (package_copyright, "copyright");

		TEST_EQ_STR (package_string, "test 1.0");
	}


	/* Check that when the program and package names differ, the
	 * package string contains both.
	 */
	TEST_FEATURE ("with different program and package names");
	TEST_ALLOC_FAIL {
		nih_main_init_full ("test", "wibble", "1.0",
				    "bugreport", "copyright");

		TEST_EQ_STR (program_name, "test");
		TEST_EQ_STR (package_name, "wibble");
		TEST_EQ_STR (package_version, "1.0");
		TEST_EQ_STR (package_bugreport, "bugreport");
		TEST_EQ_STR (package_copyright, "copyright");

		TEST_EQ_STR (package_string, "test (wibble 1.0)");
	}


	/* Check that we can pass NULL for both the bug report address and
	 * the copyright message.
	 */
	TEST_FEATURE ("with missing arguments");
	package_bugreport = package_copyright = NULL;
	nih_main_init_full ("argv0", "package", "1.0", NULL, NULL);

	TEST_EQ_P (package_bugreport, NULL);
	TEST_EQ_P (package_copyright, NULL);


	/* Check that the bug report address and copyright message are set
	 * to NULL if empty strings are passed instead.
	 */
	TEST_FEATURE ("with empty arguments");
	package_bugreport = package_copyright = NULL;
	nih_main_init_full ("argv0", "package", "1.0", "", "");

	TEST_EQ_P (package_bugreport, NULL);
	TEST_EQ_P (package_copyright, NULL);


	/* Check that the program name contains only the basename of a
	 * full path supplied, and this is replicated into the package
	 * string.
	 */
	TEST_FEATURE ("with full program path");
	TEST_ALLOC_FAIL {
		nih_main_init_full ("/usr/bin/argv0", "package", "1.0",
				    "bugreport", "copyright");

		TEST_EQ_STR (program_name, "argv0");
		TEST_EQ_STR (package_name, "package");

		TEST_EQ_STR (package_string, "argv0 (package 1.0)");
	}


	/* Check that the program name contains only the actual name
	 * of the program when it's supplied as a login shell path
	 * (prefixed with a dash).
	 */
	TEST_FEATURE ("with login shell path");
	TEST_ALLOC_FAIL {
		nih_main_init_full ("-argv0", "package", "1.0",
				    "bugreport", "copyright");

		TEST_EQ_STR (program_name, "argv0");
		TEST_EQ_STR (package_name, "package");

		TEST_EQ_STR (package_string, "argv0 (package 1.0)");
	}


	/* Check that the nih_main_init macro passes all the arguments for
	 * us, except the program name, which we pass.
	 */
	TEST_FUNCTION ("nih_main_init");
	TEST_ALLOC_FAIL {
		nih_main_init ("argv[0]");

		TEST_EQ_STR (program_name, "argv[0]");
		TEST_EQ_STR (package_name, PACKAGE_NAME);
		TEST_EQ_STR (package_version, PACKAGE_VERSION);
		TEST_EQ_STR (package_bugreport, PACKAGE_BUGREPORT);
		TEST_EQ_STR (package_copyright, PACKAGE_COPYRIGHT);
	}
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

	assert0 (pipe (fds));
	TEST_CHILD (pid) {
		char buf[80];
		int  fd;

		program_name = "test";
		fd = open ("/dev/null", O_WRONLY);
		assert (fd >= 0);
		assert (dup2 (fd, STDERR_FILENO) >= 0);
		assert0 (close (fd));

		if (nih_main_daemonise () < 0)
			exit (50);

		assert (getcwd (buf, sizeof (buf)));
		if (strcmp (buf, "/")) {
			assert (write (fds[1], "wd", 2) == 2);
			exit (10);
		}

		assert (write (fds[1], "ok", 2) == 2);
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


void
test_set_pidfile (void)
{
	const char *filename, *ptr;

	TEST_FUNCTION ("nih_main_set_pidfile");
	program_name = "test";

	/* Check that we can set a pidfile for use, and have the string
	 * copied and returned.
	 */
	TEST_FEATURE ("with new location");
	filename = "/path/to/pid";
	nih_main_set_pidfile (filename);

	ptr = nih_main_get_pidfile ();
	TEST_EQ_STR (ptr, filename);
	TEST_NE_P (ptr, filename);


	/* Check that we can pass NULL to have the default location set
	 * instead.
	 */
	TEST_FEATURE ("with default location");
	nih_main_set_pidfile (NULL);

	ptr = nih_main_get_pidfile ();
	TEST_EQ_STR (ptr, "/var/run/test.pid");


	nih_main_set_pidfile (NULL);
}

void
test_read_pidfile (void)
{
	FILE *f;
	char  filename[PATH_MAX];

	TEST_FUNCTION ("nih_main_read_pidfile");
	TEST_FILENAME (filename);
	nih_main_set_pidfile (filename);

	/* Check that reading from a valid pid file will return the pid
	 * stored there.
	 */
	TEST_FEATURE ("with valid pid file");
	f = fopen (filename, "w");
	fprintf (f, "1234\n");
	fclose (f);

	TEST_EQ (nih_main_read_pidfile (), 1234);


	/* Check that reading from a pid file without a newline will still
	 * return the pid stored there.
	 */
	TEST_FEATURE ("with no newline in pid file");
	f = fopen (filename, "w");
	fprintf (f, "1234");
	fclose (f);

	TEST_EQ (nih_main_read_pidfile (), 1234);


	/* Check that reading from an invalid pid file returns -1. */
	TEST_FEATURE ("with invalid pid file");
	f = fopen (filename, "w");
	fprintf (f, "foo\n1234\n");
	fclose (f);

	TEST_EQ (nih_main_read_pidfile (), -1);


	/* Check that reading from a non-existant pid file returns -1. */
	TEST_FEATURE ("with non-existant pid file");
	nih_main_unlink_pidfile ();

	TEST_EQ (nih_main_read_pidfile (), -1);


	nih_main_set_pidfile (NULL);
}

void
test_write_pidfile (void)
{
	FILE     *f;
	NihError *err;
	char      dirname[PATH_MAX], filename[PATH_MAX], tmpname[PATH_MAX];
	int       ret;

	TEST_FUNCTION ("nih_main_write_pidfile");
	TEST_FILENAME (dirname);
	mkdir (dirname, 0755);

	strcpy (filename, dirname);
	strcat (filename, "/test.pid");

	strcpy (tmpname, dirname);
	strcat (tmpname, "/.test.pid.tmp");

	nih_main_set_pidfile (filename);

	/* Check that we can write a pid to the file, and have it appaer
	 * on disk where we expect.
	 */
	TEST_FEATURE ("with successful write");
	ret = nih_main_write_pidfile (1234);

	TEST_EQ (ret, 0);

	f = fopen (filename, "r");
	TEST_FILE_EQ (f, "1234\n");
	fclose (f);


	/* Check that we can overwrite an existing pid file with a new
	 * value.
	 */
	TEST_FEATURE ("with overwrite of existing pid");
	ret = nih_main_write_pidfile (5678);

	TEST_EQ (ret, 0);

	f = fopen (filename, "r");
	TEST_FILE_EQ (f, "5678\n");
	fclose (f);


	/* Check that an error writing to the temporary file does not result
	 * in the replacement of the existing file and does not result in
	 * the unlinking of the temporary file.
	 */
	TEST_FEATURE ("with failure to write to temporary file");
	f = fopen (tmpname, "w");
	fclose (f);
	chmod (tmpname, 0000);

	ret = nih_main_write_pidfile (1234);

	TEST_LT (ret, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, EACCES);
	nih_free (err);

	f = fopen (filename, "r");
	TEST_FILE_EQ (f, "5678\n");
	fclose (f);

	TEST_EQ (chmod (tmpname, 0644), 0);


	nih_main_unlink_pidfile ();
	unlink (tmpname);
	rmdir (dirname);

	nih_main_set_pidfile (NULL);
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

	nih_free (func);
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

		nih_free (func);
	}
}


int
main (int   argc,
      char *argv[])
{
	test_init_gettext ();
	test_init ();
	test_suggest_help ();
	test_version ();
	test_daemonise ();
	test_set_pidfile ();
	test_read_pidfile ();
	test_write_pidfile ();
	test_main_loop ();
	test_main_loop_add_func ();

	return 0;
}
