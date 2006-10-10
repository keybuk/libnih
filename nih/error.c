/* libnih
 *
 * error.c - error handling
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


#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/logging.h>
#include <nih/string.h>

#include "error.h"


/* Prototypes for static functions */
static void nih_error_clear (void);


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
static inline void
nih_error_init (void)
{
	if (! context_stack) {
		NIH_MUST (context_stack = nih_list_new (NULL));

		nih_error_push_context ();
	}
}


/**
 * nih_error_raise:
 * @number: numeric identifier,
 * @message: human-readable message.
 *
 * Raises an error with the given details in the current error context,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * @message should be a static string, as it will not be freed when the
 * error object is.
 **/
void
nih_error_raise (int         number,
		 const char *message)
{
	NihError *error;

	nih_assert (number > 0);
	nih_assert (message != NULL);

	nih_error_init ();

	NIH_MUST (error = nih_new (CURRENT_CONTEXT, NihError));

	error->number = number;
	error->message = message;

	nih_error_raise_again (error);
}

/**
 * nih_error_raise_printf:
 * @number: numeric identifier,
 * @format: format string for human-readable message.
 *
 * Raises an error with the given details in the current error context,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * The human-readable message for the error is parsed according to @format,
 * and allocated as a child of the error object so that it is freed.
 **/
void
nih_error_raise_printf (int         number,
			const char *format,
			...)
{
	NihError *error;
	va_list   args;

	nih_assert (number > 0);
	nih_assert (format != NULL);

	nih_error_init ();

	NIH_MUST (error = nih_new (CURRENT_CONTEXT, NihError));

	error->number = number;

	va_start (args, format);
	NIH_MUST (error->message = nih_vsprintf (error, format, args));
	va_end (args);

	nih_error_raise_again (error);
}

/**
 * nih_error_raise_system:
 *
 * Raises an error with details taken from the current value of errno,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 **/
void
nih_error_raise_system (void)
{
	NihError *error;
	int       saved_errno;

	nih_assert (errno > 0);
	saved_errno = errno;

	nih_error_init ();

	NIH_MUST (error = nih_new (CURRENT_CONTEXT, NihError));

	error->number = saved_errno;
	NIH_MUST (error->message = nih_strdup (error, strerror (saved_errno)));

	nih_error_raise_again (error);
	errno = saved_errno;
}

/**
 * nih_error_raise_again:
 * @error: existing object to raise.
 *
 * Raises the existing error object in the current error context,
 * if an unhandled error already exists then an error message is emmitted
 * through the logging system; you should try to avoid this.
 *
 * This is normally used to raise a taken error that has not been handled,
 * or to raise a custom error object.
 **/
void
nih_error_raise_again (NihError *error)
{
	nih_assert (error != NULL);
	nih_assert (error->number > 0);
	nih_assert (error->message != NULL);

	nih_error_init ();

	if (CURRENT_CONTEXT->error)
		nih_error_clear ();

	CURRENT_CONTEXT->error = error;
}


/**
 * nih_error_clear:
 *
 * Clear the error from the current context, emitting a log message so that
 * it isn't lost.  In an ideal world, this would be an assertion, except
 * it would assert in the wrong place (a new error, not at the unhandled one).
 **/
static void
nih_error_clear (void)
{
	nih_assert (context_stack != NULL);
	nih_assert (CURRENT_CONTEXT->error != NULL);

	nih_error ("%s: %s", _("Unhandled Error"),
		   CURRENT_CONTEXT->error->message);

	nih_free (CURRENT_CONTEXT->error);
	CURRENT_CONTEXT->error = NULL;
}

/**
 * nih_error_get:
 *
 * Returns the last unhandled error from the current context, clearing it
 * so that further errors may be raised.
 *
 * The object must be freed with nih_free() once you are finished with it,
 * if you want to raise it again, use nih_error_raise_again().
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
	CURRENT_CONTEXT->error = NULL;

	return error;
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

	NIH_MUST (new_context = nih_new (context_stack, NihErrorCtx));

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
	if (context->error)
		nih_error_clear ();

	nih_list_remove (&context->entry);
	nih_free (context);
}
