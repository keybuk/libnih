/* libnih
 *
 * dbus_message.c - D-Bus message handling
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
#include <nih/string.h>
#include <nih/logging.h>

#include "dbus_message.h"


/* Prototypes for static functions */
static int nih_dbus_message_destroy (NihDBusMessage *msg);


/**
 * nih_dbus_message_new:
 * @parent: parent object for new message,
 * @conn: D-Bus connection to associate with,
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
		      DBusConnection *conn,
		      DBusMessage *   message)
{
	NihDBusMessage *msg;

	nih_assert (conn != NULL);
	nih_assert (message != NULL);

	msg = nih_new (parent, NihDBusMessage);
	if (! msg)
		return NULL;

	msg->conn = conn;
	dbus_connection_ref (msg->conn);

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
	dbus_connection_unref (msg->conn);

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
	if (! dbus_connection_send (msg->conn, message, NULL)) {
		dbus_message_unref (message);
		return -1;
	}

	dbus_message_unref (message);

	return 0;
}
