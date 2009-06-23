/* libnih
 *
 * error.c - error handling
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/logging.h>
#include <nih/string.h>

#include "error.h"


/* Prototypes for static functions */
static void nih_error_clear   (void);
static int  nih_error_destroy (NihError *error);


/**
 * NihErrorCtx:
 * @entry: list header,
 * @error: current error.
 *
 * This structure is used to provide barriers that errors cannot cross,
 * for example when performing an operation after an error has occurred.
 **/
typedef struct nih_error_ctx {
	NihList   entry;
	NihError *error;
} NihErrorCtx;


/**
 * context_stack:
 *
 * Stack of error contexts.
 **/
static __thread NihList *context_stack = NULL;


/**
 * CURRENT_CONTEXT:
 *
 * Macro to obtain the current context.
 **/
#define CURRENT_CONTEXT ((NihErrorCtx *)context_stack->prev)

/**
 * DEFAULT_CONTEXT:
 *
 * Macro to obtain the default context.
 **/
#define DEFAULT_CONTEXT ((NihErrorCtx *)context_stack->next)


/**
 * nih_error_init:
 *
 * Initialise the context stack.
 **/
void
nih_error_init (void)
{
	if (! context_stack) {
		context_stack = NIH_MUST (nih_list_new (NULL));

		nih_error_push_context ();

		nih_assert (atexit (nih_error_clear) == 0);
	}
}


/**
 * _nih_error_raise:
 * @filename: filename where the error was raised,
 * @line: line number of @filename where the error was raised,
 * @function: function name the error was raised within,
 * @number: numeric identifier,
 * @message: human-readable message.
 *
 * Raises an error with the given details in the current error context,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * @message should be a static string, as it will not be freed when the
 * error object is.
 *
 * This function should never be called directly, instead use the
 * nih_error_raise() macro to pass the correct arguments for @filename,
 * @line and @function.
 **/
void
_nih_error_raise (const char *filename,
		  int         line,
		  const char *function,
		  int         number,
		  const char *message)
{
	NihError *error;

	nih_assert (filename != NULL);
	nih_assert (line > 0);
	nih_assert (function != NULL);
	nih_assert (number > 0);
	nih_assert (message != NULL);

	nih_error_init ();

	error = NIH_MUST (nih_new (NULL, NihError));

	error->number = number;
	error->message = message;

	_nih_error_raise_error (filename, line, function, error);
}

/**
 * _nih_error_raise_printf:
 * @filename: filename where the error was raised,
 * @line: line number of @filename where the error was raised,
 * @function: function name the error was raised within,
 * @number: numeric identifier,
 * @format: format string for human-readable message.
 *
 * Raises an error with the given details in the current error context,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * The human-readable message for the error is parsed according to @format,
 * and allocated as a child of the error object so that it is freed.
 *
 * This function should never be called directly, instead use the
 * nih_error_raise_printf() macro to pass the correct arguments for @filename,
 * @line and @function.
 **/
void
_nih_error_raise_printf (const char *filename,
			 int         line,
			 const char *function,
			 int         number,
			 const char *format,
			 ...)
{
	NihError *error;
	va_list   args;

	nih_assert (filename != NULL);
	nih_assert (line > 0);
	nih_assert (function != NULL);
	nih_assert (number > 0);
	nih_assert (format != NULL);

	nih_error_init ();

	error = NIH_MUST (nih_new (NULL, NihError));

	error->number = number;

	va_start (args, format);
	error->message = NIH_MUST (nih_vsprintf (error, format, args));
	va_end (args);

	_nih_error_raise_error (filename, line, function, error);
}

/**
 * _nih_error_raise_system:
 * @filename: filename where the error was raised,
 * @line: line number of @filename where the error was raised,
 * @function: function name the error was raised within.
 *
 * Raises an error with details taken from the current value of errno,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * This function should never be called directly, instead use the
 * nih_error_raise_system() macro to pass the correct arguments for @filename,
 * @line and @function.
 **/
void
_nih_error_raise_system (const char *filename,
			 int         line,
			 const char *function)
{
	NihError *error;
	int       saved_errno;

	nih_assert (filename != NULL);
	nih_assert (line > 0);
	nih_assert (function != NULL);
	nih_assert (errno > 0);
	saved_errno = errno;

	nih_error_init ();

	error = NIH_MUST (nih_new (NULL, NihError));

	error->number = saved_errno;
	error->message = NIH_MUST (nih_strdup (error, strerror (saved_errno)));

	_nih_error_raise_error (filename, line, function, error);
	errno = saved_errno;
}

/**
 * _nih_error_raise_error:
 * @filename: filename where the error was raised,
 * @line: line number of @filename where the error was raised,
 * @function: function name the error was raised within,
 * @error: existing object to raise.
 *
 * Raises the existing error object in the current error context,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * This is normally used to raise a taken error that has not been handled,
 * or to raise a custom error object.
 *
 * The destructor of @error will be overwritten so that the context can
 * be cleared when the error is freed.
 *
 * This function should never be called directly, instead use the
 * nih_error_raise_error() macro to pass the correct arguments for @filename,
 * @line and @function.
 **/
void
_nih_error_raise_error (const char *filename,
			int         line,
			const char *function,
			NihError *  error)
{
	nih_assert (filename != NULL);
	nih_assert (line > 0);
	nih_assert (function != NULL);
	nih_assert (error != NULL);
	nih_assert (error->number > 0);
	nih_assert (error->message != NULL);

	nih_error_init ();
	nih_error_clear ();

	error->filename = filename;
	error->line = line;
	error->function = function;

	CURRENT_CONTEXT->error = error;

	nih_alloc_set_destructor (error, nih_error_destroy);
}


/**
 * nih_error_clear:
 *
 * Ensure that the current context has no raised error, if it does then
 * there's a programming error so we abort after logging where the error
 * was originally raised.
 **/
static void
nih_error_clear (void)
{
	nih_assert (context_stack != NULL);

	if (! NIH_UNLIKELY (CURRENT_CONTEXT->error))
		return;

	nih_fatal ("%s:%d: Unhandled error from %s: %s",
		   CURRENT_CONTEXT->error->filename,
		   CURRENT_CONTEXT->error->line,
		   CURRENT_CONTEXT->error->function,
		   CURRENT_CONTEXT->error->message);
	abort ();
}


/**
 * nih_error_get:
 *
 * Returns the last unhandled error from the current context.
 *
 * The object must be freed with nih_free() once you are finished with it,
 * otherwise the error will still considered to be raised.
 *
 * Returns: error object from current context.
 **/
NihError *
nih_error_get (void)
{
	NihError *error;

	nih_assert (context_stack != NULL);
	nih_assert (CURRENT_CONTEXT->error != NULL);

	error = CURRENT_CONTEXT->error;

	return error;
}

/**
 * nih_error_steal:
 *
 * Returns the last unhandled error from the current context, and removes
 * it from the error context.  To re-raise, it must be given to
 * nih_error_raise_error().
 *
 * Returns: error object from current context.
 **/
NihError *
nih_error_steal (void)
{
	NihError *error;

	nih_assert (context_stack != NULL);
	nih_assert (CURRENT_CONTEXT->error != NULL);

	error = CURRENT_CONTEXT->error;
	CURRENT_CONTEXT->error = NULL;

	nih_alloc_set_destructor (error, NULL);

	return error;
}


/**
 * nih_error_destroy:
 * @error: error being freed.
 *
 * This is the destructor function for errors, attached when the error
 * is connected to the context.  It ensures that the error is removed from
 * the context when it is freed.
 *
 * Returns: always zero.
 **/
static int
nih_error_destroy (NihError *error)
{
	nih_assert (error != NULL);
	nih_assert (context_stack != NULL);
	nih_assert (CURRENT_CONTEXT->error != NULL);
	nih_assert (CURRENT_CONTEXT->error == error);

	CURRENT_CONTEXT->error = NULL;

	return 0;
}



/**
 * nih_error_push_context:
 *
 * Creates a new context in which errors can occur without disturbing any
 * previous unhandled error, useful for touring a particular piece of
 * processing that handles its own errors and may be triggered as a result
 * of another error.
 **/
void
nih_error_push_context (void)
{
	NihErrorCtx *new_context;

	nih_error_init ();

	new_context = NIH_MUST (nih_new (context_stack, NihErrorCtx));

	nih_list_init (&new_context->entry);
	new_context->error = NULL;

	nih_list_add (context_stack, &new_context->entry);
}

/**
 * nih_error_pop_context:
 *
 * Ends the last context created with nih_error_push_context(), deliberate
 * care should be taken so that these are always properly nested (through
 * the correct use of scope, for example) and contexts are not left unpopped.
 **/
void
nih_error_pop_context (void)
{
	NihErrorCtx *context;

	nih_assert (context_stack != NULL);
	nih_assert (CURRENT_CONTEXT != DEFAULT_CONTEXT);

	context = CURRENT_CONTEXT;
	nih_error_clear ();

	nih_list_remove (&context->entry);
	nih_free (context);
}
