/* libnih
 *
 * logging.c - message logging
 *
 * Copyright © 2010 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2010 Canonical Ltd.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/main.h>

#include "logging.h"


/**
 * __abort_msg:
 *
 * A glibc variable that keeps the assertion message in the core dump.
 **/
extern char *__abort_msg __attribute__ ((weak));

/**
 * logger:
 *
 * Function used to output log messages.
 **/
static NihLogger logger = NULL;

/**
 * nih_log_priority:
 *
 * Lowest priority of log messages that will be given to the logger by
 * default.
 **/
NihLogLevel nih_log_priority = NIH_LOG_UNKNOWN;


/**
 * nih_log_init:
 *
 * Initialise the default logger and priority.
 **/
void
nih_log_init (void)
{
	if (! logger)
		logger =  nih_logger_printf;
	if (! nih_log_priority)
		nih_log_priority = NIH_LOG_MESSAGE;
}

/**
 * nih_log_set_logger:
 * @new_logger: new logger function.
 *
 * Sets the function that will be used to output log messages above the
 * priority set with nih_log_set_priority().
 **/
void
nih_log_set_logger (NihLogger new_logger)
{
	nih_assert (new_logger != NULL);

	nih_log_init ();

	logger = new_logger;
}

/**
 * nih_log_set_priority:
 * @new_priority: new minimum priority.
 *
 * Sets the minimum priority of log messages to be given to the logger
 * function, any messages below this will be discarded.
 **/
void
nih_log_set_priority (NihLogLevel new_priority)
{
	nih_assert (new_priority > NIH_LOG_UNKNOWN);

	nih_log_init ();

	nih_log_priority = new_priority;
}


/**
 * nih_log_abort_message:
 * @message: message to be logged.
 *
 * Save @message in the glibc __abort_msg variable so it can be retrieved
 * by debuggers if we should crash at this point.
 **/
static void
nih_log_abort_message (const char *message)
{
	if (! &__abort_msg)
		return;

	if (__abort_msg)
		nih_discard (__abort_msg);

	__abort_msg = NIH_MUST (nih_strdup (NULL, message));
}

/**
 * nih_log_message:
 * @priority: priority of message,
 * @format: printf-style format string.
 *
 * Outputs a message constructed from @format and the rest of the arguments
 * by passing it to the logger function if @priority is not lower than
 * the minimum priority.
 *
 * The message should not be newline-terminated.
 *
 * Returns: zero if successful, positive value if message was discarded due
 * to being below the minimum priority and negative value if the logger failed.
 **/
int
nih_log_message (NihLogLevel priority,
		 const char *format,
		 ...)
{
	nih_local char *message = NULL;
	va_list         args;
	int             ret;

	nih_assert (format != NULL);

	nih_log_init ();

	if (priority < nih_log_priority)
		return 1;

	va_start (args, format);
	message = NIH_MUST (nih_vsprintf (NULL, format, args));
	va_end (args);

	if (priority >= NIH_LOG_FATAL)
		nih_log_abort_message (message);

	/* Output the message */
	ret = logger (priority, message);

	return ret;
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
nih_logger_printf (NihLogLevel priority,
		   const char *message)
{
	nih_assert (message != NULL);

	/* Warnings and errors belong on stderr, and must be prefixed
	 * with the program name.  Information and debug go on stdout and
	 * are not prefixed.
	 */
	if (priority >= NIH_LOG_WARN) {
		const char *format;
		size_t      idx;

		/* Follow GNU conventions and don't put a space between the
		 * program name and message if the message is of the form
		 * "something: message"
		 */
		idx = strcspn (message, " :");
		if (message[idx] == ':') {
			format = "%s:%s\n";
		} else {
			format = "%s: %s\n";
		}

		if (fprintf (stderr, format, program_name, message) < 0)
			return -1;
	} else {
		if (printf ("%s\n", message) < 0)
			return -1;
	}

	return 0;
}

/**
 * nih_logger_syslog:
 * @priority: priority of message being logged,
 * @message: message to log.
 *
 * Outputs the @message to the system logging daemon, it is up to the
 * program to call openlog and set up the parameters for the connection.
 *
 * Returns: zero on completion, negative value on error.
 **/
int
nih_logger_syslog (NihLogLevel priority,
		   const char *message)
{
	int level;

	nih_assert (message != NULL);

	switch (priority) {
	case NIH_LOG_DEBUG:
		level = LOG_DEBUG;
		break;
	case NIH_LOG_INFO:
		level = LOG_INFO;
		break;
	case NIH_LOG_MESSAGE:
		level = LOG_NOTICE;
		break;
	case NIH_LOG_WARN:
		level = LOG_WARNING;
		break;
	case NIH_LOG_ERROR:
		level = LOG_ERR;
		break;
	case NIH_LOG_FATAL:
		level = LOG_CRIT;
		break;
	default:
		level = LOG_NOTICE;
	}

	syslog (level, "%s", message);

	return 0;
}
