/* libnih
 *
 * Copyright © 2011 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2011 Canonical Ltd.
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

#ifndef NIH_ERROR_H
#define NIH_ERROR_H

/**
 * Many functions in libnih use these functions to report information about
 * errors, those that don't use the ordinary errno mechnism which can
 * also be reported within this framework.
 *
 * Errors are raised as NihError structures, kept globally.  Only one error
 * may be active at any one time, raising an error when another is already
 * raised will result in an assertion.
 *
 * Errors are raised with the nih_error_raise() or nih_error_raise_printf()
 * functions, passing the error number and a human-readable messages.
 *
 * System errors can be raised with nih_error_raise_system(), and both
 * caught errors and self-allocated errors can be raised with
 * nih_error_raise_error().
 *
 * You then report the error condition through your return value, or some
 * other stack-based method.
 *
 * A higher function that wishes to handle the error calls nih_error_get()
 * to retrieve it, it's an error to do so if you do not know that an error
 * is pending.  This returns the currently raised error structure.
 *
 * To clear the error, it should be freed with nih_free().  To return the
 * error from your own function, simply don't free it.
 *
 * Errors may be partitioned using contexts, a new context is pushed with
 * nih_error_push_context(); any errors raised are now stored in this
 * context and any previous raised errors are hidden from view.  The context
 * can be popped again with nih_error_pop_context() provided that any raised
 * error has been dealt with.  The previously hidden raised errors are now
 * visible again.
 *
 * To raise an error from one context, into another, you can't simply call
 * nih_error_get() before nih_error_pop_context() since the latter will
 * assert because of the unfreed error.  Instead nih_error_steal() may be
 * used which returns the error as nih_error_get() does but also removes
 * it from the context.
 *
 * nih_error_steal() may also be used to stash errors before trying an
 * alternate code path.
 **/

#include <nih/macros.h>

#include <errno.h>
#include <string.h>


/**
 * NihError:
 * @filename: filename where the error was raised,
 * @line: line number of @filename where the error was raised,
 * @function: function name the error was raised within,
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
 * objects, in which case do not worry about setting @filename, @line or
 * @function since these are set when you call nih_error_raise_error(); the
 * correct way to do this is to place the macro NIH_ERROR_MEMBERS at the
 * front of your structure, rather than an NihError named member - this
 * makes code that uses your custom error a little easier.
 **/
typedef struct nih_error {
#define NIH_ERROR_MEMBERS			\
	const char *filename;			\
	int         line;			\
	const char *function;			\
						\
	int         number;			\
	const char *message;

	NIH_ERROR_MEMBERS
} NihError;


/**
 * nih_error_raise:
 * @number: numeric identifier,
 * @message: human-readable message.
 *
 * Raises an error with the given details in the current error context,
 * if an unhandled error already exists then an error message is emitted
 * through the logging system; you should try to avoid this.
 *
 * @message should be a static string, as it will not be freed when the
 * error object is.
 **/
#define nih_error_raise(number, message) \
	_nih_error_raise (__FILE__, __LINE__, __FUNCTION__, number, message)

/**
 * nih_error_raise_printf:
 * @number: numeric identifier,
 * @format: format string for human-readable message.
 *
 * Raises an error with the given details in the current error context,
 * if an unhandled error already exists then an error message is emitted
 * through the logging system; you should try to avoid this.
 *
 * The human-readable message for the error is parsed according to @format,
 * and allocated as a child of the error object so that it is freed.
 **/
#define nih_error_raise_printf(number, format, ...) \
	_nih_error_raise_printf (__FILE__, __LINE__, __FUNCTION__, \
				 number, format, __VA_ARGS__)

/**
 * nih_error_raise_system:
 *
 * Raises an error with details taken from the current value of errno,
 * if an unhandled error already exists then an error message is emitted
 * through the logging system; you should try to avoid this.
 **/
#define nih_error_raise_system() \
	_nih_error_raise_system (__FILE__, __LINE__, __FUNCTION__)

/**
 * nih_error_raise_no_memory:
 *
 * Raises an ENOMEM system error, if an unhandled error already exists then
 * an error message is emitted through the logging system; you should try
 * to avoid this.
 **/
#define nih_error_raise_no_memory()				\
	_nih_error_raise (__FILE__, __LINE__, __FUNCTION__,	\
			  ENOMEM, strerror (ENOMEM))

/**
 * nih_error_raise_error:
 * @error: existing object to raise.
 *
 * Raises the existing error object in the current error context,
 * if an unhandled error already exists then an error message is emitted
 * through the logging system; you should try to avoid this.
 *
 * This is normally used to raise a taken error that has not been handled,
 * or to raise a custom error object.
 *
 * The destructor of @error will be overwritten so that the context can
 * be cleared when the error is freed.
 **/
#define nih_error_raise_error(error) \
	_nih_error_raise_error (__FILE__, __LINE__, __FUNCTION__, error)


/**
 * nih_return_error:
 * @retval: return value for function,
 * @number: numeric identifier,
 * @message: human-readable message.
 *
 * Raises an error with the given details in the current error context,
 * if an unhandled error already exists then an error message is emitted
 * through the logging system; you should try to avoid this.
 *
 * Will return from the current function with @retval, which may be left
 * empty to return from a void function.
 **/
#define nih_return_error(retval, number,  message)			\
	do {								\
		nih_error_raise (number, message);			\
		return retval;						\
	} while (0)

/**
 * nih_return_system_error:
 * @retval: return value for function.
 *
 * Raises an error with details taken from the current value of errno,
 * if an unhandled error already exists then an error message is emitted
 * through the logging system; you should try to avoid this.
 *
 * Will return from the current function with @retval, which may be left
 * empty to return from a void function.
 **/
#define nih_return_system_error(retval)					\
	do {								\
		nih_error_raise_system ();				\
		return retval;						\
	} while (0)

/**
 * nih_return_no_memory_error:
 * @retval: return value for function.
 *
 * Raises an ENOMEM system error, if an unhandled error already exists then
 * an error message is emitted through the logging system; you should try
 * to avoid this.
 *
 * Will return from the current function with @retval, which may be left
 * empty to return from a void function.
 **/
#define nih_return_no_memory_error(retval)				\
	do {								\
		nih_error_raise_no_memory ();				\
		return retval;						\
	} while (0)


/**
 * NIH_SHOULD:
 * @_e: C expression.
 *
 * Repeats the expression @_e until it either yields a true value, or
 * raises an error other than ENOMEM.
 *
 * This can only be used when the expression always raises an error if
 * it does not yield a true value.
 *
 * The raised error remains raised and should be dealt with following
 * this function, thus you should store the value of the expression so you
 * know whether or not an error occurred.
 *
 * Returns: value of expression @_e which will be evaluated as many times
 * as necessary to become true.
 **/
#define NIH_SHOULD(_e)							\
	({								\
		typeof (_e) __ret;					\
		while (! (__ret = (_e))) {				\
			NihError *_nih_should_err;			\
									\
			_nih_should_err = nih_error_get ();		\
			if (_nih_should_err->number == ENOMEM) {	\
				nih_free (_nih_should_err);		\
			} else {					\
				break;					\
			}						\
		}							\
		__ret;							\
	})


NIH_BEGIN_EXTERN

void      nih_error_init          (void);

void      _nih_error_raise        (const char *filename, int line,
				   const char *function,
				   int number, const char *message);
void      _nih_error_raise_printf (const char *filename, int line,
				   const char *function,
				   int number, const char *format, ...)
	__attribute__ ((format (printf, 5, 6)));
void      _nih_error_raise_system (const char *filename, int line,
				   const char *function);
void      _nih_error_raise_error  (const char *filename, int line,
				   const char *function,
				   NihError *error);

NihError *nih_error_get           (void)
	__attribute__ ((warn_unused_result));
NihError *nih_error_steal         (void)
	__attribute__ ((warn_unused_result));

void      nih_error_push_context  (void);
void      nih_error_pop_context   (void);

NIH_END_EXTERN

#endif /* NIH_ERROR_H */
