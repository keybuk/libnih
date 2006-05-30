/* libnih
 *
 * logging.c - message logging
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <nih/main.h>

#include "logging.h"


/**
 * logger:
 *
 * Function used to output log messages.
 **/
static NihLogger logger = NULL;

/**
 * max_priority:
 *
 * Highest priority of log messages that will be given to the logger by
 * default.
 **/
static NihLogLevel max_priority = NIH_LOG_NONE;


/**
 * nih_log_init:
 *
 * Initialise the default logger and priority.
 **/
static void
nih_log_init (void)
{
	logger = nih_logger_printf;
	max_priority = NIH_LOG_WARN;
}

/**
 * nih_log_set_logger:
 * @new_logger: new logger function.
 *
 * Sets the function that will be used to output log messages above the
 * priority set with @nih_log_set_priority.
 **/
void
nih_log_set_logger (NihLogger new_logger)
{
	nih_assert (new_logger != NULL);

	logger = new_logger;
}

/**
 * nih_log_set_priority:
 * @new_priority: new maximum priority.
 *
 * Sets the maximum priority of log messages to be given to the logger
 * function, any messages above this will be discarded.
 **/
void
nih_log_set_priority (NihLogLevel new_priority)
{
	nih_assert (new_priority > NIH_LOG_NONE);

	max_priority = new_priority;
}


/**
 * nih_log_message:
 * @priority: priority of message,
 * @format: printf-style format string.
 *
 * Outputs a message constructed from @format and the rest of the arguments
 * by passing it to the logger function if @priority is not higher than
 * the maximum priority.
 *
 * The message should not be newline-terminated.
 *
 * Returns: zero if successful, positive value if message was discarded due
 * to being above the maximum priority and negative value if the logger failed.
 **/
int
nih_log_message (NihLogLevel  priority,
		 const char  *format, ...)
{
	static char *message = NULL;
	size_t       len;
	va_list      args;

	nih_assert (format != NULL);

	if (! max_priority)
		nih_log_init ();

	if (priority > max_priority)
		return 1;

	va_start (args, format);

	/* Find out how long the string will be */
	len = vsnprintf (NULL, 0, format, args);

	/* Adjust the buffer size to match and make the formatted string */
	message = realloc (message, len + 1);
	vsnprintf (message, len + 1, format, args);

	va_end (args);

	return logger (priority, message);
}

/**
 * nih_logger_printf:
 * @priority: priority of message being logged,
 * @message: message to log.
 *
 * Outputs the @message to standard output, or standard error depending
 * on @priority, prefixed with the program name and terminated with a new
 * line.
 *
 * Returns: zero on completion, negative value on error.
 **/
int
nih_logger_printf (NihLogLevel  priority,
		   const char  *message)
{
	const char *format;
	FILE       *stream;
	size_t      idx;

	nih_assert (message != NULL);

	/* Follow GNU conventions and don't put a space between the program
	 * name and message if the message is of the form "something: message"
	 */
	idx = strcspn (message, " :");
	if (message[idx] == ':') {
		format = "%s:%s\n";
	} else {
		format = "%s: %s\n";
	}

	/* Warnings and errors belong on stderr, information and debug on
	 * stdout
	 */
	if (priority <= NIH_LOG_WARN) {
		stream = stderr;
	} else {
		stream = stdout;
	}

	/* Output it */
	if (fprintf (stream, format, program_name, message) < 0)
		return -1;

	return 0;
}
