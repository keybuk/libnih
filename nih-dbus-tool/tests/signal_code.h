/* nih-dbus-tool
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

#ifndef NIH_DBUS_TOOL_TESTS_SIGNAL_CODE_H
#define NIH_DBUS_TOOL_TESTS_SIGNAL_CODE_H

#include <nih/macros.h>

#include <dbus/dbus.h>

#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_object.h>


typedef void (*MySignalHandler) (void *data, NihDBusMessage *message,
				 const char *msg);


NIH_BEGIN_EXTERN

int               my_emit_signal   (DBusConnection *connection,
				    const char *origin_path, const char *msg)
	__attribute__ ((warn_unused_result));

DBusHandlerResult my_com_netsplit_Nih_Test_Signal_signal (DBusConnection *connection,
							  DBusMessage *signal,
							  NihDBusProxySignal *proxied);

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_TESTS_SIGNAL_CODE_H */
