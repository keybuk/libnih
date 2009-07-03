/* libnih
 *
 * dbus_error.c - D-Bus error handling
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

	err->number = NIH_DBUS_ERROR;
	err->name = NIH_MUST (nih_strdup (err, name));
	err->message = NIH_MUST (nih_strdup (err, message));

	nih_error_raise_error ((NihError *)err);
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

	err->number = NIH_DBUS_ERROR;

	err->name = NIH_MUST (nih_strdup (err, name));

	va_start (args, format);
	err->message = NIH_MUST (nih_vsprintf (err, format, args));
	va_end (args);

	nih_error_raise_error ((NihError *)err);
}
