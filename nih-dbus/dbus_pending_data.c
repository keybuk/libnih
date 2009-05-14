/* libnih
 *
 * dbus_pending_data.c - D-Bus pending call attached data
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/logging.h>

#include "dbus_pending_data.h"


/* Prototypes for static functions */
static int nih_dbus_pending_data_destroy (NihDBusPendingData *pending_data);


/**
 * nih_dbus_pending_data_new:
 * @parent: parent object for new structure,
 * @conn: D-Bus connection to associate with,
 * @message: D-Bus message to encapulsate.
 *
 * Creates a new D-Bus pending call data object allocated using
 * nih_alloc().  You would then use this as the data pointer of a
 * DBusPendingCall to be passed to the notify function.  The structure
 * contains a reference to the underlying D-Bus connection and details
 * about the handler functions and user data pointer.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned object.  When all parents
 * of the returned object are freed, the returned object will also be
 * freed.
 *
 * Returns: new NihDBusPendingData structure, or NULL if insufficient memory.
 **/
NihDBusPendingData *
nih_dbus_pending_data_new (const void *        parent,
			   DBusConnection *    conn,
			   NihDBusReplyHandler handler,
			   NihDBusErrorHandler error_handler,
			   void *              data)
{
	NihDBusPendingData *pending_data;

	nih_assert (conn != NULL);
	nih_assert (handler != NULL);
	nih_assert (error_handler != NULL);

	pending_data = nih_new (parent, NihDBusPendingData);
	if (! pending_data)
		return NULL;

	pending_data->conn = conn;
	dbus_connection_ref (pending_data->conn);

	pending_data->handler = handler;
	pending_data->error_handler = error_handler;
	pending_data->data = data;

	nih_alloc_set_destructor (pending_data, nih_dbus_pending_data_destroy);

	return pending_data;
}

/**
 * nih_dbus_pending_data_destroy:
 * @pending_data: pending data to be destroyed.
 *
 * Unreferences the attached D-Bus connection.
 *
 * Returns: zero
 **/
static int
nih_dbus_pending_data_destroy (NihDBusPendingData *pending_data)
{
	nih_assert (pending_data != NULL);

	dbus_connection_unref (pending_data->conn);

	return 0;
}
