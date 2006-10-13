/* libnih
 *
 * test_command.c - test suite for nih/command.c
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

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/option.h>
#include <nih/command.h>
#include <nih/logging.h>


static int dry_run = 0;
static int wobble = 0;

static int was_called = 0;
static NihCommand *last_command = NULL;
static char *last_arg0 = NULL;
static char *last_arg1 = NULL;

static int
my_action (NihCommand *command, char * const *args)
{
	was_called++;
	last_command = nih_alloc (NULL, sizeof (NihCommand));
	memcpy (last_command, command, sizeof (NihCommand));

	if (args[0]) {
		last_arg0 = nih_strdup (NULL, args[0]);
		if (args[1]) {
			last_arg1 = nih_strdup (NULL, args[1]);
		} else {
			last_arg1 = NULL;
		}

		if (! strcmp (args[0], "fail"))
			return -1;
	} else {
		last_arg0 = NULL;
		last_arg1 = NULL;
	}

	return 0;
}


static NihOption options[] = {
	{ 'n', "dry-run", N_("simulate and output actions only"),
	  NULL, NULL, &dry_run, NULL },

	NIH_OPTION_LAST
};

static NihOption wibble_options[] = {
	{ 0, "wobble", N_("wobble file while wibbling"),
	  NULL, NULL, &wobble, NULL },

	NIH_OPTION_LAST
};

static NihCommandGroup test_group1 = { "First test group" };
static NihCommandGroup test_group2 = { "Second test group" };
static NihCommand commands[] = {
	{ "foo", NULL,
	  N_("do something fooish"),
	  NULL,
	  &test_group1, NULL,
	  my_action },

	{ "bar", N_("FILE"),
	  N_("do something barish to a file"),
	  NULL,
	  &test_group1, NULL,
	  my_action },

	{ "baz", NULL,
	  N_("do something bazish"),
	  NULL,
	  &test_group2, NULL,
	  my_action },

	{ "wibble", N_("SRC DEST"),
	  N_("wibble a file from one place to another"),
	  N_("Takes the file from SRC, wibbles it until any loose pieces "
	     "fall off, and until it reaches DEST.  SRC and DEST may not "
	     "be the same location."),
	  NULL,
	  wibble_options,
	  my_action },

	NIH_COMMAND_LAST
};


int
test_parser (void)
{
	FILE *output;
	char  text[81];
	char *argv[16], result;
	int   oldstderr, ret = 0, argc;

	printf ("Testing nih_command_parser()\n");
	program_name = "test";

	output = tmpfile ();
	oldstderr = dup (STDERR_FILENO);


	printf ("...with just command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "foo")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed no arguments */
	if (last_arg0 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_command);


	printf ("...with global option followed by command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-n";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	dry_run = 0;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* dry_run should be set */
	if (! dry_run) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "foo")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed no arguments */
	if (last_arg0 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_command);


	printf ("...with command followed by global option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "foo";
	argv[argc++] = "-n";
	argv[argc] = NULL;
	dry_run = 0;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* dry_run should be set */
	if (! dry_run) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "foo")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed no arguments */
	if (last_arg0 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_command);


	printf ("...with command followed by specific option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "wibble";
	argv[argc++] = "--wobble";
	argv[argc] = NULL;
	wobble = 0;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* wobble should be set */
	if (! wobble) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "wibble")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed no arguments */
	if (last_arg0 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_command);


	printf ("...with global option, command, then specific option\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--dry-run";
	argv[argc++] = "wibble";
	argv[argc++] = "--wobble";
	argv[argc] = NULL;
	wobble = 0;
	dry_run = 0;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* dry_run should be set */
	if (! dry_run) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* wobble should be set */
	if (! wobble) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "wibble")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed no arguments */
	if (last_arg0 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_command);


	printf ("...with terminator before command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--dry-run";
	argv[argc++] = "--";
	argv[argc++] = "wibble";
	argv[argc++] = "--wobble";
	argv[argc] = NULL;
	wobble = 0;
	dry_run = 0;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* dry_run should be set */
	if (! dry_run) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* wobble should be set */
	if (! wobble) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "wibble")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed no arguments */
	if (last_arg0 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_command);


	printf ("...with terminator before and after command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--dry-run";
	argv[argc++] = "--";
	argv[argc++] = "wibble";
	argv[argc++] = "--";
	argv[argc++] = "--wobble";
	argv[argc] = NULL;
	wobble = 0;
	dry_run = 0;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* dry_run should be set */
	if (! dry_run) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* wobble should not be set */
	if (wobble) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "wibble")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed --wobble as argument */
	if (strcmp (last_arg0, "--wobble")) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	/* Should be passed just one argument */
	if (last_arg1 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_arg0);
	nih_free (last_command);


	printf ("...with command and single argument\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "bar";
	argv[argc++] = "snarf";
	argv[argc] = NULL;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "bar")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed argument */
	if (strcmp (last_arg0, "snarf")) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	/* Should be passed just one argument */
	if (last_arg1 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_arg0);
	nih_free (last_command);


	printf ("...with command and multiple arguments\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "wibble";
	argv[argc++] = "snarf";
	argv[argc++] = "lick";
	argv[argc] = NULL;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "wibble")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed argument */
	if (strcmp (last_arg0, "snarf")) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	/* Should be passed another argument */
	if (strcmp (last_arg1, "lick")) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_arg0);
	nih_free (last_arg1);
	nih_free (last_command);


	printf ("...with global option followed by command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-n";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	dry_run = 0;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* dry_run should be set */
	if (! dry_run) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "foo")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed no arguments */
	if (last_arg0 != NULL) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_command);


	printf ("...with invalid global option before command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "-z";
	argv[argc++] = "foo";
	argv[argc] = NULL;
	was_called = 0;

	dup2 (fileno (output), STDERR_FILENO);
	result = nih_command_parser (NULL, argc, argv, options, commands);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be negative */
	if (result >= 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should not have been called */
	if (was_called) {
		printf ("BAD: action function called unexpectedly.\n");
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


	printf ("...with invalid option after command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "foo";
	argv[argc++] = "-z";
	argv[argc] = NULL;
	was_called = 0;

	dup2 (fileno (output), STDERR_FILENO);
	result = nih_command_parser (NULL, argc, argv, options, commands);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be negative */
	if (result >= 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should not have been called */
	if (was_called) {
		printf ("BAD: action function called unexpectedly.\n");
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


	printf ("...with missing command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc] = NULL;
	was_called = 0;

	dup2 (fileno (output), STDERR_FILENO);
	result = nih_command_parser (NULL, argc, argv, options, commands);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be negative */
	if (result >= 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should not have been called */
	if (was_called) {
		printf ("BAD: action function called unexpectedly.\n");
		ret = 1;
	}

	/* Output should be message with program name and newline */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: missing command\n")) {
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


	printf ("...with invalid command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "lick";
	argv[argc] = NULL;
	was_called = 0;

	dup2 (fileno (output), STDERR_FILENO);
	result = nih_command_parser (NULL, argc, argv, options, commands);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be negative */
	if (result >= 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should not have been called */
	if (was_called) {
		printf ("BAD: action function called unexpectedly.\n");
		ret = 1;
	}

	/* Output should be message with program name and newline */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: invalid command: lick\n")) {
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


	printf ("...with command in program name\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "snarf";
	argv[argc++] = "lick";
	argv[argc++] = "--wobble";
	argv[argc++] = "-n";
	argv[argc] = NULL;
	dry_run = 0;
	wobble = 0;
	was_called = 0;
	last_command = NULL;
	last_arg0 = NULL;
	last_arg1 = NULL;
	program_name = "wibble";
	result = nih_command_parser (NULL, argc, argv, options, commands);

	/* Return value should be zero */
	if (result) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* dry_run should be set */
	if (! dry_run) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* wobble should be set */
	if (! wobble) {
		printf ("BAD: option value wasn't what we expected.\n");
		ret = 1;
	}

	/* Action function should have been called */
	if (! was_called) {
		printf ("BAD: action function wasn't called.\n");
		ret = 1;
	}

	/* Should have been called with command */
	if (strcmp (last_command->command, "wibble")) {
		printf ("BAD: command wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be passed argument */
	if (strcmp (last_arg0, "snarf")) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	/* Should be passed another argument */
	if (strcmp (last_arg1, "lick")) {
		printf ("BAD: arguments weren't what we expected.\n");
		ret = 1;
	}

	nih_free (last_arg0);
	nih_free (last_arg1);
	nih_free (last_command);


	fclose (output);
	close (oldstderr);

	return ret;
}


int
test_help (void)
{
	FILE  *output;
	char   text[100];
	char  *argv[3];
	pid_t  pid;
	int    ret = 0, argc, status;

	printf ("Testing nih_command_help()\n");
	program_name = "test";
	package_bugreport = "foo@bar.com";

	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "help";
	argv[argc] = NULL;

	output = tmpfile ();
	pid = fork ();
	if (pid == 0) {
		unsetenv ("COLUMNS");

		dup2 (fileno (output), STDOUT_FILENO);
		nih_command_parser (NULL, argc, argv, options, commands);
		exit (1);
	}

	waitpid (pid, &status, 0);
	rewind (output);

	/* Should have exited normally */
	if ((! WIFEXITED (status)) || (WEXITSTATUS (status) != 0)) {
		printf ("BAD: process did not exit normally.\n");
		ret = 1;
	}

	/* First line of output should be first group header */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "First test group commands:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  foo                         "
			   "do something fooish\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  bar                         "
			   "do something barish to a file\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Next line of output should be second group header */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Second test group commands:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  baz                         "
			   "do something bazish\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Next line of output should be other group header */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Other commands:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  wibble                      "
			   "wibble a file from one place to another\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  help                        "
			   "display list of commands\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Next line of output should be the footer */
	fgets (text, sizeof (text), output);
	if (strcmp (text, ("For more information on a command, try "
			   "`test COMMAND --help'.\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	fclose (output);


	printf ("Testing nih_option_help()\n");

	printf ("...with no command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--help";
	argv[argc] = NULL;

	nih_option_set_synopsis ("This is my program");
	nih_option_set_help ("Some help text");

	output = tmpfile ();
	pid = fork ();
	if (pid == 0) {
		unsetenv ("COLUMNS");

		dup2 (fileno (output), STDOUT_FILENO);
		nih_command_parser (NULL, argc, argv, options, commands);
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
	if (strcmp (text, ("Usage: test [OPTION]... "
			   "COMMAND [OPTION]... [ARG]...\n"))) {
		printf ("BAD: usage line wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be the synopsis */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "This is my program\n")) {
		printf ("BAD: synopsis line wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Start of option group encountered */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Options:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -n, --dry-run               "
			   "simulate and output actions only\n"))) {
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

	/* Help text should now appear */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Some help text\n")) {
		printf ("BAD: help string wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Footer text should now appear */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "For a list of commands, try `test help'.\n")) {
		printf ("BAD: footer string wasn't what we expected.\n");
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


	printf ("...with a command\n");
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "wibble";
	argv[argc++] = "--help";
	argv[argc] = NULL;

	output = tmpfile ();
	pid = fork ();
	if (pid == 0) {
		unsetenv ("COLUMNS");

		dup2 (fileno (output), STDOUT_FILENO);
		nih_command_parser (NULL, argc, argv, options, commands);
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
	if (strcmp (text, "Usage: test wibble [OPTION]... SRC DEST\n")) {
		printf ("BAD: usage line wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be the synopsis */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "wibble a file from one place to another\n")) {
		printf ("BAD: synopsis line wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Start of option group encountered */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Options:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("      --wobble                "
			   "wobble file while wibbling\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -n, --dry-run               "
			   "simulate and output actions only\n"))) {
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

	/* Help text should now begin */
	fgets (text, sizeof (text), output);
	if (strcmp (text, ("Takes the file from SRC, wibbles it until any "
			   "loose pieces fall off, and until\n"))) {
		printf ("BAD: help string wasn't what we expected.\n");
		ret = 1;
	}

	/* Help text should finish */
	fgets (text, sizeof (text), output);
	if (strcmp (text, ("it reaches DEST.  SRC and DEST may not be the "
			   "same location.\n"))) {
		printf ("BAD: help string wasn't what we expected.\n");
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


	printf ("...with command in program_name\n");
	program_name = "wibble";
	argc = 0;
	argv[argc++] = "ignored";
	argv[argc++] = "--help";
	argv[argc] = NULL;

	output = tmpfile ();
	pid = fork ();
	if (pid == 0) {
		unsetenv ("COLUMNS");

		dup2 (fileno (output), STDOUT_FILENO);
		nih_command_parser (NULL, argc, argv, options, commands);
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
	if (strcmp (text, "Usage: wibble [OPTION]... SRC DEST\n")) {
		printf ("BAD: usage line wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be the synopsis */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "wibble a file from one place to another\n")) {
		printf ("BAD: synopsis line wasn't what we expected.\n");
		ret = 1;
	}

	/* Next line of output should be a blank line */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}


	/* Start of option group encountered */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "Options:\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("      --wobble                "
			   "wobble file while wibbling\n"))) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, ("  -n, --dry-run               "
			   "simulate and output actions only\n"))) {
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

	/* Help text should now begin */
	fgets (text, sizeof (text), output);
	if (strcmp (text, ("Takes the file from SRC, wibbles it until any "
			   "loose pieces fall off, and until\n"))) {
		printf ("BAD: help string wasn't what we expected.\n");
		ret = 1;
	}

	/* Help text should finish */
	fgets (text, sizeof (text), output);
	if (strcmp (text, ("it reaches DEST.  SRC and DEST may not be the "
			   "same location.\n"))) {
		printf ("BAD: help string wasn't what we expected.\n");
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
	ret |= test_help ();

	return ret;
}
