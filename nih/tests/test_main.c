/* libnih
 *
 * test_main.c - test suite for nih/main.c
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


#include <sys/wait.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nih/alloc.h>
#include <nih/main.h>
#include <nih/timer.h>


int
test_init_gettext (void)
{
	char *ptr;
	int   ret = 0;

	printf ("Testing nih_main_init_gettext()\n");
	nih_main_init_gettext();

	/* PACKAGE_NAME should be bound to LOCALEDIR */
	ptr = bindtextdomain (PACKAGE_NAME, NULL);
	if (strcmp (ptr, LOCALEDIR)) {
		printf ("BAD: text domain not bound to where we expected.\n");
		ret = 1;
	}

	/* Text domain should be PACKAGENAME */
	ptr = textdomain (NULL);
	if (strcmp (ptr, PACKAGE_NAME)) {
		printf ("BAD: text domain wasn't what we expected.\n");
		ret = 1;
	}

	return ret;
}

int
test_init (void)
{
	int ret = 0;

	printf ("Testing nih_main_init_full()\n");

	printf ("...with all arguments\n");
	nih_main_init_full ("argv0", "package", "version", "bugreport",
			    "copyright");

	/* Program name should be first argument */
	if (strcmp (program_name, "argv0")) {
		printf ("BAD: program_name set incorrectly.\n");
		ret = 1;
	}

	/* Package name should be second argument */
	if (strcmp (package_name, "package")) {
		printf ("BAD: package_name set incorrectly.\n");
		ret = 1;
	}

	/* Package version should be third argument */
	if (strcmp (package_version, "version")) {
		printf ("BAD: package_version set incorrectly.\n");
		ret = 1;
	}

	/* Package bug report address should be fourth argument */
	if (strcmp (package_bugreport, "bugreport")) {
		printf ("BAD: package_bugreport set incorrectly.\n");
		ret = 1;
	}

	/* Package copyright should be fifth argument */
	if (strcmp (package_copyright, "copyright")) {
		printf ("BAD: package_copyright set incorrectly.\n");
		ret = 1;
	}


	printf ("...with missing arguments\n");
	package_bugreport = package_copyright = NULL;
	nih_main_init_full ("argv0", "package", "version", NULL, NULL);

	/* Package bug report address should be NULL */
	if (package_bugreport) {
		printf ("BAD: package_bugreport changed unexpectedly.\n");
		ret = 1;
	}

	/* Package copyright should be NULL */
	if (package_copyright) {
		printf ("BAD: package_copyright changed unexpectedly.\n");
		ret = 1;
	}


	printf ("...with empty arguments\n");
	package_bugreport = package_copyright = NULL;
	nih_main_init_full ("argv0", "package", "version", "", "");

	/* Package bug report address should be NULL */
	if (package_bugreport) {
		printf ("BAD: package_bugreport changed unexpectedly.\n");
		ret = 1;
	}

	/* Package copyright should be NULL */
	if (package_copyright) {
		printf ("BAD: package_copyright changed unexpectedly.\n");
		ret = 1;
	}

	printf ("...with full program path\n");
	nih_main_init_full ("/usr/bin/argv0", "package", "version",
			    "bugreport", "copyright");

	/* Program name should be basename */
	if (strcmp (program_name, "argv0")) {
		printf ("BAD: program_name set incorrectly.\n");
		ret = 1;
	}


	printf ("Testing nih_main_init()\n");
	nih_main_init ("argv[0]");

	/* Program name should be only argument */
	if (strcmp (program_name, "argv[0]")) {
		printf ("BAD: program_name set incorrectly.\n");
		ret = 1;
	}

	/* Package name should be PACKAGE_NAME */
	if (strcmp (package_name, PACKAGE_NAME)) {
		printf ("BAD: package_name set incorrectly.\n");
		ret = 1;
	}

	/* Package version should be PACKAGE_VERSION */
	if (strcmp (package_version, PACKAGE_VERSION)) {
		printf ("BAD: package_version set incorrectly.\n");
		ret = 1;
	}

	/* Package bug report address should be PACKAGE_BUGREPORT */
	if (strcmp (package_bugreport, PACKAGE_BUGREPORT)) {
		printf ("BAD: package_bugreport set incorrectly.\n");
		ret = 1;
	}

	/* Package copyright should be PACKAGE_COPYRIGHT */
	if (strcmp (package_copyright, PACKAGE_COPYRIGHT)) {
		printf ("BAD: package_copyright set incorrectly.\n");
		ret = 1;
	}


	return ret;
}

int
test_package_string (void)
{
	const char *str;
	int         ret = 0;

	printf ("Testing nih_package_string()\n");
	nih_main_init_full ("test", "test", "1.0", "bugreport", "copyright");

	printf ("...with same program and package names\n");
	str = nih_main_package_string ();

	/* String should be just package name and version */
	if (strcmp (str, "test 1.0")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}


	printf ("...with different program and package names\n");
	nih_main_init_full ("test", "wibble", "1.0", "bugreport", "copyright");
	str = nih_main_package_string ();

	/* String should be program name as well as package name and version */
	if (strcmp (str, "test (wibble 1.0)")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	return ret;
}

int
test_suggest_help (void)
{
	FILE *output;
	char  text[81];
	int   oldstderr, ret = 0;

	printf ("Testing nih_main_suggest_help\n");
	program_name = "test";

	output = tmpfile ();
	oldstderr = dup (STDERR_FILENO);

	dup2 (fileno (output), STDERR_FILENO);
	nih_main_suggest_help ();
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Output should be message with program name and newline */
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

	fclose (output);
	close (oldstderr);

	return ret;
}

int
test_version (void)
{
	FILE *output;
	char  text[81];
	int   oldstdout, ret = 0;

	printf ("Testing nih_main_version\n");
	program_name = "test";
	package_name = "wibble";
	package_version = "1.0";
	package_copyright = "Copyright Message";

	output = tmpfile ();
	oldstdout = dup (STDOUT_FILENO);
	unsetenv ("COLUMNS");

	dup2 (fileno (output), STDOUT_FILENO);
	nih_main_version ();
	dup2 (oldstdout, STDOUT_FILENO);

	rewind (output);

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
	close (oldstdout);

	return ret;
}


int
test_daemonise (void)
{
	pid_t pid;
	char  result[2];
	int   ret = 0, status, fds[2];

	printf ("Testing nih_main_daemonise\n");
	assert (pipe (fds) == 0);

	assert ((pid = fork ()) >= 0);
	if (pid == 0) {
		char buf[80];

		program_name = "test";
		if (nih_main_daemonise () < 0)
			exit (50);

		/* Working directory should be / */
		getcwd (buf, sizeof (buf));
		if (strcmp (buf, "/")) {
			write (fds[1], "wd", 2);
			exit (0);
		}

		write (fds[1], "ok", 2);
		exit (0);
	}

	assert (waitpid (pid, &status, 0) > 0);

	/* Child process should exit 0 */
	if ((! WIFEXITED (status)) || (WEXITSTATUS (status) != 0)) {
		printf ("BAD: exit status wasn't what we expected.\n");
		ret = 1;
	}

	/* Check we got a result code */
	if (read (fds[0], result, 2) != 2) {
		printf ("BAD: result code wasn't what we expected.\n");
		ret = 1;
	}

	/* Working directory should be / */
	if (! memcmp (result, "wd", 2)) {
		printf ("BAD: working directory wasn't what we expected.\n");
		ret = 1;
	}

	/* Check the child process worked */
	if (memcmp (result, "ok", 2)) {
		printf ("BAD: unknown result code.\n");
		ret = 1;
	}

	return ret;
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

int
test_main_loop (void)
{
	NihMainLoopFunc *func;
	int              ret = 0, retval;

	printf ("Testing nih_main_loop()\n");
	callback_called = 0;
	last_data = NULL;
	func = nih_main_loop_add_func (NULL, my_callback, &ret);
	nih_timer_add_timeout (NULL, 1, my_timeout, NULL);
	retval = nih_main_loop ();

	/* Return value should be that injected by the timer */
	if (retval != 42) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Callback should have been called at least once */
	if (! callback_called) {
		printf ("BAD: loop function wasn't called.\n");
		ret = 1;
	}

	/* Callback should have been passed data pointer */
	if (last_data != &ret) {
		printf ("BAD: callback data wasn't what we expected.\n");
		ret = 1;
	}

	nih_list_free (&func->entry);

	return ret;
}


int
test_main_loop_add_func (void)
{
	NihMainLoopFunc *func;
	int              ret = 0;

	printf ("Testing nih_main_loop_add_func()\n");
	func = nih_main_loop_add_func (NULL, my_callback, &ret);

	/* Callback should be function given */
	if (func->callback != my_callback) {
		printf ("BAD: callback function set incorrectly.\n");
		ret = 1;
	}

	/* Callback data should be pointer given */
	if (func->data != &ret) {
		printf ("BAD: callback data set incorrectly.\n");
		ret = 1;
	}

	/* Should be in the loop functions list */
	if (NIH_LIST_EMPTY (&func->entry)) {
		printf ("BAD: not placed into loop functions list.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (nih_alloc_size (func) != sizeof (NihMainLoopFunc)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	nih_list_free (&func->entry);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_init_gettext ();
	ret |= test_init ();
	ret |= test_package_string ();
	ret |= test_suggest_help ();
	ret |= test_version ();
	ret |= test_daemonise ();
	ret |= test_main_loop ();
	ret |= test_main_loop_add_func ();

	return ret;
}
