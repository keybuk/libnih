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

#ifndef NIH_DBUS_MESSAGE_H
#define NIH_DBUS_MESSAGE_H

#include <nih/macros.h>

#include <dbus/dbus.h>


/**
 * NihDBusMessage:
 * @conn: D-Bus connection message was received on,
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
	DBusConnection *conn;
	DBusMessage    *message;
} NihDBusMessage;


NIH_BEGIN_EXTERN

NihDBusMessage *nih_dbus_message_new   (const void *parent,
					DBusConnection *conn,
					DBusMessage *message)
	__attribute__ ((warn_unused_result));

int             nih_dbus_message_error (NihDBusMessage *msg,
					const char *name,
					const char *format, ...)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_MESSAGE_H */
