/* libnih
 *
 * dbus_pending_data.c - D-Bus pending call attached data
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
 * @connection: D-Bus connection to associate with,
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
			   DBusConnection *    connection,
			   NihDBusReplyHandler handler,
			   NihDBusErrorHandler error_handler,
			   void *              data)
{
	NihDBusPendingData *pending_data;

	nih_assert (connection != NULL);
	nih_assert (error_handler != NULL);

	pending_data = nih_new (parent, NihDBusPendingData);
	if (! pending_data)
		return NULL;

	pending_data->connection = connection;
	dbus_connection_ref (pending_data->connection);

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

	dbus_connection_unref (pending_data->connection);

	return 0;
}
