/* libnih
 *
 * test_command.c - test suite for nih/command.c
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
#include <nih/string.h>
#include <nih/main.h>
#include <nih/option.h>
#include <nih/command.h>


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

	last_command = malloc (sizeof (NihCommand));
	memcpy (last_command, command, sizeof (NihCommand));

	if (args[0]) {
		last_arg0 = strdup (args[0]);
		if (args[1]) {
			last_arg1 = strdup (args[1]);
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

static NihCommand commands2[] = {
	{ "really-overly-long-command-name", NULL,
	  N_("does something irrelevant, and the synopsis is long enough to "
	     "wrap across multiple lines"),
	  NULL,
	  NULL,
	  NULL,
	  NULL },

	{ "hidden", NULL, NULL, NULL, NULL, NULL, NULL },

	NIH_COMMAND_LAST
};

void
test_parser (void)
{
	FILE *output;
	char *argv[16];
	int   argc, ret = 0;

	TEST_FUNCTION ("nih_command_parser");
	program_name = "test";
	output = tmpfile ();


	/* Check that the command parser calls the command function, and
	 * when there are no arguments, just passes in a NULL array.
	 */
	TEST_FEATURE ("with just command");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc] = NULL;

		was_called = 0;
		last_command = NULL;
		last_arg0 = NULL;
		last_arg1 = NULL;

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "foo");
		TEST_EQ_P (last_arg0, NULL);

		free (last_command);
	}


	/* Check that a global option that appears before a command is
	 * honoured.
	 */
	TEST_FEATURE ("with global option followed by command");
	TEST_ALLOC_FAIL {
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

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (dry_run);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "foo");
		TEST_EQ_P (last_arg0, NULL);

		free (last_command);
	}


	/* Check that a global option that appears after a command is
	 * still honoured, despite not being in the command's own options.
	 */
	TEST_FEATURE ("with command followed by global option");
	TEST_ALLOC_FAIL {
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

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (dry_run);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "foo");
		TEST_EQ_P (last_arg0, NULL);

		free (last_command);
	}


	/* Check that a command's own options are also honoured. */
	TEST_FEATURE ("with command followed by specific option");
	TEST_ALLOC_FAIL {
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

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (wobble);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "wibble");
		TEST_EQ_P (last_arg0, NULL);

		free (last_command);
	}


	/* Check that global options and command-specific options can be
	 * both given at once.
	 */
	TEST_FEATURE ("with global option, command, then specific option");
	TEST_ALLOC_FAIL {
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

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (wobble);
		TEST_TRUE (dry_run);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "wibble");
		TEST_EQ_P (last_arg0, NULL);

		free (last_command);
	}


	/* Check that a double-dash terminator may appear before a command,
	 * which only terminates the global options, not the command-specific
	 * ones.
	 */
	TEST_FEATURE ("with terminator before command");
	TEST_ALLOC_FAIL {
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

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (wobble);
		TEST_TRUE (dry_run);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "wibble");
		TEST_EQ_P (last_arg0, NULL);

		free (last_command);
	}


	/* Check that a double-dash terminator may appear after a command,
	 * which terminates the option processing for that command as well.
	 * Any option-like argument is passed to the function as an ordinary
	 * argument in the array.
	 */
	TEST_FEATURE ("with terminator before and after command");
	TEST_ALLOC_FAIL {
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

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_FALSE (wobble);
		TEST_TRUE (dry_run);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "wibble");
		TEST_EQ_STR (last_arg0, "--wobble");
		TEST_EQ_P (last_arg1, NULL);

		free (last_arg0);
		free (last_command);
	}


	/* Check that non-option arguments may follow a command, they're
	 * collected and passed to the function in a NULL-terminated array.
	 */
	TEST_FEATURE ("with command and single argument");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "bar";
		argv[argc++] = "snarf";
		argv[argc] = NULL;

		was_called = 0;
		last_command = NULL;
		last_arg0 = NULL;
		last_arg1 = NULL;

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "bar");
		TEST_EQ_STR (last_arg0, "snarf");
		TEST_EQ_P (last_arg1, NULL);

		free (last_arg0);
		free (last_command);
	}


	/* Check that multiple arguments after the command are all passed
	 * in the array.
	 */
	TEST_FEATURE ("with command and multiple arguments");
	TEST_ALLOC_FAIL {
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

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "wibble");
		TEST_EQ_STR (last_arg0, "snarf");
		TEST_EQ_STR (last_arg1, "lick");

		free (last_arg0);
		free (last_arg1);
		free (last_command);
	}


	/* Check that an invalid global option appearing results in the
	 * parser returning a negative number and outputting an error
	 * message to stderr with a suggestion about help.
	 */
	TEST_FEATURE ("with invalid global option before command");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "-z";
		argv[argc++] = "foo";
		argv[argc] = NULL;

		was_called = 0;

		TEST_DIVERT_STDERR (output) {
			ret = nih_command_parser (NULL, argc, argv,
						  options, commands);
		}
		rewind (output);

		TEST_LT (ret, 0);
		TEST_FALSE (was_called);

		TEST_FILE_EQ (output, "test: invalid option: -z\n");
		TEST_FILE_EQ (output,
			      "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that an invalid option appearing after the command also
	 * results in the parser returning an error without running the
	 * command function.
	 */
	TEST_FEATURE ("with invalid option after command");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "foo";
		argv[argc++] = "-z";
		argv[argc] = NULL;

		was_called = 0;

		TEST_DIVERT_STDERR (output) {
			ret = nih_command_parser (NULL, argc, argv,
						  options, commands);
		}
		rewind (output);

		TEST_LT (ret, 0);
		TEST_FALSE (was_called);

		TEST_FILE_EQ (output, "test: invalid option: -z\n");
		TEST_FILE_EQ (output, "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that a missing command entirely results in the parser
	 * terminating with an error and outputting a message.
	 */
	TEST_FEATURE ("with missing command");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc] = NULL;

		was_called = 0;

		TEST_DIVERT_STDERR (output) {
			ret = nih_command_parser (NULL, argc, argv,
						  options, commands);
		}
		rewind (output);

		TEST_LT (ret, 0);
		TEST_FALSE (was_called);

		TEST_FILE_EQ (output, "test: missing command\n");
		TEST_FILE_EQ (output,
			      "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that an invalid command results in the parser returning
	 * an error and outputting a message.
	 */
	TEST_FEATURE ("with invalid command");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "lick";
		argv[argc] = NULL;

		was_called = 0;

		TEST_DIVERT_STDERR (output) {
			ret = nih_command_parser (NULL, argc, argv,
						  options, commands);
		}
		rewind (output);

		TEST_LT (ret, 0);
		TEST_FALSE (was_called);

		TEST_FILE_EQ (output, "test: invalid command: lick\n");
		TEST_FILE_EQ (output,
			      "Try `test --help' for more information.\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that the command may appear in the program name instead,
	 * in which case all arguments are used including the first, and
	 * all options considered to be both global and command options.
	 */
	TEST_FEATURE ("with command in program name");
	TEST_ALLOC_FAIL {
		program_name = "wibble";

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

		ret = nih_command_parser (NULL, argc, argv, options, commands);

		TEST_EQ (ret, 0);
		TEST_TRUE (dry_run);
		TEST_TRUE (wobble);
		TEST_TRUE (was_called);
		TEST_EQ_STR (last_command->command, "wibble");
		TEST_EQ_STR (last_arg0, "snarf");
		TEST_EQ_STR (last_arg1, "lick");

		free (last_arg0);
		free (last_arg1);
		free (last_command);
	}


	fclose (output);
}


void
test_help (void)
{
	FILE  *output;
	char  *argv[4];
	pid_t  pid;
	int    argc, status;

	TEST_FUNCTION ("nih_command_help");
	output = tmpfile ();

	/* Check that we can obtain a list of command using the "help"
	 * command; which terminates the process with exit code 0.  The
	 * output should be grouped according to the command group, and
	 * each command indented with the text alongside and wrapped.
	 */
	TEST_FEATURE ("with multiple groups");
	nih_main_init_full ("test", "wibble", "1.0",
			    "foo@bar.com", "Copyright Message");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "help";
		argv[argc] = NULL;

		TEST_CHILD (pid) {
			unsetenv ("COLUMNS");

			TEST_DIVERT_STDOUT (output) {
				nih_command_parser (NULL, argc, argv,
						    options, commands);
				exit (1);
			}
		}

		waitpid (pid, &status, 0);
		rewind (output);

		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_FILE_EQ (output, "First test group commands:\n");
		TEST_FILE_EQ (output, ("  foo                         "
				       "do something fooish\n"));
		TEST_FILE_EQ (output, ("  bar                         "
				       "do something barish to a file\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Second test group commands:\n");
		TEST_FILE_EQ (output, ("  baz                         "
				       "do something bazish\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Other commands:\n");
		TEST_FILE_EQ (output, ("  wibble                      "
				       "wibble a file from one place to "
				       "another\n"));
		TEST_FILE_EQ (output, ("  help                        "
				       "display list of commands\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, ("For more information on a command, "
				       "try `test COMMAND --help'.\n"));
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that if there's only a single group, the title is different;
	 * also check that an overly long command name is wrapped properly,
	 * synopsis is wrapped to multiple lines and a command without a
	 * synopsis is not output at all.
	 */
	TEST_FEATURE ("with single group and long name");
	nih_main_init_full ("test", "wibble", "1.0",
			    "foo@bar.com", "Copyright Message");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "help";
		argv[argc] = NULL;

		TEST_CHILD (pid) {
			unsetenv ("COLUMNS");

			TEST_DIVERT_STDOUT (output) {
				nih_command_parser (NULL, argc, argv,
						    options, commands2);
				exit (1);
			}
		}

		waitpid (pid, &status, 0);
		rewind (output);

		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_FILE_EQ (output, "Commands:\n");
		TEST_FILE_EQ (output, "  really-overly-long-command-name\n");
		TEST_FILE_EQ (output, ("                              "
				       "does something irrelevant, and the "
				       "synopsis is\n"));
		TEST_FILE_EQ (output, ("                                "
				       "long enough to wrap across multiple "
				       "lines\n"));
		TEST_FILE_EQ (output, ("  help                        "
				       "display list of commands\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, ("For more information on a command, "
				       "try `test COMMAND --help'.\n"));
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that the command functions sufficiently wrap the
	 * nih_option_help function such that we can obtain help for the
	 * program as a whole and get a message saying how to see the
	 * commands list.
	 */
	TEST_FUNCTION ("nih_option_help");

	TEST_FEATURE ("with no command");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--help";
		argv[argc] = NULL;

		nih_option_set_synopsis ("This is my program");
		nih_option_set_help ("Some help text");

		TEST_CHILD (pid) {
			unsetenv ("COLUMNS");

			TEST_DIVERT_STDOUT (output) {
				nih_command_parser (NULL, argc, argv,
						    options, commands);
				exit (1);
			}
		}

		waitpid (pid, &status, 0);
		rewind (output);

		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_FILE_EQ (output, ("Usage: test [OPTION]... "
				       "COMMAND [OPTION]... [ARG]...\n"));
		TEST_FILE_EQ (output, "This is my program\n");
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Options:\n");
		TEST_FILE_EQ (output, ("  -n, --dry-run               "
				       "simulate and output actions only\n"));
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
		TEST_FILE_EQ (output, "Some help text\n");
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, ("For a list of commands, "
				       "try `test help'.\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Report bugs to <foo@bar.com>\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that the wrapping is sufficient that following a command
	 * with the --help option outputs help for that option, including
	 * the global options in the list.
	 */
	TEST_FEATURE ("with a command");
	TEST_ALLOC_FAIL {
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "wibble";
		argv[argc++] = "--help";
		argv[argc] = NULL;

		TEST_CHILD (pid) {
			unsetenv ("COLUMNS");

			TEST_DIVERT_STDOUT (output) {
				nih_command_parser (NULL, argc, argv,
						    options, commands);
				exit (1);
			}
		}

		waitpid (pid, &status, 0);
		rewind (output);

		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_FILE_EQ (output, ("Usage: test wibble [OPTION]... "
				       "SRC DEST\n"));
		TEST_FILE_EQ (output, ("wibble a file from one place "
				       "to another\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Options:\n");
		TEST_FILE_EQ (output, ("      --wobble                "
				       "wobble file while wibbling\n"));
		TEST_FILE_EQ (output, ("  -n, --dry-run               "
				       "simulate and output actions only\n"));
		TEST_FILE_EQ (output, ("  -q, --quiet                 "
				       "reduce output to errors only\n"));
		TEST_FILE_EQ (output, ("  -v, --verbose               "
				       "increase output to include "
				       "informational messages\n"));
		TEST_FILE_EQ (output, ("      --help                  "
				       "display this help and exit\n"));
		TEST_FILE_EQ (output, ("      --version               "
				       "output version information "
				       "and exit\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, ("Takes the file from SRC, wibbles it "
				       "until any loose pieces fall off, and "
				       "until\n"));
		TEST_FILE_EQ (output, ("it reaches DEST.  SRC and DEST may "
				       "not be the same location.\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Report bugs to <foo@bar.com>\n");
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);
	}


	/* Check that --help works if the program name itself is the name
	 * of the command, it should behave as if the real binary were
	 * called with the command as the first argument, except all of the
	 * usage strings, etc. should make sense.
	 */
	TEST_FEATURE ("with command in program_name");
	TEST_ALLOC_FAIL {
		program_name = "wibble";
		argc = 0;
		argv[argc++] = "ignored";
		argv[argc++] = "--help";
		argv[argc] = NULL;

		TEST_CHILD (pid) {
			unsetenv ("COLUMNS");

			TEST_DIVERT_STDOUT (output) {
				nih_command_parser (NULL, argc, argv,
						    options, commands);
				exit (1);
			}
		}

		waitpid (pid, &status, 0);
		rewind (output);

		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_FILE_EQ (output, "Usage: wibble [OPTION]... SRC DEST\n");
		TEST_FILE_EQ (output, ("wibble a file from one place to "
				       "another\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Options:\n");
		TEST_FILE_EQ (output, ("      --wobble                "
				       "wobble file while wibbling\n"));
		TEST_FILE_EQ (output, ("  -n, --dry-run               "
				       "simulate and output actions only\n"));
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
		TEST_FILE_EQ (output, ("Takes the file from SRC, wibbles it "
				       "until any loose pieces fall off, and "
				       "until\n"));
		TEST_FILE_EQ (output, ("it reaches DEST.  SRC and DEST may "
				       "not be the same location.\n"));
		TEST_FILE_EQ (output, "\n");
		TEST_FILE_EQ (output, "Report bugs to <foo@bar.com>\n");
		TEST_FILE_END (output);
	}

	fclose (output);
}


int
main (int   argc,
      char *argv[])
{
	test_parser ();
	test_help ();

	return 0;
}
