/* libnih
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

#ifndef NIH_LOGGING_H
#define NIH_LOGGING_H

/**
 * These functions provide a logging interface for outputting messages
 * at different priorities, and filtering based on them.
 *
 * The output for the logger can be selected using nih_log_set_logger(),
 * where nih_logger_printf() is the default and nih_logger_syslog() another
 * popular alternative.
 *
 * Log messages are output with different macros.
 **/

#include <stdlib.h>

#include <nih/macros.h>


/**
 * NihLogLevel:
 *
 * Severity of log messages, used both to influence formatting of the
 * message and to filter messages below a particular severity.
 **/
typedef enum {
	NIH_LOG_UNKNOWN,
	NIH_LOG_DEBUG,
	NIH_LOG_INFO,
	NIH_LOG_MESSAGE,
	NIH_LOG_WARN,
	NIH_LOG_ERROR,
	NIH_LOG_FATAL
} NihLogLevel;

/**
 * NihLogger:
 * @priority: priority of message,
 * @message: message to log.
 *
 * A logger is a function that receives a formatted message to be logged
 * in whatever manner is appropriate.  The priority of the message is given
 * so that the logger may direct it appropriately, however the function
 * should not discard any messages and instead nih_log_set_priority() used
 * to decide the treshold of logged messages.
 *
 * Returns: zero on success, negative value if the logger was not able
 * to output the message.
 **/
typedef int (*NihLogger) (NihLogLevel priority, const char *message);


/**
 * nih_debug:
 * @format: printf-style format string.
 *
 * Outputs a debugging message, including the name of the function that
 * generated it.  Almost never shown, except when debugging information is
 * required.
 **/
#define nih_debug(format, ...) \
	nih_log_message (NIH_LOG_DEBUG, "%s: " format, \
	                 __FUNCTION__, ##__VA_ARGS__)

/**
 * nih_info:
 * @format: printf-style format string.
 *
 * Outputs a message that is purely informational, usually not shown unless
 * the user wants verbose operation.
 **/
#define nih_info(format, ...) \
	nih_log_message (NIH_LOG_INFO, format, ##__VA_ARGS__)

/**
 * nih_message:
 * @format: printf-style format string.
 *
 * Outputs a message from a non-daemon process that is normally shown unless
 * the user wants quiet operation.  The difference between this and nih_warn()
 * is that this is usually send to standard output, instead of standard
 * error, and it is not prefixed.
 **/
#define nih_message(format, ...) \
	nih_log_message (NIH_LOG_MESSAGE, format, ##__VA_ARGS__)

/**
 * nih_warn:
 * @format: printf-style format string.
 *
 * Outputs a warning message, one that indicates a potential problem that
 * has been ignored; these are shown by default unless the user wants quiet
 * operation.
 **/
#define nih_warn(format, ...) \
	nih_log_message (NIH_LOG_WARN, format, ##__VA_ARGS__)

/**
 * nih_error:
 * @format: printf-style format string.
 *
 * Outputs an error message, one that the software may be able to recover
 * from but that has caused an operation to fail.  These are shown in all
 * but the most quiet of operation modes.
 **/
#define nih_error(format, ...) \
	nih_log_message (NIH_LOG_ERROR, format, ##__VA_ARGS__)

/**
 * nih_fatal:
 * @format: printf-style format string.
 *
 * Outputs a fatal error message that caused the software to cease
 * functioning.  Always shown.
 **/
#define nih_fatal(format, ...) \
	nih_log_message (NIH_LOG_FATAL, format, ##__VA_ARGS__)

/**
 * nih_assert:
 * @expr: expression to check.
 *
 * Outputs a fatal error message and terminates the process if @expr is
 * false.
 **/
#define nih_assert(expr) \
	if (! NIH_LIKELY(expr)) { \
		nih_fatal ("%s:%d: Assertion failed in %s: %s", \
			   __FILE__, __LINE__, __FUNCTION__, #expr); \
		abort (); \
	}

/**
 * nih_assert_not_reached:
 *
 * Outputs a fatal error message and terminates the process if this
 * line of code is reached.
 **/
#define nih_assert_not_reached() \
	do { \
		nih_fatal ("%s:%d: Not reached assertion failed in %s", \
			   __FILE__, __LINE__, __FUNCTION__); \
		abort (); \
	} while (0)


NIH_BEGIN_EXTERN

extern NihLogLevel nih_log_priority;


void nih_log_init         (void);

void nih_log_set_logger   (NihLogger new_logger);
void nih_log_set_priority (NihLogLevel new_priority);

int  nih_log_message      (NihLogLevel priority, const char *format, ...)
	__attribute__ ((format (printf, 2, 3)));

int  nih_logger_printf    (NihLogLevel priority, const char *message);
int  nih_logger_syslog    (NihLogLevel priority, const char *message);

NIH_END_EXTERN

#endif /* NIH_LOGGING_H */
