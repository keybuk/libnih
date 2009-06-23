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

#ifndef NIH_DBUS_MESSAGE_H
#define NIH_DBUS_MESSAGE_H

/**
 * The NihDBusMessage structure references both an incoming D-Bus message
 * and the connection it was received on, and may be created with the
 * nih_dbus_message_new() function.
 *
 * This allows both a single pointer to be passed around to deal with a
 * message, and also provides an nih_alloc() context for attaching allocated
 * data that can be discarded when the message has been processed (often
 * strings used to generate the reply, for example).
 *
 * A typical function that uses this structure is nih_dbus_message_error()
 * which generates and sends a reply to the incoming message that is an
 * error return with the given name and format.
 **/

#include <nih/macros.h>

#include <dbus/dbus.h>


/**
 * NihDBusMessage:
 * @connection: D-Bus connection message was received on,
 * @message: message object received.
 *
 * This structure is used as a context for the processing of a message; the
 * primary reason for its existance is to be used as an nih_alloc() context
 * for any reply data.
 *
 * Instances are allocated automatically and passed to marshaller functions,
 * and freed on their return.
 **/
typedef struct nih_dbus_message {
	DBusConnection *connection;
	DBusMessage *   message;
} NihDBusMessage;


NIH_BEGIN_EXTERN

NihDBusMessage *nih_dbus_message_new   (const void *parent,
					DBusConnection *connection,
					DBusMessage *message)
	__attribute__ ((warn_unused_result));

int             nih_dbus_message_error (NihDBusMessage *msg,
					const char *name,
					const char *format, ...)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_MESSAGE_H */
