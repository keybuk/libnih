/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
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

#ifndef NIH_DBUS_CONNECTION_H
#define NIH_DBUS_CONNECTION_H

/**
 * This module provides the necessary glue to hook D-Bus connections and
 * servers up to the libnih main loop.
 *
 * Connections obtained from the libdbus library can be hooked up using
 * the nih_dbus_setup() function.  Once called, data on the underlying
 * socket will be sent and received through NihIo and timeouts will be
 * maintained using NihTimer objects.  Each time through the main loop
 * any unhandled messages will be dispatched.
 *
 * The lifetime of connections is also partially managed by the library.
 * Disconnection from the D-Bus server will result in a disconnect handler
 * function being called, and the connection automatically unreferenced
 * and discarded.  A typically well-behaved application might set a timer
 * to attempt to reconnect to the bus.
 *
 * Convenient functions exist to setup connections to servers at specific
 * addresses and to well known bus daemons: nih_dbus_connect() and
 * nih_dbus_bus().  These are equivalent to calling dbus_connection_open()
 * or dbus_bus_get() followed by nih_dbus_setup().
 *
 * Mostly equivalent, anyway.  nih_dbus_bus() explicitly sets the bus
 * connection to NOT result in the application exiting on disconnection,
 * and instead you should cope in your disconnect handler.
 *
 * = Servers =
 *
 * This module may also be used to create D-Bus servers, which is
 * practically impossible to do without main loop integration.
 * nih_dbus_server() creates a server listening on the given address.
 *
 * A connection handler function is called when a new connection is made
 * to the server.  This function generally registers objects or adds
 * filters to the connection and returns TRUE to hook it up to the main
 * loop with the given disconnct handler.  If the function returns FALSE,
 * it is generally dropped.
 **/

#include <nih/macros.h>

#include <dbus/dbus.h>


/**
 * NihDBusDisconnectHandler:
 * @conn: Connection that was lost.
 *
 * A D-Bus disconnect handler is a function called when the D-Bus connection
 * @conn is disconnected from its server.  Once called, the connection is
 * automatically unreferenced.
 **/
typedef void (*NihDBusDisconnectHandler) (DBusConnection *conn);

/**
 * NihDBusConnectHandler:
 * @server: Server that received new connection,
 * @conn: New connection.
 *
 * A D-Bus connection handler is a function called when the D-Bus @server
 * receives a new connection @conn.  The function must return TRUE for the
 * connection to be accepted, otherwise it will be dropped.
 *
 * Returns: TRUE if connection accepted, FALSE otherwise.
 **/
typedef int (*NihDBusConnectHandler) (DBusServer *server,
				      DBusConnection *conn);


NIH_BEGIN_EXTERN

DBusConnection *nih_dbus_connect (const char *address,
				  NihDBusDisconnectHandler disconnect_handler)
	__attribute__ ((warn_unused_result));

DBusConnection *nih_dbus_bus     (DBusBusType bus,
				  NihDBusDisconnectHandler disconnect_handler)
	__attribute__ ((warn_unused_result));

int             nih_dbus_setup   (DBusConnection *conn,
				  NihDBusDisconnectHandler disconnect_handler)
	__attribute__ ((warn_unused_result));

DBusServer *    nih_dbus_server  (const char *address,
				  NihDBusConnectHandler connect_handler,
				  NihDBusDisconnectHandler disconnect_handler)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_CONNECTION_H */
