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
