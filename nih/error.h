/* libnih
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
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

#ifndef NIH_ERROR_H
#define NIH_ERROR_H

#include <nih/macros.h>


/**
 * NihError:
 * @number: numeric identifier,
 * @message: human-readable message.
 *
 * This structure represents an error, defining the error @number for
 * programmers to capture and handle them and a human-readable @message
 * that should be pre-translated.
 *
 * The structure is allocated when an error occurs, and only one structure
 * may exist in one context at a time; when another error is raised, the
 * existing error, if any, is freed.
 *
 * You may also use this structure as the header for more complicated error
 * objects.
 **/
typedef struct nih_error_info {
	int         number;
	const char *message;
} NihError;


/**
 * nih_return_error:
 * @retval: return value for function,
 * @number: numeric identifier,
 * @message: human-readable message.
 *
 * Raises an error with the given details in the current error context,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * Will return from the current function with @retval, which may be left
 * empty to return from a void function.
 **/
#define nih_return_error(retval, number,  message) \
	do { nih_error_raise (number, message); return retval; } while (0)

/**
 * nih_return_system_error:
 * @retval: return value for function.
 *
 * Raises an error with details taken from the current value of errno,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * Will return from the current function with @retval, which may be left
 * empty to return from a void function.
 **/
#define nih_return_system_error(retval) \
	do { nih_error_raise_system (); return retval; } while (0)


NIH_BEGIN_EXTERN

void      nih_error_raise        (int number, const char *message);
void      nih_error_raise_printf (int number, const char *format, ...);
void      nih_error_raise_system (void);
void      nih_error_raise_again  (NihError *error);

NihError *nih_error_get          (void)
	__attribute__ ((warn_unused_result));

void      nih_error_push_context (void);
void      nih_error_pop_context  (void);

NIH_END_EXTERN

#endif /* NIH_ERROR_H */
