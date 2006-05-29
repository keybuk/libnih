/* libnih
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

#ifndef NIH_LOGGING_H
#define NIH_LOGGING_H

#include <nih/macros.h>


/**
 * NihLogLevel:
 *
 * Severity of log messages, used both to influence formatting of the
 * message and to filter messages below a particular severity.
 **/
typedef enum {
	NIH_LOG_NONE,
	NIH_LOG_FATAL,
	NIH_LOG_ERROR,
	NIH_LOG_WARN,
	NIH_LOG_INFO,
	NIH_LOG_DEBUG
} NihLogLevel;

/**
 * NihLogger:
 *
 * A logger is a function that receives a formatted message to be logged
 * in whatever manner is appropriate.  The priority of the message is given
 * so that the logger may direct it appropriately, however the function
 * should not discard any messages and instead @nih_log_set_priority used
 * to decide the treshold of logged messages.
 *
 * The logger may return non-zero to indicate that it was not able to
 * output the message.
 **/
typedef int (*NihLogger) (NihLogLevel, const char *);


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
 * nih_info:
 * @format: printf-style format string.
 *
 * Outputs a message that is purely informational, usually not shown unless
 * the user wants extremely verbose operation.
 **/
#define nih_info(format, ...) \
	nih_log_message (NIH_LOG_INFO, format, ##__VA_ARGS__)

/**
 * nih_debug:
 * @format: printf-style format string.
 *
 * Outputs a debugging message, including the name of the function that
 * generated it, if the verbosity is high enough.
 **/
#define nih_debug(format, ...) \
	nih_log_message (NIH_LOG_DEBUG, "%s: " format, \
	                 __FUNCTION__, ##__VA_ARGS__)


NIH_BEGIN_EXTERN

void nih_log_set_logger   (NihLogger new_logger);
void nih_log_set_priority (NihLogLevel new_priority);

int  nih_log_message      (NihLogLevel priority, const char *format, ...)
                          __attribute__ ((format (printf, 2, 3)));

int  nih_logger_printf    (NihLogLevel priority, const char *message);

NIH_END_EXTERN

#endif /* NIH_LOGGING_H */
