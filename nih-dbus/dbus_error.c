/* libnih
 *
 * dbus_error.c - D-Bus error handling
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdarg.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include <nih-dbus/errors.h>

#include "dbus_error.h"


/**
 * nih_dbus_error_raise:
 * @name: D-Bus name for error,
 * @message: Human-readable error message.
 *
 * Raises an error which includes a D-Bus name so that it may be sent as
 * a reply to a method call, the error type is fixed to NIH_DBUS_ERROR.
 *
 * You may use this in D-Bus handlers and return a negative number to
 * automatically have this error returned as the method reply.  It is also
 * useful when mixing D-Bus and libnih function calls in your own methods
 * to return consistent error forms, in which case pass the name and message
 * members of the DBusError structure before freeing it.
 **/
void
nih_dbus_error_raise (const char *name,
		      const char *message)
{
	NihDBusError *err;

	nih_assert (name != NULL);
	nih_assert (message != NULL);

	err = NIH_MUST (nih_new (NULL, NihDBusError));

	err->error.number = NIH_DBUS_ERROR;
	err->name = NIH_MUST (nih_strdup (err, name));
	err->error.message = NIH_MUST (nih_strdup (err, message));

	nih_error_raise_error (&err->error);
}

/**
 * nih_dbus_error_raise_printf:
 * @name: D-Bus name for error,
 * @format: format string for human-readable message.
 *
 * Raises an error which includes a D-Bus name so that it may be sent as
 * a reply to a method call, the error type is fixed to NIH_DBUS_ERROR.
 *
 * The human-readable message for the error is parsed according to @format,
 * and allocated as a child of the error object so that it is freed.
 *
 * You may use this in D-Bus handlers and return a negative number to
 * automatically have this error returned as the method reply.  It is also
 * useful when mixing D-Bus and libnih function calls in your own methods
 * to return consistent error forms, in which case pass the name and message
 * members of the DBusError structure before freeing it.
 **/
void
nih_dbus_error_raise_printf (const char *name,
			     const char *format,
			     ...)
{
	NihDBusError *err;
	va_list       args;

	nih_assert (name != NULL);
	nih_assert (format != NULL);

	err = NIH_MUST (nih_new (NULL, NihDBusError));

	err->error.number = NIH_DBUS_ERROR;

	err->name = NIH_MUST (nih_strdup (err, name));

	va_start (args, format);
	err->error.message = NIH_MUST (nih_vsprintf (err, format, args));
	va_end (args);

	nih_error_raise_error (&err->error);
}
