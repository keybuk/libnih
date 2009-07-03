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

#ifndef NIH_DBUS_ERROR_H
#define NIH_DBUS_ERROR_H

/**
 * Errors in libnih are raised by placing an NihError object in a local
 * variable that can be retrieved using nih_error_get().  Errors in libdbus
 * are returned on the stack by passing the address of a DBusError object
 * when calling functions.
 *
 * This module allows the two techniques to be bridged.
 *
 * When calling a libnih function, or writing a handler called by such
 * a function, you may use nih_dbus_error_raise() or
 * nih_dbus_error_raise_printf().  This may be retrieved by nih_error_get(),
 * and the handler poll function will do so and convert this into a D-Bus
 * error message if appropriate.
 *
 * For example:
 *
 *   nih_dbus_error_raise (DBUS_ERROR_INVALID_ARGS, "Expected Int32");
 *   return -1;
 *
 * When calling a libdbus function you should initialise a DBusError with
 * dbus_error_init() and pass its address to your function call.  Should
 * an error be returned, you can raise that so it can be retrieved by
 * nih_error_get() by passing the name and message members to
 * nih_dbus_error_raise().
 *
 * For example:
 *
 *   dbus_error_init (&dbus_err);
 *   if (! dbus_connection_open (address, &dbus_err)) {
 *     nih_dbus_error_raise (dbus_err.name, dbus_err.message);
 *     dbus_error_free (&dbus_err);
 *
 *     return -1;
 *   }
 *
 * In both cases, the error variable returned by nih_error_get() is not
 * actually NihError but NihDBusError.  This extends the original structure
 * to add a "name" member containing the D-Bus error name.  The error
 * number for all such errors is NIH_DBUS_ERROR.
 *
 * The nih_dbus_message_error() function defined in nih-dbus/dbus_message.h
 * allows you to send either type of error as a D-Bus error message.
 **/

#include <nih/macros.h>
#include <nih/error.h>


/**
 * NihDBusError:
 * @name: D-Bus name.
 *
 * This structure builds on NihError to include an additional @name field
 * required for transport across D-Bus.
 *
 * If you receive a NIH_DBUS_ERROR, the returned NihError structure is
 * actually this structure and can be cast to get the additional fields.
 **/
typedef struct nih_dbus_error {
	NIH_ERROR_MEMBERS
	char *   name;
} NihDBusError;


NIH_BEGIN_EXTERN

void nih_dbus_error_raise        (const char *name, const char *message);

void nih_dbus_error_raise_printf (const char *name, const char *format, ...)
	__attribute__ ((format (printf, 2, 3)));

NIH_END_EXTERN

#endif /* NIH_DBUS_ERROR_H */
