/* libnih
 *
 * test_logging.c - test suite for nih/logging.c
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
#include <string.h>
#include <unistd.h>

#include <nih/logging.h>
#include <nih/main.h>


static NihLogLevel last_priority = NIH_LOG_NONE;
static const char *last_message = NULL;

static int
my_logger (NihLogLevel  priority,
	   const char  *message)
{
	last_priority = priority;
	last_message = message;

	if (! strcmp (message, "this should error"))
		return -1;

	return 0;
}

int
test_set_logger (void)
{
	int ret = 0;

	printf ("Testing nih_log_set_logger()\n");
	nih_log_set_priority (NIH_LOG_WARN);
	nih_log_set_logger (my_logger);

	last_priority = NIH_LOG_NONE;

	nih_fatal ("some message");

	/* Logger should have been called */
	if (last_priority != NIH_LOG_FATAL) {
		printf ("BAD: logger was not called.\n");
		ret = 1;
	}

	nih_log_set_logger (nih_logger_printf);

	return ret;
}

int
test_set_priority (void)
{
	int ret = 0;

	printf ("Testing nih_log_set_priority()\n");
	nih_log_set_logger (my_logger);
	nih_log_set_priority (NIH_LOG_DEBUG);

	last_priority = NIH_LOG_NONE;

	nih_debug ("some message");

	/* Logger should have been called */
	if (last_priority != NIH_LOG_DEBUG) {
		printf ("BAD: logger was not called.\n");
		ret = 1;
	}

	nih_log_set_logger (nih_logger_printf);
	nih_log_set_priority (NIH_LOG_WARN);

	return ret;
}

int
test_log_message (void)
{
	int ret = 0, err;

	printf ("Testing nih_log_message()\n");
	nih_log_set_logger (my_logger);

	last_priority = NIH_LOG_NONE;
	last_message = NULL;

	printf ("...with message of low enough priority\n");

	err = nih_log_message (NIH_LOG_FATAL, "message with %s %d formatting",
			       "some", 20);

	/* No error should be returned */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should be called with priority given */
	if (last_priority != NIH_LOG_FATAL) {
		printf ("BAD: logger was not called or priority wrong.\n");
		ret = 1;
	}

	/* Logger should be given formatted message */
	if (strcmp (last_message, "message with some 20 formatting")) {
		printf ("BAD: logger not called with expected message.\n");
		ret = 1;
	}


	printf ("...with message of insufficient priority\n");

	err = nih_log_message (NIH_LOG_DEBUG, "not low enough");

	/* A positive error code should be returned */
	if (err <= 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should not have been called */
	if (strcmp (last_message, "message with some 20 formatting")) {
		printf ("BAD: logger called unexpected.\n");
		ret = 1;
	}


	printf ("...with error code returned from logger\n");

	err = nih_log_message (NIH_LOG_FATAL, "this should error");

	/* Negative error code should be returned */
	if (err >= 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}


	printf ("Testing nih_debug()\n");
	nih_log_set_priority (NIH_LOG_DEBUG);

	err = nih_debug ("%s debugging message", "a");

	/* No error should be returned */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should be called with NIH_LOG_DEBUG */
	if (last_priority != NIH_LOG_DEBUG) {
		printf ("BAD: logger was not called or priority wrong.\n");
		ret = 1;
	}

	/* Message should include function name */
	if (strcmp (last_message, "test_log_message: a debugging message")) {
		printf ("BAD: logger not called with expected message.\n");
		ret = 1;
	}


	printf ("Testing nih_info()\n");

	err = nih_info ("%d formatted %s", 47, "message");

	/* No error should be returned */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should be called with NIH_LOG_INFO */
	if (last_priority != NIH_LOG_INFO) {
		printf ("BAD: logger was not called or priority wrong.\n");
		ret = 1;
	}

	/* Logger should be given formatted message */
	if (strcmp (last_message, "47 formatted message")) {
		printf ("BAD: logger not called with expected message.\n");
		ret = 1;
	}


	printf ("Testing nih_warn()\n");

	err = nih_warn ("%d formatted %s", -2, "text");

	/* No error should be returned */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should be called with NIH_LOG_WARN */
	if (last_priority != NIH_LOG_WARN) {
		printf ("BAD: logger was not called or priority wrong.\n");
		ret = 1;
	}

	/* Logger should be given formatted message */
	if (strcmp (last_message, "-2 formatted text")) {
		printf ("BAD: logger not called with expected message.\n");
		ret = 1;
	}


	printf ("Testing nih_error()\n");

	err = nih_error ("formatted %d %s", 42, "text");

	/* No error should be returned */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should be called with NIH_LOG_ERROR */
	if (last_priority != NIH_LOG_ERROR) {
		printf ("BAD: logger was not called or priority wrong.\n");
		ret = 1;
	}

	/* Logger should be given formatted message */
	if (strcmp (last_message, "formatted 42 text")) {
		printf ("BAD: logger not called with expected message.\n");
		ret = 1;
	}


	printf ("Testing nih_fatal()\n");

	err = nih_fatal ("%s message %d", "formatted", 999);

	/* No error should be returned */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Logger should be called with NIH_LOG_FATAL */
	if (last_priority != NIH_LOG_FATAL) {
		printf ("BAD: logger was not called or priority wrong.\n");
		ret = 1;
	}

	/* Logger should be given formatted message */
	if (strcmp (last_message, "formatted message 999")) {
		printf ("BAD: logger not called with expected message.\n");
		ret = 1;
	}


	nih_log_set_priority (NIH_LOG_WARN);
	nih_log_set_logger (nih_logger_printf);

	return ret;
}

int
test_logger_printf (void)
{
	FILE *output;
	char  text[81];
	int   oldstdout, oldstderr, ret = 0, err;

	printf ("Testing nih_logger_printf()\n");
	program_name = "test";
	nih_log_set_priority (NIH_LOG_DEBUG);

	output = tmpfile ();
	oldstdout = dup (STDOUT_FILENO);
	oldstderr = dup (STDERR_FILENO);

	printf ("...with high-priority message\n");
	dup2 (fileno (output), STDOUT_FILENO);
	err = nih_log_message (NIH_LOG_DEBUG, "message with %s %d formatting",
			       "some", 20);
	dup2 (oldstdout, STDOUT_FILENO);

	rewind (output);

	/* Return value should be zero */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Output should be formatted string with newline and program */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: message with some 20 formatting\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	fflush (output);
	ftruncate (fileno (output), 0);


	printf ("...with low-priority message\n");
	dup2 (fileno (output), STDERR_FILENO);
	err = nih_log_message (NIH_LOG_FATAL, "%s message %d formatted",
			       "error", -1);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be zero */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Output should be formatted string with newline and program */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test: error message -1 formatted\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}

	rewind (output);
	fflush (output);
	ftruncate (fileno (output), 0);


	printf ("...with prefixed message\n");
	dup2 (fileno (output), STDERR_FILENO);
	err = nih_log_message (NIH_LOG_FATAL, "%s:%d: some error or other",
			       "example.txt", 303);
	dup2 (oldstderr, STDERR_FILENO);

	rewind (output);

	/* Return value should be zero */
	if (err) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Output should not have a space between program and message */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "test:example.txt:303: some error or other\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should be no more output */
	if (fgets (text, sizeof (text), output)) {
		printf ("BAD: more output than we expected.\n");
		ret = 1;
	}


	printf ("...with closed stream\n");
	close (STDERR_FILENO);
	err = nih_log_message (NIH_LOG_FATAL, "an error message");
	dup2 (oldstderr, STDERR_FILENO);

	/* Return value should be negative */
	if (err >= 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	fclose (output);
	close (oldstdout);
	close (oldstderr);

	nih_log_set_priority (NIH_LOG_WARN);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_set_logger ();
	ret |= test_set_priority ();
	ret |= test_log_message ();
	ret |= test_logger_printf ();

	return ret;
}
