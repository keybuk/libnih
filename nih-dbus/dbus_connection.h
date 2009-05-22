/* libnih
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
 *
 * = Allocation =
 *
 * Note that the structures returned by nih_dbus_connect(), nih_dbus_bus()
 * and nih_dbus_server() are NOT allocated with nih_alloc() but instead
 * by the D-Bus library.
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
