/* libnih
 *
 * dbus_message.c - D-Bus message handling
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


#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/logging.h>

#include "dbus_message.h"


/* Prototypes for static functions */
static int nih_dbus_message_destroy (NihDBusMessage *msg);


/**
 * nih_dbus_message_new:
 * @parent: parent object for new message,
 * @connection: D-Bus connection to associate with,
 * @message: D-Bus message to encapulsate.
 *
 * Creates a new D-Bus message object allocated using nih_alloc().  This
 * encapsulates both an underlying D-Bus connection and message received
 * on it, referencing both.
 *
 * Objects of this structure are passed to method implementation functions
 * so the original message information may be extracted; if the function
 * is asynchronous, you should take a reference to this structure and pass
 * it when sending the reply or an error.
 *
 * When the message is freed, the references to the connection and message
 * will be dropped, which may disconnect the connection.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned object.  When all parents
 * of the returned object are freed, the returned object will also be
 * freed.
 *
 * Returns: new NihDBusMessage structure, or NULL if insufficient memory.
 **/
NihDBusMessage *
nih_dbus_message_new (const void *    parent,
		      DBusConnection *connection,
		      DBusMessage *   message)
{
	NihDBusMessage *msg;

	nih_assert (connection != NULL);
	nih_assert (message != NULL);

	msg = nih_new (parent, NihDBusMessage);
	if (! msg)
		return NULL;

	msg->connection = connection;
	dbus_connection_ref (msg->connection);

	msg->message = message;
	dbus_message_ref (msg->message);

	nih_alloc_set_destructor (msg, nih_dbus_message_destroy);

	return msg;
}

/**
 * nih_dbus_message_destroy:
 * @msg: message to be destroyed.
 *
 * Unreferences the attached D-Bus message and connection.
 *
 * Returns: zero
 **/
static int
nih_dbus_message_destroy (NihDBusMessage *msg)
{
	nih_assert (msg != NULL);

	dbus_message_unref (msg->message);
	dbus_connection_unref (msg->connection);

	return 0;
}


/**
 * nih_dbus_message_error:
 * @msg: message to reply to,
 * @name: name of D-Bus error to reply with,
 * @format: format string for human-readable message.
 *
 * Replies to an asynchronous D-Bus message @msg with the D-Bus error
 * @name with a human-readable message parsed according to @format.
 *
 * Returns: zero on success, negative value on insufficient memory.
 **/
int
nih_dbus_message_error (NihDBusMessage *msg,
			const char *    name,
			const char *    format,
			...)
{
	DBusMessage *   message;
	va_list         args;
	nih_local char *str = NULL;

	nih_assert (msg != NULL);
	nih_assert (name != NULL);
	nih_assert (format != NULL);

	/* Create the message string */
	va_start (args, format);
	str = nih_vsprintf (NULL, format, args);
	va_end (args);

	if (! str)
		return -1;

	/* And the reply */
	message = dbus_message_new_error (msg->message, name, str);
	if (! message)
		return -1;

	/* Send the error back to the connection the original message
	 * was received from.
	 */
	if (! dbus_connection_send (msg->connection, message, NULL)) {
		dbus_message_unref (message);
		return -1;
	}

	dbus_message_unref (message);

	return 0;
}
