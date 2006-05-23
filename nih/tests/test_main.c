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


#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <nih/main.h>


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
	program_name = "test";
	package_name = "test";
	package_version = "1.0";

	printf ("...with same program and package names\n");
	str = nih_main_package_string ();

	if (strcmp (str, "test 1.0")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}


	printf("...with different program and package names\n");
	package_name = "wibble";
	str = nih_main_package_string ();

	if (strcmp (str, "test (wibble 1.0)")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}


	printf ("...with full path to program and equivalent package name\n");
	program_name = "/usr/bin/test";
	package_name = "test";
	str = nih_main_package_string ();

	if (strcmp (str, "test 1.0")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}


	printf ("...with full path to program and different package name\n");
	program_name = "/usr/bin/test";
	package_name = "wibble";
	str = nih_main_package_string ();

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
	fgets (text, sizeof (text), output);

	if (strcmp (text, "Try `test --help' for more information.\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	fclose (output);

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

	dup2 (fileno (output), STDOUT_FILENO);
	nih_main_version ();
	dup2 (oldstdout, STDOUT_FILENO);

	rewind (output);

	fgets (text, sizeof (text), output);
	if (strcmp (text, "test (wibble 1.0)\n")) {
		printf ("BAD: package line wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, "Copyright Message\n")) {
		printf ("BAD: copyright line wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strncmp (text, "This is free software;", 22)) {
		printf ("BAD: first licence line wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strncmp (text, "warranty; not even for", 22)) {
		printf ("BAD: second licence line wasn't what we expected.\n");
		ret = 1;
	}

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

	ret |= test_init ();
	ret |= test_package_string ();
	ret |= test_suggest_help ();
	ret |= test_version ();

	return ret;
}
